/*
 *
 * Copyright (c) 2018-2021,2023-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 */

#ifndef _CFG_OL_H_
#define _CFG_OL_H_

#include "cfg_define.h"
#include "wmi_unified_param.h"
#include "ol_if_athvar.h"
#include "ieee80211.h"

#define MAX_CFG 0xffffffff

#define CFG_OL_ENABLE_11AX_STUB \
	CFG_INI_BOOL("enable_11ax_stub", false, \
	"Enable 802.11ax stubbing support for testing. Valid only for QCA9984")

#define CFG_OL_TX_TCP_CKSUM \
	CFG_INI_BOOL("enable_tx_tcp_cksum", false, \
	"Enable Tx TCP Checksum")

#define CFG_OL_VOW_CONFIG \
	CFG_INI_BOOL("vow_config", false, \
	"VoW Configuration")

#define CFG_OL_CARRIER_VOW_CONFIG \
	CFG_INI_BOOL("carrier_vow_config", false, \
	"Enable Vow stats and Configuration")

#define CFG_OL_FW_VOW_STATS_ENABLE \
	CFG_INI_BOOL("fw_vow_stats_enable", false, \
	"Firmware VoW stats control")

#define CFG_OL_QWRAP_ENABLE \
	CFG_INI_BOOL("qwrap_enable", false, \
	"Enable qwrap target config")

#define CFG_OL_DISABLE_AUTO_TARGET_BRINGUP \
	CFG_INI_BOOL("disable_auto_target_bringup", false, \
	"Automaticaly bringup target when executing iwpriv or cfg80211tool commands in 'wifi down' state")

#define CFG_OL_CCE_DISABLE \
	CFG_INI_BOOL("cce_disable", false, \
	"Disable Hardware CCE Component")

#define CFG_OL_LOW_MEM_SYSTEM \
	CFG_INI_BOOL("low_mem_system", false, \
	"Low Memory System")

#define CFG_OL_BEACON_OFFLOAD_DISABLE \
	CFG_INI_BOOL("beacon_offload_disable", false, \
	"Beacon offload disable")

#define CFG_OL_EAPOL_OVER_NL_ENABLE \
	CFG_INI_BOOL("enable_eapol_over_nl", false, \
	"eapol over nl  enable")

#define CFG_OL_ADV_RAW_MODE_WAR_ENABLE \
	CFG_INI_BOOL("enable_advance_rawmode_war", false, \
	"Advance raw mode war enable")

#define CFG_OL_NAWDS_LEARN_DEFER \
	CFG_INI_BOOL("nawds_learn_defer", false, \
	"nawds learn defer  enable")

#define CFG_OL_ENABLE_MONITOR_PEER_DECAP \
	CFG_INI_BOOL("enable_monitor_peer_decap", false, \
	"monitor peer decap  enable")

#define CFG_OL_ENABLE_UART_PRINT \
	CFG_INI_BOOL("enableuartprint", false, \
	"Enable uart/serial prints from target")

#define CFG_OL_ENABLE_MESH_SUPPORT \
	CFG_INI_BOOL("mesh_support", false, \
	"Configure Mesh support")

#define CFG_OL_EAPOL_MINRATE_SET \
	CFG_INI_BOOL("eapol_minrate_set", false, \
	"Enable/Disable EAPOL Minrate")

#define CFG_COMMAND_LOGGING_SUPPORT \
	CFG_INI_BOOL("config_logging_enable", false, \
	"Enable/Disable Config Logging")

#define FORCE_DISABLE_HE_MCS_12_13_ANNOUNCEMENT \
	CFG_INI_BOOL("force_disable_he_mcs_12_13_announcement", false, \
	"Enable/Disable MCS 12/13 announcement")

#define CFG_OL_EAPOL_MINRATE_AC_SET \
	CFG_INI_UINT("eapol_minrate_ac_set", \
	0, 4, 0, \
	CFG_VALUE_OR_DEFAULT, "Set AC for the EAPOL minrate set")

#define CFG_OL_CFG80211_CONFIG \
	CFG_INI_BOOL("cfg80211_config", false, \
	"cfg80211 config(enable/disable)")

#define CFG_OL_CFG_IPHDR\
	CFG_INI_BOOL("iphdr_pad", true, \
	"Disable IP header padding to manage IP header unalignment")

#define CFG_OL_LTEU_SUPPORT \
	CFG_INI_UINT("lteu_support", \
	0, 0x4, 0, \
	CFG_VALUE_OR_DEFAULT, "LTEU support")

#define CFG_OL_BMI \
	CFG_INI_UINT("bmi", \
	0, 1, 0, \
	CFG_VALUE_OR_DEFAULT, "BMI Handling: 0 - Driver, 1 - User agent")

#define CFG_OL_MAX_DESC \
	CFG_INI_UINT("max_descs", \
	0, 2198, 0, \
	CFG_VALUE_OR_DEFAULT, "Override default max descriptors")

#define CFG_OL_MAX_PEERS \
	CFG_INI_UINT("max_peers", \
	0, 1024, 0, \
	CFG_VALUE_OR_DEFAULT, "Override default max peers")

#define CFG_OL_STRICT_CHANNEL_MODE \
	CFG_INI_UINT("strict_channel_mode", \
	0, 1, 0, \
	CFG_VALUE_OR_DEFAULT, "Do not implicitly change phymode or channel")

#define CFG_OL_ACBK_MIN_FREE \
	CFG_INI_UINT("OL_ACBKMinfree", \
	0, MAX_CFG, 0, \
	CFG_VALUE_OR_DEFAULT, "Min Free buffers reserved for AC-BK")

#define CFG_OL_ACBE_MIN_FREE \
	CFG_INI_UINT("OL_ACBEMinfree", \
	0, MAX_CFG, 0, \
	CFG_VALUE_OR_DEFAULT, "Min Free buffers reserved for AC-BE")

#define CFG_OL_ACVI_MIN_FREE \
	CFG_INI_UINT("OL_ACVIMinfree", \
	0, MAX_CFG, 0, \
	CFG_VALUE_OR_DEFAULT, "Min Free buffers reserved for AC-VI")

#define CFG_OL_ACVO_MIN_FREE \
	CFG_INI_UINT("OL_ACVOMinfree", \
	0, MAX_CFG, 0, \
	CFG_VALUE_OR_DEFAULT, "Min Free buffers reserved for AC-VO")

#define CFG_OL_OTP_MOD_PARAM \
	CFG_INI_UINT("otp_mod_param", \
	0, 0xffffffff, 0xffffffff, \
	CFG_VALUE_OR_DEFAULT, "OTP")

#define CFG_OL_EMU_TYPE \
	CFG_INI_UINT("emu_type", \
	0, 2, 0, \
	CFG_VALUE_OR_DEFAULT, "Emulation Type : 0-->ASIC, 1-->M2M, 2-->BB")

#define CFG_OL_MAX_ACTIVE_PEERS \
	CFG_INI_UINT("max_active_peers", \
	0, 50, 0, \
	CFG_VALUE_OR_DEFAULT, "Override max active peers in peer qcache")

#define CFG_OL_MAX_GROUP_KEYS \
	CFG_INI_UINT("max_group_keys", \
	0, 128, 0, \
	CFG_VALUE_OR_DEFAULT, "Set maximum number of Group keys supported")

#define CFG_OL_HW_MODE_ID_SOC0 \
	CFG_INI_UINT("hw_mode_id", \
	WMI_HOST_HW_MODE_SINGLE, WMI_HOST_HW_MODE_MAX, WMI_HOST_HW_MODE_MAX, \
	CFG_VALUE_OR_DEFAULT, "Preferred HW mode id - SoC0")

#define CFG_OL_HW_MODE_ID_SOC1 \
	CFG_INI_UINT("hw_mode_id_soc1", \
	WMI_HOST_HW_MODE_SINGLE, WMI_HOST_HW_MODE_MAX, WMI_HOST_HW_MODE_MAX, \
	CFG_VALUE_OR_DEFAULT, "Preferred HW mode id - SoC1")

#define CFG_OL_HW_MODE_ID_SOC2 \
	CFG_INI_UINT("hw_mode_id_soc2", \
	WMI_HOST_HW_MODE_SINGLE, WMI_HOST_HW_MODE_MAX, WMI_HOST_HW_MODE_MAX, \
	CFG_VALUE_OR_DEFAULT, "Preferred HW mode id - SoC2")

#define CFG_OL_HW_MODE_ID_SOC3 \
	CFG_INI_UINT("hw_mode_id_soc3", \
	WMI_HOST_HW_MODE_SINGLE, WMI_HOST_HW_MODE_MAX, WMI_HOST_HW_MODE_MAX, \
	CFG_VALUE_OR_DEFAULT, "Preferred HW mode id - SoC3")

#define CFG_OL_DYNAMIC_HW_MODE \
	CFG_INI_UINT("dynamic_hw_mode", \
	WMI_HOST_DYNAMIC_HW_MODE_DISABLED, (WMI_HOST_DYNAMIC_HW_MODE_MAX - 1), \
	WMI_HOST_DYNAMIC_HW_MODE_DISABLED, \
	CFG_VALUE_OR_DEFAULT, "Dynamic HW mode change support")

#define CFG_OL_DYNAMIC_HW_MODE_PRIMARY_IF \
	CFG_INI_STRING("dynamic_hw_mode_primary_if", \
	0, 64, "", \
	"Dynamic HW mode primary interface")

#define CFG_OL_TGT_SCHED_PARAM \
	CFG_INI_UINT("tgt_sched_params", \
	0, MAX_CFG, 0, \
	CFG_VALUE_OR_DEFAULT, "Target Scheduler Parameters")

#define CFG_OL_FW_CODE_SIGN \
	CFG_INI_UINT("fw_code_sign", \
	0, 3, 0, \
	CFG_VALUE_OR_DEFAULT, "FW Code Sign")

#define CFG_OL_ALLOCRAM_TRACK_MAX \
	CFG_INI_UINT("allocram_track_max", \
	0, MAX_CFG, 0, \
	CFG_VALUE_OR_DEFAULT, "Enable target allocram tracking")

#define CFG_OL_MAX_VAPS \
	CFG_INI_UINT("max_vaps", \
	1, 51, 16, \
	CFG_VALUE_OR_DEFAULT, \
	"Max vap nodes for which mempool is statically allocated")

#define CFG_OL_MAX_CLIENTS \
	CFG_INI_UINT("max_clients", \
	1, 1024, 124, \
	CFG_VALUE_OR_DEFAULT, \
	"Max client nodes for which mempoolis statically allocated")

#define CFG_OL_ASE_OVERRIDE \
	CFG_INI_BOOL("ase_override_enabled", false, \
	"Enable ase override")

#define CFG_OL_MODE_2G_PHYB \
	CFG_INI_BOOL("mode_2g_phyb", false, \
	"Special mode 2g phyb enabled")

/* The below 2 AFC INI configs will only take effect
 * when the FW compile time AFC compliance flag is disabled.
 * 1. afc_disable_timer_check
 * 2. afc_disable_request_id_check
 *
 * If the Host AFC feature flag is disabled, use the below
 * default values to update the Host service flags.
 */
#define CFG_6GHZ_SP_POWER_MODE_DEFAULT      0
#define CFG_AFC_TIMER_CHECK_DIS_DEFAULT     0
#define CFG_AFC_REQ_ID_CHECK_DIS_DEFAULT    0

#define CFG_OL_6GHZ_SP_POWER_MODE_SUPP \
	CFG_INI_BOOL("enable_6ghz_sp_pwrmode_supp", \
	CFG_6GHZ_SP_POWER_MODE_DEFAULT, \
	"Enable support for SP Power mode in 6GHz")

#define CFG_OL_AFC_TIMER_CHECK_DIS \
	CFG_INI_BOOL("afc_disable_timer_check", \
	CFG_AFC_TIMER_CHECK_DIS_DEFAULT, \
	"Disable the AFC request timer in FW")

#define CFG_OL_AFC_REQ_ID_CHECK_DIS \
	CFG_INI_BOOL("afc_disable_request_id_check", \
	CFG_AFC_REQ_ID_CHECK_DIS_DEFAULT, \
	"Disable the AFC request ID check in FW")

/*
 * Allowed values for HTC_TIMEOUT are in the range of u32. User should
 * excercise caution when updating the ini file with value
 */
#define CFG_HTC_READY_TIMEOUT_MSEC_MIN 1
#define CFG_HTC_READY_TIMEOUT_MSEC_MAX 0xffffffff
#define CFG_HTC_READY_TIMEOUT_MSEC_DEFAULT 6000

#define CFG_HTC_READY_TIMEOUT_MSEC \
	CFG_INI_UINT("htc_ready_timeout_msec", \
	CFG_HTC_READY_TIMEOUT_MSEC_MIN, \
	CFG_HTC_READY_TIMEOUT_MSEC_MAX, \
	CFG_HTC_READY_TIMEOUT_MSEC_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, "HTC Pipe ready timeout in msec")

/*
 * Allowed values for WMI_TIMEOUT are in the range of u32. User should
 * excercise caution when updating the ini file with value
 */
#define CFG_WMI_READY_TIMEOUT_MSEC_MIN 1
#define CFG_WMI_READY_TIMEOUT_MSEC_MAX 0xffffffff
#define CFG_WMI_READY_TIMEOUT_MSEC_DEFAULT 15000

#define CFG_WMI_READY_TIMEOUT_MSEC \
	CFG_INI_UINT("wmi_ready_timeout_msec", \
	CFG_WMI_READY_TIMEOUT_MSEC_MIN, \
	CFG_WMI_READY_TIMEOUT_MSEC_MAX, \
	CFG_WMI_READY_TIMEOUT_MSEC_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, "WMI ready timeout in msec")

#define CFG_TGT_MAX_WMI_CMDS_MIN 512
#define CFG_TGT_MAX_WMI_CMDS_MAX 4096

#ifdef QCA_LOWMEM_CONFIG
#define CFG_TGT_MAX_WMI_CMDS_DEFAULT 512
#elif defined QCA_512M_CONFIG
#define CFG_TGT_MAX_WMI_CMDS_DEFAULT 1024
#else
#define CFG_TGT_MAX_WMI_CMDS_DEFAULT 2048
#endif

#define CFG_OL_CARRIER_VOW_OPTIMIZATION \
	CFG_INI_BOOL("carrier_vow_optimization", false, \
	"Enable/Disable VoW optimization for carier usecases")

#define CFG_OL_SCAN_RADIO_TX_ON_DFS_CHANNELS \
	CFG_INI_BOOL("scan_radio_tx_on_dfs_channels", false, \
	"Enable/Disable Tx on DFS channels for scan radio")

#define CFG_OL_RETAIN_NOL_ACROSS_DRIVER_RELOAD \
	CFG_INI_BOOL("dfs_retain_nol_across_driver_reload", false, \
	"Enable/Disable retaining NOL across driver reload")

#define CFG_OL_MAX_WMI_CMDS \
	CFG_INI_UINT("max_wmi_cmds", \
	CFG_TGT_MAX_WMI_CMDS_MIN, CFG_TGT_MAX_WMI_CMDS_MAX, CFG_TGT_MAX_WMI_CMDS_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, \
	"Max WMI commands ")

#define CFG_OL_CARRIER_PROFILE_CONFIG \
	CFG_INI_UINT("carrier_profile_config", \
	0, 0xFFFFFFFF, 0, \
	CFG_VALUE_OR_DEFAULT, \
	"config for carrier profile ")

#define CFG_RF_SWITCH_CONFIG \
	CFG_INI_BOOL("rf_switch_config", false, \
	"configure rf switch")

/* Number of Vdevs per radio */
#define CFG_TGT_VDEVS_MIN 1
#ifdef QCA_LOWMEM_CONFIG
#define CFG_TGT_VDEVS_MAX 9
#elif defined(QCA_512M_CONFIG)
#ifdef QCA_IPQ807X_E_BUILD
#define CFG_TGT_VDEVS_MAX 8
#else
#define CFG_TGT_VDEVS_MAX 9
#endif
#else
#define CFG_TGT_VDEVS_MAX 16
#endif
#define CFG_TGT_VDEVS_DEFAULT CFG_TGT_VDEVS_MAX

#define CFG_OL_NUM_VDEVS \
	CFG_INI_UINT("num_vdevs", \
	CFG_TGT_VDEVS_MIN, CFG_TGT_VDEVS_MAX, CFG_TGT_VDEVS_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, \
	"Max vdevs for radio")

#define CFG_OL_MAX_VDEVS_AR9887 17
#define CFG_OL_MAX_VDEVS_AR9888 16

/* Number of Peers per radio */
#define CFG_TGT_PEERS_MIN 1
#if defined(QCA_LOWMEM_CONFIG) || defined(QCA_512M_CONFIG)
#define CFG_TGT_PEERS_MAX 128
#elif defined(IPA_OFFLOAD)
#define CFG_TGT_PEERS_MAX 256
#else
#define CFG_TGT_PEERS_MAX 512
#endif
#define CFG_TGT_PEERS_DEFAULT CFG_TGT_PEERS_MAX

#define CFG_OL_NUM_PEERS \
	CFG_INI_UINT("num_peers", \
	CFG_TGT_PEERS_MIN, CFG_TGT_PEERS_MAX, CFG_TGT_PEERS_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, \
	"Max peers for radio")

/* Number of Monitor vaps per radio */
#define CFG_TGT_MONITOR_MIN 0
#if defined(QCA_LOWMEM_CONFIG) || (defined(QCA_512M_CONFIG) && !defined(QCA_IPQ807X_E_BUILD))
#define CFG_TGT_MONITOR_MAX 0
#else
#define CFG_TGT_MONITOR_MAX 1
#endif
#define CFG_TGT_MONITOR_DEFAULT CFG_TGT_MONITOR_MAX

#define CFG_OL_MONITOR_VAPS \
	CFG_INI_UINT("num_monitor_vaps", \
	CFG_TGT_MONITOR_MIN, CFG_TGT_MONITOR_MAX, CFG_TGT_MONITOR_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, \
	"monitor vap for radio")

#define CFG_TGT_NUM_VDEV_MESH_MIN 1
#define CFG_TGT_NUM_VDEV_MESH_MAX 8
#define CFG_TGT_NUM_VDEV_MESH_DEFAULT CFG_TGT_NUM_VDEV_MESH_MAX

#define CFG_OL_NUM_VDEV_MESH \
	CFG_INI_UINT("num_mesh_vap", \
	CFG_TGT_NUM_VDEV_MESH_MIN, CFG_TGT_NUM_VDEV_MESH_MAX, CFG_TGT_NUM_VDEV_MESH_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, \
	"number of mesh vaps")

/* Qwrap max client vdev counts */
#define CFG_TGT_QWRAP_VDEVS_MAX 30
#define CFG_TGT_QWRAP_VDEVS_DEFAULT 24

#define CFG_OL_QWRAP_VDEVS \
	CFG_INI_UINT("num_qwrap_vdevs", \
	CFG_TGT_VDEVS_MIN, CFG_TGT_QWRAP_VDEVS_MAX, CFG_TGT_QWRAP_VDEVS_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, \
	"Max qwrap vdevs for radio")

#define CFG_OL_NUM_IPA_RX_BUFF_MIN 512
#define CFG_OL_NUM_IPA_RX_BUFF_DEFAULT 4096
#define CFG_OL_NUM_IPA_RX_BUFF_MAX 4096
#define CFG_OL_NUM_IPA_TX_BUFF_MIN 512
#define CFG_OL_NUM_IPA_TX_BUFF_DEFAULT 4096
#define CFG_OL_NUM_IPA_TX_BUFF_MAX 4096
/* Qwrap max client peer counts */
#define CFG_TGT_QWRAP_PEERS_MAX 28
#define CFG_TGT_QWRAP_PEERS_DEFAULT 22

#define CFG_OL_QWRAP_PEERS \
	CFG_INI_UINT("num_qwrap_peers", \
	CFG_TGT_PEERS_MIN, CFG_TGT_QWRAP_PEERS_MAX, CFG_TGT_QWRAP_PEERS_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, \
	"Max qwrap peers for radio")

#define CFG_OL_NUM_IPA_RX_BUFF \
	CFG_INI_UINT("num_rx_ipa_buffers", \
	CFG_OL_NUM_IPA_RX_BUFF_MIN, CFG_OL_NUM_IPA_RX_BUFF_MAX, \
        CFG_OL_NUM_IPA_RX_BUFF_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, \
	"Max num buffers")

#define CFG_OL_NUM_IPA_TX_BUFF \
	CFG_INI_UINT("num_tx_ipa_buffers", \
	CFG_OL_NUM_IPA_TX_BUFF_MIN, CFG_OL_NUM_IPA_TX_BUFF_MAX, \
	CFG_OL_NUM_IPA_TX_BUFF_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, \
	"Max num buffers")

#define CFG_OL_FW_DUMP_OPTIONS \
	CFG_INI_UINT("fw_dump_options", \
	FW_DUMP_TO_FILE, FW_DUMP_ADD_SIGNATURE, \
	FW_DUMP_TO_CRASH_SCOPE, \
	CFG_VALUE_OR_DEFAULT, "Firmware dump options")

#define CFG_OL_SRP_SR_CONTROL \
	CFG_INI_UINT("he_srp_sr_control", \
	0, 255, \
	IEEE80211_SRP_SR_CONTROL, \
	CFG_VALUE_OR_DEFAULT, "SR Control field in" \
	"HE Spatial Reuse Parameter Set Element")

#define CFG_OL_ALLOW_MON_VAPS_IN_SR \
	CFG_INI_UINT("allow_mon_vaps_in_sr", \
	0, 1, \
	IEEE80211_ALLOW_MON_VAPS_IN_SR, \
	CFG_VALUE_OR_DEFAULT,"Allow monitor vaps to be present " \
	"while using Spatial Reuse")

#define CFG_OL_SELF_NON_SRG_OBSS_PD_THRESHOLD_DB \
	CFG_INI_INT("self_non_srg_obss_pd_threshold_db", \
	SELF_OBSS_PD_LOWER_THRESH , SELF_OBSS_PD_UPPER_THRESH,\
	IEEE80211_SELF_NON_SRG_OBSS_PD_THRESHOLD_DB,\
	CFG_VALUE_OR_DEFAULT,"Self Non-SRG OBSS-PD Threshold in dB")

#define CFG_OL_SELF_NON_SRG_OBSS_PD_THRESHOLD_DBM \
	CFG_INI_INT("self_non_srg_obss_pd_threshold_dbm", \
	SELF_OBSS_PD_LOWER_THRESH , SELF_OBSS_PD_UPPER_THRESH,\
	IEEE80211_SELF_NON_SRG_OBSS_PD_THRESHOLD_DBM,\
	CFG_VALUE_OR_DEFAULT,"Self Non-SRG OBSS-PD Threshold in dBm")

#define CFG_OL_SELF_NON_SRG_OBSS_PD_ENABLE \
	CFG_INI_UINT("self_non_srg_obss_pd_tx_enable", \
	0, 1, \
	IEEE80211_SELF_NON_SRG_OBSS_PD_ENABLE, \
	CFG_VALUE_OR_DEFAULT, "Self Non-SRG OBSS PD Tx Enable")

#define CFG_OL_SELF_SRG_OBSS_PD_THRESHOLD_DB \
	CFG_INI_INT("self_srg_obss_pd_threshold_db", \
	SELF_OBSS_PD_LOWER_THRESH , SELF_OBSS_PD_UPPER_THRESH,\
	IEEE80211_SELF_SRG_OBSS_PD_THRESHOLD_DB,\
	CFG_VALUE_OR_DEFAULT,"Self SRG OBSS-PD Threshold in dB")

#define CFG_OL_SELF_SRG_OBSS_PD_THRESHOLD_DBM \
	CFG_INI_INT("self_srg_obss_pd_threshold_dbm", \
	SELF_OBSS_PD_LOWER_THRESH , SELF_OBSS_PD_UPPER_THRESH,\
	IEEE80211_SELF_SRG_OBSS_PD_THRESHOLD_DBM,\
	CFG_VALUE_OR_DEFAULT,"Self SRG OBSS-PD Threshold in dBm")

#define CFG_OL_SELF_SRG_OBSS_PD_ENABLE \
	CFG_INI_UINT("self_srg_obss_pd_tx_enable", \
	0, 1, \
	IEEE80211_SELF_SRG_OBSS_PD_ENABLE, \
	CFG_VALUE_OR_DEFAULT, "Self SRG OBSS PD Tx Enable")

#define CFG_OL_SRP_NON_SRG_OBSS_PD_MAX_OFFSET \
	CFG_INI_UINT("he_srp_non_srg_obss_pd_max_offset", \
	0, 1000, \
	IEEE80211_SRP_NON_SRG_OBSS_PD_MAX_OFFSET, \
	CFG_VALUE_OR_DEFAULT, "Non-SRG OBSS PD MAX Offset in" \
	"HE Spatial Reuse Parameter Set Element")

#define CFG_OL_SRP_SRG_OBSS_PD_MIN_OFFSET \
	CFG_INI_UINT("he_srp_srg_obss_pd_min_offset", \
	0, 1000, \
	IEEE80211_SRP_SRG_OBSS_PD_MIN_OFFSET, \
	CFG_VALUE_OR_DEFAULT, "SRG OBSS PD MIN Offset in" \
	"HE Spatial Reuse Parameter Set Element")

#define CFG_OL_SRP_SRG_OBSS_PD_MAX_OFFSET \
	CFG_INI_UINT("he_srp_srg_obss_pd_max_offset", \
	0, 1000, \
	IEEE80211_SRP_SRG_OBSS_PD_MAX_OFFSET, \
	CFG_VALUE_OR_DEFAULT, "SRG OBSS PD MAX Offset in" \
	"HE Spatial Reuse Parameter Set Element")

#define CFG_OL_SRP_SRG_BSS_COLOR_BITMAP_HIGH \
	CFG_INI_UINT("he_srp_srg_bss_color_bitmap_high", \
	0,MAX_CFG, \
	IEEE80211_SRP_SRG_BSS_COLOR_BITMAP, \
	CFG_VALUE_OR_DEFAULT, "Higher 32 bits of SRG BSS Color Bitmap in" \
	"HE Spatial Reuse Parameter Set Element")

#define CFG_OL_SRP_SRG_BSS_COLOR_BITMAP_LOW \
	CFG_INI_UINT("he_srp_srg_bss_color_bitmap_low", \
	0,MAX_CFG, \
	IEEE80211_SRP_SRG_BSS_COLOR_BITMAP, \
	CFG_VALUE_OR_DEFAULT, "Lower 32 bits of SRG BSS Color Bitmap in" \
	"HE Spatial Reuse Parameter Set Element")

#define CFG_OL_SRP_SRG_PARTIAL_BSSID_BITMAP_HIGH \
	CFG_INI_UINT("he_srp_srg_partial_bssid_bitmap_high", \
	0,MAX_CFG, \
	IEEE80211_SRP_SRG_PARTIAL_BSSID_BITMAP, \
	CFG_VALUE_OR_DEFAULT, "Higher 32 bits of SRG Partial BSSID Bitmap in" \
	"HE Spatial Reuse Parameter Set Element")

#define CFG_OL_SRP_SRG_PARTIAL_BSSID_BITMAP_LOW \
	CFG_INI_UINT("he_srp_srg_partial_bssid_bitmap_low", \
	0,MAX_CFG, \
	IEEE80211_SRP_SRG_PARTIAL_BSSID_BITMAP, \
	CFG_VALUE_OR_DEFAULT, "Lower 32 bits of SRG Partial BSSID Bitmap in" \
	"HE Spatial Reuse Parameter Set Element")

#define CFG_OL_SR_ENABLE_PER_AC \
	CFG_INI_UINT("sr_enable_per_ac", \
	0, 1000, \
	IEEE80211_SR_ENABLE_PER_AC, \
	CFG_VALUE_OR_DEFAULT, "ACs enabled for SR transmissions")

#define CFG_OL_SELF_PSR_TX_ENABLE \
	CFG_INI_UINT("self_psr_tx_enable", \
	0, 1, \
	IEEE80211_SELF_PSR_TX_ENABLE, \
	CFG_VALUE_OR_DEFAULT, "Self PSR Tx Enable")

#define CFG_OL_SELF_SRG_BSS_COLOR_BITMAP_HIGH \
	CFG_INI_UINT("self_srg_bss_color_bitmap_high", \
	0, MAX_CFG, \
	IEEE80211_SELF_SRG_BSS_COLOR_BITMAP, \
	CFG_VALUE_OR_DEFAULT, "Self SRG BSS Color Bitmap upper 32-bits")

#define CFG_OL_SELF_SRG_BSS_COLOR_BITMAP_LOW \
	CFG_INI_UINT("self_srg_bss_color_bitmap_low", \
	0, MAX_CFG, \
	IEEE80211_SELF_SRG_BSS_COLOR_BITMAP, \
	CFG_VALUE_OR_DEFAULT, "Self SRG BSS Color Bitmap lower 32-bits")

#define CFG_OL_SELF_SRG_PARTIAL_BSSID_BITMAP_HIGH \
	CFG_INI_UINT("self_srg_partial_bssid_bitmap_high", \
	0, MAX_CFG, \
	IEEE80211_SELF_SRG_PARTIAL_BSSID_BITMAP, \
	CFG_VALUE_OR_DEFAULT, "Self SRG Partial BSSID Bitmap upper 32-bits")

#define CFG_OL_SELF_SRG_PARTIAL_BSSID_BITMAP_LOW \
	CFG_INI_UINT("self_srg_partial_bssid_bitmap_low", \
	0, MAX_CFG, \
	IEEE80211_SELF_SRG_PARTIAL_BSSID_BITMAP, \
	CFG_VALUE_OR_DEFAULT, "Self SRG Partial BSSID Bitmap lower 32-bits")

#define CFG_OL_SELF_HESIGA_SR15_ENABLE \
	CFG_INI_UINT("self_hesiga_sr15_enable", \
	0, 1, \
	IEEE80211_SELF_HESIGA_SR15_ENABLE, \
	CFG_VALUE_OR_DEFAULT, "Self HE SIGA SR15 Enable")

#define CFG_OL_TWT_ENABLE \
	CFG_INI_BOOL("twt_enable", \
	true, \
	"TWT Enable/Disable")

#define CFG_OL_B_TWT_ENABLE \
	CFG_INI_BOOL("b_twt_enable", \
	false, \
	"Broadcast TWT Enable/Disable")

#define CFG_OL_R_TWT_ENABLE \
	CFG_INI_BOOL("r_twt_enable", \
	false, \
	"Restricted TWT Enable/Disable")

#define CFG_OL_TWT_STA_CONG_TIMER_MS \
	CFG_INI_UINT("twt_sta_config_timer_ms", \
	0, 0xFFFFFFFF, 5000, \
	CFG_VALUE_OR_DEFAULT, "TWT STa config timer (ms)")

#define CFG_OL_TWT_MBSS_SUPPORT \
	CFG_INI_BOOL("twt_mbss_support", false, \
	"Enable TWT MBSS support")

#define CFG_OL_TWT_DEFAULT_SLOT_SIZE \
	CFG_INI_UINT("twt_default_slot_size", \
	0, 0xFFFFFFFF, 10, \
	CFG_VALUE_OR_DEFAULT, "TWT default slot size")

#define CFG_OL_TWT_CONGESTION_THRESH_SETUP \
	CFG_INI_UINT("twt_congestion_thresh_setup", \
	0, 100, 50, \
	CFG_VALUE_OR_DEFAULT, "Minimum congestion required to setup TWT")

#define CFG_OL_TWT_CONGESTION_THRESH_TEARDOWN \
	CFG_INI_UINT("twt_congestion_thresh_teardown", \
	0, 100, 20, \
	CFG_VALUE_OR_DEFAULT, "Minimum congestion for TWT teardown")

#define CFG_OL_TWT_CONGESTION_THRESH_CRITICAL \
	CFG_INI_UINT("twt_congestion_thresh_critical", \
	0, 100, 100, \
	CFG_VALUE_OR_DEFAULT, "TWT teardown Threshold above which TWT will not be active")

#define CFG_OL_TWT_INTERFERENCE_THRESH_TEARDOWN \
	CFG_INI_UINT("twt_interference_thresh_teardown", \
	0, 100, 80, \
	CFG_VALUE_OR_DEFAULT, "Interference threshold in percentage")

#define CFG_OL_TWT_INTERFERENCE_THRESH_SETUP \
	CFG_INI_UINT("twt_interference_thresh_setup", \
	0, 100, 50, \
	CFG_VALUE_OR_DEFAULT, "TWT Setup Interference threshold in percentage")

#define CFG_OL_TWT_MIN_NUM_STA_SETUP \
	CFG_INI_UINT("twt_min_no_sta_setup", \
	0, 4096, 10, \
	CFG_VALUE_OR_DEFAULT, "Minimum num of STA required for TWT setup")

#define CFG_OL_TWT_MIN_NUM_STA_TEARDOWN \
	CFG_INI_UINT("twt_min_no_sta_teardown", \
	0, 4096, 2, \
	CFG_VALUE_OR_DEFAULT, "Minimum num of STA below which TWT will be torn down")

#define CFG_OL_TWT_NUM_BCMC_SLOTS \
	CFG_INI_UINT("twt_no_of_bcast_mcast_slots", \
	0, 100, 2, \
	CFG_VALUE_OR_DEFAULT, "num of bcast/mcast TWT slots")

#define CFG_OL_TWT_MIN_NUM_SLOTS \
	CFG_INI_UINT("twt_min_no_twt_slots", \
	0, 1000, 2, \
	CFG_VALUE_OR_DEFAULT, "Minimum num of TWT slots")

#define CFG_OL_TWT_MAX_NUM_STA_TWT \
	CFG_INI_UINT("twt_max_no_sta_twt", \
	0, 1000, 500, \
	CFG_VALUE_OR_DEFAULT, "Maximum num of STA TWT slots")

#define CFG_OL_TWT_MODE_CHECK_INTERVAL \
	CFG_INI_UINT("twt_mode_check_interval", \
	0, 0xFFFFFFFF, 10000, \
	CFG_VALUE_OR_DEFAULT, "Interval between two successive check for TWT mode")

#define CFG_OL_TWT_ADD_STA_SLOT_INTERVAL \
	CFG_INI_UINT("twt_add_sta_slot_interval", \
	0, 0xFFFFFFFF, 1000, \
	CFG_VALUE_OR_DEFAULT, "Interval between decision making for TWT slot creation")

#define CFG_OL_TWT_REMOVE_STA_SLOT_INTERVAL \
	CFG_INI_UINT("twt_remove_sta_slot_interval", \
	0, 0xFFFFFFFF, 5000, \
	CFG_VALUE_OR_DEFAULT, "Interval between decision making for TWT slot removal")

#define CFG_OL_AP_BSS_COLOR_COLLISION_DETECTION \
	CFG_INI_BOOL("ap_bss_color_collision_detection", false, \
	"AP BSS COLOR COLLISION DETECTION")

#define CFG_OL_STA_BSS_COLOR_COLLISION_DETECTION \
	CFG_INI_BOOL("sta_bss_color_collision_detection", true, \
	"STA BSS COLOR COLLISION DETECTION")

#define CFG_OL_BSS_COLOR_DEFAULT_VAL \
	CFG_INI_UINT("bss_color_default_val", \
	0, 255, 255, \
	CFG_VALUE_OR_DEFAULT, "Default value for BSS Color (0 means disable feature and val > 63 means pick dynamically)")

#define CFG_OL_SR_IE_ENABLE \
	CFG_INI_BOOL("he_srp_ie_enable", true, \
	"Enable HE Spatial Reuse Parameter Set Element")

#define CFG_TGT_RX_MCS_MAP_MIN 0
#define CFG_TGT_RX_MCS_MAP_MAX 1
#define CFG_TGT_RX_MCS_MAP_DEFAULT 0

#define CFG_OL_SET_MAX_RX_MCS_MAP \
	CFG_INI_UINT("set_max_rx_mcs_map", \
	CFG_TGT_RX_MCS_MAP_MIN, CFG_TGT_RX_MCS_MAP_MAX, CFG_TGT_RX_MCS_MAP_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, \
	"Advertise max RX MCS map in vhtcap of Association request")

/*-------------------------Start MBSSID/EMA INI defn.-----------------------*/
#define CFG_MAX_POSSIBLE_VENDOR_IE_CONFIG 0xffffffff
#define CFG_DEFAULT_VENDOR_IE_CONFIG      0x77777777
#define CFG_MIN_BEACON_CMN_PART_SIZE      200
#define CFG_MAX_BEACON_CMN_PART_SIZE      1200
#define CFG_DEFAULT_BEACON_CMN_PART_SIZE  800

/* Set max value supporting for 4 pdevs at the max */
#define CFG_OL_MBSS_IE_ENABLE \
	CFG_INI_UINT("mbss_ie_enable", 0, 0x000f000f, 0, \
	CFG_VALUE_OR_DEFAULT, "Enable MBSS IE")

/* Exceeding value beyond (max_frm_sz - cmn_part_sz - rnr_part_sz)
 * may lead to undefined behavior
 */
#define CFG_OL_EMA_AP_VENDOR_IE_SIZE_LOW \
	CFG_INI_UINT("ema_ap_vendor_ie_size_low", 0, CFG_MAX_POSSIBLE_VENDOR_IE_CONFIG, \
    CFG_DEFAULT_VENDOR_IE_CONFIG, CFG_VALUE_OR_DEFAULT, \
            "Low 32 bit of vendor ie size config for ema ap")

/* Exceeding value beyond (max_frm_sz - cmn_part_sz - rnr_part_sz)
 * may lead to undefined behavior. msb nibble sets the vendor_ie
 * section limit for each non-tx vap. This limit gets overidden
 * with IEEE80211_EMA_VENDOR_IE_SECTION_BOUND_WITH_WPS (currently
 * defined as 350 bytes) internally in the driver if wps support
 * is enabled along with ema_ext. The same limit will be overidden
 * to IEEE80211_EMA_WPS_IE_OCTET_RESERVATION_BOUND (currently defined
 * as 150 bytes) for ema_ext disabled case
 */
#define CFG_OL_EMA_AP_VENDOR_IE_SIZE_HIGH \
	CFG_INI_UINT("ema_ap_vendor_ie_size_high", 0, CFG_MAX_POSSIBLE_VENDOR_IE_CONFIG, \
    CFG_DEFAULT_VENDOR_IE_CONFIG, CFG_VALUE_OR_DEFAULT, \
            "High 32 bit of vendor ie size config for ema ap")

/*
 * Max possible optional-ie section,
 * derived by:  Max mgmt frame size        2300
 *              RNR size                 -  214
 *              Mandatory IEs size       -   88
 *              Max common beacon size   -  800
 *              Optional IE headroom     -   30
 *              Optional IE max size     = 1168
 *
 * Default value (80 + 86), where 80 is optional ie space reserved
 * for pre-11be elements and 86 for 11be elements (further distribution
 * below.
 *
 * 86 = (5 + 15 + 3*22), where 5 = multi-link element fixed fields,
 * 15 = max common info field, 3*22 = max 3 per-STA profile of each
 * 22 bytes long.
 *
 * Per-STA profile:
 * Header = 2 bytes, STA ctrl = 2 bytes, STA info = 1 byte, STA
 * Profile = max(5+6+6, 8, 9) = 17 bytes (break up as below)
 *
 * STA Profile:
 * CSA = 5 bytes, eCSA= 6 bytes, Max Ch. Switch Time = 6 bytes,
 * Quiet = 8 Bytes, Quiet Channel Time = 9 bytes
 */
#define CFG_OL_EMA_AP_OPTIONAL_IE_SIZE \
	CFG_INI_UINT("ema_ap_optional_ie_size", 0, 1168, (80 + 86), \
	CFG_VALUE_OR_DEFAULT, "Size in bytes of Optional IE for ema ap")

#define CFG_OL_EMA_AP_BEACON_COMMON_PART_SIZE \
	CFG_INI_UINT("ema_ap_beacon_common_part_size", CFG_MIN_BEACON_CMN_PART_SIZE, \
    CFG_MAX_BEACON_CMN_PART_SIZE, CFG_DEFAULT_BEACON_CMN_PART_SIZE, \
	CFG_VALUE_OR_DEFAULT, "Max size of common part of ema ap beacon")

#define CFG_OL_EMA_AP_NUM_MAX_VAPS \
	CFG_INI_UINT("ema_ap_num_max_vaps", CFG_TGT_VDEVS_MIN, CFG_TGT_VDEVS_MAX, \
	CFG_TGT_VDEVS_DEFAULT, CFG_VALUE_OR_DEFAULT, \
		"Max vaps supported with ema ap enabled")

#define CFG_OL_ENABLE_EMA_EXT \
	CFG_INI_BOOL("enable_ema_ap_ext", true, \
		"Enable EMA-ext feature with generic IE support in non-Tx profile")

#define CFG_OL_SPLIT_NON_TX_PROFILE_ENABLED \
	CFG_INI_BOOL("enable_split_non_tx_profile", true, \
		"Split Non-Tx VAP profile if profile exceeds max IE size")

#define CFG_OL_EMA_AP_SUPPORT_WPS_IN_6GHZ \
	CFG_INI_BOOL("ema_ap_support_wps_in_6ghz", false, \
		"Support WPS in EMA mode in 6Ghz")

/* Driver internally promotes a programmed odd value through
 * this config to an even value as DTIM == n*max-PP (i.e, DTIM
 * is multiple of max-PP and current-PP is a factor of max-PP
 * at any instant in time dyanmically
 */
#define CFG_OL_EMA_AP_MAX_PP \
	CFG_INI_UINT("ema_ap_max_pp", 1, 15, 1, \
         CFG_VALUE_OR_DEFAULT, "Max-PP in case of EMA")

#define CFG_OL_EMA_AP_MAX_MGMT_FRM_SZ \
	CFG_INI_UINT("ema_ap_max_mgmt_frm_size", mgmt_frm_size_1_5k, \
         mgmt_frm_size_max, mgmt_frm_size_2_0k, CFG_VALUE_OR_DEFAULT, \
            "Max mgmt frame size in case of EMA")

/* 214 is the required number of bytes for supporting 16 6Ghz
 * APs in RNR IE */
#define CFG_OL_EMA_AP_RNR_FIELD_SIZE_LIMIT \
	CFG_INI_UINT("ema_ap_rnr_field_size_limit", 0, 514, 214, \
        CFG_VALUE_OR_DEFAULT, \
		"Max reserved bytes for RNR IEs in EMA beacons")

/* Allow MBSS common param in all vap interfaces
 * is the default behaviour. if set false, MBSS common param
 * CLI is allowed only on tx VAP interface of the respective
 * MBSS group
 */
#define CFG_OL_ALLOW_MBSS_CMN_PARAM_ALL_VAP \
	CFG_INI_BOOL("allow_mbss_param_all_vap", true, \
		"Allow MBSS common param in all vap interfaces")
/*-------------------------End MBSSID/EMA INI defn.-----------------------*/

#define CFG_OL_6GHZ_RNR_ADV_OVERRIDE \
	CFG_INI_UINT("rnr_6ghz_driver_override", 0, 3, 0, \
        CFG_VALUE_OR_DEFAULT, \
		"Override default 6Ghz RNR advertisement in 6Ghz AP only case")

#define CFG_OL_6GHZ_SELECTIVE_NONTX_ADD \
	CFG_INI_BOOL("rnr_selective_nontx_add", false, \
		"Enable/Disable Selective Non Tx AP advertisement in RNR")

/* Changed to default and max values to support max 3 RNR IEs */
#define CFG_OL_MAX_RNR_IE_ALLOWED \
	CFG_INI_UINT("max_rnr_ie_allowed", 1, 3, 3, \
		CFG_VALUE_OR_DEFAULT,\
		"Max RNR IE allowed")

#define CFG_OL_PEER_EXTENDED_RATE_STATS \
	CFG_INI_BOOL("enable_peer_extended_rate_stats", false, \
	"ENABLE PEER EXTENDED RATE STATS")

#define CFG_OL_PEER_EXTENDED_LINK_STATS \
	CFG_INI_BOOL("enable_peer_extended_link_stats", false, \
	"ENABLE PEER EXTENDED LINK STATS")

#define CFG_OL_PRI20_CFG_BLOCKCHANLIST \
	CFG_INI_STRING("pri20_cfg_blockchanlist", \
		       0, 512, "", \
		       "Primary 20MHz CFG block channel list")

#define CFG_OL_PEER_DEL_WAIT_TIME \
	CFG_INI_UINT("g_peer_del_wait_time", \
	1000, 10000, 4000, \
	CFG_VALUE_OR_DEFAULT, "Timeout to del peer refs")

#define CFG_OL_HONOR_SUPPLICANT_SCAN_IE \
	CFG_INI_BOOL("honor_supplicant_scan_ie", \
	false, \
	"Do not override the supplicant provided IEs in the scan request command when this config is enabled")

#define OFFCHAN_DWELL_TIME_MIN 0
#define OFFCHAN_DWELL_TIME_MAX 2000
#define OFFCHAN_DWELL_TIME_DEFAULT 1500

#define CFG_OL_OFFCHAN_SCAN_DWELL_TIME \
	CFG_INI_UINT("offchan_dwell_time", \
	OFFCHAN_DWELL_TIME_MIN, OFFCHAN_DWELL_TIME_MAX, \
	OFFCHAN_DWELL_TIME_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, "Offchan Scan Dwell Time")

/* ML Probe Response update timer shouldbe bounded by offchan dwell time
 * since ML probe request is sent via offchan scan.
 *
 * Input sanitization is done during fetch.
 */
#define CFG_OL_ML_PRBRESP_UPDATE_TIME \
	CFG_INI_UINT("ml_prbresp_update_time", \
	OFFCHAN_DWELL_TIME_MIN, OFFCHAN_DWELL_TIME_MAX, \
	OFFCHAN_DWELL_TIME_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, "ML Probe Response update time")

#define CFG_OL_RE_UL_RESP \
	CFG_INI_UINT("re_ul_resp", \
	0, 0xFFFFFFFF, 0, \
	CFG_VALUE_OR_DEFAULT, "Enable UL MU-OFDMA/MIMO")

#define CFG_OL_MD_CP_EXT_PDEV \
	CFG_INI_BOOL("md_cp_ext_pdev", true, \
	"Enable ol_ath_softc_net80211")

#define CFG_OL_MD_CP_EXT_PSOC \
	CFG_INI_BOOL("md_cp_ext_psoc", true, \
	"Enable ol_ath_soc_softc")

#define CFG_OL_MD_CP_EXT_VDEV \
	CFG_INI_BOOL("md_cp_ext_vdev", true, \
	"Enable ieee80211vap")

#define CFG_OL_MD_CP_EXT_PEER \
	CFG_INI_BOOL("md_cp_ext_peer", false, \
	"Enable ieee80211_node")

#define CFG_OL_MD_CP_MLO \
	CFG_INI_BOOL("md_cp_mlo", false, \
	"Enable cp_mlo")

#define CFG_OL_MD_CP_MLO_PEER \
	CFG_INI_BOOL("md_cp_mlo_peer", false, \
	"Enable wlan_mlo_peer_context")

#define CFG_OL_MD_CP_MGMT_TXRX \
	CFG_INI_BOOL("md_cp_mgmt_txrx", false, \
	"Enable mgmt_txrx")

#define CFG_OL_MD_CP_MGMT_RX \
	CFG_INI_BOOL("md_cp_mgmt_rx", false, \
	"Enable mgmt_rx")

#define CFG_OL_MD_DP_SOC \
	CFG_INI_BOOL("md_dp_soc", true, \
	"Enable dp_soc")

#define CFG_OL_MD_DP_PDEV \
	CFG_INI_BOOL("md_dp_pdev", true, \
	"Enable dp_pdev")

#define CFG_OL_MD_DP_VDEV \
	CFG_INI_BOOL("md_dp_vdev", true, \
	"Enable dp_vdev")

#define CFG_OL_MD_DP_CFG \
	CFG_INI_BOOL("md_dp_cfg", false, \
	"Enable wlan_cfg_dp")

#define CFG_OL_MD_DP_PEER \
	CFG_INI_BOOL("md_dp_peer", false, \
	"Enable dp_peer")

#define CFG_OL_MD_DP_MLO \
	CFG_INI_BOOL("md_dp_mlo", false, \
	"Enable dp_mlo")

#define CFG_OL_MD_DP_SRNG_REO \
	CFG_INI_BOOL("md_dp_srng_reo", false, \
	"Enable dp_srng_reo")

#define CFG_OL_MD_DP_SRNG_TCL \
	CFG_INI_BOOL("md_dp_srng_tcl", false, \
	"Enable dp_srng_tcl")

#define CFG_OL_MD_DP_SRNG_WBM \
	CFG_INI_BOOL("md_dp_srng_wbm", false, \
	"Enable dp_srng_wbm")

#define CFG_OL_MD_DP_SRNG_RXDMA \
	CFG_INI_BOOL("md_dp_srng_rxdma", false, \
	"Enable dp_srng_rxdma")

#define CFG_OL_MD_DP_LINK_DESC_BANK \
	CFG_INI_BOOL("md_dp_link_desc_bank", false, \
	"Enable dp_link_desc_bank")

#define CFG_OL_MD_DP_HAL_SOC \
	CFG_INI_BOOL("md_dp_hal_soc", true, \
	"Enable dp_hal_soc")

#define CFG_OL_MD_DP_MON \
	CFG_INI_BOOL("md_dp_mon", false, \
	"Enable dp_mon")

#define CFG_OL_MD_DP_MON_PEER \
	CFG_INI_BOOL("md_dp_mon_peer", false, \
	"Enable dp_mon_peer")

#define CFG_OL_MD_DP_TXRX_PEER \
	CFG_INI_BOOL("md_dp_txrx_peer", false, \
	"Enable dp_txrx_peer")

#define CFG_OL_MD_OBJMGR_GLOBAL \
	CFG_INI_BOOL("md_objmgr_global", false, \
	"Enable wlan_objmgr_global")

#define CFG_OL_MD_DP_GLOBAL_CTX \
	CFG_INI_BOOL("md_dp_global_ctx", false, \
	"Enable dp_global_ctx")

#define CFG_OL_MD_OBJMGR_PSOC \
	CFG_INI_BOOL("md_objmgr_psoc", true, \
	"Enable wlan_objmgr_psoc")

#define CFG_OL_MD_OBJMGR_PDEV \
	CFG_INI_BOOL("md_objmgr_pdev", true, \
	"Enable wlan_objmgr_pdev")

#define CFG_OL_MD_OBJMGR_VDEV \
	CFG_INI_BOOL("md_objmgr_vdev", true, \
	"Enable wlan_objmgr_vdev")

#define CFG_OL_MD_OBJMGR_PEER \
	CFG_INI_BOOL("md_objmgr_peer", false, \
	"Enable wlan_objmgr_peer")

#define CFG_OL_CONSOLE_LOG_MASK \
	CFG_INI_UINT("logger_enable_mask", \
	0x0, 0x1E, 0x0, \
	CFG_VALUE_OR_DEFAULT, "Log levels to be printed on console")

#define CFG_OL_WIDEBAND_CSA \
	CFG_INI_UINT("wideband_csa", 0, 2, 0, \
	CFG_VALUE_OR_DEFAULT, "Support for wideband (5-7GHz) CSA")

#define CFG_OL_WDS_EXTENDED \
	CFG_INI_BOOL("wds_ext", false, \
	"Enable WDS Extended feature")

#define CFG_OL_EXTERNALACS_ENABLE \
	CFG_INI_BOOL("externalacs_enable", false, \
	"Enable external Auto Channel Selection")

#define CFG_OL_LED_GPIO_ENABLE_8074 \
	CFG_INI_BOOL("led_gpio_enable_8074", false, \
	"led is toggled using gpio")

#define CFG_OL_RESET_SCAN_SPCL_VAP_STATS_ENABLE \
	CFG_INI_BOOL("reset_scan_spcl_vap_stats_enable", false, \
	"Controls resetting of special vap stats")

#define CFG_OL_EMULATION \
	CFG_INI_BOOL("emulation", \
	false, \
	"Emulation enabled")

#define CFG_OL_AFC_REG_NO_ACTION \
	CFG_INI_BOOL("afc_reg_no_action", false, \
			   "driver/user space action needed for afc resp")

#define CFG_OL_GET_PDEV_STATS_FOR_SPL_VAP \
	CFG_INI_BOOL("spl_vap_get_pdev_stats", false, \
		"Get noise floor via pdev stats for spl vap")

#define CFG_OL_GET_SPL_VAP_WO_WIFI \
	CFG_INI_BOOL("spl_vap_always_up", false, \
		     "Handle ifconfig down differently for ap_monitor")

#define CFG_OL_HOST_TRIG_TXBF_SOUNDING \
	CFG_INI_BOOL("host_trig_txbf_sounding", false, \
	"Enable Host Trigger TxBF Sounding feature")

/* Primary UMAC Knob */
#define CFG_OL_ML_PUMAC_SKIP \
	CFG_INI_BOOL("primary_umac_skip", false, \
	"Skip-Primary UMAC in MLO")

#define CFG_OL_NON_INTERSECTED_PHYMODE \
	CFG_INI_BOOL("non_intersected_phymode", false, \
	"non intersected phy mode")

/*
 * <ini>
 * gBusBandwidthHighThreshold - bus bandwidth high threshold
 *
 * @Min: 0
 * @Max: 4294967295UL
 * @Default: 2000
 *
 * This ini specifies the bus bandwidth high threshold
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_DP_BUS_BANDWIDTH_HIGH_THRESHOLD \
		CFG_INI_UINT( \
		"gBusBandwidthHighThreshold", \
		0, \
		4294967295UL, \
		2000, \
		CFG_VALUE_OR_DEFAULT, \
		"Bus bandwidth high threshold")

/*
 * <ini>
 * gBusBandwidthMediumThreshold - bus bandwidth medium threshold
 *
 * @Min: 0
 * @Max: 4294967295UL
 * @Default: 500
 *
 * This ini specifies the bus bandwidth medium threshold
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_DP_BUS_BANDWIDTH_MEDIUM_THRESHOLD \
		CFG_INI_UINT( \
		"gBusBandwidthMediumThreshold", \
		0, \
		4294967295UL, \
		500, \
		CFG_VALUE_OR_DEFAULT, \
		"Bus bandwidth medium threshold")

/*
 * <ini>
 * gBusBandwidthLowThreshold - bus bandwidth low threshold
 *
 * @Min: 0
 * @Max: 4294967295UL
 * @Default: 150
 *
 * This ini specifies the bus bandwidth low threshold
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_DP_BUS_BANDWIDTH_LOW_THRESHOLD \
		CFG_INI_UINT( \
		"gBusBandwidthLowThreshold", \
		0, \
		4294967295UL, \
		150, \
		CFG_VALUE_OR_DEFAULT, \
		"Bus bandwidth low threshold")

#if QCA_PPE_VP
#define CFG_DP_MLO_VP_CORE_MASK \
		CFG_INI_UINT( \
		"ppe_mlo_vp_core_mask", \
		0, \
		15, \
		7, \
		CFG_VALUE_OR_DEFAULT, \
		"Default core mask for VP for MLO")

#define CFG_DP_VP_CORE_MASK \
		CFG_INI_UINT( \
		"ppe_vp_core_mask", \
		0, \
		15, \
		7, \
		CFG_VALUE_OR_DEFAULT, \
		"Default core mask for VP for Non MLO")
#define CFG_OL_PPE_VP CFG(CFG_DP_MLO_VP_CORE_MASK) \
			CFG(CFG_DP_VP_CORE_MASK)
#else
#define CFG_OL_PPE_VP
#endif

/*
 * <ini>
 * mldev_mode_ap - MLD netdev mode for AP
 *
 * @Min: 0
 * @Max: 2
 * @Default: 1
 *
 * This ini sets MLD netdev mode for AP
 *
 * Usage: Internal
 *
 * </ini>
 */

#ifdef WLAN_FEATURE_11BE_MLO
#ifdef CONFIG_BOND_MOD_SUPPORT
#define CFG_OL_MLDEV_AP_MODE \
	CFG_INI_UINT("mldev_mode_ap", 0, 2, 1, CFG_VALUE_OR_DEFAULT, \
		     "MLD netdev AP mode")
#else /* CONFIG_BOND_MOD_SUPPORT */
#define CFG_OL_MLDEV_AP_MODE \
	CFG_INI_UINT("mldev_mode_ap", 0, 0, 0, CFG_VALUE_OR_DEFAULT, \
		     "MLD netdev AP mode")
#endif /* CONFIG_BOND_MOD_SUPPORT */

#define CFG_MLDEV_AP_MODE CFG(CFG_OL_MLDEV_AP_MODE)
#define MLD_AP_UNIFIED_NONBOND_MODE 0
#define MLD_AP_UNIFIED_BOND_MODE    1
#define MLD_AP_HYBRID_NONBOND_MODE  2

#else /* WLAN_FEATURE_11BE_MLO */
#define CFG_MLDEV_AP_MODE
#endif /* WLAN_FEATURE_11BE_MLO */

/*
 * <ini>
 * mldev_mode_sta - MLD netdev mode for STA
 *
 * @Min: 0
 * @Max: 1
 * @Default: 1 *
 * This ini sets MLD netdev mode for STA
 *
 * Usage: Internal
 *
 * </ini>
 */

#ifdef WLAN_FEATURE_11BE_MLO
#ifdef CONFIG_BOND_MOD_SUPPORT
#define CFG_OL_MLDEV_STA_MODE \
	CFG_INI_UINT("mldev_mode_sta", 0, 1, 1, CFG_VALUE_OR_DEFAULT, \
		     "MLD netdev STA mode")
#else /* CONFIG_BOND_MOD_SUPPORT */
#define CFG_OL_MLDEV_STA_MODE \
	CFG_INI_UINT("mldev_mode_sta", 0, 0, 0, CFG_VALUE_OR_DEFAULT, \
		     "MLD netdev STA mode")
#endif /* CONFIG_BOND_MOD_SUPPORT */

#define CFG_MLDEV_STA_MODE CFG(CFG_OL_MLDEV_STA_MODE)
#define MLD_STA_UNIFIED_NONBOND_MODE 0
#define MLD_STA_UNIFIED_BOND_MODE    1

#else /* WLAN_FEATURE_11BE_MLO */
#define CFG_MLDEV_STA_MODE
#endif /* WLAN_FEATURE_11BE_MLO */

#ifdef CONFIG_SAWF
/*
 * <ini>
 * sawf- Enable/Disable SAWF
 *
 * @Min: 0
 * @Max: 1
 * @Default: 0
 *
 * This ini enables/disables Service Aware Wifi
 *
 * Usage: Internal
 *
 * </ini>
 */

#define CFG_OL_SAWF_ENABLE \
	CFG_INI_BOOL("sawf", false, \
		     "Service Aware Wifi Enable/Disable")

#define CFG_OL_SAWF CFG(CFG_OL_SAWF_ENABLE)

#define CFG_OL_SAWF_UL_FSE_ENABLE \
	CFG_INI_BOOL("sawf_ul_fse", false, \
		     "SAWF UL via FSE")

#define CFG_OL_SAWF_UL_FSE CFG(CFG_OL_SAWF_UL_FSE_ENABLE)

#ifdef SAWF_ADMISSION_CONTROL
#define CFG_OL_SAWF_ADM_CTRL_ENABLE \
	CFG_INI_BOOL("sawf_adm_ctrl", false, \
		     "Service Aware Wifi Admission Control Enable/Disable")
#define CFG_OL_SAWF_ADM_CTRL CFG(CFG_OL_SAWF_ADM_CTRL_ENABLE)
#else
#define CFG_OL_SAWF_ADM_CTRL
#endif /* SAWF_ADMISSION_CONTROL */
#else
#define CFG_OL_SAWF
#define CFG_OL_SAWF_UL_FSE
#define CFG_OL_SAWF_ADM_CTRL
#endif /* CONFIG_SAWF */

/*
 * <ini>
 * fw_ast_indication_disable - Disable AST indication
 *
 * @Min: 0
 * @Max: 1
 * @Default: 0
 *
 * This ini disables AST indication
 *
 * Usage: Internal
 *
 * </ini>
 */

#define CFG_OL_AST_INDICATION_DISABLE \
	CFG_INI_BOOL("fw_ast_indication_disable", false, \
		     "Disable AST indication")

/*
 * <ini>
 * disable_minidump_fw_assert - enable or disable minidump collection
 *                              on fw assert
 * @Min: 0
 * @Max: 1
 * @Default: 0
 *
 * This ini enables or disables minidump collection on fw assert
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_OL_DISABLE_MINIDUMP_FW_ASSERT \
	CFG_INI_BOOL("disable_minidump_fw_assert", false, \
		     "Enable/Disable mindump during fw assert")

#ifdef WLAN_FEATURE_11BE
/*
 * <ini>
 * t2lm_enable - Enable or disable TID-to-link mapping (T2LM) feature.
 *
 * @Min: WLAN_T2LM_NOT_SUPPORTED
 * @Max: WLAN_T2LM_MAP_ALL_TIDS_TO_SAME_LINK_SET
 * @Default: WLAN_T2LM_MAP_ALL_TIDS_TO_SAME_LINK_SET
 *
 * This ini enables/disables the TID-to-link mapping feature
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_OL_T2LM_ENABLE \
	CFG_INI_UINT("t2lm_enable",                 \
	WLAN_T2LM_NOT_SUPPORTED,                         \
	WLAN_T2LM_MAP_ALL_TIDS_TO_SAME_LINK_SET,              \
	WLAN_T2LM_MAP_ALL_TIDS_TO_SAME_LINK_SET,              \
	CFG_VALUE_OR_DEFAULT,                       \
	"enable/disable TID-to-link mapping feature")

#define CFG_OL_ENABLE_T2LM CFG(CFG_OL_T2LM_ENABLE)

/*
 * <ini>
 * default_t2lm_resp - Default T2LM response when T2LM/RM APP is not available
 *
 * @Default: ACCEPT_STA_PREFERENCE
 *
 * This ini is used to set the default T2LM response when T2LM/RM APP is not
 * available. The default behavior is to accept the STA preference.
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_OL_DEFAULT_T2LM_RESP \
	CFG_INI_BOOL("default_t2lm_resp", ACCEPT_STA_PREFERENCE, \
	"Default T2LM response when T2LM/RM APP is not available")

#define CFG_OL_DEFAULT_T2LM_RESPONSE CFG(CFG_OL_DEFAULT_T2LM_RESP)

/*
 * <ini>
 * allow_non_t2lm_sta_assoc - allow non-T2LM STA assoc
 *
 * @Default: NON_T2LM_STA_ASSOC_ALLOW
 *
 * This ini is used to set the default value for allow_non_t2lm_sta_assoc when
 * sta is not supporting t2lm. The default behaviour is to allow assoc.
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_OL_ALLOW_NON_T2LM_STA_ASSOC \
        CFG_INI_BOOL("allow_non_t2lm_sta_assoc", NON_T2LM_STA_ASSOC_ALLOW, \
        "Default T2LM assoc when STA doesnot support T2LM")

#define CFG_OL_NON_T2LM_STA_ASSOCIATION CFG(CFG_OL_ALLOW_NON_T2LM_STA_ASSOC)

/*
 * <ini>
 * enable_mloadvert_degrade_on_cac - enable MLO advertisement
 *                                   degradation on CAC
 *
 * @Default: MLOADVERT_DEGRADE_ON_CAC_DISABLE
 *
 * This INI is used to set the default value of enable_mloadvert_degrade_on_cac.
 * When enable_mloadvert_degrade_on_cac is enabled, the driver carries out MLO
 * advertisement degradation wherein MLO protocol signaling sent by the non-5
 * GHz affiliated APs in the AP MLD is degraded to exclude some 5 GHz affiliated
 * AP information when this 5 GHz affiliated AP is in CAC.
 * The information excluded currently is
 * 1. RNR entry for the 5 GHz affiliated
 * 2. Per-STA profile of Basic Multi link IE
 * AP. The purpose of this exclusion is a workaround to enable clients to
 * connect immediately using non-5 GHz links.
 * This is a tentative measure required until the complete implementation
 * for MLO+DFS is available.
 *
 * Usage: Internal
 *
 *</ini>
 */
#define CFG_OL_ENABLE_MLOADVERT_DEGRADE_ON_CAC\
    CFG_INI_BOOL("enable_mloadvert_degrade_on_cac", \
            MLOADVERT_DEGRADE_ON_CAC_DISABLE, \
            "Default disable MLO advertisement degradation on CAC")

#define CFG_OL_MLOADVERT_DEGRADATION_ON_CAC CFG \
    (CFG_OL_ENABLE_MLOADVERT_DEGRADE_ON_CAC)

/*
 * <ini>
 * disable_eml_capability - Enable EML capabilities advertisement for MLD
 * @Min: false
 * @Max: true
 * @Default: true
 *
 * This ini is used to enable EML capabilities advertisement for MLD
 *
 * Related: None
 *
 * Supported Feature: Enhanced Multi-Link Operation
 *
 * Usage: Internal/External
 *
 * </ini>
 */
#define CFG_OL_ENABLE_EML_CAP \
	CFG_INI_BOOL("enable_eml_capability", true, \
	"Enable EML capabilities for MLD")

#define CFG_OL_ENABLE_EML_CAPABILITY CFG(CFG_OL_ENABLE_EML_CAP)
#else
#define CFG_OL_ENABLE_T2LM
#define CFG_OL_DEFAULT_T2LM_RESPONSE
#define CFG_OL_NON_T2LM_STA_ASSOCIATION
#define CFG_OL_MLOADVERT_DEGRADATION_ON_CAC
#define CFG_OL_ENABLE_EML_CAPABILITY
#endif /* WLAN_FEATURE_11BE */

#ifdef WLAN_FEATURE_11BE_MLO
/*
 * <ini>
 * msd_info_override_enable - Flag to enable overriding FW provided values with
 *                            user provided MSD values
 * @Min: false
 * @Max: true
 * @Default: false
 *
 * This ini is used to enable overriding of MSD capabilities advertisement for AP MLD
 *
 * Related: None
 *
 * Supported Feature: Medium Synchronization Delay
 *
 * Usage: Internal/External
 *
 * </ini>
 */
#define CFG_OL_MSD_INFO_OVERRIDE_EN \
	CFG_INI_BOOL("msd_info_override_enable", false, \
	"Enable MSD capability overriding for MLD")

#define CFG_OL_MSD_INFO_OVERRIDE_ENABLE CFG(CFG_OL_MSD_INFO_OVERRIDE_EN)

/*
 * <ini>
 * msd_info_present_override - Flag to control presence of MSD info in frame
 *                             NOTE: Valid only if msd_info_override_enable is True
 * @Min: false
 * @Max: true
 * @Default: false
 *
 * This ini is used control the presence of MSD capabilities in frame for AP MLD
 *
 * Related: None
 *
 * Supported Feature: Medium Synchronization Delay
 *
 * Usage: Internal/External
 *
 * </ini>
 */
#define CFG_OL_MSD_INFO_PRSNT_OVERRIDE \
	CFG_INI_BOOL("msd_info_present_override", false, \
	"Control presence of MLD info")

#define CFG_OL_MSD_INFO_PRESENT_OVERRIDE CFG(CFG_OL_MSD_INFO_PRSNT_OVERRIDE)

/*
 * <ini>
 * msd_info_value_override - Override default MSD values for MSD capabilities advertisement
 *                           NOTE: Valid only if msd_info_value_override_enable is True
 * @Min: 0
 * @Max: 0xFFFF
 * @Default: 0
 *
 * This ini is used to override default MSD values for MSD capabilities advertisement for AP MLD
 *
 * Related: None
 *
 * Supported Feature: Medium Synchronization Delay
 *
 * Usage: Internal/External
 *
 * </ini>
 */
#define CFG_OL_MSD_INFO_VAL_OVERRIDE \
		CFG_INI_UINT( \
		"msd_info_value_override", \
		0, \
		0xFFFF, \
		0, \
		CFG_VALUE_OR_DEFAULT, \
		"Overriding value of MSD Capabilities")

#define CFG_OL_MSD_INFO_VALUE_OVERRIDE CFG(CFG_OL_MSD_INFO_VAL_OVERRIDE)
#else
#define CFG_OL_MSD_INFO_OVERRIDE_ENABLE
#define CFG_OL_MSD_INFO_PRESENT_OVERRIDE
#define CFG_OL_MSD_INFO_VALUE_OVERRIDE
#endif /* WLAN_FEATURE__11BE_MLO*/

#if defined(WIFI_POS_CONVERGED) && defined(WLAN_FEATURE_RTT_11AZ_SUPPORT)
/*
 * <ini>
 * enable_responder_secure_ltf_support - R-STA secure LTF keyseed support
 * @Min: false
 * @Max: true
 * @Default: false
 *
 * This ini is used to enable R-STA advertising secure LTF support.
 *
 * Related: None
 *
 * Supported Feature: WIFI POS
 *
 * Usage: Internal/External
 *
 * </ini>
 */
#define CFG_RESPONDER_SECURE_LTF_SUPPORT CFG_INI_BOOL( \
        "enable_responder_secure_ltf_support", \
        false, \ "enable Responder secure LTF support")

#define CFG_OL_RESPONDER_SECURE_LTF_SUPPORT \
    CFG(CFG_RESPONDER_SECURE_LTF_SUPPORT)

/*
 * <ini>
 * enable_responder_11az_support - R-STA 11az ranging support
 * @Min: 0
 * @Max: CFG_RESPONDER_11AZ_MAX_SUPPORT
 * @Default: 0
 *
 * This ini is used to enable R-STA advertising 11az ranging
 * capabilities.
 *
 * Related: None
 *
 * Supported Feature: WIFI POS
 *
 * Usage: Internal/External
 *
 * </ini>
 */
#define CFG_RESPONDER_11AZ_SUPPORT CFG_INI_UINT( \
        "enable_responder_11az_support", \
        0, \
        CFG_RESPONDER_11AZ_MAX_SUPPORT, \
        0, \
        CFG_VALUE_OR_DEFAULT, \
        "enable Responder 11az support")

#define CFG_OL_RESPONDER_11AZ_SUPPORT CFG(CFG_RESPONDER_11AZ_SUPPORT)
#else
#define CFG_OL_RESPONDER_SECURE_LTF_SUPPORT
#define CFG_OL_RESPONDER_11AZ_SUPPORT
#endif

/*
 * <ini>
 * enable_responder_11mc_support - R-STA 11mc ranging support
 * @Min: false
 * @Max: true
 * @Default: false
 *
 * This ini is used to enable R-STA advertising 11mc ranging
 * capabilities.
 *
 * Related: None
 *
 * Supported Feature: WIFI POS
 *
 * Usage: Internal/External
 *
 * </ini>
 */
#define CFG_RESPONDER_11MC_SUPPORT CFG_INI_BOOL( \
        "enable_responder_11mc_support", \
        false, "enable Responder 11mc support")

#define CFG_OL_RESPONDER_11MC_SUPPORT CFG(CFG_RESPONDER_11MC_SUPPORT)

/*
 * <ini>
 * direct_dma_handler_ctx - Set the execution context for handler
 * @Min: WMI_RX_WORK_CTX
 * @Max: WMI_RX_DIAG_WORK_CTX
 * @Default: WMI_RX_UMAC_CTX
 *
 * This ini is used to set the execution context for handler
 *
 * Related: None
 *
 * Supported Feature: Direct DMA
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_DIRECT_DMA_HANDLER_CTX \
	CFG_INI_UINT("direct_dma_handler_ctx", \
	WMI_RX_WORK_CTX, WMI_RX_DIAG_WORK_CTX, WMI_RX_UMAC_CTX, \
	CFG_VALUE_OR_DEFAULT, "Set the handler ctx for Direct DMA processing")

/*
 * <ini>
 * enable_fw_recov_logs - Enable FW recovery logs
 * @Default: false
 *
 * This ini is used to enable/disable logs while Fw recovery
 * for KPI measures
 *
 * Related: none
 *
 * </ ini>
 */
#define CFG_OL_ENABLE_RECOVERY_LOGS \
	CFG_INI_BOOL("enable_fw_recov_logs", false, \
	"Enable logs while fw recovery")

/*
 * <ini>
 * enable_fw_recov_mode_mask - Enable FW recovery mode mask feature
 * @Default: false
 *
 * This ini is used to enable mode1 Fw recovery feature
 *
 * Related: none
 *
 * </ ini>
 */
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
#define CFG_OL_ENABLE_TARGET_RECOVERY \
	CFG_INI_BOOL("enable_fw_recov_mode_mask", false, \
	"Enable mode1 fw recovery feature")

/*
 * <ini>
 * disable_multi_recovery - Debug ini to disable multiple recovery
 * @Default: false
 *
 * This ini is used to enable mode1 Fw recovery feature
 *
 * Related: none
 *
 * </ ini>
 */
#define CFG_OL_DISABLE_MULTI_RECOVERY \
	CFG_INI_BOOL("disable_multi_recovery", false, \
	"Disable followup recovery when multi assert")
#define CFG_OL_ENABLE_TARGET_RECOVERY_TRIG CFG(CFG_OL_ENABLE_TARGET_RECOVERY)
#define CFG_OL_DISABLE_MULTI_RECOVERY_TRIG CFG(CFG_OL_DISABLE_MULTI_RECOVERY)
#else
#define CFG_OL_ENABLE_TARGET_RECOVERY_TRIG
#define CFG_OL_DISABLE_MULTI_RECOVERY_TRIG
#endif

#if QCA_MANUAL_TRIGGERED_ULOFDMA
/*
 * <ini>
 * manual_ulofdma_trig - manual triggered ulofdma
 * @Min: 0x0
 * @Max: 0x7
 * @Default: 0x0
 *
 * This ini is used to enable manual trig ulofdma feature in bitmap
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_OL_MANUAL_ULOFDMA \
		CFG_INI_UINT( \
		"manual_ulofdma_trig", \
		0, \
		0x7, \
		0, \
		CFG_VALUE_OR_DEFAULT, \
		"Manual ulofdma trigger enable bitmap")


#define CFG_OL_MANUAL_ULOFDMA_TRIG CFG(CFG_OL_MANUAL_ULOFDMA)
#else
#define CFG_OL_MANUAL_ULOFDMA_TRIG
#endif /* QCA_MANUAL_TRIGGERED_ULOFDMA */

#ifdef WLAN_DBS_POWER_REDUCTION
/*
 * <ini>
 * enable_ap_power_reduction - Enable AP power reduction feature
 * @Default: false
 *
 * This ini is used to enable AP power reduction feature based on
 * DBS In or Out trigger
 *
 * Related: none
 *
 * </ ini>
 */
#define CFG_OL_AP_POWER_REDUCTION \
	CFG_INI_BOOL("enable_ap_power_reduction", false, \
	"Enable AP power reduction feature")

/*
 * <ini>
 * eth_port_count_th - Threshold for ethernet port count to effectuate
 * eth power reduction in case of DBS IN
 * @Min: 0x0
 * @Max: 0x3
 * @Default: 0x3
 *
 * This ini is used to define ethernet port count Threshold to effectuate
 * eth power reduction in case of DBS IN
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_OL_ETH_PORT_COUNT_TH \
	CFG_INI_UINT( \
	"eth_port_count_th", \
	0, \
	0x3, \
	0x3, \
	CFG_VALUE_OR_DEFAULT, \
	"Threshold for ethernet port count for eth power reduction")

/*
 * <ini>
 * dbs_power_reduction_0_25_dbm - power reduction value for DBS in mode
 * @Min: 0x0
 * @Max: 0xf
 * @Default: 0x0
 *
 * This ini is used to indicate the thermal power reduction value
 * based on DBS in or out. Value 1 means 0.25 dbm hence, 4 means 1 dbm
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_OL_DBS_POWER_REDUCTION_DBM \
	CFG_INI_UINT( \
	"dbs_power_reduction_0_25_dbm", \
	0, \
	0xf, \
	0x0, \
	CFG_VALUE_OR_DEFAULT, \
	"DBS power reduction value with 0.25 dbm")

/*
 * <ini>
 * eth_power_reduction_0_25_dbm - power reduction value for ethernet ports
 * @Min: 0x0
 * @Max: 0xf
 * @Default: 0x0
 *
 * This ini is used to indicate the thermal power reduction value
 * based on ethernet port change. Value 1 means 0.25 dbm hence, 4 means 1 dbm
 *
 * Usage: External
 *
 * </ini>
 */
#define CFG_OL_ETH_POWER_REDUCTION_DBM \
	CFG_INI_UINT( \
	"eth_power_reduction_0_25_dbm", \
	0, \
	0xf, \
	0x0, \
	CFG_VALUE_OR_DEFAULT, \
	"Ethernet power reduction value with 0.25 dbm")

#define CFG_OL_AP_PWR_REDUCTION	CFG(CFG_OL_AP_POWER_REDUCTION)
#define CFG_OL_ETH_PORT_COUNT_THRESHOLD CFG(CFG_OL_ETH_PORT_COUNT_TH)
#define CFG_OL_DBS_PWR_REDUCTION_VALUE CFG(CFG_OL_DBS_POWER_REDUCTION_DBM)
#define CFG_OL_ETH_PWR_REDUCTION_VALUE CFG(CFG_OL_ETH_POWER_REDUCTION_DBM)
#else /* WLAN_DBS_POWER_REDUCTION */
#define CFG_OL_AP_PWR_REDUCTION
#define CFG_OL_ETH_PORT_COUNT_THRESHOLD
#define CFG_OL_DBS_PWR_REDUCTION_VALUE
#define CFG_OL_ETH_PWR_REDUCTION_VALUE
#endif /* WLAN_DBS_POWER_REDUCTION */

/*
 * <ini>
 * cohosted_bss_ind_enable - enable 11ax complient bssids for cohosted bss
 * @Min: 0x0
 * @Max: 0x7
 * @Default: 0x7
 *
 * This ini is used to set the cohosted bss
 * in a pdev to follow 11ax compliant BSSIDs
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_OL_COHOSTED_BSS_IND_ENABLE \
		CFG_INI_UINT( \
		"cohosted_bss_ind_enable", \
		0, \
		0x7, \
		0x7, \
		CFG_VALUE_OR_DEFAULT, \
		"Enable/Disable Co-hosted BSS indication")

/*
 * <ini>
 * ndev_name_ctrl - control knob to allow unrestricted string for netdev-name
 * @Default: false
 *
 * This ini-param is used to allow unrestricted strings for netdev-name
 *
 * Related: none
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_OL_WILDCARD_NDEV_NAME \
	CFG_INI_BOOL("ndev_name_ctrl", false, \
	"Enable unrestricted strings for netdev name")

/*
 * <ini>
 * disable_bridge_vap - ini to disable bridge vap support
 * @Default: false
 *
 * This ini is used to disable MLO bridge vap support
 * on a 4 link system
 *
 * Related: none
 *
 * </ ini>
 */
#define CFG_OL_BRIDGE_VAP_DISABLE \
	CFG_INI_BOOL("disable_bridge_vap", false, \
	"Disable MLO bridge VAP support")

/*
 * enable_dpdk - enable dpdk support
 * @Min: 0
 * @Max: 1
 * @Default: 0
 *
 * This ini is used to enable dpdk support
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_DPDK_WIFI CFG_INI_UINT("enable_dpdk", 0, 1, 0, \
		CFG_VALUE_OR_DEFAULT, "DPDK Wifi Configuration")

/*
 * <ini>
 * macreq_ext_enable - Enable extended MAC request feature
 * @Default: true
 *
 * This ini is used to extend MAC request to
 * MBSSID and co-hosted mode BSSID indication
 * to use a static MAC address scheme
 *
 * Usage: Internal/External
 *
 * </ini>
 */
#define CFG_OL_MACREQ_EXT_ENABLE \
	CFG_INI_BOOL("macreq_ext_enable", true, \
	             "Enable/Disable Extended MAC request BSSID indication")

/*
 * <ini>
 * enable_cfg_scan_war - Enable WAR for CFG scan issue
 * @Default: false
 *
 * This ini is used to enable WAR to address
 * issue is CFG modules related to RB tree
 * during STA side scan.
 *
 * Usage: Internal/External
 *
 * </ini>
 */
#define CFG_OL_CFG_SCAN_WAR \
	CFG_INI_BOOL("cfg_scan_war", false, \
	             "Enable/Disable WAR for CFG scan issue")


#define CFG_CFG80211_COMPLIANCE_MAX_SUPPORT 0xff
/*
 * <ini>
 * enable_cfg80211_compliance - ini to enable cfg80211 compliance
 * related implementations
 * @Min: 0
 * @Max: CFG_CFG80211_COMPLIANCE_MAX_SUPPORT
 * @Default: 0
 *
 * This ini is used to enable implementations for cfg80211 compliance.
 *
 * Related: none
 *
 * </ ini>
 */
#define CFG_CFG80211_COMPLIANCE CFG_INI_UINT("enable_cfg80211_compliance", \
        0, \
        CFG_CFG80211_COMPLIANCE_MAX_SUPPORT, \
        0, \
        CFG_VALUE_OR_DEFAULT, \
	"Enable cfg80211 compliance mode")
#define CFG_OL_CFG80211_COMPLIANCE CFG(CFG_CFG80211_COMPLIANCE)

#define CFG_OL_EXTENDED_REGINFO_SUPPORT\
	CFG_INI_BOOL("dot11_extended_reginfo_support", true, \
	             "Enable/Disable Extended MAC request BSSID indication")

#ifdef WLAN_SUPPORT_TX_PKT_CAP_CUSTOM_CLASSIFY
/*
 * <ini>
 * TX CCE - Enable/Disable TX Packet capture custom classification
 *
 * @Min: 0
 * @Max: 31
 * @Default: 0
 *
 * This ini enables/disables TX Packet capture custome classification
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_OL_TX_PKT_CAP_PROTO_MIN 0
#define CFG_OL_TX_PKT_CAP_PROTO_MAX 31
#define CFG_OL_TX_PKT_CAP_PROTO_DEFAULT 0

#define CFG_OL_TX_PKT_CAP_CUSTOM_CLASSIFY_PROTO \
	CFG_INI_UINT("tx_pkt_cap_custom_classify_proto_config", \
	CFG_OL_TX_PKT_CAP_PROTO_MIN, CFG_OL_TX_PKT_CAP_PROTO_MAX, \
	CFG_OL_TX_PKT_CAP_PROTO_DEFAULT, \
	CFG_VALUE_OR_DEFAULT, \
	"Tx pkt cap custom classify protocol config")

#define CFG_OL_TX_PKT_CAP_PROTO CFG(CFG_OL_TX_PKT_CAP_CUSTOM_CLASSIFY_PROTO)

#else
#define CFG_OL_TX_PKT_CAP_PROTO
#endif /*WLAN_SUPPORT_TX_PKT_CAP_CUSTOM_CLASSIFY */

#ifdef DP_TX_SW_DROP_STATS_INC
/*
 * <ini>
 * DP EAPOL drop Statistics - Enable/Disable DP EAPOL drop statistics
 *
 * @Default: disable
 *
 * This ini Enables/Disables DP EAPOL drop Statistics
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_DP_EAPOL_DROP_STATISTICS \
	CFG_INI_BOOL("dp_enable_eap_drop_stats", false, \
	"DP EAPOL drop statistics")
#define CFG_DP_EAPOL_STATS CFG(CFG_DP_EAPOL_DROP_STATISTICS)

#else
#define CFG_DP_EAPOL_STATS
#endif /* DP_TX_SW_DROP_STATS_INC */

#ifdef QCA_DP_PROTOCOL_STATS
/*
 * <ini>
 * DP PROTOCOL Statistics - Enable/Disable DP Protocol Statistics
 *
 * @Default: disable
 *
 * This ini Enables/Disables DP Protocol Statistics
 *
 * Usage: Internal
 *
 * </ini>
 */
#define CFG_DP_PROTOCOL_STATISTICS \
	CFG_INI_BOOL("dp_enable_protocol_stats", false, \
	"DP Protocol statistics")

#define CFG_DP_PROTO_STATS CFG(CFG_DP_PROTOCOL_STATISTICS)

#else
#define CFG_DP_PROTO_STATS
#endif /* QCA_DP_PROTOCOL_STATS */

#define CFG_OL \
	CFG(CFG_OL_ENABLE_11AX_STUB) \
	CFG(CFG_OL_TX_TCP_CKSUM) \
	CFG(CFG_OL_VOW_CONFIG) \
	CFG(CFG_OL_CARRIER_VOW_CONFIG) \
	CFG(CFG_OL_FW_VOW_STATS_ENABLE) \
	CFG(CFG_OL_QWRAP_ENABLE) \
	CFG(CFG_OL_CCE_DISABLE) \
	CFG(CFG_OL_LOW_MEM_SYSTEM) \
	CFG(CFG_OL_BEACON_OFFLOAD_DISABLE) \
	CFG(CFG_OL_EAPOL_OVER_NL_ENABLE) \
	CFG(CFG_OL_ADV_RAW_MODE_WAR_ENABLE) \
	CFG(CFG_OL_NAWDS_LEARN_DEFER) \
        CFG(CFG_OL_ENABLE_MONITOR_PEER_DECAP) \
	CFG(CFG_OL_ENABLE_UART_PRINT) \
	CFG(CFG_OL_ENABLE_MESH_SUPPORT) \
	CFG(CFG_OL_LTEU_SUPPORT) \
	CFG(CFG_OL_CFG80211_CONFIG) \
	CFG(CFG_OL_CFG_IPHDR) \
	CFG(CFG_OL_BMI) \
	CFG(CFG_OL_MAX_DESC) \
	CFG(CFG_OL_MAX_PEERS) \
	CFG(CFG_OL_STRICT_CHANNEL_MODE) \
	CFG(CFG_OL_ACBK_MIN_FREE) \
	CFG(CFG_OL_ACBE_MIN_FREE) \
	CFG(CFG_OL_ACVI_MIN_FREE) \
	CFG(CFG_OL_ACVO_MIN_FREE) \
	CFG(CFG_OL_OTP_MOD_PARAM) \
	CFG(CFG_OL_EMU_TYPE) \
	CFG(CFG_OL_MAX_ACTIVE_PEERS) \
	CFG(CFG_OL_MAX_GROUP_KEYS) \
	CFG(CFG_OL_HW_MODE_ID_SOC0) \
	CFG(CFG_OL_HW_MODE_ID_SOC1) \
	CFG(CFG_OL_HW_MODE_ID_SOC2) \
	CFG(CFG_OL_HW_MODE_ID_SOC3) \
	CFG(CFG_OL_DYNAMIC_HW_MODE) \
	CFG(CFG_OL_DYNAMIC_HW_MODE_PRIMARY_IF) \
	CFG(CFG_OL_TGT_SCHED_PARAM) \
	CFG(CFG_OL_FW_CODE_SIGN) \
	CFG(CFG_OL_ALLOCRAM_TRACK_MAX) \
	CFG(CFG_OL_MAX_VAPS) \
	CFG(CFG_OL_MAX_CLIENTS) \
	CFG(CFG_OL_NUM_VDEVS) \
	CFG(CFG_OL_NUM_PEERS) \
	CFG(CFG_OL_MONITOR_VAPS) \
	CFG(CFG_OL_NUM_VDEV_MESH) \
	CFG(CFG_OL_QWRAP_VDEVS) \
	CFG(CFG_OL_QWRAP_PEERS) \
	CFG(CFG_OL_FW_DUMP_OPTIONS) \
	CFG(CFG_OL_SRP_SR_CONTROL) \
	CFG(CFG_OL_ALLOW_MON_VAPS_IN_SR) \
	CFG(CFG_OL_SELF_NON_SRG_OBSS_PD_THRESHOLD_DB) \
	CFG(CFG_OL_SELF_NON_SRG_OBSS_PD_THRESHOLD_DBM) \
	CFG(CFG_OL_SELF_NON_SRG_OBSS_PD_ENABLE) \
	CFG(CFG_OL_SELF_SRG_OBSS_PD_THRESHOLD_DB) \
	CFG(CFG_OL_SELF_SRG_OBSS_PD_THRESHOLD_DBM) \
	CFG(CFG_OL_SELF_SRG_OBSS_PD_ENABLE) \
	CFG(CFG_OL_SRP_NON_SRG_OBSS_PD_MAX_OFFSET) \
	CFG(CFG_OL_SRP_SRG_OBSS_PD_MIN_OFFSET) \
	CFG(CFG_OL_SRP_SRG_OBSS_PD_MAX_OFFSET)\
	CFG(CFG_OL_SRP_SRG_BSS_COLOR_BITMAP_HIGH) \
	CFG(CFG_OL_SRP_SRG_BSS_COLOR_BITMAP_LOW) \
	CFG(CFG_OL_SRP_SRG_PARTIAL_BSSID_BITMAP_HIGH) \
	CFG(CFG_OL_SRP_SRG_PARTIAL_BSSID_BITMAP_LOW) \
	CFG(CFG_OL_SR_ENABLE_PER_AC) \
	CFG(CFG_OL_SELF_PSR_TX_ENABLE) \
	CFG(CFG_OL_SELF_SRG_BSS_COLOR_BITMAP_HIGH) \
	CFG(CFG_OL_SELF_SRG_BSS_COLOR_BITMAP_LOW) \
	CFG(CFG_OL_SELF_SRG_PARTIAL_BSSID_BITMAP_HIGH) \
	CFG(CFG_OL_SELF_SRG_PARTIAL_BSSID_BITMAP_LOW) \
	CFG(CFG_OL_SELF_HESIGA_SR15_ENABLE) \
	CFG(CFG_OL_EAPOL_MINRATE_SET) \
	CFG(CFG_COMMAND_LOGGING_SUPPORT) \
	CFG(CFG_OL_EAPOL_MINRATE_AC_SET) \
	CFG(CFG_OL_TWT_ENABLE) \
	CFG(CFG_OL_B_TWT_ENABLE) \
	CFG(CFG_OL_R_TWT_ENABLE) \
	CFG(CFG_OL_TWT_STA_CONG_TIMER_MS) \
	CFG(CFG_OL_TWT_MBSS_SUPPORT) \
	CFG(CFG_OL_TWT_DEFAULT_SLOT_SIZE) \
	CFG(CFG_OL_TWT_CONGESTION_THRESH_SETUP) \
	CFG(CFG_OL_TWT_CONGESTION_THRESH_TEARDOWN) \
	CFG(CFG_OL_TWT_CONGESTION_THRESH_CRITICAL) \
	CFG(CFG_OL_TWT_INTERFERENCE_THRESH_TEARDOWN) \
	CFG(CFG_OL_TWT_INTERFERENCE_THRESH_SETUP) \
	CFG(CFG_OL_TWT_MIN_NUM_STA_SETUP) \
	CFG(CFG_OL_TWT_MIN_NUM_STA_TEARDOWN) \
	CFG(CFG_OL_TWT_NUM_BCMC_SLOTS) \
	CFG(CFG_OL_TWT_MIN_NUM_SLOTS) \
	CFG(CFG_OL_TWT_MAX_NUM_STA_TWT) \
	CFG(CFG_OL_TWT_MODE_CHECK_INTERVAL) \
	CFG(CFG_OL_TWT_ADD_STA_SLOT_INTERVAL) \
	CFG(CFG_OL_TWT_REMOVE_STA_SLOT_INTERVAL) \
	CFG(CFG_OL_AP_BSS_COLOR_COLLISION_DETECTION) \
	CFG(CFG_OL_STA_BSS_COLOR_COLLISION_DETECTION) \
	CFG(CFG_OL_BSS_COLOR_DEFAULT_VAL) \
	CFG(CFG_OL_MBSS_IE_ENABLE) \
	CFG(CFG_OL_EMA_AP_VENDOR_IE_SIZE_LOW) \
	CFG(CFG_OL_EMA_AP_VENDOR_IE_SIZE_HIGH) \
	CFG(CFG_OL_EMA_AP_OPTIONAL_IE_SIZE) \
	CFG(CFG_OL_EMA_AP_BEACON_COMMON_PART_SIZE) \
	CFG(CFG_OL_EMA_AP_NUM_MAX_VAPS) \
	CFG(CFG_OL_ENABLE_EMA_EXT) \
	CFG(CFG_OL_SPLIT_NON_TX_PROFILE_ENABLED) \
	CFG(CFG_OL_EMA_AP_SUPPORT_WPS_IN_6GHZ) \
	CFG(CFG_OL_EMA_AP_MAX_PP) \
	CFG(CFG_OL_EMA_AP_MAX_MGMT_FRM_SZ) \
	CFG(CFG_OL_EMA_AP_RNR_FIELD_SIZE_LIMIT) \
	CFG(CFG_OL_SET_MAX_RX_MCS_MAP) \
	CFG(CFG_OL_SR_IE_ENABLE) \
	CFG(CFG_OL_PEER_EXTENDED_RATE_STATS) \
	CFG(CFG_OL_PEER_EXTENDED_LINK_STATS) \
	CFG(CFG_OL_PRI20_CFG_BLOCKCHANLIST) \
	CFG(CFG_OL_PEER_DEL_WAIT_TIME) \
	CFG(CFG_OL_HONOR_SUPPLICANT_SCAN_IE) \
	CFG(CFG_OL_OFFCHAN_SCAN_DWELL_TIME) \
	CFG(CFG_OL_ML_PRBRESP_UPDATE_TIME) \
	CFG(CFG_OL_RE_UL_RESP) \
	CFG(CFG_OL_MD_CP_EXT_PDEV) \
	CFG(CFG_OL_MD_CP_EXT_PSOC) \
	CFG(CFG_OL_MD_CP_EXT_VDEV) \
	CFG(CFG_OL_MD_CP_EXT_PEER) \
	CFG(CFG_OL_MD_CP_MLO) \
	CFG(CFG_OL_MD_CP_MLO_PEER) \
	CFG(CFG_OL_MD_CP_MGMT_TXRX) \
	CFG(CFG_OL_MD_CP_MGMT_RX) \
	CFG(CFG_OL_MD_DP_SOC) \
	CFG(CFG_OL_MD_DP_PDEV) \
	CFG(CFG_OL_MD_DP_VDEV) \
	CFG(CFG_OL_MD_DP_CFG) \
	CFG(CFG_OL_MD_DP_PEER) \
	CFG(CFG_OL_MD_DP_MLO) \
	CFG(CFG_OL_MD_DP_SRNG_REO) \
	CFG(CFG_OL_MD_DP_SRNG_TCL) \
	CFG(CFG_OL_MD_DP_SRNG_WBM) \
	CFG(CFG_OL_MD_DP_LINK_DESC_BANK) \
	CFG(CFG_OL_MD_DP_SRNG_RXDMA) \
	CFG(CFG_OL_MD_DP_HAL_SOC) \
	CFG(CFG_OL_MD_DP_MON) \
	CFG(CFG_OL_MD_DP_MON_PEER) \
	CFG(CFG_OL_MD_DP_TXRX_PEER) \
	CFG(CFG_OL_MD_OBJMGR_GLOBAL) \
	CFG(CFG_OL_MD_DP_GLOBAL_CTX) \
	CFG(CFG_OL_MD_OBJMGR_PSOC) \
	CFG(CFG_OL_MD_OBJMGR_PDEV) \
	CFG(CFG_OL_MD_OBJMGR_VDEV) \
	CFG(CFG_OL_MD_OBJMGR_PEER) \
	CFG(CFG_OL_ASE_OVERRIDE) \
	CFG(CFG_OL_MODE_2G_PHYB) \
	CFG(CFG_HTC_READY_TIMEOUT_MSEC) \
	CFG(CFG_WMI_READY_TIMEOUT_MSEC) \
	CFG(CFG_OL_MAX_WMI_CMDS) \
	CFG(CFG_OL_CONSOLE_LOG_MASK) \
	CFG(CFG_OL_WIDEBAND_CSA) \
	CFG(CFG_OL_6GHZ_RNR_ADV_OVERRIDE) \
	CFG(CFG_OL_6GHZ_SELECTIVE_NONTX_ADD) \
	CFG(CFG_OL_WDS_EXTENDED) \
	CFG(CFG_OL_EXTERNALACS_ENABLE) \
	CFG(CFG_OL_CARRIER_VOW_OPTIMIZATION) \
	CFG(CFG_OL_SCAN_RADIO_TX_ON_DFS_CHANNELS) \
	CFG(CFG_OL_RETAIN_NOL_ACROSS_DRIVER_RELOAD) \
	CFG(CFG_OL_LED_GPIO_ENABLE_8074) \
	CFG(CFG_OL_MAX_RNR_IE_ALLOWED) \
	CFG(CFG_OL_RESET_SCAN_SPCL_VAP_STATS_ENABLE) \
	CFG(CFG_OL_EMULATION) \
	CFG(CFG_OL_ALLOW_MBSS_CMN_PARAM_ALL_VAP) \
	CFG(CFG_OL_6GHZ_SP_POWER_MODE_SUPP) \
	CFG(CFG_OL_AFC_TIMER_CHECK_DIS) \
	CFG(CFG_OL_AFC_REQ_ID_CHECK_DIS) \
	CFG(CFG_OL_AFC_REG_NO_ACTION) \
	CFG(CFG_DP_BUS_BANDWIDTH_HIGH_THRESHOLD) \
	CFG(CFG_DP_BUS_BANDWIDTH_MEDIUM_THRESHOLD) \
	CFG(CFG_DP_BUS_BANDWIDTH_LOW_THRESHOLD) \
	CFG(CFG_OL_CARRIER_PROFILE_CONFIG) \
	CFG(CFG_OL_GET_PDEV_STATS_FOR_SPL_VAP) \
	CFG_OL_PPE_VP \
	CFG(CFG_DIRECT_DMA_HANDLER_CTX) \
	CFG(FORCE_DISABLE_HE_MCS_12_13_ANNOUNCEMENT) \
	CFG(CFG_OL_AST_INDICATION_DISABLE) \
	CFG(CFG_OL_DISABLE_MINIDUMP_FW_ASSERT) \
	CFG_OL_SAWF \
	CFG_OL_SAWF_ADM_CTRL \
	CFG_OL_SAWF_UL_FSE \
	CFG_MLDEV_AP_MODE \
	CFG_MLDEV_STA_MODE \
	CFG_OL_ENABLE_T2LM \
	CFG_OL_DEFAULT_T2LM_RESPONSE \
        CFG_OL_NON_T2LM_STA_ASSOCIATION \
	CFG_OL_MLOADVERT_DEGRADATION_ON_CAC \
	CFG_OL_RESPONDER_SECURE_LTF_SUPPORT \
	CFG_OL_RESPONDER_11AZ_SUPPORT \
	CFG_OL_RESPONDER_11MC_SUPPORT \
	CFG(CFG_OL_GET_SPL_VAP_WO_WIFI) \
	CFG_OL_ENABLE_EML_CAPABILITY \
	CFG(CFG_OL_HOST_TRIG_TXBF_SOUNDING) \
	CFG_OL_MANUAL_ULOFDMA_TRIG\
	CFG_OL_ENABLE_TARGET_RECOVERY_TRIG \
	CFG_OL_DISABLE_MULTI_RECOVERY_TRIG \
	CFG_OL_MSD_INFO_OVERRIDE_ENABLE \
	CFG_OL_MSD_INFO_PRESENT_OVERRIDE \
	CFG_OL_MSD_INFO_VALUE_OVERRIDE \
	CFG(CFG_OL_ML_PUMAC_SKIP) \
	CFG(CFG_OL_DISABLE_AUTO_TARGET_BRINGUP) \
	CFG(CFG_OL_ENABLE_RECOVERY_LOGS) \
    CFG(CFG_OL_COHOSTED_BSS_IND_ENABLE)\
    CFG(CFG_RF_SWITCH_CONFIG) \
	CFG(CFG_OL_NON_INTERSECTED_PHYMODE) \
	CFG(CFG_OL_WILDCARD_NDEV_NAME) \
	CFG(CFG_OL_BRIDGE_VAP_DISABLE) \
	CFG(CFG_DPDK_WIFI) \
	CFG(CFG_OL_MACREQ_EXT_ENABLE) \
	CFG(CFG_OL_NUM_IPA_RX_BUFF) \
	CFG(CFG_OL_NUM_IPA_TX_BUFF) \
	CFG_OL_AP_PWR_REDUCTION \
	CFG_OL_ETH_PORT_COUNT_THRESHOLD \
	CFG_OL_DBS_PWR_REDUCTION_VALUE \
	CFG_OL_ETH_PWR_REDUCTION_VALUE \
	CFG(CFG_OL_EXTENDED_REGINFO_SUPPORT) \
	CFG_OL_CFG80211_COMPLIANCE \
	CFG_OL_TX_PKT_CAP_PROTO \
	CFG_DP_PROTO_STATS \
	CFG_DP_EAPOL_STATS \
	CFG(CFG_OL_CFG_SCAN_WAR)
#endif /*_CFG_OL_H_*/
