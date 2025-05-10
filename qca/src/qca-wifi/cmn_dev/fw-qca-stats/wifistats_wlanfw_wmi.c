/*
 * Copyright (c) 2020-2023 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "wifistats.h"
#include <a_types.h>    /* A_UINT32 */
#include "wmi.h"
#include "wmi_unified.h"
#include "wmi_tlv_helper.h"
#include "wmi_tlv_defs.h"

#define WMI_MAX_STRING_LEN 1000
#define WMI_CMDS_SIZE_MAX 2048
#define WMI_BCAST_T2LM_MAX_STRING_LEN 1000

#ifdef __KERNEL__
#define WMI_STATS_PRINT DP_TRACE_STATS
#else
#define WMI_STATS_PRINT(level, fmt, ...) \
    do { \
        printf(fmt,##__VA_ARGS__); \
        printf("\n"); \
    } while (0)

void __attribute__ ((constructor)) wmistats_init(void);
void __attribute__ ((destructor)) wmistats_fini(void);

#endif /* ifdef __KERNEL__ */

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#else
#error confilicting defs of min
#endif


/*
 * Provide a forward declaration of wmistats.
 * (The definition is at the bottom of this file.)
 */
static struct wifistats_module wmistats;

/*
 * wmi_print_ctrl_path_pdev_tx_stats_tlv: display wmi_print_ctrl_path_pdev_tx_stats
 * @tag_buf: buffer containing the tlv wmi_ctrl_path_pdev_stats_struct_tlv
 *
 * return:void
 */
static inline void wmi_print_ctrl_path_pdev_tx_stats_tlv(void *tag_buf)
{
    wmi_ctrl_path_pdev_stats_struct *wmi_stats_buf =
        (wmi_ctrl_path_pdev_stats_struct *)tag_buf;
    A_UINT8  i;
    A_UINT16 index_tx                               = 0;
    A_UINT16 index_rx                               = 0;
    A_CHAR   fw_tx_mgmt_subtype[WMI_MAX_STRING_LEN] = {0};
    A_CHAR   fw_rx_mgmt_subtype[WMI_MAX_STRING_LEN] = {0};


    WMI_STATS_PRINT(FATAL, "WMI_CTRL_PATH_PDEV_TX_STATS_TLV:");

    WMI_STATS_PRINT(FATAL, "pdev_id = %u",
            wmi_stats_buf->pdev_id);

    for (i = 0; i < WMI_MGMT_FRAME_SUBTYPE_MAX; i++) {
        index_tx += snprintf(&fw_tx_mgmt_subtype[index_tx],
                WMI_MAX_STRING_LEN - index_tx,
                " %u:%u,", i,
                wmi_stats_buf->tx_mgmt_subtype[i]);
        index_rx += snprintf(&fw_rx_mgmt_subtype[index_rx],
                 WMI_MAX_STRING_LEN - index_rx,
                 " %u:%u,", i,
                 wmi_stats_buf->rx_mgmt_subtype[i]);
    }

    WMI_STATS_PRINT(FATAL, "fw_tx_mgmt_subtype = %s \n", fw_tx_mgmt_subtype);

    WMI_STATS_PRINT(FATAL, "fw_rx_mgmt_subtype = %s \n", fw_rx_mgmt_subtype);

    WMI_STATS_PRINT(FATAL, "scan_fail_dfs_violation_time_ms = %u",
            wmi_stats_buf->scan_fail_dfs_violation_time_ms);

    WMI_STATS_PRINT(FATAL, "nol_check_fail_time_stamp_ms = %u",
            wmi_stats_buf->nol_check_fail_time_stamp_ms);

    WMI_STATS_PRINT(FATAL, "nol_check_fail_last_chan_freq = %u",
            wmi_stats_buf->nol_check_fail_last_chan_freq);

    WMI_STATS_PRINT(FATAL, "nol_check_fail_time_stamp_ms = %u",
            wmi_stats_buf->nol_check_fail_time_stamp_ms);

    WMI_STATS_PRINT(FATAL, "total_peer_create_cnt = %u",
            wmi_stats_buf->total_peer_create_cnt);

    WMI_STATS_PRINT(FATAL, "total_peer_delete_cnt = %u",
            wmi_stats_buf->total_peer_delete_cnt);

    WMI_STATS_PRINT(FATAL, "total_peer_delete_resp_cnt = %u",
            wmi_stats_buf->total_peer_delete_resp_cnt);

    WMI_STATS_PRINT(FATAL, "vdev_pause_fail_rt_to_sched_algo_fifo_full_cnt = %u",
            wmi_stats_buf->vdev_pause_fail_rt_to_sched_algo_fifo_full_cnt);

    WMI_STATS_PRINT(FATAL, "qos_null_tx_over_wmi = %u",
            wmi_stats_buf->qos_null_tx_over_wmi);

    WMI_STATS_PRINT(FATAL, "qos_null_tx_send_compl_over_wmi = %u",
            wmi_stats_buf->qos_null_tx_send_compl_over_wmi);

    WMI_STATS_PRINT(FATAL, "qos_null_tx_send_event_alloc_failed = %u",
            wmi_stats_buf->qos_null_tx_send_event_alloc_failed);

    WMI_STATS_PRINT(FATAL,
        "\nWLAN_TARGET_RESOURCE_CONFIG_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "tx_chain_mask = %u",
        wmi_stats_buf->opaque_debug_tx_chain_mask);

    WMI_STATS_PRINT(FATAL, "rx_chain_mask = %u",
        wmi_stats_buf->opaque_debug_rx_chain_mask);

    WMI_STATS_PRINT(FATAL, "num_self_peers = %u",
        WMI_PDEV_STATS_NUM_SELF_PEERS_GET(
            wmi_stats_buf->opaque_debug_ema_flags));

    WMI_STATS_PRINT(FATAL, "eapol_ac_override = %u",
        WMI_PDEV_STATS_EAPOL_AC_OVERRIDE_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "max_active_vdevs = %u",
        WMI_PDEV_STATS_MAX_ACTIVE_VDEVS_GET(
            wmi_stats_buf->opaque_debug_ema_flags));

    WMI_STATS_PRINT(FATAL, "large_bcn_size = %u",
        wmi_stats_buf->opaque_debug_large_bcn_size);

    WMI_STATS_PRINT(FATAL, "ema_max_vap_count = %u",
        WMI_PDEV_STATS_EMA_MAX_VAP_CNT_GET(
            wmi_stats_buf->opaque_debug_ema_flags));

    WMI_STATS_PRINT(FATAL, "ema_max_profile_period = %u",
        WMI_PDEV_STATS_EMA_MAX_PROFILE_PERIOD_GET(
            wmi_stats_buf->opaque_debug_ema_flags));

    WMI_STATS_PRINT(FATAL, "calc_next_dtim_count = %u",
        WMI_PDEV_STATS_CALC_NEXT_DTIM_CNT_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "atf_strict_sch = %u",
        WMI_PDEV_STATS_ATF_STRICT_SCH_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "atf_config = %u",
        WMI_PDEV_STATS_ATF_CONFIG_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL,
        "\nWLAN_PDEV_MLO_CONFIG_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "num_ml_peer_entries = %u",
        wmi_stats_buf->opaque_debug_num_ml_peer_entries);

    WMI_STATS_PRINT(FATAL, "num_max_hw_links = %u",
        WMI_PDEV_STATS_NUM_MAX_HW_LINKS_GET(
            wmi_stats_buf->opaque_debug_mlo_flags));

    WMI_STATS_PRINT(FATAL, "current_chip_id = %u",
        WMI_PDEV_STATS_CURRENT_CHIP_ID_GET(
            wmi_stats_buf->opaque_debug_mlo_flags));

    WMI_STATS_PRINT(FATAL, "max_num_chips = %u",
        WMI_PDEV_STATS_MAX_NUM_CHIPS_GET(
            wmi_stats_buf->opaque_debug_mlo_flags));

    WMI_STATS_PRINT(FATAL, "is_mlo_supported = %u",
        WMI_PDEV_STATS_IS_MLO_SUPPORTED_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "num_of_linkview_msduqs_per_tid = %u",
        wmi_stats_buf->opaque_debug_num_of_linkview_msduqs_per_tid);

    WMI_STATS_PRINT(FATAL,
        "\nWLAN_PDEV_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "num_macs = %u",
        WMI_PDEV_STATS_NUM_MACS_GET(
            wmi_stats_buf->opaque_debug_num_macs_phy_vdev_up_active));

    WMI_STATS_PRINT(FATAL, "num_phy = %u",
        WMI_PDEV_STATS_NUM_PHY_GET(
            wmi_stats_buf->opaque_debug_num_macs_phy_vdev_up_active));

    WMI_STATS_PRINT(FATAL, "vdev_up_count = %u",
        WMI_PDEV_STATS_VDEV_UP_CNT_GET(
            wmi_stats_buf->opaque_debug_num_macs_phy_vdev_up_active));

    WMI_STATS_PRINT(FATAL, "vdev_active_count = %u",
        WMI_PDEV_STATS_VDEV_ACTIVE_CNT_GET(
            wmi_stats_buf->opaque_debug_num_macs_phy_vdev_up_active));

    WMI_STATS_PRINT(FATAL, "ic_flags = %u",
        wmi_stats_buf->opaque_debug_ic_flags);

    WMI_STATS_PRINT(FATAL, "pdev_suspended = %u",
        WMI_PDEV_STATS_SUSPENDED_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "paused_ap_vdev_bitmap = %u",
        wmi_stats_buf->opaque_debug_paused_ap_vdev_bitmap);

    WMI_STATS_PRINT(FATAL, "pdev_flags = %u",
        wmi_stats_buf->opaque_debug_flags);

    WMI_STATS_PRINT(FATAL, "remote_peer_count = %u",
        WMI_PDEV_STATS_REMOTE_PEER_CNT_GET(
            wmi_stats_buf->opaque_debug_remote_peer_cnt_max_rf_chains_2G_5G));

    WMI_STATS_PRINT(FATAL, "max_rf_chains_2G = %u",
        WMI_PDEV_STATS_MAX_RF_CHAIN_2G_GET(
            wmi_stats_buf->opaque_debug_remote_peer_cnt_max_rf_chains_2G_5G));

    WMI_STATS_PRINT(FATAL, "max_rf_chains_5G = %u",
        WMI_PDEV_STATS_MAX_RF_CHAIN_5G_GET(
            wmi_stats_buf->opaque_debug_remote_peer_cnt_max_rf_chains_2G_5G));

    WMI_STATS_PRINT(FATAL, "max_ht_cap_info = 0x%x",
        wmi_stats_buf->opaque_debug_max_ht_cap_info);

    WMI_STATS_PRINT(FATAL, "max_vht_cap_info = 0x%x",
        wmi_stats_buf->opaque_debug_max_vht_cap_info);

    WMI_STATS_PRINT(FATAL, "max_vht_supp_mcs = 0x%x",
        wmi_stats_buf->opaque_debug_max_vht_supp_mcs);

    WMI_STATS_PRINT(FATAL, "max_he_cap_info = 0x%x",
        wmi_stats_buf->opaque_debug_max_he_cap_info);

    WMI_STATS_PRINT(FATAL, "max_he_cap_info_ext = 0x%x",
        wmi_stats_buf->opaque_debug_max_he_cap_info_ext);

    WMI_STATS_PRINT(FATAL,
        "\nWLAN_PDEV_GREEN_AP_CONFIG_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "green_ap_Tx_chain_mask_valid = %u",
        WMI_PDEV_STATS_GAP_TX_CH_MASK_VALID_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "green_ap_RX_chain_mask_valid = %u",
        WMI_PDEV_STATS_GAP_RX_CH_MASK_VALID_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "green_ap_phy_mode_valid = %u",
        WMI_PDEV_STATS_GAP_PHY_MODE_VALID_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "green_ap_clkgate_valid = %u",
        WMI_PDEV_STATS_GAP_CLKGATE_VALID_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "green_ap_tx_chainmask = %u",
        WMI_PDEV_STATS_GAP_TX_CH_MASK_GET(
            wmi_stats_buf->opaque_debug_rts_rc_flag));

    WMI_STATS_PRINT(FATAL, "green_ap_rx_chainmask = %u",
        WMI_PDEV_STATS_GAP_TX_CH_MASK_GET(
            wmi_stats_buf->opaque_debug_rts_rc_flag));

    WMI_STATS_PRINT(FATAL, "green_ap_phy_mode = %u",
        WMI_PDEV_STATS_GAP_PHY_MODE_GET(
            wmi_stats_buf->opaque_debug_gap_phy_mode_freq));

    WMI_STATS_PRINT(FATAL, "green_ap_band_center_freq1 = %u",
        WMI_PDEV_STATS_GAP_BAND_CENTER_FREQ1_GET(
            wmi_stats_buf->opaque_debug_gap_phy_mode_freq));

    WMI_STATS_PRINT(FATAL,
        "\nWLAN_PDEV_BCN_FILTER_OFFLOAD_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "bcnsdropped = %u",
        wmi_stats_buf->opaque_debug_bcns_dropped);

    WMI_STATS_PRINT(FATAL, "bcnsrecvd = %u",
        wmi_stats_buf->opaque_debug_bcns_recvd);

    WMI_STATS_PRINT(FATAL, "bcnsdelivered = %u",
        wmi_stats_buf->opaque_debug_bcns_delivered);

    WMI_STATS_PRINT(FATAL,
        "\nWAL_PDEV_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "max_vdevs = %u",
        wmi_stats_buf->opaque_debug_max_vdevs);

    WMI_STATS_PRINT(FATAL, "total_active_vdev_cnt = %u",
        wmi_stats_buf->opaque_debug_total_active_vdev_cnt);

    WMI_STATS_PRINT(FATAL, "num_of_fils_disc_enqd = %u",
        WMI_PDEV_STATS_NUM_FILS_DISC_ENQD_GET(
            wmi_stats_buf->opaque_debug_burst_mode_pending_isr));

    WMI_STATS_PRINT(FATAL, "vdev_all_tid_pause_bitmap = %u",
        wmi_stats_buf->opaque_debug_vdev_all_tid_pause_bitmap);

    WMI_STATS_PRINT(FATAL, "vdev_all_tid_block_bitmap = %u",
        wmi_stats_buf->opaque_debug_vdev_all_tid_block_bitmap);

    WMI_STATS_PRINT(FATAL, "rx_filter =%u",
        wmi_stats_buf->opaque_debug_rx_filter);

    WMI_STATS_PRINT(FATAL, "pdev_paused = %u",
        WMI_PDEV_STATS_PAUSED_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "cac_enabled = %u",
        WMI_PDEV_STATS_CAC_ENABLED_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "mac_cold_reset = %u",
        WMI_PDEV_STATS_MAC_COLD_RESET_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "safe_to_access_hw = %u",
        WMI_PDEV_STATS_SAFE_TO_ACCESS_HW_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "sta_ps_statechg_enable = %u",
        WMI_PDEV_STATS_STA_PS_STATECHG_ENABLE_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "wal_pdev_host_scan_in_progress = %u",
        WMI_PDEV_STATS_WAL_HOST_SCAN_IN_PROGRESS_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "agg_retry_th = %u",
        WMI_PDEV_STATS_AGG_RETRY_TH_GET(
            wmi_stats_buf->opaque_debug_aggr_nonaggr_retry_th));

    WMI_STATS_PRINT(FATAL, "non_agg_retry_th = %u",
        WMI_PDEV_STATS_NON_AGG_RETRY_TH_GET(
            wmi_stats_buf->opaque_debug_aggr_nonaggr_retry_th));

    WMI_STATS_PRINT(FATAL, "max_non_data_retry_th = %u",
        WMI_PDEV_STATS_MAX_NON_DATA_RETRY_TH_GET(
            wmi_stats_buf->opaque_debug_aggr_nonaggr_retry_th));

    WMI_STATS_PRINT(FATAL, "dynamic_bw = %u",
        WMI_PDEV_STATS_DYN_BW_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "is_monitor_type_present = %u",
        WMI_PDEV_STATS_IS_MONITOR_TYPE_PRESENT_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "bcn_tx_mode = %u",
        WMI_PDEV_STATS_BCN_TX_MODE_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "num_rx_ba_sessions = %u",
        WMI_PDEV_STATS_NUM_RX_BA_SESSIONS_GET(
            wmi_stats_buf->opaque_debug_num_max_rx_ba_sessions));

    WMI_STATS_PRINT(FATAL, "max_rx_ba_sessions = %u",
        WMI_PDEV_STATS_MAX_RX_BA_SESSIONS_GET(
            wmi_stats_buf->opaque_debug_num_max_rx_ba_sessions));

    WMI_STATS_PRINT(FATAL, "chan_switch_flags = %u",
        wmi_stats_buf->opaque_debug_chan_switch_flags);

    WMI_STATS_PRINT(FATAL, "consecutive_failure = %u",
        WMI_PDEV_STATS_CONSECUTIVE_FAILURE_GET(
            wmi_stats_buf->opaque_debug_consecutive_failure_reset_cause));

    WMI_STATS_PRINT(FATAL, "reset_cause_bitmap = 0x%x",
        WMI_PDEV_STATS_RESET_CAUSE_BITMAP_GET(
            wmi_stats_buf->opaque_debug_consecutive_failure_reset_cause));

    WMI_STATS_PRINT(FATAL, "ppdu_dur_limit_us = %u",
        WMI_PDEV_STATS_PPDU_DUR_LIMIT_US_GET(
            wmi_stats_buf->opaque_debug_mu_ppdu_dur_limit_us));

    WMI_STATS_PRINT(FATAL, "mu_ppdu_dur_limit_us = %u",
        WMI_PDEV_STATS_MU_PPDU_DUR_LIMIT_US_GET(
            wmi_stats_buf->opaque_debug_mu_ppdu_dur_limit_us));

    WMI_STATS_PRINT(FATAL, "reset_in_progress = %u",
        wmi_stats_buf->opaque_debug_reset_in_progress);

    WMI_STATS_PRINT(FATAL, "vdev_migrate_state = %u",
        wmi_stats_buf->opaque_debug_vdev_migrate_state);

    WMI_STATS_PRINT(FATAL, "rts_rc_flag = %u",
        WMI_PDEV_STATS_RTS_RC_FLAGS_GET(
            wmi_stats_buf->opaque_debug_rts_rc_flag));

    WMI_STATS_PRINT(FATAL, "rts_rc = %u",
        WMI_PDEV_STATS_RTS_RC_GET(
            wmi_stats_buf->opaque_debug_rts_rc_flag));

    WMI_STATS_PRINT(FATAL, "num_home_chans = %u",
        WMI_PDEV_STATS_NUM_HOME_CHANS_GET(
            wmi_stats_buf->opaque_debug_mlo_flags));

    WMI_STATS_PRINT(FATAL, "num_cosecutive_beacon_tx_filt = %u",
        WMI_PDEV_STATS_NUM_CONSECUTIVE_BCN_TX_FILT_GET(
            wmi_stats_buf->opaque_debug_aggr_nonaggr_retry_th));

    WMI_STATS_PRINT(FATAL, "num_of_peer_delete_in_progress = %u",
        wmi_stats_buf->opaque_debug_num_of_peer_delete_in_progress);

    WMI_STATS_PRINT(FATAL, "dyn_ppdu_dur = %u",
        wmi_stats_buf->opaque_debug_dyn_ppdu_dur);

    WMI_STATS_PRINT(FATAL,
        "\nWAL_PDEV_DFS_NOL_CTXT_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "dfs_nol_count = %u",
        wmi_stats_buf->opaque_debug_dfs_nol_count);

    WMI_STATS_PRINT(FATAL, "dfs_nol_timeout = %u",
        wmi_stats_buf->opaque_debug_dfs_nol_timeout);

    WMI_STATS_PRINT(FATAL, "dfs_use_nol = %u",
        wmi_stats_buf->opaque_debug_dfs_use_nol);

    WMI_STATS_PRINT(FATAL, "isTXsuspended = %u",
        WMI_PDEV_STATS_IS_TXSUSPENDED_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "isSCHEDsuspended = %u",
        WMI_PDEV_STATS_IS_SCHEDSUSPENDED_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "cac_mode = %u",
        wmi_stats_buf->opaque_debug_cac_mode);

    WMI_STATS_PRINT(FATAL,
        "\nWAL_PDEV_HOME_CHANNEL_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "home_chan_mhz = %u",
        WMI_PDEV_STATS_HOME_CHAN_MHZ_GET(
            wmi_stats_buf->opaque_debug_home_chan_mhz_flags));

    WMI_STATS_PRINT(FATAL, "home_band_center_freq1 = %u",
        WMI_PDEV_STATS_HOME_CHAN_BAND_FREQ_1_GET(
            wmi_stats_buf->opaque_debug_home_band_center_freq));

    WMI_STATS_PRINT(FATAL, "home_band_center_freq2 = %u",
        WMI_PDEV_STATS_HOME_CHAN_BAND_FREQ_2_GET(
            wmi_stats_buf->opaque_debug_home_band_center_freq));

    WMI_STATS_PRINT(FATAL, "home_phy_mode = %u",
        wmi_stats_buf->opaque_debug_home_phy_mode);

    WMI_STATS_PRINT(FATAL, "home_chan_flags = 0x%x",
        WMI_PDEV_STATS_HOME_CHAN_FLAGS_GET(
            wmi_stats_buf->opaque_debug_home_chan_mhz_flags));

    WMI_STATS_PRINT(FATAL,
        "\nWAL_PDEV_CURRENT_CHANNEL_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "current_chan_mhz = %u",
        WMI_PDEV_STATS_CUR_CHAN_MHZ_GET(
            wmi_stats_buf->opaque_debug_cur_chan_mhz_flags));

    WMI_STATS_PRINT(FATAL, "current_band_center_freq1 = %u",
        WMI_PDEV_STATS_CUR_CHAN_BAND_FREQ_1_GET(
            wmi_stats_buf->opaque_debug_cur_band_center_freq));

    WMI_STATS_PRINT(FATAL, "current_band_center_freq2 = %u",
        WMI_PDEV_STATS_CUR_CHAN_BAND_FREQ_2_GET(
            wmi_stats_buf->opaque_debug_cur_band_center_freq));

    WMI_STATS_PRINT(FATAL, "current_phy_mode = %u",
        wmi_stats_buf->opaque_debug_cur_phy_mode);

    WMI_STATS_PRINT(FATAL, "current_chan_flags = 0x%x",
        WMI_PDEV_STATS_CUR_CHAN_FLAGS_GET(
            wmi_stats_buf->opaque_debug_cur_chan_mhz_flags));

    WMI_STATS_PRINT(FATAL,
        "\nWAL_PDEV_BEACON_CTXT_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "bcn_q_num = %u",
        WMI_PDEV_STATS_BCN_Q_NUM_GET(
            wmi_stats_buf->opaque_debug_bcn_q_num_bcns_queued_to_hw));

    WMI_STATS_PRINT(FATAL, "num_bcns_queued_to_hw = %u",
        WMI_PDEV_STATS_NUM_BCNS_QUEUED_TO_HW_GET(
            wmi_stats_buf->opaque_debug_bcn_q_num_bcns_queued_to_hw));

    WMI_STATS_PRINT(FATAL, "aifs = %u",
        wmi_stats_buf->opaque_debug_aifs);

    WMI_STATS_PRINT(FATAL, "cwmin = %u",
        wmi_stats_buf->opaque_debug_cwmin);

    WMI_STATS_PRINT(FATAL, "cwmax = %u",
        wmi_stats_buf->opaque_debug_cwmax);

    WMI_STATS_PRINT(FATAL,
        "\nWAL_PDEV_SWFDA_CTXT_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "swfda_vdev_id = %u",
        WMI_PDEV_STATS_SWFDA_VDEV_ID_GET(
            wmi_stats_buf->opaque_debug_bcn_q_num_bcns_queued_to_hw));

    WMI_STATS_PRINT(FATAL, "fils_period = %u",
        wmi_stats_buf->opaque_debug_fils_period);

    WMI_STATS_PRINT(FATAL,
        "\nWAL_PDEV_SWBA_CTXT_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "beacon_period = %u",
        wmi_stats_buf->opaque_debug_beacon_period);

    WMI_STATS_PRINT(FATAL, "staggered_beacon_intvl = %u",
        wmi_stats_buf->opaque_debug_staggered_beacon_intvl);

    WMI_STATS_PRINT(FATAL, "swba_num_of_vdev = %u",
        WMI_PDEV_STATS_SWBA_NUM_OF_VDEVS_GET(
            wmi_stats_buf->opaque_debug_consecutive_failure_reset_cause));

    WMI_STATS_PRINT(FATAL,
        "\nWAL_PDEV_TX_CTXT_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "tx_ctxt_flags = %u",
        wmi_stats_buf->opaque_debug_tx_ctxt_flags);

    WMI_STATS_PRINT(FATAL, "pending_isr_status = %u",
        WMI_PDEV_STATS_PENDING_ISR_STATUS_GET(
            wmi_stats_buf->opaque_debug_burst_mode_pending_isr));

    WMI_STATS_PRINT(FATAL, "burst_mode = %u",
        WMI_PDEV_STATS_BURST_MODE_GET(
            wmi_stats_buf->opaque_debug_burst_mode_pending_isr));

    WMI_STATS_PRINT(FATAL, "burst_enable = %u",
        WMI_PDEV_STATS_BURST_ENABLE_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "burst_dur = %u",
        wmi_stats_buf->opaque_debug_burst_dur);

    WMI_STATS_PRINT(FATAL, "tx_hw_stuck_cnt = %u",
        wmi_stats_buf->opaque_debug_tx_hw_stuck_cnt);

    WMI_STATS_PRINT(FATAL, "consecutive_lifetime_expiries = %u",
        wmi_stats_buf->opaque_debug_consecutive_lifetime_expiries);

    WMI_STATS_PRINT(FATAL, "sw_retry_mpdu_count_th = %u",
        WMI_PDEV_STATS_SW_RETRY_MPDU_COUNT_TH_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "tx_abort_reason = %u",
        WMI_PDEV_STATS_ABORT_REASON_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "sendbar_compl_in_progress = %u",
        WMI_PDEV_STATS_SENDBAR_COMPL_IN_PROGRESS_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "is_TXsuspended_with_afc = %u",
        WMI_PDEV_STATS_IS_TXSUSPENDED_WITH_AFC_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "is_SCHEDsuspended_with_afc = %u",
        WMI_PDEV_STATS_IS_SCHEDSUSPENDED_WITH_AFC_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "abort_result = %u",
        WMI_PDEV_STATS_ABORT_RESULT_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL, "sched_algo_resume_needed = %u",
        WMI_PDEV_STATS_SCHED_ALGO_RESUME_NEEDED_GET(
            wmi_stats_buf->opaque_debug_wal_pdev_bitfield));

    WMI_STATS_PRINT(FATAL,
        "\nWAL_PDEV_RX_CTXT_PARAMS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "rx_ctxt_flags = %u",
        wmi_stats_buf->opaque_debug_rx_ctxt_flags);

    WMI_STATS_PRINT(FATAL, "rx_suspend_cnt = %u",
        wmi_stats_buf->opaque_debug_rx_suspend_cnt);

    WMI_STATS_PRINT(FATAL, "rx_resume_cnt = %u",
        wmi_stats_buf->opaque_debug_rx_resume_cnt);

    WMI_STATS_PRINT(FATAL, "rx_pcie_suspend_cnt = %u",
        wmi_stats_buf->opaque_debug_rx_pcie_suspend_cnt);

    WMI_STATS_PRINT(FATAL, "rx_pcie_resume_cnt = %u",
        wmi_stats_buf->opaque_debug_rx_pcie_resume_cnt);

    WMI_STATS_PRINT(FATAL,
        "\nRESERVED_FIELDS: (pdev_id:%u)",
        wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "reserved_1 0x%x",
        wmi_stats_buf->opaque_debug_reserved_field_1);

    WMI_STATS_PRINT(FATAL, "reserved_2 0x%x",
        wmi_stats_buf->opaque_debug_reserved_field_2);

    WMI_STATS_PRINT(FATAL, "reserved_3 0x%x",
        wmi_stats_buf->opaque_debug_reserved_field_3);

    WMI_STATS_PRINT(FATAL, "reserved_4 0x%x",
        wmi_stats_buf->opaque_debug_reserved_field_4);

    WMI_STATS_PRINT(FATAL, "reserved_5 0x%x",
        wmi_stats_buf->opaque_debug_reserved_field_5);
}

static inline void wmi_print_ctrl_path_vdev_bcn_tx_stats_tlv(void *tag_buf)
{
    wmi_ctrl_path_vdev_bcn_tx_stats_struct *wmi_stats_buf =
        (wmi_ctrl_path_vdev_bcn_tx_stats_struct*) tag_buf;
    WMI_STATS_PRINT(FATAL,
        "WMI_CTRL_PATH_VDEV_BCN_TX_STATS_TLV: (vdev_id:%d)",
        wmi_stats_buf->vdev_id);
    WMI_STATS_PRINT(FATAL,
        "beacon_succ_cnt:%d", wmi_stats_buf->beacon_succ_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_outage_cnt:%d", wmi_stats_buf->beacon_outage_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_cons_bmiss_cnt:%d", wmi_stats_buf->beacon_cons_miss_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_curr_bmiss_bcnt:%d", wmi_stats_buf->beacon_curr_miss_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_sw_abort_cnt:%d", wmi_stats_buf->beacon_sw_abort_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_tx_filter_cnt:%d", wmi_stats_buf->beacon_tx_filter_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_tx_xretry_cnt:%d", wmi_stats_buf->beacon_tx_xretry_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_tx_drop_cnt:%d", wmi_stats_buf->beacon_tx_drop_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_tx_abort_cnt:%d", wmi_stats_buf->beacon_tx_abort_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_tx_tid_del_cnt:%d", wmi_stats_buf->beacon_tx_tid_del_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_tx_mig_drop_cnt:%d", wmi_stats_buf->beacon_tx_mig_drop_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_fail_others_cnt:%d", wmi_stats_buf->beacon_fail_others_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_stuck_cnt:%d", wmi_stats_buf->beacon_stuck_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_tbtt_skip_cnt:%d", wmi_stats_buf->beacon_tbtt_skip_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_swba_cnt:%d", wmi_stats_buf->beacon_swba_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_enque_fail:%d", wmi_stats_buf->beacon_enque_fail);
    WMI_STATS_PRINT(FATAL,
        "beacon_miss_systime_gap:%u",
        wmi_stats_buf->beacon_miss_systime_gap_us);
    WMI_STATS_PRINT(FATAL,
        "beacon_last_miss_time:%llu",
        wmi_stats_buf->beacon_last_miss_time_low_us + (((long long unsigned)
        wmi_stats_buf->beacon_last_miss_time_high_us) << 32));
    WMI_STATS_PRINT(FATAL,
        "last_tbtt_time:%llu",
        wmi_stats_buf->last_tbtt_time_low_us + (((long long unsigned)
        wmi_stats_buf->last_tbtt_time_high_us) << 32));
    WMI_STATS_PRINT(FATAL,
        "last_tbtt_update:%llu",
        wmi_stats_buf->last_tbtt_update_low_us + (((long long unsigned)
        wmi_stats_buf->last_tbtt_update_high_us) << 32));
    WMI_STATS_PRINT(FATAL,
        "scan_time_exceed_cnt:%d", wmi_stats_buf->scan_time_exceed_cnt);
}

static inline void wmi_print_ctrl_path_pdev_bcn_tx_stats_tlv(void *tag_buf)
{
    wmi_ctrl_path_pdev_bcn_tx_stats_struct *wmi_stats_buf =
        (wmi_ctrl_path_pdev_bcn_tx_stats_struct*) tag_buf;
    WMI_STATS_PRINT(FATAL,
        "WMI_CTRL_PATH_PDEV_BCN_TX_STATS_TLV: (pdev_id:%d)",
        wmi_stats_buf->pdev_id);
    WMI_STATS_PRINT(FATAL,
        "beacon_succ_cnt:%d", wmi_stats_buf->beacon_succ_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_outage_cnt:%d", wmi_stats_buf->beacon_outage_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_sw_abort_cnt:%d", wmi_stats_buf->beacon_sw_abort_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_tx_filter_cnt:%d", wmi_stats_buf->beacon_tx_filter_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_tx_xretry_cnt:%d", wmi_stats_buf->beacon_tx_xretry_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_tx_drop_cnt:%d", wmi_stats_buf->beacon_tx_drop_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_tx_abort_cnt:%d", wmi_stats_buf->beacon_tx_abort_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_tx_tid_del_cnt:%d", wmi_stats_buf->beacon_tx_tid_del_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_tx_mig_drop_cnt:%d", wmi_stats_buf->beacon_tx_mig_drop_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_fail_others_cnt:%d", wmi_stats_buf->beacon_fail_others_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_stuck_cnt:%d", wmi_stats_buf->beacon_stuck_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_swba_cnt:%d", wmi_stats_buf->beacon_swba_cnt);
    WMI_STATS_PRINT(FATAL,
        "beacon_enque_fail:%d", wmi_stats_buf->beacon_enque_fail);
}

static inline void wmi_print_ctrl_path_wmi_vdev_stats_tlv(void *tag_buf)
{
    wmi_ctrl_path_vdev_stats_struct *wmi_stats_buf = (wmi_ctrl_path_vdev_stats_struct*) tag_buf;
    WMI_STATS_PRINT(FATAL,
        "WMI_CTRL_PATH_VDEV_STATS_TLV: (vdev_id:%d)", wmi_stats_buf->vdev_id);

    WMI_STATS_PRINT(FATAL,
        "\nVDEV CONFIG STATS INFO: (vdev_id:%d)", wmi_stats_buf->vdev_id);

    WMI_STATS_PRINT(FATAL, "flags = 0x%x",
        wmi_stats_buf->opaque_debug_wal_vdev_flags);
    WMI_STATS_PRINT(FATAL, "vdev_id = 0x%x",
        wmi_stats_buf->vdev_id);
    WMI_STATS_PRINT(FATAL, "ifUp = %u",
        WMI_VDEV_STATS_IFUP_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "ifActive = %u",
        WMI_VDEV_STATS_IFACTIVE_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "ifPaused = %u",
        WMI_VDEV_STATS_IFPAUSED_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "ifOutOfSync = %u",
        WMI_VDEV_STATS_IFOUTOFSYNC_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "is_free = %u",
        WMI_VDEV_STATS_IS_FREE_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "is_nawds = %u",
        WMI_VDEV_STATS_IS_NAWDS_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "ic_opmode = %u",
        WMI_VDEV_STATS_IC_OPMODE_GET(
            wmi_stats_buf->opaque_debug_vdev_mode_configs));
    WMI_STATS_PRINT(FATAL, "ic_subopmode = %u",
        WMI_VDEV_STATS_IC_SUBOPMODE_GET(
            wmi_stats_buf->opaque_debug_vdev_mode_configs));
    WMI_STATS_PRINT(FATAL, "ic_curmode = %u",
        WMI_VDEV_STATS_IC_CURMODE_GET(
            wmi_stats_buf->opaque_debug_vdev_mode_configs));
    WMI_STATS_PRINT(FATAL, "vdev_up_cmd_cnt = %u",
        WMI_VDEV_STATS_VDEV_UP_CMD_CNT_GET(
            wmi_stats_buf->opaque_debug_vdev_mode_configs));
    WMI_STATS_PRINT(FATAL, "vdev_stats_id = %u",
        WMI_VDEV_STATS_ID_GET(
            wmi_stats_buf->opaque_debug_vdev_stats_id_configs));
    WMI_STATS_PRINT(FATAL, "vdev_stats_id_valid = %u",
        WMI_VDEV_STATS_ID_VALID_GET(
            wmi_stats_buf->opaque_debug_vdev_stats_id_configs));
    WMI_STATS_PRINT(FATAL, "group_cipher = %u",
        WMI_VDEV_STATS_GROUP_CIPHER_GET(
            wmi_stats_buf->opaque_debug_vdev_assoc_peer_configs));
    WMI_STATS_PRINT(FATAL, "assoc_id = %u",
        WMI_VDEV_STATS_ASSOC_ID_GET(
            wmi_stats_buf->opaque_debug_vdev_assoc_peer_configs));
    WMI_STATS_PRINT(FATAL, "bss_channel_mhz = %u",
        WMI_VDEV_STATS_BSS_CHANNEL_MHZ_GET(
            wmi_stats_buf->opaque_debug_vdev_mhz_fils_configs));
    WMI_STATS_PRINT(FATAL, "off_ch_active_dwell_time = %u",
        WMI_VDEV_STATS_OFF_CH_ACTIVE_DWELL_TIME_GET(
            wmi_stats_buf->opaque_debug_vdev_chan_configs));
    WMI_STATS_PRINT(FATAL, "off_ch_passive_dwell_time = %u",
        WMI_VDEV_STATS_OFF_CH_PASSIVE_DWELL_TIME_GET(
            wmi_stats_buf->opaque_debug_vdev_dwell_configs));
    WMI_STATS_PRINT(FATAL, "current_pause_request_id = %u",
        WMI_VDEV_STATS_CURRENT_PAUSE_REQUEST_ID_GET(
            wmi_stats_buf->opaque_debug_vdev_dwell_configs));
    WMI_STATS_PRINT(FATAL, "hide_ssid_enable = %u",
        WMI_VDEV_STATS_HIDE_SSID_ENABLE_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "b_none_protocol_paused = %u",
        WMI_VDEV_STATS_B_NONE_PROTOCOL_PAUSED_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "dpd_cal_state = %u",
        WMI_VDEV_STATS_DPD_CAL_STATE_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "is_vdev_stopping = %u",
        WMI_VDEV_STATS_IS_VDEV_STOPPING_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "is_wmi_vdev_down = %u",
        WMI_VDEV_STATS_IS_WMI_VDEV_DOWN_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "is_vdev_down_pending = %u",
        WMI_VDEV_STATS_IS_VDEV_DOWN_PENDING_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "vdev_delete_in_progress = %u",
        WMI_VDEV_STATS_VDEV_DELETE_IN_PROGRESS_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "cac_enabled = %u",
        WMI_VDEV_STATS_CAC_ENABLED_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "is_quaterrate = %u",
        WMI_VDEV_STATS_IS_QUATERRATE_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "is_halfrate = %u",
        WMI_VDEV_STATS_IS_HALFRATE_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "stop_resp_event_blocked = %u",
        WMI_VDEV_STATS_STOP_RESP_EVENT_BLOCKED_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "use_enhanced_mcast_filter = %u",
        WMI_VDEV_STATS_USE_ENHANCED_MCAST_FILTER_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "is_start_pending_on_asm = %u",
        WMI_VDEV_STATS_IS_START_PENDING_ON_ASM_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "no_null_to_ap_for_roaming = %u",
        WMI_VDEV_STATS_NO_NULL_TO_AP_FOR_ROAMING_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "is_loopback_cal_pending = %u",
        WMI_VDEV_STATS_IS_LOOPBACK_CAL_PENDING_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "vdev_delete_acked = %u",
        WMI_VDEV_STATS_VDEV_DELETE_ACKED_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "bc_proberesp_enable = %u",
        WMI_VDEV_STATS_BC_PROBERESP_ENABLE_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "hw_flag = 0x%x",
        WMI_VDEV_STATS_HW_FLAG_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "ch_req_flag = 0x%x",
        WMI_VDEV_STATS_CH_REQ_FLAG_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "restart_resp = %u",
        WMI_VDEV_STATS_RESTART_RESP_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "erpEnabled = %u",
        WMI_VDEV_STATS_ERPENABLED_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "start_responded = %u",
        WMI_VDEV_STATS_START_RESPONDED_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "vdev_down_cmd_cnt = %u",
        WMI_VDEV_STATS_VDEV_DOWN_CMD_CNT_GET(
            wmi_stats_buf->opaque_debug_vdev_bcn_drift_info));

    WMI_STATS_PRINT(FATAL,
        "\nVDEV BEACON STATS INFO: (vdev_id:%d)", wmi_stats_buf->vdev_id);

    WMI_STATS_PRINT(FATAL, "bcn_intval_us = %u",
        wmi_stats_buf->opaque_debug_bcn_intval_us);
    WMI_STATS_PRINT(FATAL, "fils_period = %u",
        wmi_stats_buf->opaque_debug_fils_period);
    WMI_STATS_PRINT(FATAL, "prev_tsf = %u",
        wmi_stats_buf->opaque_debug_prev_tsf);
    WMI_STATS_PRINT(FATAL, "sleep_entry_time = %u",
        wmi_stats_buf->opaque_debug_sleep_entry_time);
    WMI_STATS_PRINT(FATAL, "tot_sleep_dur = %u",
        wmi_stats_buf->opaque_debug_tot_sleep_dur);
    WMI_STATS_PRINT(FATAL, "tbtt_offset = %u",
        wmi_stats_buf->opaque_debug_tbtt_offset);
    WMI_STATS_PRINT(FATAL, "last_send_to_host_deauth_tsf = %u",
        wmi_stats_buf->opaque_debug_last_send_to_host_deauth_tsf);
    WMI_STATS_PRINT(FATAL, "short_ssid = %u",
        wmi_stats_buf->opaque_debug_short_ssid);
    WMI_STATS_PRINT(FATAL, "bcn_max_slot = %u",
        WMI_VDEV_STATS_BCN_MAX_SLOT_GET(
            wmi_stats_buf->opaque_debug_vdev_cap_slot_bitfield));
    WMI_STATS_PRINT(FATAL, "bcn_curr_slot = %u",
        WMI_VDEV_STATS_BCN_CURR_SLOT_GET(
            wmi_stats_buf->opaque_debug_vdev_cap_slot_bitfield));
    WMI_STATS_PRINT(FATAL, "mgmt_tx_power = %u",
        WMI_VDEV_STATS_MGMT_TX_POWER_GET(
            wmi_stats_buf->opaque_debug_vdev_cap_slot_bitfield));
    WMI_STATS_PRINT(FATAL, "consec_beacon_skip = %u",
        WMI_VDEV_STATS_CONSEC_BEACON_SKIP_GET(
            wmi_stats_buf->opaque_debug_vdev_bcn_configs));
    WMI_STATS_PRINT(FATAL, "consec_beacon_skip_cnt = %u",
        WMI_VDEV_STATS_CONSEC_BEACON_SKIP_CNT_GET(
            wmi_stats_buf->opaque_debug_vdev_bcn_configs));
    WMI_STATS_PRINT(FATAL, "max_consec_beacon_skip = %u",
        WMI_VDEV_STATS_MAX_CONSEC_BEACON_SKIP_GET(
            wmi_stats_buf->opaque_debug_vdev_bcn_configs));
    WMI_STATS_PRINT(FATAL, "bcn_drift_cnt = %u",
        WMI_VDEV_STATS_BCN_DRIFT_CNT_GET(
            wmi_stats_buf->opaque_debug_vdev_bcn_drift_info));
    WMI_STATS_PRINT(FATAL, "bcn_drift_calibration = %u",
        WMI_VDEV_STATS_BCN_DRIFT_CALIBRATION_GET(
            wmi_stats_buf->opaque_debug_vdev_bcn_drift_info));
    WMI_STATS_PRINT(FATAL, "ap_detect_out_of_sync_sleeping_sta_time_secs = %u",
        WMI_VDEV_STATS_AP_DETECT_OUT_OF_SYNC_SLEEPING_STA_TIME_SECS_GET(
            wmi_stats_buf->opaque_debug_vdev_keepalive_bitfields));
    WMI_STATS_PRINT(FATAL, "rts_cts_default = %u",
        WMI_VDEV_STATS_RTS_CTS_DEFAULT_GET(
            wmi_stats_buf->opaque_debug_vdev_bcn_drift_info));
    WMI_STATS_PRINT(FATAL, "tbtt_link_type = %u",
        WMI_VDEV_STATS_TBTT_LINK_TYPE_GET(
            wmi_stats_buf->opaque_debug_vdev_arp_configs));
    WMI_STATS_PRINT(FATAL, "is_arp_in_air = %u",
        WMI_VDEV_STATS_IS_ARP_IN_AIR_GET(
            wmi_stats_buf->opaque_debug_vdev_arp_configs));
    WMI_STATS_PRINT(FATAL, "is_ns_in_air = %u",
        WMI_VDEV_STATS_IS_NS_IN_AIR_GET(
            wmi_stats_buf->opaque_debug_vdev_arp_configs));
    WMI_STATS_PRINT(FATAL, "n_beacons_since_last_rssi_report = %u",
        WMI_VDEV_STATS_N_BEACONS_SINCE_LAST_RSSI_REPORT_GET(
            wmi_stats_buf->opaque_debug_vdev_streams_configs));
    WMI_STATS_PRINT(FATAL, "num_ofld_peer_alloced = %u",
        WMI_VDEV_STATS_NUM_OFLD_PEER_ALLOCED_GET(
            wmi_stats_buf->opaque_debug_vdev_streams_configs));
    WMI_STATS_PRINT(FATAL, "config_fils_period = %u",
        WMI_VDEV_STATS_CONFIG_FILS_PERIOD_GET(
            wmi_stats_buf->opaque_debug_vdev_mhz_fils_configs));
    WMI_STATS_PRINT(FATAL, "calc_fils_period = %u",
        WMI_VDEV_STATS_CALC_FILS_PERIOD_GET(
            wmi_stats_buf->opaque_debug_vdev_fils_period));
    WMI_STATS_PRINT(FATAL, "req_bcn_q_unpause = %u",
        WMI_VDEV_STATS_REQ_BCN_Q_UNPAUSE_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "bt_coex_enable_cts2s = %u",
        WMI_VDEV_STATS_BT_COEX_ENABLE_CTS2S_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "dpd_delay_n_beacon = %u",
        WMI_VDEV_STATS_DPD_DELAY_N_BEACON_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "b_need_check_first_beacon = %u",
        WMI_VDEV_STATS_B_NEED_CHECK_FIRST_BEACON_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "bcn_sync_crit_req_act = %u",
        WMI_VDEV_STATS_BCN_SYNC_CRIT_REQ_ACT_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "recal_notif_registered = %u",
        WMI_VDEV_STATS_RECAL_NOTIF_REGISTERED_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "bcn_tx_paused = %u",
        WMI_VDEV_STATS_BCN_TX_PAUSED_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "fils_channel_guard_time = %u",
        WMI_VDEV_STATS_FILS_CHANNEL_GUARD_TIME_GET(
            wmi_stats_buf->opaque_debug_vdev_fils_configs));
    WMI_STATS_PRINT(FATAL, "fd_tmpl_len = %u",
        WMI_VDEV_STATS_FD_TMPL_LEN_GET(
            wmi_stats_buf->opaque_debug_vdev_fils_configs));
    WMI_STATS_PRINT(FATAL, "first_beacon_recv_wait = %u",
        WMI_VDEV_STATS_FIRST_BEACON_RECV_WAIT_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));

    WMI_STATS_PRINT(FATAL,
        "\nAP-PS / KEEPALIVE / STA VDEV STATS INFO: (vdev_id:%d)",
        wmi_stats_buf->vdev_id);

    WMI_STATS_PRINT(FATAL, "event_bitmap = 0x%x",
        wmi_stats_buf->opaque_debug_event_bitmap);
    WMI_STATS_PRINT(FATAL, "peer_event_bitmap = 0x%x",
        wmi_stats_buf->opaque_debug_peer_event_bitmap);
    WMI_STATS_PRINT(FATAL, "peer_all_tid_pause_bitmap = 0x%x",
        wmi_stats_buf->opaque_debug_peer_all_tid_pause_bitmap);
    WMI_STATS_PRINT(FATAL, "peer_all_tid_block_bitmap = 0x%x",
        wmi_stats_buf->opaque_debug_peer_all_tid_block_bitmap);
    WMI_STATS_PRINT(FATAL, "tdls_peer_kickout_th = %u",
        wmi_stats_buf->opaque_debug_tdls_peer_kickout_th);
    WMI_STATS_PRINT(FATAL, "num_of_remote_peers_connected = %u",
        wmi_stats_buf->opaque_debug_num_of_remote_peers_connected);
    WMI_STATS_PRINT(FATAL, "num_group_key_enabled = %u",
        wmi_stats_buf->opaque_debug_num_group_key_enabled);
    WMI_STATS_PRINT(FATAL, "keepalive_arp_sender_ipv4 = %u",
        wmi_stats_buf->opaque_debug_keepalive_arp_sender_ipv4);
    WMI_STATS_PRINT(FATAL, "keepalive_arp_target_ipv4 = 0x%x",
        wmi_stats_buf->opaque_debug_keepalive_arp_target_ipv4);
    WMI_STATS_PRINT(FATAL, "keepalive_interval = %u",
        wmi_stats_buf->opaque_debug_keepalive_interval);
    WMI_STATS_PRINT(FATAL, "keepalive_timer_start_timestamp = %u",
        wmi_stats_buf->opaque_debug_keepalive_timer_start_timestamp);
    WMI_STATS_PRINT(FATAL, "sta_maxidle_interval = %u",
        wmi_stats_buf->opaque_debug_sta_maxidle_interval);
    WMI_STATS_PRINT(FATAL, "sta_maxidle_method = %u",
        wmi_stats_buf->opaque_debug_sta_maxidle_method);
    WMI_STATS_PRINT(FATAL, "tsf_curr_time_diff = %u",
        wmi_stats_buf->opaque_debug_tsf_curr_time_diff);
    WMI_STATS_PRINT(FATAL, "sleep_duration_us = %u",
        wmi_stats_buf->opaque_debug_sleep_duration_us);
    WMI_STATS_PRINT(FATAL, "pause_start_time_us = %u",
        wmi_stats_buf->opaque_debug_pause_start_time_us);
    WMI_STATS_PRINT(FATAL, "pause_delay_us = %u",
        wmi_stats_buf->opaque_debug_pause_delay_us);
    WMI_STATS_PRINT(FATAL, "num_supported_group_key = %u",
        wmi_stats_buf->opaque_debug_num_supported_group_key);
    WMI_STATS_PRINT(FATAL, "avg_data_null_tx_delay = %u",
        wmi_stats_buf->opaque_debug_avg_data_null_tx_delay);
    WMI_STATS_PRINT(FATAL, "avg_rx_leak_window = %u",
        wmi_stats_buf->opaque_debug_avg_rx_leak_window);
    WMI_STATS_PRINT(FATAL, "num_recv_deauth = %u",
        wmi_stats_buf->opaque_debug_num_recv_deauth);
    WMI_STATS_PRINT(FATAL, "tqm_bypass_enabled = %u",
        WMI_VDEV_STATS_TQM_BYPASS_ENABLED_GET(
            wmi_stats_buf->opaque_debug_vdev_event_delivery));
    WMI_STATS_PRINT(FATAL, "wmmac_timer_vote_cnt = %u",
        WMI_VDEV_STATS_WMMAC_TIMER_VOTE_CNT_GET(
            wmi_stats_buf->opaque_debug_vdev_event_delivery));
    WMI_STATS_PRINT(FATAL, "ic_txseqs_cmn = %u",
        WMI_VDEV_STATS_IC_TXSEQS_CMN_GET(
            wmi_stats_buf->opaque_debug_vdev_fils_period));
    WMI_STATS_PRINT(FATAL, "ap_keepalive_min_idle_inactive_time_secs = %u",
        WMI_VDEV_STATS_AP_KEEPALIVE_MIN_IDLE_INACTIVE_TIME_SECS_GET(
            wmi_stats_buf->opaque_debug_vdev_inactive_time));
    WMI_STATS_PRINT(FATAL, "ap_keepalive_max_idle_inactive_time_secs = %u",
        WMI_VDEV_STATS_AP_KEEPALIVE_MAX_IDLE_INACTIVE_TIME_SECS_GET(
            wmi_stats_buf->opaque_debug_vdev_inactive_time));
    WMI_STATS_PRINT(FATAL, "ap_keepalive_max_unresponsive_time_secs = %u",
        WMI_VDEV_STATS_AP_KEEPALIVE_MAX_UNRESPONSIVE_TIME_SECS_GET(
            wmi_stats_buf->opaque_debug_vdev_chain_mask_configs));
    WMI_STATS_PRINT(FATAL, "ext_cap_ie_len = %u",
        WMI_VDEV_STATS_EXT_CAP_IE_LEN_GET(
            wmi_stats_buf->opaque_debug_vdev_ie_len_configs));
    WMI_STATS_PRINT(FATAL, "keepalive_method = %u",
        WMI_VDEV_STATS_KEEPALIVE_METHOD_GET(
            wmi_stats_buf->opaque_debug_vdev_keepalive_bitfields));
    WMI_STATS_PRINT(FATAL, "keepalive_prohibit_data_mgmt = %u",
        WMI_VDEV_STATS_KEEPALIVE_PROHIBIT_DATA_MGMT_GET(
            wmi_stats_buf->opaque_debug_vdev_keepalive_bitfields));
    WMI_STATS_PRINT(FATAL, "resp_type = %u",
        WMI_VDEV_STATS_RESP_TYPE_GET(
            wmi_stats_buf->opaque_debug_vdev_keepalive_bitfields));
    WMI_STATS_PRINT(FATAL, "peer_event_delivery_in_progress = %u",
        WMI_VDEV_STATS_PEER_EVENT_DELIVERY_IN_PROGRESS_GET(
            wmi_stats_buf->opaque_debug_vdev_event_delivery));
    WMI_STATS_PRINT(FATAL, "vdev_event_delivery_in_progress = %u",
        WMI_VDEV_STATS_VDEV_EVENT_DELIVERY_IN_PROGRESS_GET(
            wmi_stats_buf->opaque_debug_vdev_event_delivery));
    WMI_STATS_PRINT(FATAL, "mbssid_capable_association = %u",
        WMI_VDEV_STATS_MBSSID_CAPABLE_ASSOCIATION_GET(
            wmi_stats_buf->opaque_debug_vdev_cap_slot_bitfield));
    WMI_STATS_PRINT(FATAL, "mbssid_txbssid_association = %u",
        WMI_VDEV_STATS_MBSSID_TXBSSID_ASSOCIATION_GET(
            wmi_stats_buf->opaque_debug_vdev_bcn_configs));
    WMI_STATS_PRINT(FATAL, "num_of_keepalive_attempts = %u",
        WMI_VDEV_STATS_NUM_OF_KEEPALIVE_ATTEMPTS_GET(
            wmi_stats_buf->opaque_debug_vdev_arp_configs));
    WMI_STATS_PRINT(FATAL, "ap_peer_keepalive_max_idle_time_reached = %u",
        WMI_VDEV_STATS_AP_PEER_KEEPALIVE_MAX_IDLE_TIME_REACHED_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));

    WMI_STATS_PRINT(FATAL,
        "\nVDEV TRAFFIC-THROUGHPUT-PERF ALGS USEFUL INFO: (vdev_id:%d)",
        wmi_stats_buf->vdev_id);

    WMI_STATS_PRINT(FATAL, "he_bss_rts_thld_tu = %u",
        wmi_stats_buf->opaque_debug_he_bss_rts_thld_tu);
    WMI_STATS_PRINT(FATAL, "rts_threshold = %u",
        wmi_stats_buf->opaque_debug_rts_threshold);
    WMI_STATS_PRINT(FATAL, "tx_fail_cnt_thres = %u",
        wmi_stats_buf->opaque_debug_tx_fail_cnt_thres);
    WMI_STATS_PRINT(FATAL, "mu_edca_sifs_ratio = %u",
        wmi_stats_buf->opaque_debug_mu_edca_sifs_ratio);
    WMI_STATS_PRINT(FATAL, "kickout_th = %u",
        wmi_stats_buf->opaque_debug_kickout_th);
    WMI_STATS_PRINT(FATAL, "rate_dd_bmap = %u",
        wmi_stats_buf->opaque_debug_rate_dd_bmap);
    WMI_STATS_PRINT(FATAL, "mtu_size = %u",
        wmi_stats_buf->opaque_debug_mtu_size);
    WMI_STATS_PRINT(FATAL, "atf_vdev_allowed_time = %u",
        wmi_stats_buf->opaque_debug_atf_vdev_allowed_time);
    WMI_STATS_PRINT(FATAL, "atf_vdev_used_unallocated_time = %u",
        wmi_stats_buf->opaque_debug_atf_vdev_used_unallocated_time);
    WMI_STATS_PRINT(FATAL, "atf_vdev_unused_time = %u",
        wmi_stats_buf->opaque_debug_atf_vdev_unused_time);
    WMI_STATS_PRINT(FATAL, "last_prog_time = %u",
        wmi_stats_buf->opaque_debug_last_prog_time);
    WMI_STATS_PRINT(FATAL, "last_recv_time = %u",
        wmi_stats_buf->opaque_debug_last_recv_time);
    WMI_STATS_PRINT(FATAL, "rx_pkt_on_channel = %u",
        wmi_stats_buf->opaque_debug_rx_pkt_on_channel);
    WMI_STATS_PRINT(FATAL, "traffic_config = %u",
        wmi_stats_buf->opaque_debug_traffic_config);
    WMI_STATS_PRINT(FATAL, "profile_idx = %u",
        wmi_stats_buf->opaque_debug_profile_idx);
    WMI_STATS_PRINT(FATAL, "profile_num = %u",
        wmi_stats_buf->opaque_debug_profile_num);
    WMI_STATS_PRINT(FATAL, "consec_detect_leaky_ap_cnt = %u",
        wmi_stats_buf->opaque_debug_consec_detect_leaky_ap_cnt);
    WMI_STATS_PRINT(FATAL, "sta_offset = %u",
        wmi_stats_buf->opaque_debug_sta_offset);
    WMI_STATS_PRINT(FATAL, "vdev_pn_rx_filter = %u",
        wmi_stats_buf->opaque_debug_vdev_pn_rx_filter);
    WMI_STATS_PRINT(FATAL, "dis_dyn_bw_rts = %u",
        WMI_VDEV_STATS_DIS_DYN_BW_RTS_GET(
            wmi_stats_buf->opaque_debug_vdev_amsdu_bitfield));
    WMI_STATS_PRINT(FATAL, "max_amsdu = %u",
        WMI_VDEV_STATS_MAX_AMSDU_GET(
            wmi_stats_buf->opaque_debug_vdev_amsdu_bitfield));
    WMI_STATS_PRINT(FATAL, "def_amsdu = %u",
        WMI_VDEV_STATS_DEF_AMSDU_GET(
            wmi_stats_buf->opaque_debug_vdev_amsdu_bitfield));
    WMI_STATS_PRINT(FATAL, "he_bss_color = %u",
        WMI_VDEV_STATS_HW_BSS_COLOR_GET(
            wmi_stats_buf->opaque_debug_vdev_amsdu_bitfield));
    WMI_STATS_PRINT(FATAL, "he_def_pe_duration = %u",
        WMI_VDEV_STATS_HE_DEF_PE_DURATION_GET(
            wmi_stats_buf->opaque_debug_vdev_ac_failure_configs));
    WMI_STATS_PRINT(FATAL, "minimum_allowed_mcs = %u",
        WMI_VDEV_STATS_MINIMUM_ALLOWED_MCS_GET(
            wmi_stats_buf->opaque_debug_vdev_ac_failure_configs));
    WMI_STATS_PRINT(FATAL, "max_11ac_to_leg_rts_fallback_th = %u",
        WMI_VDEV_STATS_MAX_11AC_TO_LEG_RTS_FALLBACK_TH_GET(
            wmi_stats_buf->opaque_debug_vdev_ac_failure_configs));
    WMI_STATS_PRINT(FATAL, "max_11ac_rts_consec_failure_th = %u",
        WMI_VDEV_STATS_MAX_11AC_RTS_CONSEC_FAILURE_GET(
            wmi_stats_buf->opaque_debug_vdev_ac_failure_configs));
    WMI_STATS_PRINT(FATAL, "input_pkt_type = %u",
        WMI_VDEV_STATS_INPUT_PKT_TYPE_GET(
            wmi_stats_buf->opaque_debug_vdev_pkt_type_info));
    WMI_STATS_PRINT(FATAL, "recv_pkt_type = %u",
        WMI_VDEV_STATS_RECV_PKT_TYPE_GET(
            wmi_stats_buf->opaque_debug_vdev_pkt_type_info));
    WMI_STATS_PRINT(FATAL, "disable_intra_fwd = %u",
        WMI_VDEV_STATS_DISABLE_INTRA_FWD_GET(
            wmi_stats_buf->opaque_debug_vdev_pkt_type_info));
    WMI_STATS_PRINT(FATAL, "ps_awake = %u",
        WMI_VDEV_STATS_PS_AWAKE_GET(
            wmi_stats_buf->opaque_debug_vdev_pkt_type_info));
    WMI_STATS_PRINT(FATAL, "snr_cal_count = %u",
        WMI_VDEV_STATS_SNR_CAL_COUNT_GET(
            wmi_stats_buf->opaque_debug_vdev_ba_param_bitfield));
    WMI_STATS_PRINT(FATAL, "amsdu_auto_enable = %u",
        WMI_VDEV_STATS_AMSDU_AUTO_ENABLE_GET(
            wmi_stats_buf->opaque_debug_vdev_ba_param_bitfield));
    WMI_STATS_PRINT(FATAL, "param_ba_timeout = %u",
        WMI_VDEV_STATS_PARAM_BA_TIMEOUT_GET(
            wmi_stats_buf->opaque_debug_vdev_ba_param_bitfield));
    WMI_STATS_PRINT(FATAL, "param_ba_buffer_size = %u",
        WMI_VDEV_STATS_PARAM_BA_BUFFER_SIZE_GET(
            wmi_stats_buf->opaque_debug_vdev_ba_param_bitfield));
    WMI_STATS_PRINT(FATAL, "param_amsdu_support = %u",
        WMI_VDEV_STATS_PARAM_AMSDU_SUPPORT_GET(
            wmi_stats_buf->opaque_debug_vdev_aggr_bitfield));
    WMI_STATS_PRINT(FATAL, "param_ba_retry_max = %u",
        WMI_VDEV_STATS_PARAM_BA_RETRY_MAX_GET(
            wmi_stats_buf->opaque_debug_vdev_aggr_bitfield));
    WMI_STATS_PRINT(FATAL, "tx_aggr_size = %u",
        WMI_VDEV_STATS_TX_AGGR_SIZE_GET(
            wmi_stats_buf->opaque_debug_vdev_aggr_bitfield));
    WMI_STATS_PRINT(FATAL, "rx_aggr_size = %u",
        WMI_VDEV_STATS_RX_AGGR_SIZE_GET(
            wmi_stats_buf->opaque_debug_vdev_aggr_bitfield));
    WMI_STATS_PRINT(FATAL, "preferred_tx_streams = %u",
        WMI_VDEV_STATS_PREFERRED_TX_STREAMS_GET(
            wmi_stats_buf->opaque_debug_vdev_streams_configs));
    WMI_STATS_PRINT(FATAL, "preferred_rx_streams = %u",
        WMI_VDEV_STATS_PREFERRED_RX_STREAMS_GET(
            wmi_stats_buf->opaque_debug_vdev_streams_configs));
    WMI_STATS_PRINT(FATAL, "preferred_tx_streams_160 = %u",
        WMI_VDEV_STATS_PREFERRED_TX_STREAMS_160_GET(
            wmi_stats_buf->opaque_debug_vdev_chains_configs));
    WMI_STATS_PRINT(FATAL, "preferred_rx_streams_160 = %u",
        WMI_VDEV_STATS_PREFERRED_RX_STREAMS_160_GET(
            wmi_stats_buf->opaque_debug_vdev_chains_configs));
    WMI_STATS_PRINT(FATAL, "tx_chains_num_11b = %u",
        WMI_VDEV_STATS_TX_CHAINS_NUM_11B_GET(
            wmi_stats_buf->opaque_debug_vdev_chains_configs));
    WMI_STATS_PRINT(FATAL, "tx_chains_num_11ag = %u",
        WMI_VDEV_STATS_TX_CHAINS_NUM_11AG_GET(
            wmi_stats_buf->opaque_debug_vdev_chains_configs));
    WMI_STATS_PRINT(FATAL, "supp_op_cls_ie_len = %u",
        WMI_VDEV_STATS_SUPP_OP_CLS_IE_LEN_GET(
            wmi_stats_buf->opaque_debug_vdev_power_cap_configs));
    WMI_STATS_PRINT(FATAL, "rm_en_cap_ie_len = %u",
        WMI_VDEV_STATS_RM_EN_CAP_IE_LEN_GET(
            wmi_stats_buf->opaque_debug_vdev_power_cap_configs));
    WMI_STATS_PRINT(FATAL, "power_cap_ie_len = %u",
        WMI_VDEV_STATS_POWER_CAP_IE_LEN_GET(
            wmi_stats_buf->opaque_debug_vdev_power_cap_configs));
    WMI_STATS_PRINT(FATAL, "supp_channel_ie_len = %u",
        WMI_VDEV_STATS_SUPP_CHANNEL_IE_LEN_GET(
            wmi_stats_buf->opaque_debug_vdev_power_cap_configs));
    WMI_STATS_PRINT(FATAL, "wmm_tspec_ie_len = %u",
        WMI_VDEV_STATS_WMM_TSPEC_IE_LEN_GET(
            wmi_stats_buf->opaque_debug_vdev_wmm_mbo_configs));
    WMI_STATS_PRINT(FATAL, "ccx_version_ie_len = %u",
        WMI_VDEV_STATS_CCX_VERSION_IE_LEN_GET(
            wmi_stats_buf->opaque_debug_vdev_wmm_mbo_configs));
    WMI_STATS_PRINT(FATAL, "extn_dh_ie_len = %u",
        WMI_VDEV_STATS_EXTN_DH_IE_LEN_GET(
            wmi_stats_buf->opaque_debug_vdev_wmm_mbo_configs));
    WMI_STATS_PRINT(FATAL, "mbo_ie_len = %u",
        WMI_VDEV_STATS_MBO_IE_LEN_GET(
            wmi_stats_buf->opaque_debug_vdev_wmm_mbo_configs));
    WMI_STATS_PRINT(FATAL, "rsnxe_ie_len = %u",
        WMI_VDEV_STATS_RSNXE_IE_LEN_GET(
            wmi_stats_buf->opaque_debug_vdev_remote_configs));
    WMI_STATS_PRINT(FATAL, "remote_peer_cnt = %u",
        WMI_VDEV_STATS_REMOTE_PEER_CNT_GET(
            wmi_stats_buf->opaque_debug_vdev_remote_configs));
    WMI_STATS_PRINT(FATAL, "p2p_cli_pause_type = %u",
        WMI_VDEV_STATS_P2P_CLI_PAUSE_TYPE_GET(
            wmi_stats_buf->opaque_debug_vdev_remote_configs));
    WMI_STATS_PRINT(FATAL, "mu_edca_update_count = %u",
        WMI_VDEV_STATS_MU_EDCA_UPDATE_COUNT_GET(
            wmi_stats_buf->opaque_debug_vdev_remote_configs));
    WMI_STATS_PRINT(FATAL, "he_su_bfee = %u",
        WMI_VDEV_STATS_HE_SU_BFEE_GET(
            wmi_stats_buf->opaque_debug_vdev_hu_mu_configs));
    WMI_STATS_PRINT(FATAL, "he_su_bfer = %u",
        WMI_VDEV_STATS_HE_SU_BFER_GET(
            wmi_stats_buf->opaque_debug_vdev_hu_mu_configs));
    WMI_STATS_PRINT(FATAL, "he_mu_bfee = %u",
        WMI_VDEV_STATS_HE_MU_BFEE_GET(
            wmi_stats_buf->opaque_debug_vdev_hu_mu_configs));
    WMI_STATS_PRINT(FATAL, "he_mu_bfer = %u",
        WMI_VDEV_STATS_HE_MU_BFER_GET(
            wmi_stats_buf->opaque_debug_vdev_hu_mu_configs));
    WMI_STATS_PRINT(FATAL, "he_dl_ofdma = %u",
        WMI_VDEV_STATS_HE_DL_OFDMA_GET(
            wmi_stats_buf->opaque_debug_vdev_hu_mu_configs));
    WMI_STATS_PRINT(FATAL, "he_ul_ofdma = %u",
        WMI_VDEV_STATS_HE_UL_OFDMA_GET(
            wmi_stats_buf->opaque_debug_vdev_hu_mu_configs));
    WMI_STATS_PRINT(FATAL, "he_ul_mumimo = %u",
        WMI_VDEV_STATS_HE_UL_MUMIMO_GET(
            wmi_stats_buf->opaque_debug_vdev_hu_mu_configs));
    WMI_STATS_PRINT(FATAL, "ul_mu_resp = %u",
        WMI_VDEV_STATS_UL_MU_RESP_GET(
            wmi_stats_buf->opaque_debug_vdev_hu_mu_configs));
    WMI_STATS_PRINT(FATAL, "alt_rssi_non_srg = %u",
        WMI_VDEV_STATS_ALT_RSSI_NON_SRG_GET(
            wmi_stats_buf->opaque_debug_vdev_hu_mu_configs));
    WMI_STATS_PRINT(FATAL, "alt_rssi_srg = %u",
        WMI_VDEV_STATS_ALT_RSSI_SRG_GET(
            wmi_stats_buf->opaque_debug_vdev_hu_mu_configs));
    WMI_STATS_PRINT(FATAL, "he_bss_color_en = %u",
        WMI_VDEV_STATS_HE_BSS_COLOR_EN_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "he_txbf_ofdma = %u",
        WMI_VDEV_STATS_HE_TXBF_OFDMA_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "non_srg_enable = %u",
        WMI_VDEV_STATS_NON_SRG_ENABLE_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "srg_enable = %u",
        WMI_VDEV_STATS_SRG_ENABLE_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "srp_enable = %u",
        WMI_VDEV_STATS_SRP_ENABLE_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "sr_initialized = %u",
        WMI_VDEV_STATS_SR_INITIALIZED_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "sr_rings_initialized = %u",
        WMI_VDEV_STATS_SR_RINGS_INITIALIZED_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "per_ac_obss_pd_enable = %u",
        WMI_VDEV_STATS_PER_AC_OBSS_PD_ENABLE_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "chain_mask = %u",
        WMI_VDEV_STATS_CHAIN_MASK_GET(
            wmi_stats_buf->opaque_debug_vdev_chain_mask_configs));
    WMI_STATS_PRINT(FATAL, "num_mcast_filters = %u",
        WMI_VDEV_STATS_NUM_MCAST_FILTERS_GET(
            wmi_stats_buf->opaque_debug_vdev_ie_len_configs));
    WMI_STATS_PRINT(FATAL, "rts_cts_default = %u",
        WMI_VDEV_STATS_RTS_CTS_DEFAULT_GET(
            wmi_stats_buf->opaque_debug_vdev_bcn_drift_info));
    WMI_STATS_PRINT(FATAL, "pause_cnt = %u",
        WMI_VDEV_STATS_PAUSE_CNT_GET(
            wmi_stats_buf->opaque_debug_vdev_mac_configs));
    WMI_STATS_PRINT(FATAL, "e_mac_id = %u",
        WMI_VDEV_STATS_E_MAC_ID_GET(
            wmi_stats_buf->opaque_debug_vdev_mac_configs));
    WMI_STATS_PRINT(FATAL, "is_transmit_bssid = %u",
        WMI_VDEV_STATS_IS_TRANSMIT_BSSID_GET(
            wmi_stats_buf->opaque_debug_vdev_mac_configs));
    WMI_STATS_PRINT(FATAL, "rts_rc_flag = %u",
        WMI_VDEV_STATS_RTS_RC_FLAG_GET(
            wmi_stats_buf->opaque_debug_vdev_mac_configs));
    WMI_STATS_PRINT(FATAL, "leakyAp_cts2s_enable = %u",
        WMI_VDEV_STATS_LEAKYAP_CTS2S_ENABLE_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "staSapScc_in_mcc = %u",
        WMI_VDEV_STATS_STASAPSCC_IN_MCC_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "staSapScc_in_mcc_cts2s_enable = %u",
        WMI_VDEV_STATS_STASAPSCC_IN_MCC_CTS2S_ENABLE_GET(
            wmi_stats_buf->opaque_debug_vdev_wmi_configs));
    WMI_STATS_PRINT(FATAL, "he_bss_color_en_bypass = %u",
        WMI_VDEV_STATS_HE_BSS_COLOR_EN_BYPASS_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "default_ba_mode = %u",
        WMI_VDEV_STATS_DEFAULT_BA_MODE_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "ba_256_bitmap_enable = %u",
        WMI_VDEV_STATS_BA_256_BITMAP_ENABLE_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "ba_256_bitmap_tx_disable = %u",
        WMI_VDEV_STATS_BA_256_BITMAP_TX_DISABLE_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "is_multi_group_key_enabled = %u",
        WMI_VDEV_STATS_IS_MULTI_GROUP_KEY_ENABLED_GET(
            wmi_stats_buf->opaque_debug_vdev_sm_chan_configs));
    WMI_STATS_PRINT(FATAL, "common_rsn_caps = %u",
        WMI_VDEV_STATS_COMMON_RSN_CAPS_GET(
            wmi_stats_buf->opaque_debug_vdev_chan_configs));
    WMI_STATS_PRINT(FATAL, "preferred_tx_streams_320 = %u",
        WMI_VDEV_STATS_PREFERRED_TX_STREAMS_320_GET(
            wmi_stats_buf->opaque_debug_vdev_stats_id_configs));
    WMI_STATS_PRINT(FATAL, "preferred_rx_streams_320 = %u",
        WMI_VDEV_STATS_PREFERRED_RX_STREAMS_320_GET(
            wmi_stats_buf->opaque_debug_vdev_stats_id_configs));

    WMI_STATS_PRINT(FATAL,
        "\nDEBUG FIELDS: (vdev_id:%d)", wmi_stats_buf->vdev_id);
    WMI_STATS_PRINT(FATAL, "field_1 0x%x",
             wmi_stats_buf->opaque_debug_field_1);
    WMI_STATS_PRINT(FATAL, "field_2 0x%x",
             wmi_stats_buf->opaque_debug_field_2);
    WMI_STATS_PRINT(FATAL, "field_3 0x%x",
             wmi_stats_buf->opaque_debug_field_4);
    WMI_STATS_PRINT(FATAL, "field_4 0x%x",
             wmi_stats_buf->opaque_debug_field_4);
}

/*
 * wmi_print_ctrl_path_pdev_tx_stats_tlv: display wmi_print_ctrl_path_dfs_channel_tx_stats
 * @tag_buf: buffer containing the tlv wmi_ctrl_path_dfs_channel_stats_struct_tlv
 *
 * return:void
 */
static inline void wmi_print_ctrl_path_dfs_channel_stats_tlv(void *tag_buf)
{
    wmi_ctrl_path_dfs_channel_stats_struct *wmi_stats_buf =
        (wmi_ctrl_path_dfs_channel_stats_struct *)tag_buf;

    /* print the TLV type only for the first TLV within the series */
    if (!wmi_stats_buf->nol) {
        WMI_STATS_PRINT(FATAL, "WMI_CTRL_PATH_DFS_CHANNEL_STATS_TLV:");
    }

    WMI_STATS_PRINT(FATAL,
        "pdev_id = %u | nol = %u channel = %u MHz | "
        "width = %u MHz | timeleft = %u seconds",
        wmi_stats_buf->pdev_id, wmi_stats_buf->nol,  wmi_stats_buf->channel,
        wmi_stats_buf->chwidth, wmi_stats_buf->timeleft);
}

/*
 * wmi_print_ctrl_path_btcoex_stats_tlv: display wmi_print_ctrl_path_btcoex_stats
 * @tag_buf: buffer containing the tlv wmi_ctrl_path_btcoex_stats_struct_tlv
 *
 * return:void
 */
static inline void wmi_print_ctrl_path_btcoex_stats_tlv(void *tag_buf)
{

    wmi_ctrl_path_btcoex_stats_struct *wmi_stats_buf =
        (wmi_ctrl_path_btcoex_stats_struct *)tag_buf;

    WMI_STATS_PRINT(FATAL, "WMI_CTRL_PATH_BTCOEX_STATS_TLV:\n");

    WMI_STATS_PRINT(FATAL, "pdev_id = %u ", wmi_stats_buf->pdev_id);

    WMI_STATS_PRINT(FATAL, "bt_tx_req_cntr = %u ",
        wmi_stats_buf->bt_tx_req_cntr);
    WMI_STATS_PRINT(FATAL, "bt_rx_req_cntr = %u ",
        wmi_stats_buf->bt_rx_req_cntr);
    WMI_STATS_PRINT(FATAL, "bt_req_nack_cntr = %u\n",
        wmi_stats_buf->bt_req_nack_cntr);
    WMI_STATS_PRINT(FATAL, "wl_tx_req_nack_schd_bt_reason_cntr = %u ",
        wmi_stats_buf->wl_tx_req_nack_schd_bt_reason_cntr);
    WMI_STATS_PRINT(FATAL, "wl_tx_req_nack_current_bt_reason_cntr = %u ",
        wmi_stats_buf->wl_tx_req_nack_current_bt_reason_cntr);
    WMI_STATS_PRINT(FATAL, "wl_tx_req_nack_other_wlan_tx_reason_cntr = %u ",
        wmi_stats_buf->wl_tx_req_nack_other_wlan_tx_reason_cntr);
    WMI_STATS_PRINT(FATAL, "wl_in_tx_abort_cntr = %u ",
        wmi_stats_buf->wl_in_tx_abort_cntr);
    WMI_STATS_PRINT(FATAL, "wl_tx_auto_resp_req_cntr = %u ",
        wmi_stats_buf->wl_tx_auto_resp_req_cntr);
    WMI_STATS_PRINT(FATAL, "wl_tx_req_ack_cntr = %u ",
        wmi_stats_buf->wl_tx_req_ack_cntr);
    WMI_STATS_PRINT(FATAL, "wl_tx_req_cntr = %u ",
        wmi_stats_buf->wl_tx_req_cntr);
}

/*
 * wmi_print_ctrl_path_mem_stats_tlv: display wmi_print_ctrl_path_mem_stats
 * @tag_buf: buffer containing the tlv wmi_ctrl_path_mem_stats_struct_tlv
 *
 * return:void
 */
static inline void wmi_print_ctrl_path_mem_stats_tlv(void *tag_buf)
{
    wmi_ctrl_path_mem_stats_struct *wmi_stats_buf =
        (wmi_ctrl_path_mem_stats_struct *) tag_buf;
    if (0 != wmi_stats_buf->total_bytes) {
        if (0 == wmi_stats_buf->arena_id) {
            WMI_STATS_PRINT(FATAL,
                "|------------------------------------------------------------"
                "---------------------------------------------------|");
            WMI_STATS_PRINT(FATAL,
                "| %.20s  \t\t| %.16s \t\t|                  %.16s  \t| %.16s "
                "\t\t|","Arena","Total Size","Consumption","Headroom");
            WMI_STATS_PRINT(FATAL,
                "|        \t\t|       \t\t|       \t\t|     \t\t|       \t\t|");
            WMI_STATS_PRINT(FATAL,
                "|        \t\t|       \t\t| %.16s \t\t| %.16s \t|       \t\t|",
                "Absolute", "Percent(%)");
            WMI_STATS_PRINT(FATAL,
                "|------------------------------------------------------------"
                "---------------------------------------------------|");
        }
        WMI_STATS_PRINT(FATAL,
            "| %.20s  \t\t| %-16u \t| %-16u \t| %05.2f%%  \t| %-16u \t|",
            ((A_UINT8 *) wmi_ctrl_path_fw_arena_id_to_name(
                wmi_stats_buf->arena_id) + sizeof("WMI_CTRL_PATH_STATS_ARENA")),
            wmi_stats_buf->total_bytes,
            wmi_stats_buf->allocated_bytes,
            ((wmi_stats_buf->allocated_bytes * 100.0) /
                wmi_stats_buf->total_bytes),
            (wmi_stats_buf->total_bytes - wmi_stats_buf->allocated_bytes));
        WMI_STATS_PRINT(FATAL,
            "|----------------------------------------------------------------"
            "-----------------------------------------------|");
    }
}

/*
 * wmi_print_ctrl_path_calibration_stats_tlv: display wmi_print_ctrl_path_calibration_stats
 * @tag_buf: buffer containing the tlv wmi_ctrl_path_calibration_stats_struct_tlv
 *
 * return:void
 */
static inline void wmi_print_ctrl_path_calibration_stats_tlv(void *tag_buf)
{
    wmi_ctrl_path_calibration_stats_struct *wmi_stats_buf =
        (wmi_ctrl_path_calibration_stats_struct *) tag_buf;
    static A_UINT32 prev_cal_profile = WMI_CTRL_PATH_STATS_CAL_PROFILE_INVALID;

    if (WMI_CTRL_PATH_CALIBRATION_STATS_CAL_PROFILE_GET(wmi_stats_buf->cal_info) == WMI_CTRL_PATH_STATS_CAL_PROFILE_INVALID) {
        if (!WMI_CTRL_PATH_CALIBRATION_STATS_IS_PERIODIC_CAL_GET(wmi_stats_buf->cal_info)) {
            WMI_STATS_PRINT(FATAL, "WMI_CTRL_PATH_CALIBRATION_STATS_TLV:");
            WMI_STATS_PRINT(FATAL, "pdev_id = %u", wmi_stats_buf->pdev_id);
            WMI_STATS_PRINT(FATAL,
                "|==========================================================="
                "============================================================"
                "=======|");
            WMI_STATS_PRINT(FATAL,
                "| %-25s| %-25s| %-17s| %-16s| %-16s| %-16s|",
                "cal_profile", "cal_type", "cal_triggered_cnt", "cal_fail_cnt",
                "cal_fcs_cnt", "cal_fcs_fail_cnt");
        } else {
            WMI_STATS_PRINT(FATAL,
                "|==========================================================="
                "============================================================"
                "=======|");
        }
        return;
    }

    if (prev_cal_profile != WMI_CTRL_PATH_CALIBRATION_STATS_CAL_PROFILE_GET(wmi_stats_buf->cal_info)) {
        WMI_STATS_PRINT(FATAL,
            "|==============================================================="
            "===============================================================|");
        prev_cal_profile = WMI_CTRL_PATH_CALIBRATION_STATS_CAL_PROFILE_GET(wmi_stats_buf->cal_info);
    }

    if (!WMI_CTRL_PATH_CALIBRATION_STATS_IS_PERIODIC_CAL_GET(wmi_stats_buf->cal_info)) {
        WMI_STATS_PRINT(FATAL,
            "| %-25s| %-25s| %-17u| %-16u| %-16u| %-16u|",
            ((A_UINT8 *) wmi_ctrl_path_cal_profile_id_to_name(
                WMI_CTRL_PATH_CALIBRATION_STATS_CAL_PROFILE_GET(wmi_stats_buf->cal_info)) +
                sizeof("WMI_CTRL_PATH_STATS_CAL_PROFILE")),
            ((A_UINT8 *) wmi_ctrl_path_cal_type_id_to_name(
                WMI_CTRL_PATH_CALIBRATION_STATS_CAL_TYPE_GET(wmi_stats_buf->cal_info)) +
                sizeof("WMI_CTRL_PATH_STATS_CAL_TYPE")),
            wmi_stats_buf->cal_triggered_cnt,
            wmi_stats_buf->cal_fail_cnt,
            wmi_stats_buf->cal_fcs_cnt,
            wmi_stats_buf->cal_fcs_fail_cnt);
    } else {
        WMI_STATS_PRINT(FATAL,
            "| %-25s| %-25s| %-17u| %-16u| %-16u| %-16u|",
            "PERIODIC_CAL",
            ((A_UINT8 *) wmi_ctrl_path_periodic_cal_type_id_to_name(
                WMI_CTRL_PATH_CALIBRATION_STATS_CAL_TYPE_GET(wmi_stats_buf->cal_info)) +
                sizeof("WMI_CTRL_PATH_STATS_PERIODIC_CAL_TYPE")),
            wmi_stats_buf->cal_triggered_cnt,
            wmi_stats_buf->cal_fail_cnt,
            wmi_stats_buf->cal_fcs_cnt,
            wmi_stats_buf->cal_fcs_fail_cnt);
    }

    WMI_STATS_PRINT(FATAL,
        "|```````````````````````````````````````````````````````````````````"
        "```````````````````````````````````````````````````````````|");
}

/*
 * wmi_print_ctrl_path_awgn_stats_tlv: display wmi_print_ctrl_path_awgn_stats
 * @tag_buf: buffer containing the tlv wmi_ctrl_path_awgn_stats_struct_tlv
 *
 * return:void
 */
static inline void wmi_print_ctrl_path_awgn_stats_tlv(void *tag_buf)
{
    wmi_ctrl_path_awgn_stats_struct *wmi_stats_buf =
        (wmi_ctrl_path_awgn_stats_struct *)tag_buf;

    WMI_STATS_PRINT(FATAL, "WMI_CTRL_PATH_AWGN_STATS_TLV:");

    WMI_STATS_PRINT(FATAL,
        "\n|```````````````````````````````````````````````````````````````````"
        "```````````````````````````````````````````````````````````|\n");

    WMI_STATS_PRINT(FATAL, "awgn_send_evt_cnt = %u",
                wmi_stats_buf->awgn_send_evt_cnt);

    WMI_STATS_PRINT(FATAL, "awgn_pri_int_cnt = %u",
                wmi_stats_buf->awgn_pri_int_cnt);

    WMI_STATS_PRINT(FATAL, "awgn_sec_int_cnt = %u",
                wmi_stats_buf->awgn_sec_int_cnt);

    WMI_STATS_PRINT(FATAL, "awgn_pkt_drop_trigger_cnt = %u",
                wmi_stats_buf->awgn_pkt_drop_trigger_cnt);

    WMI_STATS_PRINT(FATAL, "awgn_pkt_drop_trigger_reset_cnt = %u",
                wmi_stats_buf->awgn_pkt_drop_trigger_reset_cnt);

    WMI_STATS_PRINT(FATAL, "awgn_bw_drop_cnt = %u",
                wmi_stats_buf->awgn_bw_drop_cnt);

    WMI_STATS_PRINT(FATAL, "awgn_bw_drop_reset_cnt = %u",
                wmi_stats_buf->awgn_bw_drop_reset_cnt);

    WMI_STATS_PRINT(FATAL, "awgn_cca_int_cnt = %u",
                wmi_stats_buf->awgn_cca_int_cnt);

    WMI_STATS_PRINT(FATAL, "awgn_cca_int_reset_cnt = %u",
                wmi_stats_buf->awgn_cca_int_reset_cnt);

    WMI_STATS_PRINT(FATAL, "awgn_cca_ack_blk_cnt = %u",
                wmi_stats_buf->awgn_cca_ack_blk_cnt);

    WMI_STATS_PRINT(FATAL, "awgn_cca_ack_reset_cnt = %u",
                wmi_stats_buf->awgn_cca_ack_reset_cnt);

    WMI_STATS_PRINT(FATAL,
        "awgn_int_bw_cnt-AWGN_20[0]: %u "
        "AWGN_40[1]: %u "
        "AWGN_80[2]: %u "
        "AWGN_160[3]: %u "
        "AWGN_320[5]: %u",
        wmi_stats_buf->awgn_int_bw_cnt[0],
        wmi_stats_buf->awgn_int_bw_cnt[1],
        wmi_stats_buf->awgn_int_bw_cnt[2],
        wmi_stats_buf->awgn_int_bw_cnt[3],
        wmi_stats_buf->awgn_int_bw_cnt[5]);

    WMI_STATS_PRINT(FATAL,
        "\n|```````````````````````````````````````````````````````````````````"
        "```````````````````````````````````````````````````````````|\n");

    WMI_STATS_PRINT(FATAL, "obss_int_cnt = %u",
                wmi_stats_buf->obss_int_cnt);

    WMI_STATS_PRINT(FATAL, "obss_int_evt_sent_host_cnt = %u",
                wmi_stats_buf->obss_int_evt_sent_host_cnt);

    WMI_STATS_PRINT(FATAL, "obss_int_evt_skip_awgn_cnt = %u",
                wmi_stats_buf->obss_int_evt_skip_awgn_cnt);

    WMI_STATS_PRINT(FATAL, "obss_int_evt_skip_dup_cnt = %u",
                wmi_stats_buf->obss_int_evt_skip_dup_cnt);

    WMI_STATS_PRINT(FATAL, "obss_int_cur_int_seg = %u",
                wmi_stats_buf->obss_int_cur_int_seg);

    WMI_STATS_PRINT(FATAL, "obss_int_prv_int_seg = %u",
                wmi_stats_buf->obss_int_prv_int_seg);

    WMI_STATS_PRINT(FATAL,
        "obss_int_bw_cnt-OBSS_20[0]: %u "
        "OBSS_40[1]: %u "
        "OBSS_80[2]: %u "
        "OBSS_160[3]: %u "
        "OBSS_320[5]: %u",
        wmi_stats_buf->obss_int_bw_cnt[0],
        wmi_stats_buf->obss_int_bw_cnt[1],
        wmi_stats_buf->obss_int_bw_cnt[2],
        wmi_stats_buf->obss_int_bw_cnt[3],
        wmi_stats_buf->obss_int_bw_cnt[5]);

    WMI_STATS_PRINT(FATAL,
        "\n|```````````````````````````````````````````````````````````````````"
        "```````````````````````````````````````````````````````````|\n");
}

/*
 * wmi_print_ctrl_path_bmiss_stats_tlv: display wmi_print_ctrl_path_bmiss_stats
 * @tag_buf: buffer containing the tlv wmi_ctrl_path_bmiss_stats_struct_tlv
 *
 * return:void
 */
static inline void wmi_print_ctrl_path_bmiss_stats_tlv(void *tag_buf)
{
    wmi_ctrl_path_bmiss_stats_struct *wmi_stats_buf =
        (wmi_ctrl_path_bmiss_stats_struct *)tag_buf;
    int i;
    /* print the TLV type only for the first TLV within the series */
    WMI_STATS_PRINT(FATAL, "WMI_CTRL_PATH_BMISS_STATS_TLV:");

    WMI_STATS_PRINT(FATAL,
        "num_pre_bmiss = %u | num_first_bmiss = %u | "
        "num_final_bmiss = %u | "
        "num_null_sent_in_first_bmiss = %u | "
        "num_null_failed_in_first_bmiss = %u | "
        "num_null_sent_in_final_bmiss = %u | "
        "num_null_failed_in_final_bmiss = %u ",
        wmi_stats_buf->num_pre_bmiss, wmi_stats_buf->num_first_bmiss,
        wmi_stats_buf->num_final_bmiss,
        wmi_stats_buf->num_null_sent_in_first_bmiss,
        wmi_stats_buf->num_null_failed_in_first_bmiss,
        wmi_stats_buf->num_null_sent_in_final_bmiss,
        wmi_stats_buf->num_null_failed_in_final_bmiss);

    WMI_STATS_PRINT(FATAL,
        "num_of_bmiss_sequences = %u | num_bitmask_wraparound = %u | "
        "num_bcn_hist_lost = %u | ",
        wmi_stats_buf->cons_bmiss_stats.num_of_bmiss_sequences,
        wmi_stats_buf->cons_bmiss_stats.num_bitmask_wraparound,
        wmi_stats_buf->cons_bmiss_stats.num_bcn_hist_lost);

    for (i = 0; i < BMISS_STATS_RSSI_SAMPLE_MAX; i++){
        int idx;
        idx = wmi_stats_buf->rssi_sample_curr_index + i;
        idx = idx % BMISS_STATS_RSSI_SAMPLE_MAX;
        WMI_STATS_PRINT(FATAL,
            "index = %u | "
            "rssi = %d | sample_time = 0x%x ",
            idx,
            wmi_stats_buf->rssi_samples[idx].rssi,
            wmi_stats_buf->rssi_samples[idx].sample_time);
    }
}

/*
 * wmi_print_ctrl_path_odd_addr_read_tlv: display wmi_print_ctrl_path_odd_addr_read_tlv
 * @tag_buf: buffer containing the tlv wmi_ctrl_path_odd_addr_read_struct_tlv
 *
 * return:void
 */
static inline void wmi_print_ctrl_path_odd_addr_read_tlv(void *tag_buf)
{
    A_UINT32 i = 0;

    wmi_ctrl_path_odd_addr_read_struct *wmi_stats_buf =
        (wmi_ctrl_path_odd_addr_read_struct *)tag_buf;

    WMI_STATS_PRINT(FATAL, "WMI_CTRL_PATH_ODD_ADDR_READ_TLV:");
    switch (wmi_stats_buf->resp_type) {
        case WMI_ODD_ADDR_READ_OPTION_TYPE_ADD_ADDR_COMMAND:
            if (wmi_stats_buf->is_success) {
                WMI_STATS_PRINT(FATAL,
                    "Succesfully Added %x to at entry id : %d",
                    wmi_stats_buf->address[0],
                    wmi_stats_buf->entry_id);
            } else {
                WMI_STATS_PRINT(FATAL,
                    "Failed to add %x at entry id : %d",
                    wmi_stats_buf->address[0],
                    wmi_stats_buf->entry_id);
            }
            break;
        case WMI_ODD_ADDR_READ_OPTION_TYPE_DEL_ADDR_COMMAND:
            if (wmi_stats_buf->is_success) {
                if (wmi_stats_buf->entry_id) {
                    WMI_STATS_PRINT(FATAL,
                        "Succesfully Removed %x at entry id : %d from the list",
                        wmi_stats_buf->address[0],
                        wmi_stats_buf->entry_id);
                } else {
                    WMI_STATS_PRINT(FATAL,
                        "Succesfully Removed all the entries from the list");
                }
            } else {
                WMI_STATS_PRINT(FATAL,
                    "Failed to remove address at entry id : %d",
                    wmi_stats_buf->entry_id);
            }
            break;
        case WMI_ODD_ADDR_READ_OPTION_TYPE_DISP_ADDR_COMMAND:
            WMI_STATS_PRINT(FATAL, "Entry id | Address");
            WMI_STATS_PRINT(FATAL,
                "|=======================================================|");
            for (i = 0; i< WMI_MAX_ADDRESS_SPACE; i++) {
                WMI_STATS_PRINT(FATAL,
                    "    %u\t\t0x%08x", i+1, wmi_stats_buf->address[i]);
            }
            break;
        case WMI_ODD_ADDR_READ_OPTION_TYPE_DISP_VAL_COMMAND:
            WMI_STATS_PRINT(FATAL, "Entry id | Address     | Value");
            WMI_STATS_PRINT(FATAL,
                "|=======================================================|");
            for (i = 0; i< WMI_MAX_ADDRESS_SPACE; i++) {
                WMI_STATS_PRINT(FATAL,
                    "    %u\t0x%08x\t0x%08x",
                    i+1, wmi_stats_buf->address[i], wmi_stats_buf->data[i]);
            }
            break;
        default:
            break;
    }
}

/*
 * wmi_print_ctrl_path_afc_stats_tlv: display wmi_print_ctrl_path_afc_stats_tlv
 * @tag_buf: buffer containing the tlv wmi_ctrl_path_odd_addr_read_struct_tlv
 *
 * return:void
 */
static inline void wmi_print_ctrl_path_afc_stats_tlv(void *tag_buf)
{
    wmi_ctrl_path_afc_stats_struct *wmi_stats_buf = (wmi_ctrl_path_afc_stats_struct*) tag_buf;

    WMI_STATS_PRINT(FATAL, "WMI_CTRL_PATH_AFC_STATS_TLV:");
    WMI_STATS_PRINT(FATAL, "\n");
    WMI_STATS_PRINT(FATAL, "****General AFC counters****");

    WMI_STATS_PRINT(FATAL, "Total request ID = %u",
        wmi_stats_buf->request_id_count);

    WMI_STATS_PRINT(FATAL, "Total payload count = %u",
        wmi_stats_buf->response_count);

    WMI_STATS_PRINT(FATAL, "Total invalid payload count = %u",
        wmi_stats_buf->invalid_response_count);

    WMI_STATS_PRINT(FATAL, "Total AFC reset count = %u",
        wmi_stats_buf->reset_count);

    WMI_STATS_PRINT(FATAL, "\n");
    WMI_STATS_PRINT(FATAL, "****AFC Payload Response error counters****");

    WMI_STATS_PRINT(FATAL, "Payload id mismatch count = %u",
        wmi_stats_buf->id_mismatch_count);

    WMI_STATS_PRINT(FATAL, "Local error code success = %u",
        wmi_stats_buf->local_err_code_success);

    WMI_STATS_PRINT(FATAL, "Local error code failure = %u",
        wmi_stats_buf->local_err_code_failure);

    WMI_STATS_PRINT(FATAL, "\n");
    WMI_STATS_PRINT(FATAL, "****AFC Server Response error counters****");

    WMI_STATS_PRINT(FATAL, "Code_100 | Version not supported = %u",
        wmi_stats_buf->serv_resp_code_100);

    WMI_STATS_PRINT(FATAL, "Code_101 | Device disallowed = %u",
        wmi_stats_buf->serv_resp_code_101);

    WMI_STATS_PRINT(FATAL, "Code_102 | Missing Param = %u",
        wmi_stats_buf->serv_resp_code_102);

    WMI_STATS_PRINT(FATAL, "Code_103 | Invalid value = %u",
        wmi_stats_buf->serv_resp_code_103);

    WMI_STATS_PRINT(FATAL, "Code_106 | Unexpected param = %u",
        wmi_stats_buf->serv_resp_code_106);

    WMI_STATS_PRINT(FATAL, "Code_300 | Unsupported spectrum = %u",
        wmi_stats_buf->serv_resp_code_300);

    WMI_STATS_PRINT(FATAL, "\n");
    WMI_STATS_PRINT(FATAL, "****AFC Compliance tracker****");

    WMI_STATS_PRINT(FATAL, "Proxy_standalone 0 = %u",
        wmi_stats_buf->proxy_standalone_0);

    WMI_STATS_PRINT(FATAL, "Proxy_standalone 1 = %u",
        wmi_stats_buf->proxy_standalone_1);

    WMI_STATS_PRINT(FATAL, "Successful power event sent count = %u",
        wmi_stats_buf->power_event_counter);

    WMI_STATS_PRINT(FATAL, "Force LPI switch count = %u",
        wmi_stats_buf->force_LPI_counter);

    WMI_STATS_PRINT(FATAL, "TPC WMI success count = %u",
        wmi_stats_buf->tpc_wmi_success_count);

    WMI_STATS_PRINT(FATAL, "TPC WMI failure count = %u",
        wmi_stats_buf->tpc_wmi_failure_count);

    WMI_STATS_PRINT(FATAL, "\n");
    WMI_STATS_PRINT(FATAL, "****AFC Regulatory Compliance check counter****");

    WMI_STATS_PRINT(FATAL, "psd failure = %u",
        wmi_stats_buf->psd_failure_count);

    WMI_STATS_PRINT(FATAL, "psd end freq failure = %u",
        wmi_stats_buf->psd_end_freq_failure_count);

    WMI_STATS_PRINT(FATAL, "psd start freq failure = %u",
        wmi_stats_buf->psd_start_freq_failure_count);

    WMI_STATS_PRINT(FATAL, "eirp failure = %u",
        wmi_stats_buf->eirp_failure_count);

    WMI_STATS_PRINT(FATAL, "centre freq failure = %u",
        wmi_stats_buf->cfreq_failure_count);

    WMI_STATS_PRINT(FATAL, "\n");
    WMI_STATS_PRINT(FATAL, "****AFC Miscellaneous stats****");

    WMI_STATS_PRINT(FATAL, "Current request ID = %u",
        wmi_stats_buf->request_id);

    WMI_STATS_PRINT(FATAL, "Grace timer count = %u",
        wmi_stats_buf->grace_timer_count);

    WMI_STATS_PRINT(FATAL, "Current TTL timer = %u seconds",
        wmi_stats_buf->cur_ttl_timer);

    WMI_STATS_PRINT(FATAL, "Deployment mode = %u (%s)",
        wmi_stats_buf->deployment_mode,
        (wmi_stats_buf->deployment_mode == 1) ? "indoor" :
            (wmi_stats_buf->deployment_mode == 2) ? "outdoor" : "(unknown)");

    WMI_STATS_PRINT(FATAL, "Total AFC-Response Payload clear count = %u",
        wmi_stats_buf->payload_clear_count);

    WMI_STATS_PRINT(FATAL, "\n");
}

/*
 * wmi_print_ctrl_path_cfr_stats_tlv: function to select the tag type and
 * print the corresponding tag structure
 * @tag_buf: pointer to the tag structure
 *
 * return: void
 */
static inline void wmi_print_ctrl_path_cfr_stats_tlv(void *tag_buf)
{
    wmi_ctrl_path_cfr_stats_struct *wmi_stats_buf = (wmi_ctrl_path_cfr_stats_struct*) tag_buf;

    if (wmi_stats_buf->index == 0) {
        WMI_STATS_PRINT(FATAL, "WMI_CTRL_PATH_CFR_STATS_TLV:");
        /* Print tabe header */
        WMI_STATS_PRINT(FATAL,
            "|------------------------------------------------------------"
            "-------------------------------------------------------------"
            "----------------------------------------------|");
        WMI_STATS_PRINT(FATAL,
            "| %.10s | %.17s \t\t|\t %.16s \t\t\t|\t%.16s \t\t|"
            " \t\t%.16s \t\t\t\t| \t%.16s\t\t|",
            "Index", "PeerMac", "Cofig", "Status", "Counts", "TimeStamp(ms)");
        WMI_STATS_PRINT(FATAL,
            "|       |       \t\t|\t       \t\t\t|\t     \t\t|"
            "       \t\t\t\t\t|       \t\t\t|");
        WMI_STATS_PRINT(FATAL,
            "|       |       \t\t| %.16s \t| %.16s \t| %.16s |"
            " %.16s \t| %.16s \t| %.16s \t| %.16s \t| %.16s \t\t| %.16s \t\t|",
            "Method", "Periodicity", "InUse", "InProgress",
            "Requests", "Success", "Failure", "Req", "Resp");
        WMI_STATS_PRINT(FATAL,
            "|------------------------------------------------------------"
            "-------------------------------------------------------------"
            "----------------------------------------------|");
    }
    /* Print stats in a row */
    WMI_STATS_PRINT(FATAL,
        "|%4u\t| %02x:%02x:%02x:%02x:%02x:%02x\t| %8u\t| %8u\t| %4u\t|"
        " %4u\t\t| %8u\t| %8u\t|%8u\t| %8u\t| %8u\t|",
        wmi_stats_buf->index,
        (wmi_stats_buf->cfr_peer_mac_addr.mac_addr31to0  & 0xFF),
        (wmi_stats_buf->cfr_peer_mac_addr.mac_addr31to0 & 0xFF00) >> 8,
        (wmi_stats_buf->cfr_peer_mac_addr.mac_addr31to0 & 0xFF0000) >> 16,
        (wmi_stats_buf->cfr_peer_mac_addr.mac_addr31to0 & 0xFF000000) >> 24,
        (wmi_stats_buf->cfr_peer_mac_addr.mac_addr47to32 & 0xFF),
        (wmi_stats_buf->cfr_peer_mac_addr.mac_addr47to32 & 0xFF00) >> 8,
        wmi_stats_buf->capture_method,
        wmi_stats_buf->periodicity,
        wmi_stats_buf->peer_in_use,
        wmi_stats_buf->capture_in_progress,
        wmi_stats_buf->cfr_req_count,
        wmi_stats_buf->cfr_resp_success_count,
        wmi_stats_buf->cfr_resp_failure_count,
        wmi_stats_buf->latest_req_timestamp,
        wmi_stats_buf->latest_resp_timestamp);
    WMI_STATS_PRINT(FATAL,
        "|------------------------------------------------------------"
        "-------------------------------------------------------------"
        "----------------------------------------------|");
}

/*
 * wmi_print_ctrl_path_t2lm_stats_tlv: function to select the tag type and
 * print the corresponding tag structure
 * @tag_buf: pointer to the tag structure
 *
 * return: void
 */
static inline void wmi_print_ctrl_path_t2lm_stats_tlv(void *tag_buf)
{
    wmi_ctrl_path_t2lm_stats_struct *wmi_stats_buf =
        (wmi_ctrl_path_t2lm_stats_struct *)tag_buf;

    unsigned  i;
    A_UINT16 index_cmd                 = 0;
    A_UINT16 index_evt_map_swt_tme_tsf = 0;
    A_UINT16 index_evt_map_swt_tme_exp = 0;
    A_UINT16 index_evt_exp_dur_exp     = 0;
    A_CHAR   fw_bcast_t2lm_wmi_cmd[WMI_BCAST_T2LM_MAX_STRING_LEN] = {0};
    A_CHAR   fw_bcast_t2lm_wmi_evt_map_swt_tme_tsf[WMI_BCAST_T2LM_MAX_STRING_LEN] = {0};
    A_CHAR   fw_bcast_t2lm_wmi_evt_map_swt_tme_exp[WMI_BCAST_T2LM_MAX_STRING_LEN] = {0};
    A_CHAR   fw_bcast_t2lm_wmi_evt_exp_dur_exp[WMI_BCAST_T2LM_MAX_STRING_LEN] = {0};

    WMI_STATS_PRINT(FATAL, "WMI_CTRL_PATH_T2LM_STATS_TLV:");

    for (i = 0; i < WMI_BCAST_T2LM_MAX; i++) {
        index_cmd += snprintf(
            &fw_bcast_t2lm_wmi_cmd[index_cmd],
            WMI_BCAST_T2LM_MAX_STRING_LEN - index_cmd,
            " %u:%u,", i,
            wmi_stats_buf->bcast_t2lm_wmi_cmd[i]);
        index_evt_map_swt_tme_tsf += snprintf(
            &fw_bcast_t2lm_wmi_evt_map_swt_tme_tsf[index_evt_map_swt_tme_tsf],
            WMI_BCAST_T2LM_MAX_STRING_LEN - index_evt_map_swt_tme_tsf,
            " %u:%u,", i,
            wmi_stats_buf->bcast_t2lm_wmi_evt_map_swt_tme_tsf[i]);
        index_evt_map_swt_tme_exp += snprintf(
            &fw_bcast_t2lm_wmi_evt_map_swt_tme_exp[index_evt_map_swt_tme_exp],
            WMI_BCAST_T2LM_MAX_STRING_LEN - index_evt_map_swt_tme_exp,
            " %u:%u,", i,
            wmi_stats_buf->bcast_t2lm_wmi_evt_map_swt_tme_exp[i]);
        index_evt_exp_dur_exp += snprintf(
            &fw_bcast_t2lm_wmi_evt_exp_dur_exp[index_evt_exp_dur_exp],
            WMI_BCAST_T2LM_MAX_STRING_LEN - index_evt_exp_dur_exp,
            " %u:%u,", i,
            wmi_stats_buf->bcast_t2lm_wmi_evt_exp_dur_exp[i]);
    }
    WMI_STATS_PRINT(FATAL,
        "fw_bcast_t2lm_wmi_cmd = %s \n",
        fw_bcast_t2lm_wmi_cmd);
    WMI_STATS_PRINT(FATAL,
        "fw_bcast_t2lm_wmi_evt_map_swt_tme_tsf = %s \n",
        fw_bcast_t2lm_wmi_evt_map_swt_tme_tsf);
    WMI_STATS_PRINT(FATAL,
        "fw_bcast_t2lm_wmi_evt_map_swt_tme_exp = %s \n",
        fw_bcast_t2lm_wmi_evt_map_swt_tme_exp);
    WMI_STATS_PRINT(FATAL,
        "fw_bcast_t2lm_wmi_evt_exp_dur_exp = %s \n",
        fw_bcast_t2lm_wmi_evt_exp_dur_exp);
}

/*
 * wmi_print_ctrl_path_blanking_stats_tlv: function to select the tag type and
 * print the corresponding tag structure
 * @tag_buf: pointer to the tag structure
 *
 * return: void
 */
static inline void wmi_print_ctrl_path_blanking_stats_tlv(void *tag_buf)
{
    wmi_ctrl_path_blanking_stats_struct *wmi_stats_buf =
        (wmi_ctrl_path_blanking_stats_struct *)tag_buf;

    WMI_STATS_PRINT(FATAL, "WMI_CTRL_PATH_BLANKING_STATS_TLV:");

    WMI_STATS_PRINT(FATAL, "blanking_mode = %u",
                wmi_stats_buf->blanking_mode);
    WMI_STATS_PRINT(FATAL, "is_blanking_enabled = %u",
                wmi_stats_buf->is_blanking_enabled);
    WMI_STATS_PRINT(FATAL, "gate_2g_enabled = %u",
                wmi_stats_buf->gate_2g_enabled);
    WMI_STATS_PRINT(FATAL, "gate_5g_enabled = %u",
                wmi_stats_buf->gate_5g_enabled);
    WMI_STATS_PRINT(FATAL, "gate_6g_enabled = %u",
                wmi_stats_buf->gate_6g_enabled);
    WMI_STATS_PRINT(FATAL, "blanking_count = %u",
                wmi_stats_buf->blanking_count);
    WMI_STATS_PRINT(FATAL, "blanking_duration(us) = %u",
                wmi_stats_buf->blanking_duration);
}

/*
 »
 * wmi_print_ctrl_path_peer_stats_tlv: function to select the tag type and
 * print the corresponding tag structure
 * @tag_buf: pointer to the tag structure
 *
 * return: void
 */
static inline void wmi_print_ctrl_path_peer_stats_tlv(void *tag_buf)
{
    int i;
    A_UINT32 mac_addr31to0, mac_addr47to32;
    wmi_ctrl_path_peer_stats_struct *wmi_stats_buf =
        (wmi_ctrl_path_peer_stats_struct*) tag_buf;

    WMI_STATS_PRINT(FATAL, "WMI_CTRL_PATH_PEER_STATS_TLV: (peer_id:%d)",
        WMI_PEER_STATS_PEER_ID_GET(wmi_stats_buf->opaque_debug_id_type));

    WMI_STATS_PRINT(FATAL, "\nWLAN_PEER_PARAMS: (peer_id:%d)",
        WMI_PEER_STATS_PEER_ID_GET(wmi_stats_buf->opaque_debug_id_type));
    mac_addr31to0  = wmi_stats_buf->mac_addr.mac_addr31to0;
    mac_addr47to32 = wmi_stats_buf->mac_addr.mac_addr47to32;
    WMI_STATS_PRINT(FATAL, "wlan_peer_mac_addr = %02x:%02x:%02x:%02x:%02x:%02x",
        (mac_addr31to0  & 0x000000FF),
        (mac_addr31to0  & 0x0000FF00) >> 8,
        (mac_addr31to0  & 0x00FF0000) >> 16,
        (mac_addr31to0  & 0xFF000000) >> 24,
        (mac_addr47to32 & 0x000000FF),
        (mac_addr47to32 & 0x0000FF00) >> 8);
    WMI_STATS_PRINT(FATAL, "wlan_peer_ni_flags = 0x%x",
        wmi_stats_buf->opaque_debug_flags);
    WMI_STATS_PRINT(FATAL, "wlan_peer_ni_flags_ext = 0x%x",
        wmi_stats_buf->opaque_debug_flags_ext);
    WMI_STATS_PRINT(FATAL, "wlan_peer_delete_in_progress = %u",
        WMI_PEER_STATS_DELETE_IN_PROGRESS_GET(
            wmi_stats_buf->opaque_debug_wlan_peer_bitfield_mask));
    WMI_STATS_PRINT(FATAL, "wlan_peer_send_del_resp_tohost = %u",
        WMI_PEER_STATS_DEL_RESP_TO_HOST_GET(
            wmi_stats_buf->opaque_debug_wlan_peer_bitfield_mask));
    WMI_STATS_PRINT(FATAL, "wlan_peer_peer_migration_in_progress = %u",
        WMI_PEER_STATS_MIGRATION_IN_PROGRESS_GET(
            wmi_stats_buf->opaque_debug_wlan_peer_bitfield_mask));
    WMI_STATS_PRINT(FATAL, "wlan_peer_conn_state = %u",
        WMI_PEER_STATS_CONN_STATE_GET(
            wmi_stats_buf->opaque_debug_wlan_peer_bitfield_mask));
    WMI_STATS_PRINT(FATAL, "wlan_peer_assoc_id = %u",
        WMI_PEER_STATS_ASSOCIATE_ID_GET(
            wmi_stats_buf->opaque_debug_assoc_id_usage_cnt));
    WMI_STATS_PRINT(FATAL, "wlan_peer_usage_count = %u",
        WMI_PEER_STATS_USAGE_CNT_GET(
            wmi_stats_buf->opaque_debug_assoc_id_usage_cnt));
    WMI_STATS_PRINT(FATAL, "wlan_peer_inactivity_generation = %u sec",
        WMI_PEER_STATS_INACT_GEN_GET(
            wmi_stats_buf->opaque_debug_inact_gen));
    WMI_STATS_PRINT(FATAL, "wlan_peer_data_inactivity_generation = %u sec",
        WMI_PEER_STATS_DATA_INACT_GEN_GET(
            wmi_stats_buf->opaque_debug_inact_gen));
    WMI_STATS_PRINT(FATAL, "wlan_peer_type = %u",
        WMI_PEER_STATS_PEER_TYPE_GET(wmi_stats_buf->opaque_debug_id_type));
    WMI_STATS_PRINT(FATAL, "wlan_peer_mac_id = %u",
        WMI_PEER_STATS_MAC_ID_GET(wmi_stats_buf->opaque_debug_id_type));
    WMI_STATS_PRINT(FATAL, "wlan_peer_peer_id = %u",
        WMI_PEER_STATS_PEER_ID_GET(wmi_stats_buf->opaque_debug_id_type));

    WMI_STATS_PRINT(FATAL, "\nWAL_PEER_PARAMS: (peer_id:%d)",
        WMI_PEER_STATS_PEER_ID_GET(wmi_stats_buf->opaque_debug_id_type));
    WMI_STATS_PRINT(FATAL, "wal_peer_deleted_tidmask = 0x%x",
        wmi_stats_buf->opaque_debug_deleted_tidmask);
    WMI_STATS_PRINT(FATAL, "wal_peer_num_of_local_frames_pending = %u",
        wmi_stats_buf->opaque_debug_num_of_local_frames_pending);
    WMI_STATS_PRINT(FATAL, "wal_peer_flags = 0x%x",
        wmi_stats_buf->opaque_debug_wal_peer_flags);
    WMI_STATS_PRINT(FATAL, "wal_peer_keyid0_ast_index = %u",
        wmi_stats_buf->opaque_debug_keyid0_ast_index);
    WMI_STATS_PRINT(FATAL, "wal_peer_all_tids_block_module_bitmap = 0x%x",
        wmi_stats_buf->opaque_debug_all_tids_block_module_bitmap);
    WMI_STATS_PRINT(FATAL, "wal_peer_all_tids_pause_module_bitmap = 0x%x",
        wmi_stats_buf->opaque_debug_all_tids_pause_module_bitmap);
    WMI_STATS_PRINT(FATAL, "wal_peer_data_tids_block_module_bitmap = 0x%x",
        wmi_stats_buf->opaque_debug_data_tids_block_module_bitmap);
    WMI_STATS_PRINT(FATAL, "wal_peer_data_tids_pause_module_bitmap = 0x%x",
        wmi_stats_buf->opaque_debug_data_tids_pause_module_bitmap);
    WMI_STATS_PRINT(FATAL, "wal_peer_ppdu_fail_time = %u us",
        wmi_stats_buf->opaque_debug_ppdu_fail_time);
    WMI_STATS_PRINT(FATAL, "wal_peer_consecutive_null_failure = %u",
        wmi_stats_buf->opaque_debug_consecutive_null_failure);
    WMI_STATS_PRINT(FATAL, "wal_peer_delete_sm_state = %u",
        wmi_stats_buf->opaque_debug_peer_delete_sm_state);
    WMI_STATS_PRINT(FATAL, "wal_peer_mcbc_tids_pause_bitmap = 0x%x",
        wmi_stats_buf->opaque_debug_mcbc_tids_pause_bitmap);
    WMI_STATS_PRINT(FATAL, "wal_peer_delete_all_in_progress = %u",
        WMI_PEER_STATS_DELETE_ALL_FLAG_GET(
            wmi_stats_buf->opaque_debug_peer_delete_rc4_rekey));
    WMI_STATS_PRINT(FATAL, "wal_peer_rsvd_var 0x%x",
        wmi_stats_buf->opaque_debug_wal_peer_rt_flags);
    WMI_STATS_PRINT(FATAL, "wal_peer_qos_null_enqueue_over_wmi %u",
        WMI_PEER_STATS_QOS_NULL_OVER_WMI_GET(
            wmi_stats_buf->opaque_debug_wal_peer_bitfields));
    WMI_STATS_PRINT(FATAL, "wal_peer_new_assoc %u",
        WMI_PEER_STATS_NEW_ASSOC_GET(
            wmi_stats_buf->opaque_debug_wal_peer_bitfields));

    WMI_STATS_PRINT(FATAL, "\nQCACHE_PEER_PARAMS: (peer_id:%d)",
        WMI_PEER_STATS_PEER_ID_GET(wmi_stats_buf->opaque_debug_id_type));
    WMI_STATS_PRINT(FATAL, "qpeer_flags = 0x%x",
        wmi_stats_buf->opaque_debug_qpeer_flags);
    WMI_STATS_PRINT(FATAL, "qpeer_delete_requested_tidmask = 0x%x",
        wmi_stats_buf->opaque_debug_delete_requested_tidmask);
    WMI_STATS_PRINT(FATAL, "qpeer_tid_created_tidmask = 0x%x",
        wmi_stats_buf->opaque_debug_tid_created_tidmask);
    WMI_STATS_PRINT(FATAL, "qpeer_event_bitmap = 0x%x",
        wmi_stats_buf->opaque_debug_event_bitmap);
    WMI_STATS_PRINT(FATAL, "qpeer_rt_rsvd_var0 = 0x%x",
        wmi_stats_buf->opaque_debug_qpeer_rt_flags0);
    WMI_STATS_PRINT(FATAL, "qpeer_rt_rsvd_var1 = 0x%x",
        wmi_stats_buf->opaque_debug_qpeer_rt_flags1);
    WMI_STATS_PRINT(FATAL, "qpeer_rt_rsvd_var2 = 0x%x",
        wmi_stats_buf->opaque_debug_qpeer_rt_flags2);
    WMI_STATS_PRINT(FATAL, "qpeer_sched_algo_var_rsvd = 0x%x",
        wmi_stats_buf->opaque_debug_qpeer_sa_flags0);
    WMI_STATS_PRINT(FATAL, "qpeer_sched_algo_rsv1 = 0x%x",
        wmi_stats_buf->opaque_debug_qpeer_sa_flags1);
    WMI_STATS_PRINT(FATAL, "qpeer_be_var_rsvd = 0x%x",
        wmi_stats_buf->opaque_debug_qpeer_be_flags);

    WMI_STATS_PRINT(FATAL, "\nDCACHE_PEER_PARAMS: (peer_id:%d)",
        WMI_PEER_STATS_PEER_ID_GET(wmi_stats_buf->opaque_debug_id_type));
    WMI_STATS_PRINT(FATAL, "dpeer_seq_no_tx_fail_cnt = %u",
        wmi_stats_buf->opaque_debug_seq_no_tx_fail_cnt);
    WMI_STATS_PRINT(FATAL, "dpeer_fake_sleep_time %u ms",
        wmi_stats_buf->opaque_debug_fake_sleep_time);
    WMI_STATS_PRINT(FATAL, "dpeer_tx_frame_ctrl 0x%x",
        WMI_PEER_STATS_TX_FRAME_CTRL_GET(
            wmi_stats_buf->opaque_debug_tx_frame_qos_ctrl));
    WMI_STATS_PRINT(FATAL, "dpeer_tx_qos_ctrl 0x%x",
        WMI_PEER_STATS_TX_QOS_CTRL_GET(
            wmi_stats_buf->opaque_debug_tx_frame_qos_ctrl));
    WMI_STATS_PRINT(FATAL, "dpeer_consecutive_failure %u",
        WMI_PEER_STATS_CONSEC_FAIL_GET(
            wmi_stats_buf->opaque_debug_consec_fail_subfrm_sz));
    WMI_STATS_PRINT(FATAL, "dpeer_tx_fail_cnt %u",
        WMI_PEER_STATS_TX_FAIL_CNT_GET(
            wmi_stats_buf->opaque_debug_tx_fail_partial_aid));
    WMI_STATS_PRINT(FATAL, "dpeer_tx_partial_aid %u",
        WMI_PEER_STATS_TX_PARTIAL_AID_GET(
            wmi_stats_buf->opaque_debug_tx_fail_partial_aid));
    WMI_STATS_PRINT(FATAL, "dpeer_tac_flags 0x%x",
        wmi_stats_buf->opaque_debug_dcache_tac_flags);
    WMI_STATS_PRINT(FATAL, "dpeer_rt_flags 0x%x",
        wmi_stats_buf->opaque_debug_dcache_rt_flags);

    WMI_STATS_PRINT(FATAL, "\nTWT_PARAMS: (peer_id:%d)",
        WMI_PEER_STATS_PEER_ID_GET(wmi_stats_buf->opaque_debug_id_type));
    WMI_STATS_PRINT(FATAL, "twt_flush_tidmap = 0x%x",
        wmi_stats_buf->opaque_debug_twt_flush_tidmap);
    WMI_STATS_PRINT(FATAL, "n_TWT_SPs_to_expire = %u",
        wmi_stats_buf->opaque_debug_n_TWT_SPs_to_expire);
    WMI_STATS_PRINT(FATAL, "twt_flush_expiry_timestamp = %u us",
        wmi_stats_buf->opaque_debug_twt_flush_expiry_timestamp);
    WMI_STATS_PRINT(FATAL, "twt_flags.u16 = 0x%x",
        WMI_PEER_STATS_TWT_AP_FLAGS_GET(
            wmi_stats_buf->opaque_debug_twt_ap_peer_ctx_flags));
    WMI_STATS_PRINT(FATAL, "twt_session_count = %u",
        WMI_PEER_STATS_TWT_AP_SESSION_CNT_GET(
            wmi_stats_buf->opaque_debug_twt_ap_peer_ctx_flags));
    WMI_STATS_PRINT(FATAL, "twt_frame_retry_count = %u",
        WMI_PEER_STATS_TWT_FRM_RETRY_GET(
            wmi_stats_buf->opaque_debug_twt_ap_peer_ctx_flags));
    WMI_STATS_PRINT(FATAL, "twt_announ_ul_frame_trigger_count = %u",
        WMI_PEER_STATS_TWT_UL_TRIGGER_GET(
            wmi_stats_buf->opaque_debug_twt_ap_counters));
    WMI_STATS_PRINT(FATAL, "twt_b_session_count = %u",
        WMI_PEER_STATS_TWT_BC_SESSION_GET(
            wmi_stats_buf->opaque_debug_twt_ap_counters));
    WMI_STATS_PRINT(FATAL, "twt_pending_reporting_count = %u",
        WMI_PEER_STATS_TWT_PENDING_REPORT_GET(
            wmi_stats_buf->opaque_debug_twt_ap_counters));
    WMI_STATS_PRINT(FATAL, "twt_flow_ids = %u",
        WMI_PEER_STATS_TWT_FLOW_IDS_GET(
            wmi_stats_buf->opaque_debug_twt_ap_counters));
    WMI_STATS_PRINT(FATAL, "twt_is_twtfilter_disabled %u",
        WMI_PEER_STATS_TWT_FILT_FLAG_GET(
            wmi_stats_buf->opaque_debug_wal_peer_bitfields));
    WMI_STATS_PRINT(FATAL, "twt_is_twt_registered %u",
        WMI_PEER_STATS_TWT_REG_FLAG_GET(
            wmi_stats_buf->opaque_debug_wal_peer_bitfields));
    WMI_STATS_PRINT(FATAL, "twt_is_wmm_txq_ul_trig_disabled %u",
        WMI_PEER_STATS_WMM_UL_TRIG_FLAG_GET(
            wmi_stats_buf->opaque_debug_wal_peer_bitfields));

    WMI_STATS_PRINT(FATAL, "\nRC_PARAMS: (peer_id:%d)",
        WMI_PEER_STATS_PEER_ID_GET(wmi_stats_buf->opaque_debug_id_type));
    WMI_STATS_PRINT(FATAL, "rc_flags = 0x%x",
        wmi_stats_buf->opaque_debug_rc_flags);
    WMI_STATS_PRINT(FATAL, "rc_vht_caps = 0x%x",
        wmi_stats_buf->opaque_debug_vht_caps);
    WMI_STATS_PRINT(FATAL, "rc_caps = 0x%x",
        WMI_PEER_STATS_DEF_CAPS_GET(
            wmi_stats_buf->opaque_debug_default_ht_caps));
    WMI_STATS_PRINT(FATAL, "rc_ht_caps = 0x%x",
        WMI_PEER_STATS_HT_CAPS_GET(
            wmi_stats_buf->opaque_debug_default_ht_caps));
    WMI_STATS_PRINT(FATAL, "rc_valid_tx_chainmask_160 = 0x%x",
        WMI_PEER_STATS_TX_CHAIN_MASK_160_GET(
            wmi_stats_buf->opaque_debug_wlan_peer_bitfield_mask));
    WMI_STATS_PRINT(FATAL, "rc_valid_tx_chainmask = 0x%x",
        WMI_PEER_STATS_TX_CHAIN_MASK_GET(
            wmi_stats_buf->opaque_debug_wlan_peer_bitfield_mask));
    WMI_STATS_PRINT(FATAL, "rc_assoc_tx_chainmask = 0x%x",
        WMI_PEER_STATS_ASSOC_CHAIN_MASK_GET(
            wmi_stats_buf->opaque_debug_wlan_peer_bitfield_mask));
    WMI_STATS_PRINT(FATAL, "rc_bss_non_data_rc = %u",
        WMI_PEER_STATS_BSS_NON_DATA_RC_GET(
            wmi_stats_buf->opaque_debug_rate_params));
    WMI_STATS_PRINT(FATAL, "rc_ch_width = %u",
        WMI_PEER_STATS_CH_WIDTH_GET(wmi_stats_buf->opaque_debug_rate_params));
    WMI_STATS_PRINT(FATAL, "rc_rxmcs = %u",
        WMI_PEER_STATS_RX_MCS_GET(wmi_stats_buf->opaque_debug_rate_params));
    WMI_STATS_PRINT(FATAL, "rc_cache_rate_info_low32 = 0x%x",
        wmi_stats_buf->opaque_debug_cache_rate_info_low32);
    WMI_STATS_PRINT(FATAL, "rc_cache_rate_info_high32 = 0x%x",
        wmi_stats_buf->opaque_debug_cache_rate_info_high32);
    WMI_STATS_PRINT(FATAL, "rc_peer_nss %u",
        WMI_PEER_STATS_MAX_NSS_GET(wmi_stats_buf->opaque_debug_max_nss));
    WMI_STATS_PRINT(FATAL, "rc_peer_he_cap_info 0x%x",
        wmi_stats_buf->opaque_debug_he_cap_info);
    WMI_STATS_PRINT(FATAL, "rc_peer_he_cap_info_ext 0x%x",
        wmi_stats_buf->opaque_debug_he_cap_info_ext);
    WMI_STATS_PRINT(FATAL, "rc_mode_supported_mask 0x%x",
        wmi_stats_buf->opaque_debug_rc_mode_supported_mask);
    WMI_STATS_PRINT(FATAL, "rc_last_tx_rate_kbps = %u kbps",
        wmi_stats_buf->opaque_debug_last_tx_rate_kbps);
    WMI_STATS_PRINT(FATAL, "rc_peer_nss_160 = %u ",
        WMI_PEER_STATS_NSS_160_GET(
            wmi_stats_buf->opaque_debug_rc_node_params));
    WMI_STATS_PRINT(FATAL, "rc_phymode = %u ",
        WMI_PEER_STATS_RC_PHYMODE_GET(
            wmi_stats_buf->opaque_debug_rc_node_params));
    WMI_STATS_PRINT(FATAL, "rc_ni_legacy_rate = %u ",
        WMI_PEER_STATS_LEGACY_RATE_GET(
            wmi_stats_buf->opaque_debug_rc_node_params));
    WMI_STATS_PRINT(FATAL, "rc_min_data_rate = %u ",
        WMI_PEER_STATS_MIN_DATA_RATE_GET(
            wmi_stats_buf->opaque_debug_rc_node_params1));
    WMI_STATS_PRINT(FATAL, "rc_vht_max_rate = %u ",
        WMI_PEER_STATS_VHT_MAX_RATE_GET(
            wmi_stats_buf->opaque_debug_rc_node_params1));
    WMI_STATS_PRINT(FATAL, "rc_vht_max_streams = %u ",
        WMI_PEER_STATS_VHT_MAX_STREAMS_GET(
            wmi_stats_buf->opaque_debug_rc_node_params1));
    WMI_STATS_PRINT(FATAL, "rc_mhz = %u ",
        WMI_PEER_STATS_RC_CHAN_FREQ_GET(
            wmi_stats_buf->opaque_debug_rc_node_params1));
    WMI_STATS_PRINT(FATAL, "rc_user_start_rate = %u ",
        wmi_stats_buf->opaque_debug_rc_user_start_mcs_rate);
    WMI_STATS_PRINT(FATAL, "rc_ni_vht_mcs_set = %u ",
        wmi_stats_buf->opaque_debug_rc_vht_mcs_set);
    for (i = 0; i < WMI_HE_MAP_COUNT; i++) {
        WMI_STATS_PRINT(FATAL, "rc_he_mcs_set tx = %u for map = %u",
            wmi_stats_buf->opaque_debug_he_mcs_nss_set_tx[i], i);
        WMI_STATS_PRINT(FATAL, "rc_he_mcs_set rx = %u for map = %u",
            wmi_stats_buf->opaque_debug_he_mcs_nss_set_rx[i], i);
    }
    for (i = 0; i < WMI_EHT_MAP_COUNT; i++) {
        WMI_STATS_PRINT(FATAL, "rc_eht_mcs_set tx = %u for map = %u ",
            wmi_stats_buf->opaque_debug_eht_mcs_nss_set_tx[i], i);
        WMI_STATS_PRINT(FATAL, "rc_eht_mcs_set rx = %u for map = %u ",
            wmi_stats_buf->opaque_debug_eht_mcs_nss_set_rx[i], i);
    }

    WMI_STATS_PRINT(FATAL, "\nAP_POWERSAVE_PARAMS: (peer_id:%d)",
        WMI_PEER_STATS_PEER_ID_GET(wmi_stats_buf->opaque_debug_id_type));
    WMI_STATS_PRINT(FATAL, "AP_PS_last_rx_trigger_time = %u us",
        wmi_stats_buf->opaque_debug_last_rx_trigger_time);
    WMI_STATS_PRINT(FATAL, "AP_PS_last_poll_time = %u us",
        wmi_stats_buf->opaque_debug_last_poll_time);
    WMI_STATS_PRINT(FATAL, "AP_PS_oldest_tx_buffered_waiting_ms = %u ms",
        wmi_stats_buf->opaque_debug_oldest_tx_buffered_waiting_ms);
    WMI_STATS_PRINT(FATAL, "AP_PS_last_rxtx_activity = %u ms",
        wmi_stats_buf->opaque_debug_last_rxtx_activity);
    WMI_STATS_PRINT(FATAL, "AP_PS_flag1 = 0x%x",
        wmi_stats_buf->opaque_debug_ps_buf_peer_flag1);
    WMI_STATS_PRINT(FATAL, "AP_PS_flag2 = 0x%x",
        wmi_stats_buf->opaque_debug_ps_buf_peer_flag2);
    WMI_STATS_PRINT(FATAL, "AP_PS_flag3 = 0x%x",
        wmi_stats_buf->opaque_debug_ps_buf_peer_flag3);
    WMI_STATS_PRINT(FATAL, "AP_PS_sm_event_mask = 0x%x",
        WMI_PEER_STATS_SM_MASK_GET(
            wmi_stats_buf->opaque_debug_sm_event_mask_eosp_cnt));
    WMI_STATS_PRINT(FATAL, "AP_PS_eosp_sent_retry_count = %u",
        WMI_PEER_STATS_EOSP_RETRY_CNT_GET(
            wmi_stats_buf->opaque_debug_sm_event_mask_eosp_cnt));

    WMI_STATS_PRINT(FATAL, "\nMLO_PARAMS: (peer_id:%d)",
        WMI_PEER_STATS_PEER_ID_GET(wmi_stats_buf->opaque_debug_id_type));
    WMI_STATS_PRINT(FATAL, "MLO_hw_link_id = %u",
        wmi_stats_buf->opaque_debug_hw_link_id);
    WMI_STATS_PRINT(FATAL, "MLO_wlan_ml_partner_hw_link_id_bitmap = 0x%x",
        wmi_stats_buf->opaque_debug_ml_partner_hw_link_id_bitmap);
    WMI_STATS_PRINT(FATAL, "MLO_link_flags = 0x%x",
        wmi_stats_buf->opaque_debug_link_flags);
    WMI_STATS_PRINT(FATAL, "MLO_ml_peer_id = %u",
        wmi_stats_buf->opaque_debug_ml_peer_id);
    mac_addr31to0  = wmi_stats_buf->opaque_debug_mld_mac_addr.mac_addr31to0;
    mac_addr47to32 = wmi_stats_buf->opaque_debug_mld_mac_addr.mac_addr47to32;
    WMI_STATS_PRINT(FATAL, "MLO_mld_macaddr = %02x:%02x:%02x:%02x:%02x:%02x",
        (mac_addr31to0  & 0x000000FF),
        (mac_addr31to0  & 0x0000FF00) >> 8,
        (mac_addr31to0  & 0x00FF0000) >> 16,
        (mac_addr31to0  & 0xFF000000) >> 24,
        (mac_addr47to32 & 0x000000FF),
        (mac_addr47to32 & 0x0000FF00) >> 8);
    WMI_STATS_PRINT(FATAL, "MLO_wal_ml_peer_num_links = %u",
        WMI_PEER_STATS_NUM_LINKS_GET(
            wmi_stats_buf->opaque_debug_ml_attributes));
    WMI_STATS_PRINT(FATAL, "MLO_wal_ml_peer_id = %u",
        WMI_PEER_STATS_ML_PEER_ID_GET(
            wmi_stats_buf->opaque_debug_ml_attributes));
    WMI_STATS_PRINT(FATAL, "MLO_wal_ml_peer_pri_link_id = %u",
        WMI_PEER_STATS_PRI_LINK_ID_GET(
            wmi_stats_buf->opaque_debug_ml_attributes));
    WMI_STATS_PRINT(FATAL, "MLO_wal_ml_peer_pri_chip_id = %u",
        WMI_PEER_STATS_PRI_CHIP_ID_GET(
            wmi_stats_buf->opaque_debug_ml_attributes));
    WMI_STATS_PRINT(FATAL, "MLO_wal_ml_peer_link_init_count = %u",
        WMI_PEER_STATS_LINK_INIT_CNT_GET(
            wmi_stats_buf->opaque_debug_ml_attributes));
    WMI_STATS_PRINT(FATAL, "MLO_wal_ml_peer_num_local_links = %u",
        WMI_PEER_STATS_NUM_LOCAL_LINKS_GET(
            wmi_stats_buf->opaque_debug_ml_attributes));
    WMI_STATS_PRINT(FATAL, "MLO_wal_ml_peer_chips_bmap = 0x%x",
        WMI_PEER_STATS_CHIPS_BITMAP_GET(
            wmi_stats_buf->opaque_debug_ml_attributes));
    WMI_STATS_PRINT(FATAL, "MLO_wal_ml_peer_flags = 0x%x",
        wmi_stats_buf->opaque_debug_ml_flags);
    for (i = 0; i < WMI_MAX_MLO_LINKS; i++) {
        WMI_STATS_PRINT(FATAL, "MLO_wal_ml_link_info_valid = %u for link %u ",
            WMI_PEER_STATS_ML_PEER_LINK_INFO_VALID_GET(
                wmi_stats_buf->opaque_debug_ml_link_info_flags[i]), i);
        WMI_STATS_PRINT(FATAL, "MLO_wal_ml_link_info_active = %u for link %u",
            WMI_PEER_STATS_ML_PEER_LINK_INFO_ACTIVE_GET(
                wmi_stats_buf->opaque_debug_ml_link_info_flags[i]), i);
        WMI_STATS_PRINT(FATAL, "MLO_wal_ml_link_info_primary = %u for link %u",
            WMI_PEER_STATS_ML_PEER_LINK_INFO_PRI_GET(
                wmi_stats_buf->opaque_debug_ml_link_info_flags[i]), i);
        WMI_STATS_PRINT(FATAL,
            "MLO_wal_ml_link_info_assoc_link = %u for link %u",
            WMI_PEER_STATS_ML_PEER_LINK_INFO_ASSOC_LINK_GET(
                wmi_stats_buf->opaque_debug_ml_link_info_flags[i]), i);
        WMI_STATS_PRINT(FATAL, "MLO_wal_ml_link_chip_id = %u for link %u",
            WMI_PEER_STATS_ML_PEER_LINK_INFO_CHIP_ID_GET(
                wmi_stats_buf->opaque_debug_ml_link_info_flags[i]), i);
        WMI_STATS_PRINT(FATAL,
            "MLO_wal_ml_link_info_ieee_link = %u for link %u",
            WMI_PEER_STATS_ML_PEER_LINK_INFO_IEEE_LINK_GET(
                wmi_stats_buf->opaque_debug_ml_link_info_flags[i]), i);
        WMI_STATS_PRINT(FATAL, "MLO_wal_ml_link_info_hw_link = %u for link %u",
            WMI_PEER_STATS_ML_PEER_LINK_INFO_HW_LINK_GET(
                wmi_stats_buf->opaque_debug_ml_link_info_flags[i]), i);
        WMI_STATS_PRINT(FATAL,
            "MLO_wal_ml_link_info_logical_link = %u for link %u",
            WMI_PEER_STATS_ML_PEER_LINK_INFO_LOGICAL_LINK_GET(
                wmi_stats_buf->opaque_debug_ml_link_info_flags[i]), i);
        WMI_STATS_PRINT(FATAL,
            "MLO_wal_ml_link_info_anchor_link = %u for link %u",
            WMI_PEER_STATS_ML_PEER_LINK_INFO_ANCHOR_LINK_GET(
                wmi_stats_buf->opaque_debug_ml_link_info_flags[i]), i);
        WMI_STATS_PRINT(FATAL,
            "MLO_wal_ml_link_info_master_link = %u for link %u",
            WMI_PEER_STATS_ML_PEER_LINK_INFO_MASTER_LINK_GET(
                wmi_stats_buf->opaque_debug_ml_link_info_flags[i]), i);
        WMI_STATS_PRINT(FATAL, "MLO_wal_ml_link_info_init = %u for link %u",
            WMI_PEER_STATS_ML_PEER_LINK_INFO_INIT_GET(
                wmi_stats_buf->opaque_debug_ml_link_info_flags[i]), i);
        WMI_STATS_PRINT(FATAL, "MLO_wal_ml_link_info_peer_id = %u for link %u",
            WMI_PEER_STATS_ML_PEER_LINK_INFO_PEER_ID_GET(
                wmi_stats_buf->opaque_debug_ml_link_info_id[i]), i);
        WMI_STATS_PRINT(FATAL, "MLO_wal_ml_link_info_vdev_id = %u for link %u",
            WMI_PEER_STATS_ML_PEER_LINK_INFO_VDEV_ID_GET(
                wmi_stats_buf->opaque_debug_ml_link_info_id[i]), i);
        WMI_STATS_PRINT(FATAL,
            "MLO_wal_ml_link_info_pri_tidmask = 0x%x for link %u",
            wmi_stats_buf->opaque_debug_ml_link_info_pri_tidmask[i], i);
    }

    WMI_STATS_PRINT(FATAL, "\nPEER_KEY_PARAMS: (peer_id:%d)",
        WMI_PEER_STATS_PEER_ID_GET(wmi_stats_buf->opaque_debug_id_type));
    WMI_STATS_PRINT(FATAL, "key_rc4_rekey_cnt = %u",
        WMI_PEER_STATS_RC4_REKEY_CNT_GET(
            wmi_stats_buf->opaque_debug_peer_delete_rc4_rekey));
    WMI_STATS_PRINT(FATAL, "key_last_pn_value_low = %u",
        wmi_stats_buf->opaque_debug_next_to_last_pn_low32);
    WMI_STATS_PRINT(FATAL, "key_last_pn_value_high = %u",
        wmi_stats_buf->opaque_debug_next_to_last_pn_high32);
    WMI_STATS_PRINT(FATAL, "key_curr_pn_value_low = %u",
        wmi_stats_buf->opaque_debug_last_pn_low32);
    WMI_STATS_PRINT(FATAL, "key_curr_pn_value_high = %u",
        wmi_stats_buf->opaque_debug_last_pn_high32);
    WMI_STATS_PRINT(FATAL, "key_rc4_eapol_key_complete = %u",
        wmi_stats_buf->opaque_debug_rc4_eapol_key_complete);

    WMI_STATS_PRINT(FATAL, "\nBLOCK_ACK_PARAMS: (peer_id:%d)",
        WMI_PEER_STATS_PEER_ID_GET(wmi_stats_buf->opaque_debug_id_type));
    WMI_STATS_PRINT(FATAL, "BA_tx_state_bmap_low32 = 0x%x",
        wmi_stats_buf->opaque_debug_tx_state_bmap_low32);
    WMI_STATS_PRINT(FATAL, "BA_tx_state_bmap_high32 = 0x%x",
        wmi_stats_buf->opaque_debug_tx_state_bmap_high32);
    WMI_STATS_PRINT(FATAL, "BA_addba_mode = 0x%x",
        WMI_PEER_STATS_ADDBBA_TX_MODE_GET(
            wmi_stats_buf->opaque_debug_addba_mode));
    WMI_STATS_PRINT(FATAL, "BA_response_code = 0x%x",
        WMI_PEER_STATS_ADDBBA_RESP_MODE_GET(
            wmi_stats_buf->opaque_debug_addba_mode));
    WMI_STATS_PRINT(FATAL, "BA_tx_retry_bmap = 0x%x",
        wmi_stats_buf->opaque_debug_tx_retry_bmap);
    WMI_STATS_PRINT(FATAL, "BA_rx_state_bmap = 0x%x",
        wmi_stats_buf->opaque_debug_rx_state_bmap);
    WMI_STATS_PRINT(FATAL, "BA_tx_pending_delba_tid_bmap = 0x%x",
        wmi_stats_buf->opaque_debug_tx_pending_delba_tid_bmap);
    WMI_STATS_PRINT(FATAL, "BA_link_monitor_tid_num = %u",
        wmi_stats_buf->opaque_debug_link_monitor_tid_num);
    WMI_STATS_PRINT(FATAL, "BA_num_active_tid_not_ba_setup %u",
        WMI_PEER_STATS_ACTIVE_NOT_BA_TID_GET(
            wmi_stats_buf->opaque_debug_wal_peer_bitfields));
    WMI_STATS_PRINT(FATAL, "BA_max_amsdu_size %u bytes",
        WMI_PEER_STATS_MAX_AMSDU_SIZE_GET(
            wmi_stats_buf->opaque_debug_amsdu_size));
    WMI_STATS_PRINT(FATAL, "BA_consec_fail_subfrm_sz %u",
        WMI_PEER_STATS_SUBFRAME_SIZE_GET(
            wmi_stats_buf->opaque_debug_consec_fail_subfrm_sz));

    WMI_STATS_PRINT(FATAL, "\nOPAQUE DEBUG FIELDS: (peer_id:%d)",
        WMI_PEER_STATS_PEER_ID_GET(wmi_stats_buf->opaque_debug_id_type));

    WMI_STATS_PRINT(FATAL, "field_1 0x%x",
        wmi_stats_buf->opaque_debug_field_1);
    WMI_STATS_PRINT(FATAL, "field_2 0x%x",
        wmi_stats_buf->opaque_debug_field_2);
    WMI_STATS_PRINT(FATAL, "field_3 0x%x",
        wmi_stats_buf->opaque_debug_field_3);
    WMI_STATS_PRINT(FATAL, "field_4 0x%x",
        wmi_stats_buf->opaque_debug_field_4);
}

/*
 * wmi_stats_print_tag: function to select the tag type and
 * print the corresponding tag structure
 * @tag_type: tag type that is to be printed
 * @tag_buf: pointer to the tag structure
 *
 * return: void
 */
void wmi_stats_print_tag(
        A_UINT32 tag_type,
        void *tag_buf)
{
    switch (tag_type) {
    case WMITLV_TAG_STRUC_wmi_ctrl_path_pdev_stats_struct:
        wmi_print_ctrl_path_pdev_tx_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_mem_stats_struct:
        wmi_print_ctrl_path_mem_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_calibration_stats_struct:
        wmi_print_ctrl_path_calibration_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_dfs_channel_stats_struct:
        wmi_print_ctrl_path_dfs_channel_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_awgn_stats_struct:
        wmi_print_ctrl_path_awgn_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_btcoex_stats_struct:
        wmi_print_ctrl_path_btcoex_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_bmiss_stats_struct:
        wmi_print_ctrl_path_bmiss_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_odd_addr_read_struct:
        wmi_print_ctrl_path_odd_addr_read_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_afc_stats_struct:
        wmi_print_ctrl_path_afc_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_cfr_stats_struct:
        wmi_print_ctrl_path_cfr_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_t2lm_stats_struct:
        wmi_print_ctrl_path_t2lm_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_blanking_stats_struct:
        wmi_print_ctrl_path_blanking_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_vdev_stats_struct:
        wmi_print_ctrl_path_wmi_vdev_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_peer_stats_struct:
        wmi_print_ctrl_path_peer_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_vdev_bcn_tx_stats_struct:
        wmi_print_ctrl_path_vdev_bcn_tx_stats_tlv(tag_buf);
        break;

    case WMITLV_TAG_STRUC_wmi_ctrl_path_pdev_bcn_tx_stats_struct:
        wmi_print_ctrl_path_pdev_bcn_tx_stats_tlv(tag_buf);
        break;


    /* Add cases for newly added stats here */

    default:
        break;
    }
}

#ifndef __KERNEL__

static void wmi_stats_usage(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= USAGE =================\n");
    printf("%s %s\n", argv[0], argv[1]);
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t- ex: wifiX\n");
    printf("\t\t<cmd_id>\t- 1\n");
    printf("\t - optional args\n");
    printf("Example:\n");
    printf("\twifistats wifiX <cmd_id> <optional arg> --wmi\n");
    printf("=========================================\n");
}

static void wmi_stats_usage_pdevstats(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= CONTROL PATH PDEV STATS USAGE =================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", WMI_REQUEST_CTRL_PATH_PDEV_TX_STAT);
    printf("\t - optional args\n");
    printf("\t\t<argument>\t: --action\n");
    printf("\t\t--action\t: followed by ACTION to perform \n");
    printf("\t\t--wmi\n");
    printf("Example:\n");
    printf("1. To Display Control Path Stats\n");
    printf("\t %s %s %d --wmi \n", argv[0], argv[1], WMI_REQUEST_CTRL_PATH_PDEV_TX_STAT);
    printf("2. To Reset Control Path PATH Stats\n");
    printf("\t %s %s %d --action 2 --wmi \n", argv[0], argv[1],
            WMI_REQUEST_CTRL_PATH_PDEV_TX_STAT);
    printf("=========================================\n");
}

static void wmi_stats_usage_vdevstats(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= CONTROL PATH VDEV STATS USAGE =================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", WMI_REQUEST_CTRL_PATH_VDEV_DEBUG_STAT);
    printf("\t - optional args\n");
    printf("\t\t<argument>\t: --action\n");
    printf("\t\t--action\t: followed by ACTION to perform \n");
    printf("\t\t--wmi\n");
    printf("Example:\n");
    printf("1. To Display Control Path Stats\n");
    printf("\t %s %s %d --wmi \n", argv[0], argv[1], WMI_REQUEST_CTRL_PATH_VDEV_DEBUG_STAT);
    printf("2. To Reset Control Path PATH Stats\n");
    printf("\t %s %s %d --action 2 --wmi \n", argv[0], argv[1],
            WMI_REQUEST_CTRL_PATH_VDEV_DEBUG_STAT);
    printf("=========================================\n");
}

static void wmi_stats_usage_memstats(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= CONTROL PATH DYNAMIC MEMORY STATS USAGE =================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", WMI_REQUEST_CTRL_PATH_MEM_STAT);
    printf("\t - optional args\n");
    printf("\t\t<argument>\t: --action\n");
    printf("\t\t--action\t: followed by ACTION to perform \n");
    printf("\t\t--wmi\n");
    printf("Example:\n");
    printf("1. To Display dynamic memory control path stats\n");
    printf("\t %s %s %d --wmi \n", argv[0], argv[1], WMI_REQUEST_CTRL_PATH_MEM_STAT);
    printf("2. To Reset dynamic memory control path stats\n");
    printf("\t %s %s %d --action 2 --wmi \n", argv[0], argv[1],
            WMI_REQUEST_CTRL_PATH_MEM_STAT);
    printf("=========================================\n");
}

static void wmi_stats_usage_calibrationstats(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= CONTROL PATH CALIBRATION STATS USAGE =================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", WMI_REQUEST_CTRL_PATH_CALIBRATION_STAT);
    printf("\t - optional args\n");
    printf("\t\t<argument>\t: --action\n");
    printf("\t\t--action\t: followed by ACTION to perform \n");
    printf("\t\t--wmi\n");
    printf("Example:\n");
    printf("1. To Display Calibration control path stats\n");
    printf("\t %s %s %d --wmi \n", argv[0], argv[1], WMI_REQUEST_CTRL_PATH_CALIBRATION_STAT);
    printf("2. To Reset Calibration control path stats\n");
    printf("\t %s %s %d --action 2 --wmi \n", argv[0], argv[1],
            WMI_REQUEST_CTRL_PATH_CALIBRATION_STAT);
    printf("=========================================\n");
}

static void wmi_stats_usage_dfschannelstats(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= CONTROL PATH DFS CHANNEL STATS USAGE =================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", WMI_REQUEST_CTRL_PATH_DFS_CHANNEL_STAT);
    printf("\t\t--wmi\n");
    printf("Example:\n");
    printf("To Display DFS Channel control path stats\n");
    printf("\t %s %s %d --wmi \n", argv[0], argv[1], WMI_REQUEST_CTRL_PATH_DFS_CHANNEL_STAT);
    printf("=========================================\n");
}

static void wmi_stats_usage_awgn_stats(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= CONTROL PATH AWGN STATS USAGE =================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", WMI_REQUEST_CTRL_PATH_AWGN_STAT);
    printf("\t - optional args\n");
    printf("\t\t<argument>\t: --action\n");
    printf("\t\t--action\t: followed by ACTION to perform \n");
    printf("\t\t--wmi\n");
    printf("Example:\n");
    printf("1. To Display awgn Control Path Stats\n");
    printf("\t %s %s %d --wmi \n", argv[0], argv[1], WMI_REQUEST_CTRL_PATH_AWGN_STAT);
    printf("2. To Reset awgn Control Path PATH Stats\n");
    printf("\t %s %s %d --action 2 --wmi \n", argv[0], argv[1],
            WMI_REQUEST_CTRL_PATH_AWGN_STAT);
    printf("=========================================\n");
}

static void wmi_stats_usage_btcoexstats(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= CONTROL PATH BTCOEX STATS USAGE =================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", WMI_REQUEST_CTRL_PATH_BTCOEX_STAT);
    printf("\t - optional args\n");
    printf("\t\t<argument>\t: --action\n");
    printf("\t\t--action\t: followed by ACTION to perform \n");
    printf("\t\t--wmi\n");
    printf("Example:\n");
    printf("1. To Display BTCOEX control Path Stats\n");
    printf("\t %s %s %d --wmi \n",
        argv[0], argv[1], WMI_REQUEST_CTRL_PATH_BTCOEX_STAT);
    printf("2. To Reset BTCOEX control Path PATH Stats\n");
    printf("\t %s %s %d --action 2 --wmi \n", argv[0], argv[1],
            WMI_REQUEST_CTRL_PATH_BTCOEX_STAT);
    printf("=========================================\n");

}

static void wmi_stats_usage_oddaddrreadstats(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= CONTROL PATH ODD ADDRESS READ STATS USAGE =================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", WMI_REQUEST_CTRL_PATH_ODD_ADDR_READ);
    printf("\t\t--option\t: followed by odd read addr operation to perform \n");
    printf("0 To Configure/ Add Address Space \n");
    printf("1 To Reset/Delete Configured Addresses \n");
    printf("2 Display list of Configured Addresses \n");
    printf("3 Display Data/Value at Configured Addresses \n");
    printf("\t - optional args\n");
    printf("\t\t<argument>\t: --action\n");
    printf("\t\t--action\t: followed by ACTION to perform \n");
    printf("\t\t--wmi\n");
    printf("Example:\n");
    printf("0 To Configure/ Add Address Space \n");
    printf("\t %s %s %d --option 0 <entry to add(1 to %u)> <32bit address in hex format> --wmi \n", argv[0], argv[1], WMI_REQUEST_CTRL_PATH_ODD_ADDR_READ,WMI_MAX_ADDRESS_SPACE);
    printf("1 To Reset/Delete Configured Addresses \n");
    printf("\t %s %s %d --option 1 <entry to delete(1 to %u)> --wmi \n", argv[0], argv[1], WMI_REQUEST_CTRL_PATH_ODD_ADDR_READ,WMI_MAX_ADDRESS_SPACE);
    printf("2 Display list of Configured Addresses \n");
    printf("\t %s %s %d --option 2 --wmi \n", argv[0], argv[1], WMI_REQUEST_CTRL_PATH_ODD_ADDR_READ);
    printf("3 Display Data/Value at Configured Addresses \n");
    printf("\t %s %s %d --option 3 --wmi \n", argv[0], argv[1], WMI_REQUEST_CTRL_PATH_ODD_ADDR_READ);
    printf("=========================================\n");
}

static void wmi_stats_usage_afc_stats(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= CONTROL PATH AFC STATS USAGE =================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", WMI_REQUEST_CTRL_PATH_AFC_STAT);
    printf("\t - optional args\n");
    printf("\t\t<argument>\t: --action\n");
    printf("\t\t--action\t: followed by ACTION to perform \n");
    printf("\t\t--wmi\n");
    printf("Example:\n");
    printf("1. To Display AFC control path stats\n");
    printf("\t %s %s %d --wmi \n",
        argv[0], argv[1], WMI_REQUEST_CTRL_PATH_AFC_STAT);
    printf("2. To Reset AFC control path stats\n");
    printf("\t %s %s %d --action 2 --wmi \n", argv[0], argv[1],
            WMI_REQUEST_CTRL_PATH_AFC_STAT);
    printf("=========================================\n");
}

static void wmi_stats_usage_cfr_stats(A_INT32 argc, A_CHAR *argv[])
{
    printf("========= CONTROL PATH CFR STATS USAGE =================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", WMI_REQUEST_CTRL_PATH_CFR_STAT);
    printf("\t - optional args\n");
    printf("\t\t<argument>\t: --action\n");
    printf("\t\t--action\t: followed by ACTION to perform \n");
    printf("\t\t--wmi\n");
    printf("Example:\n");
    printf("1. To Display CFR control path stats\n");
    printf("\t %s %s %d --wmi \n",
        argv[0], argv[1], WMI_REQUEST_CTRL_PATH_CFR_STAT);
    printf("2. To Reset CFR control path stats\n");
    printf("\t %s %s %d --action 2 --wmi \n", argv[0], argv[1],
        WMI_REQUEST_CTRL_PATH_CFR_STAT);
    printf("=========================================\n");
}

static void wmi_stats_usage_t2lm_stats(A_INT32 argc, A_CHAR *argv[])
{
    printf("========= CONTROL PATH T2LM STATS USAGE =================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", WMI_REQUEST_CTRL_PATH_T2LM_STAT);
    printf("\t - optional args\n");
    printf("\t\t<argument>\t: --action\n");
    printf("\t\t--action\t: followed by ACTION to perform \n");
    printf("\t\t--wmi\n");
    printf("Example:\n");
    printf("1. To Display t2lm Control Path Stats\n");
    printf("\t %s %s %d --wmi \n", argv[0], argv[1], WMI_REQUEST_CTRL_PATH_T2LM_STAT);
    printf("2. To Reset t2lm Control Path PATH Stats\n");
    printf("\t %s %s %d --action 2 --wmi \n", argv[0], argv[1],
            WMI_REQUEST_CTRL_PATH_T2LM_STAT);
    printf("=========================================\n");
}

static void wmi_stats_usage_blanking_stats(A_INT32 argc, A_CHAR *argv[])
{
    printf("========= CONTROL PATH BLANKING STATS USAGE =================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", WMI_REQUEST_CTRL_PATH_BLANKING_STAT);
    printf("\t - optional args\n");
    printf("\t\t<argument>\t: --action\n");
    printf("\t\t--action\t: followed by ACTION to perform \n");
    printf("\t\t--wmi\n");
    printf("Example:\n");
    printf("1. To Display Blanking Control Path Stats\n");
    printf("\t %s %s %d --wmi \n", argv[0], argv[1], WMI_REQUEST_CTRL_PATH_BLANKING_STAT);
    printf("2. To Reset Blanking Control Path PATH Stats\n");
    printf("\t %s %s %d --action 2 --wmi \n", argv[0], argv[1],
            WMI_REQUEST_CTRL_PATH_BLANKING_STAT);
    printf("=========================================\n");
}

static void wmi_stats_usage_peer_stats(A_INT32 argc, A_CHAR *argv[])
{
    printf(
        "========= CONTROL PATH PEER ADDRESS READ STATS USAGE "
        "=================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", WMI_REQUEST_CTRL_PATH_PEER_STAT);
    printf("\t - optional args \n");
    printf("\t\t--peermac\t: followed by mac address of the peer \n");
    printf("\t\t--peerid\t: followed by peer id of the peer \n");
    printf("\t\t--wmi\n");
    printf("Example:\n");
    printf("1. To Display peer control path stats using MAC address\n");
    printf("\t %s %s %d --peermac <MAC_ADDR of the peer> --wmi \n",
        argv[0], argv[1], WMI_REQUEST_CTRL_PATH_PEER_STAT);
    printf("2. To Display peer control path stats using peer id");
    printf("\t %s %s %d --peerid <PEER_ID of the peer> --wmi \n",
        argv[0], argv[1], WMI_REQUEST_CTRL_PATH_PEER_STAT);
    printf("=========================================\n");
}

void wmi_stats_help(
        A_INT32 argc,
        A_CHAR *argv[])
{
    A_INT32 stats_id = atoi(argv[2]);

    if (stats_id == WMI_REQUEST_CTRL_PATH_PDEV_TX_STAT) {
        wmi_stats_usage_pdevstats(argc, argv);
    } else if (stats_id == WMI_REQUEST_CTRL_PATH_MEM_STAT) {
        wmi_stats_usage_memstats(argc, argv);
    } else if (stats_id == WMI_REQUEST_CTRL_PATH_CALIBRATION_STAT) {
        wmi_stats_usage_calibrationstats(argc, argv);
    } else if (stats_id == WMI_REQUEST_CTRL_PATH_DFS_CHANNEL_STAT) {
        wmi_stats_usage_dfschannelstats(argc, argv);
    } else if (stats_id == WMI_REQUEST_CTRL_PATH_AWGN_STAT) {
        wmi_stats_usage_awgn_stats(argc, argv);
    } else if (stats_id == WMI_REQUEST_CTRL_PATH_BTCOEX_STAT) {
        wmi_stats_usage_btcoexstats(argc, argv);
    } else if (stats_id == WMI_REQUEST_CTRL_PATH_ODD_ADDR_READ) {
        wmi_stats_usage_oddaddrreadstats(argc, argv);
    } else if (stats_id == WMI_REQUEST_CTRL_PATH_AFC_STAT) {
        wmi_stats_usage_afc_stats(argc, argv);
    } else if (stats_id == WMI_REQUEST_CTRL_PATH_CFR_STAT) {
        wmi_stats_usage_cfr_stats(argc, argv);
    } else if (stats_id == WMI_REQUEST_CTRL_PATH_T2LM_STAT) {
        wmi_stats_usage_t2lm_stats(argc, argv);
    } else if (stats_id == WMI_REQUEST_CTRL_PATH_BLANKING_STAT) {
        wmi_stats_usage_blanking_stats(argc, argv);
    } else if (stats_id == WMI_REQUEST_CTRL_PATH_VDEV_DEBUG_STAT) {
        wmi_stats_usage_vdevstats(argc, argv);
    } else if (stats_id == WMI_REQUEST_CTRL_PATH_PEER_STAT) {
        wmi_stats_usage_peer_stats(argc, argv);
    } else {
        wmi_stats_usage(argc, argv);
    }
}

/*
 * wmi_stats_buff_alloc: Allocate maximum size buffer
 *
 * return: buff
 */
static void *wmi_stats_buff_alloc(A_INT32 *buff_len)
{
    void *buff = malloc(WMI_CMDS_SIZE_MAX);

    if (!buff) {
        return NULL;
    }
    memset(buff, 0x0, WMI_CMDS_SIZE_MAX);
    *buff_len = WMI_CMDS_SIZE_MAX;

    return buff;
}

/*
 * wmi_stats_buff_free:
 * @buff - Bufferto be freed
 * return:none
 */
static void wmi_stats_buff_free(void *buff)
{
    free(buff);
}

/*
 * wmi_stats_cookie_generate:
 * Generate cookie
 * return: wmistats_cookie
 */
static A_INT32 wmi_stats_cookie_generate(void)
{
    static A_INT32 wmistats_cookie = 0;

    if (!wmistats_cookie) {
        wmistats_cookie = getpid();
    }

    return wmistats_cookie;
}

/*
 * wmi_stats_input_parse: api to be used to parse all input entered by user in CLI
 * @buff - Buffer to be used to fill wmi command
 * @argc - Total number of arguments entered by user
 * @argv[] -- character array of arguments entered by used in CLI interface
 * buff_len -- Pointer to filled, to indicate how much buffer length is filled
 * pdev_id -- Interface id converted to pdev id by host
 * return:
 */
static A_INT32 wmi_stats_input_parse(
        void *buff,
        A_INT32 argc,
        A_CHAR *argv[],
        A_INT32 *buff_len,
        A_INT32 pdev_id)
{
    wmi_request_ctrl_path_stats_cmd_fixed_param *cmd_fixed_param = (wmi_request_ctrl_path_stats_cmd_fixed_param *)buff;
    /*pointer to pdev_id_array*/
    A_UINT32 *pdev_id_array = (A_UINT32 *)(buff + sizeof(wmi_request_ctrl_path_stats_cmd_fixed_param) + WMI_TLV_HDR_SIZE);
    A_UINT32 *vdev_id_array;
    A_UINT32 *mac_addr_array;
    A_UINT32 *dialog_id_array;
    A_UINT32 *odd_options_array;
    A_UINT32 num_pdev_ids = 0;
    A_UINT32 num_vdev_ids = 0;
    A_UINT32 num_mac_addr_list = 0;
    A_UINT32 num_dialog_id_list = 0;
    A_UINT32 num_odd_options = 0;
    A_UINT32 i;
    A_UINT8 *buf_ptr=NULL;
    A_UINT32 stats_id = atoi(argv[2]);
    wmi_mac_addr *pmac_addr_array;
    A_CHAR mac_str[18] = {0};
    A_UINT8 mac_addr[IEEE80211_ADDR_LEN];
    A_UINT32 *peer_id_array;
    A_UINT32 num_peer_ids = 0;

    cmd_fixed_param->stats_id_mask = (1 << atoi(argv[2]));

    /*
     * Initialize pointers to subsequent TLV arrays by assuming that the
     * prior TLV arrays are empty (i.e. they only have an outer TLV header).
     * If num_pdev_ids > 0, then vdev_id_array (and all the following arrays)
     * have to be updated accordingly.
     * If num_vdev_ids > 0, then mac_addr_array (and all the following arrays)
     * have to be updated accordingly.
     * Etc.
     */
    vdev_id_array = (A_UINT32 *)((void *)pdev_id_array +  WMI_TLV_HDR_SIZE);
    mac_addr_array = (A_UINT32 *)((void *)vdev_id_array + WMI_TLV_HDR_SIZE);
    dialog_id_array = (A_UINT32 *)((void *)mac_addr_array + WMI_TLV_HDR_SIZE);
    odd_options_array = (A_UINT32 *)((void *)dialog_id_array + WMI_TLV_HDR_SIZE);
    peer_id_array = (A_UINT32 *)((void *)odd_options_array + WMI_TLV_HDR_SIZE);

    /* generate cookie information */
    cmd_fixed_param->request_id = wmi_stats_cookie_generate();

    if (argc < 4) {
        fprintf(stderr, "Invalid commands args\n");
        return -EIO;
    }


    switch (stats_id) {
    case WMI_REQUEST_CTRL_PATH_PDEV_TX_STAT:
    case WMI_REQUEST_CTRL_PATH_CALIBRATION_STAT:
    case WMI_REQUEST_CTRL_PATH_BTCOEX_STAT:
        /*Use pdev_id passed by host,host will convert interface id to proper pdev id*/
        pdev_id_array[num_pdev_ids] = pdev_id;
        num_pdev_ids++;
        break;

    case WMI_REQUEST_CTRL_PATH_MEM_STAT:
        /* No inputs to parse for MEM stats */
        break;

    case WMI_REQUEST_CTRL_PATH_DFS_CHANNEL_STAT:
        /*
         * Use pdev_id passed by host; host will convert interface id
         * to proper pdev id.
         */
        pdev_id_array[num_pdev_ids] = pdev_id;
        num_pdev_ids++;
        break;

    case WMI_REQUEST_CTRL_PATH_ODD_ADDR_READ:
        /*
         * Use pdev_id passed by host, host will convert interface id
         * to proper pdev id.
         */
        pdev_id_array[num_pdev_ids] = pdev_id;
        num_pdev_ids++;

        vdev_id_array = (A_UINT32 *) ((void *) pdev_id_array +
            WMI_TLV_HDR_SIZE + (sizeof(A_UINT32) * num_pdev_ids));
        mac_addr_array = (A_UINT32 *) ((void *) vdev_id_array +
            WMI_TLV_HDR_SIZE + (sizeof(A_UINT32) * num_vdev_ids));
        dialog_id_array = (A_UINT32 *) ((void *) mac_addr_array +
            WMI_TLV_HDR_SIZE + (sizeof(wmi_mac_addr) * num_mac_addr_list));
        odd_options_array = (A_UINT32 *) ((void *) dialog_id_array +
            WMI_TLV_HDR_SIZE + (sizeof(A_UINT32) * num_dialog_id_list));

        if (argc < 6) {
            return -EIO;
        } else {
            odd_options_array[num_odd_options++] =
                (A_UINT32) strtoul(argv[4], NULL, 0);
            if (argc > 6) {
                odd_options_array[num_odd_options++] =
                    (A_UINT32) strtoul(argv[5], NULL, 0);
                if (argc > 7) {
                    odd_options_array[num_odd_options++] =
                        (A_UINT32) strtoul(argv[6], NULL, 0);
                }
            }
        }

        /* Input Validations */
        switch (odd_options_array[0])
        {
            case WMI_ODD_ADDR_READ_OPTION_TYPE_ADD_ADDR_COMMAND:
                if (argc != 8) {
                    return -EIO;
                } else {
                    if (((odd_options_array[1] < 1) ||
                         (odd_options_array[1] > WMI_MAX_ADDRESS_SPACE)) &&
                        ((odd_options_array[2] & 0xFF000000) == 0))
                    {
                        return -EIO;
                    }
                }
                break;

            case WMI_ODD_ADDR_READ_OPTION_TYPE_DEL_ADDR_COMMAND:
                if (argc != 7) {
                    return -EIO;
                } else {
                    if (odd_options_array[1] > WMI_MAX_ADDRESS_SPACE) {
                        return -EIO;
                    }
                }
                break;

            case WMI_ODD_ADDR_READ_OPTION_TYPE_DISP_ADDR_COMMAND:
            case WMI_ODD_ADDR_READ_OPTION_TYPE_DISP_VAL_COMMAND:
                if (argc > 6) {
                    return -EIO;
                }
                break;

            default:
                return -EIO;
        }
        break;

    case WMI_REQUEST_CTRL_PATH_AWGN_STAT:
        pdev_id_array[num_pdev_ids] = pdev_id;
        num_pdev_ids++;
        break;

    case WMI_REQUEST_CTRL_PATH_AFC_STAT:
        pdev_id_array[num_pdev_ids] = pdev_id;
        num_pdev_ids++;
        break;

    case WMI_REQUEST_CTRL_PATH_CFR_STAT:
        pdev_id_array[num_pdev_ids] = pdev_id;
        num_pdev_ids++;
        break;

    case WMI_REQUEST_CTRL_PATH_T2LM_STAT:
        pdev_id_array[num_pdev_ids] = pdev_id;
        num_pdev_ids++;
        break;

    case WMI_REQUEST_CTRL_PATH_BLANKING_STAT:
        pdev_id_array[num_pdev_ids] = pdev_id;
        num_pdev_ids++;
        break;

    case WMI_REQUEST_CTRL_PATH_VDEV_DEBUG_STAT:
        pdev_id_array[num_pdev_ids] = pdev_id;
        num_pdev_ids++;

        vdev_id_array = (A_UINT32 *) ((void *) pdev_id_array +
            WMI_TLV_HDR_SIZE + (sizeof(A_UINT32) * num_pdev_ids));

        if (strcmp(argv[3], "--vdevid") == 0) {
            vdev_id_array[num_vdev_ids] = (A_UINT32) strtoul(argv[4], NULL, 0);
            num_vdev_ids++;
        } else {
            printf("Specify vdev id: --vdevid : vdev_id of the vap\n");
            return -EIO;
        }
        break;

    case WMI_REQUEST_CTRL_PATH_PEER_STAT:
        pdev_id_array[num_pdev_ids] = pdev_id;
        num_pdev_ids++;

        vdev_id_array = (A_UINT32 *) ((void *) pdev_id_array +
            WMI_TLV_HDR_SIZE + (sizeof(A_UINT32) * num_pdev_ids));

        pmac_addr_array = (wmi_mac_addr *) ((void *) vdev_id_array +
            WMI_TLV_HDR_SIZE + (sizeof(A_UINT32) * num_vdev_ids));
        dialog_id_array = (A_UINT32 *) ((void *) pmac_addr_array +
            WMI_TLV_HDR_SIZE + (sizeof(wmi_mac_addr) * num_mac_addr_list));
        odd_options_array = (A_UINT32 *) ((void *) dialog_id_array +
            WMI_TLV_HDR_SIZE + (sizeof(A_UINT32) * num_dialog_id_list));
        peer_id_array = (A_UINT32 *) ((void *) odd_options_array +
            WMI_TLV_HDR_SIZE + (sizeof(A_UINT32) * num_odd_options));

        if (strcmp(argv[3], "--peermac") == 0) {
            strlcpy(mac_str, argv[4], 18);
            if (extract_mac_addr(mac_addr, mac_str) != 0) {
                printf("Invalid PEER MAC\n");
                return -EIO;
            }
            WMI_CHAR_ARRAY_TO_MAC_ADDR(mac_addr, pmac_addr_array);
            num_mac_addr_list++;
        } else if (strcmp(argv[3], "--peerid") == 0) {
            peer_id_array[num_peer_ids] = (A_UINT32) strtoul(argv[4], NULL, 0);
            num_peer_ids++;

        } else {
            printf(
                "Specify MAC addr : --peermac   : "
                "followed by mac address of the peer or "
                "Specify peer id: --peerid : peer_id of the peer\n");
            return -EIO;
        }

        break;

    case WMI_REQUEST_CTRL_PATH_VDEV_BCN_TX_STAT:
        pdev_id_array[num_pdev_ids] = pdev_id;
        num_pdev_ids++;

        vdev_id_array = (A_UINT32 *) ((void *) pdev_id_array +
            WMI_TLV_HDR_SIZE + (sizeof(A_UINT32) * num_pdev_ids));

        if (strcmp(argv[3], "--vdevid") == 0) {
            vdev_id_array[num_vdev_ids] = (A_UINT32) strtoul(argv[4], NULL, 0);
            num_vdev_ids++;
        }
        break;

    case WMI_REQUEST_CTRL_PATH_PDEV_BCN_TX_STAT:
        pdev_id_array[num_pdev_ids] = pdev_id;
        num_pdev_ids++;
        break;


    /* Add case for newly wmi ctrl path added stats here */

    default:
        printf("Specify correct stats id \n");
        return -EIO;
        break;
    }

    /* By default stats is requested & action field is not passed, process get stats */
    if (argc == 4) {
        if (strcmp(argv[3], "--wmi") == 0) {
            cmd_fixed_param->action = WMI_REQUEST_CTRL_PATH_STAT_GET;
            goto parse_done;
        } else {
            printf("Specify correct arguments \n");
            return -EIO;
        }
    }

    for (i = 3; i < argc; ) {
        if (strcmp(argv[i], "--action") == 0) {
            cmd_fixed_param->action = strtoul(argv[i + 1], NULL, 0);
            i += 2;
        } else if (strcmp(argv[i], "--option") == 0) {
            i = argc - 1;
        } else if (strcmp(argv[i], "--wmi") == 0) {
            if (stats_id == WMI_REQUEST_CTRL_PATH_ODD_ADDR_READ) {
                cmd_fixed_param->action = WMI_REQUEST_CTRL_PATH_STAT_GET;
            }
            goto parse_done;
        } else if (strcmp(argv[i], "--vdevid") == 0) {
            if (stats_id == WMI_REQUEST_CTRL_PATH_VDEV_DEBUG_STAT) {
                cmd_fixed_param->action = WMI_REQUEST_CTRL_PATH_STAT_GET;
            } else if (stats_id == WMI_REQUEST_CTRL_PATH_VDEV_BCN_TX_STAT &&
                       (strcmp(argv[5], "--action") == 0)) {
                cmd_fixed_param->action = strtoul(argv[6], NULL, 0);
            } else if (stats_id == WMI_REQUEST_CTRL_PATH_VDEV_BCN_TX_STAT) {
                cmd_fixed_param->action = WMI_REQUEST_CTRL_PATH_STAT_GET;
            }
            goto parse_done;
        } else if ((strcmp(argv[i], "--peermac") == 0) ||
                   (strcmp(argv[i], "--peerid") == 0))
        {
            if (stats_id == WMI_REQUEST_CTRL_PATH_PEER_STAT) {
                cmd_fixed_param->action = WMI_REQUEST_CTRL_PATH_STAT_GET;
            }
            goto parse_done;
        } else {
           printf("Unsupported option entered\n");
           return -EIO;
        }
    }

    if (cmd_fixed_param->action == WMI_REQUEST_CTRL_PATH_STAT_RESET) {
        wmistats.timeout = 0;
    }

parse_done:

    /* Set TLV header */
    WMITLV_SET_HDR(&cmd_fixed_param->tlv_header, WMITLV_TAG_STRUC_wmi_request_ctrl_path_stats_cmd_fixed_param,WMITLV_GET_STRUCT_TLVLEN(wmi_request_ctrl_path_stats_cmd_fixed_param));

    /* Setting tlv header for pdev id arrays */
    buf_ptr = buff + sizeof(wmi_request_ctrl_path_stats_cmd_fixed_param);
    WMITLV_SET_HDR(buf_ptr,  WMITLV_TAG_ARRAY_UINT32, sizeof(A_UINT32) * num_pdev_ids);

    /* Setting tlv header for vdev id arrays */
     buf_ptr = buf_ptr + WMI_TLV_HDR_SIZE +(sizeof(A_UINT32) * num_pdev_ids);
    WMITLV_SET_HDR(buf_ptr, WMITLV_TAG_ARRAY_UINT32, sizeof(A_UINT32)*num_vdev_ids);

    /* Setting tlv header for mac addr arrays */
    buf_ptr = buf_ptr + WMI_TLV_HDR_SIZE +(sizeof(A_UINT32) * num_vdev_ids);
    WMITLV_SET_HDR(buf_ptr,WMITLV_TAG_ARRAY_FIXED_STRUC, sizeof( wmi_mac_addr)*num_mac_addr_list);

    /* Setting tlv header for dialog_id arrays */
    buf_ptr = buf_ptr + WMI_TLV_HDR_SIZE +(sizeof(wmi_mac_addr) * num_mac_addr_list);
    WMITLV_SET_HDR(buf_ptr,WMITLV_TAG_ARRAY_UINT32, sizeof(A_UINT32)*num_dialog_id_list);

    /* Setting tlv header for odd options arrays */
    buf_ptr = buf_ptr + WMI_TLV_HDR_SIZE +(sizeof(A_UINT32) * num_dialog_id_list);
    WMITLV_SET_HDR(buf_ptr,WMITLV_TAG_ARRAY_UINT32, sizeof(A_UINT32)*num_odd_options);

    /* Setting tlv header for peer id arrays */
    buf_ptr = buf_ptr + WMI_TLV_HDR_SIZE +(sizeof(A_UINT32) * num_odd_options);
    WMITLV_SET_HDR(buf_ptr,WMITLV_TAG_ARRAY_UINT32, sizeof(A_UINT32)*num_peer_ids);

    /* Calculate total buffer length */
    *buff_len =
        sizeof(wmi_request_ctrl_path_stats_cmd_fixed_param) +
        WMI_TLV_HDR_SIZE + (sizeof(A_UINT32)*(num_pdev_ids)) +
        WMI_TLV_HDR_SIZE + (sizeof(A_UINT32)*(num_vdev_ids)) +
        WMI_TLV_HDR_SIZE + (sizeof(wmi_mac_addr)*(num_mac_addr_list)) +
        WMI_TLV_HDR_SIZE +(sizeof(A_UINT32)*(num_dialog_id_list)) +
        WMI_TLV_HDR_SIZE + (sizeof(A_UINT32)*num_odd_options) +
        WMI_TLV_HDR_SIZE + (sizeof(A_UINT32)*num_peer_ids) +
        0;

    return 0;
}

/*
 * wmi_stats_handler: api to be used to parse wmi event and print all the TLV filed in buffer
 * @buff - Buffer containing wmi event
 * len  -- length of event buffer
 * return:
 */
static A_INT32 wmi_stats_handler(
        void *buff,
        A_INT32 len)
{
    A_INT32  status    = LISTEN_CONTINUE;
    A_UINT8* buf_ptr = (A_UINT8*)buff;

    A_UINT32 curr_tlv_tag = WMITLV_GET_TLVTAG(WMITLV_GET_HDR(buf_ptr));
    A_UINT32 curr_tlv_len = WMITLV_GET_TLVLEN(WMITLV_GET_HDR(buf_ptr));

    if (curr_tlv_tag ==  WMITLV_TAG_STRUC_wmi_ctrl_path_stats_event_fixed_param) {
        wmi_ctrl_path_stats_event_fixed_param *ev_fix_param = (wmi_ctrl_path_stats_event_fixed_param *)buff;
        if (!(ev_fix_param->more)) {
          status = LISTEN_DONE;
        }
       /*buffer should point to next TLV in event*/
        buf_ptr += (curr_tlv_len + WMI_TLV_HDR_SIZE) ;
        len -= (curr_tlv_len + WMI_TLV_HDR_SIZE);
    } else {
        printf("wmi_stats_handler Passed wrong buffer \n");
        return status;
    }

    curr_tlv_tag = WMITLV_GET_TLVTAG(WMITLV_GET_HDR(buf_ptr));
    curr_tlv_len = WMITLV_GET_TLVLEN(WMITLV_GET_HDR(buf_ptr));

    while ((len >= curr_tlv_len) && (curr_tlv_tag >= WMITLV_TAG_FIRST_ARRAY_ENUM)) {
        if ((curr_tlv_tag == WMITLV_TAG_ARRAY_UINT32) ||
            (curr_tlv_tag == WMITLV_TAG_ARRAY_BYTE) ||
            (curr_tlv_tag == WMITLV_TAG_ARRAY_STRUC))
        {
            /*buf pointer indicates to header of TLV*/
            buf_ptr += WMI_TLV_HDR_SIZE ;
            len -= WMI_TLV_HDR_SIZE ;
        }
        else if (curr_tlv_len)
        {
            wmi_stats_print_tag(curr_tlv_tag,(void *) buf_ptr);
            buf_ptr += curr_tlv_len + WMI_TLV_HDR_SIZE;
            len -= (curr_tlv_len + WMI_TLV_HDR_SIZE) ;
        }
        curr_tlv_tag = WMITLV_GET_TLVTAG(WMITLV_GET_HDR(buf_ptr));
        curr_tlv_len = WMITLV_GET_TLVLEN(WMITLV_GET_HDR(buf_ptr));
    }

    return status;
}

/*
 * wmi_stats_cookie_get:
 * extract cookie information from event
 * return: wmistats_cookie
 */
static A_INT32 wmi_stats_cookie_get(
        void *buff,
        A_INT32 len)
{
    A_INT32  cookie    = 0;

    wmi_ctrl_path_stats_event_fixed_param *cp_stats_fixed = (wmi_ctrl_path_stats_event_fixed_param*) buff;

    cookie = cp_stats_fixed->request_id; /* Cookie */

    return cookie;
}

int wmi_stats_tlv_length(void *data) {
    return (WMI_TLV_HDR_SIZE + WMITLV_GET_TLVLEN(WMITLV_GET_HDR((A_UINT8*)data)));
}

static void * wmi_stats_get_buf_start(void *buff, A_INT32 *len, int *listen_done) {
    A_UINT8* buf_ptr = (A_UINT8*)buff;
    A_UINT32 curr_tlv_tag = WMITLV_GET_TLVTAG(WMITLV_GET_HDR(buf_ptr));
    A_UINT32 curr_tlv_len = WMITLV_GET_TLVLEN(WMITLV_GET_HDR(buf_ptr));

    if (curr_tlv_tag ==  WMITLV_TAG_STRUC_wmi_ctrl_path_stats_event_fixed_param) {
        wmi_ctrl_path_stats_event_fixed_param *ev_fix_param = (wmi_ctrl_path_stats_event_fixed_param *)buff;
        if (!(ev_fix_param->more)) {
            *listen_done = LISTEN_DONE;
        }
        /*buffer should point to next TLV in event*/
        buf_ptr += (curr_tlv_len + WMI_TLV_HDR_SIZE) ;
        *len -= (curr_tlv_len + WMI_TLV_HDR_SIZE);
    } else {
        printf("wmi_stats_handler Passed wrong buffer \n");
        return NULL;
    }

    curr_tlv_tag = WMITLV_GET_TLVTAG(WMITLV_GET_HDR(buf_ptr));
    curr_tlv_len = WMITLV_GET_TLVLEN(WMITLV_GET_HDR(buf_ptr));

    while ((*len >= curr_tlv_len) && (curr_tlv_tag >= WMITLV_TAG_FIRST_ARRAY_ENUM)) {
        if ((curr_tlv_tag == WMITLV_TAG_ARRAY_UINT32) ||
                (curr_tlv_tag == WMITLV_TAG_ARRAY_BYTE) ||
                (curr_tlv_tag == WMITLV_TAG_ARRAY_STRUC))
        {
            /*buf pointer indicates to header of TLV*/
            buf_ptr += WMI_TLV_HDR_SIZE ;
            *len -= WMI_TLV_HDR_SIZE ;
        }
        else if (curr_tlv_len)
        {
            break;
        }
        curr_tlv_tag = WMITLV_GET_TLVTAG(WMITLV_GET_HDR(buf_ptr));
        curr_tlv_len = WMITLV_GET_TLVLEN(WMITLV_GET_HDR(buf_ptr));
    }
    return buf_ptr;
}

static void wmi_stats_print_tlv(void *tlv_ptr) {
    A_UINT32 *buf_ptr = (A_UINT32 *)tlv_ptr;
    A_UINT32 curr_tlv_tag = WMITLV_GET_TLVTAG(WMITLV_GET_HDR(buf_ptr));
    wmi_stats_print_tag(curr_tlv_tag, tlv_ptr);
}

static struct wifistats_module wmistats = {
    .name                  = "wmi_fw_stats",
    .help                  = wmi_stats_help,
    .input_buff_alloc      = wmi_stats_buff_alloc,
    .input_parse           = wmi_stats_input_parse,
    .input_buff_free       = wmi_stats_buff_free,
    .input_cookie_generate = wmi_stats_cookie_generate,
    .output_handler        = wmi_stats_handler,
    .output_cookie_get     = wmi_stats_cookie_get,
    .output_tlv_length     = wmi_stats_tlv_length,
    .output_get_buf_start  = wmi_stats_get_buf_start,
    .output_print_tlv      = wmi_stats_print_tlv,
    .timeout               = 2000,
    .tlv_hdr_len           = WMI_TLV_HDR_SIZE,
};

void wmistats_init(void)
{
    wifistats_module_register(&wmistats, sizeof(wmistats));
}

void wmistats_fini(void)
{
    wifistats_module_unregister(&wmistats, sizeof(wmistats));
}

#endif /* ifndef __KERNEL__ */
