/*
 * @File: wsplcd.h
 *
 * @Abstract: AP AutoConfig/wsplcd header file
 *
 * @Notes:  IEEE1905 AP Auto-Configuration Daemon
 *          AP Enrollee gets wifi configuration from AP Registrar via
 *          authenticated IEEE1905 Interfaces
 *
 * Copyright (c) 2011-2012, 2015-2018 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * 2011-2012, 2015-2016 Qualcomm Atheros, Inc.
 *
 * Qualcomm Atheros Confidential and Proprietary.
 * All rights reserved.
 *
 */


#ifndef _WSPLCD_H
#define _WSPLCD_H
#include "includes.h"
#include <signal.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <linux/if_packet.h>
#include <netinet/if_ether.h>
#include <linux/if_vlan.h>
#include "defs.h"
#include "common.h"
#include <linux/rtnetlink.h>
//#include "wireless_copy.h"
#include "wpa_common.h"
#include "eap_defs.h"
#include "l2_packet.h"
#include "wps_parser.h"
#ifdef HYFI10_COMPATIBLE
#include "legacy_ap.h"
#endif

#include "wps_config.h"
#include "apac_hyfi20_atf.h"

#ifdef SON_MEMORY_DEBUG

#include "qca-son-mem-debug.h"
#undef QCA_MOD_INPUT
#define QCA_MOD_INPUT QCA_MOD_WSPLCD
#include "son-mem-debug.h"

#endif /* SON_MEMORY_DEBUG */

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#include <wlan_son_ioctl.h>

struct eap_session;
typedef struct l2_ethhdr L2_ETHHDR;
typedef struct ieee802_1x_hdr IEEE8021X_HDR;
typedef struct eap_hdr EAP_HDR;
typedef struct eap_format EAP_FORMAT;

#define EAPOL_MULTICAST_GROUP	{0x01,0x80,0xc2,0x00,0x00,0x03}


#define MODE_CLIENT 0x00
#define MODE_SERVER 0x01
#define MODE_NONE   0x02

/* RF Band */
u32 WPS_RFBAND_5GHZ_LOW;
u32 WPS_RFBAND_6GHZ;
u32 WPS_RFBAND_6GHZ_LOW;

/* Frequency */
#define APAC_NUM_WIFI_FREQ 6
u32 APAC_WIFI_FREQ_2;
u32 APAC_WIFI_FREQ_5;
u32 APAC_WIFI_FREQ_60;
u32 APAC_WIFI_FREQ_5_OTHER;
u32 APAC_WIFI_FREQ_6;
u32 APAC_WIFI_FREQ_6_OTHER;
u32 APAC_WIFI_FREQ_INVALID;

// Default Push Button Ignore Duration (in seconds)
#define PUSH_BUTTON_IGNORE_DUR      10


#define WSPLC_NLMSG_IFNAME   "eth0"
#define WSPLC_EAPMSG_TXIFNAME "eth0"
#define WSPLC_EAPMSG_RXIFNAME "br0"

#define WSPLC_CLONE_TIMEOUT  180
#define WSPLC_WALK_TIMEOUT    120
#define WSPLC_REPEAT_TIMEOUT  1
#define WSPLC_INTERNAL_TIMEOUT 15
#define WSPLC_ONE_BUTTON     1
#define WSPLC_TWO_BUTTON    2

#define MAX_SSID_LEN        32
#define MAX_DOT1X_TAG_LEN   128

// WPA3 configuration data max limits
#define MAX_PASSPHRASE_LEN  64
#define MAX_SAE_PASSWORD_LEN  256
#define MAX_SEC_GROUPS_LEN  32
#define MAX_AUTH_SERVER_IP_LEN 16
#define MAX_AUTH_SECRET_LEN 128
#define MAX_MOBILITYDOMAIN_LEN 128
#define MAX_KH_KEY_HEX_LEN 128


#if MAP_ENABLED
#define MAX_RADIO_CONFIGURATION 5
#elif SON_ENABLED
#define MAX_RADIO_CONFIGURATION 3
#endif
#define MAX_WLAN_CONFIGURATION 48
#define MAX_VAP_PER_BAND 16

#define DEFAULT_WPA_GET_VALUE_RETRY_COUNT 5

#define DEVICE_INVENTORY_CONFIG_MAX_LEN 48

#define MAX_AFFILIATED_AP_LINKS 2

#define MAX_AFFILIATED_BSTA_LINKS 2

/*
 * hyCopyMACAddr - Copy MAC address variable
 */
#define hyCopyMACAddr(src, dst) memcpy( dst, src, IEEE80211_ADDR_LEN )

/*
 * hyAreEqualMACAddrs - Compare two MAC addresses (returns 1 if equal)
 */
#define hyAreEqualMACAddrs(arg1, arg2) (!memcmp(arg1, arg2, IEEE80211_ADDR_LEN))


/* SLO definitions*/
#define APAC_MAC_STR_LENGTH 18

typedef struct _wsplcd_config {
    u32     ssid_len;
    char    ssid[MAX_SSID_LEN];
    u16     auth;
    u16     encr;
    u32     passphraseLen;
    u8      passphrase[MAX_PASSPHRASE_LEN];
    u32    clone_timeout;
    u32    walk_timeout;
    u32    repeat_timeout;
    u32    internal_timeout;
    u32    button_mode;
    int     debug_level;

} WSPLCD_CONFIG;

typedef struct _wsplcd_data {
    int     mode;
    int     nlSkt;
    int     txSkt;
    int     rxSkt;
    int     txIfIndex;
    int     nlIfIndex;
    char   txIfName[IFNAMSIZ];
    char   rxIfName[IFNAMSIZ];
    char   nlIfName[IFNAMSIZ];
    u8     own_addr[ETH_ALEN];
    WSPLCD_CONFIG   wsplcConfig;

    int clone_running;
    struct eap_session* sess_list;
    struct wpa_sup* wpas;
} wsplcd_data_t;

/* stdio.h(gcc-5.2) brought dprintf() prototype which
 * is different from our existing dprintf() function
 * prototype, This causes compile time conflicts types
 * for dprintf().
 * Hence we will make our dpritf() prototype same as what
 * stdio.h is having.  */
int dprintf(int level, const char *fmt, ...);
void shutdown_fatal(void);

#ifdef HYFI10_COMPATIBLE
int wsplc_disable_cloning(wsplcd_data_t* wspd);
int wsplc_stop_cloning(wsplcd_data_t* wspd);
int wsplc_is_cloning_runnig(wsplcd_data_t* wspd);
int wsplcd_hyfi10_init(wsplcd_data_t* wspd);
int wsplcd_hyfi10_startup(wsplcd_data_t* wspd);
int wsplcd_hyfi10_stop(wsplcd_data_t* wspd);
#endif

/****************************************************
 ****************************************************
 **** Hyfi2.0 / IEEE1905 AP Auto-Configuration ******
 ****************************************************
 ****************************************************/

#include "ieee1905_defs.h"

#define APAC_SEARCH_TIMEOUT                 60
#define APAC_PUSHBUTTON_TIMEOUT            120
#define APAC_RM_COLLECT_TIMEOUT             10
#define APAC_PB_SEARCH_TIMEOUT              10
#define APAC_CHANNEL_POLLING_TIMEOUT        10

#define APAC_WPS_SESSION_TIMEOUT            30
#define APAC_WPS_RETRANSMISSION_TIMEOUT      5
#define APAC_WPS_MSG_PROCESSING_TIMEOUT     15
#define APAC_WPS_PF_RETRANSMISSION_TIMEOUT  15

#define APAC_MAXNUM_HYIF                    45  /* number of Hyfi/1905 interfaces assuming 3 radio and 15 BSSeS per Radio */
#define APAC_MAXNUM_NTWK_NODES              64  /* number of nodes in network */
#define MAX_NW_KEY_LEN                     256
#define APAC_MAX_VLAN_SUPPORTED              4
#define APAC_MAX_MLO_GROUP_SUPPORTED         IEEE1905_QCA_VENDOR_MAX_BSS * IEEE1905_QCA_VENDOR_MAX_RADIOS
#define MAX_BASIC_RADIO_CAP_LEN            128

#define APAC_CONF_FILE_PATH         "/tmp/wsplcd.conf"
#define APAC_PIPE_PATH              "/var/run/wsplc.pipe"
#define APAC_PIPE_SECONDARY_PATH    "/var/run/wsplc_sec_%d.pipe"
#define APAC_LOG_FILE_PATH          "/tmp/wsplcd.log"
#define APAC_LOCK_FILE_PATH         "/var/run/wsplcd.lock"

#ifndef WSPLCD_DBG_FILE_SIZE
#define WSPLCD_DBG_FILE_SIZE               "WSPLCD_DBG_FILE_SIZE"
#endif

#if MAP_ENABLED
#define APAC_MAP_CONF_FILE          "/etc/config/map.conf"
#endif
#define APAC_CONF_FILE_NAME_MAX_LEN 128
#define APAC_PIPE_NAME_MAX_LEN 128
/* TODO: use local variable instead of this global one */
extern u16 g_wsplcd_instance;
extern char g_log_file_path[APAC_CONF_FILE_NAME_MAX_LEN];
extern char g_cfg_file[APAC_CONF_FILE_NAME_MAX_LEN];
#if MAP_ENABLED
extern char g_map_cfg_file[APAC_CONF_FILE_NAME_MAX_LEN]; //to hold map config
extern char g_map_cfg_bsta_file[APAC_CONF_FILE_NAME_MAX_LEN]; //to hold map bsta config
#endif

/* IEEE1905 defined */
#define APAC_MULTICAST_ADDR         IEEE1905_MULTICAST_ADDR
#define APAC_ETH_P_IEEE1905         IEEE1905_ETHER_TYPE

#define APAC_TLVLEN_ROLE            sizeof(u8)
#define APAC_TLVLEN_FREQ            sizeof(u8)

#define APAC_MID_DELTA              64
#define AVLN_LEN                    7

/* Default maximum number of seconds to wait for APAC completes on each
 * band since the first Wi-Fi configuration gets stored.
 * It will only be used when the configuration parameter not provided. */
#define APAC_WAIT_WIFI_CONFIG_SECS_OTHER            20

#define APAC_Prefered5GLChannel                     0
#define APAC_Prefered5GHChannel                     0
#define APAC_Prefered6GChannel                      0

/* Default maximum number of seconds to wait for APAC completes on the first
 * band after WPS success.
 * It will only be used when the configuration parameter not provided. */
#define APAC_WAIT_WIFI_CONFIG_SECS_FIRST            30

#if MAP_ENABLED
#define MAP_BSS_TYPE_TEARDOWN  0x10
#define MAP_BSS_TYPE_FRONTHAUL 0x20
#define MAP_BSS_TYPE_BACKHAUL  0x40
#define MAP_BSS_TYPE_BSTA      0x80
#define MAP2_R1_BSTA_ASSOC_DISALLOW 0x08
#define MAP2_R2_ABOVE_BSTA_ASSOC_DISALLOW 0x04
#endif

#define APAC_MAX_NUM_SEG2_CENTERS 3

#define APAC_INDEX_160_SEG2_CENTER 0
#define APAC_INDEX_320_SEG2_CENTER1 1
#define APAC_INDEX_320_SEG2_CENTER2 2
#define APAC_INVALID_SEG2_CENTER_INDEX 0xF

/* log file mode */
typedef enum apacLogFileMode_e {
    APAC_LOG_FILE_APPEND,
    APAC_LOG_FILE_TRUNCATE,

    APAC_LOG_FILE_INVALID
} apacLogFileMode_e;


typedef enum apacWsplcdInstance_e {
    APAC_WSPLCD_INSTANCE_PRIMARY,
    APAC_WSPLCD_INSTANCE_SECONDARY,
    APAC_WSPLCD_INSTANCE_3,
    APAC_WSPLCD_INSTANCE_4,
    APAC_WSPLCD_INSTANCE_INVALID
} apacWsplcdInstance_e;

/* boolean */
typedef enum apacBool_e {
    APAC_FALSE = 0,
    APAC_TRUE = !APAC_FALSE
} apacBool_e;
#define APAC_CONFIG_STA         APAC_TRUE


/* device type */
typedef enum apacHyfi20DeviceType_e {
    APAC_IEEE1905,
    APAC_HYFI10
} apacHyfi20DeviceType_e;

/* WPS method used in registration. Only Registrar can control it */
typedef enum {
    APAC_WPS_M2,
    APAC_WPS_M8
} apacHyfi2020WPSMethod_e;
#define APAC_WPS_METHOD             APAC_WPS_M2


typedef enum apacHyfi20Role_e {
    APAC_REGISTRAR,
    APAC_MAP_CONTROLLER = APAC_REGISTRAR,
    APAC_ENROLLEE,
    APAC_MAP_AGENT = APAC_ENROLLEE,
#ifdef QCA_EXT_EM_CONTROLLER
    MultiAPEmexController = 0xA0,   /* EM+ Controller service supported */
    MultiAPEmexAgent      = 0xA1,   /* EM+ Agent service supported */
#endif
    APAC_OTHER = ~0
} apacHyfi20Role_e;

typedef enum apacHyfi20WifiBandType_e {
    APAC_WIFI_BANDTYPE_FULL,
    APAC_WIFI_BANDTYPE_HIGH,
    APAC_WIFI_BANDTYPE_LOW,
    APAC_WIFI_BANDTYPE_INVALID
} apacHyfi20WifiBandType_e;

typedef enum apacHyfi20WlanDeviceMode_e {
    APAC_WLAN_AP,
    APAC_WLAN_STA,

    APAC_INVALID_DEVICE_MODE = ~0
} apacHyfi20WlanDeviceMode_e;

enum {
    APAC_SB,
    APAC_DB,
    APAC_DBDC
};

typedef enum apacHyfi20MediaType_e {
    APAC_MEDIATYPE_ETH,
    APAC_MEDIATYPE_WIFI,
    APAC_MEDIATYPE_PLC,
    APAC_MEDIATYPE_MOCA,
    APAC_MEDIATYPE_WIFI_VLAN,
    APAC_MEDIATYPE_MLD_VLAN,

    APAC_MEDIATYPE_INVALID = ~0
} apacHyfi20MediaType_e;

typedef enum {
    /* Enrolle with PB mode */
    APAC_E_PB_IDLE = 0,     /* 0 PB de-activated */
    APAC_E_PB_WAIT_RESP,    /* 1 set Search message is sent */
    APAC_E_PB_WPS,          /* 2 set after M1 is sent */

    /* Enrollee with AP Auto Config mode */
    APAC_E_IDLE,            /* 3 initial state, set when Registration is done */
    APAC_E_WAIT_RESP,       /* 4 set after Search message is sent */
    APAC_E_WPS,             /* 5 set after sending M1 */

    /* Registrar with PB mode */
    APAC_R_PB_IDLE,             /* 6 PB de-activated */
    APAC_R_PB_WAIT_SEARCH,      /* 7 set when PB is activated */
    APAC_R_PB_WAIT_EARLY_AP_CAP,   /* 8 set when Early AP Cap is expected from r6 Agent */
    APAC_R_PB_WAIT_M1,          /* 9 set when Response message is sent */
    APAC_R_PB_WPS,

    /* Registrar with AP Auto Config mode */
    APAC_R_NO_PB,               /* 10 */

    APAC_INVALID_STATE = ~0
} apacHyfi20State_e;

typedef enum apacHyfi20MLDRole_e {
    APAC_AP_MLD,
    APAC_NON_AP_MLD,
} apacHyfi20MLDRole_e;

#if MAP_ENABLED
typedef enum apacHyfi20MapVersion_e {
    APAC_MAP_VERSION_1 = 1,
    APAC_MAP_VERSION_2,
    APAC_MAP_VERSION_3,
    APAC_MAP_VERSION_4,
    APAC_MAP_VERSION_5,
    APAC_MAP_VERSION_6,
    APAC_MAP_VERSION_INVALID = 0xff
} apacHyfi20MapVersion_e;

typedef enum apacHyfi20MapV2Provisioning_e {
    APAC_MAP_DPP_SUPPORT_ENABLED = 0x00,
    APAC_MAP_PROVISION_SUPPORT_RESERVED =0x01,
    APAC_MAP_PROVISION_SUPPORT_RESERVED_MAX = 0xff
} apacHyfi20MapV2Provisioning_e;

typedef enum apacHyfi20MapV2IntrigityAlgo_e {
    APAC_MAP_HMAC_SHA256 = 0x00,
    APAC_MAP_INTRIGITY_ALGO_RESERVED = 1,
    APAC_MAP_INTRIGITY_ALGO_RESERVED_MAX= 0xff,
} apacHyfi20MapV2IntrigityAlgo_e;

typedef enum apacHyfi20MapV2EncryptionAlgo_e {
    APAC_MAP_AES_SIV = 0x00,
    APAC_MAP_ENCRYPTION_ALGO_RESERVED = 1,
    APAC_MAP_ENCRYPTION_ALGO_RESERVED_MAX= 0xff,
} apacHyfi20MapV2EncryptionAlgo_e;

typedef struct apacMapAP_t {
    char    ssid[MAX_SSID_LEN+1];
    u32     ssid_len;
    u16     auth;
    u16     encr;
    u8      nw_key_index;
    char    nw_key[MAX_NW_KEY_LEN+1];
    u32     nw_key_len;
    u8      ap_mac[ETH_ALEN];
    u8      passphrase[MAX_PASSPHRASE_LEN+1];
    u32     passphraseLen;
    u8      new_password[MAX_PASSPHRASE_LEN+1];
    u32     new_password_len;
    u32     device_password_id;
    u32     key_wrap_authen;
    u8      mapBssType;
    u8      validTSPolicy;
    int16_t vlanID;
    int16_t vlan8021Q;
    char    nw_name[IFNAMSIZ+1];
    u8      sae;
    char    sae_password[MAX_SAE_PASSWORD_LEN+1];
    u32     sae_password_len;
    u32     sae_anticloggingthreshold;
    u32     sae_sync;
    char    sae_groups[MAX_SEC_GROUPS_LEN+1];
    u8      sae_requireMFP;
    u32     vap_disable_steering;
    u8      en_6g_sec_comp;
    u8      en_6g_sec_comp_set;
    u8      sae_pwe;
#if DOT1X_SUPPORT
    /// dot1x suiteb
    u8      suite_b;

    /// dot1x auth primary port number
    u32     auth_port;

    /// dot1x auth primary radious server address
    char    auth_server[MAX_AUTH_SERVER_IP_LEN+1];

    /// dot1x auth primary radious secret
    char    auth_secret[MAX_AUTH_SECRET_LEN+1];

    /// dot1x auth secondary port number
    u32     auth_port2;

    /// dot1x auth secondary radious server
    char    auth_server2[MAX_AUTH_SERVER_IP_LEN+1];

    /// dot1x auth secondary radious secret
    char    auth_secret2[MAX_AUTH_SECRET_LEN+1];

    /// dot1x rsn preauth interface name
    char    rsn_preauth_ifaces[IFNAMSIZ+1];

    /// dot1x group management cipher
    char    group_mgmt_cipher[IFNAMSIZ+1];

    /// dot1x wpa group rekey value
    u32     group_rekey_period;

    /// dot1x ieee8021w value
    u8      ieee80211w;

    /// dot1x rsn preauth value
    u8      rsn_preauth;

    /// flag to enable ft authentication
    u8      ft_auth;

    /// 256 bit key
    char kh_key_hex[MAX_KH_KEY_HEX_LEN+1];

    /// mobility domain
    char mobility_domain[MAX_MOBILITYDOMAIN_LEN+1];

#endif

    u8      enableMLO;
    u8      enableMLObSta;

#define APAC_STD_MAX_LEN 20
    u_int8_t standard_len;

    char standard[APAC_STD_MAX_LEN];

    u8 enable_ht40coext;

    u8 ht40coext;

    u8 skipvaprestart;

    u8 enable_hidden;

    u8 hidden;

#ifdef QCA_EXT_EM_CONTROLLER
    u8 isolate;
#endif

} apacMapAP_t;

typedef struct apacMapEProfile_t {
    /// The SSID to configure
    const char *ssid;

    /// The type of authentication to configure.
    ///
    /// This should be one of the WPS_AUTH_* values.
    u16 auth;

    /// The type of encryption to configure.
    ///
    /// This should be one of the WPS_ENCR_* values.
    u16 encr;

    /// The passphrase (if any) as a string.
    const char *nw_key;

    /// Flag indicating whether to instantiate a backhaul BSS when matching
    /// this profile.
    apacBool_e isBackhaul;

    /// Flag indicating whether to instantiate a fronthaul BSS when matching
    /// this profile.
    apacBool_e isFronthaul;

    /// Flag indicating whether to instantiate a bSTA when matching
    /// this profile.
    apacBool_e isBsta;

    /// Flag indicating whether to instantiate a Profile 1 Backhaul STA
    //  association disallowed
    apacBool_e map1bSTAAssocDisallowed;

    /// Flag indicating whether to instantiate a Profile 2 Backhaul STA
    //  association disallowed
    apacBool_e map2bSTAAssocDisallowed;

    /// The Default 802.1Q Setting VLAN ID
    int16_t primaryVlanID;

    /// The Default PCP
    int8_t pcp;

    /// VLAN ID
    int16_t vlanID;

    /// MLO Group
    const char *mldGrouping;

    /// RUID : Need this info to populate bsta mld TLV
    struct ether_addr bstaRuids[MAX_RADIO_CONFIGURATION];

    /// Num of bsta radios
    uint8_t numBstaRadios;

#if DOT1X_SUPPORT
    /// dot1x radious primary server address
    const char *auth_primary_server;

    /// dot1x radious primary secret password
    const char *auth_primary_secret;

    /// dot1x radious secondary server address
    const char *auth_secondary_server;

    /// dot1x radious secondary secret password
    const char *auth_secondary_secret;

    /// dot1x rsn preauth interface details
    const char *rsn_preauth_interfaces;

    /// group management cipher
    const char *group_mgmt_cipher;

    /// wpa reky value
    uint32_t group_rekey_period;

    /// dot1x primary port number
    uint16_t auth_primary_port;

    /// dot1x secondary port number
    uint16_t auth_secondary_port;

    /// dot1x suiteb
    uint8_t suite_b;

    /// dot1x rsn preauth value
    uint8_t rsn_preauth;

    /// dot1x iee80211 value
    uint8_t ieee80211w;

    /// flag to enable ft authentication
    u8      ft_auth;

    /// 256 bit key
    const char *kh_key_hex;

    /// mobility domain
    const char *mobility_domain;

#endif
} apacMapEProfile_t;

/**
 * @brief The type of matching used for the encryption profiles that are
 *        currently in use.
 *
 * Two methods of specifying the encryption profile are supported. The AL-
 * specific one requires an entry for each abstraction layer MAC address the
 * controller may need to configure. The generic one does not require MAC
 * addresses and instead matches based on the operating class capabilities
 * of the radios.
 */
typedef enum {
    /// Abstraction layer MAC address specific profiles
    APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC,

    /// Generic profiles that are not tied to an AL MAC and instead match
    /// against operating classes
    APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC
} apacMapEProfileMatcherType_e;

/**
 * @brief Encryption profile for a specific SSID.
 *
 * This is identified by an SSID key which is a shorthand way to refer to
 * the SSID.
 */
typedef struct apacMapEProfileSSID_t {
    /// The unique key used to identify this SSID in the profile lines
    const char *ssidKey;

    /// The encryption profile info (SSID, passphrase, etc.)
    apacMapEProfile_t eprofile;
} apacMapEProfileSSID_t;

/**
 * @brief Encryption profile for a specific MLO.
 *
 * This is identified by an MLO key which is a shorthand way to refer to
 * the MLO grouping for an SSID.
 */
typedef struct apacMapEProfileMLO_t {

    /// The unique key used to identify the MLO group
    const char *mloKey;

    /// The unique key used to identify this SSID is grouped to a MLO
    const char *ssidKey;

} apacMapEProfileMLO_t;

/// The number of operating class ranges that can be included in a single
/// BSS instantiation profile for the Multi-AP SIG config file.
#define APAC_MAP_PROFILE_MAX_OP_CLASS_RANGES 3

/**
 * @brief Configuration data for instantiating BSSes when operating in
 *        Multi-AP SIG mode.
 *
 * A profile entry has matching criteria in one of two modes along with the
 * necessary SSID, auth and encryption modes, passphrase, and whether to
 * create an fBSS and/or bBSS.
 */
typedef struct apacMapEProfileMatcher_t {
    /// The type of profile represented.
    ///
    /// This value determines which of the members in the union is active.
    apacMapEProfileMatcherType_e matcherType;

    /// Whether to terminate the search through the profiles upon a match.
    apacBool_e terminateMatching;

    union {
        /// Parameters for matching when the AL MAC is specified
        struct {
            /// Abstraction layer MAC address as a hex string (with no
            /// colons).
            const char *alId;

            /// An operating class specification.
            ///
            /// This must be one of "8x", "11x", and "12x".
            const char *opclass;

            /// The encryption profile for the BSS to instantiate.
            apacMapEProfile_t eprofile;
        } alParams;

        /// Parameters for matching a generic profile (where the AL MAC is
        /// not specified)
        struct apacMapEProfileMatcherGenericParams_t {
            /// The number of operating class ranges specified.
            ///
            /// If this is more than one, then the radio must indicate support
            /// for all of the operating classes for this profile to be
            /// considered a match.
            u8 numOpClassRanges;

            struct {
                /// The minimum operating class to match (inclusive).
                u8 minOpClass;

                /// The maximum operating class to match (inclusive).
                u8 maxOpClass;
            } opClassRanges[APAC_MAP_PROFILE_MAX_OP_CLASS_RANGES];

            apacHyfi20WifiBandType_e bandType;
            /// The index of the SSID to instantiate
            u8 ssidIndex;
            u8 mibVAPIndex;
            apacBool_e mloEnabled;
        } genericParams;
    } typeParams;
} apacMapEProfileMatcher_t;

/// The format of the Multi-AP config file.
///
/// Ideally this would not be global, but the way the command line parsing
/// is implemented right now, that seems best (since otherwise we would have
/// to put it into the config structure which is really just supposed to be
/// for Hy-Fi 2.0 data).
extern apacMapEProfileMatcherType_e g_map_cfg_file_format;

typedef struct apacPifMap {
    mapapcap_t apcap;
    apacBool_e htCapabilitiesValid;
    map_ap_ht_capabilities_t aphtcap;
    apacBool_e vhtCapabilitiesValid;
    map_ap_vht_capabilities_t apvhtcap;
    apacBool_e heCapabilitiesValid;
    map_ap_he_capabilities_t aphecap;
    apacBool_e wifi6CapabilitiesValid;
    map_ap_wifi6_capabilities_t apwifi6cap;
    apacBool_e chanscanCapabilitiesValid;
    map_op_chan_t chanscancap;

} apacPifMap_t;
#endif

#if SON_ENABLED
/* Password Management flags */
typedef enum {
    APAC_PASSWORD_ADD,
    APAC_PASSWORD_DEL,
    APAC_PASSWORD_MATCH
} apachyfi20PasswordFlag;

// Below structure is copied from qca-hostap/src/ap/ap_config.h
struct sae_password_entry {
    struct sae_password_entry *next;
    char *password;
    char *identifier;
    u8 peer_addr[ETH_ALEN];
    int vlan_id;

    // Below members are added for SON functionality
    int changed;    // Send only updates to libstorage
    char *pwd;
};

typedef struct sae_password_entry apacHyfi20SAEPassword_t;

struct string_list_entry {
    struct string_list_entry *next;
    char *data;
};
typedef struct string_list_entry apacHyfi20StringList_t;
#endif

typedef enum {
    APAC_MLD_TYPE_NOT_SET,
    APAC_SLO_MLD,
    APAC_MLO_MLD
} apacHyfi20MLDType_e;

/* MLD information */
typedef struct apacHyfi20MLD_t {
    apacBool_e valid;
    u8      mld_mac[ETH_ALEN];
    char    ssid[MAX_SSID_LEN+1];
    u32     ssid_len;
    apacHyfi20MLDRole_e role;
    apacBool_e configured;
    apacHyfi20MLDType_e mld_type;
    apacBool_e usage;
    apacBool_e newVap;
    int radioIdxBitMap;   /* use as bit field */
} apacHyfi20MLD_t;

/* AP information */
typedef struct apacHyfi20AP_t {
    apacBool_e valid;
    apacBool_e isAutoConfigured;
    apacBool_e isDualBand;
    u32 freq;
    s32      vap_index;
    char     *ifName;
    apacBool_e isStaOnly;
    apacBool_e isMLORadio;

    /* Wifi encryption settings (IEEE1905 Table 10-1); info read from MIB */
    char    ssid[MAX_SSID_LEN+1];
    u32     ssid_len;
    u16     auth;
    u16     encr;
    u8      nw_key_index;
    char    nw_key[MAX_NW_KEY_LEN+1];
    u32     nw_key_len;
    u8      ap_mac[ETH_ALEN];
    u8      passphrase[MAX_PASSPHRASE_LEN+1];
    u32     passphraseLen;
    u8      new_password[MAX_PASSPHRASE_LEN+1];
    u32     new_password_len;
    u32     device_password_id;
    u32     key_wrap_authen;

#if SON_ENABLED
    /* WPA3 security settings */
    apacBool_e is_sae_enabled;
    apacBool_e is_sae_password_set;
    apacBool_e is_sae_anticlogthres_set;
    apacBool_e is_sae_sync_set;
    apacBool_e is_sae_groups_set;
    apacBool_e is_sae_reqmfp_set;
    apacBool_e is_owe_enabled;
    apacBool_e is_hidden_ssid_set;
    apacBool_e is_owe_groups_set;
    apacBool_e is_owe_trans_if_set;
    apacBool_e is_owe_trans_ssid_set;
    apacBool_e is_owe_trans_bssid_set;
    apacBool_e is_suite_b_enabled;
    apacBool_e is_auth_server_set;
    apacBool_e is_auth_secret_set;
    apacBool_e is_auth_port_set;
    apacBool_e is_group_rekey_period_set;
    apacBool_e is_auth_server2_set;
    apacBool_e is_auth_secret2_set;
    apacBool_e is_auth_port2_set;
    apacBool_e is_mobility_domain_set;
    apacBool_e is_kh_key_hex_set;
    apacBool_e is_ieee80211w_set;
    apacBool_e is_key_set;
    apacBool_e is_bh_available; /* To indicate BH is created in local band of enrollee or not*/
    apacBool_e is_sae_pwe_set;
    apacBool_e is_en_6g_sec_comp_set;
    apacBool_e is_rsn_preauth_set;
    apacBool_e is_ft_auth_set;
    apacBool_e is_ieee80211r_set;

    u8      sae;
    char    sae_password[MAX_SAE_PASSWORD_LEN+1];
    u32     sae_password_len;
    u32     sae_anticloggingthreshold;
    u32     sae_sync;
    char    sae_groups[MAX_SEC_GROUPS_LEN+1];
    u8      sae_requireMFP;

    u8      owe;
    u8      hidden_ssid;
    char    owe_groups[MAX_SEC_GROUPS_LEN+1];
    char    owe_transition_ifname[IFNAMSIZ+1];
    char    owe_transition_ssid[MAX_SSID_LEN+1];
    char    owe_transition_bssid[MAX_SSID_LEN+1];
#endif

    u8      suite_b;
    u32     auth_port;
    char    auth_server[MAX_AUTH_SERVER_IP_LEN+1];
    char    auth_secret[MAX_AUTH_SECRET_LEN+1];
    char    mobility_domain[MAX_MOBILITYDOMAIN_LEN+1];
    char    kh_key_hex[MAX_KH_KEY_HEX_LEN+1];
    u32     auth_port2;
    char    auth_server2[MAX_AUTH_SERVER_IP_LEN+1];
    char    auth_secret2[MAX_AUTH_SECRET_LEN+1];
    char    rsn_preauth_ifaces[IFNAMSIZ+1];
    char    group_mgmt_cipher[IFNAMSIZ+1];
    u32     group_rekey_period;

    u8      ieee80211w;
    u8      ieee80211r;
    u8      ft_auth;

#if SON_ENABLED
    s32     son_vap_index[MAX_VAP_PER_BAND];
    u32     backhaul_ap;
    u8      sae_pwe;
    u8      en_6g_sec_comp;
#endif

    /* QCA vendor settings */
    u8      *qca_ext;
    u32     qca_ext_len;
    u32     channel;
#define APAC_STD_MAX_LEN 20
    u_int8_t standard_len;
    char standard[APAC_STD_MAX_LEN];
    u_int8_t bh_standard_len;
    char bh_standard[APAC_STD_MAX_LEN];
#if MAP_ENABLED
    apacPifMap_t pIFMapData;
    u8      mapBssType;
#endif
    u8      is_mld_vap;
    int     radio_index;
    char    *radioName;
    u8      radio_mac[ETH_ALEN];
} apacHyfi20AP_t;

/* information for interface */
typedef struct apacHyfi20IF_t {
    apacBool_e valid;
    apacBool_e is1905Interface;
    apacBool_e nonPBC;   /* whether PBC is disabled */
    apacHyfi20MediaType_e mediaType;
    apacHyfi20WlanDeviceMode_e wlanDeviceMode;
    u32 wifiFreq;

    u8      mac_addr[ETH_ALEN];
    s32     ifIndex;
    char    ifName[IFNAMSIZ];
    s32     sock;

    s32     vapIndex;
    s32     ctrlSock;

    /* The time when last WPS success happens on this interface */
    struct os_time last_wps_success_time;

    char    radioName[IFNAMSIZ];
    char    bridgeName[IFNAMSIZ];

#if SON_ENABLED
    apacHyfi20SAEPassword_t *sae_password_list;
    apacHyfi20StringList_t *sae_groups_list;
    apacHyfi20StringList_t *owe_groups_list;
#endif
#if MAP_ENABLED
    u32 channel;
#endif
    int     mldIndex;
} apacHyfi20IF_t;

/* configurable parameters */
typedef struct apacHyfi20Config_t {
    s32                 debug_level;
    apacHyfi20Role_e    role;
    apacHyfi20State_e   state;
    apacBool_e          config_sta;
    u32                 search_to;
    u32                 pushbutton_to;
    u32                 prefered_low_channel;
    u32                 prefered_high_channel;
    u32                 prefered_6g_channel;

    u32                 wlan_chip_cap;
    apacBool_e          band_sel_enabled;
    u32                 band_choice;
    u32                 rm_collect_to;
    apacBool_e          deep_clone_enabled;
    apacBool_e          deep_clone_no_bssid;
    apacBool_e          traffic_separation_enabled;
    apacBool_e          manage_vap_ind;
    apacBool_e          designated_pb_ap_enabled;

    apacHyfi2020WPSMethod_e wps_method;
    u32                 wps_session_to;
    u32                 wps_retransmit_to;
    u32                 wps_per_msg_to;
    struct wps_config*  wpsConf;

    apacBool_e          hyfi10_compatible;
    apacBool_e          sendOnAllIFs;
    char                ucpk[64+1];
    char                salt[64+1];
    u32                 wpa_passphrase_type;
    char                ssid_suffix[128];

    apacBool_e          pbmode_enabled;
    u32                 pb_search_to;

    /* the maximum number of seconds to wait for APAC completes on all
     * other bands after the first Wi-Fi configuration gets stored */
    u32                 wait_wifi_config_secs_other;

    /* the maximum number of seconds to wait for first Wi-Fi configured
     * after WPS success */
    u32                 wait_wifi_config_secs_first;
    apacBool_e          atf_config_enabled;     /* Enable/Disable ATF configurations */
    u32                 apac_atf_num_repeaters; /* Num Repeaters with ATF Configurations */
    ATF_REP_CONFIG      *atfConf;               /* ATF Configuration */
    u32                 cfg_changed;
    u32                 cfg_restart_short_timeout;
    u32                 cfg_restart_long_timeout;
    u32                 cfg_apply_timeout;
    u32                 cfg_max_renew_retry;
    u32                 cfg_renew_retransmit_interval;
    u8                  enable_NB_tlv;
    char*               nbtlvbuff;
#if MAP_ENABLED
    u8                  configApplyTimerStarted;
    apacBool_e          waitForWifiRestart;
#endif
    apacBool_e          enableAfcChanSelection;
    apacBool_e          configMLOSta;
#ifdef SON_MEMORY_DEBUG
    u8                  enable_mem_debug;       /* Enable/Disable SON Memory debug and also configure debug mode (BitMask- Bit0: Enable Memory debugging, Bit1: Display allocation list, Bit2: Display free list, Bit3: Display filter list ) */
    u8                  enable_audit_only;       /* Enable/Disable Only Auditing */
    u8                  enable_log_write_to_file;   /* Enable/Disable writing log to file and also configure logging mode (BitMask- Bit0: Enable/Disable Logging to file, Bit1: Write Detailed Memory summary information, Bit2: Write Graph data, Bit3: Memory debug tool debugging (for engineering purpose)   ) */
    u32                 report_interval;  /* Configure report interval (seconds)  */
    u32                 free_tracking_max_entry;  /* Configure number of freed memory information to keeptrack*/
    u64                 disable_module;  /* Disable debugging the selected module : BitMask- Each bit corresponds to one module  */
    u8                  enable_filter;  /* 0: Disable Filter, 1: Enable Blacklist 2: Enable Whitelist */
    char                filter_file[MEM_DBG_FILE_NAME_MAX_LEN];   // Filter Filename to filter selected functions */
#endif
    u32                 wpa_get_value_retry_count; /* retry count for wpa_get_value */
} apacHyfi20Config_t;


typedef struct apacHyfi20Data_t {
    u8                      alid[ETH_ALEN];
    s32                     nlSock;
    s32                     unPlcSock;
    s32                     pipeFd;
    u32                     mid;
    u32                     isCfg80211; /* Flag to enable CFG80211 */
    u8                      numRadios;

    apacHyfi20Config_t      config;
    apacHyfi20AP_t          ap[APAC_NUM_WIFI_FREQ];
    apacHyfi20IF_t          hyif[APAC_MAXNUM_HYIF];
    apacHyfi20IF_t          bridge;                   /* hy0 */
#if MAP_ENABLED
    apacHyfi20IF_t          br_guest_list[APAC_MAX_VLAN_SUPPORTED];
#endif

    apacHyfi20MLD_t         mld[APAC_MAX_MLO_GROUP_SUPPORTED];
    struct apac_wps_session*    sess_list;
    struct wpa_supp*            wpas;

    /* storage handle used to store Wi-Fi configuration params */
    void *wifiConfigHandle;

    /* the time elapsed since the first Wi-Fi configuration gets stored */
    u8 wifiConfigWaitSecs;
} apacHyfi20Data_t;

#if MAP_ENABLED
typedef struct apacMapData_t {
    apacHyfi20MapVersion_e vEnabled;
    u8 MapConfMaxBss;
    u8 mapEncrCnt;
    apacMapAP_t mapEncr[MAX_WLAN_CONFIGURATION];
    u8 eProfileCnt;
    apacMapEProfileMatcher_t eProfileMatcher[APAC_MAXNUM_NTWK_NODES];
    u8 eBstaMldCnt;
    apacMapEProfileMatcher_t eBstaProfileMatcher[APAC_MAXNUM_NTWK_NODES];
    u8 ssidCnt;
    apacMapEProfileSSID_t eProfileSSID[IEEE1905_QCA_VENDOR_MAX_INTERFACE];
    u8 mloCnt;
    apacMapEProfileMLO_t eProfileMLO[APAC_MAX_MLO_GROUP_SUPPORTED];
    u8 m1SentBand; // to differentiate between 5G L and 5G h
    u8 mapBssType[MAX_WLAN_CONFIGURATION];
    apacBool_e mapPfCompliant;
    apacBool_e mapR1R2MixNotSupported;
    u16 mapMaxServicePRules;
    u8 mapAgentCounterUnits;
    u8 numVlanSupported;
    u8 mapEnableBackhaulWPS;
    apacBool_e map2TrafficSepEnabled;
    char br_names[APAC_MAX_VLAN_SUPPORTED][IFNAMSIZ];
    char br_backhaul[IFNAMSIZ];
    apacBool_e r1AgentInNw;
    apacBool_e r2EnableMboOcePmf;
    apacBool_e map2TSSetFromHYD;
    apacBool_e isZeroBssEnabled;
    apacBool_e fastOnboardEnabled;
    apacBool_e map6GEnabled;
    apacBool_e mapStandardCloneEnabled;
    /// DPP enabled in HYD
    apacBool_e mapConfigServiceEnabled;
#if DOT1X_SUPPORT
    /// dot1x enabled
    apacBool_e dot1xSupport;
#endif
    apacBool_e mloEnabled;
    apacBool_e sloEnabled;
    apacBool_e send_packet_onbridge;
    apacBool_e external_controller;
    u8 numMLD;
    char base_mld_mac[APAC_MAC_STR_LENGTH];
    u8 sta_mld_index;
    apacBool_e onboarding_done;
    /// QOSM enabled for MAP
    apacBool_e isQosmEnabled;
    u8 CurrentRadioOpChannel[MAX_RADIO_CONFIGURATION];
    u8 CurrentRadioFreq[MAX_RADIO_CONFIGURATION];
    u8 CurrentRadioBandType[MAX_RADIO_CONFIGURATION];
    apacBool_e unicastRenewEnabled;
    apacBool_e skip6GSplitBandCheck;
    apacBool_e cloningOptimization;
    apacBool_e multiBSTAOnboarding;
    /// MAP R6 definitions
    u8 useSpecBasedDefinitions;
    u8 numRadios;
    u8 maxAPLinks;
    u8 earlyAPCapabilitySupported;
    u8 ctrlSupportSpecMloOnb;
    u8 smartMonitorEnabled;
    char serialNumber[DEVICE_INVENTORY_CONFIG_MAX_LEN];
    char executionEnvironment[DEVICE_INVENTORY_CONFIG_MAX_LEN];
    char chipsetVendor[DEVICE_INVENTORY_CONFIG_MAX_LEN];
    char softwareVersion[DEVICE_INVENTORY_CONFIG_MAX_LEN];
    u8 useLoInterface;
    struct ether_addr controllerAlid;

} apacMapData_t;
#endif

/* hold it all */
typedef struct apacInfo_t {
    wsplcd_data_t           hyfi10;
    apacHyfi20Data_t        hyfi20;
#if MAP_ENABLED
    apacMapData_t           mapData;
#endif
} apacInfo_t;

/*HYFI 2.0 and 1.0 shares some basic configuration and Operations,
  following macro provides an easy way to access each other */
#define HYFI10ToHYFI20(m) ((apacHyfi20Data_t *)&((apacInfo_t*)((char *)m - offsetof(apacInfo_t, hyfi10)))->hyfi20)
#define HYFI20ToHYFI10(m) ((wsplcd_data_t *)&((apacInfo_t*)((char *)m - offsetof(apacInfo_t, hyfi20)))->hyfi10)
#if MAP_ENABLED
#define HYFI20ToMAP(m) ((apacMapData_t *)&((apacInfo_t*)((char *)m - offsetof(apacInfo_t, hyfi20)))->mapData)
#define MAPToHYFI20(m) ((apacHyfi20Data_t *)&((apacInfo_t*)((char *)m - offsetof(apacInfo_t, mapData)))->hyfi20)
#endif

/* Public APIs */
/*
 * The function gets called if SimpleConnect (activated by wsplcd)
 * has successfully added a new node
 * (out) plc_mac: the PLC MAC address of the newly added node
 */
void pbcPlcSimpleConnectAddNode(u8 *plc_mac);

/*
 * The function gets called if hostapd has activated (by wsplcd) WPS on an 1905 AP
 * and this AP successfully added a new node
 * (out) wifi_mac: the added WIFI MAC address of the new station
 */
void pbcWifiWpsAddNode(u8 *mac_ap, u8 *mac_sta);

/*
 * This function masks signals so that signals are not delivered. Intended to be
 * used for critical sections of code which should not be interrupted. Once critical
 * code section is completed, remove the masking by calling wsplcdUnmaskSignals()
 */
void wsplcdMaskSignals();

/*
 * This function resets signal mask, should be used after wsplcdMaskSignals() for
 * critical sections of code
 */
void wsplcdUnmaskSignals();

/*
 * This function reads uci config and returns an positive integer value. Negative
 * value is returned incase of error
 */
s32 wsplcdUciGetInt(const char *config);

/* get the interface index for given ifname */
s32 apacHyfI20ieee1905GetInterface (apacHyfi20IF_t *pIF, char *ifname);
void apacHyfI20ieee1905DestroyIfaceSock(apacHyfi20Data_t *pData, s32 i);
s32 apacHyfI20ieee1905CreateIfaceSock(apacHyfi20Data_t *pData, s32 i, char *ifname);

#ifdef QCA_EXT_EM_CONTROLLER

#ifdef BUILD_RDKB
int emex_platform_get(char *module, char* path, char* value);
#endif

/** @brief Callback function to prepare  EM+ Extension TLV
 *
 * @input_param   type        type to distinguish MAP or 1905 TLV type
 * @input_param   emex_type   type to distinguish  EM+ Extension TLVs
 * @output_param  pbuf        filled tlv buffer according to provided  EM+ Extension TLV
 * @output_param  payload_len length of filled tlv
 */
typedef int ieee1905_emex_prepare_tlv(uint8_t type, uint16_t emex_type, uint8_t **pbuf, uint16_t *payload_len);

/** @brief Callback function to get config from  EM+ Extensions
 *
 * @input_param  module  module to be retrieved
 * @input_param  path    path of to be retrieved
 * @out_param    val    uci value to be placed
 */
typedef int ieee1905_emex_get_config(char *config, char *pbuf, char *val);

/* Callback function to understand EM+ vendor messages. */
typedef int ieee1905_emex_vendor_message(unsigned char **tlvs, unsigned int *len);

/** @brief Callback function to init  EM+ Extensions
 */
typedef int ieee1905_emex_init();

/** @brief Callback function to deinit  EM+ Extensions
 */
typedef void ieee1905_emex_deinit();

typedef struct ieee1905_call_bks {
    ieee1905_emex_prepare_tlv *emex_prepare_tlv;
    ieee1905_emex_get_config *emex_get_config;
    ieee1905_emex_vendor_message *emex_vendor_message;
    ieee1905_emex_init *emex_init;
    ieee1905_emex_deinit *emex_deinit;
}ieee1905_call_bks_t;

extern ieee1905_call_bks_t cbs;
#endif

#endif // _WSPLCD_H
