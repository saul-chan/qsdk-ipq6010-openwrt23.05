/*
 * Copyright (c) 2017-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * Copyright (c) 2010, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * Defintions for the Atheros Wireless LAN controller driver.
 */
#ifndef _DEV_OL_ATH_ATHVAR_H
#define _DEV_OL_ATH_ATHVAR_H

#include <osdep.h>
#include <a_types.h>
#include <a_osapi.h>
#include "ol_defines.h"
#include "ieee80211_channel.h"
#include "ieee80211_proto.h"
#include "ieee80211_rateset.h"
#include "ieee80211_regdmn.h"
#include "ieee80211_wds.h"
#include "ieee80211_acs.h"
#include "qdf_types.h"
#include "qdf_lock.h"
#include "qdf_lock.h"
#include "qdf_str.h"
#include "wmi_unified_api.h"
#include "htc_api.h"
#include "cdp_txrx_cmn_struct.h"
#include "cdp_txrx_stats_struct.h"
#include "cdp_txrx_extd_struct.h"
#include "cdp_txrx_ctrl_def.h"
#include "cdp_txrx_cmn.h"
#include "cdp_txrx_raw.h"
#include "cdp_txrx_me.h"
#include "cdp_txrx_mon.h"
#include "cdp_txrx_pflow.h"
#include "cdp_txrx_host_stats.h"
#include "cdp_txrx_wds.h"
#if WLAN_SUPPORT_MSCS
#include "cdp_txrx_mscs.h"
#endif
#if WLAN_SUPPORT_MESH_LATENCY
#include "cdp_txrx_mesh_latency.h"
#endif
#ifdef WLAN_SUPPORT_SCS
#include "cdp_txrx_scs.h"
#endif
#include <pktlog_ac_api.h>
#include "epping_test.h"
#include "wdi_event_api.h"
#include "ol_helper.h"
#include "ol_if_thermal.h"
#include "ol_if_txrx_handles.h"
#include "qca_ol_if.h"
#if PERF_FIND_WDS_NODE
#include "wds_addr_api.h"
#endif
#if FW_CODE_SIGN
#include <misc/fw_auth.h>
#endif  /* FW_CODE_SIGN */
#if OL_ATH_SUPPORT_LED
#include <linux/gpio.h>
#endif
#include <ieee80211_objmgr_priv.h>
#include <osif_private.h>
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include <nss_api_if.h>
#endif
#ifdef WLAN_NETSTANDBY_SUPPORT
#include <linux/netstandby.h>
#endif
#if !defined(BUILD_X86) && !defined(CONFIG_X86)
#include <soc/qcom/eawtp.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 24)
#include <linux/qcom-pcie.h>
#endif
#endif
#include "qdf_vfs.h"
#include "qdf_dev.h"

#include <wlan_vdev_mgr_tgt_if_tx_defs.h>

#if ATH_PERF_PWR_OFFLOAD
#include <ieee80211_sm.h>
#endif

#include <wlan_lmac_if_api.h>
#include <ol_ath_ucfg.h>
#include <ol_if_dcs.h>
#include <wlan_osif_priv.h>
#include <wlan_dfs_utils_api.h>
#include<wlan_mgmt_txrx_utils_api.h>
#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_public_structs.h>
#endif /* WLAN_FEATURE_11BE_MLO */

#ifdef SAWF_ADMISSION_CONTROL
#include "cdp_txrx_sawf.h"
#endif

/* WRAP SKB marks used by the hooks to optimize */
#define WRAP_ATH_MARK              0x8000

#define WRAP_FLOOD                 0x0001  /*don't change see NF_BR_FLOOD netfilter_bridge.h*/
#define WRAP_DROP                  0x0002  /*mark used to drop short circuited pkt*/
#define WRAP_REFLECT               0x0004  /*mark used to identify reflected multicast*/
#define WRAP_ROUTE                 0x0008  /*mark used allow local deliver to the interface*/

#define WRAP_MARK_FLOOD            (WRAP_ATH_MARK | WRAP_FLOOD)
#define WRAP_MARK_DROP             (WRAP_ATH_MARK | WRAP_DROP)
#define WRAP_MARK_REFLECT          (WRAP_ATH_MARK | WRAP_REFLECT)
#define WRAP_MARK_ROUTE            (WRAP_ATH_MARK | WRAP_ROUTE)

#define WRAP_MARK_IS_FLOOD(_mark)  ((_mark & WRAP_ATH_MARK)?((_mark & WRAP_FLOOD)?1:0):0)
#define WRAP_MARK_IS_DROP(_mark)   ((_mark & WRAP_ATH_MARK)?((_mark & WRAP_DROP)?1:0):0)
#define WRAP_MARK_IS_REFLECT(_mark) ((_mark & WRAP_ATH_MARK)?((_mark & WRAP_REFLECT)?1:0):0)
#define WRAP_MARK_IS_ROUTE(_mark)  ((_mark & WRAP_ATH_MARK)?((_mark & WRAP_ROUTE)?1:0):0)

#define EXT_TID_NONPAUSE    19

#define RTT_LOC_CIVIC_INFO_LEN      16
#define RTT_LOC_CIVIC_REPORT_LEN    64

/* Requestor ID for multiple vdev restart */
#define MULTIPLE_VDEV_RESTART_REQ_ID 0x1234

/* DFS defines */
#define DFS_RESET_TIME_S 7
#define DFS_WAIT (60 + DFS_RESET_TIME_S) /* 60 seconds */
#define DFS_WAIT_MS ((DFS_WAIT) * 1000) /*in MS*/

#define DFS_WEATHER_CHANNEL_WAIT_MIN 10 /*10 minutes*/
#define DFS_WEATHER_CHANNEL_WAIT_S (DFS_WEATHER_CHANNEL_WAIT_MIN * 60)
#define DFS_WEATHER_CHANNEL_WAIT_MS ((DFS_WEATHER_CHANNEL_WAIT_S) * 1000)       /*in MS*/


#define AGGR_BURST_AC_OFFSET 24
#define AGGR_BURST_AC_MASK 0x0f
#define AGGR_BURST_DURATION_MASK 0x00ffffff
#define AGGR_PPDU_DURATION  2000
#define AGGR_BURST_DURATION 8000

#define NUMBER_RSID_LENGTH_BITS 16

/* Lithium ratecode macros to extract mcs, nss, preamble from the ratecode
 * table in host
 */
#define RATECODE_V1_RC_SIZE     16
#define RATECODE_V1_RC_MASK     0xffff
/* This macro is used to extract preamble from the rc in the tables defined in
 * host. In these tables we have kept 4 bits for rate in the rc since 4 bits can
 * accomodate all the valid rates till HE(MCS 11). If the tables get updated in
 * the future to accomodate 5 bit rate in rc then we will have to use
 * PREAMBLE_OFFSET_IN_V1_RC in place of this macro.
 */
#define RATECODE_V1_PREAMBLE_OFFSET (4+3)
#define RATECODE_V1_PREAMBLE_MASK   0x7
/* This macro is used to extract nss from the rc in the tables defined in
 * host. In these tables we have kept 4 bits for rate in the rc since 4 bits can
 * accomodate all the valid rates till HE(MCS 11). If the tables get updated in
 * the future to accomodate 5 bit rate in rc then we will have to use
 * NSS_OFFSET_IN_V1_RC in place of this macro.
 */
#define RATECODE_V1_NSS_OFFSET  0x4
#define RATECODE_V1_NSS_MASK    0x7
/* This macro is used to extract rate from the rc in the tables defined in
 * host. In these tables we have kept 4 bits for rate in the rc since 4 bits can
 * accomodate all the valid rates till HE(MCS 11). If the tables get updated in
 * the future to accomodate 5 bit rate in rc then we will have to redefine
 * the macro to 0x1f to parse 5 bits.
 */
#define RATECODE_V1_RIX_MASK    0xf
/* Lithium ratecode macros to assemble the rate, mcs and preamble in the V1
 * coding format to send to target
 */
#define VERSION_OFFSET_IN_V1_RC  28
#define PREAMBLE_OFFSET_IN_V1_RC 8
#define NSS_OFFSET_IN_V1_RC      5
/* Following macro assembles '_rate' in V1 format
 * where '_rate' is of length 16 bits in the format
 * _rate = (((_pream) << 8) | ((_nss) << 5) | (rate))
 */
#define ASSEMBLE_RATECODE_V1(_rate, _nss, _pream) \
    ((((1) << VERSION_OFFSET_IN_V1_RC) |          \
     ((_pream) << PREAMBLE_OFFSET_IN_V1_RC)) |    \
     ((_nss) << NSS_OFFSET_IN_V1_RC) | (_rate))
#define V1_RATECODE_FROM_RATE(_rate) \
       (((1) << VERSION_OFFSET_IN_V1_RC) | _rate)

/* Legacy ratecode macros to extract rate, mcs and preamble from the ratecode
 * table in host
 */
#define RATECODE_LEGACY_RC_SIZE IEEE80211_RATE_SIZE
#define RATECODE_LEGACY_RC_MASK         0xff
#define RATECODE_LEGACY_NSS_OFFSET      0x4
#define RATECODE_LEGACY_NSS_MASK        0x7
#define RATECODE_LEGACY_RIX_MASK        0xf
#define RATECODE_LEGACY_PREAMBLE_OFFSET 7
#define RATECODE_LEGACY_PREAMBLE_MASK   0x3
/* Legacy ratecode macros to assemble the rate, mcs and preamble in the legacy
 * ratecode format to send to target
 */
#define PREAMBLE_OFFSET_IN_LEGACY_RC    6
#define NSS_MASK_IN_LEGACY_RC           0x3
/* Following macro assembles '_rate' in legacy format
 * where '_rate' is of length 8 bits in the format
 * _rate = (((_pream) << 6) | ((_nss) << 4) | (rate))
 */
#define ASSEMBLE_RATECODE_LEGACY(_rate, _nss, _pream) \
    (((_pream) << PREAMBLE_OFFSET_IN_LEGACY_RC) |     \
    ((_nss) << RATECODE_LEGACY_NSS_OFFSET) | (_rate))

#define DP_TRACE_CONFIG_DEFAULT_LIVE_MODE 0
#define DP_TRACE_CONFIG_DEFAULT_THRESH 4
#define DP_TRACE_CONFIG_DEFAULT_THRESH_TIME_LIMIT 10
#define DP_TRACE_CONFIG_DEFAULT_VERBOSTY QDF_DP_TRACE_VERBOSITY_LOW
#define DP_TRACE_CONFIG_DEFAULT_BITMAP \
        (QDF_NBUF_PKT_TRAC_TYPE_EAPOL |\
        QDF_NBUF_PKT_TRAC_TYPE_DHCP |\
        QDF_NBUF_PKT_TRAC_TYPE_MGMT_ACTION |\
        QDF_NBUF_PKT_TRAC_TYPE_ARP |\
        QDF_NBUF_PKT_TRAC_TYPE_ICMP)

#define RATE_DROPDOWN_LIMIT 7 /* Maximum Value for Rate Drop Down Logic */

#define SNIFFER_DISABLE 0
#define SNIFFER_TX_CAPTURE_MODE 1
#define SNIFFER_M_COPY_MODE 2
#define SNIFFER_TX_MONITOR_MODE 3
#define SNIFFER_EXT_M_COPY_MODE 4

#define MODE_M_COPY 1
#define MODE_EXT_M_COPY 2

#define TX_ENH_PKT_CAPTURE_DISABLE 0
#define TX_ENH_PKT_CAPTURE_ENABLE_ALL_PEERS 1
#define TX_ENH_PKT_CAPTURE_ENABLE_PER_PEER 2

#define RX_ENH_CAPTURE_DISABLED 0
#define RX_ENH_CAPTURE_MPDU 1
#define RX_ENH_CAPTURE_MPDU_MSDU 2
#define RX_ENH_CAPTURE_MODE_MASK 0x0F
#define RX_ENH_CAPTURE_PEER_MASK 0xFFFFFFF0
#define RX_ENH_CAPTURE_PEER_LSB  4

#define BEACON_TX_MODE_BURST 1
#define BEACON_TX_MODE_STAGGERED 0

#define PPDU_DESC_ENHANCED_STATS 1
#define PPDU_DESC_DEBUG_SNIFFER 2
#define PPDU_DESC_SMART_ANTENNA 3
#define PPDU_DESC_PEER_STATS 4
#define PPDU_DESC_CFR_RCC 5
#define PPDU_DESC_ATF_STATS 6

#define MAX_TIM_BITMAP_LENGTH 68 /* Max allowed TIM bitmap for 512 client is 64 + 4  (Guard length) */

/* Max wait time on fw response for hw-mode
 * switch cmd
 */
#define MAX_HW_MODE_FW_RESP_TIME 3000
/* Max wait time for hw-mode switch complete
 * in host after fw response is received
 */
#define MAX_HW_MODE_SWITCH_ACCOMPLISH_TIME_IN_HOST 1500

/* Max wait time on fw response for rf path
 * switch cmd
 */
#define MAX_RF_PATH_FW_RESP_TIME 3000
/* Max wait time for rf-path switch complete
 * in host after fw response is received
 */
#define RF_MODE_INVALID 0xff

#define FW_HANG_TIME 300  // Time (sec) after which FW should hang if wmi not reaped
#define WAIT_TIME    60   // Time (sec) after which to send fw hang comand

#define ATH_DEFAULT_NOISEFLOOR (-96) /* Default Noise floor in dBm */
#define ATH_DEFAULT_NORMAL_SNR 35    /* Default SNR which can be considered as normal signal quality */

#define WLAN_LATENCY_OPTIMIZED_DL_TID_SCHEDULING 1
#define WLAN_LATENCY_OPTIMIZED_UL_TID_SCHEDULING 2

typedef void * hif_softc_t;

#if ATH_SUPPORT_DSCP_OVERRIDE
extern A_UINT32 dscp_tid_map[WMI_HOST_DSCP_MAP_MAX];
#endif

/* Invalid Tbtt offset value from target
 */
#define OL_TBTT_OFFSET_INVALID 0xffffffff

/*
 * Maximum acceptable MTU
 * MAXFRAMEBODY - WEP - QOS - RSN/WPA:
 * 2312 - 8 - 2 - 12 = 2290
 */
#define ATH_MAX_MTU     2290
#define ATH_MIN_MTU     32

#define OL_PCP_MIN 0
#define OL_PCP_MAX 7
#define OL_TID_MIN 0
#define OL_TID_MAX 7


#ifdef QCA_OL_DMS_WAR
/**
 * struct dms_meta_hdr - Meta data structure used for DMS
 * @dms_id: DMS ID corresponding to the peer, should be non-zero
 * @unicast_addr: Address of the DMS subscribed peer
 */
struct dms_meta_hdr {
    uint8_t dms_id;
    uint8_t unicast_addr[QDF_MAC_ADDR_SIZE];
};
#endif

struct ath_version {
    u_int32_t    host_ver;
    u_int32_t    target_ver;
    u_int32_t    wlan_ver;
    u_int32_t    wlan_ver_1;
    u_int32_t    abi_ver;
};

typedef enum _ATH_BIN_FILE {
    ATH_OTP_FILE,
    ATH_FIRMWARE_FILE,
    ATH_PATCH_FILE,
    ATH_BOARD_DATA_FILE,
    ATH_FLASH_FILE,
    ATH_TARGET_EEPROM_FILE,
    ATH_UTF_FIRMWARE_FILE,
} ATH_BIN_FILE;

typedef enum _OTP_PARAM {
    PARAM_GET_EEPROM_ALL            = 0,
    PARAM_SKIP_MAC_ADDR             = 1,
    PARAM_SKIP_REG_DOMAIN           = 2,
    PARAM_SKIP_OTPSTREAM_ID_CAL_5G  = 4,
    PARAM_SKIP_OTPSTREAM_ID_CAL_2G  = 8,
    PARAM_GET_CHIPVER_BID           = 0x10,
    PARAM_USE_GOLDENTEMPLATE        = 0x20,
    PARAM_USE_OTP                   = 0x40,
    PARAM_SKIP_EEPROM               = 0x80,
    PARAM_EEPROM_SECTION_MAC        = 0x100,
    PARAM_EEPROM_SECTION_REGDMN     = 0x200,
    PARAM_EEPROM_SECTION_CAL        = 0x400,
    PARAM_OTP_SECTION_MAC           = 0x800,
    PARAM_OTP_SECTION_REGDMN        = 0x1000,
    PARAM_OTP_SECTION_CAL           = 0x2000,
    PARAM_SKIP_OTP                  = 0x4000,
    PARAM_GET_BID_FROM_FLASH        = 0x8000,
    PARAM_FLASH_SECTION_ALL         = 0x10000,
    PARAM_FLASH_ALL                 = 0x20000,
    PARAM_DUAL_BAND_2G              = 0x40000,
    PARAM_DUAL_BAND_5G              = 0x80000
}OTP_PARAM;

typedef enum _ol_target_status  {
     OL_TRGET_STATUS_CONNECTED = 0,    /* target connected */
     OL_TRGET_STATUS_RESET,        /* target got reset */
     OL_TRGET_STATUS_EJECT,        /* target got ejected */
} ol_target_status;

enum ol_ath_tx_ecodes  {
    TX_IN_PKT_INCR=0,
    TX_OUT_HDR_COMPL,
    TX_OUT_PKT_COMPL,
    PKT_ENCAP_FAIL,
    TX_PKT_BAD,
    RX_RCV_MSG_RX_IND,
    RX_RCV_MSG_PEER_MAP,
    RX_RCV_MSG_TYPE_TEST
} ;

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
/*
 * struct recovery_soc_info - Across MLO SoCs Recovery inforamtion
 * @tot_grp_socs:  Total MLO SoCs in the MLO Group
 * @grp_id: MLO Group id - which is in recovery
 * @def_soc_list: List of All MLO SoCs participating in the MLO Group
 * @curr_soc_list: List of MLO SoCs that are yet to be MLO teardown/MLO setup
 */
struct recovery_soc_info {
	uint8_t tot_grp_socs;
	uint8_t  grp_id;
	struct wlan_objmgr_psoc *def_soc_list[MAX_MLO_CHIPS];
	struct wlan_objmgr_psoc *curr_soc_list[MAX_MLO_CHIPS];
};
#endif

#define PANIC_TYPE_ASSERT 1

enum ol_recovery_option {
    RECOVERY_DISABLE = 0,
    /* Below commands unlinks UserPD assert from RootPD */
    RECOVERY_ENABLE_AUTO,       /* Automatically recover through default mode after FW assert */
    RECOVERY_ENABLE_WAIT,       /* Default FW recovery, only do FW RAM dump and wait for user */
    RECOVERY_ENABLE_SSR_MODE1,  /* Automatically recovery through mode1 SSR */
    /* Enable default FW recovery with UserPD assert linked with RootPD */
    RECOVERY_ENABLE_SSR_ONLY,
} ;

enum mpd_crash {
    ROOT_PD_ASSERT = 0,
    USER_PD_ONLY_ASSERT,
} ;

#ifdef WLAN_DBS_POWER_REDUCTION
/* Enum to indicate Power Reduction trigger points */
enum ps_metric_change {
    PS_WLAN_DBS_CHANGE = 0,
    PS_ETH_PORT_CHANGE = 1,
    PS_METRIC_CHANGE_MAX,
};

/* Enum to indicate DBS states */
enum dbs_in_out_state {
    DBS_OUT,
    DBS_IN,
};

/*
 * Structure to hold Power Reduction State along with active pdev
 * and active ethernet port counts
 */
struct ps_context {
    uint8_t num_active_pdev;
    uint8_t num_active_port;
    enum dbs_in_out_state dbs_state;
    eawtp_get_num_active_ports_cb_t get_actv_eth_ports_cb;
};
#endif /* WLAN_DBS_POWER_REDUCTION */

#define STATS_MAX_RX_CES     12
#define STATS_MAX_RX_CES_PEREGRINE 8

/*
 * structure to hold the packet error count for CE and hif layer
*/
struct ol_ath_stats {
    int hif_pipe_no_resrc_count;
    int ce_ring_delta_fail_count;
    int sw_index[STATS_MAX_RX_CES];
    int write_index[STATS_MAX_RX_CES];
};

struct ol_ath_target_cap {
    target_resource_config     wlan_resource_config; /* default resource config,the os shim can overwrite it */
    /* any other future capabilities of the target go here */

};
/* callback to be called by durin target initialization sequence
 * to pass the target
 * capabilities and target default resource config to os shim.
 * the os shim can change the default resource config (or) the
 * service bit map to enable/disable the services. The change will
 * pushed down to target.
 */
typedef void   (* ol_ath_update_fw_config_cb)\
    (ol_ath_soc_softc_t *soc, struct ol_ath_target_cap *tgt_cap);

/*
 * memory chunck allocated by Host to be managed by FW
 * used only for low latency interfaces like pcie
 */
struct ol_ath_mem_chunk {
    u_int32_t *vaddr;
    u_int32_t paddr;
    qdf_dma_mem_context(memctx);
    u_int32_t len;
    u_int32_t req_id;
};

#if OL_ATH_SUPPORT_LED
#define PEREGRINE_LED_GPIO    1
#define BEELINER_LED_GPIO    17
#define CASCADE_LED_GPIO     17
#define BESRA_LED_GPIO       17
#define IPQ8074_2G_LED_GPIO  42
#define IPQ8074_5G_LED_GPIO  43

#define LED_POLL_TIMER       500

typedef enum _OL_BLINK_STATE {
    OL_BLINK_DONE = 0,
    OL_BLINK_OFF_START = 1,
    OL_BLINK_ON_START = 2,
    OL_BLINK_STOP = 3,
    OL_NUMER_BLINK_STATE,
} OL_BLINK_STATE;

typedef enum {
    OL_LED_OFF = 0,
    OL_LED_ON
} OL_LED_STATUS;

typedef enum _OL_LED_EVENT {
    OL_ATH_LED_TX = 0,
    OL_ATH_LED_RX = 1,
    OL_ATH_LED_POLL = 2,
    OL_NUMER_LED_EVENT,
} OL_LED_EVENT;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
struct alloc_task_pvt_data {
    wmi_buf_t evt_buf;
    void *scn_handle;
};
#endif

#ifndef AR900B_REV_1
#define AR900B_REV_1 0x0
#endif

#ifndef AR900B_REV_2
#define AR900B_REV_2 0x1
#endif

#define DEFAULT_ANI_ENABLE_STATUS false

/*
 *  Error types that needs to be muted as per rate limit
 *  Currently added for pn errors and sequnce errors only.
 *  In future this structure can be exapanded for other errors.
 */
#define DEFAULT_PRINT_RATE_LIMIT_VALUE 100

/*
 * Default TX ACK time out value (micro second)
 */
#define DEFAULT_TX_ACK_TIMEOUT 0x70

/*
 * Min TX ACK time out value (micro second)
 */
#define MIN_TX_ACK_TIMEOUT 0x40

/*
 * Max TX ACK time out value (micro second)
 */
#define MAX_TX_ACK_TIMEOUT 0xFF

/*
 * Max Proberesp retry limit
 */
#define MAX_PRESP_RETRY_LIMIT 0x7F

/*
 * Max CTS TIMEOUT (micro second)
 */
#define MAX_CTS_TIMEOUT 0xFF

/*
 * Max SLOT TIME (micro second)
 */
#define MAX_SLOT_TIME 90

/*
 * Retry limit mask
 */
#define RETRY_LIMIT_MASK 0xF

/*
 * Retry limit extended mask
 */
#define RETRY_LIMIT_EXT_MASK 0x7

/*
 * Retry limit extended shift
 */
#define RETRY_LIMIT_EXT_SHIFT 4

#define MAX_AUTH_REQUEST   32
#define DEFAULT_AUTH_CLEAR_TIMER  1000

/*
 * Default timer for Host to send
 * cmd to FW to sync timer between SoCs
 */
#define DEFAULT_TBTT_SYNC_TIMER 10000

enum tbtt_sync_timer {
	TBTT_SYNC_TIMER_STOP,
	TBTT_SYNC_TIMER_START,
};

struct mute_error_types {
    u_int32_t  pn_errors;
    u_int32_t  seq_num_errors;
};

struct debug_config {
    int print_rate_limit; /* rate limit value */
    struct mute_error_types err_types;
};

#define WMI_MGMT_DESC_POOL_MAX 50
struct wmi_mgmt_desc_t {
	struct ieee80211_cb cb;
	qdf_nbuf_t   nbuf;
	uint32_t     desc_id;
};
union wmi_mgmt_desc_elem_t {
	union wmi_mgmt_desc_elem_t *next;
	struct wmi_mgmt_desc_t wmi_mgmt_desc;
};

/*
 * In parallel mode gpio_pin/func[0-4]- is chain[0-4] configuration
 * In serial mode gpio_pin/func[0]- Configuration for data signal
 *                gpio_pin/func[1]- Configuration for strobe signal
 */
#define SMART_ANT_MAX_SA_CHAINS 4
struct ol_smart_ant_gpio_conf {
    u_int32_t gpio_pin[WMI_HAL_MAX_SANTENNA]; /* GPIO pin configuration for each chain */
    u_int32_t gpio_func[WMI_HAL_MAX_SANTENNA];/* GPIO function configuration for each chain */
};

/* Max number of radios supported in SOC chip */
#define MAX_RADIOS  3

struct soc_spectral_stats {
    uint64_t  phydata_rx_errors;
};

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#define OSIF_NSS_WIFILI_MAX_NUMBER_OF_PAGE 96
#define OSIF_NSS_WIFI_MAX_PEER_ID 4096

struct nss_wifi_peer_mem {
    bool in_use[OSIF_NSS_WIFI_MAX_PEER_ID];    /* array to track peer id allocated. */
    bool nss_peer_ack[OSIF_NSS_WIFI_MAX_PEER_ID];    /* Array to store peer create ACK from nss */
    uint32_t paddr[OSIF_NSS_WIFI_MAX_PEER_ID];    /* Array to store the peer_phy mem address allocated. */
    uintptr_t vaddr[OSIF_NSS_WIFI_MAX_PEER_ID];    /* Array to store the peer_virtual mem address allocated. */

#ifdef QCA_PEER_EXT_STATS
    uint32_t pext_mem_sz;                              /* Peer extended size */
    uint32_t pext_paddr[OSIF_NSS_WIFI_MAX_PEER_ID];    /* Array to store the peer_phy mem address allocated. */
    uintptr_t pext_vaddr[OSIF_NSS_WIFI_MAX_PEER_ID];    /* Array to store the peer_virtual mem address allocated. */
#endif
    int peer_id_to_alloc_idx_map[OSIF_NSS_WIFI_MAX_PEER_ID]; /* peer_id to alloc index map */
    uint16_t available_idx;
    spinlock_t queue_lock;
};

#define OSIF_NSS_WIFILI_MAX_MSG_POOL 4
/*
 * wifili message memory pool
 *  Global memory pool for wifili message
 */
struct nss_wifili_msg_mem_pool {
    struct nss_wifili_msg *wlmsg_pool[OSIF_NSS_WIFILI_MAX_MSG_POOL];  /* pool of allocated memory */
    uint8_t mem_in_use[OSIF_NSS_WIFILI_MAX_MSG_POOL];    /* flag to indicate pool in use */
    bool pool_initialized;      /* per soc flag to indicate if pool is initialized */
    spinlock_t queue_lock;    /* per soc lock for access to memory pool */
};

struct ol_ath_softc_nss_soc {
    int nss_wifiol_id;			/* device id as the device gets probed*/
    int nss_wifiol_ce_enabled;		/* where ce is completed , wifi3.0 may not require*/
    struct {
        struct completion complete;     /* completion structure */
        int response;               /* Response from FW */
    } osif_nss_ol_wifi_cfg_complete;
    uint32_t nss_sidx;		/* assigned NSS id number */
    nss_if_num_t nss_sifnum;	/* device interface number */
    void *nss_sctx;		/* device nss context */
    struct nss_wifi_soc_ops *ops;
    uint32_t nss_scfg;
    uint32_t nss_nxthop;        /* nss next hop config */
    uint32_t desc_pmemaddr[OSIF_NSS_WIFILI_MAX_NUMBER_OF_PAGE];
    uintptr_t desc_vmemaddr[OSIF_NSS_WIFILI_MAX_NUMBER_OF_PAGE];
    uint32_t desc_memsize[OSIF_NSS_WIFILI_MAX_NUMBER_OF_PAGE];
    struct nss_wifi_peer_mem nwpmem;
    struct nss_wifili_msg_mem_pool msgmempool;
    struct device *dmadev;
    struct nss_wifili_ast_entries_db *astentry_db;
};

struct ol_ath_softc_nss_radio {
    uint32_t nss_idx;			/* assigned NSS id number for radio*/
    nss_if_num_t nss_rifnum;		/* NSS interface number for the radio */
    void *nss_rctx;		        /* nss context for the radio */
    uint8_t nss_nxthop;                 /* nss next hop config */
    uint32_t nss_scheme_id;		/* assigned NSS scheme index for radio*/
};
#endif

/**
 * struct chan_phymode_params - Channel and phymode parameters,
 *                                   used to store current channel
 * information during mode switch.
 * @freq: Frequency parameter.
 * @phymode: Phymode of the channel.
 */
struct chan_phymode_params {
    union {
	uint16_t freq;
	struct chan_params chan_p;
    };
    enum ieee80211_phymode phymode;
};

typedef struct {
    uint32_t target_band;       /* targeted band for hw_mode_switch */
    bool is_boot_in_progress;   /* is boot in-progress? */
    bool is_switch_in_progress; /* is mode switch in-progress? */
    uint8_t target_mode;        /* targeted mode during switch */
    uint8_t current_mode;       /* current mode */
    qdf_event_t event;          /* event required to wait on FW response */
    bool is_fw_resp_success;    /* FW response status - 0=fail, 1=success */
    struct chan_phymode_params curchan_params; /* primary radio's current channel params */
    struct chan_phymode_params prevchan_params; /* primary radio's previous channel params */
    bool is_bw_reduced_during_dms; /* Is channel bandwidth reduced for mode switch? */
    uint32_t dynamic_hw_mode;                   /* dynamic hw mode */
    struct wlan_objmgr_pdev *primary_pdev;      /* primary interface's pdev */
    uint32_t pdev_map[WMI_HOST_MAX_PDEV];       /* current pdev_id mapping table */
    uint32_t next_pdev_map[WMI_HOST_MAX_PDEV];  /* next pdev_id mapping table */
    uint8_t recover_mode;                       /* recover mode for SSR */
    wmi_unified_t prev_wmi_hdl;                 /* wmi handle of pdev before mode switch */

    /* Stats */
    #define MOVING_AVG_LENGTH  16
    uint32_t cnt_attempt;       /* count of total attempts */
    uint32_t cnt_success;       /* count of total successes */
    uint32_t cnt_failure;       /* count of total failures */
    int64_t ts_start;           /* timestamp for measuring switch time */
    int64_t ts_end;             /* timestamp for measuring switch time */
    uint32_t time_last;         /* last switch time */
    uint32_t time_avg;          /* moving average switch time */
} ol_ath_hw_mode_ctx_t;

typedef struct {
    uint32_t target_band;        /* targeted band for hw_mode_switch */
    bool is_switch_in_progress;  /* is mode switch in-progress? */
    uint8_t target_index;        /* targeted mode during switch */
    uint8_t current_index;       /* current mode */
    qdf_event_t rf_switch_done;  /* event required to wait on FW response */
    bool is_fw_resp_success;     /* FW response status - 0=fail, 1=success */

/* Stats */
#define MOVING_AVG_LENGTH  16
    int64_t ts_start;            /* timestamp for measuring switch time */
    int64_t ts_end;              /* timestamp for measuring switch time */
} ol_ath_rf_path_ctx_t;

typedef enum mgmt_frm_size {
    mgmt_frm_size_1_5k = 1500,
    mgmt_frm_size_2_0k = 2000,
    mgmt_frm_size_2_3k = 2300,
    mgmt_frm_size_3_5k = 3500,
    mgmt_frm_size_max  = IEEE80211_MAX_MGMT_SIZE_LIMIT,
} mgmt_frm_size_t;

typedef struct {
   mgmt_frm_size_t max_mgmt_frm_sz;
   uint8_t         max_pp;
   uint32_t        feature_config;      /* disable/enable config for mbssid/ema in lower band */
   uint8_t         mbssie_headroom;
} ol_ath_ema_ctx_t;

typedef struct {
   uint32_t vendor_ie_config_low;
   uint32_t vendor_ie_config_high;
   uint32_t vendor_ie_size_limit;/* limit per-beacon */
   uint32_t optional_ie_size;    /* configured per-vap optional-ie reservation */
   uint32_t optional_ie_size_limit; /* limit per-beacon */
   uint32_t max_non_tx_sz;       /* available per-beacon */
   uint32_t max_non_tx_sz_limit; /* limit per-beacon */
   uint8_t  max_non_tx_profile_sz_with_rsn; /* mandatory elements + RSN */
   uint16_t beacon_common_part_size;
   uint16_t rnr_field_size_limit;
   uint8_t  rnr_field_size_limit_override;
   uint8_t  num_max_vaps;
   uint8_t  max_ngroups;         /* Soc level sanitized max number of ap groups */
   uint8_t  grp_sz;
   bool     disable_6ghz_mbssid; /* disable/enable flag for mbssid support on 6G radio */
   bool     support_wps_6ghz;    /* support wps in EMA mode in 6Ghz */
   bool     ext_enabled;         /* feature enable flag for optional IE support in non-tx vap */
   bool     mbss_split_profile_enabled; /* enable/disable split profile for nonTx profile */
   /* Advertize 6ghz RNR info in 6Ghz band in non default mode.
    * Default behaviour is for 6g AP to advertize RNR in 6Ghz
    * only AP scenario. To override this below flag is used. */
   bool     rnr_6ghz_adv_override;
   bool     allow_mbss_cmn_all_vap;
} ol_ath_ema_pdev_ctx_t;

#ifndef FW_DUMP_FILE_NAME_SIZE
#define FW_DUMP_FILE_NAME_SIZE 32
#endif

#define MAX_CONFIG_COMMAND (100)    /* Maximum entries in circular buffer */
#define CONFIG_COMMAND_INTF_SIZE (32) /* Maximum size of interface name string */

enum CONFIG_TYPE {
    CONFIG_TYPE_CMD,    /* Config type: command */
    CONFIG_TYPE_RESP    /* Config type: response */
};

typedef struct {
    enum CONFIG_TYPE type; /* Entry type: 0 - command, 1 - response */
    char interface[CONFIG_COMMAND_INTF_SIZE]; /* Interface name (could be VDEV or PDEV) */
    int param;          /* Parameter type */
    int val;            /* Value to be configured */
    uint64_t time;      /* Timestamp */
} config_entry_t;

typedef struct config_cmd_log_info {
    config_entry_t* entry; /* Entry data structure populated for each call*/
    int entry_index;    /* Location of entry in circular buffer*/
    int entry_count;    /* Number of entries in the buffer */
    int init_flag;      /* Flag to indicate the context is initialized */
    int feature_init;   /* Flag to indicate if config logging feature is initialized */
    qdf_dentry_t config_log_debugfs_dir; /* Handle for debugfs directory */
    struct qdf_debugfs_fops ops; /* OPs structure that carries callback funcs */
    qdf_semaphore_t entry_lock;   /* Lock for synchronization */
} config_cmd_log_cxt_t;

enum wlan_standby_modes {
     WLAN_STANDBY_MODE_DISABLED = 0,
     WLAN_STANDBY_MODE_ACTIVE_ENTRY = 1,
     WLAN_STANDBY_MODE_ENTRY_COMPLETION_NOTIFY = 2,
     WLAN_STANDBY_MODE_INITIATE_EXIT = 3,
     WLAN_STANDBY_MODE_EXIT_COMPLETION_NOTIFY = 4,
};

#define FW_DUMP_FILE_NAME_SIZE 32
#define SOC_RESET_IN_PROGRESS_BIT 0
typedef struct ol_ath_soc_softc {
    void		    (*pci_reconnect)(struct ol_ath_soc_softc *);
    qdf_work_t 	    pci_reconnect_work;

#if defined(EPPING_TEST) && !defined(HIF_USB)
    /* for mboxping */
    HTC_ENDPOINT_ID         EppingEndpoint[4];
    qdf_spinlock_t       data_lock;
    struct sk_buff_head     epping_nodrop_queue;
    qdf_timer_t             epping_timer;
    bool                    epping_timer_running;
 #endif
    osdev_t                 sc_osdev;

    qdf_semaphore_t         stats_sem;

    /*
     * handle for code that uses adf version of OS
     * abstraction primitives
     */
    qdf_device_t   qdf_dev;

    /**
     * call back set by the os shim
     */
    ol_ath_update_fw_config_cb cfg_cb;

    u_int32_t board_id;
    u_int32_t chip_id;
    u_int16_t device_id;
    uint16_t                soc_attached;
    void      *platform_devid;
    struct targetdef_s *targetdef;

    struct ath_version      version;
    /* Is this chip a derivative of beeliner - Used for common checks that apply to derivates of ar900b */
    ol_target_status  target_status; /* target status */
    bool             is_sim;   /* is this a simulator */
    u_int32_t        sc_dump_opts;       /* fw dump collection options*/

    /* Packet statistics */
    struct ol_ath_stats     pkt_stats;
    bool                dbg_log_init;
    bool                    enableuartprint;    /* enable uart/serial prints from target */
    u_int32_t               vow_config;
    bool                    low_mem_system;

    u_int32_t               max_desc;
    u_int32_t               max_active_peers;	/* max active peers derived from max_descs */
    u_int32_t               peer_del_wait_time; /* duration to wait for peer del completion */
    u_int32_t               max_clients;
    u_int32_t               max_vaps;
    u_int32_t               max_group_keys;
    wdi_event_subscribe     scn_rx_peer_invalid_subscriber;
    wdi_event_subscribe     scn_rx_lite_monitor_mpdu_subscriber;

    u_int32_t               sa_validate_sw; /* validate Smart Antenna Software */
    u_int32_t               enable_smart_antenna; /* enable smart antenna */
    bool                    cce_disable; /* disable hw CCE block */

    struct debug_config      dbg;   /* debug support */

   struct swap_seg_info     *target_otp_codeswap_seginfo ;     /* otp codeswap seginfo */
   struct swap_seg_info     *target_otp_dataswap_seginfo ;     /* otp dataswap seginfo */
   struct swap_seg_info     *target_bin_codeswap_seginfo ;     /* target bin codeswap seginfo */
   struct swap_seg_info     *target_bin_dataswap_seginfo ;     /* target bin dataswap seginfo */
   struct swap_seg_info     *target_bin_utf_codeswap_seginfo ; /* target utf bin codeswap seginfo */
   struct swap_seg_info     *target_bin_utf_dataswap_seginfo ; /* target utf bin dataswap seginfo */
   u_int64_t                *target_otp_codeswap_cpuaddr;      /* otp codeswap cpu addr */
   u_int64_t                *target_otp_dataswap_cpuaddr;      /* otp dataswap cpu addr */
   u_int64_t                *target_bin_codeswap_cpuaddr;      /* target bin codeswap cpu addr */
   u_int64_t                *target_bin_dataswap_cpuaddr;      /* target bin dataswap cpu addr */
   u_int64_t                *target_bin_utf_codeswap_cpuaddr;  /* target utf bin codeswap cpu addr */
   u_int64_t                *target_bin_utf_dataswap_cpuaddr;  /* target utf bin dataswap cpu addr */

    bool                down_complete;
    unsigned long       reset_in_progress;
    void *cal_mem; /* virtual address for the calibration data on the flash */

    /* BMI info */
    struct bmi_info       *bmi_handle;

    void            *diag_ol_priv; /* OS-dependent private info for DIAG access */

    qdf_mempool_t mempool_ol_ath_node; /* Memory pool for nodes */
    qdf_mempool_t mempool_ol_ath_vap;  /* Memory pool for vaps */
    qdf_mempool_t mempool_ol_ath_peer; /* Memory pool for peer entry */
    qdf_mempool_t mempool_ol_rx_reorder_buf; /*  Memory pool for reorder buffers */

#if WMI_RECORDING
   struct proc_dir_entry *wmi_proc_entry;
#endif

#ifdef WLAN_FEATURE_FASTPATH
    void		    *htt_handle;
#endif /* WLAN_FEATURE_FASTPATH */


   u_int32_t                soc_idx;
   uint32_t                 tgt_sched_params;  /* target scheduler params */
   struct wlan_objmgr_psoc *psoc_obj;
   struct net_device       *netdev;
   struct ol_if_offload_ops *ol_if_ops;
   void *nbuf;
   struct ol_ath_radiostats     soc_stats;

    /* UMAC callback functions */
    void                    (*net80211_node_cleanup)(struct ieee80211_node *);
    void                    (*net80211_node_free)(struct ieee80211_node *);

#if OL_ATH_SUPPORT_LED
    const led_blink_rate  *led_blink_rate_table;   /* blinking rate table to be used */
#endif

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    struct ol_ath_softc_nss_soc nss_soc;
#endif

   int                      btcoex_enable;        /* btcoex enable/disable */
   int                      btcoex_wl_priority;   /* btcoex WL priority */
   int                      btcoex_duration;      /* wlan duration for btcoex */
   int                      btcoex_period;        /* sum of wlan and bt duration */
   int                      btcoex_gpio;          /* btcoex gpio pin # for WL priority */
   int                      btcoex_duty_cycle;    /* FW capability for btcoex duty cycle */

   qdf_spinlock_t           soc_lock;
   qdf_spinlock_t           soc_tgt_lock;
#if WLAN_SPECTRAL_ENABLE
   struct soc_spectral_stats spectral_stats;
#endif

    struct ol_ath_cookie    cookie;

   int wmi_diag_version;
   u_int8_t cal_in_flash; /* calibration data is stored in flash */
   u_int8_t cal_in_file; /* calibration data is stored in file on file system */
   u_int8_t is_target_paused;
   uint8_t  soc_lp_iot_vaps_mask;
#ifdef AH_CAL_IN_FLASH_PCI
    u_int8_t cal_idx; /* index of this radio in the CalAddr array */
#endif
   u_int8_t sc_in_delete:1;   /* don't add any more VAPs */

#if !defined(BUILD_X86) && !defined(CONFIG_X86)
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 24)
   struct qcom_pcie_register_event pcie_event;
   struct qcom_pcie_notify pcie_notify;
#endif
#endif
#ifdef WLAN_SUPPORT_TWT
#ifndef WLAN_TWT_CONV_SUPPORTED
   bool twt_enable;
   struct wmi_twt_enable_param twt;
#endif
#endif /* WLAN_SUPPORT_TWT */
   uint32_t                 tgt_iram_bkp_paddr;    /* tgt iram content available here */
   bool                     peerstats_enabled;
#if QLD
   qdf_event_t   qld_wait;  /*qdf event for upper layer transfer finish notify */
#endif
#ifdef WLAN_SUPPORT_RF_CHARACTERIZATION
   uint32_t num_rf_characterization_entries;
   struct wmi_host_rf_characterization_event_param *rf_characterization_entries;
#endif

   ol_ath_hw_mode_ctx_t hw_mode_ctx; /* context saved for handling dynamic
                                      * hw-mode change */
   ol_ath_rf_path_ctx_t rf_path_ctx; /* context saved for handling dunamic
                                      * rf path switch */
   ol_ath_ema_ctx_t ema_ctx;
   uint32_t num_tx_desc; /* Host mode */
   uint16_t num_tx_desc_0; /* NSS offload mode, pdev-0*/
   uint16_t num_tx_desc_1; /* NSS offload mode, pdev-1*/
   uint16_t num_tx_desc_2; /* NSS offload mode, pdev-2*/
   uint32_t re_ul_resp;
   config_cmd_log_cxt_t config_cmd_cxt; /* Context of config command log*/
   bool full_mon_mode_support; /* Full Monitor mode support */
   qdf_bitmap(tso_vdev_bitmap, WLAN_UMAC_PSOC_MAX_VDEVS); /* no of vdev's for which TSO is enabled*/
   uint64_t qtime_val;         /* Current qtime in SoC */
   uint64_t tbtt_soc_delta;    /* Delta between Current qtime and SoCs next Tbtt qtime */
   qdf_timer_t              tbtt_offset_sync_timer;
   uint8_t tbtt_offset_sync_timer_init;
   unsigned long tbtt_offset_sync_timer_running;
   struct ieee80211com *cp_stats_ic;
#ifdef QCA_SUPPORT_WDS_EXTENDED
   ol_txrx_rx_fp wds_ext_osif_rx;
#endif
   uint8_t fw_dump_file_name[FW_DUMP_FILE_NAME_SIZE];
   uint8_t max_rnr_ie_allowed;
#ifdef WIFI_MONITOR_SUPPORT
    struct mon_ops *soc_mon_ops;
#endif
   u_int32_t peer_ext_stats_cnt;
   uint8_t tqm_reset_flag;
#ifdef CONFIG_SAWF
   bool sawf;
#endif
   uint32_t pn_rxfilter; /* Rx Filter for frame forwarding upon PN error */

   /* Following set of identifiers for recovery only */
   int                     recovery_enable;	 /* enable/disable target recovery feature */
#define MAX_MOD_LOGS_LEVEL 4
   uint16_t recov_logs_level[MAX_MOD_LOGS_LEVEL]; /* WMI Logs level to be restored */
   bool     delay_bug_on;  /* This flag controls delaying BUG_ON in recovery*/
   u_int8_t recovery_in_progress;
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
   bool mlo_partner_hang_issued;
   bool target_ready;
   uint32_t recovery_type;
   uint32_t recovery_count;
   bool fallback_mode0;
   qdf_event_t fallback_event;
   bool mode1_recov_inprogress;
   bool retain_mlo_peer;
#endif
   bool glb_shmem_init_complete; /* This flag indicates if the global mlo arena init is completed */
#ifdef WLAN_MLO_GLOBAL_SHMEM_SUPPORT
   uint32_t *crash_info_address;
   uint32_t *recovery_mode_address;
#endif
  /* End of Recovery identifiers */
#define COMPLETE_WIFI_STAND_DOWN      1
#define INDIVIDUAL_RADIO_IN_STANDBY   2
   uint8_t standby_method;
#ifdef WLAN_FEATURE_11BE_MLO
   bool msd_info_override_enable;
   bool msd_info_present_override;
   uint16_t msd_info_value_override;
#endif /* WLAN_FEATURE_11BE_MLO */
#ifdef WLAN_FEATURE_11BE
   enum wlan_t2lm_enable t2lm_enable;
   enum default_t2lm_response default_t2lm_resp;
   uint8_t t2lm_app_version;
   uint8_t t2lm_driver_version;
   bool enable_eml_cap;
   bool rm_app_init_done;
   uint16_t dynamic_service_bitmask;
   bool rm_admctrl_service_enabled;
   bool rm_energy_service_enabled;
#endif /* WLAN_FEATURE_11BE */
#ifdef WLAN_SUPPORT_SCS_API
   uint8_t scs_api_app_support_enable:1,
           scs_api_frame_fwd_support:1,
           scs_api_nw_mgr_support:1,
           reserved:5;
#endif /* WLAN_SUPPORT_SCS_API */
   bool fw_ast_indication_disable;
   enum wlan_standby_modes standby_mode;
#ifdef WLAN_SUPPORT_DPDK
   qdf_uio_info_t uio_info;
#endif
#ifdef WLAN_DBS_POWER_REDUCTION
   bool enable_ap_pwr_reduction;
   uint8_t eth_port_count_th;
   uint8_t dbs_pwr_reduction_dbm;
   uint8_t eth_pwr_reduction_dbm;
#endif
} ol_ath_soc_softc_t;

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
struct ol_tgt_recovery_wq {
    uint8_t num_ready_socs[WLAN_MAX_MLO_GROUPS];
    struct ol_ath_soc_softc *soc_list[WLAN_MAX_MLO_GROUPS][MAX_MLO_CHIPS];
    qdf_work_t work;
    qdf_workqueue_t *work_queue;
    qdf_spinlock_t list_lock;
};
#endif

#if  ATH_DATA_TX_INFO_EN
struct ol_ath_txrx_ppdu_info_ctx {
    /* Pointer to parent scn handle */
    struct ol_ath_softc_net80211 *scn;

    /* lock to protect ppdu_info buffer queue */
    qdf_spinlock_t lock;

    /* work queue to process ppdu_info stats */
    qdf_work_t work;

    /* ppdu_info buffer queue */
    qdf_nbuf_queue_t nbufq;
};
#endif

struct ol_mgmt_softc_ctx {
    qdf_atomic_t         mgmt_pending_completions;
    qdf_nbuf_queue_t     mgmt_backlog_queue;
    qdf_spinlock_t       mgmt_backlog_queue_lock;
    bool                 mgmt_backlog_queue_lock_initialised;
    u_int16_t            mgmt_pending_max;       /* Max size of mgmt descriptor table */
    u_int16_t            mgmt_pending_probe_resp_threshold; /* Threshold size of mgmt descriptor table for probe responses */
};

typedef enum _CLI_DPD_STATUS {
    CLI_DPD_STATUS_DISABLED   =  0x0,  /* DPD disabled via CLI dpd_enable */
    CLI_DPD_STATUS_PASS       =  0x1,  /* DPD triggered via CLI dpd_enable and calibration passed */
    CLI_DPD_CMD_INPROGRES     =  0x2,  /* DPD triggered via CLI dpd_enable and no response received yet from target */
    /* Add any new status if any here */
    CLI_DPD_NA_STATE          =  0xFE, /* DPD not triggered via CLI command hence invalid/NA state */
    CLI_DPD_STATUS_FAIL       =  0xFF, /* DPD triggered via CLI dpd_enable & calibration failed OR DPD disabled BDF is loaded*/
} CLI_DPD_STATUS;

#if defined(BE_PKTLOG_SUPPORT) && defined(WLAN_FEATURE_11BE_MLO)
struct ol_ath_pdev_mlo_timestamp {
    A_UINT32   msg_type :8, /* bits  7:0 */
               pdev_id:2,   /* bits  9:8 */
               chip_id:2,   /* bits 11:10 */
               reserved1:4, /* bits 15:12 */
               mac_freq:16; /* bits 31:16*/

    /* lower 32 bits of the WLAN global time stamp at which last sync interrupt was received*/
    A_UINT32   sync_tstmp_lo_us;

    /*upper 32 bits of WLAN global time stamp at which last sync interrupt was received*/
    A_UINT32   sync_tstmp_hi_us;

    A_UINT32   mlo_offset_lo_us; /*lower 32 bits of the MLO offset in us*/
    A_UINT32   mlo_offset_hi_us; /*higher 32 bits of MLO offset in us*/
    A_UINT32   mlo_offset_clks; /*10 bit clock offset for sub us */
    A_UINT32   mlo_comp_us: 16, /*bits 15:0*/
               mlo_comp_clks: 10, /*bits 25:16*/
               reserved2: 6;
    /*bits 21:0 periodic timer at which compensation is applied*/
    A_UINT32   mlo_comp_timer_period_us : 22,
               reserved3 : 10;
};
#endif

/**
 * struct wlan_objmgr_pdev_ini_params - Pdev object to store
 *                                       pdev level ini parameter
 * @num_vdevs:         Number of vdevs for the pdev
 * @num_peers:         Number of peer for the pdev
 * @num_monitor_vaps:  Number of monitor vaps for the  pdev
 * @num_qwrap_vdevs:   Number of qwrap vdevs for the pdev
 * @num_qwrap_peers:   Number of qwrap peers for the pdev
 *
 */
struct max_config_params {
    uint32_t num_vdevs;
    uint32_t num_peers;
    uint32_t num_monitor_vaps;
    uint32_t num_qwrap_vdevs;
    uint32_t num_qwrap_peers;
};

struct ol_ath_softc_net80211 {
    struct ieee80211com     sc_ic;      /* NB: base class, must be first */
    ol_pktlog_dev_t 		*pl_dev;    /* Must be second- pktlog handle */
#if !(defined REMOVE_PKT_LOG) && (defined PKTLOG_DUMP_UPLOAD_SSR)
    int                     upload_pktlog;
#endif
    u_int32_t               sc_prealloc_idmask;   /* preallocated vap id bitmap: can only support 32 vaps */
    u_int32_t               macreq_enabled;     /* user mac request feature enable/disable */
    bool                    macreq_ext_enabled;     /* MAC request Ext mode to use static BSSID scheme */

#if ATH_DEBUG
    unsigned long rtsctsenable;
#endif

    osdev_t    sc_osdev;

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    struct ol_ath_softc_nss_radio nss_radio;
#endif
    struct ol_regdmn *ol_regdmn_handle;
    uint8_t                 ps_report;
    u_int8_t                bcn_mode;
    u_int8_t                burst_enable;
    int32_t                 cca_threshold;
    int32_t                 rxsop_sens_lvl;
#if ATH_SUPPORT_WRAP
    u_int8_t                mcast_bcast_echo;
    bool                    qwrap_enable;    /* enable/disable qwrap target config  */
#endif
    u_int8_t                dyngroup;
    u_int8_t                arp_override;
    u_int8_t                igmpmld_override;
    u_int8_t                igmpmld_tid;
    u_int16_t               burst_dur;
    periodic_chan_stats_t   mib_cycle_cnts;  /* used for channel utilization for ol model */
    /*
     * Includes host side stack level stats +
     * radio level athstats
     */
    wmi_host_dbg_stats   ath_stats;

    int                     tx_rx_time_info_flag;

    /* This structure is used to update the radio level stats, the stats
        are directly fetched from the descriptors
    */
    struct ieee80211_chan_stats chan_stats;     /* Used for channel radio-level stats */
    qdf_semaphore_t         scn_stats_sem;
    int16_t                 chan_nf;            /* noise_floor */
    int16_t                 cur_hw_nf;        /* noise_floor used to calculate Signal level*/
    u_int32_t               min_tx_power;
    u_int32_t               max_tx_power;
    u_int32_t               txpowlimit2G;
    u_int32_t               txpowlimit5G;
    u_int32_t               txpower_scale;
    u_int32_t               powerscale; /* reduce the final tx power */
    int32_t                 chan_tx_pwr;
    u_int32_t               special_ap_vap; /*ap_monitor mode*/
    u_int32_t               smart_ap_monitor; /*smart ap monitor mode*/
    u_int32_t               vdev_count;
    u_int32_t               mon_vdev_count;
    u_int32_t               bridge_vdev_count;
    qdf_spinlock_t       scn_lock;

    /** DCS configuration and running state */
    wlan_host_dcs_params_t   scn_dcs;

    u_int32_t               dtcs; /* Dynamic Tx Chainmask Selection enabled/disabled */
#if PERF_FIND_WDS_NODE
    struct wds_table        scn_wds_table;
#endif

    bool                    scn_cwmenable;    /*CWM enable/disable state*/
    bool                    is_ani_enable;    /*ANI enable/diable state*/
#if ATH_RX_LOOPLIMIT_TIMER
    qdf_timer_t          rx_looplimit_timer;
    u_int32_t               rx_looplimit_timeout;        /* timeout intval */
    bool                    rx_looplimit_valid;
    bool                    rx_looplimit;
#endif
    qdf_atomic_t            peer_count;
    int                     sc_chan_freq;           /* channel change freq in mhz */
    int                     sc_chan_band_center_f1; /* channel change band center freq in mhz */
    int                     sc_chan_band_center_f2; /* channel change band center freq in mhz */
    int                     sc_chan_phy_mode;    /* channel change PHY mode */

#if OL_ATH_SUPPORT_LED
    qdf_timer_t             scn_led_blink_timer;     /* led blinking timer */
    qdf_timer_t             scn_led_poll_timer;     /* led polling timer */
    OL_BLINK_STATE          scn_blinking; /* LED blink operation active */
    u_int32_t               scn_led_time_on;  /* LED ON time for current blink in ms */
    u_int32_t               scn_led_byte_cnt;
    u_int32_t               scn_led_total_byte_cnt;
    u_int32_t               scn_led_last_time;
    u_int32_t               scn_led_max_blink_rate_idx;
    u_int32_t               scn_led_gpio;
    u_int32_t               scn_led_gpio_flags;
    u_int32_t               scn_led_bw_per_idx;
#endif
    u_int32_t               enable_smart_antenna; /* enable smart antenna */
    u_int32_t               ol_rts_cts_rate;

    int                     sc_nstavaps;
    bool                    sc_is_blockdfs_set;
    u_int32_t               scn_last_peer_invalid_time;  /*Time interval since last invalid was sent */
    u_int32_t               scn_peer_invalid_cnt;      /* number of permissible dauth in interval */
    u_int32_t               scn_user_peer_invalid_cnt; /* configurable by user  */
    u_int32_t               aggr_burst_dur[WME_AC_VO+1]; /* maximum VO */
    u_int32_t               scn_sifs_frmtype;    /*SIFS RESP enable/disable state*/
    u_int32_t               scn_sifs_uapsd;    /*SIFS RESP UAPSD enable/disable state*/
    bool                    scn_block_interbss;  /* block interbss traffic */
    u_int16_t               txbf_sound_period;
    bool                    scn_promisc;            /* Set or clear promisc mode */
    atomic_t                sc_dev_enabled;    /* dev is enabled */
    struct thermal_param    thermal_param;
#if UNIFIED_SMARTANTENNA
    wdi_event_subscribe sa_event_sub;
#define MAX_TX_PPDU_SIZE 32
    uint32_t tx_ppdu_end[MAX_TX_PPDU_SIZE]; /* ppdu status for tx completion */
#endif
    u_int32_t               sc_dump_opts;       /* fw dump collection options*/

    u_int8_t                dpdenable;
    int8_t                  sc_noise_floor_th; /* signal noise floor in dBm used in ch hoping */
    u_int8_t                sc_enable_noise_detection; /* Enable/Disable noise detection due to channel hopping in acs */
    u_int8_t                scn_mgmt_retry_limit; /*Management retry limit*/
    u_int16_t               sc_noise_floor_report_iter; /* #of iteration noise is higher then threshold */
    u_int16_t               sc_noise_floor_total_iter;/* total # of iteration */
    u_int16_t               scn_amsdu_mask;
    u_int16_t               scn_ampdu_mask;
    uint16_t                scn_wmi_hang_after_time;
    uint16_t                scn_wmi_hang_wait_time;
    bool                    scn_wmi_dis_dump;    /* WMI disconnect dump collection flag */
    uint64_t                last_sent_time;      /* Stores time in msec when the last fw hang command was sent */


   u_int32_t                sc_arp_dbg_srcaddr;  /* ip address to monitor ARP */
   u_int32_t                sc_arp_dbg_dstaddr;  /* ip address to monitor ARP */
   u_int32_t                sc_arp_dbg_conf;   /* arp debug conf */
   u_int32_t                sc_tx_arp_req_count; /* tx arp request counters */
   u_int32_t                sc_rx_arp_req_count; /* rx arp request counters  */
   u_int32_t                sc_tx_arp_resp_count; /* tx arp response counters  */
   u_int32_t                sc_rx_arp_resp_count; /* rx arp response counters  */

   bool      periodic_chan_stats;
#if ATH_DATA_TX_INFO_EN
    u_int32_t               enable_perpkt_txstats;
#endif
   int                      enable_statsv2;
   int16_t                  chan_nf_sec80;            /* noise_floor secondary 80 */
   uint16_t                 user_config_txval;
   uint16_t                 user_config_rxval;

   uint32_t wifi_num;

   u_int32_t radio_id;
   ol_ath_soc_softc_t      *soc;
   struct net_device       *netdev;
   struct wlan_objmgr_pdev *sc_pdev;

#if WLAN_SPECTRAL_ENABLE
   QDF_STATUS (*wmi_spectral_configure_cmd_send)(void *wmi_hdl,
                struct vdev_spectral_configure_params *param);
   QDF_STATUS (*wmi_spectral_enable_cmd_send)(void *wmi_hdl,
                struct vdev_spectral_enable_params *param);
#endif

    /* UTF event information */
    struct {
        u_int8_t            *data;
        u_int8_t            currentSeq;
        u_int8_t            expectedSeq;
        u_int32_t           length;
        u_int16_t           offset;
    } utf_event_info;

    u_int32_t               max_clients;
    u_int32_t               max_vaps;

    u_int32_t               set_ht_vht_ies:1, /* true if vht ies are set on target */
                            set_he_ies:1;     /* true if HE ies are set on target */
#if ATH_DATA_TX_INFO_EN
    struct ieee80211_tx_status   *tx_status_buf;  /*per-msdu tx status info*/
#endif
    u_int8_t                vow_extstats;
    u_int8_t                retry_stats;
    u_int8_t                is_scn_stats_timer_init;
    uint8_t                 tx_ack_timeout; /* TX ack timeout value in microsec */
    uint8_t                 sc_presp_retry_limit; /* presp retry limit in us */
    uint8_t                 sc_cts_timeout;          /* cts timeout in us */
    uint8_t                 sc_slot_time;            /* slot time in us */
    u_int32_t               pdev_stats_timer;
    qdf_timer_t              scn_stats_timer;     /* stats  timer */
    wdi_event_subscribe stats_tx_monitor_frame_subscriber;
    wdi_event_subscribe stats_tx_data_subscriber;
    wdi_event_subscribe stats_rx_data_subscriber;
    wdi_event_subscribe stats_nondata_subscriber;
    wdi_event_subscribe stats_rx_nondata_subscriber;
    wdi_event_subscribe txrx_peer_create_subscriber;
    wdi_event_subscribe peer_unmap_subscriber;
    wdi_event_subscribe peer_flush_subscriber;
    wdi_event_subscribe peer_delete_subscriber;
    wdi_event_subscribe peer_primary_umac_update_subscriber;
    wdi_event_subscribe primary_mcast_update_subscriber;
    wdi_event_subscribe primary_sta_umac_update_subscriber;
#ifdef CONFIG_SAWF
    wdi_event_subscribe sawf_config_ul_subscriber;
#ifdef SAWF_ADMISSION_CONTROL
    wdi_event_subscribe sawf_peer_msduq_event_subscriber;;
#endif
#endif

#if QCN_IE
    wdi_event_subscribe stats_bpr_subscriber;
#endif
#ifdef QCA_UNDECODED_METADATA_SUPPORT
    wdi_event_subscribe rx_undecoded_ppdu_info_subscriber;
#endif
    uint32_t                soft_chain;
    wdi_event_subscribe stats_rx_subscriber;
    wdi_event_subscribe stats_tx_subscriber;
    atomic_t tx_metadata_ref;
    atomic_t rx_metadata_ref;
    atomic_t tx_data_frame_ref;
#if  ATH_DATA_TX_INFO_EN
    /* TxRx ppdu stats processing context */
    struct ol_ath_txrx_ppdu_info_ctx *tx_ppdu_stats_ctx;
    struct ol_ath_txrx_ppdu_info_ctx *rx_ppdu_stats_ctx;
#endif
    wdi_event_subscribe     htt_stats_subscriber;
    struct ol_mgmt_softc_ctx mgmt_ctx;
    wdi_event_subscribe     dp_stats_subscriber;
    wdi_event_subscribe     csa_phy_update_subscriber;
    wdi_event_subscribe     sojourn_stats_subscriber;
    wdi_event_subscribe     dp_rate_tx_stats_subscriber;
    wdi_event_subscribe     dp_rate_rx_stats_subscriber;
    wdi_event_subscribe     peer_create_subscriber;
    wdi_event_subscribe     peer_destroy_subscriber;
    wdi_event_subscribe     peer_flush_rate_stats_sub;
    wdi_event_subscribe     flush_rate_stats_req_sub;
    wdi_event_subscribe     hmwds_ast_add_status_subscriber;
#ifdef QCA_SUPPORT_LITE_MONITOR
    wdi_event_subscribe     lite_mon_rx_subscriber;
    wdi_event_subscribe     lite_mon_tx_subscriber;
#endif
    qdf_timer_t             auth_timer; /* auth timer */
    qdf_atomic_t            auth_cnt;   /* number of auth received DEFAULT_AUTH_CLEAR_TIMER */
    uint8_t                 max_auth;   /* maximum auth to receive in DEFAULT_AUTH_CLEAR_TIMER */
    uint8_t                 sc_bsta_fixed_idmask; /* Mask value to set fixed mac address for backhaul STA */
    u_int8_t                fw_disable_reset;
#if QCA_AIRTIME_FAIRNESS
    uint8_t     atf_strict_sched;
#endif
#if defined(BE_PKTLOG_SUPPORT) && defined(WLAN_FEATURE_11BE_MLO)
    struct ol_ath_pdev_mlo_timestamp tstamp;
#endif
#if CONFIG_SAWF_DEF_QUEUES
    /* Rate control radio parameters */
    uint8_t                rc_upper_cap_nss; /* upper cap nss */
    uint8_t                rc_upper_cap_mcs; /* upper cap mcs */
    uint8_t                rc_retry_thresh; /* retry collision threshold */
    uint8_t                rc_mcs_drop; /* number of mcs to drop */
    uint16_t               rc_mcs_probe_intvl_min; /* mcs probe interval min*/
    uint16_t               rc_mcs_probe_intvl_max; /* mcs probe interval max*/
    uint16_t               rc_nss_probe_intvl_min; /* nss probe interval min*/
    uint16_t               rc_nss_probe_intvl_max; /* nss probe interval max*/
#endif
    struct max_config_params max_params;
    ol_ath_ema_pdev_ctx_t ema_ctx;
    uint8_t vp_core_mask;
    bool host_trig_txbf_sounding;
#if QCA_STANDALONE_SOUNDING_TRIGGER
    bool sc_host_trig_sounding_support;
    bool sc_sounding_inprogress;
    qdf_time_t sc_txbf_sounding_last_cmd_time;
    struct txbf_cbf_ctx cbf_ctx;
#endif
#if QCA_MANUAL_TRIGGERED_ULOFDMA
    bool sc_mulofdma_support;
    bool sc_mulofdma_ev_pending;
    qdf_time_t sc_mulofdma_last_cmd_time;
    ol_man_trig_ulofdma_ctx ulofdma_ctx;
#endif
    char unique_id[PKTLOG_UNIQUE_ID_SIZE];
    char radio_name[RADIO_NAME_SIZE];
    uint8_t disabled_mode;
#if defined(IPA_OFFLOAD) && defined(IPA_OFFLOAD_LOW_MEM)
    uint16_t num_ipa_rx_buff;
    uint16_t num_ipa_tx_buff;
#endif
#ifdef WLAN_SUPPORT_TX_PKT_CAP_CUSTOM_CLASSIFY
    uint32_t tx_pkt_cap_proto;
#endif
};

#define OL_ATH_PPDU_STATS_BACKLOG_MAX 16

#define PDEV_ID(scn) scn->pdev_id
#define PDEV_UNIT(pdev_id) (pdev_id)

#define ol_scn_host_80211_enable_get(_ol_pdev_hdl) \
    (wlan_psoc_nif_feat_cap_get(\
                  ((struct ol_ath_softc_net80211 *)(_ol_pdev_hdl))->soc->psoc_obj,\
                                          WLAN_SOC_F_HOST_80211_ENABLE))

#define ol_scn_target_revision(_ol_pdev_hdl) \
    (((struct ol_ath_softc_net80211 *)(_ol_pdev_hdl))->soc->target_revision)

#define OL_ATH_SOFTC_NET80211(_ic)     ((struct ol_ath_softc_net80211 *)(_ic))

struct prb_rsp_entry {
    A_BOOL                        is_dma_mapped;
    wbuf_t                        prb_rsp_buf;
    TAILQ_ENTRY(prb_rsp_entry)    deferred_prb_rsp_list_elem;
};

struct bcn_buf_entry {
    A_BOOL                        is_dma_mapped;
    wbuf_t                        bcn_buf;
    TAILQ_ENTRY(bcn_buf_entry)    deferred_bcn_list_elem;
};

struct ol_ath_vap_net80211 {
    struct ieee80211vap             av_vap; /* NB: base class, must be first */
    struct ieee80211_beacon_offsets av_beacon_offsets; /* bcn fields offsets */
    struct ieee80211_beacon_offsets av_prb_rsp_offsets; /* unsolicited probe resp offsets */
    struct ieee80211_beacon_offsets av_reg_prb_rsp_offsets; /* regular probe resp offsets */
    qdf_spinlock_t                  avn_lock;
    TAILQ_HEAD(, bcn_buf_entry)  deferred_bcn_list; /* Deferred bcn buf list */
    TAILQ_HEAD(, prb_rsp_entry)  deferred_prb_rsp_list;
    struct ieee80211_ath_channel *av_ol_resmgr_chan; /* Channel ptr in target*/
    struct ieee80211_ath_channel  av_ol_resmgr_chan_obj; /* Channel obj in target*/
    wbuf_t          av_wbuf;                 /* Beacon buffer */
    wbuf_t          av_pr_rsp_wbuf;          /* 20 TU pr Resp buf */
    uint32_t        vdev_param_capabilities; /*vdev param capabilities state*/
    bool            is_dma_mapped;
};

#define OL_ATH_VAP_NET80211(_vap)      ((struct ol_ath_vap_net80211 *)(_vap))

struct ol_ath_node_net80211 {
    struct ieee80211_node       an_node;     /* NB: base class, must be first */
};

#define OL_ATH_NODE_NET80211(_ni)      ((struct ol_ath_node_net80211 *)(_ni))

/* Below BW_GAIN should be added to the SNR value of every ppdu based on the
 * bandwidth. This table is obtained from HALPHY.
 * BW         BW_Gain
 * 20          0
 * 40          3dBm
 * 80          6dBm
 * 160/80P80   9dBm
 * 320         12dBm
 */

#define BW_GAIN_20MHZ 0
#define BW_GAIN_40MHZ 3
#define BW_GAIN_80MHZ 6
#define BW_GAIN_160MHZ 9
#ifdef WLAN_FEATURE_11BE
#define BW_GAIN_320MHZ 12
#endif

/**
 * ol_ath_get_bw_offset() - Get BW offset from channel width
 * @ni_chwidth: channel width of the node
 *
 * Return: Bandwidth offset derived from channel width
 */
static inline uint8_t ol_ath_get_bw_offset(enum ieee80211_cwm_width ni_chwidth)
{
    uint8_t bw_offset = 0;

    switch (ni_chwidth) {
    case IEEE80211_CWM_WIDTH20:
        bw_offset = BW_GAIN_20MHZ;
        break;
    case IEEE80211_CWM_WIDTH40:
        bw_offset = BW_GAIN_40MHZ;
        break;
    case IEEE80211_CWM_WIDTH80:
        bw_offset = BW_GAIN_80MHZ;
        break;
    case IEEE80211_CWM_WIDTH160:
    case IEEE80211_CWM_WIDTH80_80:
        bw_offset = BW_GAIN_160MHZ;
        break;
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_CWM_WIDTH320:
        bw_offset = BW_GAIN_320MHZ;
        break;
#endif
    default:
        break;
    }

    return bw_offset;
}

#if OBSS_PD
/**
 * enum srtype - Spatial reuse type
 * @SR_TYPE_NON_SRG_OBSS_PD : Non-SRG based OBDD PD Spatial reuse
 * @SR_TYPE_SRG_OBSS_PD : SRG based OBDD PD Spatial reuse
 * @SR_TYPE_PSR: PSR based Spatial Reuse
 * @SR_TYPE_OBSS_PD : OBSS PD Spatial reuse. Considers both SRG and Non-SRG based.
 */
enum srtype {
    SR_TYPE_NON_SRG_OBSS_PD = 0,
    SR_TYPE_SRG_OBSS_PD,
    SR_TYPE_PSR,
    SR_TYPE_OBSS_PD,
    SR_TYPE_MAX,
};

struct standby_context {
     struct net_device *dev;
     struct net_device *wakeup_dev;
     bool wq_init;
     qdf_work_t work;
     qdf_workqueue_t *work_queue;
     qdf_spinlock_t list_lock;
     bool trigger_sent;
#ifdef WLAN_NETSTANDBY_SUPPORT
     uint16_t protocol;
     netstandby_event_compl_cb_t enter_cmp_cb;
     netstandby_event_compl_cb_t exit_cmp_cb;
     netstandby_trigger_notif_cb_t trigger_cb;
#endif
};

#define SR_TYPE_LENGTH 16
static inline enum srtype ol_ath_extact_sr_type(void *data, uint32_t data_len)
{
    char srtype[SR_TYPE_LENGTH];

    if (!data_len)
        goto err;

    if (data_len > SR_TYPE_LENGTH-1)
        data_len = SR_TYPE_LENGTH-1;

    qdf_mem_copy(srtype, data, data_len);
    srtype[data_len] = '\0';

    if (qdf_str_eq(srtype, "non-srg"))
        return SR_TYPE_NON_SRG_OBSS_PD;
    else if(qdf_str_eq(srtype, "srg"))
        return SR_TYPE_SRG_OBSS_PD;
    else if(qdf_str_eq(srtype, "obss_pd"))
        return SR_TYPE_OBSS_PD;
    else if(qdf_str_eq(srtype, "psr"))
        return SR_TYPE_PSR;

err:
    qdf_err("Wrong Spatial reuse type");
    return SR_TYPE_MAX;
}

static inline int set_obss_pd_enable_bit(uint32_t *threshold,
    enum srtype type, uint8_t value)
{
    switch (type) {
    case SR_TYPE_NON_SRG_OBSS_PD:
        SET_SELF_OBSS_PD_ENABLE(NON_SRG, *threshold, value);
        break;

    case SR_TYPE_SRG_OBSS_PD:
        SET_SELF_OBSS_PD_ENABLE(SRG, *threshold, value);
        break;

    default:
        qdf_err("Invalid Spatial Reuse type");
        return -1;
    }

    return 0;
}

static inline int set_obss_pd_threshold(uint32_t *threshold,
    enum srtype type, uint8_t value)
{
    switch (type) {
    case SR_TYPE_NON_SRG_OBSS_PD:
        SET_SELF_OBSS_PD_THRESH(NON_SRG, *threshold, value);
        break;

    case SR_TYPE_SRG_OBSS_PD:
        SET_SELF_OBSS_PD_THRESH(SRG, *threshold, value);
        break;

    default:
        qdf_err("Invalid Spatial Reuse type");
        return -1;
    }

    return 0;
}

static inline int get_obss_pd_enable_bit(uint32_t threshold,
    enum srtype type, uint8_t *value)
{
    switch (type) {
    case SR_TYPE_NON_SRG_OBSS_PD:
        *value = GET_SELF_OBSS_PD_ENABLE(NON_SRG, threshold);
        break;

    case SR_TYPE_SRG_OBSS_PD:
        *value = GET_SELF_OBSS_PD_ENABLE(SRG, threshold);
        break;

    default:
        qdf_err("Invalid Spatial Reuse type");
        return -1;
    }

    return 0;
}

static inline int get_obss_pd_threshold(uint32_t threshold,
    enum srtype type, uint8_t *value)
{
    switch (type) {
    case SR_TYPE_NON_SRG_OBSS_PD:
        *value = GET_SELF_OBSS_PD_THRESH(NON_SRG, threshold);
        break;

    case SR_TYPE_SRG_OBSS_PD:
        *value = GET_SELF_OBSS_PD_THRESH(SRG, threshold);
        break;

    default:
        qdf_err("Invalid Spatial Reuse type");
        return -1;
    }

    return 0;
}

static inline int set_sr_per_ac(uint32_t *enable_bitmap,
    enum srtype type, uint8_t value)
{
    switch (type) {
    case SR_TYPE_OBSS_PD:
        SET_SELF_SR_PER_AC(OBSS_PD, *enable_bitmap, value);
        break;

    case SR_TYPE_PSR:
        SET_SELF_SR_PER_AC(PSR, *enable_bitmap, value);
        break;

    default:
        qdf_err("Invalid Spatial Reuse type");
        return -1;
    }

    return 0;
}

static inline int get_sr_per_ac(uint32_t enable_bitmap,
    enum srtype type, uint8_t *value)
{
    switch (type) {
    case SR_TYPE_OBSS_PD:
        *value = GET_SELF_SR_PER_AC(OBSS_PD, enable_bitmap);
        break;

    case SR_TYPE_PSR:
        *value = GET_SELF_SR_PER_AC(PSR, enable_bitmap);
        break;

    default:
        qdf_err("Invalid Spatial Reuse type");
        return -1;
    }

    return 0;
}

static inline void set_obss_pd_threshold_unit(uint32_t *threshold, uint8_t value)
{
	SR_SET_FIELD(*threshold, value, SELF_OBSS_PD_THRESH_UNITS);
}
#endif

int ol_ath_handle_tqm_reset(struct ol_ath_softc_net80211 *scn, uint32_t enable);
bool ol_ath_is_dynamic_hw_mode_enabled(ol_ath_soc_softc_t *soc);
uint8_t ol_ath_get_max_supported_radios(ol_ath_soc_softc_t *soc);
int ol_ath_handle_hw_mode_switch(struct ol_ath_softc_net80211 *scn, uint32_t mode);
int ol_ath_handle_rf_path_switch(struct ol_ath_softc_net80211 *scn, uint32_t mode);
int ol_ath_set_hw_mode_omn_timer(struct ol_ath_softc_net80211 *scn,
        uint32_t omn_timeout);
int ol_ath_set_hw_mode_omn_enable(struct ol_ath_softc_net80211 *scn,
				  int enable);
QDF_STATUS ol_ath_set_hw_mode_primary_if(struct ol_ath_softc_net80211 *scn,
				  unsigned int if_num);

bool ol_ath_is_ifce_allowed_in_dynamic_hw_mode(void *i_scn);

void ol_target_failure(void *instance, QDF_STATUS status);

int ol_ath_soc_attach(ol_ath_soc_softc_t *soc, IEEE80211_REG_PARAMETERS *ieee80211_conf_parm, ol_ath_update_fw_config_cb cb);

void ol_ath_assign_mbssid_ref_bssid(void *i_scn, bool partially_random);

void ol_ath_assign_cohosted_ref_bssid(void *i_scn, bool partially_random);

int ol_ath_pdev_attach(struct ol_ath_softc_net80211 *scn, IEEE80211_REG_PARAMETERS *ieee80211_conf_parm, uint8_t phy_id);

int ol_asf_adf_attach(ol_ath_soc_softc_t *soc);

int ol_asf_adf_detach(ol_ath_soc_softc_t *soc);

#ifdef MU_CAP_WAR_ENABLED
void ieee80211_mucap_vattach(struct ieee80211vap *vap);
void ieee80211_mucap_vdetach(struct ieee80211vap *vap);
#endif

void ol_ath_target_status_update(ol_ath_soc_softc_t *soc, ol_target_status status);

int ol_ath_soc_detach(ol_ath_soc_softc_t *soc, int force);
int ol_ath_pdev_detach(struct ol_ath_softc_net80211 *scn, int force);

QDF_STATUS ol_ath_unsubscribe_peer_wdi_event(struct ol_ath_softc_net80211 *scn);
QDF_STATUS ol_ath_subscribe_peer_wdi_event(struct ol_ath_softc_net80211 *scn);

#ifdef QVIT
void ol_ath_qvit_detach(struct ol_ath_softc_net80211 *scn);
void ol_ath_qvit_attach(struct ol_ath_softc_net80211 *scn);
void ol_ath_pdev_qvit_detach(struct ol_ath_softc_net80211 *scn);
void ol_ath_pdev_qvit_attach(struct ol_ath_softc_net80211 *scn);
#endif

#ifdef CE_TASKLET_DEBUG_ENABLE
void ol_ath_enable_ce_latency_stats(struct ol_ath_soc_softc *soc, uint8_t val);
#endif
void ol_ath_suspend_resume_attach(struct ol_ath_softc_net80211 *scn);

int ol_ath_resume(struct ol_ath_softc_net80211 *scn);
int ol_ath_suspend(struct ol_ath_softc_net80211 *scn);

bool ol_ath_non_ap_required_checks_to_clear_bitmap(struct ieee80211com *ic,
                                                   u_int8_t *bssid);
void ol_ath_vap_attach(struct ieee80211com *ic);
void ol_ath_vap_soc_attach(ol_ath_soc_softc_t *soc);
int ol_ath_recover_deferred_start(ol_ath_soc_softc_t *soc);
int ol_ath_recover_start(struct ol_ath_soc_softc *soc);
int ol_ath_recover_work_queue_init(void);
int ol_ath_recover_work_queue_reset(void);
int ol_ath_recover_work_queue_deinit(void);
void ol_recovery_check_beacon_stop(struct ieee80211vap *vap);

int ol_ath_cwm_attach(struct ol_ath_softc_net80211 *scn);

void ol_ath_soc_rate_stats_attach(ol_ath_soc_softc_t *soc);
void ol_ath_soc_rate_stats_detach(ol_ath_soc_softc_t *soc);

struct ieee80211vap *ol_ath_vap_get(struct ol_ath_softc_net80211 *scn, u_int8_t vdev_id);
struct ieee80211vap *ol_ath_getvap(osif_dev *osdev);
u_int8_t ol_ath_vap_get_myaddr(struct ol_ath_softc_net80211 *scn, u_int8_t vdev_id,
                                 u_int8_t *macaddr);
struct ieee80211vap *ol_ath_pdev_vap_get(struct wlan_objmgr_pdev *pdev, u_int8_t vdev_id);
QDF_STATUS ol_if_enable_hw_vlan_insert(wlan_if_t vap,
                                                struct ieee80211_node *ni);
void ol_ath_release_vap(struct ieee80211vap *vap);

void ol_ath_beacon_attach(struct ieee80211com *ic);
void ol_ath_beacon_soc_attach(ol_ath_soc_softc_t *soc);

int ol_ath_node_attach(struct ol_ath_softc_net80211 *scn, struct ieee80211com *ic);
int ol_ath_node_soc_attach(ol_ath_soc_softc_t *soc);

void ol_ath_resmgr_attach(struct ieee80211com *ic);

int ol_ath_get_ofdma_max_users(struct ol_ath_soc_softc *soc);
int ol_ath_get_mumimo_max_users(struct ol_ath_soc_softc *soc);

#if QCA_AIRTIME_FAIRNESS
int ol_ath_set_atf(struct ieee80211com *ic);
int ol_ath_send_atf_peer_request(struct ieee80211com *ic);
int ol_ath_set_atf_grouping(struct ieee80211com *ic);
int ol_ath_set_bwf(struct ieee80211com *ic);
#endif

void ol_chan_stats_event (struct ieee80211com *ic,
         periodic_chan_stats_t *pstats, periodic_chan_stats_t *nstats);

int ol_ath_invalidate_channel_stats(struct ieee80211com *ic);

int ol_ath_periodic_chan_stats_config(struct ol_ath_softc_net80211 *scn,
        bool enable, u_int32_t stats_period);

void ol_ath_power_attach(struct ieee80211com *ic);

struct ieee80211_ath_channel
ol_ath_find_full_channel(struct ieee80211com *ic, struct chan_params *chan_p);

int ol_ath_vap_send_data(struct ieee80211vap *vap, wbuf_t wbuf);

void ol_ath_vap_send_hdr_complete(void *ctx, HTC_PACKET_QUEUE *htc_pkt_list);

void ol_ath_check_and_reconfig_hw_mode(ol_ath_soc_softc_t *soc);

QDF_STATUS ol_ath_hw_mode_setup_ctx(ol_ath_soc_softc_t *soc);

void ol_rx_indicate(void *ctx, wbuf_t wbuf);

void ol_rx_handler(void *ctx, HTC_PACKET *htc_packet);

extern int ol_ath_do_waltest(struct device *dev);

/**
  * ol_ath_check_and_start_target: API to check if the target is down and start it
  *                                if the conditions are met, else do not start the target
  * scn: Pointer to scn struct
  * Return: 0: target is started or condition to start target is not set, non zero: failed return
 */
int ol_ath_check_and_start_target(struct ol_ath_softc_net80211 *scn);

enum ol_rx_err_type {
    OL_RX_ERR_DEFRAG_MIC,
    OL_RX_ERR_PN,
    OL_RX_ERR_UNKNOWN_PEER,
    OL_RX_ERR_MALFORMED,
    OL_RX_ERR_TKIP_MIC,
};

/**
 * @brief Provide notification of failure during host rx processing
 * @details
 *  Indicate an error during host rx data processing, including what
 *  kind of error happened, when it happened, which peer and TID the
 *  erroneous rx frame is from, and what the erroneous rx frame itself
 *  is.
 *
 * @param vdev_id - ID of the virtual device received the erroneous rx frame
 * @param peer_mac_addr - MAC address of the peer that sent the erroneous
 *      rx frame
 * @param tid - which TID within the peer sent the erroneous rx frame
 * @param tsf32  - the timstamp in TSF units of the erroneous rx frame, or
 *      one of the fragments that when reassembled, constitute the rx frame
 * @param err_type - what kind of error occurred
 * @param rx_frame - the rx frame that had an error
 */
void
ol_rx_err(
    ol_pdev_handle pdev,
    u_int8_t vdev_id,
    u_int8_t *peer_mac_addr,
    int tid,
    u_int32_t tsf32,
    enum ol_rx_err_type err_type,
    qdf_nbuf_t rx_frame);


enum ol_rx_notify_type {
    OL_RX_NOTIFY_IPV4_IGMP,
};

/*
 * The enum values are alligned with the
 * values used by 3.0. This ensures compatibility
 * across both the versions.
 */
enum {
    OL_TIDMAP_PRTY_DSCP_SVLAN_HLOS = 0,
    OL_TIDMAP_PRTY_DSCP_CVLAN_HLOS = 1,
    OL_TIDMAP_PRTY_DSCP_HLOS_SVLAN = 2,
    OL_TIDMAP_PRTY_DSCP_HLOS_CVLAN = 3,
    OL_TIDMAP_PRTY_SVLAN_DSCP_HLOS = 4,
    OL_TIDMAP_PRTY_CVLAN_DSCP_HLOS = 5,
    OL_TIDMAP_PRTY_SVLAN_HLOS_DSCP = 6,
    OL_TIDMAP_PRTY_CVLAN_HLOS_DSCP = 7,
    OL_TIDMAP_PRTY_HLOS_SVLAN_DSCP = 8,
    OL_TIDMAP_PRTY_HLOS_CVLAN_DSCP = 9,
    OL_TIDMAP_PRTY_HLOS_DSCP_SVLAN = 10,
    OL_TIDMAP_PRTY_HLOS_DSCP_CVLAN = 11,
};

struct firmware_priv {
	size_t size;
	const u8 *data;
	void **pages;
	/* firmware loader private fields */
	void *priv;
};

/**
 * @brief Handle add wds entry event
 *
 * @param soc - opaque handle to psoc
 * @param vdev_id - ID of the virtual device
 * @param peer_mac - MAC address of the peer
 * @param peer_id - peer id of the peer
 * @param dest_mac - MAC address of the destination
 * @param next_node_mac - MAC address of the next node corresponding to destination
 * @param flags - flags
 * @param type - type of entry
 *
 * Return: Integer value indicating status
 */
int ol_ath_node_add_wds_entry(struct cdp_ctrl_objmgr_psoc *soc, uint8_t vdev_id,
                              uint8_t *peer_mac, uint16_t peer_id, const u_int8_t *dest_mac,
                              u_int8_t *next_node_mac, u_int32_t flags, u_int8_t type);

/**
 * @brief Handle update wds entry event
 *
 * @param psoc - opaque handle to psoc
 * @param vdev_id - ID of the virtual device
 * @param wds_macaddr - wds MAC address
 * @param peer_macaddr - MAC address of the peer
 * @param flags - flags
 *
 * Return: Integer value indicating status
 */
int ol_ath_node_update_wds_entry(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t vdev_id,
                                 u_int8_t *wds_macaddr,
                                 u_int8_t *peer_macaddr, u_int32_t flags);

/**
 * @brief Handle delete wds entry event
 *
 * @param soc - opaque handle to psoc
 * @param vdev_id - ID of the virtual device
 * @param dest_mac - MAC address of the destination to be deleted
 * @param type - type of entry
 * @param delete_in_fw - Flag to indicate if entry needs to be deleted in fw
 *
 * Return: None
 */
void ol_ath_node_del_wds_entry(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t vdev_id,
                               u_int8_t *dest_mac,
                               uint8_t type, uint8_t delete_in_fw);

#ifdef WLAN_FEATURE_MULTI_AST_DEL
/**
 * @brief Handle delete multi wds entry event
 *
 * @param psoc - opaque handle to psoc
 * @param vdev_id - ID of the virtual device
 * @param wds_list - Struct containing MAC address of the destination to be
 * deleted
 *
 * Return: None
 */
void ol_ath_node_del_multi_wds_entry(
		 struct cdp_ctrl_objmgr_psoc *psoc, uint8_t vdev_id,
		 struct peer_del_multi_wds_entries *wds_list);
#endif

#ifdef RX_PEER_INVALID_ENH
/**
 * @brief Handle invalid peer event
 *
 * @param soc - opaque handle to psoc
 * @param pdev_id - ID of the physical device
 * @param msg - wdi msg
 *
 * Return: Integer value indicating status
 */
uint8_t ol_ath_rx_invalid_peer(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t pdev_id, void *msg);
#endif
/**
 * @brief Handle peer deleted event
 *
 * @param soc - opaque handle to psoc
 * @param pdev_id - ID of the physical device
 * @param peer_mac - MAC address of the peer
 * @param vdev_mac - MAC address of vdev
 * @param opmode - opmode of peer
 *
 * Return: Integer value indicating status
 */
int ol_ath_peer_unref_delete(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t pdev_id, uint8_t *peer_mac,
                             uint8_t *vdev_mac, enum wlan_op_mode opmode);
/**
 * @brief Handle peer map event
 *
 * @param soc - opaque handle to psoc
 * @param peer_id - ID of the peer
 * @param hw_peer_id - hw ID of the peer
 * @param vdev_id - id of vdev
 * @param peer_mac_addr - MAC address of the peer
 * @param peer_type - type of peer
 * @param tx_ast_hash - ast hash value
 *
 * Return: Integer value indicating status
 */
int ol_peer_map_event(struct cdp_ctrl_objmgr_psoc *psoc, uint16_t peer_id, uint16_t hw_peer_id,
                      uint8_t vdev_id, uint8_t *peer_mac_addr, enum cdp_txrx_ast_entry_type  peer_type,
                      uint32_t tx_ast_hash);
/**
 * @brief Handle peer unmap event
 *
 * @param psoc - opaque handle to psoc
 * @param peer_id - ID of the peer
 * @param vdev_id - id of vdev
 * @param mac_addr - pointer to peer Mac address
 *
 * Return: Integer value indicating status
 */
int ol_peer_unmap_event(struct cdp_ctrl_objmgr_psoc *psoc, uint16_t peer_id,
                        uint8_t vdev_id, uint8_t *mac_addr);
/**
 * @brief Send event to ECM to flush connections for a flow
 *
 * @param soc - opaque handle to psoc
 * @param vdev_id - id of vdev
 * @param mac_addr - mac address
 *
 * Return: none
 */
void ol_ath_send_ecm_event(struct cdp_ctrl_objmgr_psoc *soc,
                           uint8_t vdev_id,
                           const uint8_t *mac_addr);

#ifdef SAWF_ADMISSION_CONTROL
/**
 * @brief Send SAWF Peer MSDU Event to RM
 *
 * @param vdev - vdev handle
 * @param intf - Pointer to interface structure
 *
 * Return: none
 */
void ol_ath_send_sawf_peer_msduq_event(struct wlan_objmgr_vdev *vdev,
                                       struct cdp_sawf_peer_msduq_event_intf *intf);
#endif

/**
 * @brief Handle mic error event
 *
 * @param soc - opaque handle to psoc
 * @param peer_id - ID of the peer
 * @param info - mic error info
 *
 * Return: None
 */
void ol_ath_rx_mic_error(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t pdev_id, struct cdp_rx_mic_err_info *info);

/**
 * @brief Handle sta kickout event
 *
 * @param soc - opaque handle to psoc
 * @param peer_id - ID of the peer
 * @param peer_mac - mac address of peer
 *
 * Return: Integer value indicating status
 */
int ol_ath_peer_sta_kickout(struct cdp_ctrl_objmgr_psoc *psoc, uint16_t peer_id, uint8_t *peer_mac);

/**
 * @brief Handle multiple wds entry delete event
 *
 * @param soc - opaque handle to psoc
 * @param vdev_id - ID of the virtual device
 * @param wds_macaddr - MAC address of wds
 * @param peer_mac - MAC address of the peer
 * @param flags - flags
 *
 * Return: Integer value indicating status
 */
int
ol_ath_node_delete_multiple_wds_entries(struct cdp_ctrl_objmgr_psoc *psoc,
                                        uint8_t vdev_id, uint8_t *wds_macaddr,
                                        uint8_t *peer_macaddr, uint32_t flags);
int ol_ath_pdev_update_lmac_n_target_pdev_id(struct cdp_ctrl_objmgr_psoc *psoc,
                                             uint8_t *pdev_id, uint8_t *lmac_id,
                                             uint8_t *target_pdev_id);

/**
 * @brief Handle get device name
 *
 * @param soc - opaque handle to psoc
 * @param pdev_id - ID of the pdev
 *
 * Return: pointer to device name
 */
char *ol_ath_get_pdev_dev_name(struct cdp_ctrl_objmgr_psoc *ctrl_soc,
                                uint8_t pdev_id);

void
ol_ath_mgmt_soc_attach(ol_ath_soc_softc_t *soc);
void ol_ath_mgmt_attach(struct ieee80211com *ic);
void ol_ath_mgmt_detach(struct ieee80211com *ic);
void ol_ath_mgmt_tx_complete(void *ctxt, wbuf_t wbuf, int err);
void ol_ath_mgmt_register_offload_beacon_tx_status_event(
        struct ieee80211com *ic, bool unregister);

/**
 * ol_ath_mgmt_register_bss_color_collision_det_config_evt() - Register
 * bss color collision detect event
 * @ic: ic handle
 *
 * Return: none
 */
void ol_ath_mgmt_register_bss_color_collision_det_config_evt(
        struct ieee80211com *ic);

/**
 * ol_ath_beacon_alloc() - allocates beacon buffer
 * @vap: vap pointer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ol_ath_beacon_alloc(struct ieee80211vap *vap);

/**
 * ol_ath_beacon_stop() - Stops beacon
 * @avn: pointer to avn
 *
 * Return: none
 */
void ol_ath_beacon_stop(struct ol_ath_vap_net80211 *avn);

/**
 * ol_ath_20tu_prb_rsp_alloc() - allocates probe response
 * @ic: ic pointer
 * @if_id: interface id
 *
 * Return: none
 */
void ol_ath_20tu_prb_rsp_alloc(struct ieee80211com *ic, int if_id);

/**
 * ol_ath_prb_rsp_stop() - Stops probe response
 * @avn: pointer to avn
 *
 * Return: none
 */
void ol_ath_prb_rsp_stop(struct ol_ath_vap_net80211 *avn);

/**
 * ol_ath_beacon_free() - frees beacon
 * @vap: pointer to ieee80211 vap
 *
 * Return: none
 */
void ol_ath_beacon_free(struct ieee80211vap *vap);

/**
 * ol_ath_bcn_tmpl_send() - sends beacon template
 * @vdev_id: vdev id
 * @vap: pointer to ieee80211 vap
 *
 * Return: 0 on success, other value on failure
 */
int ol_ath_bcn_tmpl_send(uint8_t vdev_id, struct ieee80211vap *vap);

/**
 * ol_ath_prb_rsp_free() - free probe response
 * @vap: pointer to ieee80211 vap
 *
 * Return: none
 */
void ol_ath_prb_rsp_free(struct ieee80211vap *vap);

/**
 * ol_ath_prb_resp_tmpl_send() - sends probe response template
 * @vdev_id: vdev id
 * @vap: pointer to ieee80211 vap
 *
 * Return: 0 on success, other value on failure
 */
int ol_ath_prb_resp_tmpl_send(uint8_t vdev_id, struct ieee80211vap *vap);

int ol_ath_net80211_newassoc(struct ieee80211_node *ni, int isnew);

void ol_ath_phyerr_attach(ol_ath_soc_softc_t *soc);
void ol_ath_phyerr_detach(ol_ath_soc_softc_t *soc);

/*
 * ol_ath_phyerr_enable() - Enable PHY errors
 * @ic: ic pointer
 *
 * For now, this just enables the DFS PHY errors rather than
 * being able to select which PHY errors to enable
 *
 * Return: none
 */
void ol_ath_phyerr_enable(struct ieee80211com *ic);

/*
 * ol_ath_phyerr_disable() - Disbale PHY errors
 * @ic: ic pointer
 *
 * For now, this just disables the DFS PHY errors rather than
 * being able to select which PHY errors to disable
 *
 * Return: none
 */
void ol_ath_phyerr_disable(struct ieee80211com *ic);

int
ol_transfer_target_eeprom_caldata(ol_ath_soc_softc_t *soc, u_int32_t address, bool compressed);

int
ol_transfer_bin_file(ol_ath_soc_softc_t *soc, ATH_BIN_FILE file,
                    u_int32_t address, bool compressed);

int
ol_ath_request_firmware(struct firmware_priv **fw_entry, const char *file,
		                        struct device *dev, int dev_id);
void
ol_ath_release_firmware(struct firmware_priv *fw_entry);

int
__ol_ath_check_wmi_ready(ol_ath_soc_softc_t *soc);


void __ol_target_paused_event(ol_ath_soc_softc_t *soc);

u_int32_t host_interest_item_address(u_int32_t target_type, u_int32_t item_offset);

int
ol_ath_set_config_param(struct ol_ath_softc_net80211 *scn,
        enum _ol_ath_param_t param, void *buff, bool *restart_vaps, int *extra_buffer);

int
ol_ath_get_config_param(struct ol_ath_softc_net80211 *scn, enum _ol_ath_param_t param, void *buff);

int
ol_hal_set_config_param(struct ol_ath_softc_net80211 *scn, enum _ol_hal_param_t param, void *buff);

int
ol_hal_get_config_param(struct ol_ath_softc_net80211 *scn, enum _ol_hal_param_t param, void *buff);

int
ol_net80211_set_mu_whtlist(wlan_if_t vap, u_int8_t *macaddr, u_int16_t tidmask);

/*
 * ol_ath_config_bss_color_offload() - bss color offload function
 * @vap: pointer to vap
 * @disable: flag to enable/disable collision detection
 *
 * Return: none
 */
void ol_ath_config_bss_color_offload(wlan_if_t vap, bool disable);

uint32_t ol_get_phymode_info(struct ol_ath_softc_net80211 *scn,
        uint32_t chan_mode, bool is_2gvht_en);

/**
 * ol_get_phymode_info_from_wlan_phymode() - Converts wlan_phymode to WMI_HOST_WLAN_PHY_MODE
 * @scn: scn pointer
 * @chan_mode: Input wlan_phymode.
 * @is_2gvht_en: VHT modes enabled for 2G.
 *
 * Return: uint32_t WMI_HOST_WLAN_PHY_MODE
 */

uint32_t
ol_get_phymode_info_from_wlan_phymode(struct ol_ath_softc_net80211 *scn,
        uint32_t chan_mode, bool is_2gvht_en);

unsigned int ol_ath_bmi_user_agent_init(ol_ath_soc_softc_t *soc);
int ol_ath_wait_for_bmi_user_agent(ol_ath_soc_softc_t *soc);
void ol_ath_signal_bmi_user_agent_done(ol_ath_soc_softc_t *soc);

void ol_ath_diag_user_agent_init(ol_ath_soc_softc_t *soc);
void ol_ath_diag_user_agent_fini(ol_ath_soc_softc_t *soc);
void ol_ath_host_config_update(ol_ath_soc_softc_t *soc);

void ol_ath_suspend_resume_attach(struct ol_ath_softc_net80211 *scn);
int ol_ath_suspend_target(ol_ath_soc_softc_t *soc, int disable_target_intr);
int ol_ath_resume_target(ol_ath_soc_softc_t *soc);
void ol_ath_set_ht_vht_ies(struct ieee80211_node *ni);

int ol_print_scan_config(wlan_if_t vaphandle, struct seq_file *m);

/**
 * ol_power_set_ap_ps_param() - Set AP power save parameters
 * @vap: pointer to vap
 * @anode: pointer to ath node
 * @param: ap ps parameter
 * @value: ap ps parameter value
 *
 * Return: 0 on success, other value on failure
 */
int ol_power_set_ap_ps_param(struct ieee80211vap *vap,
                             struct ol_ath_node_net80211 *anode,
                             uint32_t param, uint32_t value);

/**
 * ol_power_set_sta_ps_param() - Set STA power save parameters
 * @vap: pointer to vap
 * @param: sta ps parameter
 * @value: sta ps parameter value
 *
 * Return: 0 on success, other value on failure
 */
int ol_power_set_sta_ps_param(struct ieee80211vap *vap,
                              uint32_t param, uint32_t value);

/**
 * ol_ath_wmi_send_vdev_param() - Sends vdev parameters to firmware via wmi
 * @vdev: pointer to vdev object
 * @param_id: pdev param id
 * @param_value: parameter value
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_wmi_send_vdev_param(struct wlan_objmgr_vdev *vdev,
                               wmi_conv_vdev_param_id param_id,
                               uint32_t param_value);

/**
 * ol_ath_vdev_set_intra_bss() - Sends infra bss param to firmware via wmi
 * @vdev: pointer to vdev object
 * @enable: enable/disable
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_vdev_set_intra_bss(struct wlan_objmgr_psoc *psoc,
                              uint16_t vdev_id,
                              bool enable);
#ifdef QCA_SUPPORT_MBSS_CMN_PARAM
/**
 * ol_ath_wmi_send_multi_vdev_param - Sends vdev parameters to firmware
 * for multiple VAP via wmi
 * @vdev: pointer to vdev object
 * @param_id: pdev param id
 * @param_value: parameter value
 * @vdev_bmap: vdev bitmap
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ol_ath_wmi_send_multi_vdev_param(struct wlan_objmgr_vdev *vdev,
                                            wmi_conv_vdev_param_id param_id,
                                            uint32_t param_value,
                                            unsigned long vdev_bmap);
#endif

/**
 * ol_ath_wmi_send_sifs_trigger() - Sends sifs trigger param value to fw via wmi
 * @vdev: pointer to vdev object
 * @param_value: parameter value
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_wmi_send_sifs_trigger(struct wlan_objmgr_vdev *vdev,
                                 uint32_t param_value);
int ol_ath_pdev_set_param(struct wlan_objmgr_pdev *pdev,
                          wmi_conv_pdev_params_id param_id,
                          uint32_t param_value);

/**
 * ol_ath_pdev_set_burst() - Send burst enable command to FW
 * @scn: Pointer to struct ol_ath_softc_net80211
 * @value: Value to be set
 *
 * If target is capable of burst mode, then enable the burst mode in
 * host and send WMI_PDEV_PARAM_BURST_ENABLE command to target.
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_set_burst(struct ol_ath_softc_net80211 *scn, bool value);

/**
 * ol_ath_send_peer_assoc() - Send assoc to peer
 * @ni: pointer to node
 * @isnew: peer association type
 *
 * Return: 0 on success, other value on failure
 */
int ol_ath_send_peer_assoc(struct ieee80211_node *ni, int isnew);

/**
 * ol_ath_set_fw_hang() - API to Inject FW assert through WMI
 * @psoc: Pointer to psoc objmgr
 * @delay_time_ms: Wait time before FW assert to be introduced
 * @assert_type: Type of panic ASSERT,FATAL etc
 *
 * Return: 0 on success, other value on failure
 */
int ol_ath_set_fw_hang(struct wlan_objmgr_psoc *psoc, u_int32_t delay_time_ms,
                       uint32_t assert_type);

/**
 * ol_ath_get_fw_recov_mode() - API to get FW recovery mode values
 * @scn: Pointer to SoC Structure
 * @value: Value to get the FW recovery value
 *
 * Return: FW recovery mode values
 */
uint32_t ol_ath_get_fw_recov_mode(ol_ath_soc_softc_t *soc, int value);

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
/**
 * ol_ath_send_fw_recovery() - API to send fw recovery command to FW
 * @psoc: Pointer to psoc objmgr
 * @recovery_type: Type of recovery mode0 or mode1 etc
 *
 * Return: 0 on success, other value on failure
 */
int ol_ath_send_fw_recovery(struct wlan_objmgr_psoc *psoc, uint32_t recovery_type);

/**
 * ol_ath_ml_link_update() - API to update ml links
 * @soc: Pointer to the primary soc object of type struct ol_ath_soc_softc.
 *
 * Return: none.
 */
void ol_ath_ml_link_update(ol_ath_soc_softc_t *soc);
#endif

int peer_sta_kickout(struct ol_ath_softc_net80211 *scn, A_UINT8 *peer_macaddr);
/**
 * ol_ath_set_beacon_filter() - set beacon filter
 * @vap: pointer to vap
 * @ie: pointer to information element
 *
 * Return: 0 on success, other values on failure
 */
int ol_ath_set_beacon_filter(wlan_if_t vap, uint32_t *ie);
int ol_ath_remove_beacon_filter(wlan_if_t vap);
int ol_get_tx_free_desc(struct ol_ath_softc_net80211 *scn);
void ol_get_radio_stats(struct ol_ath_softc_net80211 *scn,
                        struct ol_ath_radiostats *stats);
/**
 * ol_ath_node_set_param() - Sends node parameters to firmware via wmi layer
 * @pdev: Pointer to pdev object
 * @peer_addr: address of peer
 * @param_id: pdev param id
 * @param_val: parameter value
 * @vdev_id:  vdev id
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_node_set_param(struct wlan_objmgr_pdev *pdev, uint8_t *peer_addr,
                          uint32_t param_id, uint32_t param_val,
                          uint32_t vdev_id);
#if UNIFIED_SMARTANTENNA
int ol_ath_smart_ant_rxfeedback(ol_txrx_pdev_handle pdev, ol_txrx_peer_handle peer, struct sa_rx_feedback *rx_feedback);
int ol_smart_ant_enabled(struct ol_ath_softc_net80211 *scn);
#endif /* UNIFIED_SMARTANTENNA */
#if QCA_LTEU_SUPPORT
void ol_ath_nl_attach(struct ieee80211com *ic);
void ol_ath_nl_detach(struct ieee80211com *ic);
#endif

#define DEFAULT_PERIOD  100         /* msec */
#define DEFAULT_WLAN_DURATION   80  /* msec */
int
ol_ath_btcoex_duty_cycle(ol_ath_soc_softc_t *soc,u_int32_t period, u_int32_t duration);
int
ol_ath_btcoex_wlan_priority(ol_ath_soc_softc_t *soc, u_int32_t val);
int
ol_ath_send_egid_info_cmd(ol_ath_soc_softc_t *soc, uint32_t *egid_info);
int ol_ath_packet_power_info_get(struct wlan_objmgr_pdev *pdev,
                                 struct packet_power_info_params *param);
int ol_ath_halphy_cal_status_get(struct wlan_objmgr_pdev *pdev);
int
ieee80211_extended_ioctl_chan_switch (struct net_device *dev,
                     struct ieee80211com *ic, caddr_t param);
int
ieee80211_extended_ioctl_chan_scan (struct net_device *dev,
                struct ieee80211com *ic, caddr_t param);

int
ieee80211_extended_ioctl_rep_move (struct net_device *dev,
                struct ieee80211com *ic, caddr_t param);

#if ATH_SUPPORT_WRAP && DBDC_REPEATER_SUPPORT
void ol_ioctl_disassoc_clients(struct ol_ath_softc_net80211 *scn);
int32_t ol_ioctl_get_primary_radio(struct ol_ath_softc_net80211 *scn, caddr_t param);
int32_t ol_ioctl_get_mpsta_mac_addr(struct ol_ath_softc_net80211 *scn, caddr_t param);
int32_t ol_ioctl_get_force_client_mcast(struct ol_ath_softc_net80211 *scn, caddr_t param);
int32_t ol_ioctl_get_max_priority_radio(struct ol_ath_softc_net80211 *scn, caddr_t param);
#endif
#if DBDC_REPEATER_SUPPORT
u_int16_t ol_ioctl_get_disconnection_timeout(struct ol_ath_softc_net80211 *scn, caddr_t param);
void ol_ioctl_iface_mgr_status(struct ol_ath_softc_net80211 *scn, caddr_t param);
#endif
u_int8_t ol_ioctl_get_stavap_connection(struct ol_ath_softc_net80211 *scn, caddr_t param);
int32_t ol_ioctl_get_preferred_uplink(struct ol_ath_softc_net80211 *scn, caddr_t param);
int32_t ol_ioctl_get_chan_vendorsurvey_info(struct ol_ath_softc_net80211 *scn,
        caddr_t param);
#if defined(WLAN_DISP_CHAN_INFO)
QDF_STATUS ol_ioctl_get_chan_info(struct ieee80211com *ic,
                                  struct ieee80211req_chaninfo_full  *req_chan);
#endif /* WLAN_DISP_CHAN_INFO */
int ol_ath_punctured_band_setting_check(struct ieee80211com *ic, uint32_t value);
#if OL_ATH_SUPPORT_LED
extern void ol_ath_led_event(struct ol_ath_softc_net80211 *scn, OL_LED_EVENT event);
#endif
int
ol_get_board_id(ol_ath_soc_softc_t *soc, char *boarddata_file );

int ol_ath_set_tx_capture (struct ol_ath_softc_net80211 *scn, int val);
int ol_ath_set_debug_sniffer(struct ol_ath_softc_net80211 *scn, int val);

int ol_ath_set_debug_sniffer_rx_mon_2_0(struct ol_ath_softc_net80211 *scn, int val);
int ol_ath_set_debug_sniffer_tx_mon_2_0(struct ol_ath_softc_net80211 *scn, int val);
int ol_ath_disable_debug_sniffer_rx_mon_2_0 (struct ol_ath_softc_net80211 *scn);
int ol_ath_disable_debug_sniffer_tx_mon_2_0 (struct ol_ath_softc_net80211 *scn);

#ifdef QCA_UNDECODED_METADATA_SUPPORT
int ol_ath_set_undecoded_metadata_capture(struct ol_ath_softc_net80211 *scn, int val);
#endif

int ol_ath_set_capture_latency(struct ol_ath_softc_net80211 *scn, int val);

void ol_ath_get_min_and_max_power(struct ieee80211com *ic,
                                  int8_t *max_tx_power,
                                  int8_t *min_tx_power);
int ol_ath_vap_get_tsf(struct wlan_objmgr_pdev *pdev, uint8_t vdev_id);
bool ol_ath_is_regulatory_offloaded(struct ieee80211com *ic);
uint64_t ol_ath_get_modeSelect(struct ieee80211com *ic);
uint32_t ol_ath_get_chip_mode(struct ieee80211com *ic);
osdev_t get_osdev_from_psoc(struct wlan_objmgr_psoc *psoc);

/**
 * ol_ath_fill_umac_radio_band_info(): Fills the radio band information
 * based on the channel list supported by regdmn and chip capability.
 * @pdev: Pointer to the pdev object.
 */
uint8_t ol_ath_fill_umac_radio_band_info(struct wlan_objmgr_pdev *pdev);

uint32_t ol_ath_get_interface_id(struct wlan_objmgr_pdev *pdev);

/**
 * ol_ath_fill_tx_rx_obss_chan_util(): Fills Channel Utilisation stats
 * with below sequence in a 32-bit Unsigned interger.
 * @pdev: Pointer to the pdev object.
 *
 * Sequence:
 *    AP Rx Util --- 24-31 bits
 *    AP Tx Util --- 16-23 bits
 *    OBSS Rx Util --- 8-15 bits
 *    Total Util --- 0-7 bits
 *
 * Return: 32-bit unsigned integer packed with above data and mapped bits
 *         Failure 0
 */
uint32_t ol_ath_fill_tx_rx_obss_chan_util(struct wlan_objmgr_pdev *pdev);

/**
 * ol_ath_init_and_enable_radar_table() - Initialize and enable the radar table
 * @ic: ieee80211com object
 *
 * Return: none
 */
void ol_ath_init_and_enable_radar_table(struct ieee80211com *ic);

/**
 * ol_ath_num_mcast_tbl_elements() - get number of mcast table elements
 * @ic: ieee80211com object
 *
 * To get number of mcast table elements
 *
 * Return: number of mcast table elements
 */
uint32_t ol_ath_num_mcast_tbl_elements(struct ieee80211com *ic);

/**
 * ol_ath_num_mcast_grps() - get number of mcast groups
 * @ic: ieee80211com object
 *
 * To get number of mcast groups
 *
 * Return: number of mcast grps
 */
uint32_t ol_ath_num_mcast_grps(struct ieee80211com *ic);

/**
 * ol_ath_is_target_ar900b() - check  target type
 * @ic: ieee80211com object
 *
 * To check target type
 *
 * Return: True if the target type is ar900b else False
 */
bool ol_ath_is_target_ar900b(struct ieee80211com *ic);

uint8_t ol_ath_get_tgt_res_cfg(struct ieee80211com *ic, uint8_t *tx_chain_mask, uint8_t *rx_chain_mask);

/**
 * ol_ath_get_tgt_type() - get target type
 * @ic: ieee80211com object
 *
 * To get target type
 *
 * Return: target type
 */
uint32_t ol_ath_get_tgt_type(struct ieee80211com *ic);

#ifdef OL_ATH_SMART_LOGGING
int32_t
ol_ath_enable_smart_log(struct ol_ath_softc_net80211 *scn, uint32_t cfg);

QDF_STATUS
send_fatal_cmd(struct ol_ath_softc_net80211 *scn, uint32_t type,
        uint32_t subtype);

QDF_STATUS
ol_smart_log_connection_fail_start(struct ol_ath_softc_net80211 *scn);

QDF_STATUS
ol_smart_log_connection_fail_stop(struct ol_ath_softc_net80211 *scn);

#ifndef REMOVE_PKT_LOG
QDF_STATUS
ol_smart_log_fw_pktlog_enable(struct ol_ath_softc_net80211 *scn);

QDF_STATUS
ol_smart_log_fw_pktlog_disable(struct ol_ath_softc_net80211 *scn);

QDF_STATUS
ol_smart_log_fw_pktlog_start(struct ol_ath_softc_net80211 *scn,
        u_int32_t fw_pktlog_types);

QDF_STATUS
ol_smart_log_fw_pktlog_stop(struct ol_ath_softc_net80211 *scn);

QDF_STATUS
ol_smart_log_fw_pktlog_stop_and_block(struct ol_ath_softc_net80211 *scn,
        int32_t host_pktlog_types, bool block_only_if_started);

void
ol_smart_log_fw_pktlog_unblock(struct ol_ath_softc_net80211 *scn);
#endif /* REMOVE_PKT_LOG */
#endif /* OL_ATH_SMART_LOGGING */

#ifdef BIG_ENDIAN_HOST
     /* This API is used in copying in elements to WMI message,
        since WMI message uses multilpes of 4 bytes, This API
        converts length into multiples of 4 bytes, and performs copy
     */
#define OL_IF_MSG_COPY_CHAR_ARRAY(destp, srcp, len)  do { \
      int j; \
      u_int32_t *src, *dest; \
      src = (u_int32_t *)srcp; \
      dest = (u_int32_t *)destp; \
      for(j=0; j < roundup(len, sizeof(u_int32_t))/4; j++) { \
          *(dest+j) = qdf_le32_to_cpu(*(src+j)); \
      } \
   } while(0)

/* This macro will not work for anything other than a multiple of 4 bytes */
#define OL_IF_SWAPBO(x, len)  do { \
      int numWords; \
      int i; \
      void *pv = &(x); \
      u_int32_t *wordPtr; \
      numWords = (len)/sizeof(u_int32_t); \
      wordPtr = (u_int32_t *)pv; \
      for (i = 0; i < numWords; i++) { \
          *(wordPtr + i) = __cpu_to_le32(*(wordPtr + i)); \
      } \
   } while(0)

#else

#define OL_IF_MSG_COPY_CHAR_ARRAY(destp, srcp, len)  do { \
    OS_MEMCPY(destp, srcp, len); \
   } while(0)

#endif
#define AR9887_DEVICE_ID    (0x0050)
#define AR9888_DEVICE_ID    (0x003c)

/*
    *  * options for firmware dump generation
    *      - 0x1 - Dump to file
    *      - 0x2 - Dump to crash scope
    *      - 0x4 - Do not crash the host after dump
    *      - 0x8 - host/target recovery without dump
    *
*/
#define FW_DUMP_TO_FILE                 0x1u
#define FW_DUMP_TO_CRASH_SCOPE          0x2u
#define FW_DUMP_NO_HOST_CRASH           0x4u
#define FW_DUMP_RECOVER_WITHOUT_CORE    0x8u
#define FW_DUMP_ADD_SIGNATURE           0x10u

#define VHT_MCS_SET_FOR_NSS(x, ss) ( ((x) & (3 << ((ss)<<1))) >> ((ss)<<1) )
#define VHT_MAXRATE_IDX_SHIFT   4

/*Monitor filter types*/
typedef enum _monitor_filter_type {
    MON_FILTER_ALL_DISABLE          = 0x0,   //disable all filters
    MON_FILTER_ALL_EN               = 0x01,  //enable all filters
    MON_FILTER_TYPE_OSIF_MAC        = 0x02,  //enable osif MAC addr based filter
    MON_FILTER_TYPE_UCAST_DATA      = 0x04,  //enable htt unicast data filter
    MON_FILTER_TYPE_MCAST_DATA      = 0x08,  //enable htt multicast cast data filter
    MON_FILTER_TYPE_NON_DATA        = 0x10,  //enable htt non-data filter
    MON_FILTER_TYPE_LAST            = 0x1F,  //last
} monitor_filter_type;

#define FILTER_MODE(val) (((val) >> 16) & 0xFF)
#define FILTER_PASS_ONLY 1
#define MONITOR_OTHER_ONLY 2
#define INVALID_FILTER 3
#define MON_FILTER_TYPE_GET(val) (0xFFFF & (val)) //get first 16 bits of val
#define MON_FILTER_SUBTYPE_MGMT_GET(val) (((val) >> 32) & 0xFFFF) //get bytes 5-6 of val
#define MON_FILTER_SUBTYPE_CTRL_GET(val) (((val) >> 48) & 0xFFFF) //get bytes 7-8 of val

#define RX_MON_FILTER_PASS          0x0001
#define RX_MON_FILTER_OTHER         0x0002
#define RX_MON_FILTER_PASS_OTHER    0x0004

#define FILTER_MGMT_EN              0xFFFF
#define FILTER_CTRL_EN              0xFFFF

#define FILTER_DATA_UCAST_EN        0x8000
#define FILTER_DATA_MCAST_EN        0x4000

#define FILTER_TYPE_ALL_EN          0x0001
#define FILTER_TYPE_UCAST_DATA      0x0004
#define FILTER_TYPE_MCAST_DATA      0x0008
#define FILTER_TYPE_NON_DATA        0x0010

#define FILTER_TYPE_UCAST_DATA_EN   0x0005
#define FILTER_TYPE_MCAST_DATA_EN   0x0009
#define FILTER_TYPE_NON_DATA_EN     0x0011

#define FILTER_TYPE_BOTH            0xFFFF
#define FILTER_TYPE_FP              0x00FF
#define FILTER_TYPE_MO              0xFF00

#define SET_MON_RX_FILTER_MASK      0x00FF

#define SET_MON_FILTER_MODE(val)    \
        ((val & SET_MON_RX_FILTER_MASK) ?\
        RX_MON_FILTER_PASS | RX_MON_FILTER_OTHER: 0)

#define SET_MON_FILTER_MGMT(val)    \
        ((val & FILTER_TYPE_NON_DATA_EN) ? MON_FILTER_SUBTYPE_MGMT_GET(val) : FILTER_MGMT_EN)

#define SET_MON_FILTER_CTRL(val)    \
        ((val & FILTER_TYPE_NON_DATA_EN) ? MON_FILTER_SUBTYPE_CTRL_GET(val) : FILTER_CTRL_EN)

#define SET_MON_FILTER_DATA(val)    \
        (((val & FILTER_TYPE_UCAST_DATA_EN) ? 0 : FILTER_DATA_UCAST_EN) |\
        ((val & FILTER_TYPE_MCAST_DATA_EN) ? 0 : FILTER_DATA_MCAST_EN))

#if FW_CODE_SIGN
/* ideally these are supposed to go into a different file, in interest of time
 * where this involves LOST approvals etc, for all new files, adding it in the
 * current file, and would require reorg og the code, post the check-in. This
 * entire block would need to go into a file, fw_sign.h
 */
/* known product magic numbers */
#define FW_IMG_MAGIC_BEELINER       0x424c4e52U                      /* BLNR */
#define FW_IMG_MAGIC_CASCADE        0x43534345U                      /* CSCE */
#define FW_IMG_MAGIC_SWIFT          0x53574654U                      /* SWFT */
#define FW_IMG_MAGIC_PEREGRINE      0x5052474eU                      /* PRGN */
#define FW_IMG_MAGIC_DAKOTA         0x44414b54U                      /* DAKT */
#define FW_IMG_MAGIC_UNKNOWN        0x00000000U                      /* zero*/
/* chip idenfication numbers
 * Most of the times chip identifier is pcie device id. In few cases that could
 * be a non-pci device id if the device is not pci device. It is assumed at
 * at build time, it is known to the tools for which the firmware is getting
 * built.
 */
#define RSA_PSS1_SHA256 1

#ifndef NELEMENTS
#define NELEMENTS(__array) sizeof((__array))/sizeof ((__array)[0])
#endif

typedef struct _fw_device_id{
    u_int32_t      dev_id;                  /* this pcieid or internal device id */
    char       *dev_name;                    /* short form of the device name */
    u_int32_t      img_magic;                    /* image magic for this product */
} fw_device_id;

struct cert {
    const unsigned int cert_len;
    const unsigned char *cert;
};
enum {
    FW_SIGN_ERR_INTERNAL,
    FW_SIGN_ERR_INV_VER_STRING,
    FW_SIGN_ERR_INV_DEV_ID,
    FW_SIGN_ERR_INPUT,
    FW_SIGN_ERR_INPUT_FILE,
    FW_SIGN_ERR_FILE_FORMAT,
    FW_SIGN_ERR_FILE_ACCESS,
    FW_SIGN_ERR_CREATE_OUTPUT_FILE,
    FW_SIGN_ERR_UNSUPP_CHIPSET,
    FW_SIGN_ERR_FILE_WRITE,
    FW_SIGN_ERR_INVALID_FILE_MAGIC,
    FW_SIGN_ERR_INFILE_READ,
    FW_SIGN_ERR_IMAGE_VER,
    FW_SIGN_ERR_SIGN_ALGO,
    FW_SIGN_ERR_MAX
};

typedef uint fw_img_magic_t;
/* current version of the firmware signing , major 1, minor 0*/
#define THIS_FW_IMAGE_VERSION ((1<<15) | 0)

/* fw_img_file_magic
 * In current form of firmware download process, there are three different
 * kinds of files, that get downloaded. All of these can be in different
 * formats.
 *
 * 1. downloadable, executable, target resident (athwlan.bin)
 * 2. downloadable, executable, target non-resident otp (otp.bin)
 * 3. downloadable, non-executable, target file (fakeBoard)
 * 4. no-download, no-execute, host-resident code swap file.
 * 5. Add another for future
 * Each of these can be signed or unsigned, each of these can signed
 * with single key or can be signed with multiple keys, provisioning
 * all kinds in this list.
 * This list contains only filenames that are generic. Each board might
 * name their boards differently, but they continue to use same types.
 */
#define FW_IMG_FILE_MAGIC_TARGET_WLAN       0x5457414eu  /* target WLAN - TWLAN*/
#define FW_IMG_FILE_MAGIC_TARGET_OTP        0x544f5450u    /* target OTP TOTP */
#define FW_IMG_FILE_MAGIC_TARGET_BOARD_DATA 0x54424446u         /* target TBDF*/
#define FW_IMG_FILE_MAGIC_TARGET_CODE_SWAP  0x4857414eu         /* host HWLAN */
#define FW_IMG_FILE_MAGIC_INVALID           0

typedef uint fw_img_file_magic_t;

/* fw_img_sign_ver
 *
 * This supports for multiple revisions of this module to support different
 * features in future. This is defined in three numbers, major and minor
 * major - In general this would not change, unless, there is new header types
 *         are added or major crypto algorithms changed
 * minor - Minor changes like, adding new devices, new files etc.
 * There is no sub-release version for this, probably change another minor
 * version if really needs a change
 * All these are listed  in design document, probably in .c files
 */

/* fw_ver_rel_type
 * FW version release could be either test, of production, zero is not allowed
 */
#define FW_IMG_VER_REL_TEST         0x01
#define FW_IMG_VER_REL_PROD         0x02
#define FW_IMG_VER_REL_UNDEFIND     0x00

/*
 * fw_ver_maj, fw_ver_minor
 * major and minor versions are planned below.
 * u_int32_t  fw_ver_maj;                        * 32bits, MMMM.MMMM.SCEE.1111 *
 * u_int32_t  fw_ver_minor;                      * 32bits, mmmm.mmmm.rrrr.rrrr *
 */
/*
 * extrat major version number from fw_ver_maj field
 * Higher 16 bits of 32 bit quantity
 * FW_VER_GET_MAJOR - Extract Major version
 * FW_VER_SET_MAJOR - Clear the major version bits and set the bits
 */
#define FW_VER_MAJOR_MASK 0xffff0000u
#define FW_VER_MAJOR_SHIFT 16
#define FW_VER_GET_MAJOR(__mj)  (((__mj)->fw_ver_maj & FW_VER_MAJOR_MASK) >> FW_VER_MAJOR_SHIFT)
#define FW_VER_SET_MAJOR(__mj, __val)  (__mj)->fw_ver_maj =\
                        ((((__val) & 0x0000ffff) << FW_VER_MAJOR_SHIFT) |\
                        ((__mj)->fw_ver_maj  & ~FW_VER_MAJOR_MASK))

/*
 * Extract build variants. The following variants are defined at this moement.
 * This leaves out scope for future types.
 * This is just a number, so this can contain upto 255 values, 0 is undefined
 */
#define FW_VER_IMG_TYPE_S_RETAIL        0x1U
#define FW_VER_IMG_TYPE_E_ENTERPRISE    0x2U
#define FW_VER_IMG_TYPE_C_CARRIER       0x3U
#define FW_VER_IMG_TYPE_X_UNDEF         0x0U

#define FW_VER_IMG_TYPE_MASK            0x0000ff00
#define FW_VER_IMG_TYPE_SHIFT           8
#define FW_VER_GET_IMG_TYPE(__t) (((__t)->fw_ver_maj & FW_VER_IMG_TYPE_MASK) >>\
                                            FW_VER_IMG_TYPE_SHIFT)
#define FW_VER_SET_IMG_TYPE(__t, __val) \
        (__t)->fw_ver_maj  = \
            ((__t)->fw_ver_maj &~FW_VER_IMG_TYPE_MASK) | \
                ((((u_int32_t)(__val)) & 0xff) << FW_VER_IMG_TYPE_SHIFT)

#define FW_VER_IMG_TYPE_VER_MASK            0x000000ff
#define FW_VER_IMG_TYPE_VER_SHIFT           0

#define FW_VER_GET_IMG_TYPE_VER(__t) (((__t)->fw_ver_maj & \
                     FW_VER_IMG_TYPE_VER_MASK) >> FW_VER_IMG_TYPE_VER_SHIFT)

#define FW_VER_SET_IMG_TYPE_VER(__t, __val) (__t)->fw_ver_maj = \
                         ((__t)->fw_ver_maj&~FW_VER_IMG_TYPE_VER_MASK) |\
                         ((((u_int32_t)(__val)) & 0xff) << FW_VER_IMG_TYPE_VER_SHIFT)

#define FW_VER_IMG_MINOR_VER_MASK           0xffff0000
#define FW_VER_IMG_MINOR_VER_SHIFT          16
#define FW_VER_IMG_MINOR_SUBVER_MASK        0x0000ffff
#define FW_VER_IMG_MINOR_SUBVER_SHIFT       0

#define FW_VER_IMG_MINOR_RELNBR_MASK        0x0000ffff
#define FW_VER_IMG_MINOR_RELNBR_SHIFT       0

#define FW_VER_IMG_GET_MINOR_VER(__m) (((__m)->fw_ver_minor &\
                                        FW_VER_IMG_MINOR_VER_MASK) >>\
                                            FW_VER_IMG_MINOR_VER_SHIFT)

#define FW_VER_IMG_SET_MINOR_VER(__t, __val) (__t)->fw_ver_minor = \
                     ((__t)->fw_ver_minor &~FW_VER_IMG_MINOR_VER_MASK) |\
                     ((((u_int32_t)(__val)) & 0xffff) << FW_VER_IMG_MINOR_VER_SHIFT)

#define FW_VER_IMG_GET_MINOR_SUBVER(__m) (((__m)->fw_ver_minor & \
                     FW_VER_IMG_MINOR_SUBVER_MASK) >> FW_VER_IMG_MINOR_SUBVER_SHIFT)

#define FW_VER_IMG_SET_MINOR_SUBVER(__t, __val) (__t)->fw_ver_minor = \
                     ((__t)->fw_ver_minor&~FW_VER_IMG_MINOR_SUBVER_MASK) |\
                     ((((u_int32_t)(__val)) & 0xffff) << FW_VER_IMG_MINOR_SUBVER_SHIFT)

#define FW_VER_IMG_GET_MINOR_RELNBR(__m) (((__m)->fw_ver_bld_id &\
                     FW_VER_IMG_MINOR_RELNBR_MASK) >> FW_VER_IMG_MINOR_RELNBR_SHIFT)

#define FW_VER_IMG_SET_MINOR_RELNBR(__t, __val) (__t)->fw_ver_bld_id = \
                     ((__t)->fw_ver_bld_id &~FW_VER_IMG_MINOR_RELNBR_MASK) |\
                     ((((u_int32_t)(__val)) & 0xffff) << FW_VER_IMG_MINOR_RELNBR_SHIFT)

/* signed/unsigned - bit 0 of fw_hdr_flags */
#define FW_IMG_FLAGS_SIGNED                 0x00000001U
#define FW_IMG_FLAGS_UNSIGNED               0x00000000U
#define FW_IMG_IS_SIGNED(__phdr) ((__phdr)->fw_hdr_flags & FW_IMG_FLAGS_SIGNED)

/* file format type - bits 1,2,3*/
#define FW_IMG_FLAGS_FILE_FORMAT_MASK       0x0EU
#define FW_IMG_FLAGS_FILE_FORMAT_SHIFT      0x1U
#define FW_IMG_FLAGS_FILE_FORMAT_TEXT       0x1U
#define FW_IMG_FLAGS_FILE_FORMAT_BIN        0x2U
#define FW_IMG_FLAGS_FILE_FORMAT_UNKNOWN    0x0U
#define FW_IMG_FLAGS_FILE_FORMAT_GET(__flags) \
                    (((__flags) & FW_IMG_FLAGS_FILE_FORMAT_MASK) >> \
                    FW_IMG_FLAGS_FILE_FORMAT_SHIFT)

#define FW_IMG_FLAGS_FILE_COMPRES_MASK       0x10U
#define FW_IMG_FLAGS_FILE_COMPRES_SHIFT      0x4U
#define FW_IMG_FLAGS_COMPRESSED             0x1U
#define FW_IMG_FLAGS_UNCOMPRESSED           0x0U
#define FW_IMG_IS_COMPRESSED(__flags) ((__flags)&FW_IMG_FLAGS_FILE_COMPRES_MASK)
#define FW_IMG_FLAGS_SET_COMRESSED(__flags) \
                            ((__flags) |= 1 << FW_IMG_FLAGS_FILE_COMPRES_SHIFT)

#define FW_IMG_FLAGS_FILE_ENCRYPT_MASK       0x20U
#define FW_IMG_FLAGS_FILE_ENCRYPT_SHIFT      0x5U
#define FW_IMG_FLAGS_ENCRYPTED               0x1U
#define FW_IMG_FLAGS_UNENCRYPTED             0x0U
#define FW_IMG_IS_ENCRYPTED(__flags) ((__flags)&FW_IMG_FLAGS_FILE_ENCRYPT_MASK)
#define FW_IMG_FLAGS_SET_ENCRYPT(__flags) \
                            ((__flags) |= 1 << FW_IMG_FLAGS_FILE_ENCRYPT_SHIFT)

/* any file that is dowloaded is marked target resident
 * any file that is not downloaded but loaded at host
 * would be marked NOT TARGET RESIDENT file, or host resident file
 */
#define FW_IMG_FLAGS_FILE_TARGRES_MASK       0x40U
#define FW_IMG_FLAGS_FILE_TARGRES_SHIFT      0x6U
#define FW_IMG_FLAGS_TARGRES                 0x1U
#define FW_IMG_FLAGS_HOSTRES                 0x0U

#define FW_IMG_IS_TARGRES(__flags) ((__flags)&FW_IMG_FLAGS_FILE_TARGRES_MASK)

#define FW_IMG_FLAGS_SET_TARGRES(__flags) \
                   ((__flags) |= (1 <<FW_IMG_FLAGS_FILE_TARGRES_SHIFT))

#define FW_IMG_FLAGS_FILE_EXEC_MASK       0x80U
#define FW_IMG_FLAGS_FILE_EXEC_SHIFT      0x7U
#define FW_IMG_FLAGS_EXEC                 0x1U
#define FW_IMG_FLAGS_NONEXEC                 0x0U
#define FW_IMG_IS_EXEC(__flags) ((__flags)&FW_IMG_FLAGS_FILE_EXEC_MASK)
#define FW_IMG_FLAGS_SET_EXEC (__flags) \
                            ((__flags) |= 1 << FW_IMG_FLAGS_FILE_EXEC_SHIFT)

/* signing algorithms, only rsa-pss1 with sha256 is supported*/
enum {
    FW_IMG_SIGN_ALGO_UNSUPPORTED = 0,
    FW_IMG_SIGN_ALGO_RSAPSS1_SHA256  = 1
};
/* header of the firmware file, also contains the pointers to the file itself
 * and the signature at end of the file
 */
struct firmware_head {
    fw_img_magic_t      fw_img_magic_number;       /* product magic, eg, BLNR */
    u_int32_t              fw_chip_identification;/*firmware chip identification */

    /* boarddata, otp, swap, athwlan.bin etc */
    fw_img_file_magic_t fw_img_file_magic;
    u_int16_t              fw_img_sign_ver;        /* signing method version */

    u_int16_t              fw_img_spare1;                      /* undefined. */
    u_int32_t              fw_img_spare2;                       /* undefined */

    /* Versioning and release types */
    u_int32_t              fw_ver_rel_type;              /* production, test */
    u_int32_t              fw_ver_maj;        /* 32bits, MMMM.MMMM.SSEE.1111 */
    u_int32_t              fw_ver_minor;      /* 32bits, mmmm.mmmm.ssss.ssss */
    u_int32_t              fw_ver_bld_id;                 /* actual build id */
    /* image versioning is little tricky to handle. We assume there are three
     * different versions that we can encode.
     * MAJOR - 16 bits, lower 16 bits of this is spare, chip version encoded
               in lower 16 bits
     * minor - 16 bits, 0-65535,
     * sub-release - 16 bits, usually this would be zero. if required we
     * can use this. For eg. BL.2_1.400.2, is, Beeliner, 2.0, first major
     * release, build 400, and sub revision of 2 in the same build
     */
    u_int32_t             fw_ver_spare3;

     /* header identificaiton */
    u_int32_t              fw_hdr_length;                   /* header length */
    u_int32_t              fw_hdr_flags;                /* extra image flags */
    /* image flags are different single bit flags of different characterstics
    // At this point of time these flags include below, would extend as
    // required
    //                      signed/unsigned,
    //                      bin/text,
    //                      compressed/uncompressed,
    //                      unencrypted,
    //                      target_resident/host_resident,
    //                      executable/non-execuatable
    */
    u_int32_t              fw_hdr_spare4;                      /* future use */

    u_int32_t              fw_img_size;                       /* image size; */
    u_int32_t              fw_img_length;            /* image length in byes */
    /* there is no real requirement for keeping the size, the size is
     * fw_img_size = fw_img_length - ( header_size + signature)
     * in otherwords fw_img_size is actual image size that would be
     * downloaded to target board.
     */
    u_int32_t              fw_spare5;
    u_int32_t              fw_spare6;

    /* security details follows here after */
    u_int16_t              fw_sig_len;     /* index into known signature lengths */
    u_int8_t               fw_sig_algo;                   /* signature algorithm */
    u_int8_t               fw_oem_id;            /* oem ID, to access otp or etc */

#if 0
    /* actual image body    */
    u_int8_t              *fw_img_body;                     /* fw_img_size bytes */
    u_int8_t              *fw_img_padding;          /*if_any_upto_4byte_boundary */
    u_int8_t              *fw_signature;                  /* pointer to checksum */
#endif
};
#endif /* FW_CODE_SIGN */

enum pdev_oper {
       PDEV_ITER_POWERUP,
       PDEV_ITER_TARGET_ASSERT,
       PDEV_ITER_PCIE_ASSERT,
       PDEV_ITER_PDEV_ENTRY_ADD,
       PDEV_ITER_PDEV_ENTRY_DEL,
       PDEV_ITER_RECOVERY_AHB_REMOVE,
       PDEV_ITER_RECOVERY_REMOVE,
       PDEV_ITER_RECOVERY_WAIT,
       PDEV_ITER_RECOVERY_STOP,
       PDEV_ITER_RECOVERY_PROBE,
       PDEV_ITER_LED_GPIO_STATUS,
       PDEV_ITER_PDEV_NETDEV_STOP,
       PDEV_ITER_PDEV_NETDEV_OPEN,
       PDEV_ITER_TARGET_FWDUMP,
       PDEV_ITER_SEND_SUSPEND,
       PDEV_ITER_SEND_RESUME,
       PDEV_ITER_PDEV_DEINIT_BEFORE_SUSPEND,
       PDEV_ITER_PDEV_DETACH_OP,
       PDEV_ITER_PDEV_DEINIT_OP,
       PDEV_ITER_PDEV_RESET_PARAMS,
       PDEV_ITER_FATAL_SHUTDOWN,
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
       PDEV_ITER_ML_REINIT_DONE,
#endif
};

struct pdev_op_args {
    enum pdev_oper type;
    void *pointer;
    int8_t ret_val;
};

enum status_code {
    PDEV_ITER_STATUS_INIT,
    PDEV_ITER_STATUS_OK,
    PDEV_ITER_STATUS_FAIL,
};

/**
 * enum ol_ath_umac_reset_cmds - UMAC reset debug commands
 * @OL_ATH_UMAC_RESET_STATS: UMAC reset stats
 * @OL_ATH_UMAC_RESET_SKEL: Enable/disable UMAC reset skeleton code
 */
enum ol_ath_umac_reset_cmds {
    OL_ATH_UMAC_RESET_STATS = 1,
    OL_ATH_UMAC_RESET_SKEL,
};

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
void wlan_recovery_peer_release_handler(struct wlan_objmgr_psoc *psoc,
                                        void *obj, void *args);

void wlan_recovery_vdev_delete_peers(struct wlan_objmgr_psoc *psoc,
                                     void *obj, void *args);

void wlan_recovery_vdev_flush_mlo_peers(struct wlan_objmgr_psoc *psoc,
                                        void *obj, void *args);

void wlan_mlo_read_tsf2_offset(struct wlan_objmgr_psoc *psoc,
			       uint8_t pdev_id,
			       uint8_t lmac_id);

void wlan_mlo_read_tqm_offset(struct wlan_objmgr_psoc *psoc);
#endif

#ifdef WLAN_FEATURE_11BE_MLO
/**
 * wlan_mlo_wait_for_all_mld_down - Wait for all MLDs down
 *
 * @wait_time_hz: Timeout value in system time unit.
 * @max_wait_itr: Max wait iteration.
 *
 * This API waits for all link in all MLDs to be
 * in INIT state with timeout value equals to
 * (wait_time_hz * max_wait_itr).
 *
 * Return: QDF_STATUS_SUCCESS if all links of all MLDs
 *         are in INIT state. QDF_STATUS_E_FAILURE otherwise.
 */
QDF_STATUS wlan_mlo_wait_for_all_mld_down(unsigned long wait_time_hz,
                                          uint32_t max_wait_itr);
#endif

void wlan_pdev_operation(struct wlan_objmgr_psoc *psoc,
                             void *obj, void *args);
struct wlan_lmac_if_tx_ops;

extern QDF_STATUS wlan_global_lmac_if_set_txops_registration_cb(WLAN_DEV_TYPE dev_type,
                        QDF_STATUS (*handler)(struct wlan_lmac_if_tx_ops *));
extern QDF_STATUS wlan_lmac_if_set_umac_txops_registration_cb
                        (QDF_STATUS (*handler)(struct wlan_lmac_if_tx_ops *));
extern QDF_STATUS wlan_lmac_if_set_umac_crypto_rxpn_ops_registration_cb
                        (QDF_STATUS (*handler)(struct wlan_lmac_if_rx_ops *));

QDF_STATUS olif_register_umac_tx_ops(struct wlan_lmac_if_tx_ops *tx_ops);
QDF_STATUS olif_register_umac_crypto_rxpn_ops(struct wlan_lmac_if_rx_ops *rx_ops);

bool ol_ath_is_beacon_offload_enabled(ol_ath_soc_softc_t *soc);

QDF_STATUS
ol_ath_mgmt_beacon_send(struct wlan_objmgr_vdev *vdev,
                        wbuf_t wbuf);
QDF_STATUS
ol_if_mgmt_send (struct wlan_objmgr_vdev *vdev,
                 qdf_nbuf_t nbuf, u_int32_t desc_id,
                 void *mgmt_tx_params);

/**
 * ol_ath_mgmt_rx_frame_handler() - Handles rx mgmt frames
 * @pdev: pdev for which this management frame was intended
 * @wbuf: buffer
 * @mgmt_rx_params: rx event params
 *
 * This function handles mgmt rx frames and will be registered as the legacy
 * rx frame handler in mgmt txrx component.
 *
 * Return: QDF_STATUS of operation
 */
QDF_STATUS ol_ath_mgmt_rx_frame_handler(
			struct wlan_objmgr_pdev *pdev,
			qdf_nbuf_t wbuf,
			struct mgmt_rx_event_params *mgmt_rx_params);

QDF_STATUS register_legacy_wmi_service_ready_callback(void);

int ol_ath_vdev_getpn(struct ieee80211vap *vap, struct ol_ath_softc_net80211 *scn, u_int8_t if_id,
                      u_int8_t *macaddr, u_int8_t keyix,
                      uint32_t keytype);

QDF_STATUS ol_ath_vdev_get_rxpn(struct ol_ath_softc_net80211 *scn,
				u_int8_t vdev_id, u_int8_t *macaddr, uint16_t keyix);

/**
 * ol_ath_vdev_install_key_send() - install keys
 * @vap: pointer to vap
 * @key: pointer to wlan crypto key
 * @macaddr: mac address
 * @def_keyid: key id
 * @force_none: force cipher flag
 * @keytype: key type
 *
 * Return: 0 on success, other values on failure
 */
int ol_ath_vdev_install_key_send(struct ieee80211vap *vap,
                                 struct wlan_crypto_key *key, uint8_t *macaddr,
                                 uint8_t def_keyid, bool force_none,
                                 uint32_t keytype);

#if QCA_11AX_STUB_SUPPORT
/**
 * @brief Determine whether 802.11ax stubbing is enabled or not
 *
 * @param soc - ol_ath_soc_softc_t structure for the soc
 * @return Integer status value.
 *      -1 : Failure
 *       0 : Disabled
 *       1 : Enabled
 */
extern int ol_ath_is_11ax_stub_enabled(ol_ath_soc_softc_t *soc);

#define OL_ATH_IS_11AX_STUB_ENABLED(_soc) ol_ath_is_11ax_stub_enabled((_soc))

#else
#define OL_ATH_IS_11AX_STUB_ENABLED(_soc) (0)
#endif /* QCA_11AX_STUB_SUPPORT */

/*
 * ol_ath_get_num_clients() - Get ic num clients
 * @pdev: Pointer to pdev object
 */
uint16_t ol_ath_get_num_clients(struct wlan_objmgr_pdev *pdev);

QDF_STATUS target_if_register_tx_ops(struct wlan_lmac_if_tx_ops *tx_ops);
int ol_ath_offload_bcn_tx_status_event_handler(ol_scn_t sc, uint8_t *data, uint32_t datalen);
int ol_ath_pdev_csa_status_event_handler(struct wlan_objmgr_psoc *psoc,
        struct pdev_csa_switch_count_status csa_status);

bool ol_target_lithium(struct wlan_objmgr_psoc *psoc);
bool ol_target_be(struct wlan_objmgr_psoc *psoc);
void rx_dp_peer_invalid(void *scn_handle, enum WDI_EVENT event, void *data, uint16_t peer_id);
#if QCA_STANDALONE_SOUNDING_TRIGGER
int ol_ath_txbf_standalone_sounding_complete_event_handler(ol_soc_t sc,
                                    u_int8_t *data, u_int32_t datalen);
#endif

QDF_STATUS
ol_if_dfs_enable(struct wlan_objmgr_pdev *pdev, int *is_fastclk,
                 struct wlan_dfs_phyerr_param *param,
                 uint32_t dfsdomain);

QDF_STATUS
ol_if_get_tsf64(struct wlan_objmgr_pdev *pdev, uint64_t *tsf64);

QDF_STATUS
ol_dfs_get_caps(struct wlan_objmgr_pdev *pdev,
                struct wlan_dfs_caps *dfs_caps);

QDF_STATUS ol_if_dfs_get_thresholds(struct wlan_objmgr_pdev *pdev,
                                    struct wlan_dfs_phyerr_param *param);

QDF_STATUS
ol_if_dfs_disable(struct wlan_objmgr_pdev *pdev, int no_cac);

QDF_STATUS
ol_if_dfs_get_ext_busy(struct wlan_objmgr_pdev *pdev, int *ext_chan_busy);

QDF_STATUS
ol_ath_get_target_type(struct wlan_objmgr_pdev *pdev, uint32_t *target_type);

QDF_STATUS
ol_is_mode_offload(struct wlan_objmgr_pdev *pdev, bool *is_offload);

QDF_STATUS ol_ath_get_ah_devid(struct wlan_objmgr_pdev *pdev, uint16_t *devid);

QDF_STATUS ol_ath_get_phymode_info(struct wlan_objmgr_pdev *pdev,
                                   uint32_t chan_mode,
                                   uint32_t *mode_info,
                                   bool is_2gvht_en);

/**
 * ol_ath_get_persistent_nol_info() - Check if config for storing NOL in
 * persistent memory is enabled.
 * @pdev: Pointer to structure wlan_objmgr_pdev.
 *
 * Return: True if the config is enabled, false otherwise.
 */
bool ol_ath_get_persistent_nol_info(struct wlan_objmgr_pdev *pdev);

void ol_ath_pdev_config_update(struct ieee80211com *ic);
u_int32_t ol_if_peer_get_rate(struct wlan_objmgr_peer *peer , u_int8_t type);

void ol_ath_find_logical_del_peer_and_release_ref(struct ieee80211vap *vap,
						  uint8_t *peer_mac_addr);
int ol_ath_rel_ref_for_logical_del_peer(struct ieee80211vap *vap,
        struct ieee80211_node *ni, uint8_t *peer_mac_addr);
#define VALIDATE_TX_CHAINMASK 1
#define VALIDATE_RX_CHAINMASK 2
bool ol_ath_validate_chainmask(struct ol_ath_softc_net80211 *scn,
        uint32_t chainmask, int direction, int phymode);

/**
 * Store bandwidth support capabilities for each chain mask.
 * "direction" indicates whether it is TX or RX chainmask.
 */
void ol_ath_update_chainmask_caps(struct ieee80211com *ic, uint8_t direction);
#if ATH_SUPPORT_NAC_RSSI
int
ol_ath_config_fw_for_nac_rssi(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t pdev_id, uint8_t vdev_id,
                              enum cdp_nac_param_cmd cmd, char *bssid, char *client_macaddr,
                              uint8_t chan_num);
int
ol_ath_config_bssid_in_fw_for_nac_rssi(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t pdev_id, uint8_t vdev_id,
                                       enum cdp_nac_param_cmd cmd, char *bssid, char *client_macaddr);
#endif

void ol_ath_process_ppdu_stats(void *pdev_hdl, enum WDI_EVENT event, void *data,
                               uint16_t peer_id, enum htt_cmn_rx_status status);
void ol_ath_process_tx_metadata(struct ieee80211com *ic, void *data);
void ol_ath_process_rx_metadata(struct ieee80211com *ic, void *data);
void ol_ath_subscribe_ppdu_desc_info(struct ol_ath_softc_net80211 *scn, uint8_t context);
void ol_ath_unsubscribe_ppdu_desc_info(struct ol_ath_softc_net80211 *scn, uint8_t context);
#ifdef ATH_SUPPORT_DFS
#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
QDF_STATUS
ol_if_is_host_dfs_check_support_enabled(struct wlan_objmgr_pdev *pdev,
        bool *enabled);
void
ol_vdev_add_dfs_violated_chan_to_nol(struct ieee80211com *ic,
                                     struct ieee80211_ath_channel *chan);
#endif /* HOST_DFS_SPOOF_TEST */
QDF_STATUS ol_if_hw_mode_switch_state(struct wlan_objmgr_pdev *pdev,
        bool *is_hw_mode_switch_in_progress);
void ol_vdev_pick_random_chan_and_restart(wlan_if_t vap);

/**
 * ol_ath_init_dfs_nol(): Initialize the NOL list from persistent memory
 * and mark the channel as radar in regulatory channel list.
 * @pdev: Pointer to the pdev object.
 */
QDF_STATUS ol_ath_init_dfs_nol(struct wlan_objmgr_pdev *pdev);
#endif
enum ieee80211_opmode ieee80211_new_opmode(struct ieee80211vap *vap, bool vap_active);

int ol_ath_send_ft_roam_start_stop(struct ieee80211vap *vap, uint32_t start);

/* The following definitions are used by SON application currently.
 * The converged SON does not have apis exposed to set / get pdev param
 * currently. Hence these are not put into son specific files as retreiving
 * these definitions would be tough. Another copy of these definitions are
 * made in band_steering_api.h file to make one-to-one mapping easier at
 * application level.
 */
#define LOW_BAND_MIN_FIVEG_FREQ 5180 /* FIVEG_LOW_BAND_MIN_FREQ */
#define LOW_BAND_MAX_FIVEG_FREQ 5320 /* FIVEG_LOW_BAND_MAX_FREQ */
#define MAX_FREQ_IN_TWOG        2484 /* TWOG_MAX_FREQ */
#define LOW_BAND_MAX_SIXG_FREQ  6515 /* SIXG_LOW_BAND_MAX_FREQ */
#define HIGH_BAND_MIN_SIXG_FREQ 6535 /* SIXG_HIGH_BAND_MIN_FREQ */
/**
 * @brief Get whether the Radio is tuned for low, high, full band or 2g.
 */
enum {
    NO_BAND_INFORMATION_AVAILABLE = 0, /* unable to retrieve band info due to some error */
    HIGH_BAND_5G_RADIO            = 1, /* RADIO_IN_5G_HIGH_BAND */
    FULL_BAND_5G_RADIO            = 2, /* RADIO_IN_5G_FULL_BAND */
    LOW_BAND_5G_RADIO             = 3, /* RADIO_IN_5G_LOW_BAND */
    NON_5G_RADIO                  = 4, /* RADIO_IS_NON_5G */
    FULL_BAND_6G_RADIO            = 5, /* RADIO_IS_6G_FULL_BAND */
    BAND_5G_6G_RADIO              = 6, /* RADIO_IS_EITHER_5G_6G */
    LOW_BAND_6G_RADIO             = 7, /* RADIO_IN_6G_LOW_BAND */
    HIGH_BAND_6G_RADIO            = 8, /* RADIO_IN_6G_HIGH_BAND */
};
QDF_STATUS
ol_ath_set_pcp_tid_map(ol_txrx_vdev_handle vdev, uint32_t mapid);
QDF_STATUS
ol_ath_set_tidmap_prty(ol_txrx_vdev_handle vdev, uint32_t prec_val);

/**
 * ol_ath_set_default_pcp_tid_map() -Set pcp to tid mapping
 * @pdev: pdev object
 * @pcp: pcp value
 * @tid: tid value
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_set_default_pcp_tid_map(struct wlan_objmgr_pdev *pdev, uint32_t pcp,
				   uint32_t tid);

/**
 * ol_ath_set_default_tidmap_prty() -Set tid map priority
 * @pdev: pdev object
 * @val: tid map priority value
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_set_default_tidmap_prty(struct wlan_objmgr_pdev *pdev, uint32_t val);

#ifdef QCA_SUPPORT_AGILE_DFS
/**
 * @brief: Reset the ADFS engine by sending it an OCAC abort cmd and clear
 * agile_precac_active state.
 * @ic: Pointer to struct ieee80211com.
 * Return: Sucess/failure.
 */
QDF_STATUS ol_if_dfs_reset_agile_cac(struct ieee80211com *ic);

/**
 * @brief: Update FW aDFS support to DFS.
 * @ic: Pointer to struct ieee80211com.
 * @chainmask: Current chainmask of the ic.
 *
 * Return: void.
 */
void ol_ath_update_fw_adfs_support(struct ieee80211com *ic, uint32_t chainmask);
#else
static inline QDF_STATUS ol_if_dfs_reset_agile_cac(struct ieee80211com *ic)
{
    return QDF_STATUS_SUCCESS;
}

static inline void ol_ath_update_fw_adfs_support(struct ieee80211com *ic,
                                                 uint32_t chainmask)
{
}
#endif

/* @brief: Re-initialize DFS pdev config after dynamic hw mode and RF band switch
 * @ic: Pointer to struct ieee80211com.
 */
int ol_if_dfs_pdev_reinit_post_mode_switch(struct ieee80211com *ic);

/* @brief: Reset DFS timers and clear NOL/CAC lists per pdev before dynamic hw mode and RF band switch
 * @scn: Pointer to struct ol_ath_softc_net80211.
 */
int ol_if_dfs_pdev_deinit_pre_mode_switch(struct ol_ath_softc_net80211 *scn);

/* @brief: Reset per PSOC DFS config before dynamic hw mode and RF band switch
 * @psoc: Pointer to psoc object.
 */
void ol_if_dfs_psoc_deinit_pre_mode_switch(struct wlan_objmgr_psoc *psoc);

/*
 * @brief: Configure cong ctrl max msdus in FW.
 * @scn: Pointer to ol_ath_softc_net80211
 */
int ol_ath_configure_cong_ctrl_max_msdus(struct ol_ath_softc_net80211 *scn);

/*
 * @brief: Check if TBR RSTA support is enabled in FW and send RSID range
 * to FW for the same.
 * @psoc: Pointer to wlan_objmgr_psoc
 * @pdev: Pointer to wlan_objmgr_pdev
 */
int ol_tbr_rsta_config_rsid(struct wlan_objmgr_psoc *psoc,
                            struct wlan_objmgr_pdev *pdev);

/*
 * @brief: Configure VAP capabilities after mode switch.
 * @vap: Pointer to ieee80211vap
 * @ic: Pointer to ieee80211com
 */
void ol_ath_update_vap_caps(struct ieee80211vap *vap, struct ieee80211com *ic);

/**
 * ol_ath_copy_curchan_params() - Copy current channel parameter of the primary
 * radio.
 * @ic: ieee80211com handle.
 * @hw_mode_ctx: HW mode switch context.
 *
 * Return: QDF_STATUS_SUCCESS if the current channel will be part of the new
 * channel list of the primary radio, else QDF_STATUS_E_FAILURE.
 */
QDF_STATUS ol_ath_copy_curchan_params(struct ieee80211com *ic,
        ol_ath_hw_mode_ctx_t *hw_mode_ctx);

/**
 * ol_ath_reinit_channel_params() - Reinitialize current and previous channel
 * structures of the primary radio.
 * @ic: ieee80211com handle.
 * @hw_mode_ctx: HW mode switch context.
 *
 * Return: QDF_STATUS_SUCCESS if the current channel can be configured back
 * for the primary radio, else QDF_STATUS_E_FAILURE.
 */
QDF_STATUS ol_ath_reinit_channel_params(struct ieee80211com *ic,
        ol_ath_hw_mode_ctx_t *hw_mode_ctx);

/**
 * ol_if_deinit_dfs_for_mode_switch_fast() - Deinit DFS object for mode switch
 * (fast variant).
 * @scn: Pointer to the primary scn object of type struct ol_ath_softc_net80211.
 * @hw_mode_ctx: Hardware Mode Context
 *
 * Return: QDF_STATUS_SUCCESS if deinit is success, else non zero error value.
 */
QDF_STATUS ol_if_deinit_dfs_for_mode_switch_fast(
        struct ol_ath_softc_net80211 *scn,
        ol_ath_hw_mode_ctx_t *hw_mode_ctx);

/**
 * ol_if_reinit_dfs_for_mode_switch_fast() - Reinit DFS object after mode switch
 * (fast variant).
 * @scn: Pointer to the primary scn object of type struct ol_ath_softc_net80211.
 * @target_hw_mode: Targetted HW mode.
 *
 * Return: QDF_STATUS_SUCCESS if deinit is success, else non zero error value.
 */
QDF_STATUS ol_if_reinit_dfs_for_mode_switch_fast(
        struct ol_ath_softc_net80211 *scn,
        uint8_t target_hw_mode);

/*
 * @brief: Reinit NOL data to corresponding DFS objects of the PDEV from
 * the temporary PSOC copy.
 * @ic: Pointer to struct ieee80211com representing the pdev.
 *
 * Return: void.
 */
void ol_if_dfs_reinit_nol(
        struct ol_ath_softc_net80211 *scn,
        uint8_t target_hw_mode);

/**
 * ol_ath_assemble_ratecode(): Assemble ratecode from rate
 * @vap: Pointer to vap
 * @cur_chan: Pointer to current channel
 * @rate: Rate value entry from the rate array
 *
 * Return: ratecode
 */
uint32_t ol_ath_assemble_ratecode(struct ieee80211vap *vap,
                                  struct ieee80211_ath_channel *cur_chan,
                                  uint32_t rate);

/**
 * config_cmd_resp_init() - Initialize cfg command logging context
 * @soc: Pointer to the primary soc object of type struct ol_ath_soc_softc.
 *
 * Return: none.
 */
void config_cmd_resp_init(ol_ath_soc_softc_t *soc);

/**
 * config_cmd_resp_log() - Log configuration command to debugfs. Logging will be in
 *						   a circular buffer.
 * @soc: Pointer to the primary soc object of type struct ol_ath_soc_softc.
 * @type: log type defined in enum CFG_TYPE
 * @interface: interface name string, e.g. ath0, wifi1
 * @id: parameter id
 * @val: input value
 *
 * Return: none.
 */
void config_cmd_resp_log(ol_ath_soc_softc_t *soc, uint8_t type, char* interface, int id, int val);

/**
 * config_cmd_resp_deinit() - De-initialize cfg command logging context
 * @soc: Pointer to the primary soc object of type struct ol_ath_soc_softc.
 *
 * Return: none.
 */
void config_cmd_resp_deinit(ol_ath_soc_softc_t *soc);

/**
 * ol_ath_set_opclass_tbl() - Set Opclass Table index value.
 * @ic: Pointer to ieee80211_com structure.
 * @opclass_tbl: opclass table index value.
 *
 * Return: 0 on sucess, non-zero on failure.
 */
int ol_ath_set_opclass_tbl(struct ieee80211com *ic, uint8_t opclass);

/**
 * ol_ath_get_opclass_tbl() - Get Opclass Table index value.
 * @ic: Pointer to ieee80211_com structure.
 * @opclass_tbl: opclass table index value.
 *
 * Return: 0 on sucess, non-zero on failure.
 */
int ol_ath_get_opclass_tbl(struct ieee80211com *ic, uint8_t *opclass);

static inline
bool ol_ath_is_mcopy_enabled(struct ieee80211com *ic)
{
	if (ic->ic_debug_sniffer == SNIFFER_M_COPY_MODE ||
	    ic->ic_debug_sniffer == SNIFFER_EXT_M_COPY_MODE)
		return true;

	return false;
}

extern void osif_nss_br_fdb_update_notifier_unregister(void);
extern void osif_nss_br_fdb_notifier_unregister(void);
extern void osif_nss_wifi_mac_db_obj_init(void);
extern void osif_nss_wifi_mac_db_obj_deinit(void);

/**
 * ol_ath_set_vap_beacon_tx_power() - Set beacon tx power
 * @vdev: vdev object
 * @tx_power: Value of tx power to set
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_set_vap_beacon_tx_power(struct wlan_objmgr_vdev *vdev,
                                   uint8_t tx_power);

/**
 * ol_ath_vdev_disa() - Send disa encrypt/decrypt params to fw
 * @vdev: vdev object
 * @fips_buf: fips cmd buffer
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_vdev_disa(struct wlan_objmgr_vdev *vdev,
                     struct ath_fips_cmd *fips_buf);

/**
 * ol_ath_set_vap_pcp_tid_map() - Set vdev pcp tid mapping
 * @vdev: vdev object
 * @pcp: pcp value
 * @tid: tid value
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_set_vap_pcp_tid_map(struct wlan_objmgr_vdev *vdev, uint32_t pcp,
                               uint32_t tid);

/**
 * ol_ath_set_vap_tidmap_tbl_id() - Set vdev tid mapping
 * @vdev: vdev object
 * @mapid: mapid value
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_set_vap_tidmap_tbl_id(struct wlan_objmgr_vdev *vdev, uint32_t mapid);

/**
 * ol_ath_set_vap_tidmap_prty() - Set vdev tid priority
 * @vdev: vdev object
 * @val: priority value
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_set_vap_tidmap_prty(struct wlan_objmgr_vdev *vdev, uint32_t val);

/**
 * ol_net80211_nss_change() - change node NSS values based on BW
 * @ni: Node information
 *
 * Return: none
 */
void ol_net80211_nss_change(struct ieee80211_node *ni);

/**
 * ol_net80211_ext_nss_change() - change node NSS values based on chwidth
 * @ni: Node information
 *
 * Return: none
 */
void ol_net80211_ext_nss_change(struct ieee80211_node *ni,
                                uint8_t *peer_update_count);

/**
 * ol_net80211_chwidth_change() - change ch width of node
 * @ni: Node information
 *
 * Return: none
 */
void ol_net80211_chwidth_change(struct ieee80211_node *ni);

/**
 * ol_net80211_set_sta_fixed_rate() - set fixed rate for peer
 * @ni: Node information
 *
 * Return: none
 */
void ol_net80211_set_sta_fixed_rate(struct ieee80211_node *ni);

extern void osif_radio_activity_update(struct ol_ath_softc_net80211 *scn);
extern void osif_vap_activity_update(wlan_if_t vap);
extern void osif_vap_tx_activity_update(wlan_if_t vap);

/*
 * wideband_csa_modes:
 * Enumerations for supported wideband (5GHz-7GHz) CSA modes.
 */
enum wideband_csa_modes {
    WIDEBAND_CSA_DISABLED = 0,
    WIDEBAND_CSA_COMPATIBILITY,
    WIDEBAND_CSA_FORCED,
};

/**
 * ol_ath_fw_unit_test() - sends fw unit test cmd via wmi
 * @vdev: vdev object
 * @fw_unit_test_cmd: fw unit test cmd parameters
 *
 * Return: 0 on success, other value on failure
 */
int32_t
ol_ath_fw_unit_test(struct wlan_objmgr_vdev *vdev,
                    struct ieee80211_fw_unit_test_cmd *fw_unit_test_cmd);

/**
 * ol_ath_coex_cfg() - set coex configuration
 * @vdev: vdev object
 * @type: coex param type
 * @arg: coex param argument array
 *
 * Return: 0 on success, other value if failure
 */
int ol_ath_coex_cfg(struct wlan_objmgr_vdev *vdev, uint32_t type, uint32_t *arg);

/**
 * ol_ath_frame_injector_config() - sets the frame injector configuration
 * @vdev: vdev object
 * @frametype: frame type value
 * @enable: value set to enable
 * @inject_period: inject time period
 * @bandwidth: Frame bandwidth
 * @duration: Frame duration field
 * @dstmac: destination mac
 *
 * Return: 0 on success, other value if failure
 */
int ol_ath_frame_injector_config(struct wlan_objmgr_vdev *vdev, uint32_t frametype,
                                 uint32_t enable, uint32_t inject_period,
                                 uint32_t bandwidth,
                                 uint32_t duration, uint8_t *dstmac);

/**
 * ol_ath_print_peer_refs() - prints peer references
 * @vdev: vdev object
 * @assert: assert value
 *
 * Return: None
 */
void ol_ath_print_peer_refs(struct wlan_objmgr_vdev *vdev, bool assert);

/**
 * ol_ath_set_vap_dscp_tid_map() - sets dcsp tid map for vap
 * @vap: vap for which map is to be set
 *
 * Return: 0 on success, other value if failure
 */
int ol_ath_set_vap_dscp_tid_map(struct ieee80211vap *vap);

int print_peer_refs_ratelimit(void);

/*
 * enum cfg80211_precac_channel_status:
 * Provides status codes for a precac status.
 */
enum cfg80211_precac_channel_status {
    PRECAC_STATUS_CLEAR,
    PRECAC_STATUS_PENDING,
    PRECAC_STATUS_MAX,
};

/*
 * struct precac_channel_status_list:
 * Provides preCAC channel status list to send to ICM.
 */
struct precac_channel_status_list {
    uint32_t chan_freq;
    uint8_t  precac_status;
};

/**
 * @brief Handle 1st 4-addr frame reception information from backhaul
 *
 * @param ctrl_psoc - opaque handle to psoc
 * @param peer_id - peer id of backhaul peer
 * @param vdev_id - ID of virtual device
 * @param peer_macaddr - MAC address of backhaul peer
 *
 * Return: pointer to device name
 */
#ifdef QCA_SUPPORT_WDS_EXTENDED
void ol_ath_wds_ext_peer_learn(struct cdp_ctrl_objmgr_psoc *ctrl_psoc,
                               uint16_t peer_id, uint8_t vdev_id,
                               uint8_t *peer_macaddr);
#endif /* QCA_SUPPORT_WDS_EXTENDED */

QDF_STATUS
ol_ath_peer_update_mesh_latency_params(struct cdp_ctrl_objmgr_psoc *soc,
		    uint8_t vdev_id, uint8_t *peer_mac, uint8_t tid,
			uint32_t service_interval_dl, uint32_t burst_size_dl,
			uint32_t service_interval_ul, uint32_t burst_size_ul,
			uint8_t add_or_sub, uint8_t ac);

#ifdef CONFIG_SAWF
QDF_STATUS
ol_ath_peer_update_sawf_ul_params(struct cdp_ctrl_objmgr_psoc *soc,
                                  uint8_t vdev_id, uint8_t *peer_mac,
                                  uint8_t tid, uint8_t ac,
                                  uint32_t service_interval,
                                  uint32_t burst_size, uint32_t min_tput,
                                  uint32_t max_latency, uint8_t add_or_sub);
#endif

#ifndef WIFI_MONITOR_SUPPORT
static inline
int monitor_ol_ath_set_tx_sniffer_mode(struct ol_ath_softc_net80211 *scn,
				       uint8_t pdev_id,
				       ol_txrx_soc_handle soc_txrx_handle)
{
	return EOK;
}
static inline
int monitor_ol_ath_set_lite_mode(struct ol_ath_softc_net80211 *scn,
				 uint8_t pdev_id,
				 ol_txrx_soc_handle soc_txrx_handle,
				 int val)
{
	return EOK;
}
static inline void monitor_ol_ath_set_bpr_wifi3(struct ol_ath_softc_net80211 *scn,
						int val)
{
}
#ifdef QCA_UNDECODED_METADATA_SUPPORT
static inline int
monitor_ol_ath_set_undeocded_metadata_capture(struct ol_ath_softc_net80211 *scn,
				 uint8_t pdev_id,
				 ol_txrx_soc_handle soc_txrx_handle,
				 int val)
{
	return EOK;
}
#endif /* QCA_UNDECODED_METADATA_SUPPORT */
#endif

void ol_ath_mgmt_handler(struct wlan_objmgr_pdev *pdev,
                         struct ol_ath_softc_net80211 *scn,
                         wbuf_t wbuf, struct ieee80211_frame * wh,
                         struct mgmt_rx_event_params *rx_event,
                         bool null_data_handler);
#ifdef IPA_OFFLOAD
QDF_STATUS
osif_ol_vap_hardstart_wifi3_ipa_offload(struct sk_buff *skb, struct net_device *dev);
#endif

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
bool ol_ath_mlo_capable(struct wlan_objmgr_psoc *psoc);
uint8_t ol_ath_mlo_get_group_id(struct wlan_objmgr_psoc *psoc);
uint8_t ol_ath_mlo_get_chip_id(struct wlan_objmgr_psoc *psoc);
bool ol_ath_check_mlo_grp_recovery(struct wlan_objmgr_psoc *psoc, bool check_all_soc);
#endif
QDF_STATUS wlan_scan_register_callbacks(struct wlan_objmgr_psoc *psoc);
QDF_STATUS wlan_scan_unregister_callbacks(struct wlan_objmgr_psoc *psoc);
QDF_STATUS ol_ath_notify_asserted_soc(struct ol_ath_soc_softc *soc);

/**
 * ol_ath_ema_init_ctx() - API to init the ema context for that particular psoc
 * @psoc: Pointer to wlan_objmgr_psoc
 *
 * Return: void
 */
void ol_ath_ema_init_ctx(struct wlan_objmgr_pdev *pdev);

#ifdef WLAN_FEATURE_11BE
/**
 * ol_ath_send_mu_param() - API to send WMI command for managed MU
 * @scn: Pointer to SCN structure
 * @mu_ctx: MU context
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ol_ath_send_mu_param(void * scn,
                                struct wlan_mu_context *mu_ctx);

/**
 * ol_ath_send_mlo_peer_tid_to_link_map() - Wrapper function to send TID-to-link
 * mapping to FW.
 * @ml_peer: Pointer to MLO peer context
 * @include_t2lm_params: Flag to determine whether T2LM parameters should be
 *                       added or not
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ol_ath_send_mlo_peer_tid_to_link_map(
        struct wlan_mlo_peer_context *ml_peer, bool include_t2lm_params);

/**
 * ol_ath_send_mlo_peer_established_mapping() - When the peer-to-peer T2LM is
 * torn down, send the established mapping of advertised T2LM to the firmware
 * if it is present.
 * @vdev: Pointer to vdev object
 * @ml_peer: Pointer to ml_peer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ol_ath_send_mlo_peer_established_mapping(
        struct wlan_objmgr_vdev *vdev, struct wlan_mlo_peer_context *ml_peer);

/**
 * ol_ath_get_t2lm_enabled() - API to get the T2LM enabled value
 * @ic: Pointer to the current radio
 * @t2lm_enable: Pointer to save the T2LM enable value
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ol_ath_get_t2lm_enabled(struct ieee80211com *ic,
                                   uint8_t *t2lm_enable);

#endif /* WLAN_FEATURE_11BE */

#if (defined(WLAN_SUPPORT_SCS) && defined(WLAN_FEATURE_11BE)) || defined(CONFIG_SAWF)
/**
 * ol_ath_send_ul_scs_qos_params() - API to send UL SCS QoS attributes to FW
 * @ic: Pointer to the current radio
 * @mac_addr: Mac address of the node
 * @qos_attr: Pointer to QoS attributes to be passed to FW
 * @add_or_sub: Flag to indicate add or subtract
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ol_ath_send_ul_scs_qos_params(struct ieee80211com *ic,
                                         uint8_t *mac_addr,
                                         struct ieee80211_qos_attr *qos_attr,
                                         uint8_t add_or_sub);
#endif /* (WLAN_FEATURE_11BE && WLAN_SUPPORT_SCS) || CONFIG_SAWF */

#ifdef WLAN_FEATURE_11BE_MLO
/**
 * ol_ath_get_eml_phy_cap() - API to get the EML CAP from target
 * @ic: Pointer to the current radio
 * @phy_eml_cap: Pointer to save the EML CAP
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ol_ath_get_eml_phy_cap(struct ieee80211com *ic,
                                   struct wlan_mlo_eml_cap *phy_eml_cap);

/**
 * ol_ath_get_mld_phy_cap() - API to get the MLD CAP from target
 * @ic: Pointer to the current radio
 * @phy_mld_cap: Pointer to save the MLD CAP
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ol_ath_get_mld_phy_cap(struct ieee80211com *ic,
                                  struct wlan_mlo_mld_cap *phy_mld_cap);

/**
 * ol_ath_get_msd_phy_cap() - API to get the MSD CAP from target
 * @ic: Pointer to the current radio
 * @phy_msd_cap: Pointer to save the MSD CAP
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ol_ath_get_msd_phy_cap(struct ieee80211com *ic,
                                  struct wlan_mlo_msd_cap *phy_msd_cap);
#endif

#ifdef WLAN_SUPPORT_SCS
/**
 * ol_ath_peer_scs_rule_match() - Check if SCS rule is matching for peer
 * @soc: Opaque handle to psoc
 * @vdev_id: ID of the virtual device
 * @rule_id: SCS rule_id
 * @peer_mac: Peer mac address
 *
 * Return: true if rule is matching else return false
 */
bool ol_ath_peer_scs_rule_match(struct cdp_ctrl_objmgr_psoc *soc,
                                uint8_t vdev_id, uint32_t rule_id,
                                uint8_t *peer_mac);
#endif

#ifdef IPA_OFFLOAD
#ifdef IPA_OFFLOAD_LOW_MEM
/**
 * ol_ath_get_num_buff() - Get num of buff according to pdev band
 * @psoc - opaque handle to psoc
 * @pdev_id - pdev id
 * @buff_type - buff type TX/RX request
 *
 * Return: number of buffers
 */
uint16_t ol_ath_get_num_buff(struct cdp_ctrl_objmgr_psoc *psoc,
			     uint8_t pdev_id,
                             enum qdf_buff_type_tx_rx buff_type);
#endif
#if defined(IPA_WDS_EASYMESH_FEATURE) && defined(FEATURE_AST)
/**
 * @brief Handle peer send wds disconnect event
 *
 * @param psoc - opaque handle to psoc
 * @param mac_addr - pointer to wds node Mac address
 * @param vdev_id - id of vdev
 *
 * Return: Integer value indicating status
 */
void ol_peer_send_wds_disconnect(struct cdp_ctrl_objmgr_psoc *psoc,
                                 uint8_t *mac_addr, uint8_t vdev_id);
#endif
#if defined(QCA_IPA_LL_TX_FLOW_CONTROL)
/**
 * ol_ath_get_mlo_chip_id() - Get mlo chip id
 * @psoc: Opaque handle to psoc
 *
 * Return: None
 */
uint8_t ol_ath_get_mlo_chip_id(struct cdp_ctrl_objmgr_psoc *psoc);
#endif
#endif

typedef enum {
        PRIMARY_RF_BAND,
        SECONDARY_RF_BAND
} ol_rf_path_t;

/*
 * ol_ath_notify_eapol_tx_compl_status() - Notify Eapol completion to hostapd
 *                                         API
 * @psoc: Opaque handle to soc
 * @nbuf: nbuf pointer
 * @vdev_id: id of vdev
 * @tx_status: EAPOL Tx status from firmware
 *
 * Return: void
 */
void ol_ath_notify_eapol_tx_compl_status(struct cdp_ctrl_objmgr_psoc *psoc, qdf_nbuf_t nbuf,
                                         uint8_t vdev_id, uint8_t tx_status);

/*
 * ol_ath_send_standby_entry_complete() - Enter netstandby
 *                                        completion callback API
 * @scn: Pointer to SCN structure
 *
 * Return: void
 */
void ol_ath_send_standby_entry_complete(struct ol_ath_softc_net80211 *scn);

/*
 * ol_ath_send_standby_entry_complete() - Exit netstandby
 *                                        completion callback API
 * @scn: Pointer to SCN structure
 *
 * Return: void
 */
void ol_ath_send_standby_exit_complete(struct ol_ath_softc_net80211 *scn);

/*
 * ol_ath_send_wakeup_trigger() - Wakeup trigger to exit netstandby
 *                                API
 * @psoc: Opaque handle to psoc
 *
 * Return: void
 */
void ol_ath_send_wakeup_trigger(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t vdev_id);

/*
 * ol_ath_set_standby_mode() - Set standby mode for all SoC
 * @scn: Pointer to SCN structure
 *
 * Return: void
 */
void ol_ath_set_standby_mode(struct ol_ath_softc_net80211 *scn, uint32_t value);

/*
 * ol_ath_wifi_notify_standby_entry_cmp() - Notify standby entry completion
 *
 * Return: 0 if success, else error code
 */
int ol_ath_wifi_notify_standby_entry_cmp(void);

/*
 * ol_ath_wifi_notify_standby_exit_cmp() - Notify standby exit completion
 *
 * Return: 0 if success, else error code
 */
int ol_ath_wifi_notify_standby_exit_cmp(void);

/*
 * ol_ath_wifi_standby_wakeup_trigger() - Trigger wakeup notification to netstandby
 * @dev: Pointer of net device structure on which received trigger
 *
 * Return: 0 if success, else error code
 */
int ol_ath_wifi_standby_wakeup_trigger(struct net_device *dev);
#ifdef WLAN_FEATURE_DYNAMIC_WSI_REMAP

/*
 * ol_ath_dynamic_wsi_remap() - Trigger dynamic wsi remap
 * @soc: Pointer to the primary soc object of type struct ol_ath_soc_softc.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ol_ath_dynamic_wsi_remap(ol_ath_soc_softc_t *soc);

/*
 * ol_ath_dynamic_wsi_remap_mlo_teardown() - Trigger dynamic wsi remap mlo teardown
 * @soc: Pointer to the primary soc object of type struct ol_ath_soc_softc.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ol_ath_dynamic_wsi_remap_mlo_teardown(ol_ath_soc_softc_t *soc);

/*
 * ol_ath_dynamic_wsi_remap_mlo_setup() - Trigger dynamic wsi remap mlo setup
 * @soc: Pointer to the primary soc object of type struct ol_ath_soc_softc.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ol_ath_dynamic_wsi_remap_mlo_setup(ol_ath_soc_softc_t *soc);
#endif

#ifdef WLAN_DBS_POWER_REDUCTION
/*
 * ol_ath_update_active_pdev_count() - Trigger happens when a client joins or
 * leaves any radio to find active Radio count and update Power Reduction values
 * to Firmware.
 * @ic: Pointer to ic object
 *
 * Return: None
 */
void ol_ath_update_active_pdev_count(struct ieee80211com *ic);
#endif

/*
 * ol_ath_send_hwcts2self_command() - Sends hwcts2self param value to fw via WMI
 * @vdev: pointer to vdev object
 * @value: parameter value
 *
 * Return: 0 if success, else error code
 */
int ol_ath_send_hwcts2self_command(struct wlan_objmgr_vdev *vdev, int value);

#endif /* _DEV_OL_ATH_ATHVAR_H  */
