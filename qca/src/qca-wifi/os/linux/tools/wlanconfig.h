/*
 * Copyright (c) 2017-2018 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 */

#ifndef _WLANCONFIG_H_
#define _WLANCONFIG_H_

#if UMAC_SUPPORT_CFG80211
#ifndef BUILD_X86
#include <netlink/genl/genl.h>
#endif
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <qcatools_lib.h>
#ifdef ANDROID
#include <compat.h>
#endif

#include <ieee80211_external.h>

#define PS_ACTIVITY  1<<0
#define HUMAN_FORMAT 1<<1
#define ALLCHANS     1<<2
#define LIST_STATION_CFG_ALLOC_SIZE 5500
#define LIST_STATION_IOC_ALLOC_SIZE 0xFFFF

#define WLANCONFIG_NL80211_CMD_SOCK_ID DEFAULT_NL80211_CMD_SOCK_ID
#define WLANCONFIG_NL80211_EVENT_SOCK_ID DEFAULT_NL80211_EVENT_SOCK_ID

#define	IEEE80211_C_BITS \
    "\020\1WEP\2TKIP\3AES\4AES_CCM\6CKIP\7FF\10TURBOP\11IBSS\12PMGT\13HOSTAP\14AHDEMO" \
"\15SWRETRY\16TXPMGT\17SHSLOT\20SHPREAMBLE\21MONITOR\22TKIPMIC\30WPA1" \
"\31WPA2\32BURST\33WME"

/*
 * These are taken from ieee80211_node.h
 */

#define IEEE80211_NODE_TURBOP   0x0001		/* Turbo prime enable */
#define IEEE80211_NODE_AR       0x0010		/* AR capable */
#define IEEE80211_NODE_BOOST    0x0080
#define MACSTR_LEN 18
#define IEEE80211_MAX_2G_CHANNEL 14

void usage(void);

int handle_command_create  (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_destroy (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_list    (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_nawds   (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_hmwds   (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_ald     (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_wnm     (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_hmmc    (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_hmmc_v6    (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_deny_list    (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_deny_list_v6    (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_vendorie(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_addie   (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_atfstat (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_nac     (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_nac_rssi(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_peer_isolation(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_atf     (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_cfr     (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_flow_tag(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_mu_snif(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_rtt     (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_mlnawds (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_mesh    (int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
#ifdef CONFIG_SAWF_DEF_QUEUES
int handle_command_service_class(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
#endif
#ifdef CONFIG_SAWF
int handle_command_sawf_telemetry(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_sawf_sla(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_sawf_sla_detect(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
#endif
#ifdef ATH_BUS_PM
static int suspend(const char *ifname, int suspend);
#endif
#ifdef WLAN_FEATURE_11BE
int handle_command_t2lm(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_link_map_config(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_epcs(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
#endif
#ifdef WLAN_WIFI_RADAR_ENABLE
int handle_command_wifi_radar(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
#endif
typedef void (*cfg80211_cb) (struct cfg80211_data *buffer);

#endif
int handle_command_tdma_schedule(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_enable_ol_stats(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_create_pasn_peer(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int get_mac_addr(const char *src, char *dest);
int handle_command_set_ranging_context(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_send_pasn_auth_status(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
int handle_command_delete_pasn_peer(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx);
