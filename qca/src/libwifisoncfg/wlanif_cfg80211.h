/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef _CFG80211_NL_H
#define _CFG80211_NL_H

#include "wlanif_cmn.h"
#include "cfg80211_nlwrapper_pvt.h"
#include "qca-vendor.h"
#include "ieee80211_ioctl.h"
#include <dbg.h>

extern int finish_handler(struct nl_msg *msg, void *arg);
extern int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg);
extern int valid_handler(struct nl_msg *msg, void *arg);
extern int ack_handler(struct nl_msg *msg, void *arg);

#define MAX_CMD_LEN 128
static const unsigned NL80211_ATTR_MAX_INTERNAL = 256;
#define WIRELESS_EXT (22)

struct wdev_info {
    enum nl80211_iftype nlmode;
    char name[IFNAMSIZ];
    struct ether_addr mldaddr;
};

struct wlanif_cfg80211_cmn {
    struct nl_sock *gen_sock;
    int drv_id;
};

/* For debugging support */
struct soncfgDbg_t {
    struct dbgModule *dbgModule;
} soncfgDbgS;

/*Structure for private and public ioctls*/
struct wlanif_cfg80211_priv {
    /* QCA Context for all vendor specfic nl80211 message */
    wifi_cfg80211_context cfg80211_ctx_qca;
    /* Generic nl80211 context */
    struct wlanif_cfg80211_cmn cfg80211_ctx_cmn;
};

struct socket_context {
    u_int8_t cfg80211; /* cfg80211 enable flag */

    wifi_cfg80211_context cfg80211_ctxt; /* cfg80211 context */

    int sock_fd; /* wext socket file descriptor */
};

/**
 * @brief Populated to send radar event to driver.
 */
struct radarhandler {
    int s;
    struct ath_diag atd;
    struct socket_context sock_ctx;
};

typedef enum config_mode_type {
    CONFIG_IOCTL    = 0, /* driver config mode is WEXT */
    CONFIG_CFG80211 = 1, /* driver config mode is cfg80211 */
    CONFIG_INVALID  = 2, /* invalid driver config mode */
} config_mode_type;

/********************* Provisioning Event *************************/
#define MAX_PREFERRED_LINKS 4
#define NUM_TIDS 8
#define MAX_AC 4

enum link_band_caps_e {
    BAND_INVALID = 0,
    BAND_2GHZ = 1,
    BAND_5GHZ = 2,
    BAND_5GHZ_LOW = 3,
    BAND_5GHZ_HIGH = 4,
    BAND_6GHZ = 5,
    BAND_6GHZ_LOW = 6,
    BAND_6GHZ_HIGH = 7,
    BAND_MAX,
};

enum t2lm_direction_e {
    T2LM_INVALID_DIRECTION = 0,
    T2LM_DL_DIRECTION = 1,
    T2LM_UL_DIRECTION = 2,
    T2LM_BIDI_DIRECTION = 3,
    T2LM_MAX_DIRECTION,
};

struct lib_t2lm_of_tids_t {
    enum t2lm_direction_e direction;
	u_int16_t t2lm_provisioned_links[NUM_TIDS];
};

struct preferred_links_t {
    uint8_t peer_mld_mac_addr[6];
    uint8_t num_t2lm_of_tids;
    int8_t homogeneous_mapping;
    struct lib_t2lm_of_tids_t t2lm[T2LM_MAX_DIRECTION];
    uint8_t num_pref_links;
    enum link_band_caps_e preffered_link_order[MAX_PREFERRED_LINKS];
    uint32_t max_latency[MAX_AC];
};

#endif
