/*
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <net/if.h>
#include <net/ethernet.h>
#include <asm/types.h>
#include <linux/nl80211.h>
#define _LINUX_IF_H /* Avoid redefinition of stuff */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

/*
struct ucred {
    __u32   pid;
    __u32   uid;
    __u32   gid;
};
*/

#include <ieee80211_external.h>
#include <cfg80211_external.h>
#include "wlanif_cfg80211.h"

#ifdef BUILD_YOCTO
#define __bool_already_defined__
#include "cdp_txrx_stats_struct.h"
#endif

#include <ol_ath_ucfg.h>

#define _LINUX_TYPES_H
#include <wlan_dfs_ioctl.h>

#ifdef SON_MEMORY_DEBUG

#include "qca-son-mem-debug.h"
#undef QCA_MOD_INPUT
#define QCA_MOD_INPUT QCA_MOD_LIBWIFISONCFG
#include "son-mem-debug.h"

#endif /* SON_MEMORY_DEBUG */

#include <wlan_son_ioctl.h>

#define TRACE_ENTRY() dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s: Enter \n",__func__)
#define TRACE_EXIT() dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s: Exit \n",__func__)
#define TRACE_EXIT_ERR() dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: Exit with err %d\n",__func__,ret)

/* OL Radio xml parse using the shift and PARAM_BAND_INFO is fixed for cmd */
#define RADIO_PARAM_SHIFT 4096
#define RADIO_PARAM_BAND_INFO 399
#define RADIO_PARAM_FALLBACK_FREQ 444
#define OL_ATH_PARAM_BAND_INFO  (RADIO_PARAM_SHIFT + RADIO_PARAM_BAND_INFO)
#define OL_ATH_PARAM_NXT_RDR_FREQ  (RADIO_PARAM_SHIFT + RADIO_PARAM_FALLBACK_FREQ)
#define LIST_STA_MAX_CFG80211_LENGTH (3500)

#define QCA_WLAN_VENDOR_ATTR_CONFIG_PHY_MODE 62
#define QCA_WLAN_VENDOR_ATTR_CONFIG_CHANNEL_WIDTH 63
static char *fragmentDataHandle = NULL;
static int totalFragmentLen = 0;

#if QCA_AIRTIME_FAIRNESS
struct atftable *atfInfo = NULL;
#endif

/**
 * @brief Function to send the cfg command or ioctl command.
 *
 * @param [in] radar  pointer to radarhandler
 * @param [in] ifname   interface name
 * @param [in] buf   buffer
 * @param [in] buflen   buffer length
 *
 * @return 0 for success, -1 for failure
 */
int radar_send_command (struct radarhandler *radar, const char *ifname, void *buf, size_t buflen,
                        int ioctl_sock_fd)
{
    struct cfg80211_data buffer;
    int nl_cmd = QCA_NL80211_VENDOR_SUBCMD_PHYERR;
    int msg;
    wifi_cfg80211_context pcfg80211_sock_ctx;

    pcfg80211_sock_ctx = radar->sock_ctx.cfg80211_ctxt;
    buffer.data = buf;
    buffer.length = buflen;
    buffer.callback = NULL;
    buffer.parse_data = 0;
    msg = wifi_cfg80211_sendcmd(&pcfg80211_sock_ctx, nl_cmd, ifname,
                                (char *)&buffer, buflen);
    if (msg < 0) {
        fprintf(stderr, "Couldn't send NL command\n");
        return -1;
    }

    return 0;
}

/**
 * @brief Function to Handle bangradar commands.
 *
 * @param [in] radar  pointer to radar handler
 *
 * @return 0 for success, -1 for failure
 */
int radarBangradar(struct radarhandler *radar,int32_t freq_offset, int32_t bang_radar_type)
{
    struct dfs_bangradar_params pe;
    pe.bangradar_type = bang_radar_type;
    pe.seg_id = 0;
    pe.is_chirp = 0;
    pe.freq_offset = freq_offset;
    pe.detector_id = 0;

    radar->atd.ad_id = DFS_BANGRADAR | ATH_DIAG_IN;
    radar->atd.ad_out_data = NULL;
    radar->atd.ad_out_size = 0;
    radar->atd.ad_in_data = (void *) &pe;
    radar->atd.ad_in_size = sizeof(struct dfs_bangradar_params);
    if (radar_send_command(radar, radar->atd.ad_name, (caddr_t)&radar->atd,
                           sizeof(struct ath_diag), radar->s) < 0) {
        radar->atd.ad_in_data = NULL;
        return -1;
    }
    radar->atd.ad_in_data = NULL;
    return 0;
}

/**
 * @brief Initialize the context
 *
 * @param [in] sock_ctx socket context
 * @param [in] cmd_sock_id, event_sock_id: If application can run as background
 *                               process/daemon then use unique port numbers
 *                               otherwise default socket id for simple applications.
 *
 * @return 0 for success, -1 for failure
 */
int init_socket_context (struct socket_context *sock_ctx,
        int cmd_sock_id, int event_sock_id)
{
    int err = 0;

    sock_ctx->cfg80211_ctxt.pvt_cmd_sock_id = cmd_sock_id;
    sock_ctx->cfg80211_ctxt.pvt_event_sock_id = event_sock_id;

    err = wifi_init_nl80211(&(sock_ctx->cfg80211_ctxt));
    if (err) {
        return -1;
    }

    return 0;
}

void cfg82011_station_info_cb(struct cfg80211_data *buffer)
{
    int offset = totalFragmentLen;
    totalFragmentLen += buffer->length;

    if (buffer->length) {
        char *temp = fragmentDataHandle;
        fragmentDataHandle = (char *)realloc(fragmentDataHandle, totalFragmentLen+1);
        if (!fragmentDataHandle) {
            dbgf(soncfgDbgS.dbgModule, DBGERR, " %s: Memory re-allocation Failed\n",__func__);
            free(temp);
            return;
        }
        memcpy((fragmentDataHandle + offset), (char *)buffer->data, buffer->length);
    }

    /*
     * when application requires multiple callbacks from driver, restore
     * buffer->length to max expected length to make sure libqca_nl80211 library
     * sends valid data to the sub sequent call backs.
     */
    buffer->length = LIST_STA_MAX_CFG80211_LENGTH;
}

/* nl handler for IW based ioctl*/
static int wdev_info_handler(struct nl_msg *msg, void *arg)
{
    struct nlattr *nl_msg[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct wdev_info *info = arg;

    nla_parse(nl_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

    if (nl_msg[NL80211_ATTR_IFTYPE])
    {
        info->nlmode = nla_get_u32(nl_msg[NL80211_ATTR_IFTYPE]);
    } else {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "NL80211_ATTR_IFTYPE not found\n");
        return -EINVAL;
    }

    if(nl_msg[NL80211_ATTR_IFNAME])
    {
        memcpy(info->name, nla_data(nl_msg[NL80211_ATTR_IFNAME]), nla_len(nl_msg[NL80211_ATTR_IFNAME]));
        info->name[nla_len(nl_msg[NL80211_ATTR_IFNAME])] = '\0';
    } else {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "NL80211_ATTR_IFNAME not found\n");
        return -EINVAL;
    }

#ifdef WIFISON_SUPPORT_11BE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0))
    if(nl_msg[NL80211_ATTR_MLD_ADDR])
    {
        memcpy(info->mldaddr.ether_addr_octet, nla_data(nl_msg[NL80211_ATTR_MLD_ADDR]), nla_len(nl_msg[NL80211_ATTR_MLD_ADDR]));
    } else {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "NL80211_ATTR_MLD_ADDR not found\n");
        return -EINVAL;
    }
#else
    if(nl_msg[NL80211_ATTR_MLD_MAC])
    {
        memcpy(info->mldaddr.ether_addr_octet, nla_data(nl_msg[NL80211_ATTR_MLD_MAC]), nla_len(nl_msg[NL80211_ATTR_MLD_MAC]));
    } else {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "NL80211_ATTR_MLD_MAC not found\n");
        return -EINVAL;
    }
#endif
#endif
    return NL_SKIP;
}

/*allocate and send nlmsg to handle IW based ioctl*/
int send_nlmsg_wdev_info ( const char *ifname, wifi_cfg80211_context *cfgCtx, struct wdev_info *dev_info)
{
    struct nl_msg *nlmsg;
    struct nl_cb *cb;
    int ret, err;

    nlmsg = nlmsg_alloc();
    if (!nlmsg) {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "ERROR: Failed to allocate netlink message for msg.\n");
        return -ENOMEM;
    }

    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "ERROR: Failed to allocate netlink callbacks.\n");
        nlmsg_free(nlmsg);
        return -ENOMEM;
    }

    /* Prepare nlmsg get the Interface attributes */
    genlmsg_put(nlmsg, 0, 0, cfgCtx->nl80211_family_id , 0, 0, NL80211_CMD_GET_INTERFACE, 0);
    nla_put_u32(nlmsg, NL80211_ATTR_IFINDEX, if_nametoindex(ifname));

    err = 1;

    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM,wdev_info_handler , dev_info);

    /* send message */
    ret = nl_send_auto_complete(cfgCtx->cmd_sock, nlmsg);
    if (ret < 0) {
        goto out;
    }

    /*   wait for reply */
    while (err > 0) {  /* error will be set by callbacks */
        ret = nl_recvmsgs(cfgCtx->cmd_sock, cb);
        if (ret) {
            dbgf(soncfgDbgS.dbgModule, DBGERR, "nl80211: %s->nl_recvmsgs failed: %d\n", __func__, ret);
        }
    }

out:
    if (cb) {
        nl_cb_put(cb);
    }
    if (nlmsg) {
        nlmsg_free(nlmsg);
    }
    return err;
}

//cfg80211 command to get param from driver
int send_command_get_cfg80211( wifi_cfg80211_context *cfgCtx, const char *ifname, int op, int *data)
{
    int ret;
    struct cfg80211_data buffer;
    buffer.data = data;
    buffer.length = sizeof(int);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;
    if((ret=wifi_cfg80211_send_getparam_command(cfgCtx, QCA_NL80211_VENDOR_SUBCMD_WIFI_PARAMS, op, ifname, (char *)&buffer, sizeof(int))) < 0)
    {
        return -EIO;
    }
    return 0;
}

/*cfg80211 command to set param in driver*/
int send_command_set_cfg80211(wifi_cfg80211_context *cfgCtx, const char *ifname, int op, int *data, int data_len)
{
    int ret;
    struct cfg80211_data buffer;
    buffer.data = data;
    buffer.length = sizeof(int);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;
    if((ret=wifi_cfg80211_send_setparam_command(cfgCtx, QCA_NL80211_VENDOR_SUBCMD_WIFI_PARAMS, op, ifname, (char *)&buffer, sizeof(int), 0)) < 0)
    {
        return -EIO;
    }
    return 0;
}

int send_mesh_get_cfg80211( wifi_cfg80211_context *cfgCtx, const char *ifname, int op, int *data)
{
    int ret;
    struct cfg80211_data buffer;
    buffer.data = data;
    buffer.length = sizeof(int);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;
    if((ret=wifi_cfg80211_send_getparam_command(cfgCtx, QCA_NL80211_VENDOR_SUBCMD_MESH_CONFIGURATION, op, ifname, (char *)&buffer, sizeof(int))) < 0)
    {
        return -EIO;
    }
    return 0;
}

/*cfg80211 command to set param in driver*/
int send_mesh_set_cfg80211(wifi_cfg80211_context *cfgCtx, const char *ifname, int op, int *data, int data_len)
{
    int ret;
    struct cfg80211_data buffer;
    buffer.data = data;
    buffer.length = data_len;
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;
    if((ret=wifi_cfg80211_send_setparam_command(cfgCtx, QCA_NL80211_VENDOR_SUBCMD_MESH_CONFIGURATION, op, ifname, (char *)&buffer, sizeof(int), 0)) < 0)
    {
        return -EIO;
    }
    return 0;
}

#if QCA_AIRTIME_FAIRNESS
static void get_atf_table(struct cfg80211_data *buffer)
{
    static uint32_t length = 0;

    length += buffer->length;

    if (length >= sizeof(struct atftable)) {
        length = 0;
    }
}

static void get_vendor_atf_table(struct cfg80211_data *buffer)
{
    static uint32_t length = 0;
    u_int8_t *atfInfoPtr = NULL;
    u_int8_t *temp = NULL;
    uint32_t resp_length = 0;

    struct nlattr *attr_vendor[NL80211_ATTR_MAX_INTERNAL];

    if ( (!buffer) || (!buffer->nl_vendordata) || (!atfInfo) ) {
        return;
    }
    else {
        atfInfoPtr = (u_int8_t *)atfInfo;
    }

    nla_parse(attr_vendor, QCA_WLAN_VENDOR_ATTR_PARAM_MAX, buffer->nl_vendordata, buffer->nl_vendordata_len, NULL);

    if (attr_vendor[QCA_WLAN_VENDOR_ATTR_PARAM_DATA]) {
         /* memcpy tb_vendor to data */
         temp = nla_data(attr_vendor[QCA_WLAN_VENDOR_ATTR_PARAM_DATA]);
         resp_length = nla_get_u32(attr_vendor[QCA_WLAN_VENDOR_ATTR_PARAM_LENGTH]);
    }

    if (temp && resp_length) {
        memcpy(atfInfoPtr + length, temp, resp_length);
    }
    length += resp_length;

    if (length >= sizeof(struct atftable)) {
        length = 0;
    }

}
#endif

/* Cfg80211 command to send genric commands to driver*/
int send_generic_command_cfg80211(wifi_cfg80211_context *cfgCtx, const char *ifname, int cmd, char *data, int data_len)
{
    int res;
    struct cfg80211_data buffer;
    buffer.data = (void *)data;
    buffer.length = data_len;
    buffer.callback = NULL;
#if QCA_AIRTIME_FAIRNESS
    if (cmd == QCA_NL80211_VENDOR_SUBCMD_ATF)
    {
        struct atf_data *atfdata = (struct atf_data *)data;
        if ( (atfdata->id_type == IEEE80211_IOCTL_ATF_SHOWATFTBL)
                || (atfdata->id_type == IEEE80211_IOCTL_ATF_GET_STATS) ) {
            buffer.callback = &get_vendor_atf_table;
            buffer.parse_data = 1;
        }
        else {
            buffer.callback = &get_atf_table;
            buffer.parse_data = 0;
        }
    }
#endif
    res = wifi_cfg80211_send_generic_command(cfgCtx, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, cmd, ifname, MLO_INVALID_LINK_ID, (char *)&buffer, data_len);
    if (res < 0) {
        dbgf(soncfgDbgS.dbgModule, DBGERR, " %s : send NL command failed cmd:%d \n",__func__, cmd);
        return res;
    }

    return 0;
}

/* Cfg80211 command to send genric commands to driver*/
int send_generic_command_with_param_cfg80211(wifi_cfg80211_context *cfgCtx, const char *ifname, int cmd, char *data, int data_len, u_int8_t linkId, int param)
{
    int res;
    struct cfg80211_data buffer;
    buffer.data = (void *)data;
    buffer.length = data_len;
    buffer.callback = NULL;
    buffer.flags = 0;
    struct optional_params opt = {0};

    res = wifi_cfg80211_send_setparam_command_optional_params_linkid(cfgCtx,
                    cmd, param, ifname, linkId, (char *)&buffer, data_len, buffer.flags, &opt);

    if (res < 0) {
        dbgf(soncfgDbgS.dbgModule, DBGERR, " %s : send NL command failed cmd:%d \n",__func__, cmd);
        return res;
    }

    return 0;
}

/* Function to get name of the dev */
int getName_cfg80211(void *ctx, const char * ifname, char *name )
{

    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct wdev_info devinfo = {0};

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_nlmsg_wdev_info(ifname, &(cfgPriv->cfg80211_ctx_qca), &devinfo)) < 0) {
        goto err;
    }

    strlcpy( name, devinfo.name, IFNAMSIZ );

    TRACE_EXIT();

    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to get name of the dev */
int getMLDAddr_cfg80211(void *ctx, const char * ifname, struct ether_addr *mldAddr )
{

    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct wdev_info devinfo = {0};

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_nlmsg_wdev_info(ifname, &(cfgPriv->cfg80211_ctx_qca), &devinfo)) < 0) {
        goto err;
    }

    memcpy( mldAddr->ether_addr_octet, devinfo.mldaddr.ether_addr_octet, ETH_ALEN);

    TRACE_EXIT();

    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int getPhyStats_cfg80211(void * ctx , const char *ifname, void *data, uint32_t len)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);
    if(( ret = send_generic_command_cfg80211 (&(cfgPriv->cfg80211_ctx_qca), ifname, QCA_NL80211_VENDOR_SUBCMD_PHYSTATS, data, len)) < 0 )
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;
err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to check whether the current device is AP */
int isAP_cfg80211(void *ctx, const char * ifname, uint32_t *result)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct wdev_info devinfo = {0};

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_nlmsg_wdev_info(ifname, &(cfgPriv->cfg80211_ctx_qca), &devinfo)) < 0) {
        goto err;
    }

    *result = ( devinfo.nlmode == NL80211_IFTYPE_AP ? 1 : 0 );

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

static struct nla_policy
wlan_cfg80211_get_wireless_mode_policy[QCA_WLAN_VENDOR_ATTR_PARAM_MAX + 1] = {

  [QCA_WLAN_VENDOR_ATTR_PARAM_LENGTH] = {.type = NLA_U32 },
  [QCA_WLAN_VENDOR_ATTR_PARAM_DATA] = {.type = NLA_STRING },
};

void cfg82011_wificonfiguration_cb(struct cfg80211_data *buffer)
{
    struct nlattr *attr_vendor[QCA_WLAN_VENDOR_ATTR_PARAM_MAX + 1];

    nla_parse(attr_vendor, QCA_WLAN_VENDOR_ATTR_PARAM_MAX,
            (struct nlattr *)buffer->data,
            buffer->length, wlan_cfg80211_get_wireless_mode_policy);
}

/* Function to get BSSID address */
int getBSSID_cfg80211(void *ctx, const char * ifname, struct ether_addr *BSSID )
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    struct cfg80211_data buffer;
    buffer.data = BSSID;
    buffer.length = IEEE80211_ADDR_LEN;
    buffer.parse_data = 0;
    buffer.callback = &cfg82011_wificonfiguration_cb;
    if((ret=wifi_cfg80211_send_getparam_command(&(cfgPriv->cfg80211_ctx_qca), QCA_NL80211_VENDORSUBCMD_BSSID, 0, ifname, (char *)&buffer, IEEE80211_ADDR_LEN)) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Funtion to get ESSID info */
int getESSID_cfg80211(void *ctx, const char * ifname, void *buf, uint32_t *len )
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    memset(buf, 0, (IEEE80211_NWID_LEN + 1));
    buffer.data = (void *)buf;
    buffer.length = IEEE80211_NWID_LEN;
    buffer.parse_data = 0; /* Enable callback */
    buffer.callback = &cfg82011_wificonfiguration_cb;

    if((ret=wifi_cfg80211_send_getparam_command(&(cfgPriv->cfg80211_ctx_qca),
                    QCA_NL80211_VENDORSUBCMD_GET_SSID, 0,
                    ifname, (char *)&buffer, IEEE80211_NWID_LEN)) < 0)
    {
        goto err;
    }
    *len = strlen((char *)buf);

    TRACE_EXIT();
    return 0;
err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to get frequency info*/
/* Devinfo command fails if the interface is not up, so using the vendor command to get frequency */
int getFreq_cfg80211(void *ctx, const char * ifname, int32_t * freq)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, IEEE80211_PARAM_GET_FREQUENCY, freq)) < 0) {
        goto err;
    }

    *freq = (*freq * 100000);

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int getStaSignal_cfg80211(void *ctx, const char * ifname, int *rssi)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, IEEE80211_PARAM_GET_SIGNAL_LEVEL, rssi)) < 0) {
        goto err;
    }
    if(*rssi >= 64)
        *rssi -= 0x100;

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int getAcsMinDwellTime_cfg80211(void *ctx, const char * ifname, int32_t *mindwelltime)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, IEEE80211_PARAM_MIN_DWELL_ACS_REPORT, mindwelltime)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int getPP_cfg80211(void *ctx, const char *ifname, int *puncture_pattern) {
    int ret = -1;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();
    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

#if defined WIFISON_SUPPORT_11BE && defined WIFISON_MODULE_PUNCTURE
    if ((ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, IEEE80211_PARAM_PUNCTURE_BITMAP, puncture_pattern)) < 0) {
        goto err;
    }
#else
    *puncture_pattern = 0;
    goto err;
#endif

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to get wireless extension */
int getRange_cfg80211(void *ctx, const char *ifname, int *we_version)
{
    TRACE_ENTRY();

    /*
     * Since this cfg path does not use wireless extension feature,
     * assigning a constant that always passes.
     */
    *we_version = WIRELESS_EXT;

    //TRACE_EXIT();
    return 0;
}

/* Function to get channel width*/
int getChannelWidth_cfg80211(void *ctx, const char * ifname, int * chwidth)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, IEEE80211_PARAM_CHWIDTH, chwidth)) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to get channel extoffset */
int getChannelExtOffset_cfg80211(void *ctx, const char * ifname, int * choffset)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, IEEE80211_PARAM_CHEXTOFFSET, choffset)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;

}

/*Function to get ACS info*/
int getAcsState_cfg80211(void *ctx, const char * ifname, int * acsstate)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, IEEE80211_PARAM_GET_ACS, acsstate)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to get CAC info*/
int getCacState_cfg80211(void *ctx, const char * ifname, int * cacstate)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, IEEE80211_PARAM_GET_CAC, cacstate)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to get ParentIndex info*/
int getParentIfindex_cfg80211(void *ctx, const char * ifname, int * parentIndex)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, IEEE80211_PARAM_PARENT_IFINDEX, parentIndex)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/*Function to get smart monitor info*/
int getSmartMonitor_cfg80211(void *ctx, const char * ifname, int * smartmonitor)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, IEEE80211_PARAM_RX_FILTER_SMART_MONITOR, smartmonitor)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/*Function to get channel bandwidth*/
int getChannelBandwidth_cfg80211(void *ctx, const char * ifname, int * bandwidth)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, IEEE80211_PARAM_BANDWIDTH, bandwidth)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;

}

/* Function to get Atf related generic info*/
int getGenericInfoAtf_cfg80211(void *ctx, const char * ifname, int cmd ,void * chanInfo, int chanInfoSize)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

#if QCA_AIRTIME_FAIRNESS
    if ( (cmd == IEEE80211_IOCTL_ATF_SHOWATFTBL) || (cmd == IEEE80211_IOCTL_ATF_GET_STATS) ){
        struct atf_data atfdata;
        memset(&atfdata, 0, sizeof(atfdata));

        atfdata.id_type = cmd;
        atfdata.buf = chanInfo;
        atfdata.len = chanInfoSize;

        atfInfo = malloc(sizeof(struct atftable));
        if (atfInfo == NULL) {
            ret = -EIO;
            goto err;
        }

        ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, QCA_NL80211_VENDOR_SUBCMD_ATF, (void *)&atfdata, chanInfoSize);
        memcpy(chanInfo, atfInfo, sizeof(struct atftable));

        // Releasing the ATF memory
        free(atfInfo);
        atfInfo=NULL;
    }
    else
#endif
    {
        ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, QCA_NL80211_VENDOR_SUBCMD_ATF, chanInfo, chanInfoSize);
    }

    if (ret < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to get Ald related generic info*/
int getGenericInfoAld_cfg80211(void *ctx, const char * ifname,void * chanInfo, int chanInfoSize)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, QCA_NL80211_VENDOR_SUBCMD_ALD_PARAMS, chanInfo, chanInfoSize)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to hmwds related generic info*/
int getGenericInfoHmwds_cfg80211(void *ctx, const char * ifname,void * chanInfo, int chanInfoSize)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, QCA_NL80211_VENDOR_SUBCMD_HMWDS_PARAMS, chanInfo, chanInfoSize)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to remove ast entry related generic info*/
int getGenericInfoAstRemove_cfg80211(void *ctx, const char * ifname,void * chanInfo, int chanInfoSize)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, QCA_NL80211_VENDOR_SUBCMD_DELETE_WDS_AST, chanInfo, chanInfoSize)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to Nac related generic info*/
int getGenericNac_cfg80211(void *ctx, const char * ifname,void * config, int configSize)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, QCA_NL80211_VENDOR_SUBCMD_NAC, config, configSize)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to get centre frequency*/
int getCfreq2_cfg80211(void *ctx, const char * ifname, int32_t * cfreq2)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, IEEE80211_PARAM_SECOND_CENTER_FREQ,cfreq2)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to get channel utilization */
int getChUtil_cfg80211(void *ctx, const char * ifname, int32_t * chutil)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, IEEE80211_PARAM_CHAN_UTIL,chutil)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}


/* Function to setparam in the driver*/
int setParam_cfg80211(void *ctx, const char *ifname, int cmd, void *data, uint32_t len)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret = send_command_set_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, cmd, data, len)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to mesh setparam in the driver*/
int setParam_mesh_cfg80211(void *ctx, const char *ifname, int cmd, void *data, uint32_t len)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret = send_mesh_set_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, cmd, data, len)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/*Functin to set maccmd, special case in argument handling*/
int setParamMaccmd_cfg80211(void *ctx, const char *ifname, void *data, uint32_t len)
{
    int ret,temp[2];
    struct wlanif_cfg80211_priv * cfgPriv;
    memcpy(temp, data, len);

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret = send_command_set_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, temp[0], &temp[1], sizeof(temp[1]))) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;

}

/*Functin to set mapvapbeacon, special case in argument handling*/
int setMapVapBeacon_cfg80211(void *ctx, const char *ifname, void *data, uint32_t len)
{
    int ret,temp[2];
    struct wlanif_cfg80211_priv * cfgPriv;
    memcpy(temp, data, len);

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret = send_mesh_set_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, temp[0], &temp[1], sizeof(temp[1]))) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;

}

/* Function to get channel info*/
int getChannelInfo_cfg80211(void *ctx, const char * ifname, void * chanInfo, int chanInfoSize)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, QCA_NL80211_VENDOR_SUBCMD_LIST_CHAN, chanInfo, chanInfoSize)) < 0) {
        goto err;
    }


    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;

}

/* Function to get channel info160*/
int getChannelInfo160_cfg80211(void *ctx, const char * ifname, void * chanInfo, int chanInfoSize)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, QCA_NL80211_VENDOR_SUBCMD_LIST_CHAN160, chanInfo, chanInfoSize)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;

}

/* Function to get station info*/
int getStationInfo_cfg80211(void * ctx , const char *ifname, void **data , int * data_len)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    int subCmd = QCA_NL80211_VENDOR_SUBCMD_LIST_STA;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    fragmentDataHandle = (char *)malloc(LIST_STA_MAX_CFG80211_LENGTH);
    char *cfgResBuf = (char *)malloc(LIST_STA_MAX_CFG80211_LENGTH);
    if(!cfgResBuf) {
        if (fragmentDataHandle) {
            free(fragmentDataHandle);
            fragmentDataHandle = NULL;
        }
        dbgf(soncfgDbgS.dbgModule, DBGERR, " %s : malloc failed  \n", __func__);
        return -ENOMEM;
    }
    buffer.data = (void *)cfgResBuf;
    buffer.length = LIST_STA_MAX_CFG80211_LENGTH;
    buffer.parse_data = 0; /* Enable callback */
    buffer.flags = 0;
    buffer.callback = &cfg82011_station_info_cb;

    ret = wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca), QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, subCmd, ifname, MLO_INVALID_LINK_ID, (char *)&buffer, *data_len);
    if (ret < 0) {
        dbgf(soncfgDbgS.dbgModule, DBGERR, " %s : send NL command failed \n",__func__);
        if (fragmentDataHandle)
        {
            free(fragmentDataHandle);
            fragmentDataHandle = NULL;
        }
        if (cfgResBuf) {
            free(cfgResBuf);
            cfgResBuf = NULL;
        }
        goto err;
    }

    *data = fragmentDataHandle;
    fragmentDataHandle=NULL;
    *data_len = totalFragmentLen;
    totalFragmentLen = 0;
    free(cfgResBuf);

    TRACE_EXIT();
    return 0;
err:
    TRACE_EXIT_ERR();
    free(cfgResBuf);
    return ret;
}

/* Function to get Mesh info*/
int getMeshreq_cfg80211(void * ctx , const char *ifname, void *data , uint32_t data_len)
{

	int ret;
	struct wlanif_cfg80211_priv * cfgPriv;
        struct mesh_dbg_req_t *mesh_req;

	TRACE_ENTRY();

	cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
	assert(cfgPriv != NULL);

	mesh_req = (struct mesh_dbg_req_t *) data;
	assert(mesh_req != NULL);

	if (mesh_req->mesh_cmd == MESH_BSTEERING_GET_DATARATE_INFO) {
		mesh_req->needs_reply = DBGREQ_REPLY_IS_REQUIRED;
	}
	if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, data, data_len)) < 0)
	{
		goto err;
	}
	TRACE_EXIT();
	return 0;
err:
	TRACE_EXIT_ERR();
	return ret;
}

/* Function to get dbreq info*/
int getDbgreq_cfg80211(void * ctx , const char *ifname, void *data , uint32_t data_len)
{

	int ret;
	struct wlanif_cfg80211_priv * cfgPriv;
	struct ieee80211req_athdbg * req;

	TRACE_ENTRY();

	cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
	assert(cfgPriv != NULL);

	req = (struct ieee80211req_athdbg *) data;
	assert(req != NULL);

	if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, QCA_NL80211_VENDOR_SUBCMD_DBGREQ, data, data_len) < 0))
	{
		goto err;
	}
	TRACE_EXIT();
	return 0;
err:
	TRACE_EXIT_ERR();
	return ret;
}

/*Funtion to get extended subcommands */
int getExtended_cfg80211(void * ctx , const char *ifname, void *data , uint32_t data_len)
{

    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, QCA_NL80211_VENDOR_SUBCMD_EXTENDEDSTATS, data, data_len) < 0))
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;
err:
    TRACE_EXIT_ERR();
    return ret;
}

int getIfaceStats_cfg80211(void * ctx , const char *ifname, void *data , uint32_t data_len)
{

    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, QCA_NL80211_VENDOR_SUBCMD_80211STATS, data, data_len)) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;
err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Funtion to get station stats*/
int getStaStats_cfg80211(void * ctx , const char *ifname, void *data , uint32_t data_len)
{

    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca),ifname, QCA_NL80211_VENDOR_SUBCMD_STA_STATS, data, data_len)) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;
err:
    TRACE_EXIT_ERR();
    return ret;
}

/*Funtion to handle Add/Del/Kick Mac commands*/
int addDelKickMAC_cfg80211(void * ctx , const char *ifname, int operation, void *data, uint32_t len)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    int cfg_id=-1;
    char *ptr = (char *)data;

    TRACE_ENTRY();

    /* TODO: Check for proper handling */
    /* set len to 6 bytes to be coherent with the driver changes */
    len = ETH_ALEN;
    ptr = ptr + 2; /* Move 2 bytes (sa_family) to get the mac starting address */
    data = (void *)ptr;

    switch (operation)
    {
        case IO_OPERATION_ADDMAC:
            cfg_id = QCA_NL80211_VENDORSUBCMD_ADDMAC;
            break;
        case IO_OPERATION_DELMAC:
            cfg_id = QCA_NL80211_VENDORSUBCMD_DELMAC;
            break;
        case IO_OPERATION_KICKMAC:
            cfg_id = QCA_NL80211_VENDORSUBCMD_KICKMAC;
            break;
       case IO_OPERATION_DEAUTH:
            cfg_id = QCA_NL80211_VENDOR_SUBCMD_DEAUTHMAC;
            break;
        default:
            /*Unsupported operation*/
            return -1;
    }

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if(( ret = send_generic_command_cfg80211 (&(cfgPriv->cfg80211_ctx_qca), ifname, cfg_id, data, len)) < 0 )
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;
err:
    TRACE_EXIT_ERR();
    return ret;
}

/*Function to set filter command */
int setFilter_cfg80211(void * ctx , const char *ifname, void *data, uint32_t len)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);
    if(( ret = send_generic_command_cfg80211 (&(cfgPriv->cfg80211_ctx_qca), ifname, QCA_NL80211_VENDORSUBCMD_SETFILTER, data, len)) < 0 )
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;
err:
    TRACE_EXIT_ERR();
    return ret;
}

/*Function to get Wireless mode from driver*/
int getWirelessMode_cfg80211(void * ctx , const char *ifname, void *data, uint32_t len)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    buffer.data = (void *)data;
    buffer.length = len;
    buffer.parse_data = 0; /* Enable callback */
    buffer.callback = &cfg82011_wificonfiguration_cb;

    ret = wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca), QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_CONFIGURATION, QCA_NL80211_VENDORSUBCMD_WIRELESS_MODE, ifname, MLO_INVALID_LINK_ID, (char *)&buffer, len);

    if (ret < 0) {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s :  send NL command failed - error %d\n",__func__,ret);
        goto err;
    }

    TRACE_EXIT();
    return 0;
err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to send mgmt packet*/
int sendMgmt_cfg80211(void * ctx , const char *ifname, void *data, uint32_t len)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);
    if(( ret = send_generic_command_cfg80211 (&(cfgPriv->cfg80211_ctx_qca), ifname, QCA_NL80211_VENDORSUBCMD_SEND_MGMT, data, len)) < 0 )
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;
err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to get station count */
int getStaCount_cfg80211(void *ctx, const char * ifname, int32_t * result)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, IEEE80211_PARAM_STA_COUNT, result)) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;
err:
    TRACE_EXIT_ERR();
    return ret;
}

/// @see setIntfMode(const void *, const char *, const char *, u_int8_t);
int setIntfMode_cfg80211(void *ctx, const char * ifname, const char * mode, u_int8_t len)
{
    int ret;
    struct cfg80211_data buffer;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    buffer.data = (void *)mode;
    buffer.length = len;
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;
    buffer.flags = 0;
    if((ret=wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca),
                    QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION,
                    QCA_NL80211_VENDORSUBCMD_WIRELESS_MODE,
                    ifname, MLO_INVALID_LINK_ID, (char *)&buffer, len)) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to get Band Info for the given radio interface*/
int getBandInfo_cfg80211(void *ctx, const char * ifname, uint8_t * band_info)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    int bandType;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, OL_ATH_PARAM_BAND_INFO, &bandType)) < 0)
    {
        goto err;
    }
    *band_info = (uint8_t)bandType;

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/**
 * @brief Enable group atf
 *
 * @param [in] iface   ifname to enable group atf
 * @param [in] val   one to enable group, zero to disable group
 *
 * @return int   zero - success, non-zero - failure
 */
int atfGetGroupValue(void *ctx, const char *iface, uint8_t *group_val) {
    int val = 0;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (!iface || !group_val) {
        return -1;
    }
    *group_val = 0;

#if QCA_AIRTIME_FAIRNESS
    if(send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), iface, IEEE80211_PARAM_ATF_SSID_GROUP, &val) < 0) {
        return -1;
    }

    *group_val = val;
#endif
    return 0;
}

/**
 * @brief Get Link ID
 *
 * @param [in] iface   ifname to get Link ID
 * @param [out] linkID   Link ID
 *
 * @return int   zero - success, non-zero - failure
 */
int getMLDLinkID_cfg80211(void *ctx, const char *iface, uint8_t *linkID) {
#ifdef WLAN_FEATURE_11BE_MLO
    int val = 0;
#endif
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (!iface || !linkID) {
        return -1;
    }
    *linkID = 0;

#ifdef WLAN_FEATURE_11BE_MLO
    if(send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), iface, IEEE80211_PARAM_IEEE_LINK_ID, &val) < 0) {
        return -1;
    }

    *linkID = val;
#endif
    return 0;
}

/* Function to get fallback for the given radio interface*/
int getFallbackFreq_cfg80211(void *ctx, const char * ifname, int * fallbackFreq)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    int fallbackFreqLocal;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, OL_ATH_PARAM_NXT_RDR_FREQ,
          &fallbackFreqLocal)) < 0)
    {
        TRACE_EXIT_ERR();
        return ret;
    }
    *fallbackFreq = fallbackFreqLocal;

    TRACE_EXIT();
    return 0;

}

/* Function to set fallback for the given radio interface*/
int setFallbackFreq_cfg80211(void *ctx, const char * ifname, int fallbackFreq)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = send_command_set_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, OL_ATH_PARAM_NXT_RDR_FREQ,
          &fallbackFreq, sizeof(int))) < 0)
    {
        TRACE_EXIT_ERR();
        return ret;
    }

    TRACE_EXIT();
    return 0;

}

/* Function to get number of active vdev count on a Radio */
int getActiveVdevCount_cfg80211(void *ctx, const char *ifname, int *vdev_count)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = send_mesh_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, MESH_PARAM_ACTIVE_VDEV_COUNT, vdev_count)) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to get MixedBh_uplink rate */
int getUplinkRate_cfg80211(void *ctx, const char * ifname, uint16_t * ul_rate)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    int upLinkRate;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = send_mesh_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, MESH_WHC_MIXEDBH_ULRATE, &upLinkRate)) < 0)
    {
        goto err;
    }

    *ul_rate = (uint16_t)upLinkRate;

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to set Map Version */
int setMapVersion_cfg80211(void * ctx, const char *ifname, uint8_t map)
{
    int ret;

    TRACE_ENTRY();

    if (( ret = setParam_mesh_cfg80211(ctx, ifname, MESH_PARAM_MAP, &map, sizeof(map))) < 0)
    {
        return ret;
    }

    TRACE_EXIT();
    return 0;
}

/* Function to set MapBssType */
int setMapBssType_cfg80211(void * ctx, const char *ifname, uint8_t mapBssType)
{
    int ret;

    TRACE_ENTRY();

    if (( ret = setParam_mesh_cfg80211(ctx, ifname, MESH_MAP_BSS_TYPE, &mapBssType, sizeof(mapBssType))) < 0)
    {
        return ret;
    }

    TRACE_EXIT();
    return 0;
}

/* Function to set MixedBh_uplink rate */
int setUplinkRate_cfg80211(void * ctx, const char *ifname, uint16_t ul_rate)
{
    int ret;
    int upLinkRate;

    upLinkRate = (int)ul_rate;

    TRACE_ENTRY();

    if (( ret = setParam_mesh_cfg80211(ctx, ifname, MESH_WHC_MIXEDBH_ULRATE, &upLinkRate, sizeof(upLinkRate))) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to Get Version Compatibility Information */
int getVersionCompatibility_cfg80211(void *ctx, const char * ifname, void *version_info, int data_len)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    char version_data[MESH_VERSION_STR_MAX_LEN];
    int *comp_status = (int*)version_data;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    memset(version_data, 0, sizeof(version_data));
    strlcpy(version_data, version_info, sizeof(version_data));

    /* set operation is used because version information can be passed to driver module
     * and reply data from driver is interpreted as version compatibility result */
    if (( ret = send_mesh_set_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, MESH_VERSION_COMPATIBILITY_CHECK, (int *)version_data, data_len)) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return *comp_status;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to set the son mode and BackHaul_type */
int setSonBhType_cfg80211(void * ctx, const char *ifname, uint8_t bh_type)
{
    int ret;
    int bhType;

    bhType = (int)bh_type;

    TRACE_ENTRY();

    if (( ret = setParam_mesh_cfg80211(ctx, ifname, MESH_WHC_BACKHAUL_TYPE, &bhType, sizeof(bhType))) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to set the son enabled  */
int setParamSon_cfg80211(void * ctx, const char *ifname, uint8_t paramSon)
{
    int ret;


    TRACE_ENTRY();

    if (( ret = setParam_mesh_cfg80211(ctx, ifname, MESH_PARAM_SON, &paramSon, sizeof(paramSon))) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int setParamVapInd_cfg80211(void *ctx, const char *ifname, void *data, uint32_t len)
{
    int ret,temp[2];
    struct wlanif_cfg80211_priv * cfgPriv;
    memcpy(temp, data, len);

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret = send_command_set_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, temp[0], &temp[1], sizeof(temp[1]))) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int setCSA_cfg80211(void *ctx, const char * ifname, int freq, int band, int chwidth, uint8_t cntrChan, int pp)
{
    int ret;
    uint8_t tbtt = 5;
    struct cfg80211_data buffer;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct optional_params opt = {0};

    TRACE_ENTRY();
    memset(&buffer, 0, sizeof(struct cfg80211_data));

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    buffer.data = &tbtt;
    buffer.length = sizeof(uint8_t);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;

    opt.num_params = 2;
    opt.attr_num[0] = QCA_WLAN_VENDOR_ATTR_CONFIG_PHY_MODE;
    opt.attr_value[0] = cntrChan;
    opt.attr_num[1] = QCA_WLAN_VENDOR_ATTR_CONFIG_CHANNEL_WIDTH;
    opt.attr_value[1] = pp;

    if((ret=wifi_cfg80211_send_setparam_command_optional_params(&(cfgPriv->cfg80211_ctx_qca),
                    QCA_NL80211_VENDORSUBCMD_CHAN_WIDTHSWITCH, freq, ifname, (char *)&buffer,
                    chwidth, band, &opt)) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int setFreq_cfg80211(void *ctx, const char * ifname, int freq, int band, int pp, int cntrChanFreq)
{
    int ret, freq2 = cntrChanFreq;
    struct cfg80211_data buffer;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();
    memset(&buffer, 0, sizeof(struct cfg80211_data));

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    buffer.data = &freq2;
    buffer.length = sizeof(int);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;
    if((ret=wifi_cfg80211_send_setparam_command(&(cfgPriv->cfg80211_ctx_qca), QCA_NL80211_VENDORSUBCMD_CHANNEL_CONFIG, freq, ifname, (char *)&buffer, band, pp)) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int setNOLChannel_cfg80211(void *ctx, const char * radioName, int32_t freq_offset, int32_t bang_radar_type)
{
    struct radarhandler radar;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);


    memset(&radar, 0, sizeof(radar));
    radar.sock_ctx.cfg80211_ctxt = cfgPriv->cfg80211_ctx_qca;

    strlcpy(radar.atd.ad_name, radioName, sizeof(radar.atd.ad_name));

    if(radarBangradar(&radar, freq_offset, bang_radar_type) < 0) {
        return -1;
    }

    return 0;
}

int getNOLChannel_cfg80211(void *ctx, const char * radioName, void* nolinfo)
{
    struct radarhandler radar;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    memset(&radar, 0, sizeof(radar));
    radar.sock_ctx.cfg80211_ctxt = cfgPriv->cfg80211_ctx_qca;

    strlcpy(radar.atd.ad_name, radioName, sizeof(radar.atd.ad_name));

    radar.atd.ad_id = DFS_GET_NOL | ATH_DIAG_DYN;
    radar.atd.ad_in_data = NULL;
    radar.atd.ad_in_size = 0;
    radar.atd.ad_out_data = nolinfo;
    radar.atd.ad_out_size = sizeof(struct dfsreq_nolinfo);

    if (radar_send_command(&radar, radar.atd.ad_name,
                (caddr_t)&radar.atd, sizeof(struct ath_diag),
                radar.s) < 0) {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: failed to get nol list %s",__func__, radar.atd.ad_name);
        return -1;
    }

    return 0;
}


/// @see setCFreq2(const void *, const char *, int)
int setCFreq2_cfg80211(void *ctx, const char * ifname, int chan_num)
{
    int ret;

    TRACE_ENTRY();

    if (( ret = setParam_cfg80211(ctx, ifname, IEEE80211_PARAM_SECOND_CENTER_FREQ, &chan_num, sizeof(chan_num))) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}


/// @see getPrivArgs(const void *, const char *, size_t *)
struct iw_priv_args *getPrivArgs_cfg80211(void *ctxt, const char *ifname, size_t *len) {
    TRACE_ENTRY();

    TRACE_EXIT();
    return NULL;
}

/// @see getACSReport(void *, const char *, u_int8_t *, ieee80211_acs_report_t[],
///                   u_int8_t *, ieee80211_neighbor_info_t[], u_int8_t[])
int getACSReport_cfg80211(void *ctx, const char *ifName, u_int8_t* numChans,
                          ieee80211_acs_report_t *chanData, u_int8_t* numNeighbors,
                          ieee80211_neighbor_info_t *neighborData, u_int8_t neighborChans[]) {
    struct ieee80211req_athdbg req;
    struct ieee80211_acs_dbg *acs = NULL;
    struct cfg80211_data buffer;
    struct wlanif_cfg80211_priv * cfgPriv;
    int ret=0;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;

    acs = (struct ieee80211_acs_dbg *)calloc(1, sizeof(struct ieee80211_acs_dbg));

    if(!acs) {
        ret = -ENOMEM;;
        TRACE_EXIT_ERR();
        goto cleanup;
    }

    req.cmd = IEEE80211_DBGREQ_GETACSREPORT;
    req.needs_reply = DBGREQ_REPLY_IS_REQUIRED;
    req.data.acs_rep.data_addr = acs;
    req.data.acs_rep.data_size = sizeof(struct ieee80211_acs_dbg);
    req.data.acs_rep.index = 0;
    acs->entry_id = 0;
    acs->acs_type = ACS_CHAN_STATS;

    buffer.data = &req;
    buffer.length = sizeof(req);
    buffer.callback = NULL;
    buffer.parse_data = 0;

    ret = wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca),
            QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION,
            QCA_NL80211_VENDOR_SUBCMD_DBGREQ, ifName, MLO_INVALID_LINK_ID, (char *)&buffer, buffer.length);
    if (ret < 0) {
        TRACE_EXIT_ERR();
        goto cleanup;
    }

    // Loop through each channel data and get the neighbors
    u_int8_t idxChan = 0, idxNeighbor = 0;
    for (idxChan = 0; idxChan < *numChans && idxChan < acs->nchans; ++idxChan) {
        acs->entry_id = idxChan;
        req.cmd = IEEE80211_DBGREQ_GETACSREPORT;

        ret = wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca),
              QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION,
              QCA_NL80211_VENDOR_SUBCMD_DBGREQ, ifName, MLO_INVALID_LINK_ID, (char *)&buffer, buffer.length);
        if (ret < 0) {
            TRACE_EXIT_ERR();
            goto cleanup;
        }

        // Copy the channel data
        memcpy(&chanData[idxChan], acs, sizeof(struct ieee80211_acs_dbg));
    }

    // Append the neighbor data
    u_int8_t i,j;
    for (i = 0; i < acs->nchans; i++) {
        acs->entry_id = i;
        acs->acs_type = ACS_NEIGHBOUR_GET_LIST_COUNT;
        req.cmd = IEEE80211_DBGREQ_GETACSREPORT;
        req.needs_reply = DBGREQ_REPLY_IS_NOT_REQUIRED;
        ret = wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca),
                QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION,
                QCA_NL80211_VENDOR_SUBCMD_DBGREQ, ifName, MLO_INVALID_LINK_ID, (char *)&buffer, buffer.length);

        if (ret < 0) {
            TRACE_EXIT_ERR();
            goto cleanup;
        }

        if (acs->chan_nbss) {
            acs->neighbor_list = (void *) calloc (acs->chan_nbss,sizeof(ieee80211_neighbor_info));

            if(!acs->neighbor_list) {
                ret = -ENOMEM;
                TRACE_EXIT_ERR();
                goto cleanup;
            }

            acs->neighbor_size = sizeof(ieee80211_neighbor_info) * acs->chan_nbss;
            acs->entry_id = i;
            acs->acs_type = ACS_NEIGHBOUR_GET_LIST;
            req.cmd = IEEE80211_DBGREQ_GETACSREPORT;
            req.needs_reply = DBGREQ_REPLY_IS_NOT_REQUIRED;
            ret = wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca),
                    QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION,
                    QCA_NL80211_VENDOR_SUBCMD_DBGREQ, ifName, MLO_INVALID_LINK_ID, (char *)&buffer, buffer.length);

            if (ret < 0) {
                TRACE_EXIT_ERR();
                goto cleanup;
            }

            for (j = 0; j < acs->chan_nbss && idxNeighbor < *numNeighbors; j++) {
                memcpy(&neighborData[idxNeighbor], &acs->neighbor_list[j], sizeof(ieee80211_neighbor_info));
                neighborChans[idxNeighbor] = acs->ieee_chan;
                idxNeighbor++;
            }
            free(acs->neighbor_list);
            acs->neighbor_list = NULL;
        }
    }

    *numChans = idxChan;
    *numNeighbors = idxNeighbor;
    TRACE_EXIT();

cleanup:
    if (acs) {
        if (acs->neighbor_list) {
            free(acs->neighbor_list);
            acs->neighbor_list = NULL;
        }
        free(acs);
        acs = NULL;
    }

    return ret;
}

/* Function to get Country Code*/
int getCountryCode_cfg80211(void *ctx, const char *ifname, size_t size, char *countryCode) {
    int ret;
    struct wlanif_cfg80211_priv *cfgPriv;
    char country[4] = {0};

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *)ctx;
    assert(cfgPriv != NULL);

    struct cfg80211_data buffer;
    buffer.data = (void *)country;
    buffer.length = sizeof(country);
    buffer.parse_data = 0;
    buffer.callback = &cfg82011_wificonfiguration_cb;
    if (( ret = wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca),
                                           QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_CONFIGURATION,
                                           QCA_NL80211_VENDORSUBCMD_COUNTRY_CONFIG, ifname,
                                           MLO_INVALID_LINK_ID, (char *) &buffer, sizeof(country))) < 0)
    {
        goto err;
    }

    memcpy(countryCode, country, size);
    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int getNoiseFloor_cfg80211(void *ctx, const char *ifname, int16_t *noise) {

    int ret;
    struct wlanif_cfg80211_priv *cfgPriv;
    int16_t nf = 0;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *)ctx;
    assert(cfgPriv != NULL);

    struct cfg80211_data buffer;
    buffer.data = (void *)&nf;
    buffer.length = sizeof(nf);
    buffer.parse_data = 0;
    buffer.callback = &cfg82011_wificonfiguration_cb;
    if (( ret = wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca),
                                           QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_CONFIGURATION,
                                           QCA_NL80211_VENDOR_SUBCMD_GET_NOISE_FLOOR, ifname,
                                           MLO_INVALID_LINK_ID, (char *) &buffer, sizeof(nf))) < 0)
    {
        goto err;
    }
    *noise = nf;
    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int getFullRadioMetrics_cfg80211(void *ctx, const char * ifname, u_int32_t *allMetrics)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    int status = 0;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname,
                     (OL_ATH_PARAM_ALL_CHAN_UTIL | ATH_PARAM_SHIFT), &status)) != 0)
    {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s : %s couldn't get all_status", __func__, ifname);
        goto err;
    }
    *allMetrics = (u_int32_t) status;
    dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s : %s full metrics  value = %d", __func__, ifname, status);

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int getTxChanUtil_cfg80211(void *ctx, const char * ifname, u_int8_t *txUtil)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    int chanUtil = 0;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, (OL_ATH_PARAM_CHAN_AP_TX_UTIL | ATH_PARAM_SHIFT), &chanUtil)) != 0)
    {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s : %s couldn't get tx", __func__, ifname);
        goto err;
    }
    *txUtil = (u_int8_t) chanUtil;
    dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s : %s tx util value = %d", __func__, ifname, chanUtil);

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int getRxSelfChanUtil_cfg80211(void *ctx, const char * ifname, u_int8_t *rxSelf)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    int chanUtil = 0;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, (OL_ATH_PARAM_CHAN_AP_RX_UTIL | ATH_PARAM_SHIFT), &chanUtil)) != 0)
    {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s : %s couldn't get rx self", __func__, ifname);
        goto err;
    }
    *rxSelf = (u_int8_t) chanUtil;
    dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s : %s rx self value = %d", __func__, ifname, chanUtil);

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int getRxObssChanUtil_cfg80211(void *ctx, const char * ifname, u_int8_t *rxObss)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    int chanUtil = 0;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, (OL_ATH_PARAM_CHAN_OBSS_RX_UTIL | ATH_PARAM_SHIFT), &chanUtil)) != 0)
    {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s : %s couldn't get rx obss\n", __func__, ifname);
        goto err;
    }
    *rxObss = (u_int8_t) chanUtil;
    dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s : %s rx obss value = %d\n", __func__, ifname, chanUtil);

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int getApPwrType_cfg80211(void *ctx, const char * ifname, int * apPwrType)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    int buf;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, (OL_ATH_PARAM_SHIFT | OL_ATH_PARAM_DISPLAY_AP_PWR_TYPE),
          &buf)) < 0)
    {
        TRACE_EXIT_ERR();
        return ret;
    }
    *apPwrType = buf;

    TRACE_EXIT();
    return 0;

}

int setApPwrType_cfg80211(void *ctx, const char * ifname, int apPwrType)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = setParam_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, (OL_ATH_PARAM_SHIFT | OL_ATH_PARAM_SET_AP_PWR_TYPE),
          &apPwrType, sizeof(apPwrType))) < 0)
    {
        TRACE_EXIT_ERR();
        return ret;
    }

    TRACE_EXIT();
    return 0;

}

int getStrictPunc_cfg80211(void *ctx, const char *ifname, int *strict_punc) {
    int ret = -1;
    struct wlanif_cfg80211_priv * cfgPriv;
    int buf = 0;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

#if defined WIFISON_SUPPORT_11BE && defined WIFISON_MODULE_PUNCTURE
    if (( ret = send_command_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, OL_ATH_PARAM_STRICT_PUNCTURING, &buf)) < 0) {
        TRACE_EXIT_ERR();
        return ret;
    }
#endif

    *strict_punc = buf;
    TRACE_EXIT();
    return ret;
}

int setDfsPuncture_cfg80211(void *ctx, const char * ifname, uint8_t dfs_punc_enable)
{
    int ret = -1;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

#if defined WIFISON_SUPPORT_11BE && defined WIFISON_MODULE_PUNCTURE
    if (( ret = setParam_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, (OL_ATH_PARAM_SHIFT | OL_ATH_PARAM_DFS_PUNCTURE),
          &dfs_punc_enable, sizeof(dfs_punc_enable))) < 0)
    {
        TRACE_EXIT_ERR();
        return ret;
    }
#endif

    TRACE_EXIT();
    return ret;
}

int setAWGNChannel_cfg80211(void *ctx, const char * radioName, uint8_t channel)
{
    int ret = -1;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();
    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = setParam_cfg80211(&(cfgPriv->cfg80211_ctx_qca), radioName, (OL_ATH_PARAM_SHIFT | OL_ATH_PARAM_SET_AWGN_EVENT),
                    &channel, sizeof(channel)) ) < 0)
    {
        dbgf(soncfgDbgS.dbgModule, DBGINFO, "%s:%d> AWGN set failed radio:%s ret %d \n",__func__, __LINE__, radioName, ret) ;
        TRACE_EXIT_ERR();
        return ret;
    }
    dbgf(soncfgDbgS.dbgModule, DBGINFO, "%s:%d> set AWGN radio %s \n",__func__, __LINE__, radioName);
    TRACE_EXIT();
    return ret;
}

int setEirpThreshold_cfg80211(void *ctx, const char * ifname, uint8_t eirp_th) {
    int ret = -1;
    struct wlanif_cfg80211_priv * cfgPriv;
#if defined WIFISON_SUPPORT_11BE && defined WIFISON_MODULE_PUNCTURE
    unsigned int eirp_val;
#endif

    TRACE_ENTRY();
    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

#if defined WIFISON_SUPPORT_11BE && defined WIFISON_MODULE_PUNCTURE
    eirp_val = eirp_th;
    if (( ret = setParam_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, (OL_ATH_PARAM_SHIFT | OL_ATH_PARAM_PUNC_EIRP_THRES),
          &eirp_val, sizeof(eirp_val))) < 0)
    {
        dbgf(soncfgDbgS.dbgModule, DBGINFO, "%s:%d> set eirp ifname %s threshold %d ret %d \n",__func__, __LINE__, ifname, eirp_th, ret) ;
        TRACE_EXIT_ERR();
        return ret;
    }
#endif
    dbgf(soncfgDbgS.dbgModule, DBGINFO, "%s:%d> set eirp ifname %s threshold %d ret %d \n",__func__, __LINE__, ifname, eirp_th, ret) ;
    TRACE_EXIT();
    return ret;
}

int getChanListAFC_cfg80211(void *ctx, const char* ifname, void *afcChanList)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct wlan_chan_list_6g *list = NULL;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *)ctx;
    assert(cfgPriv != NULL);

    list = (struct wlan_chan_list_6g *)calloc(AFC_POWER_TYPE_MAX, sizeof(struct wlan_chan_list_6g));
    struct cfg80211_data buffer;
    buffer.data = (void *)list;
    buffer.length = sizeof(struct wlan_chan_list_6g) * AFC_POWER_TYPE_MAX;
    buffer.parse_data = 0;
    buffer.flags = 0;
    buffer.callback = NULL;
    if (( ret = wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca),
                                           QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_CONFIGURATION,
                                           QCA_NL80211_VENDOR_SUBCMD_GET_6G_CHAN_LIST, ifname,
                                           MLO_INVALID_LINK_ID, (char *) &buffer, buffer.length)) < 0)
    {
        perror("RetVal ");
        goto err;
    }

    memcpy(afcChanList, list, buffer.length);
    if(list)
        free(list);
    TRACE_EXIT();
    return 0;

err:
    if(list)
        free(list);

    TRACE_EXIT_ERR();
    return ret;
}

int setAfcTimerReset_cfg80211(void * ctx , const char *ifname)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    TRACE_ENTRY();
    char data=1;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);
    if(( ret = send_generic_command_cfg80211 (&(cfgPriv->cfg80211_ctx_qca), ifname, QCA_NL80211_VENDOR_SUBCMD_AFC_RESET, &data, sizeof(data))) < 0 )
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;
err:
    TRACE_EXIT_ERR();
    return ret;
}

int setAcsChanList_cfg80211(void *ctx, const char *ifName, u_int8_t numChans, u_int8_t *channels, int band) {
    int ret;
    struct ieee80211req_athdbg req;
    struct cfg80211_data buffer;
    struct wlanif_cfg80211_priv *cfgPriv;
    u_int8_t i;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *)ctx;
    assert(cfgPriv != NULL);

    memset(&req, 0, sizeof(struct ieee80211req_athdbg));


    req.cmd = IEEE80211_DBGREQ_SETACSUSERCHANLIST;
    req.needs_reply = DBGREQ_REPLY_IS_NOT_REQUIRED;
    req.data.user_chanlist.n_chan = numChans;

    for (i = 0; i < numChans; i++) {
        req.data.user_chanlist.chans[i].chan = channels[i];
        req.data.user_chanlist.chans[i].band = band; //TODO: should pass proper band index for 6Ghz support
    }

    buffer.data = &req;
    buffer.length = sizeof(req);
    buffer.callback = NULL;
    buffer.parse_data = 0;

    ret = wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca),
                QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION,
                QCA_NL80211_VENDOR_SUBCMD_DBGREQ, ifName, MLO_INVALID_LINK_ID, (char *)&buffer, buffer.length);

    if (ret < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int getMapBssRole_cfg80211(void *ctx, const char *ifName, u_int8_t *mapBssRole) {
    int ret, result;
    struct wlanif_cfg80211_priv *cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *)ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_mesh_get_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifName,
                                         MESH_MAP_BSS_TYPE, &result)) < 0) {
        goto err;
    }

    *mapBssRole = (u_int8_t)result;

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int setRRM_cfg80211(void *ctx, const char * ifname, u_int8_t value)
{
    int ret;

    TRACE_ENTRY();

    if (( ret = setParam_cfg80211(ctx, ifname, IEEE80211_PARAM_RRM_CAP, &value, sizeof(value))) < 0)
    {
        TRACE_EXIT_ERR();
        return ret;
    }

    TRACE_EXIT();
    return 0;
}

int setWds_cfg80211(void *ctx, const char * ifname, int value)
{
    int ret;

    TRACE_ENTRY();

    if (( ret = setParam_cfg80211(ctx, ifname, IEEE80211_PARAM_WDS, &value, sizeof(value))) < 0)
    {
        TRACE_EXIT_ERR();
        return ret;
    }

    TRACE_EXIT();
    return 0;
}

int set6GsecComp_cfg80211(void *ctx, const char * ifname, int value)
{
    int ret;

    TRACE_ENTRY();

    if (( ret = setParam_cfg80211(ctx, ifname, IEEE80211_PARAM_6G_SECURITY_COMP, &value, sizeof(value))) < 0)
    {
        TRACE_EXIT_ERR();
        return ret;
    }

    TRACE_EXIT();
    return 0;
}

int setCACTimeout_cfg80211(void *ctx, const char * ifname, int cac_timeout)
{
    int ret;

    TRACE_ENTRY();

    if (( ret = setParam_cfg80211(ctx, ifname, IEEE80211_PARAM_DFS_CACTIMEOUT, &cac_timeout, sizeof(cac_timeout))) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int setRRMFilter_cfg80211(void *ctx, const char * ifname, u_int8_t rrm_filter)
{
    int ret;

    TRACE_ENTRY();

    if (( ret = setParam_cfg80211(ctx, ifname,  IEEE80211_PARAM_RRM_FILTER, &rrm_filter, sizeof(rrm_filter))) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int setWNMFilter_cfg80211(void *ctx, const char * ifname, u_int8_t wnm_filter)
{
    int ret;

    TRACE_ENTRY();

    if (( ret = setParam_cfg80211(ctx, ifname, IEEE80211_PARAM_WNM_FILTER, &wnm_filter, sizeof(wnm_filter))) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/* Function to SAWF set svc class related generic info*/
int setGenericSAWF_cfg80211(void *ctx, const char * ifname,void * config, int configSize)
{

    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname, QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG, config, configSize)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int setHlosTidOvrd_cfg80211(void *ctx, const char * ifname, int enable)
{
    int ret;

    TRACE_ENTRY();

    if (( ret = setParam_cfg80211(ctx, ifname, IEEE80211_PARAM_HLOS_TID_OVERRIDE, &enable, sizeof(enable))) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int setDscpOvrd_cfg80211(void *ctx, const char * ifname, int enable)
{
    int ret;

    TRACE_ENTRY();

    if (( ret = setParam_cfg80211(ctx, ifname, IEEE80211_PARAM_DSCP_MAP_ID, &enable, sizeof(enable))) < 0)
    {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

int setDscpTidMap_cfg80211(void *ctx, const char * ifname, int tos, int tid)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = &tos;
    buffer.length = sizeof(int);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_setparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_WIFI_PARAMS,
                                                IEEE80211_PARAM_DSCP_TID_MAP, ifname, (char *)&buffer,
                                                tid, 0)) < 0)
    {
        return -EIO;
    }
    return 0;
}

#define IEEE80211_DSCP_POLICY_ATTR          2
#define IEEE80211_DSCP_POLICY_TCLAS_ATTR    3
int setDscpPolicy_cfg80211(void *ctx, const char * ifname, u_int8_t attrId, u_int8_t policyId,
                          u_int8_t reqType, u_int8_t dscp, u_int8_t srcIp[], u_int8_t dstIp[],
                          u_int16_t srcPort, u_int16_t dstPort, u_int8_t protocolNum) {
    int ret;
    struct ieee80211req_athdbg req = {0};
    struct cfg80211_data buffer;
    struct wlanif_cfg80211_priv *cfgPriv;
    struct ieee80211_qos_mgmt_ie* qos_ie = &req.data.qos_data;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *)ctx;
    assert(cfgPriv != NULL);

    memset(&req, 0, sizeof(struct ieee80211req_athdbg));

    req.cmd = IEEE80211_DBGREQ_ADD_DSCPACTION_POLICY;
    req.needs_reply = DBGREQ_REPLY_IS_NOT_REQUIRED;

    /* Get the dialog_token */
    qos_ie->dialog_token = 123;
    /* Get the attr_id */
    qos_ie->qos_mgmt_attr[0].policy_attr.attr_id = IEEE80211_DSCP_POLICY_ATTR;
    /* Note down the policy ID */
    qos_ie->qos_mgmt_attr[0].policy_attr.policy_id = policyId;
    /* Get the request type */
    qos_ie->qos_mgmt_attr[0].policy_attr.req_type = reqType;
    /* Get the DSCP value */
    qos_ie->qos_mgmt_attr[0].policy_attr.dscp = dscp;
    /* Indicate that tcla attribute is present */
    qos_ie->qos_mgmt_attr[0].tclas_attr_present = 4;
    /* Note down classifier type and mask */
    qos_ie->qos_mgmt_attr[0].tclas.type = 4;
    qos_ie->qos_mgmt_attr[0].tclas.mask = 0x5f;
    /* Curently only IPV4 version */
    qos_ie->qos_mgmt_attr[0].tclas.tclas.type14.type14_v4.version = 4;
    /* Note down Source IPv4 */
    memcpy(qos_ie->qos_mgmt_attr[0].tclas.tclas.type14.type14_v4.src_ip,
            srcIp, IEEE80211_IPV4_LEN);
    /* Note down Dest IPv4 */
    memcpy(qos_ie->qos_mgmt_attr[0].tclas.tclas.type14.type14_v4.dst_ip,
            dstIp, IEEE80211_IPV4_LEN);
    /* Note down Source port */
    qos_ie->qos_mgmt_attr[0].tclas.tclas.type14.type14_v4.src_port = srcPort;
    /* Note down Dest port */
    qos_ie->qos_mgmt_attr[0].tclas.tclas.type14.type14_v4.dst_port = dstPort;
    /* Note down protocol number */
    qos_ie->qos_mgmt_attr[0].tclas.tclas.type14.type14_v4.protocol = protocolNum;

    qos_ie->no_of_policies = 1;

    buffer.data = &req;
    buffer.length = sizeof(req);
    buffer.callback = NULL;
    buffer.parse_data = 0;

    ret = wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca),
                QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION,
                QCA_NL80211_VENDOR_SUBCMD_DBGREQ, ifname, MLO_INVALID_LINK_ID, (char *)&buffer, buffer.length);

    if (ret < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
return 0;
}

int sendDscpPolicy_cfg80211(void *ctx, const char * ifname, struct ether_addr *staMac,
                            u_int8_t policyId, u_int8_t reset) {
    int ret;
    struct ieee80211req_athdbg req = {0};
    struct cfg80211_data buffer;
    struct wlanif_cfg80211_priv *cfgPriv;
    struct ieee80211_qos_req* dscp_req = &req.data.dscp_req;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *)ctx;
    assert(cfgPriv != NULL);

    memset(&req, 0, sizeof(struct ieee80211req_athdbg));

    req.cmd = IEEE80211_DBGREQ_SEND_DSCPACTION_POLICY;
    req.needs_reply = DBGREQ_REPLY_IS_NOT_REQUIRED;

    memcpy(req.dstmac, staMac->ether_addr_octet, IEEE80211_ADDR_LEN);

    dscp_req->policy_id_list[0] = policyId;
    dscp_req->no_of_policies = 1;

    dscp_req->reset = reset;
    if (dscp_req->reset == 0)
        dscp_req->reset = 2;

    buffer.data = &req;
    buffer.length = sizeof(req);
    buffer.callback = NULL;
    buffer.parse_data = 0;

    ret = wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca),
                QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION,
                QCA_NL80211_VENDOR_SUBCMD_DBGREQ, ifname, MLO_INVALID_LINK_ID, (char *)&buffer, buffer.length);

    if (ret < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
return 0;
}

int addDelInterfaceToMLDLink_cfg80211(void *ctx, const char *ifname, const struct ether_addr *mld_addr, int addDelValue)
{
    int ret = -1;
#ifdef WLAN_FEATURE_11BE
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_generic_command_with_param_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname,
                QCA_NL80211_VENDOR_SUBCMD_MLD_LINK_ADD_DEL, (void *) (mld_addr->ether_addr_octet),
                IEEE80211_ADDR_LEN, MLO_INVALID_LINK_ID, addDelValue)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
#endif
    return ret;
}

/* Function to set t2lm mapping config*/
int setT2lmMapConfig_cfg80211(void *ctx, const char * ifname,void * config, int configSize, int linkId)
{
    int ret = -1;
#ifdef WLAN_FEATURE_11BE
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname,
            QCA_NL80211_VENDOR_SUBCMD_T2LM_LINK_MAP_CONFIG, config, configSize)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
#endif
    return ret;
}

/* Function to set t2lm mapping config per peer*/
int setT2lmMapConfigPerPeer_cfg80211(void *ctx, const char * ifname,void * config, int configSize, int linkId)
{
    int ret = -1;
#ifdef WLAN_FEATURE_11BE
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if ((ret = send_generic_command_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname,
            QCA_NL80211_VENDOR_SUBCMD_T2LM, config, configSize)) < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
#endif
    return ret;
}

/*========================================================================*/
/*======================== BSS Color functions ===========================*/
/*========================================================================*/

/*===== Setter functions =====*/

/*Function to set BSS Color*/
int setBSSColor_cfg80211(void *ctx, const char *ifname, uint8_t color, int override)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = &override;
    buffer.length = sizeof(int);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_setparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_BSS_COLOR,
                                                color, ifname, (char *)&buffer, 0, 0)) < 0)
    {
        return -EIO;
    }
    return 0;
}

/*Function to enable/disable HE-SIGA SR15*/
int setEnableHeSigaSR15_cfg80211(void *ctx, const char * ifname, int enable)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = &enable;
    buffer.length = sizeof(int);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_setparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_SR_CONFIG,
                                                HE_SR_SR15_ENABLE, ifname, (char *)&buffer,
                                                sizeof(int), 0)) < 0)
    {
        return -EIO;
    }
    return 0;
}

/*Function to set SRG information valid and SRG OBSSPD min and max offset */
int setSRGValid_cfg80211(void *ctx, const char * ifname, int valid, int minOffset, int maxOffset)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = &valid;
    buffer.length = sizeof(int);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_setparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_SR_CONFIG,
                                                HE_SR_SRG_OBSSPD_ENABLE, ifname, (char *)&buffer,
                                                minOffset, maxOffset)) < 0)
    {
        return -EIO;
    }
    return 0;
}

/*Function to enable/disable PSR*/
int setPSREnabled_cfg80211(void *ctx, const char * ifname, int enable)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = &enable;
    buffer.length = sizeof(int);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_setparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                  QCA_NL80211_VENDOR_SUBCMD_HE_SR_CONFIG,
                                                  HE_SR_PSR_ENABLE, ifname,
                                                  (char *)&buffer, 0, 0)) < 0)
    {
        return -EIO;
    }
    return 0;
}

/*Function to set non SRG information valid and non SRG OBSSPD max offset*/
int setNonSRGValid_cfg80211(void *ctx, const char * ifname, int valid, int maxOffset)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = &valid;
    buffer.length = sizeof(int);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_setparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_SR_CONFIG,
                                                HE_SR_NON_SRG_OBSSPD_ENABLE, ifname,
                                                (char *)&buffer, maxOffset, 0)) < 0)
    {
        return -EIO;
    }
    return 0;
}

/*Function to set SRG BSS color bitmap*/
int setBSSColorBmp_cfg80211(void *ctx, const char * ifname, u_int32_t bitmap0, u_int32_t bitmap1)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;

    buffer.data = &bitmap0;
    buffer.length = sizeof(u_int32_t);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_setparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_SRG_BITMAP,
                                                HE_SRP_IE_SRG_BSS_COLOR_BITMAP, ifname,
                                                (char *)&buffer, bitmap1, 0)) < 0)
    {
        return -EIO;
    }
    return 0;
}

/*Function to set partial BSSID bitmap*/
int setPartialBSSIDBmp_cfg80211(void *ctx, const char * ifname, u_int32_t bitmap0, u_int32_t bitmap1)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;

    buffer.data = &bitmap0;
    buffer.length = sizeof(u_int32_t);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_setparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_SRG_BITMAP,
                                                HE_SRP_IE_SRG_PARTIAL_BSSID_BITMAP,
                                                ifname, (char *)&buffer, bitmap1, 0)) < 0)
    {
        return -EIO;
    }
    return 0;
}

int setColorCollisionApPeriod_cfg80211(void *ctx, const char * ifname, int period) {
    int ret;
    struct ieee80211req_athdbg req;
    struct cfg80211_data buffer;
    struct wlanif_cfg80211_priv *cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *)ctx;
    assert(cfgPriv != NULL);

    memset(&req, 0, sizeof(struct ieee80211req_athdbg));


    req.cmd = IEEE80211_DBGREQ_BSSCOLOR_DOT11_COLOR_COLLISION_AP_PERIOD;
    req.needs_reply = DBGREQ_REPLY_IS_NOT_REQUIRED;

    req.data.param[0] = period;

    buffer.data = &req;
    buffer.length = sizeof(req);
    buffer.callback = NULL;
    buffer.parse_data = 0;

    ret = wifi_cfg80211_send_generic_command(&(cfgPriv->cfg80211_ctx_qca),
                QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION,
                QCA_NL80211_VENDOR_SUBCMD_DBGREQ, ifname, MLO_INVALID_LINK_ID, (char *)&buffer, buffer.length);

    if (ret < 0) {
        goto err;
    }

    TRACE_EXIT();
    return 0;

err:
    TRACE_EXIT_ERR();
    return ret;
}

/*===== Getter functions =====*/

#define HE_MESH_CONFIG_STRING       20
#define HE_SRG_BITMAP_STRING        20
#define HE_SR_CONFIG_OBSSPD_STRING  48

/* Function to get BSS color */
int getBSSColor_cfg80211(void *ctx, const char *ifname, char* bssColor)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = (void *)bssColor;
    buffer.length = HE_MESH_CONFIG_STRING;
    buffer.parse_data = 0;
    buffer.callback = &cfg82011_wificonfiguration_cb;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_getparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_MESH_CONFIG,
                                                0, ifname, (char *)&buffer,
                                                HE_MESH_CONFIG_STRING)) < 0)
    {
        return -EIO;
    }

    return 0;
}

/* Function to get HE-SIGA SR15 enable status */
int getEnableHeSigaSR15_cfg80211(void *ctx, const char * ifname, uint32_t * enable)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = (uint32_t *)enable;
    buffer.length = HE_SR_CONFIG_OBSSPD_STRING;
    buffer.parse_data = 0;
    buffer.callback = &cfg82011_wificonfiguration_cb;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_getparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_SR_CONFIG,
                                                HE_SR_SR15_ENABLE, ifname, (char *)&buffer,
                                                sizeof(int))) < 0)
    {
        return -EIO;
    }

    return 0;
}

/*Function to get SRG information valid and SRG OBSSPD max and min offset*/
int getSRGValid_cfg80211(void *ctx, const char * ifname, char* srgInfo)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = (void *)srgInfo;
    buffer.length = HE_SR_CONFIG_OBSSPD_STRING;
    buffer.parse_data = 0;
    buffer.callback = &cfg82011_wificonfiguration_cb;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_getparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_SR_CONFIG,
                                                HE_SR_SRG_OBSSPD_ENABLE, ifname,
                                                (char *)&buffer, 0)) < 0)
    {
        return -EIO;
    }

    return 0;
}

/*Function to get enable status of PSR*/
/* 0 - disable
   1 - enable
*/
int getPSREnabled_cfg80211(void *ctx, const char * ifname, uint32_t *enable)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = (uint32_t *)enable;
    buffer.length = HE_SR_CONFIG_OBSSPD_STRING;
    buffer.parse_data = 0;
    buffer.callback = &cfg82011_wificonfiguration_cb;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_getparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_SR_CONFIG,
                                                HE_SR_PSR_ENABLE,
                                                ifname, (char *)&buffer, 0)) < 0)
    {
        return -EIO;
    }

    return 0;
}

/*Function to get Non SRG information valid and Non SRG OBSSPD max offset*/
int getNonSRGValid_cfg80211(void *ctx, const char * ifname, char* nonSrgInfo)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = (void *)nonSrgInfo;
    buffer.length = HE_SR_CONFIG_OBSSPD_STRING;
    buffer.parse_data = 0;
    buffer.callback = &cfg82011_wificonfiguration_cb;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_getparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_SR_CONFIG,
                                                HE_SR_NON_SRG_OBSSPD_ENABLE,
                                                ifname, (char *)&buffer, 0)) < 0)
    {
        return -EIO;
    }

    return 0;
}

/*Function to get SRG BSS color bitmap*/
int getBSSColorBmp_cfg80211(void *ctx, const char * ifname, char* bitmap)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = (void *)bitmap;
    buffer.length = HE_SRG_BITMAP_STRING;
    buffer.parse_data = 0;
    buffer.callback = &cfg82011_wificonfiguration_cb;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_getparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_SRG_BITMAP,
                                                HE_SRP_IE_SRG_BSS_COLOR_BITMAP,
                                                ifname, (char *)&buffer, 0)) < 0)
    {
        return -EIO;
    }

    return 0;
}

/*Function to get partial BSSID bitmap*/
int getPartialBSSIDBmp_cfg80211(void *ctx, const char * ifname, char* bitmap)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = (void *)bitmap;
    buffer.length = HE_SRG_BITMAP_STRING;
    buffer.parse_data = 0;
    buffer.callback = &cfg82011_wificonfiguration_cb;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_getparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_SRG_BITMAP,
                                                HE_SRP_IE_SRG_PARTIAL_BSSID_BITMAP,
                                                ifname, (char *)&buffer, 0)) < 0)
    {
        return -EIO;
    }

    return 0;
}

/*Function to get neighbor color bitmap*/
int getNeighborColorBmp_cfg80211(void *ctx, const char * ifname, char* bitmap)
{
    #if 0
    /* Enable this section after driver code check-in. Not needed for WFA */

    struct wlanif_cfg80211_priv * cfgPriv;
    struct cfg80211_data buffer;
    buffer.data = (void *)bitmap;
    buffer.length = HE_SRG_BITMAP_STRING;
    buffer.parse_data = 0;
    buffer.callback = &cfg82011_wificonfiguration_cb;
    buffer.parse_data = 0;

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if((ret=wifi_cfg80211_send_getparam_command(&(cfgPriv->cfg80211_ctx_qca),
                                                QCA_NL80211_VENDOR_SUBCMD_HE_SRG_BITMAP,
                                                HE_SRP_SRG_NEIGHBOR_BSS_COLOR_BITMAP,
                                                ifname, (char *)&buffer, 0)) < 0)
    {
        return -EIO;
    }
    #endif

    strlcpy(bitmap, "0x00000000 00000000", HE_SR_CONFIG_OBSSPD_STRING);

    return 0;
}


/*
 * cfg80211_event_getwifi: Function which fills outputbuf with inputbuf(eventdata)
 * @ifindex: ifindex for a particular interface
 * @cmdid: Event type
 * @inpubuf: Event data
 * @len: length of the data
 * @outputbuf : Filled in format of LBD strcture ath_vendorcfg_event
 * with int cmdid, int ifindex and with Event Data(inputbuf)
 * @return 0 on success, otherwise return 1
 */
u_int8_t cfg80211_event_getwifi(int ifindex, int cmdid, void *inputbuf,
        uint32_t len, void *outbuf)
{
    int *locbuf;

    if (inputbuf == NULL || outbuf == NULL) {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: %d ERROR!! Received NULL buffer\n",__func__, __LINE__);
        return 1;
    }

    dbgf(soncfgDbgS.dbgModule, DBGDEBUG, ":%s Received Event with cmdid:%d ifidx:%d len:%d \n",__func__, cmdid, ifindex, len);

#ifdef WIFISON_SUPPORT_11BE
    if ((cmdid != QCA_NL80211_VENDOR_SUBCMD_DBGREQ) && (cmdid != QCA_NL80211_VENDOR_SUBCMD_FWD_RRM_RPT)
          && (cmdid != QCA_NL80211_VENDOR_SUBCMD_FWD_BTM_RPT) && (cmdid != QCA_NL80211_VENDOR_SUBCMD_SMPS_UPDATE)
            && (cmdid != QCA_NL80211_VENDOR_SUBCMD_OPMODE_UPDATE) && (cmdid != QCA_NL80211_VENDOR_SUBCMD_PEER_T2LM_POLICY)
            && (cmdid != QCA_NL80211_VENDOR_SUBCMD_AWGN_INTERFERENCE_UPDATE))
#else
    if ((cmdid != QCA_NL80211_VENDOR_SUBCMD_DBGREQ) && (cmdid != QCA_NL80211_VENDOR_SUBCMD_FWD_RRM_RPT)
          && (cmdid != QCA_NL80211_VENDOR_SUBCMD_FWD_BTM_RPT) && (cmdid != QCA_NL80211_VENDOR_SUBCMD_SMPS_UPDATE)
            && (cmdid != QCA_NL80211_VENDOR_SUBCMD_OPMODE_UPDATE) && (cmdid != QCA_NL80211_VENDOR_SUBCMD_AWGN_INTERFERENCE_UPDATE))
#endif
    {
         dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: %d unknown subcmd:%d cannot handle \n", __func__, __LINE__, cmdid);
         return 1;
    }
    locbuf = (int *)outbuf;
    *locbuf = cmdid;
    locbuf++;
    *locbuf = ifindex;
    locbuf++;

    switch (cmdid) {
        case QCA_NL80211_VENDOR_SUBCMD_DBGREQ:
            {
                struct ieee80211req_athdbg_event *dbg_event;
                struct ieee80211_hmwds_ast_add_status *ast_status = (struct ieee80211_hmwds_ast_add_status *) locbuf;
                u_int8_t offset = 0;

                dbg_event = (struct ieee80211req_athdbg_event *)inputbuf;
                if (dbg_event->cmd == IEEE80211_DBGREQ_HMWDS_AST_ADD_STATUS) {
                    ast_status->cmd = dbg_event->cmd;
                    memcpy(ast_status->peer_mac, (u_int8_t *) &(dbg_event->fw_unit_test), IEEE80211_ADDR_LEN);
                    offset+=IEEE80211_ADDR_LEN;
                    memcpy(ast_status->ast_mac, (((u_int8_t *)&dbg_event->fw_unit_test) + offset), IEEE80211_ADDR_LEN);
                    offset+=IEEE80211_ADDR_LEN;
                    ast_status->status = *(int *)(((u_int8_t *)&(dbg_event->fw_unit_test)) + offset);
                }

            }
            break;
        case QCA_NL80211_VENDOR_SUBCMD_FWD_RRM_RPT:
        case QCA_NL80211_VENDOR_SUBCMD_FWD_BTM_RPT:
        case QCA_NL80211_VENDOR_SUBCMD_SMPS_UPDATE:
        case QCA_NL80211_VENDOR_SUBCMD_OPMODE_UPDATE:
        case QCA_NL80211_VENDOR_SUBCMD_AWGN_INTERFERENCE_UPDATE:
#ifdef WIFISON_SUPPORT_11BE
        case QCA_NL80211_VENDOR_SUBCMD_PEER_T2LM_POLICY:
#endif
            {
                memcpy((u_int8_t *)locbuf, (u_int8_t *)inputbuf, len*sizeof(u_int8_t));
            }
            break;
        default:
            dbgf(soncfgDbgS.dbgModule, DBGERR, "%s:CFG80211 event unknown cmd: %d\n",__func__,cmdid);
            return 1;
            break;
        }
    return 0;
}


/*
 * nl80211_vendor_event_qca_parse_get_wifi: nl80211 vendor event to get wifi configuration
 * @ifname: interface name
 * @data: pointer to data
 * @len: length of the data
 * @return 0 on success, otherwise return 1
 */
u_int8_t nl80211_vendor_event_qca_parse_get_wifi(int ifidx,
        uint8_t *data, size_t len, void *outbuf)
{
    struct nlattr *tb_array[QCA_WLAN_VENDOR_ATTR_CONFIG_MAX + 1];
    struct nlattr *tb;
    void *buffer = NULL;
    uint32_t buffer_len = 0;
    uint32_t subcmd;

    if (nla_parse(tb_array, QCA_WLAN_VENDOR_ATTR_CONFIG_MAX,
                (struct nlattr *) data, len, NULL)) {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: INVALID EVENT\n",__func__);
        return 1;
    }
    tb = tb_array[QCA_WLAN_VENDOR_ATTR_CONFIG_GENERIC_COMMAND];
    if (!tb) {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "ERROR!!!GENERIC CMD not found within get-wifi subcmd\n");
        return 1;
    }
    subcmd = nla_get_u32(tb);

    tb = tb_array[QCA_WLAN_VENDOR_ATTR_CONFIG_GENERIC_DATA];
    if (tb) {
        buffer = nla_data(tb);
        buffer_len = nla_len(tb);
        if (cfg80211_event_getwifi(ifidx, subcmd, buffer, buffer_len, outbuf) != 0)
            return 1;
    }
    return 0;
}

#ifdef WIFISON_SUPPORT_11BE
/*
 * nl80211_vendor_event_qca_parse_t2lm_message: nl80211 vendor event to get t2lm link preference
 * @ifname: interface name
 * @data: pointer to data
 * @len: length of the data
 * @return 0 on success, otherwise return 1
 */
u_int8_t nl80211_vendor_event_qca_parse_t2lm_message(int ifidx,
        uint8_t *data, size_t len, void *outbuf)
{
    struct nlattr *tb_array[QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_MAX + 1];
    struct nlattr *t2lm_tb[T2LM_MAX_DIRECTION + 1];
    struct nlattr *t2lm_info;
    int t2lm_dir_limit = 0;
    u_int8_t dir_count = 0;
    struct nlattr *prov_link[NUM_TIDS + 1];
    struct nlattr *tb_prov;
    int t2lm_link_limit = 0;
    u_int8_t t2lm_count = 0;
    struct nlattr *tb_pref_order[MAX_PREFERRED_LINKS+1];
    struct nlattr *tb_pref;
    int t2lm_pref_limit = 0;
    u_int8_t perf_count = 0;
    struct nlattr *tb_max_latency[MAX_AC+1];
    struct nlattr *tb_latency;
    int t2lm_latency_limit = 0;
    u_int8_t latency_count = 0;
    void *buffer = NULL;
    uint32_t buffer_len = 0;
    uint32_t subcmd;
    struct preferred_links_t perf = {0};
    struct nlattr *t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_CONFIG_MAX + 1];
    struct nlattr *t2lm_pref;
    int t2lm_peer_limit = 0;
    int err;

    if (nla_parse(tb_array, QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_MAX, (struct nlattr*)data, len, NULL))
    {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: INVALID EVENT\n",__func__);
        return 1;
    }

    subcmd = QCA_NL80211_VENDOR_SUBCMD_PEER_T2LM_POLICY;
    if(tb_array[QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_LINK_QUALIFIER])
    {
        nla_for_each_nested(t2lm_pref, tb_array[QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_LINK_QUALIFIER], t2lm_peer_limit)
        {
            /// Note: Using nla_parse_nested right now but in cae of error try using nla_parse
            err = nla_parse_nested(t2lm_link_pref, QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_MAX, t2lm_pref, NULL);
            if(!err)
            {
                // Get the Peer MAC address
                if (t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_MAC_ADDR])
                {
                    // Parse the MAC address
                    memcpy(&perf.peer_mld_mac_addr[0], nla_data(t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_MAC_ADDR]), 6);
                    dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s:Peer MAC acddress: %02X:%02X:%02X:%02X:%02X:%02X",__func__, perf.peer_mld_mac_addr[0],
                            perf.peer_mld_mac_addr[1], perf.peer_mld_mac_addr[2], perf.peer_mld_mac_addr[3], perf.peer_mld_mac_addr[4], perf.peer_mld_mac_addr[5]);

                    // Parse the T2LM of TIDS / Provisioning info
                    if(t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_NUM_T2LM])
                    {
                        perf.num_t2lm_of_tids = nla_get_u8(t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_NUM_T2LM]);
                        dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s:num_t2lm_of_tids: %d",__func__, perf.num_t2lm_of_tids);

                        if(perf.num_t2lm_of_tids) // Parse provisioning info only of num_t2lm_of_tids > 0
                        {
                            // Parse the Homogeneous mapping
                            if(t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_HOMOGENEOUS_MAPPING])
                            {
                                perf.homogeneous_mapping = nla_get_u8(t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_HOMOGENEOUS_MAPPING]);
                                dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s:Homogeneous Mapping: %d",__func__, perf.homogeneous_mapping);
                            }
                            else
                            {
                                dbgf(soncfgDbgS.dbgModule, DBGERR, "fatal: t2lm message doesn't have homogeneous_mapping");
                                return 1;
                            }

                            // Parse t2lm_of_tids_t
                            if(t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_T2LM_OF_TIDS])
                            {
                                dir_count = 0;
                                nla_for_each_nested(t2lm_info, t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_T2LM_OF_TIDS], t2lm_dir_limit)
                                {
                                    err = nla_parse_nested(t2lm_tb, T2LM_MAX_DIRECTION, t2lm_info, NULL);
                                    if(!err)
                                    {
                                        // Direction
                                        if (t2lm_tb[QCA_WLAN_VENDOR_ATTR_QUALIFIER_DIRECTION])
                                        {
                                            perf.t2lm[dir_count].direction = nla_get_u8(t2lm_tb[QCA_WLAN_VENDOR_ATTR_QUALIFIER_DIRECTION]);
                                            dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s: perf.t2lm[%d].direction: %d",__func__, dir_count, perf.t2lm[dir_count].direction);
                                        }
                                        else
                                        {
                                            dbgf(soncfgDbgS.dbgModule, DBGERR, "fatal: t2lm message doesn't have provisioned direction");
                                            return 1;
                                        }

                                        // Provisoned links
                                        if(t2lm_tb[QCA_WLAN_VENDOR_ATTR_QUALIFIER_PROVISIONED_LINKS])
                                        {
                                            t2lm_count = 0;
                                            nla_for_each_nested(tb_prov, t2lm_tb[QCA_WLAN_VENDOR_ATTR_QUALIFIER_PROVISIONED_LINKS], t2lm_link_limit)
                                            {
                                                err = nla_parse_nested(prov_link, NUM_TIDS, tb_prov, NULL);
                                                if(!err)
                                                {
                                                    if(prov_link[QCA_WLAN_VENDOR_ATTR_TID_MAP])
                                                    {
                                                        perf.t2lm[dir_count].t2lm_provisioned_links[t2lm_count] = nla_get_u16(prov_link[QCA_WLAN_VENDOR_ATTR_TID_MAP]);
                                                        dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s: perf.t2lm[%d].t2lm_provisioned_links[%d]: %d",__func__, dir_count, t2lm_count, perf.t2lm[dir_count].t2lm_provisioned_links[t2lm_count]);
                                                        t2lm_count++;
                                                    }
                                                }
                                                else
                                                {
                                                    dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: err: %s", __func__, strerror (errno));
                                                    return 1;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            dbgf(soncfgDbgS.dbgModule, DBGERR, "fatal: t2lm message doesn't have provisioned links");
                                            return 1;
                                        }
                                        dir_count++;
                                    }
                                    else
                                    {
                                        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: err: %s", __func__, strerror (errno));
                                        return 1;
                                    }
                                }
                            }
                            else
                            {
                                dbgf(soncfgDbgS.dbgModule, DBGERR, "fatal: t2lm message doesn't have t2lm_of_tids_t");
                                return 1;
                            }
                        }
                    }
                    else
                    {
                        dbgf(soncfgDbgS.dbgModule, DBGERR, "fatal: t2lm message doesn't have num_t2lm_of_tids");
                    }

                    // Get the Preference info
                    if(t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_NUM_LINKS])
                    {
                        perf.num_pref_links = nla_get_u8(t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_NUM_LINKS]);
                        dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s:num_pref_links: %d",__func__, perf.num_pref_links);
                        if(perf.num_pref_links)
                        {
                            // Get preferred link order
                            if(t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_ORDER])
                            {
                                perf_count = 0;
                                nla_for_each_nested(tb_pref, t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_ORDER], t2lm_pref_limit)
                                {
                                    err = nla_parse_nested(tb_pref_order, MAX_PREFERRED_LINKS, tb_pref, NULL);
                                    if(!err)
                                    {
                                        if(tb_pref_order[QCA_WLAN_VENDOR_ATTR_QUALIFIER_ORDER])
                                        {
                                            perf.preffered_link_order[perf_count] = nla_get_u8(tb_pref_order[QCA_WLAN_VENDOR_ATTR_QUALIFIER_ORDER]);
                                            dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s: perf.preffered_link_order[%d]: %d",__func__, perf_count, perf.preffered_link_order[perf_count]);
                                            perf_count++;
                                        }
                                        else
                                        {
                                            dbgf(soncfgDbgS.dbgModule, DBGERR, "fatal: t2lm message doesn't have Preference Order");
                                            return 1;
                                        }
                                    }
                                    else
                                    {
                                        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: err: %s", __func__, strerror (errno));
                                        return 1;
                                    }
                                }
                            }

                            // Get Max Latency
                            if(t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_TIMEOUT])
                            {
                                latency_count = 0;
                                nla_for_each_nested(tb_latency, t2lm_link_pref[QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_TIMEOUT], t2lm_latency_limit)
                                {
                                    err = nla_parse_nested(tb_max_latency, MAX_AC, tb_latency, NULL);
                                    if(!err)
                                    {
                                        if(tb_max_latency[QCA_WLAN_VENDOR_ATTR_TIMEOUT])
                                        {
                                            perf.max_latency[latency_count] = nla_get_u32(tb_max_latency[QCA_WLAN_VENDOR_ATTR_TIMEOUT]);
                                            dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s: perf.preffered_link_order[%d]: %d",__func__, latency_count, perf.max_latency[latency_count]);
                                            latency_count++;
                                        }
                                        else
                                        {
                                            dbgf(soncfgDbgS.dbgModule, DBGERR, "fatal: t2lm message doesn't have Latency");
                                            return 1;
                                        }
                                    }
                                    else
                                    {
                                        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: err: %s", __func__, strerror (errno));
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        dbgf(soncfgDbgS.dbgModule, DBGERR, "fatal: t2lm message doesn't have num_pref_links");
                    }
                }
                else
                {
                    dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: fatal: message doesn't mld peer mac\n",__func__);
                    return 1;
                }
            }
            else
            {
                dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: err: %s", __func__, strerror (errno));
                return 1;
            }

            buffer = (void*)(&perf);
            buffer_len = sizeof(struct preferred_links_t);
            if (cfg80211_event_getwifi(ifidx, subcmd, buffer, buffer_len, outbuf) != 0)
                return 1;
        }
    }
    else {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: The t2lm event has no Attribute (QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_LINK_QUALIFIER) \n",__func__);
        return 1;
    }

    return 0;
}
#endif

#ifdef EZMESH_MODULE_AFC
u_int8_t cfg80211_qca_parse_afc_response_payload(struct nlattr **tb_array,
                    ieee80211_afc_response_t *afcResponse)
{
    struct nlattr *tb;

    tb = tb_array[QCA_WLAN_VENDOR_ATTR_AFC_EVENT_REQ_ID];
    if (tb) {
        afcResponse->resp_id = nla_get_u32(tb);
    }

    tb = tb_array[QCA_WLAN_VENDOR_ATTR_AFC_EVENT_STATUS_CODE];
    if (tb) {
        afcResponse->fw_status_code = nla_get_u8(tb);
    }

    tb = tb_array[QCA_WLAN_VENDOR_ATTR_AFC_EVENT_SERVER_RESP_CODE];
    if (tb) {
        afcResponse->server_resp_code = nla_get_u32(tb);
    }

    tb = tb_array[QCA_WLAN_VENDOR_ATTR_AFC_EVENT_AFC_WFA_VERSION];
    if (tb) {
        afcResponse->afc_wfa_version = nla_get_u32(tb);
    }

    tb = tb_array[QCA_WLAN_VENDOR_ATTR_AFC_EVENT_EXP_DATE];
    if (tb) {
        afcResponse->afc_expiry_date = nla_get_u32(tb);
    }

    tb = tb_array[QCA_WLAN_VENDOR_ATTR_AFC_EVENT_EXP_TIME];
    if (tb) {
        afcResponse->afc_expiry_time = nla_get_u32(tb);
    }

    return 0;
}
#endif

/*
 * nl80211_vendor_event_qca_parse_afc_evet: nl80211 vendor event to get AFC events
 * @ifname: interface name
 * @data: pointer to data
 * @len: length of the data
 * @return 0 on success, otherwise return 1
 */
#ifdef EZMESH_MODULE_AFC
u_int8_t nl80211_vendor_event_qca_parse_afc_evet(int ifidx,
        uint8_t *data, size_t len, void *outbuf)
{
    struct nlattr *tb_array[QCA_WLAN_VENDOR_ATTR_CONFIG_MAX + 1];
    struct nlattr *tb;
    int *locbuf;
    uint8_t eventType;
    uint32_t buffer_len = 0;
    ieee80211_afc_response_t *afcPayload;

    if (nla_parse(tb_array, QCA_WLAN_VENDOR_ATTR_CONFIG_MAX,
                (struct nlattr *) data, len, NULL)) {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s: INVALID EVENT\n",__func__);
        return 1;
    }
    tb = tb_array[QCA_WLAN_VENDOR_ATTR_AFC_EVENT_TYPE];
    if (!tb) {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "ERROR!!!Power Event type not found within AFC event\n");
        return 1;
    }
    eventType = nla_get_u8(tb);

    locbuf = (int *)outbuf;
    *locbuf = eventType;
    locbuf++;
    *locbuf = ifidx;
    locbuf++;

    switch ( eventType ) {
        case QCA_WLAN_VENDOR_AFC_EVENT_TYPE_POWER_UPDATE_COMPLETE:
            dbgf(soncfgDbgS.dbgModule, DBGDEBUG, "%s: Received AFC Payload Response\n", __func__);
            afcPayload = (ieee80211_afc_response_t *)locbuf;
            cfg80211_qca_parse_afc_response_payload(tb_array, afcPayload);
            break;
        case QCA_WLAN_VENDOR_AFC_EVENT_TYPE_EXPIRY:
            tb = tb_array[QCA_WLAN_VENDOR_ATTR_AFC_EVENT_REQ_ID];
            if (tb) {
                /*Only copy Request ID*/
                ieee80211_afc_expiry_t *afcExpiryData = (ieee80211_afc_expiry_t*)locbuf;
                afcExpiryData->req_id = nla_get_u32(tb);
                buffer_len = nla_len(tb);
            }
            dbgf(soncfgDbgS.dbgModule, DBGDEBUG, "%s: Received AFC Power Expiry Event with length %d\n",
                                __func__, buffer_len);
            break;
        default:
            break;
    }
    return 0;
}
#endif

/*
 * cfg80211_event_callback: cfg80211 event callback to get wifi configuration
 * @ifname: interface name
 * @subcmd: enum value for sub command
 * @data: pointer to the data
 * @len: length of the data
 */
void cfg80211_event_callback(char *ifname,
        uint32_t subcmd, uint8_t *data, size_t len)
{
    /* Dummy callback function for ceating the event socket */
    return;
}

int get_cfg80211_event_sock(void * ctx)
{
    struct wlanif_cfg80211_priv * cfgPriv = (struct wlanif_cfg80211_priv *)ctx;

    return cfgPriv->cfg80211_ctx_qca.event_sock->s_fd;
}

/*
 * get_nl80211_event_msg: Function to parse the cfg event data
 * @msg: Pointer to netlink event message
 * @outbuf: out put, pointer on whcih parsed data is stored
 * @return 0 on success
 */

int get_nl80211_event_msg(u_int8_t *msg, void * ctx, void *outbuf)
{
    struct genlmsghdr *gnlh = (struct genlmsghdr *)msg;
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    int ifidx = -1;
    char ifname[20] = {0};

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);

    if (tb[NL80211_ATTR_IFINDEX])
        ifidx = nla_get_u32(tb[NL80211_ATTR_IFINDEX]);

    if (ifidx != -1) {
        if_indextoname(ifidx, ifname);
    }

    switch (gnlh->cmd) {
        case NL80211_CMD_VENDOR:
            {
                u_int32_t vendor_id, subcmd;
                u_int8_t *data = NULL;
                size_t len = 0;

                if (!tb[NL80211_ATTR_VENDOR_ID] ||
                        !tb[NL80211_ATTR_VENDOR_SUBCMD])
                    return -1;

                vendor_id = nla_get_u32(tb[NL80211_ATTR_VENDOR_ID]);
                subcmd = nla_get_u32(tb[NL80211_ATTR_VENDOR_SUBCMD]);
                dbgf(soncfgDbgS.dbgModule, DBGDUMP, "%s %d: subcommand: %d", __func__, __LINE__, subcmd);

                if (tb[NL80211_ATTR_VENDOR_DATA]) {
                    data = nla_data(tb[NL80211_ATTR_VENDOR_DATA]);
                    len = nla_len(tb[NL80211_ATTR_VENDOR_DATA]);
                }
                switch (vendor_id) {
                    case OUI_QCA:
                        switch(subcmd) {
                            case QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_CONFIGURATION:
                                if (nl80211_vendor_event_qca_parse_get_wifi(ifidx, data, len, outbuf)!= 0)
                                    return 1;
                                break;
#ifdef WIFISON_SUPPORT_11BE
                            case QCA_NL80211_VENDOR_SUBCMD_PEER_T2LM_POLICY:
                                if (nl80211_vendor_event_qca_parse_t2lm_message(ifidx, data, len, outbuf) != 0)
                                    return 1;
                                break;
#endif
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
            }
        case NL80211_CMD_CH_SWITCH_NOTIFY:
            dbgf(soncfgDbgS.dbgModule, DBGINFO, "%s:%d> Received NL80211_CMD_CH_SWITCH_NOTIFY \n", __func__, __LINE__) ;
            break;

        default:
            break;
    }
    return 0;
}

/*
 * parse_afc_cfg80211_event_msg: Function to parse the AFC event data
 * @msg: Pointer to netlink event message
 * @ctx: nl event context
 * @outbuf: out put, pointer on whcih parsed data is stored
 * @return 0 on success
 */
#ifdef EZMESH_MODULE_AFC
int parse_afc_cfg80211_event_msg(u_int8_t *msg, void * ctx, void *outbuf)
{
    struct genlmsghdr *gnlh = (struct genlmsghdr *)msg;
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    int ifidx = -1;
    char ifname[20] = {0};

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);

    if (tb[NL80211_ATTR_IFINDEX])
        ifidx = nla_get_u32(tb[NL80211_ATTR_IFINDEX]);

    if (ifidx != -1) {
        if_indextoname(ifidx, ifname);
    }

    switch (gnlh->cmd) {
        case NL80211_CMD_VENDOR:
            {
                u_int32_t vendor_id, subcmd;
                u_int8_t *data = NULL;
                size_t len = 0;

                if (!tb[NL80211_ATTR_VENDOR_ID] ||
                        !tb[NL80211_ATTR_VENDOR_SUBCMD])
                    return -1;

                vendor_id = nla_get_u32(tb[NL80211_ATTR_VENDOR_ID]);
                subcmd = nla_get_u32(tb[NL80211_ATTR_VENDOR_SUBCMD]);

                if (tb[NL80211_ATTR_VENDOR_DATA]) {
                    data = nla_data(tb[NL80211_ATTR_VENDOR_DATA]);
                    len = nla_len(tb[NL80211_ATTR_VENDOR_DATA]);
                }
                switch (vendor_id) {
                    case OUI_QCA:
                        switch(subcmd) {
                            case QCA_NL80211_VENDOR_SUBCMD_AFC_EVENT:
                                if (nl80211_vendor_event_qca_parse_afc_evet(ifidx, data, len, outbuf) == 0){
                                    /*Successfully handled the sub command*/
                                    return 0;
                                }
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
            }
            break;

        default:
            break;
    }

    return 1;
}
#endif

/* Init Fucnction to handle private ioctls*/
int wlanif_cfg80211_init(struct wlanif_config *cfg80211_conf)
{
    struct wlanif_cfg80211_priv * cfgPriv;

    int ret;
    /* Initialize debug module used for logging */
    soncfgDbgS.dbgModule = dbgModuleFind("libsoncfg");
    soncfgDbgS.dbgModule->Level=DBGERR;

    assert(cfg80211_conf != NULL);

    cfg80211_conf->IsCfg80211 = 1;
    cfg80211_conf->ctx = malloc(sizeof(struct wlanif_cfg80211_priv));

    if (cfg80211_conf->ctx == NULL)
    {
        printf("%s: Failed\n",__func__);
        return -ENOMEM;
    }

    cfgPriv = (struct wlanif_cfg80211_priv *) cfg80211_conf->ctx;

    assert(cfgPriv != NULL);

    /* Fill the private socket id for command and events */
    cfgPriv->cfg80211_ctx_qca.pvt_cmd_sock_id = cfg80211_conf->pvt_cmd_sock_id;
    cfgPriv->cfg80211_ctx_qca.pvt_event_sock_id = cfg80211_conf->pvt_event_sock_id;

    if ( (cfg80211_conf->pvt_event_sock_id == WSPLCD_NL80211_EVENT_SOCK)
            || (cfg80211_conf->pvt_event_sock_id == LIBSTORAGE_NL80211_EVENT_SOCK) )
    {
        cfgPriv->cfg80211_ctx_qca.event_callback = NULL;
    }
    else {
        cfgPriv->cfg80211_ctx_qca.event_callback = cfg80211_event_callback;
    }
    /*Initializing event related members to zero for not event supporting module*/
    cfgPriv->cfg80211_ctx_qca.event_thread_running = 0;
    cfgPriv->cfg80211_ctx_qca.event_sock = NULL;

    ret = wifi_init_nl80211(&(cfgPriv->cfg80211_ctx_qca));
    if (ret) {
        dbgf(soncfgDbgS.dbgModule, DBGERR, "%s : unable to create NL socket\n",__func__) ;
        return -EIO;
    }

    return 0;
}

/* Destroy the intialized context for cfg80211*/
void wlanif_cfg80211_deinit(struct wlanif_config *cfg80211_conf)
{
    struct wlanif_cfg80211_priv * cfgPriv;

    assert(cfg80211_conf != NULL);

    cfgPriv = (struct wlanif_cfg80211_priv *) cfg80211_conf->ctx;

    wifi_destroy_nl80211(&(cfgPriv->cfg80211_ctx_qca));

    free(cfg80211_conf->ctx);
}


int setWideBandScan_cfg80211(void *ctx, const char *ifname, int value)
{
    int ret;
    struct wlanif_cfg80211_priv * cfgPriv;

    TRACE_ENTRY();

    cfgPriv = (struct wlanif_cfg80211_priv *) ctx;
    assert(cfgPriv != NULL);

    if (( ret = send_command_set_cfg80211(&(cfgPriv->cfg80211_ctx_qca), ifname,
                                   (OL_ATH_PARAM_WIDE_BAND_SCAN | OL_ATH_PARAM_SHIFT),
                                   &value, sizeof(int))) < 0)
    {
        TRACE_EXIT_ERR();
        return ret;
    }

    TRACE_EXIT();
    return 0;
}
