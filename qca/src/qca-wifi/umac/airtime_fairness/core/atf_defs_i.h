/*
 * Copyright (c) 2008 Atheros Communications Inc.
 * All Rights Reserved.
 *
 * Copyright (c) 2011-2016,2017,2022-2023 Qualcomm Innovation Center, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011-2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 */

#ifndef _ATF_DEFS_I_H_
#define _ATF_DEFS_I_H_

#include <wlan_objmgr_cmn.h>
#include <wlan_objmgr_global_obj.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_objmgr_vdev_obj.h>
#include <wlan_objmgr_peer_obj.h>
#include <qdf_list.h>
#include <qdf_timer.h>
#include <qdf_util.h>
#include <wlan_atf_utils_defs.h>

#define atf_log(level, args...) \
QDF_TRACE(QDF_MODULE_ID_ATF, level, ## args)

#define atf_logfl(level, format, args...) atf_log(level, FL(format), ## args)

#define atf_fatal(format, args...) \
	atf_logfl(QDF_TRACE_LEVEL_FATAL, format, ## args)
#define atf_err(format, args...) \
	atf_logfl(QDF_TRACE_LEVEL_ERROR, format, ## args)
#define atf_warn(format, args...) \
	atf_logfl(QDF_TRACE_LEVEL_WARN, format, ## args)
#define atf_info(format, args...) \
	atf_logfl(QDF_TRACE_LEVEL_INFO, format, ## args)
#define atf_debug(format, args...) \
	atf_logfl(QDF_TRACE_LEVEL_DEBUG, format, ## args)

#define CFG_NUM_VDEV                           16
#define PER_UNIT_1000                          1000
#define PER_UNIT_100                           100
#define ATF_TPUT_MAX_STA                       20
#define ATF_MIN_BUFS                           64
#define ATF_MAX_BUFS                           1024
#define ATF_MAX_SUBGROUPS                      20
#define ATF_TPUT_RESV_AIRTIME                  10
/* Num of Access Catergories */
#define WME_NUM_AC                             4
/* build up allocate table (secs)*/
#define ATF_CFG_WAIT                           1
/* ATF Scheduling Policies */
#define ATF_GROUP_SCHED_FAIR                    0    /* Can Borrow & contribute */
#define ATF_GROUP_SCHED_STRICT                  1    /* Cannot borrow & contribute */
#define ATF_GROUP_SCHED_FAIR_UB                 2    /* Can contribute, cannot borrow */
#define ATF_GROUP_NUM_SCHED_POLICIES            3    /* Max number of supported sched policies */
#define ATF_MAX_GROUPS                         (ATF_ACTIVED_MAX_ATFGROUPS + 1)
/* 200 msec */
#define ATF_TOKEN_INTVL_MS                     200
#define ATF_DENOMINATION                       1000
#define ATF_GROUP_NAME_LEN                     32
#define ATF_TRUNCATE_GRP_NAME                  4
/* SSID group name created & maintained in the driver by default */
#define DEFAULT_GROUPNAME                      "QC-DeFaUlT-AtFgRoUp\0"
/* Sub-group name created & maintained in the driver by default */
#define DEFAULT_SUBGROUPNAME                   "QC-DeFaUlT-AtFsUbGrOuP"
/* Subgroup types */
#define ATF_SUBGROUP_TYPE_SSID                 0x0
#define ATF_SUBGROUP_TYPE_PEER                 0x1
#define ATF_SUBGROUP_TYPE_AC                   0x2
#define ATF_SG_ACTIVE                          0x1 /* Indicates subgroup is active */
#define ATF_SET_SG_ACTIVE(_v)                 ((_v)->sg_flag |= ATF_SG_ACTIVE)
#define ATF_RESET_SG_ACTIVE(_v)               ((_v)->sg_flag &= ~ATF_SG_ACTIVE)
#define ATF_IS_SG_ACTIVE(_v)                  ((_v)->sg_flag & ATF_SG_ACTIVE)
#define ATF_SET_PEER_CFG(_v)                  (_v |= 1)
#define ATF_RESET_PEER_CFG(_v)                (_v &= ~(1))

/* Subgroup config types */
#define ATF_CFGTYPE_SSID                       0x0
#define ATF_CFGTYPE_PEER                       0x1
#define ATF_CFGTYPE_AC                         0x2
#define ATF_CFGTYPE_GROUP                      0x3
/* Minimum Data log (history) required for ATF Fairqueuing algo */
#define ATF_DATA_LOG_SIZE                      3
#define ATF_DATA_LOG_ZERO                      0
/* ATF scheduling policy */
#define ATF_GROUP_SCHED_FAIR                   0
/* Block traffic on all TID's */
#define ATF_TX_BLOCK_AC_ALL                    0xF

/* ATF STATS minimum accumulation timeout in seconds */
#define ATF_STATS_MIN_TIMEOUT                  10
/* ATF STATS default accumulation timeout in seconds */
#define ATF_STATS_DEFAULT_TIMEOUT              30
/* ATF STATS maximum accumulation timeout in seconds */
#define ATF_STATS_MAX_TIMEOUT                  50

#ifdef WLAN_ATF_INCREASED_STA
/**
 * enum atf_mode - Airtime Mode
 * @ATF_MODE_DISABLE: Disable case
 * @ATF_MODE_ENABLE: Enable case with full feature support
 * @ATF_MODE_50_MAX_CLIENT: Enable with 50 restricted clients
 */
enum atf_mode {
	ATF_MODE_DISABLE,
	ATF_MODE_ENABLE,
	ATF_MODE_50_MAX_CLIENT,
};

/**
 * enum atf_fw_update_flag - Airtime Firmware Update methods
 * @ATF_FW_FULL_UPDATE: Full update and default update method
 * @ATF_FW_PARTIAL_JOIN_UPDATE: Partial update method for peer join case
 * @ATF_FW_PARTIAL_LEAVE_UPDATE: Partial update method for peer leave case
 * @ATF_FW_NO_UPDATE: No update to FW where no peer associated
 */
enum atf_fw_update_flag {
	ATF_FW_FULL_UPDATE,
	ATF_FW_PARTIAL_JOIN_UPDATE,
	ATF_FW_PARTIAL_LEAVE_UPDATE,
	ATF_FW_NO_UPDATE,
};

/* Set No Update */
#define ATF_SET_FW_NO_UPDATE(_pa) (_pa->atf_fw_flag = ATF_FW_NO_UPDATE)
/* Set Full update method */
#define ATF_SET_FW_FULL_UPDATE(_pa) (_pa->atf_fw_flag = ATF_FW_FULL_UPDATE)
/* Check Full update is enabled or not */
#define ATF_IS_FW_FULL_UPDATE(_pa) (_pa->atf_fw_flag == ATF_FW_FULL_UPDATE)
/* Check partial update for peer join */
#define ATF_IS_FW_JOIN_UPDATE(_pa) (_pa->atf_fw_flag == ATF_FW_PARTIAL_JOIN_UPDATE)
/* Check partial update for peer leave */
#define ATF_IS_FW_LEAVE_UPDATE(_pa) (_pa->atf_fw_flag == ATF_FW_PARTIAL_LEAVE_UPDATE)
/* Set Partial Update for Peer Join */
#define ATF_SET_FW_JOIN_UPDATE(_pa)                                         \
do {                                                                        \
	if (!ATF_IS_FW_FULL_UPDATE(_pa))                                    \
		_pa->atf_fw_flag = ATF_FW_PARTIAL_JOIN_UPDATE;              \
} while (0)
/* Set Partial Update for Peer Leave */
#define ATF_SET_FW_LEAVE_UPDATE(_pa)                                        \
do {                                                                        \
	if (!ATF_IS_FW_FULL_UPDATE(_pa) && !ATF_IS_FW_JOIN_UPDATE(_pa))     \
		_pa->atf_fw_flag = ATF_FW_PARTIAL_LEAVE_UPDATE;             \
} while (0)
/* Set Peer dirty flag to consider in FW update */
#define ATF_SET_DIRTY_PEER(_pa) (_pa.dirty_peer = true)
/* Clear Peer dirty flag to avoid in FW update */
#define ATF_CLEAR_DIRTY_PEER(_pa) (_pa.dirty_peer = false)
/* Check if peer dirty flag is set */
#define ATF_IS_DIRTY_PEER(_pa) (_pa.dirty_peer)
/* Copy dirty peer flag during peer entry delete */
#define ATF_COPY_DIRTY_PEER_FLAG(_da, _sa) (_da.dirty_peer = _sa.dirty_peer)
#else
#define ATF_SET_FW_FULL_UPDATE(_pa) {}
#define ATF_SET_FW_JOIN_UPDATE(_pa) {}
#define ATF_SET_FW_LEAVE_UPDATE(_pa) {}
#define ATF_SET_DIRTY_PEER(_pa) {}
#define ATF_CLEAR_DIRTY_PEER(_pa) {}
#define ATF_COPY_DIRTY_PEER_FLAG(_da, _sa) {}
#endif

/**
 * struct atf_subtype - To get private command
 * @id_type: Sub command of IOCTL
 */
struct atf_subtype {
	uint16_t   id_type;
};

/**
 * struct ssid_val - SSID configuration object
 * @id_type:    Sub command
 * @ssid:       SSID
 * @value:      Value to configure
 * @ssid_exist: Flag to check SSID exist
 */
struct ssid_val {
	uint16_t   id_type;
	uint8_t    ssid[WLAN_SSID_MAX_LEN + 1];
	uint32_t   value;
	uint8_t    ssid_exist;
};

/**
 * struct atf_ac - AC configuration object
 * @id_type:     Sub command
 * @ac_id:       Access category number
 * @ac_val:      Value to configure
 * @ac_ssid:     Associated SSID
 */
struct atf_ac {
	uint16_t  id_type;
	int8_t ac_id;
	int32_t ac_val;
	uint8_t ac_ssid[WLAN_SSID_MAX_LEN+1];
};

/**
 * struct sta_val - STA configuration object
 * @id_type: Sub command
 * @sta_mac: STA MAC address
 * @ssid:    SSID to which sta is connected
 * @value:   Value to configure
 */
struct sta_val {
	uint16_t   id_type;
	uint8_t    sta_mac[QDF_MAC_ADDR_SIZE];
	uint8_t    ssid[WLAN_SSID_MAX_LEN + 1];
	uint32_t   value;
};

/**
 * struct atf_group - ATF group
 * @id_type: Sub command
 * @name:    Name of group
 * @ssid:    SSID
 * @value:   value to configure
 */
struct atf_group {
	uint16_t    id_type;
	uint8_t     name[ATF_GROUP_NAME_LEN];
	uint8_t     ssid[WLAN_SSID_MAX_LEN + 1];
	uint32_t    value;
};

/**
 * struct atf_subgroup_list - ATF Subgroup list element
 * @sg_next:                List of ATF subgroup instances
 * @sg_name:                Subgroup name
 * @sg_id:                  Subgroup id
 * @sg_type:                Type of subgroup 0 - SSID, 1 - Peer, 2 - AC
 * @sg_peermac:             Peer MAC for peer config entry
 * @ac_id:                  Access category Id, for AC config entry
 * @sg_del:                 Mark subgroup for deletion
 * @usr_cfg_val:            Airtime value configured by user
 * @sg_atf_units:           Allotted airtime for this subgroup
 * @sg_flag:                Flag to indicate the subgroup status
 * @sg_atf_debug_mask:      Size of the history of statistics
 * @sg_atf_debug_id:        Write index into the history of statistics
 * @sg_contribute_id:       Index used for contribution level
 * @sg_contribute_level:    level of contribution
 *                          node and subgroup
 * @atf_sg_lock:            Lock to protect ATF subgroup
 * @sg_atf_debug:           History of statistics
 */
struct atf_subgroup_list {
	qdf_list_node_t  sg_next;
	uint8_t     sg_name[WLAN_SSID_MAX_LEN + 1];
	uint32_t    sg_id;
	uint8_t     sg_type;
	int8_t      sg_peermac[6];
	int8_t      ac_id;
	int8_t      sg_del;
	uint32_t    usr_cfg_val;
	uint32_t    sg_atf_units;
	uint32_t    sg_flag;
	uint16_t    sg_atf_debug_mask;
	uint16_t    sg_atf_debug_id;
	uint32_t    sg_contribute_id;
	uint32_t    sg_contribute_level;
	qdf_spinlock_t  atf_sg_lock;
	struct  atf_stats  *sg_atf_debug;
};

/**
 * struct atf_ac_config - ATF AC config parameters
 * @ac:             Airtime corresponding to each AC ID
 * @ac_bitmap:      Bitmap to indicate configured ACs
 * @ac_cfg_flag:    Flag to indicate AC configuration
 */
struct atf_ac_config {
	uint32_t    ac[WME_NUM_AC][1]; /* Airtime corresponding to each AC id*/
	uint8_t     ac_bitmap;      /* Bitmap to indicate configured ACs */
	uint8_t     ac_cfg_flag;    /* Flag to indicate AC configuration */
};

/**
 * struct atf_group_list - ATF group list element
 * @group_next:                      List of ATF group instances
 * @group_name:                      Group name
 * @group_del:                       Indicates this group is to be deleted
 * @group_airtime:                   User configured airtime for the group
 * @subgroup_count:                  No. of subgroups in this group
 * @num_subgroup:                    No. of subgroups in this group
 * @tot_airtime_peer:                Total airtime assigned to all peer subgroups within a group
 * @tot_airtime_ac:                  Total airtime assigned to all ac subgroups withing a group
 * @group_sched:                     Group scheduling policy
 * @atf_subgroups:                   Subgroup list
 */
struct atf_group_list {
	qdf_list_node_t  group_next;
	uint8_t          group_name[WLAN_SSID_MAX_LEN + 1];
	uint8_t          group_del;
	uint32_t         group_airtime;
	uint32_t         subgroup_count;
	uint32_t         num_subgroup;
	uint32_t         tot_airtime_peer;
	uint32_t         tot_airtime_ac;
	uint32_t         group_sched;
	qdf_list_t       atf_subgroups;
};

/**
 * struct atf_actual_airtime - Actual Airtime accumulation structure
 * @actual_airtime:      Actual Airtime acumulation if ac_cfg_flag is disabled
 * @actual_ac_airtime:   Actual Airtime per AC if ac_cfg_flag is enabled
 */
struct atf_actual_airtime {
	uint32_t        actual_airtime;
	uint32_t        actual_ac_airtime[WME_NUM_AC];
};

/**
 * struct atf_config - ATF configuration structure
 * @struct peer_id:  Peer details (number varies with increased STA support)
 * @    cfg_flag:         Peer config
 * @    sta_cfg_value:    User config % for sta
 * @    sta_cfg_mark:     Label sta cfg from user input until re-update table
 *                        to decide if remove this mark or remove this entry
 * @    index_vdev:       Peer associated vdev index [1-16],
 *                        not vdev configurated 0xff, empty entry 0
 * @    sta_mac:          Station mac address
 * @    sta_cal_value:    Result of calulate
 *                        (sta_cfg_value[atfcfg_set.peer_id[j].index_vdev]
 *                        *vap_cfg_value)/100
 * @    sta_assoc_status: Sta association status,
 *                        1: associated, 0:No-assoc if sta is configed
 * @    index_group:      Index of group to which peer is linked.
 *                        '0XFF' if peer is not part of any atfgroup
 * @    sta_cal_ac_value: Airtime calculated for ac configured STA
 * @    actual_airtime:   Actual Aritime acculation per STA
 * @    actual_ul_airtime:Actual UL Aritime acculation per STA
 * @    is_sta_vap_node:  Flag to indicate bss sta of a STA vap
 * @    peer_cal_flag:    Flag to indicate sta allocated from global config
 * @    dirty_peer:       Flag to consider the peer in partial update
 * @struct vdev:     Total Vdevs
 * @    vdev_cfg_value:   User config % for ssid
 * @    essid:            SSID name
 * @    cfg_flag:         Vdev Config
 * @    atf_cfg_ac:       AC configuration for ssid
 * @    actual_airtime:   Actual Aritime acculation per vdev
 * @    actual_ul_airtime:Actual UL Aritime acculation per vdev
 * @    is_sta_vap:       Flag to indicate STA vap
 * @struct atfgroup: Group details
 * @    grpname:          group name
 * @    grp_num_ssid:     Number of ssids added in this group
 * @    grp_sched_policy: Group schedule policy
 * @    grp_cfg_value:    Airtime for this group
 * @    grp_vip_cal_val:  Calculated airtime for VIP group
 * @    num_impl_peers:   Number of implicit peers
 * @    num_expl_peers:   Number of explicit peers
 * @    total_expl_units: Total explicit peers' airtime units to be removed
 * @                      from total airtime units to get Total Implicit peer
 * @                      airtime units
 * @    grp_ssid:         List of SSIDs in the group
 * @    atf_cfg_ac:       AC configuration for group
 * @    grplist_entry:    Group list entry
 * @    act_airtime:      Actual Aritime acculation per group
 * @    act_ul_airtime:   Actual UL Aritime acculation per group
 * @grp_num_cfg:     Total number of groups configured
 * @vdev_num_cfg:    User config how many vdev
 * @peer_num_cfg:    User config how many stas
 * @peer_num_cal:    Total stas need to be pass down fm
 * @max_clients:     Denotes max allowed number of client configs
 * @percentage_unit: default 1000, range 100 or 1000
 */
struct atf_config {
	struct {
		uint8_t    cfg_flag;
		uint32_t   sta_cfg_value[ATF_CFG_NUM_VDEV + 1];
		uint8_t    sta_cfg_mark;
		uint8_t    index_vdev;
		uint8_t    sta_mac[QDF_MAC_ADDR_SIZE];
		uint32_t   sta_cal_value;
		uint8_t    sta_assoc_status;
		uint8_t    index_group;
		uint32_t   sta_cal_ac_value[WME_NUM_AC];
		uint32_t   actual_airtime;
		uint32_t   actual_ul_airtime;
		bool       is_sta_vap_node;
		bool       peer_cal_flag;
#ifdef WLAN_ATF_INCREASED_STA
		bool       dirty_peer;
	} peer_id[ATF_ACTIVED_MAX_CLIENTS_V2];
#else /* WLAN_ATF_INCREASED_STA */
	} peer_id[ATF_ACTIVED_MAX_CLIENTS];
#endif /* WLAN_ATF_INCREASED_STA */
	struct {
		uint32_t   vdev_cfg_value;
		uint8_t    essid[WLAN_SSID_MAX_LEN+1];
		uint8_t    cfg_flag;
		struct     atf_ac_config atf_cfg_ac;
		uint32_t   actual_airtime;
		uint32_t   actual_ul_airtime;
		bool       is_sta_vap;
	} vdev[ATF_CFG_NUM_VDEV];
	struct {
		uint8_t    grpname[WLAN_SSID_MAX_LEN + 1];
		uint32_t   grp_num_ssid;
		uint32_t   grp_sched_policy;
		uint32_t   grp_cfg_value;
		uint16_t   grp_vip_cal_val;
#ifdef WLAN_ATF_INCREASED_STA
		uint16_t   num_impl_peers;
		uint16_t   num_expl_peers;
		uint16_t   total_expl_units;
#endif /* WLAN_ATF_INCREASED_STA */
		uint8_t    grp_ssid[ATF_CFG_NUM_VDEV]
					[WLAN_SSID_MAX_LEN + 1];
		struct     atf_ac_config atf_cfg_ac;
		struct     atf_group_list *grplist_entry;
		struct     atf_actual_airtime act_airtime;
		struct     atf_actual_airtime act_ul_airtime;
	} atfgroup[ATF_ACTIVED_MAX_ATFGROUPS];
	uint8_t    grp_num_cfg;
	uint8_t    vdev_num_cfg;
	uint16_t   peer_num_cfg;
	uint16_t   peer_num_cal;
	uint16_t   max_clients;
	uint16_t   percentage_unit;
};

/**
 * struct atf_tput_table - ATF throughput table
 * @mac_addr: MAC address
 * @airtime:  Airtime allocated to the peer
 * @order:    Order of the entry
 * @tput:     Allocated throughput
 */
struct atf_tput_table {
	uint8_t    mac_addr[QDF_MAC_ADDR_SIZE];
	uint8_t    airtime;
	uint8_t    order;
	uint32_t   tput;
};

/**
 * struct peer_atf - peer specific atf object
 * @atf_peer_stats:            Statistics
 * @ac_list_ptr:               Pointer to atf subgroup per AC
 * @atf_group:                 List of atf group
 * @block_tx_traffic:          Block data traffic tx for this node
 * @atf_tput:                  Configured throughput for the node
 * @throughput:                Possible UDP DL BE throughput for this node,
 *                             from ratectrl
 * @atf_token_allocated:       ATF token allocated
 * @atf_token_utilized:        ATF token utilized
 * @block_tx_bitmap:           Bit map indicating the AC blocked
 * @peer_obj:                  Reference to peer global object
 */
struct peer_atf {
	uint32_t                atf_units;
	uint32_t                raw_tx_tokens;
	struct atf_stats        atf_peer_stats;
	struct atf_stats        *atf_debug;
	struct atf_subgroup_list *ac_list_ptr[WME_NUM_AC];
	struct atf_group_list   *atf_group;
	uint8_t                 block_tx_traffic;
	uint32_t                atf_tput;
	uint32_t                throughput;
	uint16_t                atf_token_allocated;
	uint16_t                atf_token_utilized;
	uint8_t                 block_tx_bitmap;
	uint8_t                 ac_blk_cnt;
	struct wlan_objmgr_peer *peer_obj;
};

/**
 * struct vdev_atf - vdev specific atf object
 * @vdev_atf_sched:    Per ssid atf scheduling policy
 * @block_tx_traffic:  Block data traffic tx for this vap
 * @ac_blk_cnt;        Number of ACs under this vap blocked to transmit
 * @vdev_obj:          Reference to vdev global object
 */
struct vdev_atf {
	uint8_t                 vdev_atf_sched:2,
	                        block_tx_traffic:1;
	uint8_t                 ac_blk_cnt;
	struct wlan_objmgr_vdev *vdev_obj;
};

/**
 * struct pdev_atf - radio specific atf object
 * @atf_commit:                      1- atf enable, 0- atf disable
 * @atf_vip_infra:                   1 - atf vip infra enable, 0 - disable
 * @atf_tput_based:                  Throughput based airtime allocation
 * @atf_tput_tbl_num:                Number of entries in throughput table
 * @atf_tput_order_max:              Track the maximum order for tput table
 * @atf_txbuf_share:                 Indicate Tx Buffer share enable/disable
 * @atf_resv_airtime:                Reserved airtime for a Radio
 * @atf_logging:                     ATF logging enable/disable
 * @atf_txbuf_max:                   Maximum number of Tx buffers configured
 * @atf_txbuf_min:                   Minimum number of Tx buffers configured
 * @atf_airtime_override:            Airtime to be overridden
 * @atf_tput_tbl:                    ATF throughput table
 * @atfcfg_set:                      ATF configurations
 * @atf_req:                         ATF request reference
 * @atf_peer_req:                    ATF peer request reference
 * @atf_group_req:                   ATF group request reference
 * @atf_group_ac_req:                ATF WNM AC Group based request
 * @bwf_req:                         Bandwith fairness request reference
 * @atfcfg_timer:                    Timer to update atf table on STA join/leave
 * @atf_tasksched:                   Flag to indicate update task scheduled
 * @atf_lock:                        Lock to synchronise atf table updates and
 *                                   tasksched flag updates
 * @atf_sched:                       Enable/Disable strict and OBSS scheduling
 * @atf_chbusy:                      Channel busy stats collected every atf
 *                                   timer interval
 * @atf_ssidgroup:                   Enable/disable ATF ssid grouping support
 * @atf_groups:                      List of ATF group instances
 * @stop_looping:                    While peer travarsal default itrator
 *                                   function doesn't support partial traversal.
 *                                   This flag used to stop processing further.
 *                                   Internal use only.
 * @atf_stats_enable:                ATF Stats enabled flag
 * @atf_stats_timeout:               ATF Stats accumulation timeout period
 * @atf_stats_timer:                 ATF Stats accumulation timer
 * @actual_total_airtime:            ATF Stats total accumulated airtime per Radio
 * @actual_total_ul_airtime:         ATF Stats total accumulated UL airtime per Radio
 * @atf_vip_sta_cnt:                 ATF VIP station count
 * @atf_fw_flag:                     Indicate Firmware update method
 * @atf_req_v2:                      ATF peer request reference with optimised details
 * @atf_group_req_v2:                ATF group request reference with optimised details
 * @pdev_obj:                        Reference to pdev global object
 */
struct pdev_atf {
	uint8_t atf_commit;
	uint8_t atf_vip_infra;
	uint16_t atf_tput_based:1,
		 atf_tput_tbl_num:7,
		 atf_tput_order_max:7;
	uint8_t atf_txbuf_share;
	uint8_t atf_resv_airtime;
	uint8_t atf_logging;
	uint16_t atf_txbuf_max;
	uint16_t atf_txbuf_min;
	uint16_t atf_airtime_override;
	struct atf_tput_table atf_tput_tbl[ATF_TPUT_MAX_STA];
	struct atf_config atfcfg_set;
	struct pdev_atf_req atf_req;
	struct pdev_atf_peer_ext_request atf_peer_req;
	struct pdev_atf_ssid_group_req atf_group_req;
	struct pdev_atf_group_wmm_ac_req atf_group_ac_req;
	struct pdev_bwf_req bwf_req;
	qdf_timer_t atfcfg_timer;
	uint8_t atf_tasksched;
	qdf_spinlock_t atf_lock;
	uint32_t atf_sched;
	uint32_t atf_chbusy;
	uint8_t atf_ssidgroup;
	qdf_list_t atf_groups;
	uint8_t stop_looping;
	bool atf_stats_enable;
	qdf_atomic_t atf_stats_pause;
	uint16_t atf_stats_timeout;
	qdf_timer_t atf_stats_timer;
	uint32_t actual_total_airtime;
	uint32_t actual_total_ul_airtime;
	uint16_t atf_vip_sta_cnt;
#ifdef WLAN_ATF_INCREASED_STA
	enum atf_fw_update_flag atf_fw_flag;
	struct pdev_atf_req_v2 atf_req_v2;
	struct pdev_atf_ssid_group_req_v2 atf_group_req_v2;
#endif
	struct wlan_objmgr_pdev *pdev_obj;
};

/**
 * struct atf_context : ATF global context
 * Members used in Distribution phases
 * @atf_mode:                        Indicate atf_mode module parameter value
 * @atf_fwcap_512_max_clients:       Indicate Firmware capability of ATF 512
 *                                   maximum client support
 * @atf_512_max_clients:             Indicate ATF 512 maximum client support
 * @atf_fmcap:                       Indicate firmware capability of ATF
 * @atf_msdu_desc:                   Number of descriptors configured
 * @atf_peers:                       Number of peers configured
 * @atf_max_vdevs:                   Number of vdevs configured
 * @psoc_obj:                        Reference to psoc global object
 */
struct atf_context {
	uint8_t atf_mode;
#ifdef WLAN_ATF_INCREASED_STA
	uint8_t atf_fwcap_512_max_clients;
	bool atf_512_max_clients;
#endif
	uint32_t atf_fmcap;
	uint32_t atf_msdu_desc;
	uint32_t atf_peers;
	uint32_t atf_max_vdevs;
	struct wlan_objmgr_psoc *psoc_obj;
};

int32_t atf_set(struct wlan_objmgr_vdev *vdev);
int32_t atf_clear(struct wlan_objmgr_vdev *vdev);
int32_t atf_set_maxclient(struct wlan_objmgr_pdev *pdev, uint8_t enable);
int32_t atf_get_maxclient(struct wlan_objmgr_pdev *pdev);
void atf_open(struct wlan_objmgr_psoc *psoc);
void atf_enable(struct wlan_objmgr_psoc *psoc);
void atf_disable(struct wlan_objmgr_psoc *psoc);
#endif /* _ATF_DEFS_I_H_*/
