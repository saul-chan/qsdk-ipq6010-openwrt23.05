/*
 * Copyright (c) 2018-2020 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#define HTT_CTRL_PATH_STATS_CAL_TYPE_STRINGS
#ifdef __KERNEL__
#include "qdf_types.h"
#include "dp_types.h"
#include "dp_internal.h"
#else
#include "wifistats.h"
#endif
#include <stddef.h> /* offsetof */
#include "htt.h"

#define HTT_CTRL_PATH_STATS_CAL_TYPE_STRINGS
#ifndef INLINE
  #define INLINE inline
#endif

#include "htt_stats.h"

#define HTT_TLV_HDR_LEN HTT_T2H_EXT_STATS_CONF_TLV_HDR_SIZE
#define HTT_MAX_STRING_LEN 1000

/*
 * This HTT_CHECK_FOR_SPACE_ON_PRINT_BUFFER is a compile time check
 * creates a array with negative size if the check fails. This
 * failure check occurs during compile time
 *
 */
#define HTT_MAX_PRINT_CHAR_PER_ELEM 15

#ifdef __KERNEL__
#define HTT_CHECK_FOR_SPACE_ON_PRINT_BUFFER(num_elements, print_buffer_length)
#define malloc(size) qdf_mem_malloc(size)
#define free(ptr) qdf_mem_free(ptr)
#define FATAL 1
static qdf_debugfs_file_t file = NULL;
#define HTT_STATS_PRINT(level, fmt, ...)  qdf_debugfs_printf(file, fmt"\n", ##__VA_ARGS__)

static inline A_UINT32 *htt_stats_msg_get(void *data)
{
    struct htt_dbgfs_cfg *dbgfs_cfg = (struct htt_dbgfs_cfg *)data;
    file = dbgfs_cfg->m;
    return dbgfs_cfg->msg_word;
}
#define HTT_STATS_ERR DP_TRACE
#else
#define HTT_CHECK_FOR_SPACE_ON_PRINT_BUFFER(num_elements, print_buffer_length) \
    typedef __attribute__ ((__unused__)) char string_buf[(2 * (print_buffer_length >= (num_elements * HTT_MAX_PRINT_CHAR_PER_ELEM))) - 1];

#define HTT_STATS_PRINT(level, fmt, ...) \
    do { \
        if (httstats.output_fp) { \
            fprintf(httstats.output_fp, fmt,##__VA_ARGS__); \
            fprintf(httstats.output_fp, "\n"); \
        } else { \
            printf(fmt,##__VA_ARGS__); \
            printf("\n"); \
        } \
    } while (0)

#define HTT_STATS_ERR HTT_STATS_PRINT
/*
 * Provide a forward declaration of httstats.
 * (The definition is at the bottom of this file.)
 */
static struct wifistats_module httstats;
#define htt_stats_msg_get(data) data
void __attribute__ ((constructor)) httstats_init(void);
void __attribute__ ((destructor)) httstats_fini(void);
#endif /* ifdef __KERNEL__ */

struct httstats_cmd_request {
    A_UINT32 config_param0;

    A_UINT32 config_param1;
    A_UINT32 config_param2;
    A_UINT32 config_param3;
    A_INT32  pid;
    A_INT8   stats_id;
};

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

/*
 * htt_print_stats_string_tlv: display htt_stats_string_tlv
 * @tag_buf: buffer containing the tlv htt_stats_string_tlv
 *
 * return:void
 */
static void htt_print_stats_string_tlv(A_UINT32 *tag_buf)
{
    htt_stats_string_tlv *htt_stats_buf =
        (htt_stats_string_tlv *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                    = 0;
    A_CHAR   data[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                  = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_STATS_STRING_TLV:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&data[index],
                HTT_MAX_STRING_LEN - index,
                "%.*s", 4, (A_CHAR *)&(htt_stats_buf->data[i]));
    }

    HTT_STATS_PRINT(FATAL, "data = %s\n", data);
}

static void htt_stringify_array(
        A_CHAR *str,
        A_UINT32 str_size,
        A_UINT32 *array,
        A_UINT32 num_array_elements,
        A_UINT32 index_offset,
        A_UINT8 skip_zero_vals )
{
    A_UINT8  i;
    A_UINT16 index = 0;
    A_UINT8  null_output = 1;

    if ( str && str_size ) {
        str[0] = '\0';
    } else {
        return;
    }

    for (i = 0; i < num_array_elements; i++) {
        if ( !skip_zero_vals || array[i] ) {
            null_output = 0;
            index += snprintf(str+index,
                    str_size - index,
                    " %u:%u,", i+index_offset,
                    array[i]);
        }
    }

    if ( null_output && skip_zero_vals ) {
            index += snprintf(str+index,
                    str_size - index,
                    " NONE");
    }
}

A_CHAR g_mgmt_fc_subtype[HTT_STATS_SUBTYPE_MAX][12]={
    "ASSOC_REQ",    /* 0 */
    "ASSOC_RES",    /* 1 */
    "REASSOC_REQ",  /* 2 */
    "REASSOC_RES",  /* 3 */
    "PRB_REQ",      /* 4 */
    "PRB_RES",      /* 5 */
    "RESV",         /* 6 */
    "RESV",         /* 7 */
    "BCN",          /* 8 */
    "ATIM",         /* 9 */
    "DISASSOC",     /* 10 */
    "AUTH",         /* 11 */
    "DAUTH",        /* 12 */
    "ACTN",         /* 13 */
    "RESV",         /* 14 */
    "RESV",         /* 15 */
};

/*
 * htt_print_peer_ctrl_path_txrx_stats_tlv: display htt_peer_ctrl_path_txrx_stats_tlv
 * @tag_buf: buffer containing the tlv htt_peer_ctrl_path_txrx_stats_tlv
 *
 * return: void
 */
static void htt_print_peer_ctrl_path_txrx_stats_tlv(A_UINT32 *tag_buf)
{
    htt_peer_ctrl_path_txrx_stats_tlv *htt_stats_buf =
        (htt_peer_ctrl_path_txrx_stats_tlv *)tag_buf;
    A_UINT8  i;
    A_UINT16 index;
    A_CHAR   *peer_tx_mgmt_subtype = NULL;
    A_CHAR   *peer_rx_mgmt_subtype = NULL;

    peer_tx_mgmt_subtype = (A_CHAR *)malloc(HTT_MAX_STRING_LEN * sizeof(A_CHAR));
    if (!peer_tx_mgmt_subtype) {
       HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
       return;
    }

    peer_rx_mgmt_subtype = (A_CHAR *)malloc(HTT_MAX_STRING_LEN * sizeof(A_CHAR));
    if (!peer_rx_mgmt_subtype) {
       HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
       free(peer_tx_mgmt_subtype);
       return;
    }

    HTT_STATS_PRINT(FATAL, "HTT_STATS_PEER_CTRL_PATH_TXRX_STATS_TAG:");
    HTT_STATS_PRINT(FATAL, "peer_mac_addr = %02x:%02x:%02x:%02x:%02x:%02x",
            (htt_stats_buf->peer_mac_addr.mac_addr31to0  & 0x000000FF),
            (htt_stats_buf->peer_mac_addr.mac_addr31to0  & 0x0000FF00) >> 8,
            (htt_stats_buf->peer_mac_addr.mac_addr31to0  & 0x00FF0000) >> 16,
            (htt_stats_buf->peer_mac_addr.mac_addr31to0  & 0xFF000000) >> 24,
            (htt_stats_buf->peer_mac_addr.mac_addr47to32 & 0x000000FF),
            (htt_stats_buf->peer_mac_addr.mac_addr47to32 & 0x0000FF00) >> 8);

    index = 0;
    memset(peer_tx_mgmt_subtype, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_SUBTYPE_MAX; i++) {
        if (strncmp(g_mgmt_fc_subtype[i],"RESV",4) == 0) {
            continue;
        }
        index += snprintf(&peer_tx_mgmt_subtype[index],
                HTT_MAX_STRING_LEN - index,
                " %s[%u]:%u,", g_mgmt_fc_subtype[i], i,
                htt_stats_buf->peer_tx_mgmt_subtype[i]);
    }
    HTT_STATS_PRINT(FATAL, "peer_tx_mgmt_fc_subtype = %s ", peer_tx_mgmt_subtype);

    index = 0;
    memset(peer_rx_mgmt_subtype, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_SUBTYPE_MAX; i++) {
        if (strncmp(g_mgmt_fc_subtype[i],"RESV",4) == 0) {
            continue;
        }
        index += snprintf(&peer_rx_mgmt_subtype[index],
                HTT_MAX_STRING_LEN - index,
                " %s[%u]:%u,", g_mgmt_fc_subtype[i], i,
                htt_stats_buf->peer_rx_mgmt_subtype[i]);
    }
    HTT_STATS_PRINT(FATAL, "peer_rx_mgmt_fc_subtype = %s \n", peer_rx_mgmt_subtype);

    free(peer_tx_mgmt_subtype);
    free(peer_rx_mgmt_subtype);
}

/*
 * htt_print_tx_pdev_stats_cmn_tlv: display htt_tx_pdev_stats_cmn_tlv
 * @tag_buf: buffer containing the tlv htt_tx_pdev_stats_cmn_tlv
 *
 * return:void
 */
static void htt_print_tx_pdev_stats_cmn_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_stats_cmn_tlv *htt_stats_buf =
        (htt_tx_pdev_stats_cmn_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_STATS_CMN_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__word & 0xFF);
    HTT_STATS_PRINT(FATAL, "comp_delivered = %u",
            htt_stats_buf->comp_delivered);
    HTT_STATS_PRINT(FATAL, "self_triggers = %u",
            htt_stats_buf->self_triggers);
    /*HTT_STATS_PRINT(FATAL, "word(reserved) = %u",
            ((htt_stats_buf->mac_id__word & 0xFFFFFF00) >> 8 ));*/
    HTT_STATS_PRINT(FATAL, "hw_queued = %u",
            htt_stats_buf->hw_queued);
    HTT_STATS_PRINT(FATAL, "hw_reaped = %u",
            htt_stats_buf->hw_reaped);
    HTT_STATS_PRINT(FATAL, "underrun = %u",
            htt_stats_buf->underrun);
    HTT_STATS_PRINT(FATAL, "hw_paused = %u",
            htt_stats_buf->hw_paused);
    HTT_STATS_PRINT(FATAL, "hw_flush = %u",
            htt_stats_buf->hw_flush);
    HTT_STATS_PRINT(FATAL, "ofdma_seq_flush = %u",
            htt_stats_buf->ofdma_seq_flush);
    HTT_STATS_PRINT(FATAL, "hw_filt = %u",
            htt_stats_buf->hw_filt);
    HTT_STATS_PRINT(FATAL, "tx_abort = %u",
            htt_stats_buf->tx_abort);
    HTT_STATS_PRINT(FATAL, "ppdu_ok = %u",
            htt_stats_buf->ppdu_ok);
    HTT_STATS_PRINT(FATAL, "mpdu_requeued = %u",
            htt_stats_buf->mpdu_requed);
    HTT_STATS_PRINT(FATAL, "tx_xretry = %u",
            htt_stats_buf->tx_xretry);
    HTT_STATS_PRINT(FATAL, "data_rc = %u",
            htt_stats_buf->data_rc);
    HTT_STATS_PRINT(FATAL, "mpdu_dropped_xretry = %u",
            htt_stats_buf->mpdu_dropped_xretry);
    HTT_STATS_PRINT(FATAL, "illegal_rate_phy_err = %u",
            htt_stats_buf->illgl_rate_phy_err);
    HTT_STATS_PRINT(FATAL, "cont_xretry = %u",
            htt_stats_buf->cont_xretry);
    HTT_STATS_PRINT(FATAL, "tx_timeout = %u",
            htt_stats_buf->tx_timeout);
    HTT_STATS_PRINT(FATAL, "tx_time_dur_data = %u",
            htt_stats_buf->tx_time_dur_data);
    HTT_STATS_PRINT(FATAL, "pdev_resets = %u",
            htt_stats_buf->pdev_resets);
    HTT_STATS_PRINT(FATAL, "phy_underrun = %u",
            htt_stats_buf->phy_underrun);
    HTT_STATS_PRINT(FATAL, "txop_ovf = %u",
            htt_stats_buf->txop_ovf);
    HTT_STATS_PRINT(FATAL, "seq_posted = %u",
            htt_stats_buf->seq_posted);
    HTT_STATS_PRINT(FATAL, "seq_failed_queueing = %u",
            htt_stats_buf->seq_failed_queueing);
    HTT_STATS_PRINT(FATAL, "seq_completed = %u",
            htt_stats_buf->seq_completed);
    HTT_STATS_PRINT(FATAL, "seq_restarted = %u",
            htt_stats_buf->seq_restarted);
    HTT_STATS_PRINT(FATAL, "seq_txop_repost_stop = %u",
            htt_stats_buf->seq_txop_repost_stop);
    HTT_STATS_PRINT(FATAL, "next_seq_cancel = %u",
            htt_stats_buf->next_seq_cancel);
    HTT_STATS_PRINT(FATAL, "dl_mu_mimo_seq_posted = %u",
            htt_stats_buf->mu_seq_posted);
    HTT_STATS_PRINT(FATAL, "dl_mu_ofdma_seq_posted = %u",
            htt_stats_buf->mu_ofdma_seq_posted);
    HTT_STATS_PRINT(FATAL, "ul_mu_mimo_seq_posted = %u",
            htt_stats_buf->ul_mumimo_seq_posted);
    HTT_STATS_PRINT(FATAL, "ul_mu_ofdma_seq_posted = %u",
            htt_stats_buf->ul_ofdma_seq_posted);
    HTT_STATS_PRINT(FATAL, "dyn_mimo_ps_dlmumimo_sequences = %u",
            htt_stats_buf->num_dyn_mimo_ps_dlmumimo_sequences);
    HTT_STATS_PRINT(FATAL, "num_su_txbf_denylisted = %u",
            htt_stats_buf->num_su_txbf_denylisted);
    HTT_STATS_PRINT(FATAL, "mu_mimo_peer_denylist_count = %u",
            htt_stats_buf->num_mu_peer_blacklisted);
    HTT_STATS_PRINT(FATAL, "seq_qdepth_repost_stop = %u",
            htt_stats_buf->seq_qdepth_repost_stop);
    HTT_STATS_PRINT(FATAL, "seq_min_msdu_repost_stop = %u",
            htt_stats_buf->seq_min_msdu_repost_stop);
    HTT_STATS_PRINT(FATAL, "mu_seq_min_msdu_repost_stop = %u",
            htt_stats_buf->mu_seq_min_msdu_repost_stop);
    HTT_STATS_PRINT(FATAL, "seq_switch_hw_paused = %u",
            htt_stats_buf->seq_switch_hw_paused);
    HTT_STATS_PRINT(FATAL, "next_seq_posted_dsr = %u",
            htt_stats_buf->next_seq_posted_dsr);
    HTT_STATS_PRINT(FATAL, "seq_posted_isr = %u",
            htt_stats_buf->seq_posted_isr);
    HTT_STATS_PRINT(FATAL, "seq_ctrl_cached = %u",
            htt_stats_buf->seq_ctrl_cached);
    HTT_STATS_PRINT(FATAL, "mpdu_count_tqm = %u",
            htt_stats_buf->mpdu_count_tqm);
    HTT_STATS_PRINT(FATAL, "msdu_count_tqm = %u",
            htt_stats_buf->msdu_count_tqm);
    HTT_STATS_PRINT(FATAL, "mpdu_removed_tqm = %u",
            htt_stats_buf->mpdu_removed_tqm);
    HTT_STATS_PRINT(FATAL, "msdu_removed_tqm = %u",
            htt_stats_buf->msdu_removed_tqm);
    HTT_STATS_PRINT(FATAL, "remove_mpdus_max_retries = %u",
            htt_stats_buf->remove_mpdus_max_retries);
    HTT_STATS_PRINT(FATAL, "mpdus_sw_flush = %u",
            htt_stats_buf->mpdus_sw_flush);
    HTT_STATS_PRINT(FATAL, "mpdus_hw_filter = %u",
            htt_stats_buf->mpdus_hw_filter);
    HTT_STATS_PRINT(FATAL, "mpdus_truncated = %u",
            htt_stats_buf->mpdus_truncated);
    HTT_STATS_PRINT(FATAL, "mpdus_ack_failed = %u",
            htt_stats_buf->mpdus_ack_failed);
    HTT_STATS_PRINT(FATAL, "mpdus_expired = %u",
            htt_stats_buf->mpdus_expired);
    HTT_STATS_PRINT(FATAL, "mpdus_seq_hw_retry = %u",
            htt_stats_buf->mpdus_seq_hw_retry);
    HTT_STATS_PRINT(FATAL, "ack_tlv_proc = %u",
            htt_stats_buf->ack_tlv_proc);
    HTT_STATS_PRINT(FATAL, "coex_abort_mpdu_cnt_valid = %u",
            htt_stats_buf->coex_abort_mpdu_cnt_valid);
    HTT_STATS_PRINT(FATAL, "coex_abort_mpdu_cnt = %u",
            htt_stats_buf->coex_abort_mpdu_cnt);
    HTT_STATS_PRINT(FATAL, "num_total_ppdus_tried_ota = %u",
            htt_stats_buf->num_total_ppdus_tried_ota);
    HTT_STATS_PRINT(FATAL, "num_data_ppdus_tried_ota = %u",
            htt_stats_buf->num_data_ppdus_tried_ota);
    HTT_STATS_PRINT(FATAL, "local_ctrl_mgmt_enqued = %u",
            htt_stats_buf->local_ctrl_mgmt_enqued);
    HTT_STATS_PRINT(FATAL, "local_ctrl_mgmt_freed = %u",
            htt_stats_buf->local_ctrl_mgmt_freed);
    HTT_STATS_PRINT(FATAL, "local_data_enqued = %u",
            htt_stats_buf->local_data_enqued);
    HTT_STATS_PRINT(FATAL, "local_data_freed = %u",
            htt_stats_buf->local_data_freed);
    HTT_STATS_PRINT(FATAL, "mpdu_tried = %u",
            htt_stats_buf->mpdu_tried);
    HTT_STATS_PRINT(FATAL, "isr_wait_seq_posted = %u",
            htt_stats_buf->isr_wait_seq_posted);
    HTT_STATS_PRINT(FATAL, "tx_active_dur_us_low = %u",
            htt_stats_buf->tx_active_dur_us_low);
    HTT_STATS_PRINT(FATAL, "tx_active_dur_us_high = %u",
            htt_stats_buf->tx_active_dur_us_high);
    HTT_STATS_PRINT(FATAL, "fes_offsets_err_cnt = %u",
            htt_stats_buf->fes_offsets_err_cnt);
    HTT_STATS_PRINT(FATAL, "thermal_suspend_cnt = %u",
            htt_stats_buf->thermal_suspend_cnt);
    HTT_STATS_PRINT(FATAL, "dfs_suspend_cnt = %u",
            htt_stats_buf->dfs_suspend_cnt);
    HTT_STATS_PRINT(FATAL, "tx_abort_suspend_cnt = %u",
            htt_stats_buf->tx_abort_suspend_cnt);
    HTT_STATS_PRINT(FATAL, "suspended txq mask = %u",
            htt_stats_buf->tgt_specific_opaque_txq_suspend_info);
    HTT_STATS_PRINT(FATAL, "last_suspend_reason = %u",
            htt_stats_buf->last_suspend_reason);
    HTT_STATS_PRINT(FATAL, "pdev_up_time_us_low = %u",
            htt_stats_buf->pdev_up_time_us_low);
    HTT_STATS_PRINT(FATAL, "pdev_up_time_us_high = %u\n",
            htt_stats_buf->pdev_up_time_us_high);
}

/*
 *  htt_print_rx_fw_ring_stats_tlv : display htt_rx_fw_ring_stats_tlv_v
 *  @tag_buf: buffer containing the htt_rx_fw_ring_stats_tlv_v
 *
 *  return:void
 */
static void htt_print_rx_fw_ring_stats_tlv(A_UINT32 *tag_buf)
{
    htt_rx_fw_ring_stats_tlv_v *htt_stats_buf =
        (htt_rx_fw_ring_stats_tlv_v*)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_RX_FW_RING_STATUS_TLV_V:");

    HTT_STATS_PRINT(FATAL, "SW2RXDMA");
    HTT_STATS_PRINT(FATAL, "max_num_entries = %u",
        HTT_STATS_RX_FW_RING_SIZE_NUM_ENTRIES(
            htt_stats_buf->entry_status_sw2rxdma));
    HTT_STATS_PRINT(FATAL, "curr_num_entries = %u",
        HTT_STATS_RX_FW_RING_CURR_NUM_ENTRIES(
            htt_stats_buf->entry_status_sw2rxdma));

    HTT_STATS_PRINT(FATAL, "RXDMA2REO");
    HTT_STATS_PRINT(FATAL, "max_num_entries = %u",
        HTT_STATS_RX_FW_RING_SIZE_NUM_ENTRIES(
            htt_stats_buf->entry_status_rxdma2reo));
    HTT_STATS_PRINT(FATAL, "curr_num_entries = %u",
        HTT_STATS_RX_FW_RING_CURR_NUM_ENTRIES(
            htt_stats_buf->entry_status_rxdma2reo));

    HTT_STATS_PRINT(FATAL, "REO2SW1");
    HTT_STATS_PRINT(FATAL, "max_num_entries = %u",
        HTT_STATS_RX_FW_RING_SIZE_NUM_ENTRIES(
            htt_stats_buf->entry_status_reo2sw1));
    HTT_STATS_PRINT(FATAL, "curr_num_entries = %u",
        HTT_STATS_RX_FW_RING_CURR_NUM_ENTRIES(
            htt_stats_buf->entry_status_reo2sw1));

    HTT_STATS_PRINT(FATAL, "REO2SW4");
    HTT_STATS_PRINT(FATAL, "max_num_entries = %u",
        HTT_STATS_RX_FW_RING_SIZE_NUM_ENTRIES(
            htt_stats_buf->entry_status_reo2sw4));
    HTT_STATS_PRINT(FATAL, "curr_num_entries = %u",
        HTT_STATS_RX_FW_RING_CURR_NUM_ENTRIES(
            htt_stats_buf->entry_status_reo2sw4));

    HTT_STATS_PRINT(FATAL, "BackPressure Histogram: "
        "0ms to 250ms = %u, 250ms to 500ms = %u, Above 500ms = %u",
        htt_stats_buf->reo2sw4ringipa_backpress_hist[0],
        htt_stats_buf->reo2sw4ringipa_backpress_hist[1],
        htt_stats_buf->reo2sw4ringipa_backpress_hist[2]);

    HTT_STATS_PRINT(FATAL, "REFILLRINGIPA");
    HTT_STATS_PRINT(FATAL, "max_num_entries = %u",
        HTT_STATS_RX_FW_RING_SIZE_NUM_ENTRIES(
            htt_stats_buf->entry_status_refillringipa));
    HTT_STATS_PRINT(FATAL, "curr_num_entries = %u",
        HTT_STATS_RX_FW_RING_CURR_NUM_ENTRIES(
            htt_stats_buf->entry_status_refillringipa));
    HTT_STATS_PRINT(FATAL, "avg_entries = %u",
        htt_stats_buf->datarate_refillringipa);

    HTT_STATS_PRINT(FATAL, "REFILLRINGHOST");
    HTT_STATS_PRINT(FATAL, "max_num_entries = %u",
        HTT_STATS_RX_FW_RING_SIZE_NUM_ENTRIES(
            htt_stats_buf->entry_status_refillringhost));
    HTT_STATS_PRINT(FATAL, "curr_num_entries = %u",
        HTT_STATS_RX_FW_RING_CURR_NUM_ENTRIES(
            htt_stats_buf->entry_status_refillringhost));
    HTT_STATS_PRINT(FATAL, "avg_entries = %u\n",
        htt_stats_buf->datarate_refillringhost);
}

/*
 * htt_print_tx_pdev_stats_urrn_tlv_v: display htt_tx_pdev_stats_urrn_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_pdev_stats_urrn_tlv_v
 *
 * return:void
 */
static void htt_print_tx_pdev_stats_urrn_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_pdev_stats_urrn_tlv_v *htt_stats_buf =
        (htt_tx_pdev_stats_urrn_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                          = 0;
    A_CHAR   urrn_stats[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                        = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_STATS_URRN_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&urrn_stats[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->urrn_stats[i]);
    }

    HTT_STATS_PRINT(FATAL, "urrn_stats = %s\n", urrn_stats);
}

/*
 * htt_print_tx_pdev_stats_flush_tlv_v: display htt_tx_pdev_stats_flush_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_pdev_stats_flush_tlv_v
 *
 * return:void
 */
static void htt_print_tx_pdev_stats_flush_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_pdev_stats_flush_tlv_v *htt_stats_buf =
        (htt_tx_pdev_stats_flush_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                          = 0;
    A_CHAR   flush_errs[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                        = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_STATS_FLUSH_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&flush_errs[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->flush_errs[i]);
    }

    HTT_STATS_PRINT(FATAL, "flush_errs = %s\n", flush_errs);
}

/*
 * htt_print_tx_pdev_stats_mlo_abort_tlv_v: display htt_tx_pdev_stats_mlo_abort_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_pdev_stats_mlo_abort_tlv_v
 *
 * return:void
 */
static void htt_print_tx_pdev_stats_mlo_abort_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_pdev_stats_mlo_abort_tlv_v *htt_stats_buf =
        (htt_tx_pdev_stats_mlo_abort_tlv_v *)tag_buf;
    A_UINT16 index = 0;
    A_CHAR   mlo_abort_cnt[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 i;
    A_UINT32 tag_len32 = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_STATS_MLO_ABORT_TLV_V:");

    for (i = 0; i < tag_len32; i++) {
        index += snprintf(&mlo_abort_cnt[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->mlo_abort_cnt[i]);
    }

    HTT_STATS_PRINT(FATAL, "mlo_abort_cnt = %s\n", mlo_abort_cnt);
}

/*
 * htt_print_tx_pdev_stats_mlo_txop_abort_tlv_v: display htt_tx_pdev_stats_mlo_txop_abort_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_pdev_stats_mlo_txop_abort_tlv_v
 *
 * return:void
 */
static void htt_print_tx_pdev_stats_mlo_txop_abort_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_pdev_stats_mlo_txop_abort_tlv_v *htt_stats_buf =
        (htt_tx_pdev_stats_mlo_txop_abort_tlv_v *)tag_buf;
    A_UINT16 index = 0;
    A_CHAR   mlo_txop_abort_cnt[HTT_MAX_STRING_LEN]   = {0};
    A_UINT32 i;
    A_UINT32 tag_len32 = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_STATS_MLO_TXOP_ABORT_TLV_V:");

    for (i = 0; i < tag_len32; i++) {
        index += snprintf(&mlo_txop_abort_cnt[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->mlo_txop_abort_cnt[i]);
    }

    HTT_STATS_PRINT(FATAL, "mlo_txop_abort_cnt = %s\n", mlo_txop_abort_cnt);
}

/*
 * htt_print_pdev_ctrl_path_tx_stats_tlv: display htt_pdev_ctrl_path_tx_stats_tlv_v
 * @tag_buf: buffer containing the tlv htt_pdev_ctrl_path_tx_stats_tlv_v
 *
 * return:void
 */
static void htt_print_pdev_ctrl_path_tx_stats_tlv(A_UINT32 *tag_buf)
{
    htt_pdev_ctrl_path_tx_stats_tlv_v *htt_stats_buf =
        (htt_pdev_ctrl_path_tx_stats_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                    = 0;
    A_CHAR   fw_tx_mgmt_subtype[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_STATS_PDEV_CTRL_PATH_TX_STATS_TAG:");
    for (i = 0; i < HTT_STATS_SUBTYPE_MAX; i++) {
        index += snprintf(&fw_tx_mgmt_subtype[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->fw_tx_mgmt_subtype[i]);
    }

    HTT_STATS_PRINT(FATAL, "fw_tx_mgmt_subtype = %s \n", fw_tx_mgmt_subtype);
}

/*
 * htt_print_tx_pdev_stats_sifs_tlv_v: display htt_tx_pdev_stats_sifs_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_pdev_stats_sifs_tlv_v
 *
 * return:void
 */
static void htt_print_tx_pdev_stats_sifs_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_pdev_stats_sifs_tlv_v *htt_stats_buf =
        (htt_tx_pdev_stats_sifs_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                           = 0;
    A_CHAR   sifs_status[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                         = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_STATS_SIFS_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&sifs_status[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->sifs_status[i]);
    }

    HTT_STATS_PRINT(FATAL, "sifs_status = %s\n", sifs_status);
}

/*
 * htt_print_tx_pdev_stats_phy_err_tlv_v: display htt_tx_pdev_stats_phy_err_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_pdev_stats_phy_err_tlv_v
 *
 * return:void
 */
static void htt_print_tx_pdev_stats_phy_err_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_pdev_stats_phy_err_tlv_v *htt_stats_buf =
        (htt_tx_pdev_stats_phy_err_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                        = 0;
    A_CHAR   phy_errs[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                      = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_STATS_PHY_ERR_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&phy_errs[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->phy_errs[i]);
    }

    HTT_STATS_PRINT(FATAL, "phy_errs = %s\n", phy_errs);
}

/*
 * htt_print_tx_pdev_stats_sifs_hist_tlv_v: display htt_tx_pdev_stats_sifs_hist_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_pdev_stats_sifs_hist_tlv_v
 *
 * return:void
 */
static void htt_print_tx_pdev_stats_sifs_hist_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_pdev_stats_sifs_hist_tlv_v *htt_stats_buf =
        (htt_tx_pdev_stats_sifs_hist_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                = 0;
    A_CHAR   sifs_hist_status[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                              = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_STATS_SIFS_HIST_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&sifs_hist_status[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->sifs_hist_status[i]);
    }

    HTT_STATS_PRINT(FATAL, "sifs_hist_status = %s\n", sifs_hist_status);
}

/*
 * htt_print_tx_pdev_stats_tx_ppdu_stats_tlv_v: display htt_tx_pdev_stats_tx_ppdu_stats_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_pdev_stats_tx_ppdu_stats_tlv_v
 *
 * return:void
 */
static void htt_print_tx_pdev_stats_tx_ppdu_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_pdev_stats_tx_ppdu_stats_tlv_v *htt_stats_buf =
        (htt_tx_pdev_stats_tx_ppdu_stats_tlv_v *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_STATS_TX_PPDU_STATS_TLV_V:");

    HTT_STATS_PRINT(FATAL, "num_data_ppdus_legacy_su = %u",
            htt_stats_buf->num_data_ppdus_legacy_su);

    HTT_STATS_PRINT(FATAL, "num_data_ppdus_ac_su = %u",
            htt_stats_buf->num_data_ppdus_ac_su);

    HTT_STATS_PRINT(FATAL, "num_data_ppdus_ax_su = %u",
            htt_stats_buf->num_data_ppdus_ax_su);

    HTT_STATS_PRINT(FATAL, "num_data_ppdus_ac_su_txbf = %u",
            htt_stats_buf->num_data_ppdus_ac_su_txbf);

    HTT_STATS_PRINT(FATAL, "num_data_ppdus_ax_su_txbf = %u\n",
            htt_stats_buf->num_data_ppdus_ax_su_txbf);
}

/*
 * htt_print_tx_pdev_mu_ppdu_dist_stats_tlv_v: display htt_pdev_mu_ppdu_dist_tlv_v
 * @tag_buf: buffer containing the tlv htt_pdev_mu_ppdu_dist_tlv_v
 *
 * return:void
 */
static void htt_print_tx_pdev_mu_ppdu_dist_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_pdev_mu_ppdu_dist_tlv_v *htt_stats_buf =
        (htt_pdev_mu_ppdu_dist_tlv_v *)tag_buf;
    A_UINT8 i = 0, j = 0, k = 0;
    A_UINT16 index = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_CHAR hw_mode_prefix[][3] = {"ac", "ax", "be"};

    if (htt_stats_buf->hw_mode == HTT_STATS_HWMODE_AC) {
        HTT_STATS_PRINT(FATAL, "\nHTT_TX_PDEV_MU_PPDU_DISTRIBUTION_STATS:\n");
    } else if(htt_stats_buf->hw_mode == HTT_STATS_HWMODE_AX) {
        k = 1;
    } else if(htt_stats_buf->hw_mode == HTT_STATS_HWMODE_BE) {
        k = 2;
    } else {
        return;
        /* hw_mode not recognized */
    }

    /* Iterations - 2 for nr -4 and nr -8 in each hw mode*/
    for (i = 0; i < 2; i++) {
        HTT_STATS_PRINT(FATAL,
            "%s_mu_mimo_num_seq_posted_nr%u = %u",
            hw_mode_prefix[k],
            ((i+1)*4),
            htt_stats_buf->mu_mimo_num_seq_posted[i]);
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (j = 0; j < HTT_STATS_MAX_NUM_MU_PPDU_PER_BURST ; j++){
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", j, htt_stats_buf->mu_mimo_num_ppdu_posted_per_burst[i * HTT_STATS_MAX_NUM_MU_PPDU_PER_BURST + j]);
        }
        HTT_STATS_PRINT(FATAL,
            "%s_mu_mimo_num_ppdu_posted_per_burst_nr%u = %s ",
            hw_mode_prefix[k], ((i+1)*4), str_buf);
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (j = 0; j < HTT_STATS_MAX_NUM_MU_PPDU_PER_BURST ; j++){
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", j, htt_stats_buf->mu_mimo_num_ppdu_completed_per_burst[i * HTT_STATS_MAX_NUM_MU_PPDU_PER_BURST + j]);
        }
        HTT_STATS_PRINT(FATAL,
            "%s_mu_mimo_num_ppdu_completed_per_burst_nr%u = %s ",
            hw_mode_prefix[k], ((i+1)*4), str_buf);
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (j = 0; j < HTT_STATS_MAX_NUM_SCHED_STATUS ; j++){
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", j, htt_stats_buf->mu_mimo_num_seq_term_status[i * HTT_STATS_MAX_NUM_SCHED_STATUS + j]);
            }
        HTT_STATS_PRINT(FATAL,
            "%s_mu_mimo_num_seq_term_status_nr%u = %s\n ",
            hw_mode_prefix[k], ((i+1)*4), str_buf);
    }
}

/*
 * htt_print_tx_pdev_stats_tried_mpdu_cnt_hist_tlv_v: display htt_tx_pdev_stats_tried_mpdu_cnt_hist_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_pdev_stats_tried_mpdu_cnt_hist_tlv_v
 *
 * return:void
 */
static void htt_print_tx_pdev_stats_tried_mpdu_cnt_hist_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_pdev_stats_tried_mpdu_cnt_hist_tlv_v *htt_stats_buf =
        (htt_tx_pdev_stats_tried_mpdu_cnt_hist_tlv_v *)tag_buf;

    A_UINT8  i;
    A_UINT16 index                                   = 0;
    A_CHAR   tried_mpdu_cnt_hist[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 num_elements                            =
        ((HTT_STATS_TLV_LENGTH_GET(*tag_buf) - sizeof(htt_stats_buf->hist_bin_size)) >> 2);
    A_UINT32 required_buffer_size = HTT_MAX_PRINT_CHAR_PER_ELEM * num_elements;

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_STATS_TRIED_MPDU_CNT_HIST_TLV_V:");
    HTT_STATS_PRINT(FATAL, "TRIED_MPDU_CNT_HIST_BIN_SIZE : %u", htt_stats_buf->hist_bin_size);

    if (required_buffer_size < HTT_MAX_STRING_LEN) {
        for (i = 0; i < num_elements; i++) {
            index += snprintf(&tried_mpdu_cnt_hist[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i, htt_stats_buf->tried_mpdu_cnt_hist[i]);
        }

        HTT_STATS_PRINT(FATAL, "tried_mpdu_cnt_hist = %s\n", tried_mpdu_cnt_hist);
    } else {
        HTT_STATS_PRINT(FATAL, "INSUFFICIENT PRINT BUFFER\n");
    }
}

/*
 * htt_print_hw_stats_intr_misc_tlv: display htt_hw_stats_intr_misc_tlv
 * @tag_buf: buffer containing the tlv htt_hw_stats_intr_misc_tlv
 *
 * return:void
 */
static void htt_print_hw_stats_intr_misc_tlv(A_UINT32 *tag_buf)
{
    htt_hw_stats_intr_misc_tlv *htt_stats_buf =
        (htt_hw_stats_intr_misc_tlv *)tag_buf;
    A_CHAR   hw_intr_name[HTT_STATS_MAX_HW_INTR_NAME_LEN + 1] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_HW_STATS_INTR_MISC_TLV:");
    memcpy(hw_intr_name, &(htt_stats_buf->hw_intr_name[0]), HTT_STATS_MAX_HW_INTR_NAME_LEN);
    HTT_STATS_PRINT(FATAL, "hw_intr_name = %s ", hw_intr_name);
    HTT_STATS_PRINT(FATAL, "mask = %u",
            htt_stats_buf->mask);
    HTT_STATS_PRINT(FATAL, "count = %u\n",
            htt_stats_buf->count);
}

/*
 * htt_print_hw_stats_wd_timeout_tlv: display htt_hw_stats_wd_timeout_tlv
 * @tag_buf: buffer containing the tlv htt_hw_stats_wd_timeout_tlv
 *
 * return:void
 */
static void htt_print_hw_stats_wd_timeout_tlv(A_UINT32 *tag_buf)
{
    htt_hw_stats_wd_timeout_tlv *htt_stats_buf =
        (htt_hw_stats_wd_timeout_tlv *)tag_buf;
    A_CHAR   hw_module_name[HTT_STATS_MAX_HW_MODULE_NAME_LEN + 1] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_HW_STATS_WD_TIMEOUT_TLV:");
    memcpy(hw_module_name, &(htt_stats_buf->hw_module_name[0]), HTT_STATS_MAX_HW_MODULE_NAME_LEN);
    HTT_STATS_PRINT(FATAL, "hw_module_name = %s ", hw_module_name);
    HTT_STATS_PRINT(FATAL, "count = %u",
            htt_stats_buf->count);
}

/*
 * htt_print_hw_stats_pdev_errs_tlv: display htt_hw_stats_pdev_errs_tlv
 * @tag_buf: buffer containing the tlv htt_hw_stats_pdev_errs_tlv
 *
 * return:void
 */
static void htt_print_hw_stats_pdev_errs_tlv(A_UINT32 *tag_buf)
{
    htt_hw_stats_pdev_errs_tlv *htt_stats_buf =
        (htt_hw_stats_pdev_errs_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_HW_STATS_PDEV_ERRS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__word & 0xFF);
    /*HTT_STATS_PRINT(FATAL, "word = %u",
            ((htt_stats_buf->mac_id__word & 0xFFFFFF00) >> 8 ));*/
    HTT_STATS_PRINT(FATAL, "tx_abort = %u",
            htt_stats_buf->tx_abort);
    HTT_STATS_PRINT(FATAL, "tx_abort_fail_count = %u",
            htt_stats_buf->tx_abort_fail_count);
    HTT_STATS_PRINT(FATAL, "rx_abort = %u",
            htt_stats_buf->rx_abort);
    HTT_STATS_PRINT(FATAL, "rx_abort_fail_count = %u",
            htt_stats_buf->rx_abort_fail_count);
    HTT_STATS_PRINT(FATAL, "rx_flush_cnt = %u",
            htt_stats_buf->rx_flush_cnt);
    HTT_STATS_PRINT(FATAL, "warm_reset = %u",
            htt_stats_buf->warm_reset);
    HTT_STATS_PRINT(FATAL, "cold_reset = %u",
            htt_stats_buf->cold_reset);
    HTT_STATS_PRINT(FATAL, "mac_cold_reset_restore_cal = %u",
            htt_stats_buf->mac_cold_reset_restore_cal);
    HTT_STATS_PRINT(FATAL, "mac_cold_reset = %u",
            htt_stats_buf->mac_cold_reset);
    HTT_STATS_PRINT(FATAL, "mac_warm_reset = %u",
            htt_stats_buf->mac_warm_reset);
    HTT_STATS_PRINT(FATAL, "mac_only_reset = %u",
            htt_stats_buf->mac_only_reset);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset = %u",
            htt_stats_buf->phy_warm_reset);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset_ucode_trig = %u",
            htt_stats_buf->phy_warm_reset_ucode_trig);
    HTT_STATS_PRINT(FATAL, "mac_warm_reset_restore_cal = %u",
            htt_stats_buf->mac_warm_reset_restore_cal);
    HTT_STATS_PRINT(FATAL, "mac_sfm_reset = %u",
            htt_stats_buf->mac_sfm_reset);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset_m3_ssr = %u",
            htt_stats_buf->phy_warm_reset_m3_ssr);
    HTT_STATS_PRINT(FATAL, "fw_rx_rings_reset = %u",
            htt_stats_buf->fw_rx_rings_reset);
    HTT_STATS_PRINT(FATAL, "tx_flush = %u",
            htt_stats_buf->tx_flush);
    HTT_STATS_PRINT(FATAL, "tx_glb_reset = %u",
            htt_stats_buf->tx_glb_reset);
    HTT_STATS_PRINT(FATAL, "tx_txq_reset = %u",
            htt_stats_buf->tx_txq_reset);
    HTT_STATS_PRINT(FATAL, "rx_timeout_reset = %u\n",
            htt_stats_buf->rx_timeout_reset);

    HTT_STATS_PRINT(FATAL, "PDEV_PHY_WARM_RESET_REASONS:");

    HTT_STATS_PRINT(FATAL, "phy_warm_reset_reason_phy_m3 = %u",
            htt_stats_buf->phy_warm_reset_reason_phy_m3);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset_reason_tx_hw_stuck = %u",
            htt_stats_buf->phy_warm_reset_reason_tx_hw_stuck);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset_reason_num_cca_rx_frame_stuck = %u",
            htt_stats_buf->phy_warm_reset_reason_num_cca_rx_frame_stuck);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset_reason_wal_rx_recovery_rst_rx_busy = %u",
            htt_stats_buf->phy_warm_reset_reason_wal_rx_recovery_rst_rx_busy);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset_reason_wal_rx_recovery_rst_mac_hang = %u",
            htt_stats_buf->phy_warm_reset_reason_wal_rx_recovery_rst_mac_hang);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset_reason_mac_reset_converted_phy_reset = %u",
            htt_stats_buf->phy_warm_reset_reason_mac_reset_converted_phy_reset);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset_reason_tx_lifetime_expiry_cca_stuck = %u",
            htt_stats_buf->phy_warm_reset_reason_tx_lifetime_expiry_cca_stuck);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset_reason_tx_consecutive_flush9_war = %u",
            htt_stats_buf->phy_warm_reset_reason_tx_consecutive_flush9_war);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset_reason_tx_hwsch_reset_war = %u",
            htt_stats_buf->phy_warm_reset_reason_tx_hwsch_reset_war);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset_reason_hwsch_wdog_or_cca_wdog_war = %u\n",
            htt_stats_buf->phy_warm_reset_reason_hwsch_wdog_or_cca_wdog_war);


    HTT_STATS_PRINT(FATAL, "WAL_RX_RECOVERY_STATS:");

    HTT_STATS_PRINT(FATAL, "wal_rx_recovery_rst_mac_hang_count = %u",
            htt_stats_buf->wal_rx_recovery_rst_mac_hang_count);
    HTT_STATS_PRINT(FATAL, "wal_rx_recovery_rst_known_sig_count = %u",
            htt_stats_buf->wal_rx_recovery_rst_known_sig_count);
    HTT_STATS_PRINT(FATAL, "wal_rx_recovery_rst_no_rx_count = %u",
            htt_stats_buf->wal_rx_recovery_rst_no_rx_count);
    HTT_STATS_PRINT(FATAL, "wal_rx_recovery_rst_no_rx_consecutive_count = %u",
            htt_stats_buf->wal_rx_recovery_rst_no_rx_consecutive_count);
    HTT_STATS_PRINT(FATAL, "wal_rx_recovery_rst_rx_busy_count = %u",
            htt_stats_buf->wal_rx_recovery_rst_rx_busy_count);
    HTT_STATS_PRINT(FATAL, "wal_rx_recovery_rst_phy_mac_hang_count = %u\n",
            htt_stats_buf->wal_rx_recovery_rst_phy_mac_hang_count);

    HTT_STATS_PRINT(FATAL, "HTT_RX_DEST_DRAIN_STATS:");
    HTT_STATS_PRINT(FATAL, "rx_dest_drain_rx_descs_leak_prevention_done = %u",
            htt_stats_buf->rx_dest_drain_rx_descs_leak_prevention_done);
    HTT_STATS_PRINT(FATAL, "rx_dest_drain_rx_descs_saved_cnt = %u",
            htt_stats_buf->rx_dest_drain_rx_descs_saved_cnt);
    HTT_STATS_PRINT(FATAL, "rx_dest_drain_rxdma2reo_leak_detected = %u",
            htt_stats_buf->rx_dest_drain_rxdma2reo_leak_detected);
    HTT_STATS_PRINT(FATAL, "rx_dest_drain_rxdma2fw_leak_detected = %u",
            htt_stats_buf->rx_dest_drain_rxdma2fw_leak_detected);
    HTT_STATS_PRINT(FATAL, "rx_dest_drain_rxdma2wbm_leak_detected = %u",
            htt_stats_buf->rx_dest_drain_rxdma2wbm_leak_detected);
    HTT_STATS_PRINT(FATAL, "rx_dest_drain_rxdma1_2sw_leak_detected = %u",
            htt_stats_buf->rx_dest_drain_rxdma1_2sw_leak_detected);
    HTT_STATS_PRINT(FATAL, "rx_dest_drain_rx_drain_ok_mac_idle = %u",
            htt_stats_buf->rx_dest_drain_rx_drain_ok_mac_idle);
    HTT_STATS_PRINT(FATAL, "rx_dest_drain_ok_mac_not_idle = %u",
            htt_stats_buf->rx_dest_drain_ok_mac_not_idle);
    HTT_STATS_PRINT(FATAL, "rx_dest_drain_prerequisite_invld = %u",
            htt_stats_buf->rx_dest_drain_prerequisite_invld);
    HTT_STATS_PRINT(FATAL, "rx_dest_drain_skip_for_non_lmac_reset = %u\n",
            htt_stats_buf->rx_dest_drain_skip_for_non_lmac_reset);
}

/*
 * htt_print_msdu_flow_stats_tlv: display htt_msdu_flow_stats_tlv
 * @tag_buf: buffer containing the tlv htt_msdu_flow_stats_tlv
 *
 * return:void
 */
static void htt_print_msdu_flow_stats_tlv(A_UINT32 *tag_buf)
{
    htt_msdu_flow_stats_tlv *htt_stats_buf =
        (htt_msdu_flow_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_MSDU_FLOW_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "last_update_timestamp = %u",
            htt_stats_buf->last_update_timestamp);
    HTT_STATS_PRINT(FATAL, "last_add_timestamp = %u",
            htt_stats_buf->last_add_timestamp);
    HTT_STATS_PRINT(FATAL, "last_remove_timestamp = %u",
            htt_stats_buf->last_remove_timestamp);
    HTT_STATS_PRINT(FATAL, "total_processed_msdu_count = %u",
            htt_stats_buf->total_processed_msdu_count);
    HTT_STATS_PRINT(FATAL, "cur_msdu_count_in_flowq = %u",
            htt_stats_buf->cur_msdu_count_in_flowq);
    HTT_STATS_PRINT(FATAL, "sw_peer_id = %u",
            htt_stats_buf->sw_peer_id);
    HTT_STATS_PRINT(FATAL, "tx_flow_no = %u",
            htt_stats_buf->tx_flow_no__tid_num__drop_rule & 0xFFFF);
    HTT_STATS_PRINT(FATAL, "tid_num = %u",
            (htt_stats_buf->tx_flow_no__tid_num__drop_rule & 0xF0000) >> 16);
    HTT_STATS_PRINT(FATAL, "drop_rule = %u",
            (htt_stats_buf->tx_flow_no__tid_num__drop_rule & 0x100000) >> 20);
    HTT_STATS_PRINT(FATAL, "last_cycle_enqueue_count = %u",
            htt_stats_buf->last_cycle_enqueue_count);
    HTT_STATS_PRINT(FATAL, "last_cycle_dequeue_count = %u",
            htt_stats_buf->last_cycle_dequeue_count);
    HTT_STATS_PRINT(FATAL, "last_cycle_drop_count = %u",
            htt_stats_buf->last_cycle_drop_count);
    HTT_STATS_PRINT(FATAL, "current_drop_th = %u\n",
            htt_stats_buf->current_drop_th);
}

/*
 * htt_print_tx_tid_stats_tlv: display htt_tx_tid_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_tid_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_tid_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_tid_stats_tlv *htt_stats_buf =
        (htt_tx_tid_stats_tlv *)tag_buf;
    A_CHAR   tid_name[MAX_HTT_TID_NAME + 1] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_TX_TID_STATS_TLV:");
    memcpy(tid_name, &(htt_stats_buf->tid_name[0]), MAX_HTT_TID_NAME);
    HTT_STATS_PRINT(FATAL, "tid_name = %s ", tid_name);
    HTT_STATS_PRINT(FATAL, "sw_peer_id = %u",
            htt_stats_buf->sw_peer_id__tid_num & 0xFFFF);
    HTT_STATS_PRINT(FATAL, "tid_num = %u",
            (htt_stats_buf->sw_peer_id__tid_num & 0xFFFF0000) >> 16);
    HTT_STATS_PRINT(FATAL, "num_sched_pending = %u",
            htt_stats_buf->num_sched_pending__num_ppdu_in_hwq & 0xFF);
    HTT_STATS_PRINT(FATAL, "num_ppdu_in_hwq = %u",
            (htt_stats_buf->num_sched_pending__num_ppdu_in_hwq & 0xFF00) >> 8);
    HTT_STATS_PRINT(FATAL, "tid_flags = 0x%x",
            htt_stats_buf->tid_flags);
    HTT_STATS_PRINT(FATAL, "hw_queued = %u",
            htt_stats_buf->hw_queued);
    HTT_STATS_PRINT(FATAL, "hw_reaped = %u",
            htt_stats_buf->hw_reaped);
    HTT_STATS_PRINT(FATAL, "mpdus_hw_filter = %u",
            htt_stats_buf->mpdus_hw_filter);
    HTT_STATS_PRINT(FATAL, "qdepth_bytes = %u",
            htt_stats_buf->qdepth_bytes);
    HTT_STATS_PRINT(FATAL, "qdepth_num_msdu = %u",
            htt_stats_buf->qdepth_num_msdu);
    HTT_STATS_PRINT(FATAL, "qdepth_num_mpdu = %u",
            htt_stats_buf->qdepth_num_mpdu);
    HTT_STATS_PRINT(FATAL, "last_scheduled_tsmp = %u",
            htt_stats_buf->last_scheduled_tsmp);
    HTT_STATS_PRINT(FATAL, "pause_module_id = %u",
            htt_stats_buf->pause_module_id);
    HTT_STATS_PRINT(FATAL, "block_module_id = %u\n",
            htt_stats_buf->block_module_id);
    /*HTT_STATS_PRINT(FATAL, "tid_tx_airtime = %u\n",
            htt_stats_buf->tid_tx_airtime);*/
}

/*
 * htt_print_tx_tid_stats_v1_tlv: display htt_tx_tid_stats_v1_tlv
 * @tag_buf: buffer containing the tlv htt_tx_tid_stats_v1_tlv
 *
 * return:void
 */
static void htt_print_tx_tid_stats_v1_tlv(A_UINT32 *tag_buf)
{
    htt_tx_tid_stats_v1_tlv *htt_stats_buf =
        (htt_tx_tid_stats_v1_tlv *)tag_buf;
    A_CHAR   tid_name[MAX_HTT_TID_NAME + 1] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_TX_TID_STATS_V1_TLV:");
    memcpy(tid_name, &(htt_stats_buf->tid_name[0]), MAX_HTT_TID_NAME);
    HTT_STATS_PRINT(FATAL, "tid_name = %s ", tid_name);
    HTT_STATS_PRINT(FATAL, "sw_peer_id = %u",
            htt_stats_buf->sw_peer_id__tid_num & 0xFFFF);
    HTT_STATS_PRINT(FATAL, "tid_num = %u",
            (htt_stats_buf->sw_peer_id__tid_num & 0xFFFF0000) >> 16);
    HTT_STATS_PRINT(FATAL, "num_sched_pending = %u",
            htt_stats_buf->num_sched_pending__num_ppdu_in_hwq & 0xFF);
    HTT_STATS_PRINT(FATAL, "num_ppdu_in_hwq = %u",
            (htt_stats_buf->num_sched_pending__num_ppdu_in_hwq & 0xFF00) >> 8);
    HTT_STATS_PRINT(FATAL, "tid_flags = 0x%x",
            htt_stats_buf->tid_flags);
    HTT_STATS_PRINT(FATAL, "max_qdepth_bytes = %u",
            htt_stats_buf->max_qdepth_bytes);
    HTT_STATS_PRINT(FATAL, "max_qdepth_n_msdus = %u",
            htt_stats_buf->max_qdepth_n_msdus);
    HTT_STATS_PRINT(FATAL, "rsvd = %u",
            htt_stats_buf->rsvd);
    HTT_STATS_PRINT(FATAL, "qdepth_bytes = %u",
            htt_stats_buf->qdepth_bytes);
    HTT_STATS_PRINT(FATAL, "qdepth_num_msdu = %u",
            htt_stats_buf->qdepth_num_msdu);
    HTT_STATS_PRINT(FATAL, "qdepth_num_mpdu = %u",
            htt_stats_buf->qdepth_num_mpdu);
    HTT_STATS_PRINT(FATAL, "last_scheduled_tsmp = %u",
            htt_stats_buf->last_scheduled_tsmp);
    HTT_STATS_PRINT(FATAL, "pause_module_id = %u",
            htt_stats_buf->pause_module_id);
    HTT_STATS_PRINT(FATAL, "block_module_id = %u",
            htt_stats_buf->block_module_id);
    /*HTT_STATS_PRINT(FATAL, "tid_tx_airtime = %u\n",
            htt_stats_buf->tid_tx_airtime);*/
    HTT_STATS_PRINT(FATAL, "allow_n_flags = 0x%x",
            htt_stats_buf->allow_n_flags);
    HTT_STATS_PRINT(FATAL, "sendn_frms_allowed = %u",
            htt_stats_buf->sendn_frms_allowed);
    HTT_STATS_PRINT(FATAL, "tid_ext_flags = 0x%x",
            htt_stats_buf->tid_ext_flags);
    HTT_STATS_PRINT(FATAL, "tid_ext2_flags = 0x%x",
            htt_stats_buf->tid_ext2_flags);
    HTT_STATS_PRINT(FATAL, "tid_flush_reason = 0x%x",
            htt_stats_buf->tid_flush_reason);
    HTT_STATS_PRINT(FATAL, "mlo_flush_tqm_status_pending = 0x%x%x",
            htt_stats_buf->mlo_flush_tqm_status_pending_high,
            htt_stats_buf->mlo_flush_tqm_status_pending_low);
    HTT_STATS_PRINT(FATAL, "mlo_flush_partner_info= 0x%x%x",
            htt_stats_buf->mlo_flush_partner_info_high,
            htt_stats_buf->mlo_flush_partner_info_low);
    HTT_STATS_PRINT(FATAL, "mlo_flush_initator_info= 0x%x%x",
            htt_stats_buf->mlo_flush_initator_info_high,
            htt_stats_buf->mlo_flush_initator_info_low);
    HTT_STATS_PRINT(FATAL, "head_msdu_tqm_timestamp_us = %u",
            htt_stats_buf->head_msdu_tqm_timestamp_us);
    HTT_STATS_PRINT(FATAL, "head_msdu_tqm_latency_us = %u\n",
            htt_stats_buf->head_msdu_tqm_latency_us);
}

/*
 * htt_print_rx_tid_stats_tlv: display htt_rx_tid_stats_tlv
 * @tag_buf: buffer containing the tlv htt_rx_tid_stats_tlv
 *
 * return:void
 */
static void htt_print_rx_tid_stats_tlv(A_UINT32 *tag_buf)
{
    htt_rx_tid_stats_tlv *htt_stats_buf =
        (htt_rx_tid_stats_tlv *)tag_buf;
    A_CHAR   tid_name[MAX_HTT_TID_NAME + 1] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_RX_TID_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "sw_peer_id = %u",
            htt_stats_buf->sw_peer_id__tid_num & 0xFFFF);
    HTT_STATS_PRINT(FATAL, "tid_num = %u",
            (htt_stats_buf->sw_peer_id__tid_num & 0xFFFF0000) >> 16);
    memcpy(tid_name, &(htt_stats_buf->tid_name[0]), MAX_HTT_TID_NAME);
    HTT_STATS_PRINT(FATAL, "tid_name = %s ", tid_name);
    HTT_STATS_PRINT(FATAL, "dup_in_reorder = %u",
            htt_stats_buf->dup_in_reorder);
    HTT_STATS_PRINT(FATAL, "dup_past_outside_window = %u",
            htt_stats_buf->dup_past_outside_window);
    HTT_STATS_PRINT(FATAL, "dup_past_within_window = %u",
            htt_stats_buf->dup_past_within_window);
    HTT_STATS_PRINT(FATAL, "rxdesc_err_decrypt = %u\n",
            htt_stats_buf->rxdesc_err_decrypt);
}

/*
 * htt_print_counter_tlv: display htt_counter_tlv
 * @tag_buf: buffer containing the tlv htt_counter_tlv
 *
 * return:void
 */
static void htt_print_counter_tlv(A_UINT32 *tag_buf)
{
    htt_counter_tlv *htt_stats_buf =
        (htt_counter_tlv *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                            = 0;
    A_CHAR   counter_name[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_COUNTER_TLV:");

    for (i = 0; i < HTT_MAX_COUNTER_NAME; i++) {
        index += snprintf(&counter_name[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->counter_name[i]);
    }

    HTT_STATS_PRINT(FATAL, "counter_name = %s ", counter_name);
    HTT_STATS_PRINT(FATAL, "count = %u\n",
            htt_stats_buf->count);
}

/*
 * htt_print_peer_stats_cmn_tlv: display htt_peer_stats_cmn_tlv
 * @tag_buf: buffer containing the tlv htt_peer_stats_cmn_tlv
 *
 * return:void
 */
static void htt_print_peer_stats_cmn_tlv(A_UINT32 *tag_buf)
{
    htt_peer_stats_cmn_tlv *htt_stats_buf =
        (htt_peer_stats_cmn_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_PEER_STATS_CMN_TLV:");
    HTT_STATS_PRINT(FATAL, "ppdu_cnt = %u",
            htt_stats_buf->ppdu_cnt);
    HTT_STATS_PRINT(FATAL, "mpdu_cnt = %u",
            htt_stats_buf->mpdu_cnt);
    HTT_STATS_PRINT(FATAL, "msdu_cnt = %u",
            htt_stats_buf->msdu_cnt);
    HTT_STATS_PRINT(FATAL, "pause_bitmap = %u",
            htt_stats_buf->pause_bitmap);
    HTT_STATS_PRINT(FATAL, "block_bitmap = %u",
            htt_stats_buf->block_bitmap);
    HTT_STATS_PRINT(FATAL, "last_rssi = %d",
            htt_stats_buf->rssi);
    HTT_STATS_PRINT(FATAL, "enqueued_count = %llu",
            htt_stats_buf->peer_enqueued_count_low | ((unsigned long long)htt_stats_buf->peer_enqueued_count_high << 32));
    HTT_STATS_PRINT(FATAL, "dequeued_count = %llu",
            htt_stats_buf->peer_dequeued_count_low | ((unsigned long long)htt_stats_buf->peer_dequeued_count_high << 32));
    HTT_STATS_PRINT(FATAL, "dropped_count = %llu",
            htt_stats_buf->peer_dropped_count_low | ((unsigned long long)htt_stats_buf->peer_dropped_count_high << 32));
    HTT_STATS_PRINT(FATAL, "transmitted_ppdu_bytes = %llu",
            htt_stats_buf->ppdu_transmitted_bytes_low | ((unsigned long long)htt_stats_buf->ppdu_transmitted_bytes_high << 32));
    HTT_STATS_PRINT(FATAL, "remove_mpdus_max_retries = %u",
            htt_stats_buf->remove_mpdus_max_retries);
    /*HTT_STATS_PRINT(FATAL, "current_timestamp = %u",
            htt_stats_buf->current_timestamp);*/
    HTT_STATS_PRINT(FATAL, "ttl_removed_count = %u",
            htt_stats_buf->peer_ttl_removed_count);
    HTT_STATS_PRINT(FATAL, "inactive_time = %u\n",
            htt_stats_buf->inactive_time);
}

/*
 * htt_print_ast_entry_tlv: display htt_ast_entry_tlv
 * @tag_buf: buffer containing the tlv htt_ast_entry_details_tlv
 *
 * return:void
 */
static void htt_print_ast_entry_tlv(A_UINT32 *tag_buf)
{
    htt_ast_entry_tlv *htt_stats_buf = (htt_ast_entry_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL,
        "ast_index : %u | mac_addr : %02x:%02x:%02x:%02x:%02x:%02x |",
        htt_stats_buf->ast_index,
        htt_stats_buf->mac_addr.mac_addr31to0 & 0xFF,
        (htt_stats_buf->mac_addr.mac_addr31to0 & 0xFF00) >> 8,
        (htt_stats_buf->mac_addr.mac_addr31to0 & 0xFF0000) >> 16,
        (htt_stats_buf->mac_addr.mac_addr31to0 & 0xFF000000) >> 24,
        (htt_stats_buf->mac_addr.mac_addr47to32 & 0xFF),
        (htt_stats_buf->mac_addr.mac_addr47to32 & 0xFF00) >> 8);

    if (htt_stats_buf->chip_id == 0x3) {
        HTT_STATS_PRINT(FATAL,
            "pdev_id   : %u | vdev_id  : %u | sw_peer_id : %u |",
            htt_stats_buf->pdev_id,
            htt_stats_buf->vdev_id,
            htt_stats_buf->sw_peer_id);
    } else {
        HTT_STATS_PRINT(FATAL,
            "chip_id   : %u | pdev_id  : %u | vdev_id    : %u "
            "| sw_peer_id : %u | ml_peer_id : %u |",
            htt_stats_buf->chip_id,
            htt_stats_buf->pdev_id,
            htt_stats_buf->vdev_id,
            htt_stats_buf->sw_peer_id,
            htt_stats_buf->ml_peer_id);
    }

    HTT_STATS_PRINT(FATAL,
        "next_hop : %u | mcast : %u | monitor_direct :%u | mesh_sta : %u | "
        "intra_bss  :  %u | mec : %u |",
        htt_stats_buf->next_hop,
        htt_stats_buf->mcast,
        htt_stats_buf->monitor_direct,
        htt_stats_buf->mesh_sta,
        htt_stats_buf->intra_bss,
        htt_stats_buf->mec);

    HTT_STATS_PRINT(FATAL,
        "onchip : %u | tx_monitor_override_sta : %u | rx_monitor_override_sta : %u |\n",
        htt_stats_buf->on_chip,
        htt_stats_buf->tx_monitor_override_sta,
        htt_stats_buf->rx_monitor_override_sta);
}

/*
 * htt_print_peer_details_tlv: display htt_peer_details_tlv
 * @tag_buf: buffer containing the tlv htt_peer_details_tlv
 *
 * return:void
 */
static void htt_print_peer_details_tlv(A_UINT32 *tag_buf)
{
    htt_peer_details_tlv *htt_stats_buf =
        (htt_peer_details_tlv *)tag_buf;
    A_UINT32 *htt_buf_ptr = (A_UINT32 *) htt_stats_buf;

    HTT_STATS_PRINT(FATAL, "HTT_PEER_DETAILS_TLV:");
    HTT_STATS_PRINT(FATAL, "peer_type = %u",
            htt_stats_buf->peer_type);
    HTT_STATS_PRINT(FATAL, "sw_peer_id = %u",
            htt_stats_buf->sw_peer_id);
    HTT_STATS_PRINT(FATAL, "vdev_id = %u",
            htt_stats_buf->vdev_pdev_ast_idx & 0xFF);
    HTT_STATS_PRINT(FATAL, "pdev_id = %u",
            (htt_stats_buf->vdev_pdev_ast_idx & 0xFF00) >> 8);
    HTT_STATS_PRINT(FATAL, "ast_idx = %u",
            (htt_stats_buf->vdev_pdev_ast_idx & 0xFFFF0000) >> 16);
    HTT_STATS_PRINT(FATAL, "mac_addr = %02x:%02x:%02x:%02x:%02x:%02x",
            htt_stats_buf->mac_addr.mac_addr31to0 & 0xFF,
            (htt_stats_buf->mac_addr.mac_addr31to0 & 0xFF00) >> 8,
            (htt_stats_buf->mac_addr.mac_addr31to0 & 0xFF0000) >> 16,
            (htt_stats_buf->mac_addr.mac_addr31to0 & 0xFF000000) >> 24,
            (htt_stats_buf->mac_addr.mac_addr47to32 & 0xFF),
            (htt_stats_buf->mac_addr.mac_addr47to32 & 0xFF00) >> 8);
    HTT_STATS_PRINT(FATAL, "peer_flags = 0x%x",
            htt_stats_buf->peer_flags);
    HTT_STATS_PRINT(FATAL, "qpeer_flags = 0x%x",
            htt_stats_buf->qpeer_flags);

    htt_buf_ptr += HTT_PEER_DETAILS_ML_PEER_OFFSET_DWORD;

    HTT_STATS_PRINT(FATAL, "link_idx = %u",
        HTT_PEER_DETAILS_GET(*htt_buf_ptr, LINK_IDX));

    if (HTT_PEER_DETAILS_GET(*htt_buf_ptr, ML_PEER_ID_VALID)) {
        HTT_STATS_PRINT(FATAL, "ml_peer_id = %u\n",
            HTT_PEER_DETAILS_GET(*htt_buf_ptr, ML_PEER_ID));
    } else {
        HTT_STATS_PRINT(FATAL, "ml_peer_id = INVALID\n");
    }

    HTT_STATS_PRINT(FATAL, "use_ppe  = 0x%x",
        HTT_PEER_DETAILS_GET(*htt_buf_ptr, USE_PPE));

    ++htt_buf_ptr;
    HTT_STATS_PRINT(FATAL, "src_info = 0x%x",
        HTT_PEER_DETAILS_GET(*htt_buf_ptr, SRC_INFO));
}

static const char *htt_ax_tx_ru_size_to_str(HTT_TX_PDEV_STATS_AX_RU_SIZE ru_size)
{
    switch (ru_size) {
    case HTT_TX_PDEV_STATS_AX_RU_SIZE_26:
        return "26";
    case HTT_TX_PDEV_STATS_AX_RU_SIZE_52:
        return "52";
    case HTT_TX_PDEV_STATS_AX_RU_SIZE_106:
        return "106";
    case HTT_TX_PDEV_STATS_AX_RU_SIZE_242:
        return "242";
    case HTT_TX_PDEV_STATS_AX_RU_SIZE_484:
        return "484";
    case HTT_TX_PDEV_STATS_AX_RU_SIZE_996:
        return "996";
    case HTT_TX_PDEV_STATS_AX_RU_SIZE_996x2:
        return "996x2";
    default:
        return "unknown";
    }
}

static const char *htt_be_tx_ru_size_to_str(HTT_RX_PDEV_STATS_BE_RU_SIZE ru_size)
{
    switch (ru_size) {
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_26:
        return "26";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_52:
        return "52";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_52_26:
        return "52+26";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_106:
        return "106";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_106_26:
        return "106+26";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_242:
        return "242";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_484:
        return "484";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_484_242:
        return "484+242";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_996:
        return "996";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_996_484:
        return "996+484";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_996_484_242:
        return "996+484+242";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_996x2:
        return "996x2";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_996x2_484:
        return "996x2+484";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_996x3:
        return "996x3";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_996x3_484:
        return "996x3+484";
    case HTT_TX_PDEV_STATS_BE_RU_SIZE_996x4:
        return "996x4";
    default:
        return "unknown";
    }
}

static const char *htt_stats_tx_ru_size_to_str(htt_stats_ru_type ru_type, A_UINT8 ru_size)
{
    if(ru_type == HTT_STATS_RU_TYPE_SINGLE_RU_ONLY) {
        return htt_ax_tx_ru_size_to_str(ru_size);
    } else if (ru_type == HTT_STATS_RU_TYPE_SINGLE_AND_MULTI_RU) {
        return htt_be_tx_ru_size_to_str(ru_size);
    } else {
        return "unknown";
    }
}

/*
 * htt_print_tx_per_rate_stats_tlv: display htt_tx_rate_stats_per_tlv
 * @tag_buf: buffer containing the tlv htt_tx_rate_stats_per_tlv
 *
 * return:void
 */
static void htt_print_tx_per_rate_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_rate_stats_per_tlv *htt_stats_buf =
        (htt_tx_rate_stats_per_tlv *)tag_buf;
    A_UINT8  i = 0, j = 0;
    A_UINT16 index;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_CHAR mode_prefix[][8] = {"su", "mu", "ofdma", "ulmu", "ulofdma"};
    A_UINT32 ru_size_cnt = 0;

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_DLSU) {
        HTT_STATS_PRINT(FATAL, "HTT_TX_PER_STATS:\n");
        HTT_STATS_PRINT(FATAL, "PER_STATS_SU:\n");
    } else if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_DLMUMIMO) {
        HTT_STATS_PRINT(FATAL, "\nPER_STATS_DL_MUMIMO:\n");
        j = 1;
    } else if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_DLOFDMA) {
        HTT_STATS_PRINT(FATAL, "\nPER_STATS_DL_OFDMA:\n");
        j = 2;
        if (htt_stats_buf->ru_type == HTT_STATS_RU_TYPE_SINGLE_RU_ONLY) {
            ru_size_cnt = HTT_TX_PDEV_STATS_NUM_AX_RU_SIZE_COUNTERS;
        } else if (htt_stats_buf->ru_type == HTT_STATS_RU_TYPE_SINGLE_AND_MULTI_RU) {
            ru_size_cnt = HTT_TX_PDEV_STATS_NUM_BE_RU_SIZE_COUNTERS;
        }
    } else if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULMUMIMO) {
        HTT_STATS_PRINT(FATAL, "\nHTT_RX_PER_STATS:\n");
        HTT_STATS_PRINT(FATAL, "\nPER_STATS_UL_MUMIMO:\n");
        j = 3;
    } else if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULOFDMA) {
        HTT_STATS_PRINT(FATAL, "\nPER_STATS_UL_OFDMA:\n");
        j = 4;
        if (htt_stats_buf->ru_type == HTT_STATS_RU_TYPE_SINGLE_RU_ONLY) {
            ru_size_cnt = HTT_TX_PDEV_STATS_NUM_AX_RU_SIZE_COUNTERS;
        } else if (htt_stats_buf->ru_type == HTT_STATS_RU_TYPE_SINGLE_AND_MULTI_RU) {
            ru_size_cnt = HTT_TX_PDEV_STATS_NUM_BE_RU_SIZE_COUNTERS;
        }
    } else {
        return;
    }

    HTT_STATS_PRINT(FATAL, "PER per BW:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_bw[i].ppdus_tried);
    }
    /*
     * Now i = HTT_TX_PDEV_STATS_NUM_BW_COUNTERS.
     * Adding 320 MHz ppdus tried.
     */
    {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_bw320.ppdus_tried);
    }
    if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULMUMIMO ||
        htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULOFDMA)
    {
        HTT_STATS_PRINT(FATAL, "data_ppdus_%s = %s ", mode_prefix[j], str_buf);
    } else {
        HTT_STATS_PRINT(FATAL, "ppdus_tried_%s = %s ", mode_prefix[j], str_buf);
    }
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_bw[i].ppdus_ack_failed);
    }
    /*
     * Now i = HTT_TX_PDEV_STATS_NUM_BW_COUNTERS.
     * Adding 320 MHz ppdus ack failed.
     */
    {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_bw320.ppdus_ack_failed);
    }

    if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULMUMIMO ||
        htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULOFDMA)
    {
        HTT_STATS_PRINT(FATAL,
            "non_data_ppdus_%s = %s ", mode_prefix[j], str_buf);
    } else {
        HTT_STATS_PRINT(FATAL,
            "ppdus_ack_failed_%s = %s ", mode_prefix[j], str_buf);
    }
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_bw[i].mpdus_tried);
    }
    /*
     * Now i = HTT_TX_PDEV_STATS_NUM_BW_COUNTERS.
     * Adding 320 MHz mpdus tried.
     */
    {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_bw320.mpdus_tried);
    }

    HTT_STATS_PRINT(FATAL, "mpdus_tried_%s = %s ", mode_prefix[j], str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_bw[i].mpdus_failed);
    }
    /*
     * Now i = HTT_TX_PDEV_STATS_NUM_BW_COUNTERS.
     * Adding 320 MHz mpdus failed.
     */
    {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_bw320.mpdus_failed);
    }

    HTT_STATS_PRINT(FATAL, "mpdus_failed_%s = %s \n", mode_prefix[j], str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    HTT_STATS_PRINT(FATAL, "PER per NSS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_nss[i].ppdus_tried);
    }

    if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULMUMIMO ||
        htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULOFDMA)
    {
        HTT_STATS_PRINT(FATAL, "data_ppdus_%s = %s ", mode_prefix[j], str_buf);
    } else {
        HTT_STATS_PRINT(FATAL, "ppdus_tried_%s = %s ", mode_prefix[j], str_buf);
    }
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_nss[i].ppdus_ack_failed);
    }

    if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULMUMIMO ||
        htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULOFDMA) {
        HTT_STATS_PRINT(FATAL,
            "non_data_ppdus_%s = %s ", mode_prefix[j], str_buf);
    } else {
        HTT_STATS_PRINT(FATAL,
            "ppdus_ack_failed_%s = %s ", mode_prefix[j], str_buf);
    }
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_nss[i].mpdus_tried);
    }

    HTT_STATS_PRINT(FATAL, "mpdus_tried_%s = %s ", mode_prefix[j], str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_nss[i].mpdus_failed);
    }

    HTT_STATS_PRINT(FATAL, "mpdus_failed_%s = %s \n", mode_prefix[j],str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    HTT_STATS_PRINT(FATAL, "PER per Mcs:");

    for (i = 0; i < HTT_TX_TXBF_RATE_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_mcs[i].ppdus_tried);
    }

    if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULMUMIMO ||
        htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULOFDMA)
    {
        HTT_STATS_PRINT(FATAL, "data_ppdus_%s = %s ",mode_prefix[j], str_buf);
    } else {
        HTT_STATS_PRINT(FATAL, "ppdus_tried_%s = %s ",mode_prefix[j], str_buf);
    }
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_TXBF_RATE_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_mcs[i].ppdus_ack_failed);
    }

    if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULMUMIMO ||
        htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULOFDMA)
    {
        HTT_STATS_PRINT(FATAL,
            "non_data_ppdus_%s = %s ",mode_prefix[j], str_buf);
    } else {
        HTT_STATS_PRINT(FATAL,
            "ppdus_ack_failed_%s = %s ",mode_prefix[j], str_buf);
    }
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_TXBF_RATE_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_mcs[i].mpdus_tried);
    }

    HTT_STATS_PRINT(FATAL, "mpdus_tried_%s = %s ", mode_prefix[j],str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_TXBF_RATE_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->per_mcs[i].mpdus_failed);
    }

    HTT_STATS_PRINT(FATAL, "mpdus_failed_%s = %s \n", mode_prefix[j], str_buf);

    if ((htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_DLOFDMA ||
         htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULOFDMA) &&
        htt_stats_buf->ru_type != HTT_STATS_RU_TYPE_INVALID)
    {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        HTT_STATS_PRINT(FATAL, "PER per RU :");

        for (i = 0; i < ru_size_cnt; i++) {
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %s:%u,",
                htt_stats_tx_ru_size_to_str(htt_stats_buf->ru_type, i),
                htt_stats_buf->per_ru[i].ppdus_tried);
        }

        if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULOFDMA) {
            HTT_STATS_PRINT(FATAL,
                "data_ppdus_%s = %s ",mode_prefix[j], str_buf);
        } else {
            HTT_STATS_PRINT(FATAL,
                "ppdus_tried_%s = %s ",mode_prefix[j], str_buf);
        }
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (i = 0; i < ru_size_cnt; i++) {
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %s:%u,", htt_stats_tx_ru_size_to_str(htt_stats_buf->ru_type, i), htt_stats_buf->per_ru[i].ppdus_ack_failed);
        }

        if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_ULOFDMA) {
            HTT_STATS_PRINT(FATAL,
                "non_data_ppdus_%s = %s ",mode_prefix[j], str_buf);
        } else {
            HTT_STATS_PRINT(FATAL,
                "ppdus_ack_failed_%s = %s ",mode_prefix[j], str_buf);
        }
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (i = 0; i < ru_size_cnt; i++) {
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %s:%u,", htt_stats_tx_ru_size_to_str(htt_stats_buf->ru_type, i), htt_stats_buf->per_ru[i].mpdus_tried);
        }

        HTT_STATS_PRINT(FATAL, "mpdus_tried_%s = %s ", mode_prefix[j],str_buf);
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (i = 0; i < ru_size_cnt; i++) {
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %s:%u,", htt_stats_tx_ru_size_to_str(htt_stats_buf->ru_type, i), htt_stats_buf->per_ru[i].mpdus_failed);
        }

        HTT_STATS_PRINT(FATAL, "mpdus_failed_%s = %s ",mode_prefix[j], str_buf);
    }

    if (htt_stats_buf->rc_mode == HTT_STATS_RC_MODE_DLMUMIMO) {
        HTT_STATS_PRINT(FATAL, "last_probed_bw  = %u ",
            htt_stats_buf->last_probed_bw);
        HTT_STATS_PRINT(FATAL, "last_probed_nss = %u ",
            htt_stats_buf->last_probed_nss);
        HTT_STATS_PRINT(FATAL, "last_probed_mcs = %u ",
            htt_stats_buf->last_probed_mcs);

        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (i = 0; i < HTT_RC_MODE_2D_COUNT; i++) {
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->probe_cnt_per_rcmode[i]);
        }
        HTT_STATS_PRINT(FATAL, "MU Probe count per RC MODE = %s", str_buf);
    }
}

/*
 * htt_print_tx_peer_rate_txbf_stats_tlv: display htt_tx_peer_rate_txbf_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_peer_rate_txbf_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_peer_rate_txbf_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_txbf_rate_stats_tlv *htt_stats_buf =
        (htt_tx_pdev_txbf_rate_stats_tlv *)tag_buf;
    A_UINT8  i;
    A_UINT16 index;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN]               = {0};
    A_UINT32 tag_len                                   = (HTT_STATS_TLV_LENGTH_GET(*tag_buf));
    A_UINT32 legacy_stats_end_pos;
    A_UINT32 j;

    legacy_stats_end_pos =
        offsetof(htt_tx_pdev_txbf_rate_stats_tlv,tx_legacy_ofdm_rate) -
        sizeof(htt_tlv_hdr_t) +
        sizeof(htt_stats_buf->tx_legacy_ofdm_rate);

    HTT_STATS_PRINT(FATAL, "HTT_STATS_PDEV_TX_RATE_TXBF_STATS:");

    if (legacy_stats_end_pos <= tag_len) {
        HTT_STATS_PRINT(FATAL,
            "Legacy OFDM Rates: 6 Mbps: %u, 9 Mbps: %u, 12 Mbps: %u, 18 Mbps: %u\n"
            "                   24 Mbps: %u, 36 Mbps: %u, 48 Mbps: %u, 54 Mbps: %u",
            htt_stats_buf->tx_legacy_ofdm_rate[0],
            htt_stats_buf->tx_legacy_ofdm_rate[1],
            htt_stats_buf->tx_legacy_ofdm_rate[2],
            htt_stats_buf->tx_legacy_ofdm_rate[3],
            htt_stats_buf->tx_legacy_ofdm_rate[4],
            htt_stats_buf->tx_legacy_ofdm_rate[5],
            htt_stats_buf->tx_legacy_ofdm_rate[6],
            htt_stats_buf->tx_legacy_ofdm_rate[7]);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_TXBF_RATE_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_su_ol_mcs[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_ol_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_TXBF_RATE_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_su_ibf_mcs[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_ibf_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_TXBF_RATE_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_su_txbf_mcs[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_txbf_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_su_ol_nss[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_ol_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_su_ibf_nss[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_ibf_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_su_txbf_nss[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_txbf_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_TXBF_RATE_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_su_ol_bw[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_ol_bw = %s ", str_buf);

    for (i = 0; i < HTT_TX_TXBF_RATE_STATS_NUM_REDUCED_CHAN_TYPES; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (j = 0; j < HTT_TX_TXBF_RATE_STATS_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_stats_buf->reduced_tx_su_ol_bw[i][j]);
        }

        HTT_STATS_PRINT(FATAL, "%s = %s ", i == 0 ? "half_tx_ol_bw":"quarter_tx_ol_bw", str_buf);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_TXBF_RATE_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_su_ibf_bw[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_ibf_bw = %s ", str_buf);

    for (i = 0; i < HTT_TX_TXBF_RATE_STATS_NUM_REDUCED_CHAN_TYPES; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (j = 0; j < HTT_TX_TXBF_RATE_STATS_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_stats_buf->reduced_tx_su_ibf_bw[i][j]);
        }

        HTT_STATS_PRINT(FATAL, "%s = %s ", i == 0 ? "half_tx_ibf_bw":"quarter_tx_ibf_bw", str_buf);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_TXBF_RATE_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_su_txbf_bw[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_txbf_bw = %s ", str_buf);

    for (i = 0; i < HTT_TX_TXBF_RATE_STATS_NUM_REDUCED_CHAN_TYPES; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (j = 0; j < HTT_TX_TXBF_RATE_STATS_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_stats_buf->reduced_tx_su_txbf_bw[i][j]);
        }

        HTT_STATS_PRINT(FATAL, "%s = %s ", i == 0 ? "half_tx_txbf_bw":"quarter_tx_txbf_bw", str_buf);
    }

    HTT_STATS_PRINT(FATAL, "HTT_STATS_PDEV_TXBF_FLAG_RETURN_STATS:");

    HTT_STATS_PRINT(FATAL,
        "TXBF_reason_code_stats: "
        "0: %u, 1: %u, 2: %u, 3: %u, 4: %u, 5: %u, 6: %u, 7:%u",
        htt_stats_buf->txbf_flag_set_mu_mode,
        htt_stats_buf->txbf_flag_set_final_status,
        htt_stats_buf->txbf_flag_not_set_verified_txbf_mode,
        htt_stats_buf->txbf_flag_not_set_disable_p2p_access,
        htt_stats_buf->txbf_flag_not_set_max_nss_reached_in_he160,
        htt_stats_buf->txbf_flag_not_set_disable_ul_dl_ofdma,
        htt_stats_buf->txbf_flag_not_set_mcs_threshold_value,
        htt_stats_buf->txbf_flag_not_set_final_status);
}

/*
 * htt_print_tx_peer_rate_stats_tlv: display htt_tx_peer_rate_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_peer_rate_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_peer_rate_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_peer_rate_stats_tlv *htt_stats_buf =
        (htt_tx_peer_rate_stats_tlv *)tag_buf;
    A_UINT8  i, j;
    A_UINT16 index                                     = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN]               = {0};
    A_CHAR   *tx_gi[HTT_TX_PEER_STATS_NUM_GI_COUNTERS] = {0};

    for (i = 0; i < HTT_TX_PEER_STATS_NUM_GI_COUNTERS; i++) {
        tx_gi[i] = (A_CHAR *)malloc(HTT_MAX_STRING_LEN);
        if (!tx_gi[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(tx_gi[j]);
           }
           return;
        }
    }

    HTT_STATS_PRINT(FATAL, "HTT_TX_PEER_RATE_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "tx_ldpc = %u",
            htt_stats_buf->tx_ldpc);
    HTT_STATS_PRINT(FATAL, "rts_cnt = %u",
            htt_stats_buf->rts_cnt);
    HTT_STATS_PRINT(FATAL, "ack_rssi = %u",
            htt_stats_buf->ack_rssi);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PEER_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_mcs[i]);
    }
    for (i = 0; i < HTT_TX_PEER_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_TX_PEER_STATS_NUM_MCS_COUNTERS,
                htt_stats_buf->tx_mcs_ext[i]);
    }
    for (i = 0; i < HTT_TX_PEER_STATS_NUM_EXTRA2_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_TX_PEER_STATS_NUM_EXTRA2_MCS_COUNTERS,
                htt_stats_buf->tx_mcs_ext_2[i]);
    }
    HTT_STATS_PRINT(FATAL, "tx_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PEER_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_su_mcs[i]);
    }
    for (i = 0; i < HTT_TX_PEER_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_TX_PEER_STATS_NUM_MCS_COUNTERS,
                htt_stats_buf->tx_su_mcs_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "tx_su_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PEER_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_mu_mcs[i]);
    }
    for (i = 0; i < HTT_TX_PEER_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_TX_PEER_STATS_NUM_MCS_COUNTERS,
                htt_stats_buf->tx_mu_mcs_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "tx_mu_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PEER_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->tx_nss[i]);
    }
    HTT_STATS_PRINT(FATAL, "tx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PEER_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_bw[i]);
    }
    index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->tx_bw_320mhz);
    HTT_STATS_PRINT(FATAL, "tx_bw = %s ", str_buf);

    for (i = 0; i < HTT_TX_PEER_STATS_NUM_REDUCED_CHAN_TYPES; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (j = 0; j < HTT_TX_PEER_STATS_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_stats_buf->reduced_tx_bw[i][j]);
        }

        HTT_STATS_PRINT(FATAL, "%s = %s ", i == 0 ? "half_tx_bw" : "quarter_tx_bw", str_buf);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PEER_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_stbc[i]);
    }
    for (i = 0; i < HTT_TX_PEER_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_TX_PEER_STATS_NUM_MCS_COUNTERS,
                htt_stats_buf->tx_stbc_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "tx_stbc = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PEER_STATS_NUM_PREAMBLE_TYPES; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_pream[i]);
    }
    HTT_STATS_PRINT(FATAL, "tx_pream = %s ", str_buf);

    for (j = 0; j < HTT_TX_PEER_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        for (i = 0; i < HTT_TX_PEER_STATS_NUM_MCS_COUNTERS; i++) {
            index += snprintf(&tx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->tx_gi[j][i]);
        }
        for (i = 0; i < HTT_TX_PEER_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
            index += snprintf(&tx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i + HTT_TX_PEER_STATS_NUM_MCS_COUNTERS,
                    htt_stats_buf->tx_gi_ext[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "tx_gi[%u] = %s ", j, tx_gi[j]);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PEER_STATS_NUM_DCM_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_dcm[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_dcm = %s\n", str_buf);

    for (i = 0; i < HTT_TX_PEER_STATS_NUM_GI_COUNTERS; i++) {
        free(tx_gi[i]);
    }
}

/*
 * htt_print_rx_peer_rate_stats_tlv: display htt_rx_peer_rate_stats_tlv
 * @tag_buf: buffer containing the tlv htt_rx_peer_rate_stats_tlv
 *
 * return:void
 */
static void htt_print_rx_peer_rate_stats_tlv(A_UINT32 *tag_buf)
{
    htt_rx_peer_rate_stats_tlv *htt_stats_buf =
        (htt_rx_peer_rate_stats_tlv *)tag_buf;
    A_UINT8  i, j;
    A_UINT16 index;
    A_CHAR   *rssi_chain[HTT_RX_PEER_STATS_NUM_SPATIAL_STREAMS];
    A_CHAR   *rx_gi[HTT_RX_PEER_STATS_NUM_GI_COUNTERS];
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};

    for (i = 0; i < HTT_RX_PEER_STATS_NUM_SPATIAL_STREAMS; i++) {
        rssi_chain[i] = malloc(HTT_MAX_STRING_LEN);
        if (!rssi_chain[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(rssi_chain[j]);
           }
           return;
        }
    }

    for (i = 0; i < HTT_RX_PEER_STATS_NUM_GI_COUNTERS; i++) {
        rx_gi[i] = malloc(HTT_MAX_STRING_LEN);
        if (!rx_gi[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(rx_gi[j]);
           }
           for (j = 0; j < HTT_RX_PEER_STATS_NUM_SPATIAL_STREAMS; j++) {
                free(rssi_chain[j]);
           }
           return;
        }
    }

    HTT_STATS_PRINT(FATAL, "HTT_RX_PEER_RATE_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "nsts = %u",
            htt_stats_buf->nsts);
    HTT_STATS_PRINT(FATAL, "rx_ldpc = %u",
            htt_stats_buf->rx_ldpc);
    HTT_STATS_PRINT(FATAL, "rts_cnt = %u",
            htt_stats_buf->rts_cnt);
    HTT_STATS_PRINT(FATAL, "rssi_mgmt = %u",
            htt_stats_buf->rssi_mgmt);
    HTT_STATS_PRINT(FATAL, "rssi_data = %u",
            htt_stats_buf->rssi_data);
    HTT_STATS_PRINT(FATAL, "rssi_comb = %d",
            htt_stats_buf->rssi_comb);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PEER_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_mcs[i]);
    }
    for (i = 0; i < HTT_RX_PEER_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_RX_PEER_STATS_NUM_MCS_COUNTERS,
                htt_stats_buf->rx_mcs_ext[i]);
    }
    for (i = 0; i < HTT_RX_PEER_STATS_NUM_EXTRA2_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_RX_PEER_STATS_NUM_EXTRA2_MCS_COUNTERS,
                htt_stats_buf->rx_mcs_ext_2[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PEER_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->rx_nss[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PEER_STATS_NUM_DCM_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_dcm[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_dcm = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PEER_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_stbc[i]);
    }
    for (i = 0; i < HTT_RX_PEER_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_RX_PEER_STATS_NUM_MCS_COUNTERS,
                htt_stats_buf->rx_stbc_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_stbc = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PEER_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_bw[i]);
    }
    index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->rx_bw_320mhz);
    HTT_STATS_PRINT(FATAL, "rx_bw = %s ", str_buf);

    for (i = 0; i < HTT_RX_PEER_STATS_NUM_REDUCED_CHAN_TYPES; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (j = 0; j < HTT_RX_PEER_STATS_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_stats_buf->reduced_rx_bw[i][j]);
        }

        HTT_STATS_PRINT(FATAL, "%s = %s ", i == 0 ? "half_rx_bw" : "quarter_rx_bw", str_buf);

    }

    for (j = 0; j < HTT_RX_PEER_STATS_NUM_SPATIAL_STREAMS; j++) {
        memset(rssi_chain[j], 0x0, HTT_MAX_STRING_LEN);
        index = 0;

        for (i = 0; i < HTT_RX_PEER_STATS_NUM_BW_COUNTERS; i++) {
            index += snprintf(&rssi_chain[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->rssi_chain[j][i]);
        }

        HTT_STATS_PRINT(FATAL, "rssi_chain[%u] = %s ", j, rssi_chain[j]);
    }

    for (j = 0; j < HTT_RX_PEER_STATS_NUM_SPATIAL_STREAMS; j++) {
        memset(rssi_chain[j], 0x0, HTT_MAX_STRING_LEN);
        index = 0;

        for (i = 0; i < HTT_RX_PEER_STATS_NUM_BW_EXT_COUNTERS; i++) {
            index += snprintf(&rssi_chain[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->rssi_chain_ext[j][i]);
        }

        HTT_STATS_PRINT(FATAL, "rssi_chain_ext[%u] = %s ", j, rssi_chain[j]);
    }

    for (j = 0; j < HTT_RX_PEER_STATS_NUM_GI_COUNTERS; j++) {
        memset(rx_gi[j], 0x0, HTT_MAX_STRING_LEN);
        index = 0;

        for (i = 0; i < HTT_RX_PEER_STATS_NUM_MCS_COUNTERS; i++) {
            index += snprintf(&rx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->rx_gi[j][i]);
        }
        for (i = 0; i < HTT_RX_PEER_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
            index += snprintf(&rx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i + HTT_RX_PEER_STATS_NUM_MCS_COUNTERS,
                    htt_stats_buf->rx_gi_ext[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "rx_gi[%u] = %s ", j, rx_gi[j]);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PEER_STATS_NUM_PREAMBLE_TYPES; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_pream[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_pream = %s", str_buf);

    for (i = 0; i < HTT_RX_PEER_STATS_NUM_SPATIAL_STREAMS; i++) {
        free(rssi_chain[i]);
    }

    for (i = 0; i < HTT_RX_PEER_STATS_NUM_GI_COUNTERS; i++) {
        free(rx_gi[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_ulofdma_non_data_ppdu = %u",
            htt_stats_buf->rx_ulofdma_non_data_ppdu);
    HTT_STATS_PRINT(FATAL, "rx_ulofdma_data_ppdu = %u",
            htt_stats_buf->rx_ulofdma_data_ppdu);
    HTT_STATS_PRINT(FATAL, "rx_ulofdma_mpdu_ok = %u",
            htt_stats_buf->rx_ulofdma_mpdu_ok);
    HTT_STATS_PRINT(FATAL, "rx_ulofdma_mpdu_fail = %u",
            htt_stats_buf->rx_ulofdma_mpdu_fail);

    HTT_STATS_PRINT(FATAL, "rx_ulmumimo_non_data_ppdu = %u",
            htt_stats_buf->rx_ulmumimo_non_data_ppdu);
    HTT_STATS_PRINT(FATAL, "rx_ulmumimo_data_ppdu = %u",
            htt_stats_buf->rx_ulmumimo_data_ppdu);
    HTT_STATS_PRINT(FATAL, "rx_ulmumimo_mpdu_ok = %u",
            htt_stats_buf->rx_ulmumimo_mpdu_ok);
    HTT_STATS_PRINT(FATAL, "rx_ulmumimo_mpdu_fail = %u",
            htt_stats_buf->rx_ulmumimo_mpdu_fail);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PEER_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%d,", i, htt_stats_buf->rx_ul_fd_rssi[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_ul_fd_rssi = %s", str_buf);

    HTT_STATS_PRINT(FATAL, "per_chain_rssi_pkt_type = %#x",
            htt_stats_buf->per_chain_rssi_pkt_type);

    for (j = 0; j < HTT_RX_PEER_STATS_NUM_SPATIAL_STREAMS; j++) {
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        index = 0;

        for (i = 0; i < HTT_RX_PEER_STATS_NUM_BW_COUNTERS; i++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%hhd,",
                    i, htt_stats_buf->rx_per_chain_rssi_in_dbm[j][i]);
        }

        HTT_STATS_PRINT(FATAL,
                "rx_per_chain_rssi_in_dbm[%u] = %s ", j, str_buf);
    }

    for (j = 0; j < HTT_RX_PEER_STATS_NUM_SPATIAL_STREAMS; j++) {
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        index = 0;

        for (i = 0; i < HTT_RX_PEER_STATS_NUM_BW_EXT_COUNTERS; i++) {
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%hhd,",
                i, htt_stats_buf->rx_per_chain_rssi_in_dbm_ext[j][i]);
        }

        HTT_STATS_PRINT(FATAL,
                "rx_per_chain_rssi_in_dbm_ext[%u] = %s ", j, str_buf);
    }

    HTT_STATS_PRINT(FATAL, "\n");
}

/*
 * htt_print_tx_hwq_mu_mimo_sch_stats_tlv: display htt_tx_hwq_mu_mimo_sch_stats
 * @tag_buf: buffer containing the tlv htt_tx_hwq_mu_mimo_sch_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_hwq_mu_mimo_sch_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_hwq_mu_mimo_sch_stats_tlv *htt_stats_buf =
        (htt_tx_hwq_mu_mimo_sch_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_HWQ_MU_MIMO_SCH_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mu_mimo_sch_posted = %u",
            htt_stats_buf->mu_mimo_sch_posted);
    HTT_STATS_PRINT(FATAL, "mu_mimo_sch_failed = %u",
            htt_stats_buf->mu_mimo_sch_failed);
    HTT_STATS_PRINT(FATAL, "mu_mimo_ppdu_posted = %u\n",
            htt_stats_buf->mu_mimo_ppdu_posted);
}

/*
 * htt_print_tx_hwq_mu_mimo_mpdu_stats_tlv: display htt_tx_hwq_mu_mimo_mpdu_stats
 * @tag_buf: buffer containing the tlv htt_tx_hwq_mu_mimo_mpdu_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_hwq_mu_mimo_mpdu_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_hwq_mu_mimo_mpdu_stats_tlv *htt_stats_buf =
        (htt_tx_hwq_mu_mimo_mpdu_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_HWQ_MU_MIMO_MPDU_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mu_mimo_mpdus_queued_usr = %u",
            htt_stats_buf->mu_mimo_mpdus_queued_usr);
    HTT_STATS_PRINT(FATAL, "mu_mimo_mpdus_tried_usr = %u",
            htt_stats_buf->mu_mimo_mpdus_tried_usr);
    HTT_STATS_PRINT(FATAL, "mu_mimo_mpdus_failed_usr = %u",
            htt_stats_buf->mu_mimo_mpdus_failed_usr);
    HTT_STATS_PRINT(FATAL, "mu_mimo_mpdus_requeued_usr = %u",
            htt_stats_buf->mu_mimo_mpdus_requeued_usr);
    HTT_STATS_PRINT(FATAL, "mu_mimo_err_no_ba_usr = %u",
            htt_stats_buf->mu_mimo_err_no_ba_usr);
    HTT_STATS_PRINT(FATAL, "mu_mimo_mpdu_underrun_usr = %u",
            htt_stats_buf->mu_mimo_mpdu_underrun_usr);
    HTT_STATS_PRINT(FATAL, "mu_mimo_ampdu_underrun_usr = %u\n",
            htt_stats_buf->mu_mimo_ampdu_underrun_usr);
}

/*
 * htt_print_tx_hwq_mu_mimo_cmn_stats_tlv: display htt_tx_hwq_mu_mimo_cmn_stats
 * @tag_buf: buffer containing the tlv htt_tx_hwq_mu_mimo_cmn_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_hwq_mu_mimo_cmn_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_hwq_mu_mimo_cmn_stats_tlv *htt_stats_buf =
        (htt_tx_hwq_mu_mimo_cmn_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_HWQ_MU_MIMO_CMN_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__hwq_id__word & 0xFF);
    HTT_STATS_PRINT(FATAL, "hwq_id = %u\n",
            (htt_stats_buf->mac_id__hwq_id__word & 0xFF00) >> 8);
    /*HTT_STATS_PRINT(FATAL, "word = %u\n",
            (htt_stats_buf->mac_id__hwq_id__word & 0xFFFF0000) >> 16);*/
}

/*
 * htt_print_tx_hwq_stats_cmn_tlv: display htt_tx_hwq_stats_cmn_tlv
 * @tag_buf: buffer containing the tlv htt_tx_hwq_stats_cmn_tlv
 *
 * return:void
 */
static void htt_print_tx_hwq_stats_cmn_tlv(A_UINT32 *tag_buf)
{
    htt_tx_hwq_stats_cmn_tlv *htt_stats_buf =
        (htt_tx_hwq_stats_cmn_tlv *)tag_buf;

    // TODO: HKDBG
    HTT_STATS_PRINT(FATAL, "HTT_TX_HWQ_STATS_CMN_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__hwq_id__word & 0xFF);
    HTT_STATS_PRINT(FATAL, "hwq_id = %u",
            (htt_stats_buf->mac_id__hwq_id__word & 0xFF00) >> 8);
    /*HTT_STATS_PRINT(FATAL, "word = %u\n",
            (htt_stats_buf->mac_id__hwq_id__word & 0xFFFF0000) >> 16);*/
    HTT_STATS_PRINT(FATAL, "xretry = %u",
            htt_stats_buf->xretry);
    HTT_STATS_PRINT(FATAL, "underrun_cnt = %u",
            htt_stats_buf->underrun_cnt);
    HTT_STATS_PRINT(FATAL, "flush_cnt = %u",
            htt_stats_buf->flush_cnt);
    HTT_STATS_PRINT(FATAL, "filt_cnt = %u",
            htt_stats_buf->filt_cnt);
    HTT_STATS_PRINT(FATAL, "null_mpdu_bmap = %u",
            htt_stats_buf->null_mpdu_bmap);
    HTT_STATS_PRINT(FATAL, "user_ack_failure = %u",
            htt_stats_buf->user_ack_failure);
    HTT_STATS_PRINT(FATAL, "ack_tlv_proc = %u",
            htt_stats_buf->ack_tlv_proc);
    HTT_STATS_PRINT(FATAL, "sched_id_proc = %u",
            htt_stats_buf->sched_id_proc);
    HTT_STATS_PRINT(FATAL, "null_mpdu_tx_count = %u",
            htt_stats_buf->null_mpdu_tx_count);
    HTT_STATS_PRINT(FATAL, "mpdu_bmap_not_recvd = %u",
            htt_stats_buf->mpdu_bmap_not_recvd);
    HTT_STATS_PRINT(FATAL, "num_bar = %u",
            htt_stats_buf->num_bar);
    HTT_STATS_PRINT(FATAL, "rts = %u",
            htt_stats_buf->rts);
    HTT_STATS_PRINT(FATAL, "cts2self = %u",
            htt_stats_buf->cts2self);
    HTT_STATS_PRINT(FATAL, "qos_null = %u",
            htt_stats_buf->qos_null);
    HTT_STATS_PRINT(FATAL, "mpdu_tried_cnt = %u",
            htt_stats_buf->mpdu_tried_cnt);
    HTT_STATS_PRINT(FATAL, "mpdu_queued_cnt = %u",
            htt_stats_buf->mpdu_queued_cnt);
    HTT_STATS_PRINT(FATAL, "mpdu_ack_fail_cnt = %u",
            htt_stats_buf->mpdu_ack_fail_cnt);
    HTT_STATS_PRINT(FATAL, "mpdu_filt_cnt = %u",
            htt_stats_buf->mpdu_filt_cnt);
    HTT_STATS_PRINT(FATAL, "false_mpdu_ack_count = %u",
            htt_stats_buf->false_mpdu_ack_count);
    HTT_STATS_PRINT(FATAL, "txq_timeout = %u\n",
            htt_stats_buf->txq_timeout);
}

/*
 * htt_print_tx_hwq_difs_latency_stats_tlv_v: display
 *                    htt_tx_hwq_difs_latency_stats_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_hwq_difs_latency_stats_tlv_v
 *
 * return:void
 */
static void htt_print_tx_hwq_difs_latency_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_hwq_difs_latency_stats_tlv_v *htt_stats_buf =
        (htt_tx_hwq_difs_latency_stats_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                 = 0;
    A_CHAR   difs_latency_hist[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                               = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_HWQ_DIFS_LATENCY_STATS_TLV_V:");
    HTT_STATS_PRINT(FATAL, "hist_intvl = %u",
            htt_stats_buf->hist_intvl);

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&difs_latency_hist[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->difs_latency_hist[i]);
    }

    HTT_STATS_PRINT(FATAL, "difs_latency_hist = %s\n", difs_latency_hist);
}

/*
 * htt_print_tx_hwq_cmd_result_stats_tlv_v: display htt_tx_hwq_cmd_result_stats
 * @tag_buf: buffer containing the tlv htt_tx_hwq_cmd_result_stats_tlv_v
 *
 * return:void
 */
static void htt_print_tx_hwq_cmd_result_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_hwq_cmd_result_stats_tlv_v *htt_stats_buf =
        (htt_tx_hwq_cmd_result_stats_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                          = 0;
    A_CHAR   cmd_result[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                        = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_HWQ_CMD_RESULT_STATS_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&cmd_result[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->cmd_result[i]);
    }

    HTT_STATS_PRINT(FATAL, "cmd_result = %s \n", cmd_result);
}

/*
 * htt_print_tx_hwq_cmd_stall_stats_tlv_v: display htt_tx_hwq_cmd_stall_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_hwq_cmd_stall_stats_tlv_v
 *
 * return:void
 */
static void htt_print_tx_hwq_cmd_stall_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_hwq_cmd_stall_stats_tlv_v *htt_stats_buf =
        (htt_tx_hwq_cmd_stall_stats_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                = 0;
    A_CHAR   cmd_stall_status[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                              = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_HWQ_CMD_STALL_STATS_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&cmd_stall_status[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->cmd_stall_status[i]);
    }

    HTT_STATS_PRINT(FATAL, "cmd_stall_status = %s\n", cmd_stall_status);
}

/*
 * htt_print_tx_hwq_fes_result_stats_tlv_v: display htt_tx_hwq_fes_result_stats
 * @tag_buf: buffer containing the tlv htt_tx_hwq_fes_result_stats_tlv_v
 *
 * return:void
 */
static void htt_print_tx_hwq_fes_result_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_hwq_fes_result_stats_tlv_v *htt_stats_buf =
        (htt_tx_hwq_fes_result_stats_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                          = 0;
    A_CHAR   fes_result[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                        = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_HWQ_FES_RESULT_STATS_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&fes_result[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->fes_result[i]);
    }

    HTT_STATS_PRINT(FATAL, "fes_result = %s \n", fes_result);
}

/*
 * htt_print_tx_hwq_tried_mpdu_cnt_hist_tlv_v: display htt_tx_hwq_tried_mpdu_cnt_hist
 * @tag_buf: buffer containing the tlv htt_tx_hwq_tried_mpdu_cnt_hist_tlv_v
 *
 * return:void
 */
static void htt_print_tx_hwq_tried_mpdu_cnt_hist_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_hwq_tried_mpdu_cnt_hist_tlv_v *htt_stats_buf =
        (htt_tx_hwq_tried_mpdu_cnt_hist_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                   = 0;
    A_CHAR   tried_mpdu_cnt_hist[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 num_elements                            =
        ((HTT_STATS_TLV_LENGTH_GET(*tag_buf) - sizeof(htt_stats_buf->hist_bin_size)) >> 2);
    A_UINT32 required_buffer_size = HTT_MAX_PRINT_CHAR_PER_ELEM * num_elements;

    HTT_STATS_PRINT(FATAL, "HTT_TX_HWQ_TRIED_MPDU_CNT_HIST_TLV_V:");
    HTT_STATS_PRINT(FATAL, "TRIED_MPDU_CNT_HIST_BIN_SIZE : %u", htt_stats_buf->hist_bin_size);

    if (required_buffer_size < HTT_MAX_STRING_LEN) {
        for (i = 0; i < num_elements; i++) {
            index += snprintf(&tried_mpdu_cnt_hist[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i, htt_stats_buf->tried_mpdu_cnt_hist[i]);
        }

        HTT_STATS_PRINT(FATAL, "tried_mpdu_cnt_hist = %s \n", tried_mpdu_cnt_hist);
    } else {
        HTT_STATS_PRINT(FATAL, "INSUFFICIENT PRINT BUFFER ");
    }
}

/*
 * htt_print_tx_hwq_txop_used_cnt_hist_tlv_v: display htt_tx_hwq_txop_used_cnt_hist
 * @tag_buf: buffer containing the tlv htt_tx_hwq_txop_used_cnt_hist_tlv_v
 *
 * return:void
 */
static void htt_print_tx_hwq_txop_used_cnt_hist_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_hwq_txop_used_cnt_hist_tlv_v *htt_stats_buf =
        (htt_tx_hwq_txop_used_cnt_hist_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                  = 0;
    A_CHAR   txop_used_cnt_hist[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 num_elements                           = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);
    A_UINT32 required_buffer_size                   = HTT_MAX_PRINT_CHAR_PER_ELEM * num_elements;

    HTT_STATS_PRINT(FATAL, "HTT_TX_HWQ_TXOP_USED_CNT_HIST_TLV_V:");

    if (required_buffer_size < HTT_MAX_STRING_LEN) {
        for (i = 0; i < num_elements; i++) {
            index += snprintf(&txop_used_cnt_hist[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i, htt_stats_buf->txop_used_cnt_hist[i]);
        }

        HTT_STATS_PRINT(FATAL, "txop_used_cnt_hist = %s \n", txop_used_cnt_hist);
    } else {
        HTT_STATS_PRINT(FATAL, "INSUFFICIENT PRINT BUFFER ");
    }
}

/*
 * htt_tx_sounding_stats_tlv: display htt_tx_sounding_stats
 * @tag_buf: buffer containing the tlv htt_tx_sounding_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_sounding_stats_tlv(A_UINT32 *tag_buf)
{
    A_INT32 i, j = 0;
    A_UINT32 tag_len = 0;
    A_UINT16 index = 0;
    A_CHAR str_buf[HTT_MAX_STRING_LEN] = {0};
    htt_tx_sounding_stats_tlv *htt_stats_buf =
        (htt_tx_sounding_stats_tlv *)tag_buf;

    if (htt_stats_buf->tx_sounding_mode == HTT_TX_AC_SOUNDING_MODE) {
        HTT_STATS_PRINT(FATAL, "\nHTT_TX_AC_SOUNDING_STATS_TLV: \n");
        HTT_STATS_PRINT(FATAL, "ac_cbf_20 = IBF : %u, SU_SIFS : %u, SU_RBO : %u, MU_SIFS : %u, MU_RBO : %u ",
                htt_stats_buf->cbf_20[HTT_IMPLICIT_TXBF_STEER_STATS],
                htt_stats_buf->cbf_20[HTT_EXPLICIT_TXBF_SU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_20[HTT_EXPLICIT_TXBF_SU_RBO_STEER_STATS],
                htt_stats_buf->cbf_20[HTT_EXPLICIT_TXBF_MU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_20[HTT_EXPLICIT_TXBF_MU_RBO_STEER_STATS]);
        HTT_STATS_PRINT(FATAL, "ac_cbf_40 = IBF : %u, SU_SIFS : %u, "
                               "SU_RBO : %u, MU_SIFS : %u, MU_RBO : %u",
                htt_stats_buf->cbf_40[HTT_IMPLICIT_TXBF_STEER_STATS],
                htt_stats_buf->cbf_40[HTT_EXPLICIT_TXBF_SU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_40[HTT_EXPLICIT_TXBF_SU_RBO_STEER_STATS],
                htt_stats_buf->cbf_40[HTT_EXPLICIT_TXBF_MU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_40[HTT_EXPLICIT_TXBF_MU_RBO_STEER_STATS]);
        HTT_STATS_PRINT(FATAL, "ac_cbf_80 = IBF : %u, SU_SIFS : %u, "
                               "SU_RBO : %u, MU_SIFS : %u, MU_RBO : %u",
                htt_stats_buf->cbf_80[HTT_IMPLICIT_TXBF_STEER_STATS],
                htt_stats_buf->cbf_80[HTT_EXPLICIT_TXBF_SU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_80[HTT_EXPLICIT_TXBF_SU_RBO_STEER_STATS],
                htt_stats_buf->cbf_80[HTT_EXPLICIT_TXBF_MU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_80[HTT_EXPLICIT_TXBF_MU_RBO_STEER_STATS]);
        HTT_STATS_PRINT(FATAL, "ac_cbf_160 = IBF : %u, SU_SIFS : %u, "
                               "SU_RBO : %u, MU_SIFS : %u, MU_RBO : %u",
                htt_stats_buf->cbf_160[HTT_IMPLICIT_TXBF_STEER_STATS],
                htt_stats_buf->cbf_160[HTT_EXPLICIT_TXBF_SU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_160[HTT_EXPLICIT_TXBF_SU_RBO_STEER_STATS],
                htt_stats_buf->cbf_160[HTT_EXPLICIT_TXBF_MU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_160[HTT_EXPLICIT_TXBF_MU_RBO_STEER_STATS]);

        for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AC_MUMIMO_USER_STATS; i++) {
            HTT_STATS_PRINT(FATAL, "Sounding User %u = 20MHz: %u, 40MHz : %u, 80MHz: %u, 160MHz: %u ", i,
                    htt_stats_buf->sounding[j+0], htt_stats_buf->sounding[j+1],
                    htt_stats_buf->sounding[j+2], htt_stats_buf->sounding[j+3]);
            j += 4;
        }
    } else if (htt_stats_buf->tx_sounding_mode == HTT_TX_AX_SOUNDING_MODE) {
        HTT_STATS_PRINT(FATAL, "\nHTT_TX_AX_SOUNDING_STATS_TLV:\n");
        HTT_STATS_PRINT(FATAL, "ax_cbf_20 = IBF : %u, SU_SIFS : %u, "
                               "SU_RBO : %u, MU_SIFS : %u, MU_RBO : %u ",
                htt_stats_buf->cbf_20[HTT_IMPLICIT_TXBF_STEER_STATS],
                htt_stats_buf->cbf_20[HTT_EXPLICIT_TXBF_SU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_20[HTT_EXPLICIT_TXBF_SU_RBO_STEER_STATS],
                htt_stats_buf->cbf_20[HTT_EXPLICIT_TXBF_MU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_20[HTT_EXPLICIT_TXBF_MU_RBO_STEER_STATS]);
        HTT_STATS_PRINT(FATAL, "ax_cbf_40 = IBF : %u, SU_SIFS : %u, "
                               "SU_RBO : %u, MU_SIFS : %u, MU_RBO : %u",
                htt_stats_buf->cbf_40[HTT_IMPLICIT_TXBF_STEER_STATS],
                htt_stats_buf->cbf_40[HTT_EXPLICIT_TXBF_SU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_40[HTT_EXPLICIT_TXBF_SU_RBO_STEER_STATS],
                htt_stats_buf->cbf_40[HTT_EXPLICIT_TXBF_MU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_40[HTT_EXPLICIT_TXBF_MU_RBO_STEER_STATS]);
        HTT_STATS_PRINT(FATAL, "ax_cbf_80 = IBF : %u, SU_SIFS : %u, "
                               "SU_RBO : %u, MU_SIFS : %u, MU_RBO : %u",
                htt_stats_buf->cbf_80[HTT_IMPLICIT_TXBF_STEER_STATS],
                htt_stats_buf->cbf_80[HTT_EXPLICIT_TXBF_SU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_80[HTT_EXPLICIT_TXBF_SU_RBO_STEER_STATS],
                htt_stats_buf->cbf_80[HTT_EXPLICIT_TXBF_MU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_80[HTT_EXPLICIT_TXBF_MU_RBO_STEER_STATS]);
        HTT_STATS_PRINT(FATAL, "ax_cbf_160 = IBF : %u, SU_SIFS : %u, "
                               "SU_RBO : %u, MU_SIFS : %u, MU_RBO : %u",
                htt_stats_buf->cbf_160[HTT_IMPLICIT_TXBF_STEER_STATS],
                htt_stats_buf->cbf_160[HTT_EXPLICIT_TXBF_SU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_160[HTT_EXPLICIT_TXBF_SU_RBO_STEER_STATS],
                htt_stats_buf->cbf_160[HTT_EXPLICIT_TXBF_MU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_160[HTT_EXPLICIT_TXBF_MU_RBO_STEER_STATS]);

        for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS; i++) {
            HTT_STATS_PRINT(FATAL, "Sounding User %u = 20MHz: %u, 40MHz : %u, 80MHz: %u, 160MHz: %u ", i,
                    htt_stats_buf->sounding[j+0], htt_stats_buf->sounding[j+1],
                    htt_stats_buf->sounding[j+2], htt_stats_buf->sounding[j+3]);
            j += 4;
        }
    } else if (htt_stats_buf->tx_sounding_mode == HTT_TX_BE_SOUNDING_MODE) {
        HTT_STATS_PRINT(FATAL, "\nHTT_TX_BE_SOUNDING_STATS_TLV:\n");
        HTT_STATS_PRINT(FATAL, "be_cbf_20 = IBF : %u, SU_SIFS : %u, "
                               "SU_RBO : %u, MU_SIFS : %u, MU_RBO : %u ",
                htt_stats_buf->cbf_20[HTT_IMPLICIT_TXBF_STEER_STATS],
                htt_stats_buf->cbf_20[HTT_EXPLICIT_TXBF_SU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_20[HTT_EXPLICIT_TXBF_SU_RBO_STEER_STATS],
                htt_stats_buf->cbf_20[HTT_EXPLICIT_TXBF_MU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_20[HTT_EXPLICIT_TXBF_MU_RBO_STEER_STATS]);
        HTT_STATS_PRINT(FATAL, "be_cbf_40 = IBF : %u, SU_SIFS : %u, "
                               "SU_RBO : %u, MU_SIFS : %u, MU_RBO : %u",
                htt_stats_buf->cbf_40[HTT_IMPLICIT_TXBF_STEER_STATS],
                htt_stats_buf->cbf_40[HTT_EXPLICIT_TXBF_SU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_40[HTT_EXPLICIT_TXBF_SU_RBO_STEER_STATS],
                htt_stats_buf->cbf_40[HTT_EXPLICIT_TXBF_MU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_40[HTT_EXPLICIT_TXBF_MU_RBO_STEER_STATS]);
        HTT_STATS_PRINT(FATAL, "be_cbf_80 = IBF : %u, SU_SIFS : %u, "
                               "SU_RBO : %u, MU_SIFS : %u, MU_RBO : %u",
                htt_stats_buf->cbf_80[HTT_IMPLICIT_TXBF_STEER_STATS],
                htt_stats_buf->cbf_80[HTT_EXPLICIT_TXBF_SU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_80[HTT_EXPLICIT_TXBF_SU_RBO_STEER_STATS],
                htt_stats_buf->cbf_80[HTT_EXPLICIT_TXBF_MU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_80[HTT_EXPLICIT_TXBF_MU_RBO_STEER_STATS]);
        HTT_STATS_PRINT(FATAL, "be_cbf_160 = IBF : %u, SU_SIFS : %u, "
                               "SU_RBO : %u, MU_SIFS : %u, MU_RBO : %u",
                htt_stats_buf->cbf_160[HTT_IMPLICIT_TXBF_STEER_STATS],
                htt_stats_buf->cbf_160[HTT_EXPLICIT_TXBF_SU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_160[HTT_EXPLICIT_TXBF_SU_RBO_STEER_STATS],
                htt_stats_buf->cbf_160[HTT_EXPLICIT_TXBF_MU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_160[HTT_EXPLICIT_TXBF_MU_RBO_STEER_STATS]);
        HTT_STATS_PRINT(FATAL, "be_cbf_320 = IBF : %u, SU_SIFS : %u, "
                               "SU_RBO : %u, MU_SIFS : %u, MU_RBO : %u",
                htt_stats_buf->cbf_320[HTT_IMPLICIT_TXBF_STEER_STATS],
                htt_stats_buf->cbf_320[HTT_EXPLICIT_TXBF_SU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_320[HTT_EXPLICIT_TXBF_SU_RBO_STEER_STATS],
                htt_stats_buf->cbf_320[HTT_EXPLICIT_TXBF_MU_SIFS_STEER_STATS],
                htt_stats_buf->cbf_320[HTT_EXPLICIT_TXBF_MU_RBO_STEER_STATS]);

        for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MUMIMO_USER_STATS; i++) {
            HTT_STATS_PRINT(FATAL, "Sounding User %u = 20MHz: %u, 40MHz : %u, 80MHz: %u, 160MHz: %u, 320MHz: %u", i,
                    htt_stats_buf->sounding[j+0], htt_stats_buf->sounding[j+1],
                    htt_stats_buf->sounding[j+2], htt_stats_buf->sounding[j+3],
                    /* 320MHz spundings stats are independent of other bws */
                    htt_stats_buf->sounding_320[i]
            );
            j += 4;
        }
    } else if (htt_stats_buf->tx_sounding_mode == HTT_TX_CMN_SOUNDING_MODE) {
        HTT_STATS_PRINT(FATAL, "\nCV UPLOAD HANDLER STATS:-");
        HTT_STATS_PRINT(FATAL, "cv_nc_mismatch_err         : %u ", htt_stats_buf->cv_nc_mismatch_err);
        HTT_STATS_PRINT(FATAL, "cv_fcs_err                 : %u ", htt_stats_buf->cv_fcs_err);
        HTT_STATS_PRINT(FATAL, "cv_frag_idx_mismatch       : %u ", htt_stats_buf->cv_frag_idx_mismatch);
        HTT_STATS_PRINT(FATAL, "cv_invalid_peer_id         : %u ", htt_stats_buf->cv_invalid_peer_id);
        HTT_STATS_PRINT(FATAL, "cv_no_txbf_setup           : %u ", htt_stats_buf->cv_no_txbf_setup);
        HTT_STATS_PRINT(FATAL, "cv_expiry_in_update        : %u ", htt_stats_buf->cv_expiry_in_update);
        HTT_STATS_PRINT(FATAL, "cv_pkt_bw_exceed           : %u ", htt_stats_buf->cv_pkt_bw_exceed);
        HTT_STATS_PRINT(FATAL, "cv_dma_not_done_err        : %u ", htt_stats_buf->cv_dma_not_done_err);
        HTT_STATS_PRINT(FATAL, "cv_update_failed           : %u ", htt_stats_buf->cv_update_failed);
        HTT_STATS_PRINT(FATAL, "cv_dma_timeout_error       : %u ", htt_stats_buf->cv_dma_timeout_error);
        HTT_STATS_PRINT(FATAL, "cv_buf_ibf_uploads         : %u ", htt_stats_buf->cv_buf_ibf_uploads);
        HTT_STATS_PRINT(FATAL, "cv_buf_ebf_uploads         : %u ", htt_stats_buf->cv_buf_ebf_uploads);
        HTT_STATS_PRINT(FATAL, "cv_buf_received            : %u ", htt_stats_buf->cv_buf_received);
        HTT_STATS_PRINT(FATAL, "cv_buf_fed_back            : %u ", htt_stats_buf->cv_buf_fed_back);

        HTT_STATS_PRINT(FATAL, "\nCV QUERY STATS:-");
        HTT_STATS_PRINT(FATAL, "cv_total_query             : %u ", htt_stats_buf->cv_total_query);
        HTT_STATS_PRINT(FATAL, "cv_total_pattern_query     : %u ", htt_stats_buf->cv_total_pattern_query);
        HTT_STATS_PRINT(FATAL, "cv_total_bw_query          : %u ", htt_stats_buf->cv_total_bw_query);
        HTT_STATS_PRINT(FATAL, "cv_invalid_bw_coding       : %u ", htt_stats_buf->cv_invalid_bw_coding);
        HTT_STATS_PRINT(FATAL, "cv_forced_sounding         : %u ", htt_stats_buf->cv_forced_sounding);
        HTT_STATS_PRINT(FATAL, "cv_standalone_sounding     : %u ", htt_stats_buf->cv_standalone_sounding);
        HTT_STATS_PRINT(FATAL, "cv_nc_mismatch             : %u ", htt_stats_buf->cv_nc_mismatch);
        HTT_STATS_PRINT(FATAL, "cv_fb_type_mismatch        : %u ", htt_stats_buf->cv_fb_type_mismatch);
        HTT_STATS_PRINT(FATAL, "cv_ofdma_bw_mismatch       : %u ", htt_stats_buf->cv_ofdma_bw_mismatch);
        HTT_STATS_PRINT(FATAL, "cv_bw_mismatch             : %u ", htt_stats_buf->cv_bw_mismatch);
        HTT_STATS_PRINT(FATAL, "cv_pattern_mismatch        : %u ", htt_stats_buf->cv_pattern_mismatch);
        HTT_STATS_PRINT(FATAL, "cv_preamble_mismatch       : %u ", htt_stats_buf->cv_preamble_mismatch);
        HTT_STATS_PRINT(FATAL, "cv_nr_mismatch             : %u ", htt_stats_buf->cv_nr_mismatch);
        HTT_STATS_PRINT(FATAL, "cv_in_use_cnt_exceeded     : %u ", htt_stats_buf->cv_in_use_cnt_exceeded);
        HTT_STATS_PRINT(FATAL, "cv_ntbr_sounding           : %u", htt_stats_buf->cv_ntbr_sounding);
        HTT_STATS_PRINT(FATAL, "cv_found_upload_in_progress: %u", htt_stats_buf->cv_found_upload_in_progress);
        HTT_STATS_PRINT(FATAL, "cv_expired_during_query    : %u", htt_stats_buf->cv_expired_during_query);
        HTT_STATS_PRINT(FATAL, "cv_found                   : %u ", htt_stats_buf->cv_found);
        HTT_STATS_PRINT(FATAL, "cv_not_found               : %u ", htt_stats_buf->cv_not_found);
        HTT_STATS_PRINT(FATAL, "cv_total_query_ibf         : %u ", htt_stats_buf->cv_total_query_ibf);
        HTT_STATS_PRINT(FATAL, "cv_found_ibf               : %u ", htt_stats_buf->cv_found_ibf);
        HTT_STATS_PRINT(FATAL, "cv_not_found_ibf           : %u ", htt_stats_buf->cv_not_found_ibf);
        HTT_STATS_PRINT(FATAL, "cv_expired_during_query_ibf: %u ", htt_stats_buf->cv_expired_during_query_ibf);
        HTT_STATS_PRINT(FATAL, "adaptive_snd_total_query   : %u ", htt_stats_buf->adaptive_snd_total_query);

        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        tag_len = (HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS +
            HTT_TX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS);

        for (i = 0; i < tag_len; i++) {
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->adaptive_snd_total_mcs_drop[i]);
        }
        HTT_STATS_PRINT(FATAL, "adaptive_snd_total_mcs_drop: %s ", str_buf);

        HTT_STATS_PRINT(FATAL, "adaptive_snd_kicked_in     : %u ", htt_stats_buf->adaptive_snd_kicked_in);
        HTT_STATS_PRINT(FATAL, "adaptive_snd_back_to_default: %u ", htt_stats_buf->adaptive_snd_back_to_default);
    } else if (htt_stats_buf->tx_sounding_mode == HTT_TX_CV_CORR_MODE) {
        HTT_STATS_PRINT(FATAL, "\nCV CORRELATION TRIGGER STATS:-");
        HTT_STATS_PRINT(FATAL, "cv_corr_trigger_online_mode         : %u", htt_stats_buf->cv_corr_trigger_online_mode);
        HTT_STATS_PRINT(FATAL, "cv_corr_trigger_offline_mode        : %u", htt_stats_buf->cv_corr_trigger_offline_mode);
        HTT_STATS_PRINT(FATAL, "cv_corr_trigger_hybrid_mode         : %u", htt_stats_buf->cv_corr_trigger_hybrid_mode);
        HTT_STATS_PRINT(FATAL, "cv_corr_trigger_computation_level_0 : %u", htt_stats_buf->cv_corr_trigger_computation_level_0);
        HTT_STATS_PRINT(FATAL, "cv_corr_trigger_computation_level_1 : %u", htt_stats_buf->cv_corr_trigger_computation_level_1);
        HTT_STATS_PRINT(FATAL, "cv_corr_trigger_computation_level_2 : %u", htt_stats_buf->cv_corr_trigger_computation_level_2);

        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (i = 0; i < HTT_TX_CV_CORR_MAX_NUM_COLUMNS; i++) {
            index += snprintf(&str_buf[index], (HTT_MAX_STRING_LEN - index),
                " %u_user: %u, ",
                (i + 1), htt_stats_buf->cv_corr_trigger_num_users[i]);
        }
        HTT_STATS_PRINT(FATAL, "cv_corr_trigger_num_users           : %s", str_buf);

        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (i = 0; i < HTT_TX_CV_CORR_MAX_NUM_COLUMNS; i++) {
            index += snprintf(&str_buf[index], (HTT_MAX_STRING_LEN - index),
                " %u_stream: %u, ",
                (i + 1), htt_stats_buf->cv_corr_trigger_num_streams[i]);
        }
        HTT_STATS_PRINT(FATAL, "cv_corr_trigger_num_streams         : %s", str_buf);

        HTT_STATS_PRINT(FATAL, "\nCV CORRELATION UPLOAD STATS:-");
        HTT_STATS_PRINT(FATAL, "cv_corr_upload_total_buf_received          : %u", htt_stats_buf->cv_corr_upload_total_buf_received);
        HTT_STATS_PRINT(FATAL, "cv_corr_upload_total_buf_fed_back          : %u", htt_stats_buf->cv_corr_upload_total_buf_fed_back);
        HTT_STATS_PRINT(FATAL, "cv_corr_upload_total_processing_failed     : %u", htt_stats_buf->cv_corr_upload_total_processing_failed);
        HTT_STATS_PRINT(FATAL, "cv_corr_upload_failed_total_users_zero     : %u", htt_stats_buf->cv_corr_upload_failed_total_users_zero);
        HTT_STATS_PRINT(FATAL, "cv_corr_upload_failed_total_users_exceeded : %u", htt_stats_buf->cv_corr_upload_failed_total_users_exceeded);
        HTT_STATS_PRINT(FATAL, "cv_corr_upload_failed_peer_not_found       : %u", htt_stats_buf->cv_corr_upload_failed_peer_not_found);
        HTT_STATS_PRINT(FATAL, "cv_corr_upload_user_nss_exceeded           : %u", htt_stats_buf->cv_corr_upload_user_nss_exceeded);
        HTT_STATS_PRINT(FATAL, "cv_corr_upload_invalid_lookup_index        : %u", htt_stats_buf->cv_corr_upload_invalid_lookup_index);

        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (i = 0; i < HTT_TX_CV_CORR_MAX_NUM_COLUMNS; i++) {
            index += snprintf(&str_buf[index], (HTT_MAX_STRING_LEN - index),
                " %u_user: %u, ",
                (i + 1), htt_stats_buf->cv_corr_upload_total_num_users[i]);
        }
        HTT_STATS_PRINT(FATAL, "cv_corr_upload_total_num_users             : %s", str_buf);

        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (i = 0; i < HTT_TX_CV_CORR_MAX_NUM_COLUMNS; i++) {
            index += snprintf(&str_buf[index], (HTT_MAX_STRING_LEN - index),
                " %u_stream: %u, ",
                (i + 1), htt_stats_buf->cv_corr_upload_total_num_streams[i]);
        }
        HTT_STATS_PRINT(FATAL, "cv_corr_upload_total_num_streams           : %s", str_buf);
    }
}

/*
 * htt_print_tx_selfgen_cmn_stats_tlv: display htt_tx_selfgen_cmn_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_selfgen_cmn_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_selfgen_cmn_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_selfgen_cmn_stats_tlv *htt_stats_buf =
        (htt_tx_selfgen_cmn_stats_tlv *)tag_buf;
    int i;
    int index;
    A_CHAR str_buf[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_TX_SELFGEN_CMN_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__word & 0xFF);
    /*HTT_STATS_PRINT(FATAL, "word = %u",
            ((htt_stats_buf->mac_id__word & 0xFFFFFF00) >> 8 ));*/
    HTT_STATS_PRINT(FATAL, "su_bar = %u",
            htt_stats_buf->su_bar);
    HTT_STATS_PRINT(FATAL, "rts = %u",
            htt_stats_buf->rts);
    HTT_STATS_PRINT(FATAL, "cts2self = %u",
            htt_stats_buf->cts2self);
    HTT_STATS_PRINT(FATAL, "qos_null = %u",
            htt_stats_buf->qos_null);
    HTT_STATS_PRINT(FATAL, "delayed_bar_1 = %u",
            htt_stats_buf->delayed_bar_1);
    HTT_STATS_PRINT(FATAL, "delayed_bar_2 = %u",
            htt_stats_buf->delayed_bar_2);
    HTT_STATS_PRINT(FATAL, "delayed_bar_3 = %u",
            htt_stats_buf->delayed_bar_3);
    HTT_STATS_PRINT(FATAL, "delayed_bar_4 = %u",
            htt_stats_buf->delayed_bar_4);
    HTT_STATS_PRINT(FATAL, "delayed_bar_5 = %u",
            htt_stats_buf->delayed_bar_5);
    HTT_STATS_PRINT(FATAL, "delayed_bar_6 = %u",
            htt_stats_buf->delayed_bar_6);
    HTT_STATS_PRINT(FATAL, "delayed_bar_7 = %u",
            htt_stats_buf->delayed_bar_7);
    HTT_STATS_PRINT(FATAL, "bar_with_tqm_head_seq_num = %u",
            htt_stats_buf->bar_with_tqm_head_seq_num);
    HTT_STATS_PRINT(FATAL, "bar_with_tid_seq_num = %u",
            htt_stats_buf->bar_with_tid_seq_num);
    HTT_STATS_PRINT(FATAL, "su_sw_rts_queued = %u",
            htt_stats_buf->su_sw_rts_queued);
    HTT_STATS_PRINT(FATAL, "su_sw_rts_tried = %u",
            htt_stats_buf->su_sw_rts_tried);
    HTT_STATS_PRINT(FATAL, "su_sw_rts_err = %u",
            htt_stats_buf->su_sw_rts_err);
    HTT_STATS_PRINT(FATAL, "su_sw_rts_flushed = %u",
            htt_stats_buf->su_sw_rts_flushed);
    HTT_STATS_PRINT(FATAL, "su_sw_rts_rcvd_cts_diff_bw = %u",
            htt_stats_buf->su_sw_rts_rcvd_cts_diff_bw);

    index = 0;
    for (i = 0; i < HTT_MAX_NUM_SBT_INTR; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->smart_basic_trig_sch_histogram[i]);
    }
    HTT_STATS_PRINT(FATAL, "smart_basic_trig_sch_histogram = %s\n", str_buf);
}

/*
 * htt_print_tx_selfgen_ac_stats_tlv: display htt_tx_selfgen_ac_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_selfgen_ac_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_selfgen_ac_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_selfgen_ac_stats_tlv *htt_stats_buf =
        (htt_tx_selfgen_ac_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_SELFGEN_AC_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "ac_su_ndpa_queued = %u",
            htt_stats_buf->ac_su_ndpa_queued);
    HTT_STATS_PRINT(FATAL, "ac_su_ndpa_tried = %u",
                htt_stats_buf->ac_su_ndpa);
    HTT_STATS_PRINT(FATAL, "ac_su_ndp_queued = %u",
            htt_stats_buf->ac_su_ndp_queued);
    HTT_STATS_PRINT(FATAL, "ac_su_ndp_tried = %u",
                htt_stats_buf->ac_su_ndp);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_ndpa_queued = %u",
            htt_stats_buf->ac_mu_mimo_ndpa_queued);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_ndpa_tried = %u",
                htt_stats_buf->ac_mu_mimo_ndpa);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_ndp_queued = %u",
            htt_stats_buf->ac_mu_mimo_ndp_queued);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_ndp_tried = %u",
                htt_stats_buf->ac_mu_mimo_ndp);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brpoll1_queued = %u",
            htt_stats_buf->ac_mu_mimo_brpoll_1_queued);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brpoll1_tried = %u",
                htt_stats_buf->ac_mu_mimo_brpoll_1);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brpoll2_queued = %u",
            htt_stats_buf->ac_mu_mimo_brpoll_2_queued);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brpoll2_tried = %u",
                htt_stats_buf->ac_mu_mimo_brpoll_2);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brpoll3_queued = %u",
            htt_stats_buf->ac_mu_mimo_brpoll_3_queued);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brpoll3_tried = %u\n",
                htt_stats_buf->ac_mu_mimo_brpoll_3);
}

/* DEPRECATED */
/*
 * htt_print_txbf_ofdma_ndpa_stats_tlv: display htt_txbf_ofdma_ndpa_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_ndpa_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_ndpa_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_ndpa_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_ndpa_stats_tlv *)tag_buf;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_NDPA_STATS_TLV:");

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_ndpa_queued, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_ndpa_queued = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_ndpa_tried, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_ndpa_tried = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_ndpa_flushed, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_ndpa_flushed = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_ndpa_err, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_ndpa_err = %s\n", str_buf);
}

/* DEPRECATED */
/*
 * htt_print_txbf_ofdma_ndp_stats_tlv: display htt_txbf_ofdma_ndp_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_ndp_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_ndp_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_ndp_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_ndp_stats_tlv *)tag_buf;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_NDP_STATS_TLV:");

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_ndp_queued, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_ndp_queued = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_ndp_tried, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_ndp_tried = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_ndp_flushed, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_ndp_flushed = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_ndp_err, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_ndp_err = %s\n", str_buf);
}

/* DEPRECATED */
/*
 * htt_print_txbf_ofdma_brp_stats_tlv: display htt_txbf_ofdma_brp_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_brp_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_brp_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_brp_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_brp_stats_tlv *)tag_buf;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_BRP_STATS_TLV:");

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_brpoll_queued, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_brpoll_queued = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_brpoll_tried, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_brpoll_tried = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_brpoll_flushed, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_brpoll_flushed = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_brp_err, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_brp_err = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_brp_err_num_cbf_rcvd, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS + 1, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_brp_err_num_cbf_rcvd = %s\n", str_buf);
}

/* DEPRECATED */
/*
 * htt_print_txbf_ofdma_steer_stats_tlv: display htt_txbf_ofdma_steer_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_brp_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_steer_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_steer_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_steer_stats_tlv *)tag_buf;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_STEER_STATS_TLV:");

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_num_ppdu_steer, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_num_ppdu_steer = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_num_ppdu_ol, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_num_ppdu_ol = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_num_usrs_prefetch, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_num_usrs_prefetch = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_num_usrs_sound, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_num_usrs_sound = %s", str_buf);

    htt_stringify_array(str_buf, HTT_MAX_STRING_LEN, htt_stats_buf->ax_ofdma_num_usrs_force_sound, HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS, 1, 1);
    HTT_STATS_PRINT(FATAL, "ax_ofdma_num_usrs_force_sound = %s\n", str_buf);
}

/*
 * htt_print_txbf_ofdma_ax_ndpa_stats_tlv: display htt_txbf_ofdma_ax_ndpa_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_ax_ndpa_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_ax_ndpa_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_ax_ndpa_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_ax_ndpa_stats_tlv *)tag_buf;

    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_UINT8  i;
    A_UINT16 index;
    A_UINT8  null_output;

    A_UINT32 num_elements = htt_stats_buf->num_elems_ax_ndpa_arr;

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_AX_NDPA_STATS_TLV:");

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_ndpa[i].ax_ofdma_ndpa_queued) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_ndpa[i].ax_ofdma_ndpa_queued);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndpa_queued = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndpa_queued = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_ndpa[i].ax_ofdma_ndpa_tried) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_ndpa[i].ax_ofdma_ndpa_tried);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndpa_tried = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndpa_tried = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_ndpa[i].ax_ofdma_ndpa_flushed) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_ndpa[i].ax_ofdma_ndpa_flushed);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndpa_flushed = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndpa_flushed = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_ndpa[i].ax_ofdma_ndpa_err) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_ndpa[i].ax_ofdma_ndpa_err);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndpa_err = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndpa_err = %s", str_buf);
    }
}

/*
 * htt_print_txbf_ofdma_ax_ndp_stats_tlv: display htt_txbf_ofdma_ax_ndp_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_ax_ndp_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_ax_ndp_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_ax_ndp_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_ax_ndp_stats_tlv *)tag_buf;

    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_UINT8  i;
    A_UINT16 index;
    A_UINT8  null_output;

    A_UINT32 num_elements = htt_stats_buf->num_elems_ax_ndp_arr;

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_AX_NDP_STATS_TLV:");

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_ndp[i].ax_ofdma_ndp_queued) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_ndp[i].ax_ofdma_ndp_queued);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndp_queued = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndp_queued = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_ndp[i].ax_ofdma_ndp_tried) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_ndp[i].ax_ofdma_ndp_tried);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndp_tried = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndp_tried = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_ndp[i].ax_ofdma_ndp_flushed) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_ndp[i].ax_ofdma_ndp_flushed);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndp_flushed = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndp_flushed = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_ndp[i].ax_ofdma_ndp_err) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_ndp[i].ax_ofdma_ndp_err);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndp_err = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_ndp_err = %s", str_buf);
    }
}

/*
 * htt_print_txbf_ofdma_ax_brp_stats_tlv: display htt_txbf_ofdma_ax_brp_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_ax_brp_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_ax_brp_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_ax_brp_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_ax_brp_stats_tlv *)tag_buf;

    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_UINT8  i;
    A_UINT16 index;
    A_UINT8  null_output;

    A_UINT32 num_elements = htt_stats_buf->num_elems_ax_brp_arr;

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_AX_BRP_STATS_TLV:");

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_brp[i].ax_ofdma_brpoll_queued) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_brp[i].ax_ofdma_brpoll_queued);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_brpoll_queued = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_brpoll_queued = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_brp[i].ax_ofdma_brpoll_tried) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_brp[i].ax_ofdma_brpoll_tried);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_brpoll_tried = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_brpoll_tried = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_brp[i].ax_ofdma_brpoll_flushed) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_brp[i].ax_ofdma_brpoll_flushed);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_brpoll_flushed = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_brpoll_flushed = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_brp[i].ax_ofdma_brp_err) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_brp[i].ax_ofdma_brp_err);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_brp_err = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_brp_err = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_brp[i].ax_ofdma_brp_err_num_cbf_rcvd) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_brp[i].ax_ofdma_brp_err_num_cbf_rcvd);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_brp_err_num_cbf_rcvd = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_brp_err_num_cbf_rcvd = %s", str_buf);
    }
}

/*
 * htt_print_txbf_ofdma_ax_steer_stats_tlv: display htt_txbf_ofdma_ax_steer_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_ax_steer_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_ax_steer_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_ax_steer_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_ax_steer_stats_tlv *)tag_buf;

    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_UINT8  i;
    A_UINT16 index;
    A_UINT8  null_output;

    A_UINT32 num_elements = htt_stats_buf->num_elems_ax_steer_arr;

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_AX_STEER_STATS_TLV:");

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_steer[i].ax_ofdma_num_ppdu_steer) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_steer[i].ax_ofdma_num_ppdu_steer);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_num_ppdu_steer = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_num_ppdu_steer = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_steer[i].ax_ofdma_num_ppdu_ol) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_steer[i].ax_ofdma_num_ppdu_ol);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_num_ppdu_ol = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_num_ppdu_ol = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_steer[i].ax_ofdma_num_usrs_prefetch) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_steer[i].ax_ofdma_num_usrs_prefetch);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_num_usrs_prefetch = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_num_usrs_prefetch = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_steer[i].ax_ofdma_num_usrs_sound) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_steer[i].ax_ofdma_num_usrs_sound);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_num_usrs_sound = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_num_usrs_sound = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->ax_steer[i].ax_ofdma_num_usrs_force_sound) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_steer[i].ax_ofdma_num_usrs_force_sound);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_num_usrs_force_sound = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_num_usrs_force_sound = %s", str_buf);
    }
}

/*
 * htt_print_txbf_ofdma_ax_steer_mpdu_stats_tlv:
 *     display htt_txbf_ofdma_ax_steer_mpdu_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_ax_steer_mpdu_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_ax_steer_mpdu_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_ax_steer_mpdu_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_ax_steer_mpdu_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_AX_STEER_MPDU_STATS_TLV:");

    HTT_STATS_PRINT(FATAL, "rbo_steer_mpdus_tried = %u",
        htt_stats_buf->ax_ofdma_rbo_steer_mpdus_tried);
    HTT_STATS_PRINT(FATAL, "rbo_steer_mpdus_failed = %u",
        htt_stats_buf->ax_ofdma_rbo_steer_mpdus_failed);
    HTT_STATS_PRINT(FATAL, "sifs_steer_mpdus_tried = %u",
        htt_stats_buf->ax_ofdma_sifs_steer_mpdus_tried);
    HTT_STATS_PRINT(FATAL, "sifs_steer_mpdus_failed = %u",
        htt_stats_buf->ax_ofdma_sifs_steer_mpdus_failed);
}

/*
 * htt_print_txbf_ofdma_be_ndpa_stats_tlv: display htt_txbf_ofdma_be_ndpa_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_be_ndpa_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_be_ndpa_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_be_ndpa_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_be_ndpa_stats_tlv *)tag_buf;

    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_UINT8  i;
    A_UINT16 index;
    A_UINT8  null_output;

    A_UINT32 num_elements = htt_stats_buf->num_elems_be_ndpa_arr;

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_BE_NDPA_STATS_TLV:");

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_ndpa[i].be_ofdma_ndpa_queued) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_ndpa[i].be_ofdma_ndpa_queued);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndpa_queued = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndpa_queued = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_ndpa[i].be_ofdma_ndpa_tried) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_ndpa[i].be_ofdma_ndpa_tried);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndpa_tried = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndpa_tried = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_ndpa[i].be_ofdma_ndpa_flushed) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_ndpa[i].be_ofdma_ndpa_flushed);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndpa_flushed = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndpa_flushed = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_ndpa[i].be_ofdma_ndpa_err) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_ndpa[i].be_ofdma_ndpa_err);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndpa_err = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndpa_err = %s", str_buf);
    }
}

/*
 * htt_print_txbf_ofdma_be_ndp_stats_tlv: display htt_txbf_ofdma_be_ndp_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_be_ndp_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_be_ndp_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_be_ndp_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_be_ndp_stats_tlv *)tag_buf;

    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_UINT8  i;
    A_UINT16 index;
    A_UINT8  null_output;

    A_UINT32 num_elements = htt_stats_buf->num_elems_be_ndp_arr;

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_BE_NDP_STATS_TLV:");

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_ndp[i].be_ofdma_ndp_queued) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_ndp[i].be_ofdma_ndp_queued);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndp_queued = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndp_queued = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_ndp[i].be_ofdma_ndp_tried) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_ndp[i].be_ofdma_ndp_tried);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndp_tried = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndp_tried = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_ndp[i].be_ofdma_ndp_flushed) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_ndp[i].be_ofdma_ndp_flushed);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndp_flushed = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndp_flushed = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_ndp[i].be_ofdma_ndp_err) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_ndp[i].be_ofdma_ndp_err);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndp_err = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_ndp_err = %s", str_buf);
    }
}

/*
 * htt_print_txbf_ofdma_be_brp_stats_tlv: display htt_txbf_ofdma_be_brp_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_be_brp_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_be_brp_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_be_brp_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_be_brp_stats_tlv *)tag_buf;

    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_UINT8  i;
    A_UINT16 index;
    A_UINT8  null_output;

    A_UINT32 num_elements = htt_stats_buf->num_elems_be_brp_arr;

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_BE_BRP_STATS_TLV:");

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_brp[i].be_ofdma_brpoll_queued) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_brp[i].be_ofdma_brpoll_queued);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_brpoll_queued = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_brpoll_queued = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_brp[i].be_ofdma_brpoll_tried) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_brp[i].be_ofdma_brpoll_tried);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_brpoll_tried = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_brpoll_tried = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_brp[i].be_ofdma_brpoll_flushed) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_brp[i].be_ofdma_brpoll_flushed);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_brpoll_flushed = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_brpoll_flushed = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_brp[i].be_ofdma_brp_err) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_brp[i].be_ofdma_brp_err);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_brp_err = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_brp_err = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_brp[i].be_ofdma_brp_err_num_cbf_rcvd) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_brp[i].be_ofdma_brp_err_num_cbf_rcvd);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_brp_err_num_cbf_rcvd = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_brp_err_num_cbf_rcvd = %s", str_buf);
    }
}

/*
 * htt_print_txbf_ofdma_be_steer_stats_tlv: display htt_txbf_ofdma_be_steer_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_be_steer_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_be_steer_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_be_steer_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_be_steer_stats_tlv *)tag_buf;

    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_UINT8  i;
    A_UINT16 index;
    A_UINT8  null_output;

    A_UINT32 num_elements = htt_stats_buf->num_elems_be_steer_arr;

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_BE_STEER_STATS_TLV:");

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_steer[i].be_ofdma_num_ppdu_steer) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_steer[i].be_ofdma_num_ppdu_steer);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_num_ppdu_steer = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_num_ppdu_steer = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_steer[i].be_ofdma_num_ppdu_ol) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_steer[i].be_ofdma_num_ppdu_ol);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_num_ppdu_ol = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_num_ppdu_ol = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_steer[i].be_ofdma_num_usrs_prefetch) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_steer[i].be_ofdma_num_usrs_prefetch);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_num_usrs_prefetch = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_num_usrs_prefetch = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_steer[i].be_ofdma_num_usrs_sound) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_steer[i].be_ofdma_num_usrs_sound);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_num_usrs_sound = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_num_usrs_sound = %s", str_buf);
    }

    index = 0;
    null_output = 1;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < num_elements; i++) {
        if (htt_stats_buf->be_steer[i].be_ofdma_num_usrs_force_sound) {
            null_output = 0;
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_steer[i].be_ofdma_num_usrs_force_sound);
        }
    }
    if (null_output) {
        HTT_STATS_PRINT(FATAL, "be_ofdma_num_usrs_force_sound = %s", "NONE");
    } else {
        HTT_STATS_PRINT(FATAL, "be_ofdma_num_usrs_force_sound = %s", str_buf);
    }
}

/*
 * htt_print_txbf_ofdma_be_steer_mpdu_stats_tlv:
 *     display htt_txbf_ofdma_be_steer_mpdu_stats_tlv
 * @tag_buf: buffer containing the tlv htt_txbf_ofdma_be_steer_mpdu_stats_tlv
 *
 * return:void
 */
static void htt_print_txbf_ofdma_be_steer_mpdu_stats_tlv(A_UINT32 *tag_buf)
{
    htt_txbf_ofdma_be_steer_mpdu_stats_tlv *htt_stats_buf =
        (htt_txbf_ofdma_be_steer_mpdu_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TXBF_OFDMA_BE_STEER_MPDU_STATS_TLV:");

    HTT_STATS_PRINT(FATAL, "rbo_steer_mpdus_tried = %u",
        htt_stats_buf->be_ofdma_rbo_steer_mpdus_tried);
    HTT_STATS_PRINT(FATAL, "rbo_steer_mpdus_failed = %u",
        htt_stats_buf->be_ofdma_rbo_steer_mpdus_failed);
    HTT_STATS_PRINT(FATAL, "sifs_steer_mpdus_tried = %u",
        htt_stats_buf->be_ofdma_sifs_steer_mpdus_tried);
    HTT_STATS_PRINT(FATAL, "sifs_steer_mpdus_failed = %u",
        htt_stats_buf->be_ofdma_sifs_steer_mpdus_failed);
}

static void htt_print_odd_pdev_mandatory_tlv(A_UINT32 *tag_buf)
{
    htt_odd_mandatory_pdev_stats_tlv *htt_stats_buf =
        (htt_odd_mandatory_pdev_stats_tlv *) tag_buf;
    A_UINT8  i;
    A_UINT16 index = 0;
    A_UINT32 tag_len  = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_ODD_PDEV_MANDATORY_TLV:");

    HTT_STATS_PRINT(FATAL, "hw_queued = %u",
            htt_stats_buf->hw_queued);
    HTT_STATS_PRINT(FATAL, "hw_reaped = %u",
            htt_stats_buf->hw_reaped);
    HTT_STATS_PRINT(FATAL, "hw_paused = %u",
            htt_stats_buf->hw_paused);
    HTT_STATS_PRINT(FATAL, "hw_filt = %u",
            htt_stats_buf->hw_filt);
    HTT_STATS_PRINT(FATAL, "seq_posted = %u",
            htt_stats_buf->seq_posted);
    HTT_STATS_PRINT(FATAL, "seq_completed = %u",
            htt_stats_buf->seq_completed);
    HTT_STATS_PRINT(FATAL, "underrun = %u",
            htt_stats_buf->underrun);
    HTT_STATS_PRINT(FATAL, "hw_flush = %u",
            htt_stats_buf->hw_flush);
    HTT_STATS_PRINT(FATAL, "next_seq_posted_dsr = %u",
            htt_stats_buf->next_seq_posted_dsr);
    HTT_STATS_PRINT(FATAL, "seq_posted_isr = %u",
            htt_stats_buf->seq_posted_isr);
    HTT_STATS_PRINT(FATAL, "mpdu_cnt_fcs_ok = %u",
            htt_stats_buf->mpdu_cnt_fcs_ok);
    HTT_STATS_PRINT(FATAL, "mpdu_cnt_fcs_err = %u",
            htt_stats_buf->mpdu_cnt_fcs_err);
    HTT_STATS_PRINT(FATAL, "msdu_count_tqm = %u",
            htt_stats_buf->msdu_count_tqm);
    HTT_STATS_PRINT(FATAL, "mpdu_count_tqm = %u",
            htt_stats_buf->mpdu_count_tqm);
    HTT_STATS_PRINT(FATAL, "mpdus_ack_failed = %u",
            htt_stats_buf->mpdus_ack_failed);
    HTT_STATS_PRINT(FATAL, "num_data_ppdus_tried_ota = %u",
            htt_stats_buf->num_data_ppdus_tried_ota);
    HTT_STATS_PRINT(FATAL, "ppdu_ok = %u",
            htt_stats_buf->ppdu_ok);
    HTT_STATS_PRINT(FATAL, "num_total_ppdus_tried_ota = %u",
            htt_stats_buf->num_total_ppdus_tried_ota);
    HTT_STATS_PRINT(FATAL, "thermal_suspend_cnt = %u",
            htt_stats_buf->thermal_suspend_cnt);
    HTT_STATS_PRINT(FATAL, "dfs_suspend_cnt = %u",
            htt_stats_buf->dfs_suspend_cnt);
    HTT_STATS_PRINT(FATAL, "tx_abort_suspend_cnt = %u",
            htt_stats_buf->tx_abort_suspend_cnt);
    HTT_STATS_PRINT(FATAL, "suspended_txq_mask = %u",
            htt_stats_buf->suspended_txq_mask);
    HTT_STATS_PRINT(FATAL, "last_suspend_reason = %u",
            htt_stats_buf->last_suspend_reason);
    HTT_STATS_PRINT(FATAL, "seq_failed_queueing = %u",
            htt_stats_buf->seq_failed_queueing);
    HTT_STATS_PRINT(FATAL, "seq_restarted = %u",
            htt_stats_buf->seq_restarted);
    HTT_STATS_PRINT(FATAL, "seq_txop_repost_stop = %u",
            htt_stats_buf->seq_txop_repost_stop);
    HTT_STATS_PRINT(FATAL, "next_seq_cancel = %u",
            htt_stats_buf->next_seq_cancel);
    HTT_STATS_PRINT(FATAL, "seq_min_msdu_repost_stop = %u",
            htt_stats_buf->seq_min_msdu_repost_stop);
    HTT_STATS_PRINT(FATAL, "total_phy_err_cnt = %u",
            htt_stats_buf->total_phy_err_cnt);
    HTT_STATS_PRINT(FATAL, "ppdu_recvd = %u",
            htt_stats_buf->ppdu_recvd);
    HTT_STATS_PRINT(FATAL, "tcp_msdu_cnt = %u",
            htt_stats_buf->tcp_msdu_cnt);
    HTT_STATS_PRINT(FATAL, "tcp_ack_msdu_cnt = %u",
            htt_stats_buf->tcp_ack_msdu_cnt);
    HTT_STATS_PRINT(FATAL, "udp_msdu_cnt = %u \n",
            htt_stats_buf->udp_msdu_cnt);

    for (i = 0; i < HTT_STATS_SUBTYPE_MAX; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->fw_tx_mgmt_subtype[i]);
    }
    HTT_STATS_PRINT(FATAL, "fw_tx_mgmt_subtype = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_STATS_SUBTYPE_MAX; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->fw_rx_mgmt_subtype[i]);
    }
    HTT_STATS_PRINT(FATAL, "fw_rx_mgmt_subtype = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_STATS_RXDMA_MAX_ERR; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->fw_ring_mpdu_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "fw_ring_mpdu_err = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_MAX_URRN_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->urrn_stats[i]);
    }
    HTT_STATS_PRINT(FATAL, "urrn_stats = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_MAX_SIFS_BURST_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->sifs_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "sifs_status = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_SIFS_BURST_HIST_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i + 1, htt_stats_buf->sifs_hist_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "sifs_hist_status = %s \n", str_buf);

    HTT_STATS_PRINT(FATAL, "rx_suspend_cnt = %u",
            htt_stats_buf->rx_suspend_cnt);
    HTT_STATS_PRINT(FATAL, "rx_suspend_fail_cnt = %u",
            htt_stats_buf->rx_suspend_fail_cnt);
    HTT_STATS_PRINT(FATAL, "rx_resume_cnt = %u",
            htt_stats_buf->rx_resume_cnt);
    HTT_STATS_PRINT(FATAL, "rx_resume_fail_cnt = %u \n",
            htt_stats_buf->rx_resume_fail_cnt);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_HWQ_MAX_CMD_RESULT_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i + 1, htt_stats_buf->hwq_beacon_cmd_result[i]);
    }
    HTT_STATS_PRINT(FATAL, "hwq_beacon_cmd_result = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_HWQ_MAX_CMD_RESULT_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i + 1, htt_stats_buf->hwq_voice_cmd_result[i]);
    }
    HTT_STATS_PRINT(FATAL, "hwq_voice_cmd_result = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_HWQ_MAX_CMD_RESULT_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i + 1, htt_stats_buf->hwq_video_cmd_result[i]);
    }
    HTT_STATS_PRINT(FATAL, "hwq_video_cmd_result = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_HWQ_MAX_CMD_RESULT_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i + 1, htt_stats_buf->hwq_best_effort_cmd_result[i]);
    }
    HTT_STATS_PRINT(FATAL, "hwq_best_effort_cmd_result = %s \n", str_buf);

    HTT_STATS_PRINT(FATAL, "hwq_beacon_mpdu_tried_cnt = %u",
            htt_stats_buf->hwq_beacon_mpdu_tried_cnt);
    HTT_STATS_PRINT(FATAL, "hwq_voice_mpdu_tried_cnt = %u",
            htt_stats_buf->hwq_voice_mpdu_tried_cnt);
    HTT_STATS_PRINT(FATAL, "hwq_video_mpdu_tried_cnt = %u",
            htt_stats_buf->hwq_video_mpdu_tried_cnt);
    HTT_STATS_PRINT(FATAL, "hwq_best_effort_mpdu_tried_cnt = %u",
            htt_stats_buf->hwq_best_effort_mpdu_tried_cnt);
    HTT_STATS_PRINT(FATAL, "hwq_beacon_mpdu_queued_cnt = %u",
            htt_stats_buf->hwq_beacon_mpdu_queued_cnt);
    HTT_STATS_PRINT(FATAL, "hwq_voice_mpdu_queued_cnt = %u",
            htt_stats_buf->hwq_voice_mpdu_queued_cnt);
    HTT_STATS_PRINT(FATAL, "hwq_video_mpdu_queued_cnt = %u",
            htt_stats_buf->hwq_video_mpdu_queued_cnt);
    HTT_STATS_PRINT(FATAL, "hwq_best_effort_mpdu_queued_cnt = %u",
            htt_stats_buf->hwq_best_effort_mpdu_queued_cnt);
    HTT_STATS_PRINT(FATAL, "hwq_beacon_mpdu_ack_fail_cnt = %u",
            htt_stats_buf->hwq_beacon_mpdu_ack_fail_cnt);
    HTT_STATS_PRINT(FATAL, "hwq_voice_mpdu_ack_fail_cnt = %u",
            htt_stats_buf->hwq_voice_mpdu_ack_fail_cnt);
    HTT_STATS_PRINT(FATAL, "hwq_video_mpdu_ack_fail_cnt = %u",
            htt_stats_buf->hwq_video_mpdu_ack_fail_cnt);
    HTT_STATS_PRINT(FATAL, "hwq_best_effort_mpdu_ack_fail_cnt = %u",
            htt_stats_buf->hwq_best_effort_mpdu_ack_fail_cnt);
    HTT_STATS_PRINT(FATAL, "pdev_resets = %u",
            htt_stats_buf->pdev_resets);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset = %u",
            htt_stats_buf->phy_warm_reset);
    HTT_STATS_PRINT(FATAL, "hwsch_reset_count = %u",
            htt_stats_buf->hwsch_reset_count);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset_ucode_trig = %u",
            htt_stats_buf->phy_warm_reset_ucode_trig);
    HTT_STATS_PRINT(FATAL, "mac_cold_reset = %u",
            htt_stats_buf->mac_cold_reset);
    HTT_STATS_PRINT(FATAL, "mac_warm_reset = %u",
            htt_stats_buf->mac_warm_reset);
    HTT_STATS_PRINT(FATAL, "mac_warm_reset_restore_cal = %u",
            htt_stats_buf->mac_warm_reset_restore_cal);
    HTT_STATS_PRINT(FATAL, "phy_warm_reset_m3_ssr = %u",
            htt_stats_buf->phy_warm_reset_m3_ssr);
    HTT_STATS_PRINT(FATAL, "fw_rx_rings_reset = %u",
            htt_stats_buf->fw_rx_rings_reset);
    HTT_STATS_PRINT(FATAL, "tx_flush = %u",
            htt_stats_buf->tx_flush);
    HTT_STATS_PRINT(FATAL, "hwsch_dev_reset_war = %u",
            htt_stats_buf->hwsch_dev_reset_war);
    HTT_STATS_PRINT(FATAL, "mac_cold_reset_restore_cal = %u",
            htt_stats_buf->mac_cold_reset_restore_cal);
    HTT_STATS_PRINT(FATAL, "mac_only_reset = %u",
            htt_stats_buf->mac_only_reset);
    HTT_STATS_PRINT(FATAL, "mac_sfm_reset = %u",
            htt_stats_buf->mac_sfm_reset);
    HTT_STATS_PRINT(FATAL, "rx_ldpc = %u",
            htt_stats_buf->rx_ldpc);
    HTT_STATS_PRINT(FATAL, "tx_ldpc = %u \n",
            htt_stats_buf->tx_ldpc);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_TQM_MAX_GEN_MPDU_END_REASON; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->gen_mpdu_end_reason[i]);
    }
    HTT_STATS_PRINT(FATAL, "gen_mpdu_end_reason = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_TQM_MAX_LIST_MPDU_END_REASON; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->list_mpdu_end_reason[i]);
    }
    HTT_STATS_PRINT(FATAL, "list_mpdu_end_reason = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    tag_len = (HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS +
        HTT_TX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS +
        HTT_TX_PDEV_STATS_NUM_EXTRA2_MCS_COUNTERS);

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->tx_mcs[i]);
    }
    HTT_STATS_PRINT(FATAL, "tx_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i + 1, htt_stats_buf->tx_nss[i]);
    }
    HTT_STATS_PRINT(FATAL, "tx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->tx_bw[i]);
    }
    HTT_STATS_PRINT(FATAL, "tx_bw = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->half_tx_bw[i]);
    }
    HTT_STATS_PRINT(FATAL, "half_tx_bw = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->quarter_tx_bw[i]);
    }
    HTT_STATS_PRINT(FATAL, "quarter_tx_bw = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_PUNCTURED_MODE_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->tx_su_punctured_mode[i]);
    }
    HTT_STATS_PRINT(FATAL, "tx_su_punctured_mode = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    tag_len = (HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS +
        HTT_RX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS +
        HTT_RX_PDEV_STATS_NUM_EXTRA2_MCS_COUNTERS);

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->rx_mcs[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i + 1, htt_stats_buf->rx_nss[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->rx_bw[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_bw = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    tag_len = (HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS +
        HTT_RX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS +
        HTT_RX_PDEV_STATS_NUM_EXTRA2_MCS_COUNTERS);

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->rx_stbc[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_stbc = %s", str_buf);

    HTT_STATS_PRINT(FATAL, "rts_cnt = %u",
        htt_stats_buf->rts_cnt);
    HTT_STATS_PRINT(FATAL, "rts_success = %u",
        htt_stats_buf->rts_success);

    HTT_STATS_PRINT(FATAL, "====================");
}

static void htt_print_dmac_reset_stats_tlv(A_UINT32 *tag_buf)
{
    htt_dmac_reset_stats_tlv *htt_stats_buf =
        (htt_dmac_reset_stats_tlv *)tag_buf;
    HTT_STATS_PRINT(FATAL, "HTT_DMAC_RESET_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "reset_count = %u",
            htt_stats_buf->reset_count);
    HTT_STATS_PRINT(FATAL, "reset_time_hi_ms = 0x%x",
            htt_stats_buf->reset_time_hi_ms);
    HTT_STATS_PRINT(FATAL, "reset_time_lo_ms = 0x%x",
            htt_stats_buf->reset_time_lo_ms);
    HTT_STATS_PRINT(FATAL, "disengage_time_hi_ms = 0x%x",
            htt_stats_buf->disengage_time_hi_ms);
    HTT_STATS_PRINT(FATAL, "disengage_time_lo_ms = 0x%x",
            htt_stats_buf->disengage_time_lo_ms);
    HTT_STATS_PRINT(FATAL, "engage_time_hi_ms = 0x%x",
            htt_stats_buf->engage_time_hi_ms);
    HTT_STATS_PRINT(FATAL, "engage_time_lo_ms = 0x%x",
            htt_stats_buf->engage_time_lo_ms);
    HTT_STATS_PRINT(FATAL, "disengage_count = %u",
            htt_stats_buf->disengage_count);
    HTT_STATS_PRINT(FATAL, "engage_count = %u",
            htt_stats_buf->engage_count);
    HTT_STATS_PRINT(FATAL, "drain_dest_ring_mask = %u\n",
            htt_stats_buf->drain_dest_ring_mask);
}

/*
 * htt_print_tx_selfgen_ax_stats_tlv: display htt_tx_selfgen_ax_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_selfgen_ax_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_selfgen_ax_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_selfgen_ax_stats_tlv *htt_stats_buf =
        (htt_tx_selfgen_ax_stats_tlv *)tag_buf;
    A_UINT8 i;
    A_UINT16 index                       = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_TX_SELFGEN_AX_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "ax_su_ndpa_queued = %u",
            htt_stats_buf->ax_su_ndpa_queued);
    HTT_STATS_PRINT(FATAL, "ax_su_ndpa_tried = %u",
            htt_stats_buf->ax_su_ndpa);
    HTT_STATS_PRINT(FATAL, "ax_su_ndp_queued = %u",
            htt_stats_buf->ax_su_ndp_queued);
    HTT_STATS_PRINT(FATAL, "ax_su_ndp_tried = %u",
            htt_stats_buf->ax_su_ndp);
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_ndpa_queued = %u",
            htt_stats_buf->ax_mu_mimo_ndpa_queued);
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_ndpa_tried = %u",
            htt_stats_buf->ax_mu_mimo_ndpa);
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_ndp_queued = %u",
            htt_stats_buf->ax_mu_mimo_ndp_queued);
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_ndp_tried = %u",
            htt_stats_buf->ax_mu_mimo_ndp);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS - 1; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i + 1, htt_stats_buf->ax_mu_mimo_brpoll_queued[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_brpollX_queued = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS - 1; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i + 1, htt_stats_buf->ax_mu_mimo_brpoll[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_brpollX_tried = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i + 1, htt_stats_buf->ax_ul_mumimo_trigger[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_ul_mumimo_trigger = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "ax_basic_trigger = %u",
        htt_stats_buf->ax_basic_trigger);
    HTT_STATS_PRINT(FATAL, "ax_ulmumimo_total_trigger = %u",
        htt_stats_buf->ax_ulmumimo_trigger);
    HTT_STATS_PRINT(FATAL, "ax_bsr_trigger = %u",
        htt_stats_buf->ax_bsr_trigger);
    HTT_STATS_PRINT(FATAL, "ax_mu_bar_trigger = %u",
        htt_stats_buf->ax_mu_bar_trigger);
    HTT_STATS_PRINT(FATAL, "ax_mu_rts_trigger = %u",
        htt_stats_buf->ax_mu_rts_trigger);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->combined_ax_bsr_trigger_tried[i]);
    }
    HTT_STATS_PRINT(FATAL, "combined_ax_bsr_trigger_tried = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->combined_ax_bsr_trigger_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "combined_ax_bsr_trigger_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->standalone_ax_bsr_trigger_tried[i]);
    }
    HTT_STATS_PRINT(FATAL, "standalone_ax_bsr_trigger_tried = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->standalone_ax_bsr_trigger_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "standalone_ax_bsr_trigger_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->manual_ax_su_ulofdma_basic_trigger[i]);
    }
    HTT_STATS_PRINT(FATAL, "manual_ax_su_ulofdma_basic_trigger = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->manual_ax_su_ulofdma_basic_trigger_err[i]);
    }
    HTT_STATS_PRINT(FATAL,
        "manual_ax_su_ulofdma_basic_trigger_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->manual_ax_mu_ulofdma_basic_trigger[i]);
    }
    HTT_STATS_PRINT(FATAL, "manual_ax_mu_ulofdma_basic_trigger = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->manual_ax_mu_ulofdma_basic_trigger_err[i]);
    }
    HTT_STATS_PRINT(FATAL,
        "manual_ax_mu_ulofdma_basic_trigger_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->ax_basic_trigger_per_ac[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_basic_trigger_per_ac = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->ax_basic_trigger_errors_per_ac[i]);
    }
    HTT_STATS_PRINT(FATAL,
        "ax_basic_trigger_errors_per_ac = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->ax_mu_bar_trigger_per_ac[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_bar_trigger_per_ac = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->ax_mu_bar_trigger_errors_per_ac[i]);
    }
    HTT_STATS_PRINT(FATAL,
        "ax_mu_bar_trigger_errors_per_ac = %s \n", str_buf);
}

/*
 * htt_print_tx_selfgen_be_stats_tlv: display htt_tx_selfgen_be_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_selfgen_be_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_selfgen_be_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_selfgen_be_stats_tlv *htt_stats_buf =
        (htt_tx_selfgen_be_stats_tlv *)tag_buf;
    A_UINT8 i;
    A_UINT16 index                       = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_TX_SELFGEN_BE_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "be_su_ndpa_queued = %u",
        htt_stats_buf->be_su_ndpa_queued);
    HTT_STATS_PRINT(FATAL, "be_su_ndpa_tried = %u",
        htt_stats_buf->be_su_ndpa);
    HTT_STATS_PRINT(FATAL, "be_su_ndp_queued = %u",
        htt_stats_buf->be_su_ndp_queued);
    HTT_STATS_PRINT(FATAL, "be_su_ndp_tried = %u",
        htt_stats_buf->be_su_ndp);
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_ndpa_queued = %u",
        htt_stats_buf->be_mu_mimo_ndpa_queued);
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_ndpa_tried = %u",
        htt_stats_buf->be_mu_mimo_ndpa);
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_ndp_queued = %u",
        htt_stats_buf->be_mu_mimo_ndp_queued);
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_ndp_tried = %u",
        htt_stats_buf->be_mu_mimo_ndp);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MUMIMO_USER_STATS - 1; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i + 1, htt_stats_buf->be_mu_mimo_brpoll_queued[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_brpollX_queued = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MUMIMO_USER_STATS - 1; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i + 1, htt_stats_buf->be_mu_mimo_brpoll[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_brpollX_tried = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MUMIMO_USER_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i + 1, htt_stats_buf->be_ul_mumimo_trigger[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_ul_mumimo_trigger = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "be_basic_trigger = %u",
        htt_stats_buf->be_basic_trigger);
    HTT_STATS_PRINT(FATAL, "be_ulmumimo_total_trigger = %u",
        htt_stats_buf->be_ulmumimo_trigger);
    HTT_STATS_PRINT(FATAL, "be_bsr_trigger = %u",
        htt_stats_buf->be_bsr_trigger);
    HTT_STATS_PRINT(FATAL, "be_mu_bar_trigger = %u",
        htt_stats_buf->be_mu_bar_trigger);
    HTT_STATS_PRINT(FATAL, "be_mu_rts_trigger = %u",
        htt_stats_buf->be_mu_rts_trigger);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->combined_be_bsr_trigger_tried[i]);
    }
    HTT_STATS_PRINT(FATAL, "combined_be_bsr_trigger_tried = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->combined_be_bsr_trigger_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "combined_be_bsr_trigger_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->standalone_be_bsr_trigger_tried[i]);
    }
    HTT_STATS_PRINT(FATAL, "standalone_be_bsr_trigger_tried = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->standalone_be_bsr_trigger_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "standalone_be_bsr_trigger_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->manual_be_su_ulofdma_basic_trigger[i]);
    }
    HTT_STATS_PRINT(FATAL, "manual_be_su_ulofdma_basic_trigger = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->manual_be_su_ulofdma_basic_trigger_err[i]);
    }
    HTT_STATS_PRINT(FATAL,
        "manual_be_su_ulofdma_basic_trigger_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->manual_be_mu_ulofdma_basic_trigger[i]);
    }
    HTT_STATS_PRINT(FATAL, "manual_be_mu_ulofdma_basic_trigger = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->manual_be_mu_ulofdma_basic_trigger_err[i]);
    }
    HTT_STATS_PRINT(FATAL,
        "manual_be_mu_ulofdma_basic_trigger_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->be_basic_trigger_per_ac[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_basic_trigger_per_ac = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->be_basic_trigger_errors_per_ac[i]);
    }
    HTT_STATS_PRINT(FATAL,
        "be_basic_trigger_errors_per_ac = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->be_mu_bar_trigger_per_ac[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_bar_trigger_per_ac = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,
            htt_stats_buf->be_mu_bar_trigger_errors_per_ac[i]);
    }
    HTT_STATS_PRINT(FATAL,
        "be_mu_bar_trigger_errors_per_ac = %s \n", str_buf);
}

/*
 * htt_print_tx_selfgen_ac_err_stats_tlv: display htt_tx_selfgen_ac_err_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_selfgen_ac_err_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_selfgen_ac_err_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_selfgen_ac_err_stats_tlv *htt_stats_buf =
        (htt_tx_selfgen_ac_err_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_SELFGEN_AC_ERR_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "ac_su_ndp_err = %u",
            htt_stats_buf->ac_su_ndp_err);
    HTT_STATS_PRINT(FATAL, "ac_su_ndp_flushed = %u",
            htt_stats_buf->ac_su_ndp_flushed);
    HTT_STATS_PRINT(FATAL, "ac_su_ndpa_err = %u",
            htt_stats_buf->ac_su_ndpa_err);
    HTT_STATS_PRINT(FATAL, "ac_su_ndpa_flushed = %u",
            htt_stats_buf->ac_su_ndpa_flushed);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_ndpa_err = %u",
            htt_stats_buf->ac_mu_mimo_ndpa_err);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_ndpa_flushed = %u",
            htt_stats_buf->ac_mu_mimo_ndpa_flushed);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_ndp_err = %u",
            htt_stats_buf->ac_mu_mimo_ndp_err);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_ndp_flushed = %u",
            htt_stats_buf->ac_mu_mimo_ndp_flushed);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brp1_err = %u",
            htt_stats_buf->ac_mu_mimo_brp1_err);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brp2_err = %u",
            htt_stats_buf->ac_mu_mimo_brp2_err);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brp3_err = %u\n",
            htt_stats_buf->ac_mu_mimo_brp3_err);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brp1_flushed = %u",
            htt_stats_buf->ac_mu_mimo_brpoll1_flushed);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brp2_flushed = %u",
            htt_stats_buf->ac_mu_mimo_brpoll2_flushed);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brp3_flushed = %u\n",
            htt_stats_buf->ac_mu_mimo_brpoll3_flushed);
}

/*
 * htt_print_tx_selfgen_ax_err_stats_tlv: display htt_tx_selfgen_ax_err_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_selfgen_ax_err_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_selfgen_ax_err_stats_tlv(A_UINT32 *tag_buf)
{
    A_UINT16 index = 0;
    A_UINT8  i = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};

    htt_tx_selfgen_ax_err_stats_tlv *htt_stats_buf =
        (htt_tx_selfgen_ax_err_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_SELFGEN_AX_ERR_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "ax_su_ndp_err = %u",
            htt_stats_buf->ax_su_ndp_err);
    HTT_STATS_PRINT(FATAL, "ax_su_ndp_flushed = %u",
            htt_stats_buf->ax_su_ndp_flushed);
    HTT_STATS_PRINT(FATAL, "ax_su_ndpa_err = %u",
            htt_stats_buf->ax_su_ndpa_err);
    HTT_STATS_PRINT(FATAL, "ax_su_ndpa_flushed = %u",
            htt_stats_buf->ax_su_ndpa_flushed);
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_ndpa_err = %u",
            htt_stats_buf->ax_mu_mimo_ndpa_err);
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_ndpa_flushed = %u",
            htt_stats_buf->ax_mu_mimo_ndpa_flushed);
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_ndp_err = %u",
            htt_stats_buf->ax_mu_mimo_ndp_err);
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_ndp_flushed = %u",
            htt_stats_buf->ax_mu_mimo_ndp_flushed);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS - 1; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_mu_mimo_brp_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_brpX_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS - 1; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_mu_mimo_brpoll_flushed[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_brpollX_flushed = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index, " %u:%u,",
            i, htt_stats_buf->ax_mu_mimo_brp_err_num_cbf_received[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_num_cbf_rcvd_on_brp_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index, " %u:%u,",
            i+1, htt_stats_buf->ax_ul_mumimo_trigger_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_ul_mumimo_trigger_err = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "ax_basic_trigger_err = %u",
            htt_stats_buf->ax_basic_trigger_err);
    HTT_STATS_PRINT(FATAL, "ax_ulmumimo_total_trigger_err = %u",
            htt_stats_buf->ax_ulmumimo_trigger_err);
    HTT_STATS_PRINT(FATAL, "ax_bsr_trigger_err = %u",
            htt_stats_buf->ax_bsr_trigger_err);
    HTT_STATS_PRINT(FATAL, "ax_mu_bar_trigger_err = %u",
            htt_stats_buf->ax_mu_bar_trigger_err);
    HTT_STATS_PRINT(FATAL, "ax_mu_rts_trigger_err = %u",
            htt_stats_buf->ax_mu_rts_trigger_err);

    HTT_STATS_PRINT(FATAL, "ax_basic_trigger_partial_resp = %u",
        htt_stats_buf->ax_basic_trigger_partial_resp);
    HTT_STATS_PRINT(FATAL, "ax_bsr_trigger_partial_resp = %u",
        htt_stats_buf->ax_bsr_trigger_partial_resp);
    HTT_STATS_PRINT(FATAL, "ax_mu_bar_trigger_partial_resp = %u",
        htt_stats_buf->ax_mu_bar_trigger_partial_resp);

    HTT_STATS_PRINT(FATAL, "\n");
}

/*
 * htt_print_tx_selfgen_be_err_stats_tlv: display htt_tx_selfgen_be_err_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_selfgen_be_err_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_selfgen_be_err_stats_tlv(A_UINT32 *tag_buf)
{
    A_UINT16 index = 0;
    A_UINT8  i = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};

    htt_tx_selfgen_be_err_stats_tlv *htt_stats_buf =
        (htt_tx_selfgen_be_err_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_SELFGEN_BE_ERR_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "be_su_ndp_err = %u",
            htt_stats_buf->be_su_ndp_err);
    HTT_STATS_PRINT(FATAL, "be_su_ndp_flushed = %u",
            htt_stats_buf->be_su_ndp_flushed);
    HTT_STATS_PRINT(FATAL, "be_su_ndpa_err = %u",
            htt_stats_buf->be_su_ndpa_err);
    HTT_STATS_PRINT(FATAL, "be_su_ndpa_flushed = %u",
            htt_stats_buf->be_su_ndpa_flushed);
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_ndpa_err = %u",
            htt_stats_buf->be_mu_mimo_ndpa_err);
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_ndpa_flushed = %u",
            htt_stats_buf->be_mu_mimo_ndpa_flushed);
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_ndp_err = %u",
            htt_stats_buf->be_mu_mimo_ndp_err);
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_ndp_flushed = %u",
            htt_stats_buf->be_mu_mimo_ndp_flushed);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MUMIMO_USER_STATS - 1; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_mu_mimo_brp_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_brpX_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MUMIMO_USER_STATS - 1; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_mu_mimo_brpoll_flushed[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_brpollX_flushed = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MUMIMO_USER_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index, " %u:%u,",
            i, htt_stats_buf->be_mu_mimo_brp_err_num_cbf_received[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_num_cbf_rcvd_on_brp_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MUMIMO_USER_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index, " %u:%u,",
            i+1, htt_stats_buf->be_ul_mumimo_trigger_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_ul_mumimo_trigger_err = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "be_basic_trigger_err = %u",
            htt_stats_buf->be_basic_trigger_err);
    HTT_STATS_PRINT(FATAL, "be_ulmumimo_total_trigger_err = %u",
            htt_stats_buf->be_ulmumimo_trigger_err);
    HTT_STATS_PRINT(FATAL, "be_bsr_trigger_err = %u",
            htt_stats_buf->be_bsr_trigger_err);
    HTT_STATS_PRINT(FATAL, "be_mu_bar_trigger_err = %u",
            htt_stats_buf->be_mu_bar_trigger_err);
    HTT_STATS_PRINT(FATAL, "be_mu_rts_trigger_err = %u",
            htt_stats_buf->be_mu_rts_trigger_err);

    HTT_STATS_PRINT(FATAL, "be_basic_trigger_partial_resp = %u",
        htt_stats_buf->be_basic_trigger_partial_resp);
    HTT_STATS_PRINT(FATAL, "be_bsr_trigger_partial_resp = %u",
        htt_stats_buf->be_bsr_trigger_partial_resp);
    HTT_STATS_PRINT(FATAL, "be_mu_bar_trigger_partial_resp = %u",
        htt_stats_buf->be_mu_bar_trigger_partial_resp);

    HTT_STATS_PRINT(FATAL, "be_mu_rts_trigger_blocked = %u",
        htt_stats_buf->be_mu_rts_trigger_blocked);
    HTT_STATS_PRINT(FATAL, "be_bsr_trigger_blocked = %u",
        htt_stats_buf->be_bsr_trigger_blocked);

    HTT_STATS_PRINT(FATAL, "\n");
}

/*
 * htt_print_tx_selfgen_ac_sched_status_stats_tlv: display htt_tx_selfgen_ac_sched_status_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_selfgen_ac_sched_status_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_selfgen_ac_sched_status_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_selfgen_ac_sched_status_stats_tlv *htt_stats_buf = (htt_tx_selfgen_ac_sched_status_stats_tlv*) tag_buf;
    A_UINT8 i;
    A_UINT16 index                       = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_TX_SELFGEN_AC_SCHED_STATUS_STATS_TLV:");

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ac_su_ndpa_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "ac_su_ndpa_sch_status = %s ", str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ac_su_ndp_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "ac_su_ndp_sch_status = %s ", str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ac_mu_mimo_ndpa_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_ndpa_sch_status = %s ", str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ac_mu_mimo_ndp_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_ndp_sch_status = %s ", str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ac_mu_mimo_brp_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brp_sch_status = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_SELFGEN_NUM_SCH_TSFLAG_ERROR_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ac_su_ndp_sch_flag_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "ac_su_ndp_sch_flag_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_SELFGEN_NUM_SCH_TSFLAG_ERROR_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ac_mu_mimo_ndp_sch_flag_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_ndp_sch_flag_err = %s ", str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_SELFGEN_NUM_SCH_TSFLAG_ERROR_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ac_mu_mimo_brp_sch_flag_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_brp_sch_flag_err = %s \n ", str_buf);
}

/*
 htt_print_tx_selfgen_ax_sched_status_stats_tlv : display htt_tx_selfgen_ax_sched_status_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_selfgen_ax_sched_status_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_selfgen_ax_sched_status_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_selfgen_ax_sched_status_stats_tlv *htt_stats_buf = (htt_tx_selfgen_ax_sched_status_stats_tlv*) tag_buf;
    A_UINT8 i;
    A_UINT16 index                       = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_TX_SELFGEN_AX_SCHED_STATUS_STATS_TLV:");

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_su_ndpa_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_su_ndpa_sch_status = %s ", str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_su_ndp_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_su_ndp_sch_status = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_mu_mimo_ndpa_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_ndpa_sch_status = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_mu_mimo_ndp_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_ndp_sch_status = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_mu_brp_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_brp_sch_status = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_mu_bar_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_bar_sch_status = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_basic_trig_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_basic_trig_sch_status = %s ", str_buf);


    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_SELFGEN_NUM_SCH_TSFLAG_ERROR_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_su_ndp_sch_flag_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_su_ndp_sch_flag_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_SELFGEN_NUM_SCH_TSFLAG_ERROR_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_mu_mimo_ndp_sch_flag_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_ndp_sch_flag_err = %s ", str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_SELFGEN_NUM_SCH_TSFLAG_ERROR_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_mu_brp_sch_flag_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_brp_sch_flag_err = %s ", str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_SELFGEN_NUM_SCH_TSFLAG_ERROR_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_mu_bar_sch_flag_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_bar_sch_flag_err = %s ", str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_SELFGEN_NUM_SCH_TSFLAG_ERROR_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_basic_trig_sch_flag_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_basic_trig_sch_flag_err = %s \n ", str_buf);
}

/*
 * htt_print_tx_selfgen_be_sched_status_stats_tlv : display htt_tx_selfgen_be_sched_status_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_selfgen_be_sched_status_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_selfgen_be_sched_status_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_selfgen_be_sched_status_stats_tlv *htt_stats_buf = (htt_tx_selfgen_be_sched_status_stats_tlv*) tag_buf;
    A_UINT8 i;
    A_UINT16 index                       = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_TX_SELFGEN_BE_SCHED_STATUS_STATS_TLV:");

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_su_ndpa_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_su_ndpa_sch_status = %s ", str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_su_ndp_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_su_ndp_sch_status = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_mu_mimo_ndpa_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_ndpa_sch_status = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_mu_mimo_ndp_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_ndp_sch_status = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_mu_brp_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_brp_sch_status = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_mu_bar_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_bar_sch_status = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_TX_ERR_STATUS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_basic_trig_sch_status[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_basic_trig_sch_status = %s ", str_buf);


    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_SELFGEN_NUM_SCH_TSFLAG_ERROR_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_su_ndp_sch_flag_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_su_ndp_sch_flag_err = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_SELFGEN_NUM_SCH_TSFLAG_ERROR_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_mu_mimo_ndp_sch_flag_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_ndp_sch_flag_err = %s ", str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_SELFGEN_NUM_SCH_TSFLAG_ERROR_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_mu_brp_sch_flag_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_brp_sch_flag_err = %s ", str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_SELFGEN_NUM_SCH_TSFLAG_ERROR_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_mu_bar_sch_flag_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_bar_sch_flag_err = %s ", str_buf);
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_SELFGEN_NUM_SCH_TSFLAG_ERROR_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_basic_trig_sch_flag_err[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_basic_trig_sch_flag_err = %s \n ", str_buf);
}

static void htt_print_tx_pdev_dl_mu_mimo_sch_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_dl_mu_mimo_sch_stats_tlv *htt_stats_buf =
        (htt_tx_pdev_dl_mu_mimo_sch_stats_tlv *)tag_buf;
    A_UINT8 i;

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_MU_MIMO_SCH_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mu_mimo_sch_posted = %u",
            htt_stats_buf->mu_mimo_sch_posted);
    HTT_STATS_PRINT(FATAL, "mu_mimo_sch_failed = %u",
            htt_stats_buf->mu_mimo_sch_failed);
    HTT_STATS_PRINT(FATAL, "mu_mimo_ppdu_posted = %u\n",
            htt_stats_buf->mu_mimo_ppdu_posted);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AC_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "ac_mu_mimo_sch_posted_per_group_index %u = %u,",
            i, htt_stats_buf->ac_mu_mimo_sch_posted_per_grp_sz[i]);
    }
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AC_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "ac_mu_mimo_sch_posted_per_group_index %u = %u,",
            i + HTT_TX_PDEV_STATS_NUM_AC_MUMIMO_USER_STATS,
            htt_stats_buf->ac_mu_mimo_sch_posted_per_grp_sz_ext[i]);
    }

    HTT_STATS_PRINT(FATAL, "\n");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "ax_mu_mimo_sch_posted_per_group_index %u = %u,",
                i, htt_stats_buf->ax_mu_mimo_sch_posted_per_grp_sz[i]);
    }
    HTT_STATS_PRINT(FATAL, "\n");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "be_mu_mimo_sch_posted_per_group_index %u = %u,",
                i, htt_stats_buf->be_mu_mimo_sch_posted_per_grp_sz[i]);
    }
    HTT_STATS_PRINT(FATAL, "\n");

/*
    A_UINT32 ac_mu_mimo_sch_nusers[HTT_TX_PDEV_STATS_NUM_AC_MUMIMO_USER_STATS];
    A_UINT32 ax_mu_mimo_sch_nusers[HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS];
 */
    HTT_STATS_PRINT(FATAL, "11ac DL MU_MIMO SCH STATS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AC_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "ac_mu_mimo_sch_nusers_%u = %u", i,
                htt_stats_buf->ac_mu_mimo_sch_nusers[i]);
    }

    HTT_STATS_PRINT(FATAL, "\n11ax DL MU_MIMO SCH STATS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "ax_mu_mimo_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_mu_mimo_sch_nusers[i]);
    }
    HTT_STATS_PRINT(FATAL, "\n");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "be_mu_mimo_sch_nusers_%u = %u", i,
                htt_stats_buf->be_mu_mimo_sch_nusers[i]);
    }
    HTT_STATS_PRINT(FATAL, "\n");
}

static void htt_print_tx_pdev_ul_mu_mimo_sch_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_ul_mu_mimo_sch_stats_tlv *htt_stats_buf =
        (htt_tx_pdev_ul_mu_mimo_sch_stats_tlv *)tag_buf;
    A_UINT8 i;

    HTT_STATS_PRINT(FATAL, "\n11ax UL MU_MIMO SCH STATS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_UL_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "ax_ul_mu_mimo_basic_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_ul_mu_mimo_basic_sch_nusers[i]);
        HTT_STATS_PRINT(FATAL, "ax_ul_mu_mimo_brp_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_ul_mu_mimo_brp_sch_nusers[i]);
    }
}

static void htt_print_tx_pdev_be_ul_mu_mimo_sch_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_be_ul_mu_mimo_sch_stats_tlv *htt_stats_buf =
        (htt_tx_pdev_be_ul_mu_mimo_sch_stats_tlv *)tag_buf;
    A_UINT8 i;

    HTT_STATS_PRINT(FATAL, "\n11be UL MU_MIMO SCH STATS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_UL_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "be_ul_mu_mimo_basic_sch_nusers_%u = %u", i,
                htt_stats_buf->be_ul_mu_mimo_basic_sch_nusers[i]);
        HTT_STATS_PRINT(FATAL, "be_ul_mu_mimo_brp_sch_nusers_%u = %u", i,
                htt_stats_buf->be_ul_mu_mimo_brp_sch_nusers[i]);
    }
}

static void htt_print_tx_pdev_ul_mu_ofdma_sch_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_ul_mu_ofdma_sch_stats_tlv *htt_stats_buf =
        (htt_tx_pdev_ul_mu_ofdma_sch_stats_tlv *)tag_buf;
    A_UINT8 i;

    HTT_STATS_PRINT(FATAL, "\n11ax UL MU_OFDMA SCH STATS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "ax_ul_mu_ofdma_basic_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_ul_mu_ofdma_basic_sch_nusers[i]);
        HTT_STATS_PRINT(FATAL, "ax_ul_mu_ofdma_bsr_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_ul_mu_ofdma_bsr_sch_nusers[i]);
        HTT_STATS_PRINT(FATAL, "ax_ul_mu_ofdma_bar_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_ul_mu_ofdma_bar_sch_nusers[i]);
        HTT_STATS_PRINT(FATAL, "ax_ul_mu_ofdma_brp_sch_nusers_%u = %u\n", i,
                htt_stats_buf-> ax_ul_mu_ofdma_brp_sch_nusers[i]);
    }
}

static void htt_print_tx_pdev_dl_mu_ofdma_sch_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_dl_mu_ofdma_sch_stats_tlv *htt_stats_buf =
        (htt_tx_pdev_dl_mu_ofdma_sch_stats_tlv *)tag_buf;
    A_UINT8 i;

    HTT_STATS_PRINT(FATAL, "\n11ax DL MU_OFDMA SCH STATS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "ax_mu_ofdma_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_mu_ofdma_sch_nusers[i]);
    }
}

/*
 * htt_print_tx_pdev_mu_mimo_sch_stats_tlv: display htt_tx_pdev_mu_mimo_sch_stats
 * @tag_buf: buffer containing the tlv htt_tx_pdev_mu_mimo_sch_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_pdev_mu_mimo_sch_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_mu_mimo_sch_stats_tlv *htt_stats_buf =
        (htt_tx_pdev_mu_mimo_sch_stats_tlv *)tag_buf;
    A_UINT8 i;

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_MU_MIMO_SCH_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mu_mimo_sch_posted = %u",
            htt_stats_buf->mu_mimo_sch_posted);
    HTT_STATS_PRINT(FATAL, "mu_mimo_sch_failed = %u",
            htt_stats_buf->mu_mimo_sch_failed);
    HTT_STATS_PRINT(FATAL, "mu_mimo_ppdu_posted = %u\n",
            htt_stats_buf->mu_mimo_ppdu_posted);

    HTT_STATS_PRINT(FATAL,
        "ac_mu_mimo_sch_posted_per_group_index %u (SU) = %u,",
        0, htt_stats_buf->ac_mu_mimo_sch_posted_per_grp_sz[0]);
    for (i = 1; i < HTT_TX_PDEV_STATS_NUM_AC_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL,
            "ac_mu_mimo_sch_posted_per_group_index %u "
            "(TOTAL STREAMS = %u) = %u,",
            i, i+1, htt_stats_buf->ac_mu_mimo_sch_posted_per_grp_sz[i]);
    }
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AC_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL,
            "ac_mu_mimo_sch_posted_per_group_index %u "
            "(TOTAL STREAMS = %u) = %u,",
            i+HTT_TX_PDEV_STATS_NUM_AC_MUMIMO_USER_STATS,
            i+HTT_TX_PDEV_STATS_NUM_AC_MUMIMO_USER_STATS+1,
            htt_stats_buf->ac_mu_mimo_sch_posted_per_grp_sz_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "\n");

    HTT_STATS_PRINT(FATAL,
        "ax_mu_mimo_sch_posted_per_group_index %u (SU) = %u,",
        0, htt_stats_buf->ax_mu_mimo_sch_posted_per_grp_sz[0]);
    for (i = 1; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL,
            "ax_mu_mimo_sch_posted_per_group_index %u "
            "(TOTAL STREAMS = %u) = %u,",
            i,i+1,htt_stats_buf->ax_mu_mimo_sch_posted_per_grp_sz[i]);
    }
    HTT_STATS_PRINT(FATAL, "\n");

    HTT_STATS_PRINT(FATAL,
        "be_mu_mimo_sch_posted_per_group_index %u (SU) = %u,",
        0, htt_stats_buf->be_mu_mimo_sch_posted_per_grp_sz[0]);
    for (i = 1; i < HTT_TX_PDEV_STATS_NUM_BE_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL,
            "be_mu_mimo_sch_posted_per_group_index %u "
            "(TOTAL STREAMS = %u) = %u,",
            i,i+1,htt_stats_buf->be_mu_mimo_sch_posted_per_grp_sz[i]);
    }
    HTT_STATS_PRINT(FATAL, "\n");

    HTT_STATS_PRINT(FATAL, "11ac MU_MIMO SCH STATS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AC_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "ac_mu_mimo_sch_nusers_%u = %u", i,
                htt_stats_buf->ac_mu_mimo_sch_nusers[i]);
    }

    HTT_STATS_PRINT(FATAL, "\n11ax MU_MIMO SCH STATS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "ax_mu_mimo_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_mu_mimo_sch_nusers[i]);
    }

    HTT_STATS_PRINT(FATAL, "\n11be MU_MIMO SCH STATS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "be_mu_mimo_sch_nusers_%u = %u", i,
                htt_stats_buf->be_mu_mimo_sch_nusers[i]);
    }

    HTT_STATS_PRINT(FATAL, "\n11ax OFDMA SCH STATS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "ax_ofdma_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_ofdma_sch_nusers[i]);
        HTT_STATS_PRINT(FATAL, "ax_ul_ofdma_basic_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_ul_ofdma_basic_sch_nusers[i]);
        HTT_STATS_PRINT(FATAL, "ax_ul_ofdma_bsr_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_ul_ofdma_bsr_sch_nusers[i]);
        HTT_STATS_PRINT(FATAL, "ax_ul_ofdma_bar_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_ul_ofdma_bar_sch_nusers[i]);
        HTT_STATS_PRINT(FATAL, "ax_ul_ofdma_brp_sch_nusers_%u = %u\n", i,
                htt_stats_buf-> ax_ul_ofdma_brp_sch_nusers[i]);
    }

    HTT_STATS_PRINT(FATAL, "\n11ax UL MUMIMO SCH STATS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_UL_MUMIMO_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "ax_ul_mumimo_basic_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_ul_mumimo_basic_sch_nusers[i]);
        HTT_STATS_PRINT(FATAL, "ax_ul_mumimo_brp_sch_nusers_%u = %u", i,
                htt_stats_buf->ax_ul_mumimo_brp_sch_nusers[i]);
    }
}

/*
 * htt_print_tx_pdev_mumimo_grp_stats_tlv: display htt_tx_pdev_mumimo_grp_stats
 * @tag_buf: buffer containing the tlv htt_tx_pdev_mumimo_grp_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_pdev_mumimo_grp_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_mumimo_grp_stats_tlv *htt_stats_buf =
        (htt_tx_pdev_mumimo_grp_stats_tlv *) tag_buf;
    A_UINT8 i, j;
    A_UINT16 index = 0;
    A_CHAR str_buf[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "\nHTT_TX_PDEV_MUMIMO_GRP_STATS:\n");

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_MAX_MUMIMO_GRP_SZ; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->dl_mumimo_grp_best_grp_size[i]);
    }
    HTT_STATS_PRINT(FATAL, "dl_mumimo_grp_best_grp_size = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i+1, htt_stats_buf->dl_mumimo_grp_best_num_usrs[i]);
    }
    HTT_STATS_PRINT(FATAL, "dl_mumimo_grp_best_num_usrs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_MUMIMO_TPUT_NUM_BINS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->dl_mumimo_grp_tputs[i]);
    }
    HTT_STATS_PRINT(FATAL, "dl_mumimo_grp_tputs_observed (per bin = 300 mbps) = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_MAX_MUMIMO_GRP_SZ; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->dl_mumimo_grp_eligible[i]);
    }
    HTT_STATS_PRINT(FATAL, "dl_mumimo_grp eligible = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_MAX_MUMIMO_GRP_SZ; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->dl_mumimo_grp_ineligible[i]);
    }
    HTT_STATS_PRINT(FATAL, "dl_mumimo_grp ineligible = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "dl_mumimo_grp_invalid: ");
    for (i = 0; i < HTT_STATS_MAX_MUMIMO_GRP_SZ; i++) {
        HTT_STATS_PRINT(FATAL, "grp_id = %u ", i);
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (j = 0; j < HTT_STATS_MAX_INVALID_REASON_CODE; j++) {
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                "%u:%u,", j, htt_stats_buf->dl_mumimo_grp_invalid[i * HTT_STATS_MAX_INVALID_REASON_CODE + j]);
        }
        HTT_STATS_PRINT(FATAL, "%s ", str_buf);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_MAX_MUMIMO_GRP_SZ; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->ul_mumimo_grp_best_grp_size[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_mumimo_grp_best_grp_size = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i+1, htt_stats_buf->ul_mumimo_grp_best_num_usrs[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_mumimo_grp_best_num_usrs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_MUMIMO_TPUT_NUM_BINS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->ul_mumimo_grp_tputs[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_mumimo_grp_tputs_observed (per bin = 300 mbps) = %s\n ", str_buf);
}

/*
 * htt_print_tx_pdev_mu_mimo_mpdu_stats_tlv: display
 *                htt_tx_pdev_mpdu_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_pdev_mpdu_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_pdev_mu_mimo_mpdu_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_mpdu_stats_tlv *htt_stats_buf =
        (htt_tx_pdev_mpdu_stats_tlv *)tag_buf;

    if (htt_stats_buf->tx_sched_mode == HTT_STATS_TX_SCHED_MODE_MU_MIMO_AC) {
        if (!htt_stats_buf->user_index) {
            HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_MU_MIMO_AC_MPDU_STATS:\n");
        }

        if (htt_stats_buf->user_index < HTT_TX_PDEV_STATS_NUM_AC_MUMIMO_USER_STATS) {
            HTT_STATS_PRINT(FATAL, "ac_mu_mimo_mpdus_queued_usr_%u = %u",     htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_queued_usr);
            HTT_STATS_PRINT(FATAL, "ac_mu_mimo_mpdus_tried_usr_%u = %u",      htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_tried_usr);
            HTT_STATS_PRINT(FATAL, "ac_mu_mimo_mpdus_failed_usr_%u = %u",     htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_failed_usr);
            HTT_STATS_PRINT(FATAL, "ac_mu_mimo_mpdus_requeued_usr_%u = %u",   htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_requeued_usr);
            HTT_STATS_PRINT(FATAL, "ac_mu_mimo_err_no_ba_usr_%u = %u",        htt_stats_buf->user_index,
                    htt_stats_buf->err_no_ba_usr);
            HTT_STATS_PRINT(FATAL, "ac_mu_mimo_mpdu_underrun_usr_%u = %u",    htt_stats_buf->user_index,
                    htt_stats_buf->mpdu_underrun_usr);
            HTT_STATS_PRINT(FATAL, "ac_mu_mimo_ampdu_underrun_usr_%u = %u\n", htt_stats_buf->user_index,
                    htt_stats_buf->ampdu_underrun_usr);
        }
    }

    if (htt_stats_buf->tx_sched_mode == HTT_STATS_TX_SCHED_MODE_MU_MIMO_AX) {
        if (!htt_stats_buf->user_index) {
            HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_MU_MIMO_AX_MPDU_STATS:\n");
        }

        if (htt_stats_buf->user_index < HTT_TX_PDEV_STATS_NUM_AX_MUMIMO_USER_STATS) {
            HTT_STATS_PRINT(FATAL, "ax_mu_mimo_mpdus_queued_usr_%u = %u",     htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_queued_usr);
            HTT_STATS_PRINT(FATAL, "ax_mu_mimo_mpdus_tried_usr_%u = %u",      htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_tried_usr);
            HTT_STATS_PRINT(FATAL, "ax_mu_mimo_mpdus_failed_usr_%u = %u",     htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_failed_usr);
            HTT_STATS_PRINT(FATAL, "ax_mu_mimo_mpdus_requeued_usr_%u = %u",   htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_requeued_usr);
            HTT_STATS_PRINT(FATAL, "ax_mu_mimo_err_no_ba_usr_%u = %u",        htt_stats_buf->user_index,
                    htt_stats_buf->err_no_ba_usr);
            HTT_STATS_PRINT(FATAL, "ax_mu_mimo_mpdu_underrun_usr_%u = %u",    htt_stats_buf->user_index,
                    htt_stats_buf->mpdu_underrun_usr);
            HTT_STATS_PRINT(FATAL, "ax_mu_mimo_ampdu_underrun_usr_%u = %u\n", htt_stats_buf->user_index,
                    htt_stats_buf->ampdu_underrun_usr);
        }
    }

    if (htt_stats_buf->tx_sched_mode == HTT_STATS_TX_SCHED_MODE_MU_MIMO_BE) {
        if (!htt_stats_buf->user_index) {
            HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_MU_MIMO_BE_MPDU_STATS:\n");
        }

        if (htt_stats_buf->user_index < HTT_TX_PDEV_STATS_NUM_BE_MUMIMO_USER_STATS) {
            HTT_STATS_PRINT(FATAL, "be_mu_mimo_mpdus_queued_usr_%u = %u",     htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_queued_usr);
            HTT_STATS_PRINT(FATAL, "be_mu_mimo_mpdus_tried_usr_%u = %u",      htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_tried_usr);
            HTT_STATS_PRINT(FATAL, "be_mu_mimo_mpdus_failed_usr_%u = %u",     htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_failed_usr);
            HTT_STATS_PRINT(FATAL, "be_mu_mimo_mpdus_requeued_usr_%u = %u",   htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_requeued_usr);
            HTT_STATS_PRINT(FATAL, "be_mu_mimo_err_no_ba_usr_%u = %u",        htt_stats_buf->user_index,
                    htt_stats_buf->err_no_ba_usr);
            HTT_STATS_PRINT(FATAL, "be_mu_mimo_mpdu_underrun_usr_%u = %u",    htt_stats_buf->user_index,
                    htt_stats_buf->mpdu_underrun_usr);
            HTT_STATS_PRINT(FATAL, "be_mu_mimo_ampdu_underrun_usr_%u = %u\n", htt_stats_buf->user_index,
                    htt_stats_buf->ampdu_underrun_usr);
        }
    }

    if (htt_stats_buf->tx_sched_mode == HTT_STATS_TX_SCHED_MODE_MU_OFDMA_AX) {
        if (!htt_stats_buf->user_index) {
            HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_AX_MU_OFDMA_MPDU_STATS:\n");
        }

        if (htt_stats_buf->user_index < HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS) {
            HTT_STATS_PRINT(FATAL, "ax_mu_ofdma_mpdus_queued_usr_%u = %u",     htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_queued_usr);
            HTT_STATS_PRINT(FATAL, "ax_mu_ofdma_mpdus_tried_usr_%u = %u",      htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_tried_usr);
            HTT_STATS_PRINT(FATAL, "ax_mu_ofdma_mpdus_failed_usr_%u = %u",     htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_failed_usr);
            HTT_STATS_PRINT(FATAL, "ax_mu_ofdma_mpdus_requeued_usr_%u = %u",   htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_requeued_usr);
            HTT_STATS_PRINT(FATAL, "ax_mu_ofdma_err_no_ba_usr_%u = %u",        htt_stats_buf->user_index,
                    htt_stats_buf->err_no_ba_usr);
            HTT_STATS_PRINT(FATAL, "ax_mu_ofdma_mpdu_underrun_usr_%u = %u",    htt_stats_buf->user_index,
                    htt_stats_buf->mpdu_underrun_usr);
            HTT_STATS_PRINT(FATAL, "ax_mu_ofdma_ampdu_underrun_usr_%u = %u\n", htt_stats_buf->user_index,
                    htt_stats_buf->ampdu_underrun_usr);
        }
    }

    if (htt_stats_buf->tx_sched_mode == HTT_STATS_TX_SCHED_MODE_MU_OFDMA_BE) {
        if (!htt_stats_buf->user_index) {
            HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_BE_MU_OFDMA_MPDU_STATS:\n");
        }

        if (htt_stats_buf->user_index < HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS) {
            HTT_STATS_PRINT(FATAL, "be_mu_ofdma_mpdus_queued_usr_%u = %u",     htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_queued_usr);
            HTT_STATS_PRINT(FATAL, "be_mu_ofdma_mpdus_tried_usr_%u = %u",      htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_tried_usr);
            HTT_STATS_PRINT(FATAL, "be_mu_ofdma_mpdus_failed_usr_%u = %u",     htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_failed_usr);
            HTT_STATS_PRINT(FATAL, "be_mu_ofdma_mpdus_requeued_usr_%u = %u",   htt_stats_buf->user_index,
                    htt_stats_buf->mpdus_requeued_usr);
            HTT_STATS_PRINT(FATAL, "be_mu_ofdma_err_no_ba_usr_%u = %u",        htt_stats_buf->user_index,
                    htt_stats_buf->err_no_ba_usr);
            HTT_STATS_PRINT(FATAL, "be_mu_ofdma_mpdu_underrun_usr_%u = %u",    htt_stats_buf->user_index,
                    htt_stats_buf->mpdu_underrun_usr);
            HTT_STATS_PRINT(FATAL, "be_mu_ofdma_ampdu_underrun_usr_%u = %u\n", htt_stats_buf->user_index,
                    htt_stats_buf->ampdu_underrun_usr);
        }
    }
}

/*
 * htt_print_sched_txq_cmd_posted_tlv_v: display htt_sched_txq_cmd_posted_tlv_v
 * @tag_buf: buffer containing the tlv htt_sched_txq_cmd_posted_tlv_v
 *
 * return:void
 */
static void htt_print_sched_txq_cmd_posted_tlv_v(A_UINT32 *tag_buf)
{
    htt_sched_txq_cmd_posted_tlv_v *htt_stats_buf =
        (htt_sched_txq_cmd_posted_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                = 0;
    A_CHAR   sched_cmd_posted[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                              = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_SCHED_TXQ_CMD_POSTED_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&sched_cmd_posted[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->sched_cmd_posted[i]);
    }

    HTT_STATS_PRINT(FATAL, "sched_cmd_posted = %s\n", sched_cmd_posted);
}

/*
 * htt_print_sched_txq_cmd_reaped_tlv_v: display htt_sched_txq_cmd_reaped_tlv_v
 * @tag_buf: buffer containing the tlv htt_sched_txq_cmd_reaped_tlv_v
 *
 * return:void
 */
static void htt_print_sched_txq_cmd_reaped_tlv_v(A_UINT32 *tag_buf)
{
    htt_sched_txq_cmd_reaped_tlv_v *htt_stats_buf =
        (htt_sched_txq_cmd_reaped_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                = 0;
    A_CHAR   sched_cmd_reaped[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                              = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_SCHED_TXQ_CMD_REAPED_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&sched_cmd_reaped[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->sched_cmd_reaped[i]);
    }

    HTT_STATS_PRINT(FATAL, "sched_cmd_reaped = %s\n", sched_cmd_reaped);
}

/*
 * htt_print_sched_txq_sched_order_su_tlv_v: display htt_sched_txq_sched_order_su_tlv_v
 * @tag_buf: buffer containing the tlv htt_sched_txq_sched_order_su_tlv_v
 *
 * return:void
 */
static void htt_print_sched_txq_sched_order_su_tlv_v(A_UINT32 *tag_buf)
{
    htt_sched_txq_sched_order_su_tlv_v *htt_stats_buf =
        (htt_sched_txq_sched_order_su_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                              = 0;
    A_CHAR   sched_order_su[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 sched_order_su_num_entries         = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2); /* each entry is A_UINT32, i.e. 4 bytes */

    HTT_STATS_PRINT(FATAL, "HTT_SCHED_TXQ_SCHED_ORDER_SU_TLV_V:");

    for (i = 0; i < sched_order_su_num_entries; i++) {
        index += snprintf(&sched_order_su[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->sched_order_su[i]);

        /*
         * Only process the next array element if there's enough space within
         * the print buffer to hold the entire array element printout.
         */
        if (index >= (HTT_MAX_STRING_LEN - HTT_MAX_PRINT_CHAR_PER_ELEM)) {
            break;
        }
    }

    HTT_STATS_PRINT(FATAL, "sched_order_su = %s\n", sched_order_su);
}

/*
 * htt_print_sched_txq_sched_ineligibility_tlv_v: display htt_sched_txq_sched_ineligibility_tlv_v
 * @tag_buf: buffer containing the tlv htt_sched_txq_sched_ineligibility_tlv_v
 *
 * return:void
 */
static void htt_print_sched_txq_sched_ineligibility_tlv_v(A_UINT32 *tag_buf)
{
    htt_sched_txq_sched_ineligibility_tlv_v *htt_stats_buf =
        (htt_sched_txq_sched_ineligibility_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                   = 0;
    A_CHAR   sched_ineligibility[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 sched_ineligibility_num_entries         = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2); /* each entry is A_UINT32, i.e. 4 bytes */

    HTT_STATS_PRINT(FATAL, "HTT_SCHED_TXQ_SCHED_INELIGIBILITY_V:");

    for (i = 0; i < sched_ineligibility_num_entries && index < HTT_MAX_STRING_LEN; i++) {
        index += snprintf(&sched_ineligibility[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->sched_ineligibility[i]);

        /*
         * Only process the next array element if there's enough space within
         * the print buffer to hold the entire array element printout.
         */
        if (index >= (HTT_MAX_STRING_LEN - HTT_MAX_PRINT_CHAR_PER_ELEM)) {
            break;
        }
    }

    HTT_STATS_PRINT(FATAL, "sched_ineligibility = %s\n", sched_ineligibility);
}

/*
 * htt_print_sched_txq_supercycle_trigger_tlv_v: display htt_sched_txq_supercycle_triggers_tlv_v
 * @tag_buf: buffer containing the tlv htt_sched_txq_supercycle_triggers_tlv_v
 *
 * return:void
 */
static void htt_print_sched_txq_supercycle_trigger_tlv_v(A_UINT32 *tag_buf)
{
    htt_sched_txq_supercycle_triggers_tlv_v *htt_stats_buf =
        (htt_sched_txq_supercycle_triggers_tlv_v *)tag_buf;
    A_UINT32  i;
    A_UINT16 index                                   = 0;
    A_CHAR   supercycle_triggers[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 supercycle_triggers_num_entries         = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2); /* each entry is A_UINT32, i.e. 4 bytes */

    HTT_STATS_PRINT(FATAL, "HTT_SCHED_TXQ_SUPERCYCLE_TRIGGER_V:");

    for (i = 0; i < supercycle_triggers_num_entries && index < HTT_MAX_STRING_LEN; i++) {
        index += snprintf(&supercycle_triggers[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->supercycle_triggers[i]);

        /*
         * Only process the next array element if there's enough space within
         * the print buffer to hold the entire array element printout.
         */
        if (index >= (HTT_MAX_STRING_LEN - HTT_MAX_PRINT_CHAR_PER_ELEM)) {
            break;
        }
    }

    HTT_STATS_PRINT(FATAL, "supercycle_triggers = %s\n", supercycle_triggers);
}

/*
 * htt_print_tx_pdev_stats_sched_per_txq_tlv: display
 *                htt_tx_pdev_stats_sched_per_txq_tlv
 * @tag_buf: buffer containing the tlv htt_tx_pdev_stats_sched_per_txq_tlv
 *
 * return:void
 */
static void htt_print_tx_pdev_stats_sched_per_txq_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_stats_sched_per_txq_tlv *htt_stats_buf =
        (htt_tx_pdev_stats_sched_per_txq_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_STATS_SCHED_PER_TXQ_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__txq_id__word & 0xFF);
    HTT_STATS_PRINT(FATAL, "txq_id = %u",
            (htt_stats_buf->mac_id__txq_id__word & 0xFF00) >> 8);
    /*HTT_STATS_PRINT(FATAL, "word = %u\n",
            (htt_stats_buf->mac_id__txq_id__word & 0xFFFF0000) >> 16);*/
    HTT_STATS_PRINT(FATAL, "sched_policy = %u",
            htt_stats_buf->sched_policy);
    HTT_STATS_PRINT(FATAL, "last_sched_cmd_posted_timestamp = %u",
            htt_stats_buf->last_sched_cmd_posted_timestamp);
    HTT_STATS_PRINT(FATAL, "last_sched_cmd_compl_timestamp = %u",
            htt_stats_buf->last_sched_cmd_compl_timestamp);
    HTT_STATS_PRINT(FATAL, "sched_2_tac_lwm_count = %u",
            htt_stats_buf->sched_2_tac_lwm_count);
    HTT_STATS_PRINT(FATAL, "sched_2_tac_ring_full = %u",
            htt_stats_buf->sched_2_tac_ring_full);
    HTT_STATS_PRINT(FATAL, "sched_cmd_post_failure = %u",
            htt_stats_buf->sched_cmd_post_failure);
    HTT_STATS_PRINT(FATAL, "num_active_tids = %u",
            htt_stats_buf->num_active_tids);
    HTT_STATS_PRINT(FATAL, "num_ps_schedules = %u",
            htt_stats_buf->num_ps_schedules);
    HTT_STATS_PRINT(FATAL, "sched_cmds_pending = %u",
            htt_stats_buf->sched_cmds_pending);
    HTT_STATS_PRINT(FATAL, "num_tid_register = %u",
            htt_stats_buf->num_tid_register);
    HTT_STATS_PRINT(FATAL, "num_tid_unregister = %u",
            htt_stats_buf->num_tid_unregister);
    HTT_STATS_PRINT(FATAL, "num_qstats_queried = %u",
            htt_stats_buf->num_qstats_queried);
    HTT_STATS_PRINT(FATAL, "qstats_update_pending = %u",
            htt_stats_buf->qstats_update_pending);
    HTT_STATS_PRINT(FATAL, "last_qstats_query_timestamp = %u",
            htt_stats_buf->last_qstats_query_timestamp);
    HTT_STATS_PRINT(FATAL, "num_tqm_cmdq_full = %u",
            htt_stats_buf->num_tqm_cmdq_full);
    HTT_STATS_PRINT(FATAL, "num_de_sched_algo_trigger = %u",
            htt_stats_buf->num_de_sched_algo_trigger);
    HTT_STATS_PRINT(FATAL, "num_rt_sched_algo_trigger = %u",
            htt_stats_buf->num_rt_sched_algo_trigger);
    HTT_STATS_PRINT(FATAL, "num_tqm_sched_algo_trigger = %u",
            htt_stats_buf->num_tqm_sched_algo_trigger);
    HTT_STATS_PRINT(FATAL, "notify_sched = %u",
            htt_stats_buf->notify_sched);
    HTT_STATS_PRINT(FATAL, "dur_based_sendn_term = %u",
            htt_stats_buf->dur_based_sendn_term);
    HTT_STATS_PRINT(FATAL, "su_notify2_sched = %u",
            htt_stats_buf->su_notify2_sched);
    HTT_STATS_PRINT(FATAL, "su_optimal_queued_msdus_sched = %u",
            htt_stats_buf->su_optimal_queued_msdus_sched);
    HTT_STATS_PRINT(FATAL, "su_delay_timeout_sched = %u",
            htt_stats_buf->su_delay_timeout_sched);
    HTT_STATS_PRINT(FATAL, "su_min_txtime_sched_delay = %u",
            htt_stats_buf->su_min_txtime_sched_delay);
    HTT_STATS_PRINT(FATAL, "su_no_delay = %u",
            htt_stats_buf->su_no_delay);
    HTT_STATS_PRINT(FATAL, "num_supercycles = %u",
            htt_stats_buf->num_supercycles);
    HTT_STATS_PRINT(FATAL, "num_subcycles_with_sort = %u",
            htt_stats_buf->num_subcycles_with_sort);
    HTT_STATS_PRINT(FATAL, "num_subcycles_no_sort = %u\n",
            htt_stats_buf->num_subcycles_no_sort);
}

/*
 * htt_print_stats_tx_sched_cmn_tlv: display htt_stats_tx_sched_cmn_tlv
 * @tag_buf: buffer containing the tlv htt_stats_tx_sched_cmn_tlv
 *
 * return:void
 */
static void htt_print_stats_tx_sched_cmn_tlv(A_UINT32 *tag_buf)
{
    htt_stats_tx_sched_cmn_tlv *htt_stats_buf =
        (htt_stats_tx_sched_cmn_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_STATS_TX_SCHED_CMN_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__word & 0xFF);
    /*HTT_STATS_PRINT(FATAL, "word = %u",
            ((htt_stats_buf->mac_id__word & 0xFFFFFF00) >> 8 ));*/
    HTT_STATS_PRINT(FATAL, "current_timestamp = %u\n",
            htt_stats_buf->current_timestamp);
}

/*
 * htt_print_tx_tqm_gen_mpdu_stats_tlv_v: display htt_tx_tqm_gen_mpdu_stats_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_tqm_gen_mpdu_stats_tlv_v
 *
 * return:void
 */
static void htt_print_tx_tqm_gen_mpdu_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_tqm_gen_mpdu_stats_tlv_v *htt_stats_buf =
        (htt_tx_tqm_gen_mpdu_stats_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                   = 0;
    A_CHAR   gen_mpdu_end_reason[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                                 = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_TQM_GEN_MPDU_STATS_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&gen_mpdu_end_reason[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->gen_mpdu_end_reason[i]);
    }

    HTT_STATS_PRINT(FATAL, "gen_mpdu_end_reason = %s\n", gen_mpdu_end_reason);
}

/*
 * htt_print_tx_tqm_list_mpdu_stats_tlv_v: display htt_tx_tqm_list_mpdu_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_tqm_list_mpdu_stats_tlv_v
 *
 * return:void
 */
static void htt_print_tx_tqm_list_mpdu_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_tqm_list_mpdu_stats_tlv_v *htt_stats_buf =
        (htt_tx_tqm_list_mpdu_stats_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                    = 0;
    A_CHAR   list_mpdu_end_reason[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                                  = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_TQM_LIST_MPDU_STATS_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&list_mpdu_end_reason[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->list_mpdu_end_reason[i]);
    }

    HTT_STATS_PRINT(FATAL, "list_mpdu_end_reason = %s\n",
            list_mpdu_end_reason);
}

/*
 * htt_print_tx_tqm_list_mpdu_cnt_tlv_v: display htt_tx_tqm_list_mpdu_cnt_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_tqm_list_mpdu_cnt_tlv_v
 *
 * return:void
 */
static void htt_print_tx_tqm_list_mpdu_cnt_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_tqm_list_mpdu_cnt_tlv_v *htt_stats_buf =
        (htt_tx_tqm_list_mpdu_cnt_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                  = 0;
    A_CHAR   list_mpdu_cnt_hist[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                                = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_TX_TQM_LIST_MPDU_CNT_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&list_mpdu_cnt_hist[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->list_mpdu_cnt_hist[i]);
    }

    HTT_STATS_PRINT(FATAL, "list_mpdu_cnt_hist = %s\n", list_mpdu_cnt_hist);
}

/*
 * htt_print_tx_tqm_pdev_stats_tlv_v: display htt_tx_tqm_pdev_stats_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_tqm_pdev_stats_tlv_v
 *
 * return:void
 */
static void htt_print_tx_tqm_pdev_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_tqm_pdev_stats_tlv_v *htt_stats_buf =
        (htt_tx_tqm_pdev_stats_tlv_v *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_TQM_PDEV_STATS_TLV_V:");
    HTT_STATS_PRINT(FATAL, "msdu_count = %u",
            htt_stats_buf->msdu_count);
    HTT_STATS_PRINT(FATAL, "mpdu_count = %u",
            htt_stats_buf->mpdu_count);
    HTT_STATS_PRINT(FATAL, "remove_msdu = %u",
            htt_stats_buf->remove_msdu);
    HTT_STATS_PRINT(FATAL, "remove_mpdu = %u",
            htt_stats_buf->remove_mpdu);
    HTT_STATS_PRINT(FATAL, "remove_msdu_ttl = %u",
            htt_stats_buf->remove_msdu_ttl);
    HTT_STATS_PRINT(FATAL, "send_bar = %u",
            htt_stats_buf->send_bar);
    HTT_STATS_PRINT(FATAL, "bar_sync = %u",
            htt_stats_buf->bar_sync);
    HTT_STATS_PRINT(FATAL, "notify_mpdu = %u",
            htt_stats_buf->notify_mpdu);
    HTT_STATS_PRINT(FATAL, "sync_cmd = %u",
            htt_stats_buf->sync_cmd);
    HTT_STATS_PRINT(FATAL, "write_cmd = %u",
            htt_stats_buf->write_cmd);
    HTT_STATS_PRINT(FATAL, "hwsch_trigger = %u",
            htt_stats_buf->hwsch_trigger);
    HTT_STATS_PRINT(FATAL, "ack_tlv_proc = %u",
            htt_stats_buf->ack_tlv_proc);
    HTT_STATS_PRINT(FATAL, "gen_mpdu_cmd = %u",
            htt_stats_buf->gen_mpdu_cmd);
    HTT_STATS_PRINT(FATAL, "gen_list_cmd = %u",
            htt_stats_buf->gen_list_cmd);
    HTT_STATS_PRINT(FATAL, "remove_mpdu_cmd = %u",
            htt_stats_buf->remove_mpdu_cmd);
    HTT_STATS_PRINT(FATAL, "remove_mpdu_tried_cmd = %u",
            htt_stats_buf->remove_mpdu_tried_cmd);
    HTT_STATS_PRINT(FATAL, "mpdu_queue_stats_cmd = %u",
            htt_stats_buf->mpdu_queue_stats_cmd);
    HTT_STATS_PRINT(FATAL, "mpdu_head_info_cmd = %u",
            htt_stats_buf->mpdu_head_info_cmd);
    HTT_STATS_PRINT(FATAL, "msdu_flow_stats_cmd = %u",
            htt_stats_buf->msdu_flow_stats_cmd);
    HTT_STATS_PRINT(FATAL, "remove_msdu_cmd = %u",
            htt_stats_buf->remove_msdu_cmd);
    HTT_STATS_PRINT(FATAL, "remove_msdu_ttl_cmd = %u",
            htt_stats_buf->remove_msdu_ttl_cmd);
    HTT_STATS_PRINT(FATAL, "flush_cache_cmd = %u",
            htt_stats_buf->flush_cache_cmd);
    HTT_STATS_PRINT(FATAL, "update_mpduq_cmd = %u",
            htt_stats_buf->update_mpduq_cmd);
    HTT_STATS_PRINT(FATAL, "enqueue = %u",
            htt_stats_buf->enqueue);
    HTT_STATS_PRINT(FATAL, "enqueue_notify = %u",
            htt_stats_buf->enqueue_notify);
    HTT_STATS_PRINT(FATAL, "notify_mpdu_at_head = %u",
            htt_stats_buf->notify_mpdu_at_head);
    HTT_STATS_PRINT(FATAL, "notify_mpdu_state_valid = %u",
            htt_stats_buf->notify_mpdu_state_valid);
    HTT_STATS_PRINT(FATAL, "sched_udp_notify1 = %u",
            htt_stats_buf->sched_udp_notify1);
    HTT_STATS_PRINT(FATAL, "sched_udp_notify2 = %u",
            htt_stats_buf->sched_udp_notify2);
    HTT_STATS_PRINT(FATAL, "sched_nonudp_notify1 = %u",
            htt_stats_buf->sched_nonudp_notify1);
    HTT_STATS_PRINT(FATAL, "sched_nonudp_notify2 = %u\n",
            htt_stats_buf->sched_nonudp_notify2);
    HTT_STATS_PRINT(FATAL, "tqm_enqueue_msdu_count = %u\n",
            htt_stats_buf->tqm_enqueue_msdu_count);
    HTT_STATS_PRINT(FATAL, "tqm_dropped_msdu_count = %u\n",
            htt_stats_buf->tqm_dropped_msdu_count);
    HTT_STATS_PRINT(FATAL, "tqm_dequeue_msdu_count = %u\n",
            htt_stats_buf->tqm_dequeue_msdu_count);
}

/*
 * htt_print_tx_tqm_cmn_stats_tlv: display htt_tx_tqm_cmn_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_tqm_cmn_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_tqm_cmn_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_tqm_cmn_stats_tlv *htt_stats_buf =
        (htt_tx_tqm_cmn_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_TQM_CMN_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
        htt_stats_buf->mac_id__word & 0xFF);
    /*HTT_STATS_PRINT(FATAL, "word = %u",
        ((htt_stats_buf->mac_id__word & 0xFFFFFF00) >> 8 ));*/
    HTT_STATS_PRINT(FATAL, "max_cmdq_id = %u",
        htt_stats_buf->max_cmdq_id);
    HTT_STATS_PRINT(FATAL, "list_mpdu_cnt_hist_intvl = %u",
        htt_stats_buf->list_mpdu_cnt_hist_intvl);
    HTT_STATS_PRINT(FATAL, "add_msdu = %u",
        htt_stats_buf->add_msdu);
    HTT_STATS_PRINT(FATAL, "q_empty = %u",
        htt_stats_buf->q_empty);
    HTT_STATS_PRINT(FATAL, "q_not_empty = %u",
        htt_stats_buf->q_not_empty);
    HTT_STATS_PRINT(FATAL, "drop_notification = %u",
        htt_stats_buf->drop_notification);
    HTT_STATS_PRINT(FATAL, "desc_threshold = %u",
        htt_stats_buf->desc_threshold);
    HTT_STATS_PRINT(FATAL, "hwsch_tqm_invalid_status = %u",
        htt_stats_buf->hwsch_tqm_invalid_status);
    HTT_STATS_PRINT(FATAL, "missed_tqm_gen_mpdus = %u",
        htt_stats_buf->missed_tqm_gen_mpdus);
    HTT_STATS_PRINT(FATAL, "total_msduq_timestamp_updates = %u",
        htt_stats_buf->total_msduq_timestamp_updates);
    HTT_STATS_PRINT(FATAL,
        "total_msduq_timestamp_updates_by_get_mpdu_head_info_cmd = %u",
        htt_stats_buf->total_msduq_timestamp_updates_by_get_mpdu_head_info_cmd);
    HTT_STATS_PRINT(FATAL,
        "total_msduq_timestamp_updates_by_empty_to_nonempty_status = %u",
        htt_stats_buf->total_msduq_timestamp_updates_by_empty_to_nonempty_status);
    HTT_STATS_PRINT(FATAL,
        "total_get_mpdu_head_info_cmds_by_sched_algo_la_query = %u",
        htt_stats_buf->total_get_mpdu_head_info_cmds_by_sched_algo_la_query);
    HTT_STATS_PRINT(FATAL, "total_get_mpdu_head_info_cmds_by_tac = %u",
        htt_stats_buf->total_get_mpdu_head_info_cmds_by_tac);
    HTT_STATS_PRINT(FATAL, "total_gen_mpdu_cmds_by_sched_algo_la_query = %u",
        htt_stats_buf->total_gen_mpdu_cmds_by_sched_algo_la_query);

    if (htt_stats_buf->tlv_hdr.length >
        (offsetof(htt_tx_tqm_cmn_stats_tlv, tqm_active_tids) -
         offsetof(htt_tx_tqm_cmn_stats_tlv, mac_id__word)))
    {
        HTT_STATS_PRINT(FATAL, "active_tqm_tids = %u",
            htt_stats_buf->tqm_active_tids);
        HTT_STATS_PRINT(FATAL, "inactive_tqm_tids = %u",
            htt_stats_buf->tqm_inactive_tids);
        HTT_STATS_PRINT(FATAL, "tqm_active_msduq_flows = %u",
            htt_stats_buf->tqm_active_msduq_flows);
    }
    HTT_STATS_PRINT(FATAL, "hi_prio_q_not_empty = %u",
        htt_stats_buf->high_prio_q_not_empty);

    HTT_STATS_PRINT(FATAL, "\n");
}

/*
 * htt_print_unavailable_error_stats_tlv: display htt_stats_error_tlv_v
 * @tag_buf: buffer containing the tlv htt_stats_error_tlv_v
 *
 * return:void
 */
static void htt_print_unavailable_error_stats_tlv(A_UINT32 *tag_buf)
{
   htt_stats_error_tlv_v *error_stats_tlv = (htt_stats_error_tlv_v *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_ERROR_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "No stats to print for current request: %d",
            error_stats_tlv->htt_stats_type);
}

/*
 * htt_print_unsupported_error_stats_tlv: display htt_tx_tqm_error_stats_tlv
 * @tag_buf: buffer containing the tlv htt_stats_error_tlv_v
 *
 * return:void
 */
static void htt_print_unsupported_error_stats_tlv(A_UINT32 *tag_buf)
{
    htt_stats_error_tlv_v *error_stats_tlv = (htt_stats_error_tlv_v *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_ERROR_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "Unsupported HTT stats type: %d",
            error_stats_tlv->htt_stats_type);
}

/*
 * htt_print_tx_tqm_error_stats_tlv: display htt_tx_tqm_error_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_tqm_error_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_tqm_error_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_tqm_error_stats_tlv *htt_stats_buf =
        (htt_tx_tqm_error_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_TQM_ERROR_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "q_empty_failure = %u",
            htt_stats_buf->q_empty_failure);
    HTT_STATS_PRINT(FATAL, "q_not_empty_failure = %u",
            htt_stats_buf->q_not_empty_failure);
    HTT_STATS_PRINT(FATAL, "add_msdu_failure = %u\n",
            htt_stats_buf->add_msdu_failure);

    HTT_STATS_PRINT(FATAL, "TQM_ERROR_RESET_STATS:");
    HTT_STATS_PRINT(FATAL, "tqm_cache_ctl_err = %u",
            htt_stats_buf->tqm_cache_ctl_err);
    HTT_STATS_PRINT(FATAL, "tqm_soft_reset = %u",
            htt_stats_buf->tqm_soft_reset);
    HTT_STATS_PRINT(FATAL, "tqm_reset_total_num_in_use_link_descs = %u",
            htt_stats_buf->tqm_reset_total_num_in_use_link_descs);
    HTT_STATS_PRINT(FATAL, "tqm_reset_worst_case_num_lost_link_descs = %u",
            htt_stats_buf->tqm_reset_worst_case_num_lost_link_descs);
    HTT_STATS_PRINT(FATAL,
            "tqm_reset_worst_case_num_lost_host_tx_bufs_count = %u",
            htt_stats_buf->tqm_reset_worst_case_num_lost_host_tx_bufs_count);
    HTT_STATS_PRINT(FATAL, "tqm_reset_num_in_use_link_descs_internal_tqm = %u",
            htt_stats_buf->tqm_reset_num_in_use_link_descs_internal_tqm);
    HTT_STATS_PRINT(FATAL,
            "tqm_reset_num_in_use_link_descs_wbm_idle_link_ring = %u",
            htt_stats_buf->tqm_reset_num_in_use_link_descs_wbm_idle_link_ring);
    HTT_STATS_PRINT(FATAL, "tqm_reset_time_to_tqm_hang_delta_ms = %u",
            htt_stats_buf->tqm_reset_time_to_tqm_hang_delta_ms);
    HTT_STATS_PRINT(FATAL, "tqm_reset_recovery_time_ms = %u",
            htt_stats_buf->tqm_reset_recovery_time_ms);
    HTT_STATS_PRINT(FATAL, "tqm_reset_num_peers_hdl = %u",
            htt_stats_buf->tqm_reset_num_peers_hdl);
    HTT_STATS_PRINT(FATAL, "tqm_reset_cumm_dirty_hw_mpduq_proc_cnt = %u",
            htt_stats_buf->tqm_reset_cumm_dirty_hw_mpduq_proc_cnt);
    HTT_STATS_PRINT(FATAL, "tqm_reset_cumm_dirty_hw_msduq_proc = %u\n",
            htt_stats_buf->tqm_reset_cumm_dirty_hw_msduq_proc);
    HTT_STATS_PRINT(FATAL, "tqm_reset_flush_cache_cmd_su_cnt = %u",
            htt_stats_buf->tqm_reset_flush_cache_cmd_su_cnt);
    HTT_STATS_PRINT(FATAL, "tqm_reset_flush_cache_cmd_other_cnt = %u",
            htt_stats_buf->tqm_reset_flush_cache_cmd_other_cnt);
    HTT_STATS_PRINT(FATAL, "tqm_reset_flush_cache_cmd_trig_type = %u",
            htt_stats_buf->tqm_reset_flush_cache_cmd_trig_type);
    HTT_STATS_PRINT(FATAL, "tqm_reset_flush_cache_cmd_trig_cfg = %u",
            htt_stats_buf->tqm_reset_flush_cache_cmd_trig_cfg);
    HTT_STATS_PRINT(FATAL,
            "tqm_reset_flush_cache_cmd_skip_cmd_status_null = %u",
            htt_stats_buf->tqm_reset_flush_cache_cmd_skip_cmd_status_null);
}

/*
 * htt_print_tx_tqm_cmdq_status_tlv: display htt_tx_tqm_cmdq_status_tlv
 * @tag_buf: buffer containing the tlv htt_tx_tqm_cmdq_status_tlv
 *
 * return:void
 */
static void htt_print_tx_tqm_cmdq_status_tlv(A_UINT32 *tag_buf)
{
    htt_tx_tqm_cmdq_status_tlv *htt_stats_buf =
        (htt_tx_tqm_cmdq_status_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_TQM_CMDQ_STATUS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__cmdq_id__word & 0xFF);
    HTT_STATS_PRINT(FATAL, "cmdq_id = %u\n",
            (htt_stats_buf->mac_id__cmdq_id__word & 0xFF00) >> 8);
    /*HTT_STATS_PRINT(FATAL, "word = %u\n",
            (htt_stats_buf->mac_id__cmdq_id__word & 0xFFFF0000) >> 16);*/
    HTT_STATS_PRINT(FATAL, "sync_cmd = %u",
            htt_stats_buf->sync_cmd);
    HTT_STATS_PRINT(FATAL, "write_cmd = %u",
            htt_stats_buf->write_cmd);
    HTT_STATS_PRINT(FATAL, "gen_mpdu_cmd = %u",
            htt_stats_buf->gen_mpdu_cmd);
    HTT_STATS_PRINT(FATAL, "mpdu_queue_stats_cmd = %u",
            htt_stats_buf->mpdu_queue_stats_cmd);
    HTT_STATS_PRINT(FATAL, "mpdu_head_info_cmd = %u",
            htt_stats_buf->mpdu_head_info_cmd);
    HTT_STATS_PRINT(FATAL, "msdu_flow_stats_cmd = %u",
            htt_stats_buf->msdu_flow_stats_cmd);
    HTT_STATS_PRINT(FATAL, "remove_mpdu_cmd = %u",
            htt_stats_buf->remove_mpdu_cmd);
    HTT_STATS_PRINT(FATAL, "remove_msdu_cmd = %u",
            htt_stats_buf->remove_msdu_cmd);
    HTT_STATS_PRINT(FATAL, "flush_cache_cmd = %u",
            htt_stats_buf->flush_cache_cmd);
    HTT_STATS_PRINT(FATAL, "update_mpduq_cmd = %u",
            htt_stats_buf->update_mpduq_cmd);
    HTT_STATS_PRINT(FATAL, "update_msduq_cmd = %u\n",
            htt_stats_buf->update_msduq_cmd);
}

/*
 * htt_print_tx_de_eapol_packets_stats_tlv: display htt_tx_de_eapol_packets_stats
 * @tag_buf: buffer containing the tlv htt_tx_de_eapol_packets_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_de_eapol_packets_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_de_eapol_packets_stats_tlv *htt_stats_buf =
        (htt_tx_de_eapol_packets_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_DE_EAPOL_PACKETS_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "m1_packets = %u",
            htt_stats_buf->m1_packets);
    HTT_STATS_PRINT(FATAL, "m1_success = %u",
            htt_stats_buf->m1_success);
    HTT_STATS_PRINT(FATAL, "m1_compl_fail = %u",
            htt_stats_buf->m1_compl_fail);
    HTT_STATS_PRINT(FATAL, "m2_packets = %u",
            htt_stats_buf->m2_packets);
    HTT_STATS_PRINT(FATAL, "m2_success = %u",
            htt_stats_buf->m2_success);
    HTT_STATS_PRINT(FATAL, "m2_compl_fail = %u",
            htt_stats_buf->m2_compl_fail);
    HTT_STATS_PRINT(FATAL, "m3_packets = %u",
            htt_stats_buf->m3_packets);
    HTT_STATS_PRINT(FATAL, "m3_success = %u",
            htt_stats_buf->m3_success);
    HTT_STATS_PRINT(FATAL, "m3_compl_fail = %u",
            htt_stats_buf->m3_compl_fail);
    HTT_STATS_PRINT(FATAL, "m4_packets = %u",
            htt_stats_buf->m4_packets);
    HTT_STATS_PRINT(FATAL, "m4_success = %u",
            htt_stats_buf->m4_success);
    HTT_STATS_PRINT(FATAL, "m4_compl_fail = %u",
            htt_stats_buf->m4_compl_fail);
    HTT_STATS_PRINT(FATAL, "g1_packets = %u",
            htt_stats_buf->g1_packets);
    HTT_STATS_PRINT(FATAL, "g1_success = %u",
            htt_stats_buf->g1_success);
    HTT_STATS_PRINT(FATAL, "g1_compl_fail = %u",
            htt_stats_buf->g1_compl_fail);
    HTT_STATS_PRINT(FATAL, "g2_packets = %u",
            htt_stats_buf->g2_packets);
    HTT_STATS_PRINT(FATAL, "g2_success = %u",
            htt_stats_buf->g2_success);
    HTT_STATS_PRINT(FATAL, "g2_compl_fail = %u",
            htt_stats_buf->g2_compl_fail);
    HTT_STATS_PRINT(FATAL, "rc4_packets = %u",
            htt_stats_buf->rc4_packets);
    HTT_STATS_PRINT(FATAL, "eap_packets = %u",
            htt_stats_buf->eap_packets);
    HTT_STATS_PRINT(FATAL, "eapol_start_packets = %u",
            htt_stats_buf->eapol_start_packets);
    HTT_STATS_PRINT(FATAL, "eapol_logoff_packets = %u",
            htt_stats_buf->eapol_logoff_packets);
    HTT_STATS_PRINT(FATAL, "eapol_encap_asf_packets = %u\n",
            htt_stats_buf->eapol_encap_asf_packets);
}

/*
 * htt_print_tx_de_classify_failed_stats_tlv: display
 *                htt_tx_de_classify_failed_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_de_classify_failed_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_de_classify_failed_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_de_classify_failed_stats_tlv *htt_stats_buf =
        (htt_tx_de_classify_failed_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_DE_CLASSIFY_FAILED_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "ap_bss_peer_not_found = %u",
            htt_stats_buf->ap_bss_peer_not_found);
    HTT_STATS_PRINT(FATAL, "ap_bcast_mcast_no_peer = %u",
            htt_stats_buf->ap_bcast_mcast_no_peer);
    HTT_STATS_PRINT(FATAL, "sta_delete_in_progress = %u",
            htt_stats_buf->sta_delete_in_progress);
    HTT_STATS_PRINT(FATAL, "ibss_no_bss_peer = %u",
            htt_stats_buf->ibss_no_bss_peer);
    HTT_STATS_PRINT(FATAL, "invaild_vdev_type = %u",
            htt_stats_buf->invaild_vdev_type);
    HTT_STATS_PRINT(FATAL, "invalid_ast_peer_entry = %u",
            htt_stats_buf->invalid_ast_peer_entry);
    HTT_STATS_PRINT(FATAL, "peer_entry_invalid = %u",
            htt_stats_buf->peer_entry_invalid);
    HTT_STATS_PRINT(FATAL, "ethertype_not_ip = %u",
            htt_stats_buf->ethertype_not_ip);
    HTT_STATS_PRINT(FATAL, "eapol_lookup_failed = %u",
            htt_stats_buf->eapol_lookup_failed);
    HTT_STATS_PRINT(FATAL, "qpeer_not_allow_data = %u",
            htt_stats_buf->qpeer_not_allow_data);
    HTT_STATS_PRINT(FATAL, "fse_tid_override = %u",
            htt_stats_buf->fse_tid_override);
    HTT_STATS_PRINT(FATAL, "ipv6_jumbogram_zero_length = %u",
            htt_stats_buf->ipv6_jumbogram_zero_length);
    HTT_STATS_PRINT(FATAL, "qos_to_non_qos_in_prog = %u",
            htt_stats_buf->qos_to_non_qos_in_prog);
    HTT_STATS_PRINT(FATAL, "ap_bcast_mcast_eapol = %u",
            htt_stats_buf->ap_bcast_mcast_eapol);
    HTT_STATS_PRINT(FATAL, "unicast_on_ap_bss_peer = %u",
            htt_stats_buf->unicast_on_ap_bss_peer);
    HTT_STATS_PRINT(FATAL, "ap_vdev_invalid = %u",
            htt_stats_buf->ap_vdev_invalid);
    HTT_STATS_PRINT(FATAL, "incomplete_llc = %u",
            htt_stats_buf->incomplete_llc);
    HTT_STATS_PRINT(FATAL, "eapol_duplicate_m3 = %u",
            htt_stats_buf->eapol_duplicate_m3);
    HTT_STATS_PRINT(FATAL, "eapol_duplicate_m4 = %u\n",
            htt_stats_buf->eapol_duplicate_m4);
}

/*
 * htt_print_tx_de_classify_stats_tlv: display htt_tx_de_classify_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_de_classify_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_de_classify_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_de_classify_stats_tlv *htt_stats_buf =
        (htt_tx_de_classify_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_DE_CLASSIFY_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "arp_packets = %u",
            htt_stats_buf->arp_packets);
    HTT_STATS_PRINT(FATAL, "arp_request = %u",
            htt_stats_buf->arp_request);
    HTT_STATS_PRINT(FATAL, "arp_response = %u",
            htt_stats_buf->arp_response);
    HTT_STATS_PRINT(FATAL, "igmp_packets = %u",
            htt_stats_buf->igmp_packets);
    HTT_STATS_PRINT(FATAL, "dhcp_packets = %u",
            htt_stats_buf->dhcp_packets);
    HTT_STATS_PRINT(FATAL, "host_inspected = %u",
            htt_stats_buf->host_inspected);
    HTT_STATS_PRINT(FATAL, "htt_included = %u",
            htt_stats_buf->htt_included);
    HTT_STATS_PRINT(FATAL, "htt_valid_mcs = %u",
            htt_stats_buf->htt_valid_mcs);
    HTT_STATS_PRINT(FATAL, "htt_valid_nss = %u",
            htt_stats_buf->htt_valid_nss);
    HTT_STATS_PRINT(FATAL, "htt_valid_preamble_type = %u",
            htt_stats_buf->htt_valid_preamble_type);
    HTT_STATS_PRINT(FATAL, "htt_valid_chainmask = %u",
            htt_stats_buf->htt_valid_chainmask);
    HTT_STATS_PRINT(FATAL, "htt_valid_guard_interval = %u",
            htt_stats_buf->htt_valid_guard_interval);
    HTT_STATS_PRINT(FATAL, "htt_valid_retries = %u",
            htt_stats_buf->htt_valid_retries);
    HTT_STATS_PRINT(FATAL, "htt_valid_bw_info = %u",
            htt_stats_buf->htt_valid_bw_info);
    HTT_STATS_PRINT(FATAL, "htt_valid_power = %u",
            htt_stats_buf->htt_valid_power);
    HTT_STATS_PRINT(FATAL, "htt_valid_key_flags = 0x%x",
            htt_stats_buf->htt_valid_key_flags);
    HTT_STATS_PRINT(FATAL, "htt_valid_no_encryption = %u",
            htt_stats_buf->htt_valid_no_encryption);
    HTT_STATS_PRINT(FATAL, "fse_entry_count = %u",
            htt_stats_buf->fse_entry_count);
    HTT_STATS_PRINT(FATAL, "fse_priority_be = %u",
            htt_stats_buf->fse_priority_be);
    HTT_STATS_PRINT(FATAL, "fse_priority_high = %u",
            htt_stats_buf->fse_priority_high);
    HTT_STATS_PRINT(FATAL, "fse_priority_low = %u",
            htt_stats_buf->fse_priority_low);
    HTT_STATS_PRINT(FATAL, "fse_traffic_ptrn_be = %u",
            htt_stats_buf->fse_traffic_ptrn_be);
    HTT_STATS_PRINT(FATAL, "fse_traffic_ptrn_over_sub = %u",
            htt_stats_buf->fse_traffic_ptrn_over_sub);
    HTT_STATS_PRINT(FATAL, "fse_traffic_ptrn_bursty = %u",
            htt_stats_buf->fse_traffic_ptrn_bursty);
    HTT_STATS_PRINT(FATAL, "fse_traffic_ptrn_interactive = %u",
            htt_stats_buf->fse_traffic_ptrn_interactive);
    HTT_STATS_PRINT(FATAL, "fse_traffic_ptrn_periodic = %u",
            htt_stats_buf->fse_traffic_ptrn_periodic);
    HTT_STATS_PRINT(FATAL, "fse_hwqueue_alloc = %u",
            htt_stats_buf->fse_hwqueue_alloc);
    HTT_STATS_PRINT(FATAL, "fse_hwqueue_created = %u",
            htt_stats_buf->fse_hwqueue_created);
    HTT_STATS_PRINT(FATAL, "fse_hwqueue_send_to_host = %u",
            htt_stats_buf->fse_hwqueue_send_to_host);
    HTT_STATS_PRINT(FATAL, "mcast_entry = %u",
            htt_stats_buf->mcast_entry);
    HTT_STATS_PRINT(FATAL, "bcast_entry = %u",
            htt_stats_buf->bcast_entry);
    HTT_STATS_PRINT(FATAL, "htt_update_peer_cache = %u",
            htt_stats_buf->htt_update_peer_cache);
    HTT_STATS_PRINT(FATAL, "htt_learning_frame = %u",
            htt_stats_buf->htt_learning_frame);
    HTT_STATS_PRINT(FATAL, "fse_invalid_peer = %u",
            htt_stats_buf->fse_invalid_peer);
    HTT_STATS_PRINT(FATAL, "mec_notify = %u\n",
            htt_stats_buf->mec_notify);
}

/*
 * htt_print_tx_de_classify_status_stats_tlv: display
 *                htt_tx_de_classify_status_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_de_classify_status_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_de_classify_status_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_de_classify_status_stats_tlv *htt_stats_buf =
        (htt_tx_de_classify_status_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_DE_CLASSIFY_STATUS_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "eok = %u",
            htt_stats_buf->eok);
    HTT_STATS_PRINT(FATAL, "classify_done = %u",
            htt_stats_buf->classify_done);
    HTT_STATS_PRINT(FATAL, "lookup_failed = %u",
            htt_stats_buf->lookup_failed);
    HTT_STATS_PRINT(FATAL, "send_host_dhcp = %u",
            htt_stats_buf->send_host_dhcp);
    HTT_STATS_PRINT(FATAL, "send_host_mcast = %u",
            htt_stats_buf->send_host_mcast);
    HTT_STATS_PRINT(FATAL, "send_host_unknown_dest = %u",
            htt_stats_buf->send_host_unknown_dest);
    HTT_STATS_PRINT(FATAL, "send_host = %u",
            htt_stats_buf->send_host);
    HTT_STATS_PRINT(FATAL, "status_invalid = %u\n",
            htt_stats_buf->status_invalid);
}

/*
 * htt_print_tx_de_enqueue_packets_stats_tlv: display
 *                htt_tx_de_enqueue_packets_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_de_enqueue_packets_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_de_enqueue_packets_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_de_enqueue_packets_stats_tlv *htt_stats_buf =
        (htt_tx_de_enqueue_packets_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_DE_ENQUEUE_PACKETS_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "enqueued_pkts = %u",
            htt_stats_buf->enqueued_pkts);
    HTT_STATS_PRINT(FATAL, "to_tqm = %u",
            htt_stats_buf->to_tqm);
    HTT_STATS_PRINT(FATAL, "to_tqm_bypass = %u\n",
            htt_stats_buf->to_tqm_bypass);
}

/*
 * htt_print_tx_de_enqueue_discard_stats_tlv: display
 *                    htt_tx_de_enqueue_discard_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_de_enqueue_discard_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_de_enqueue_discard_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_de_enqueue_discard_stats_tlv *htt_stats_buf =
        (htt_tx_de_enqueue_discard_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_DE_ENQUEUE_DISCARD_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "discarded_pkts = %u",
            htt_stats_buf->discarded_pkts);
    HTT_STATS_PRINT(FATAL, "local_frames = %u",
            htt_stats_buf->local_frames);
    HTT_STATS_PRINT(FATAL, "is_ext_msdu = %u",
            htt_stats_buf->is_ext_msdu);
    HTT_STATS_PRINT(FATAL, "mlo_invalid_routing_discard = %u",
            htt_stats_buf->mlo_invalid_routing_discard);
    HTT_STATS_PRINT(FATAL, "mlo_invalid_routing_dup_entry_discard = %u",
            htt_stats_buf->mlo_invalid_routing_dup_entry_discard);
    HTT_STATS_PRINT(FATAL, "discard_peer_unauthorized_pkts = %u\n",
            htt_stats_buf->discard_peer_unauthorized_pkts);
}

/*
 * htt_print_tx_de_compl_stats_tlv: display htt_tx_de_compl_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_de_compl_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_de_compl_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_de_compl_stats_tlv *htt_stats_buf =
        (htt_tx_de_compl_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_DE_COMPL_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "tcl_dummy_frame = %u",
            htt_stats_buf->tcl_dummy_frame);
    HTT_STATS_PRINT(FATAL, "tqm_dummy_frame = %u",
            htt_stats_buf->tqm_dummy_frame);
    HTT_STATS_PRINT(FATAL, "tqm_notify_frame = %u",
            htt_stats_buf->tqm_notify_frame);
    HTT_STATS_PRINT(FATAL, "fw2wbm_enq = %u",
            htt_stats_buf->fw2wbm_enq);
    HTT_STATS_PRINT(FATAL, "tqm_bypass_frame = %u\n",
            htt_stats_buf->tqm_bypass_frame);
}

/*
 * htt_print_tx_de_fw2wbm_ring_full_hist_tlv: display htt_tx_de_fw2wbm_ring_full_hist_tlv
 * @tag_buf: buffer containing the tlv  htt_tx_de_fw2wbm_ring_full_hist_tlv
 *
 * return:void
 */
static void htt_print_tx_de_fw2wbm_ring_full_hist_tlv(A_UINT32 *tag_buf)
{
    htt_tx_de_fw2wbm_ring_full_hist_tlv *htt_stats_buf =
        (htt_tx_de_fw2wbm_ring_full_hist_tlv *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                     = 0;
    A_CHAR   fw2wbm_ring_full_hist[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 num_elements                              = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);
    A_UINT32 required_buffer_size                      = HTT_MAX_PRINT_CHAR_PER_ELEM * num_elements;

    HTT_STATS_PRINT(FATAL, "HTT_TX_DE_FW2WBM_RING_FULL_HIST_TLV");

    if (required_buffer_size < HTT_MAX_STRING_LEN) {
        for (i = 0; i < num_elements; i++) {
            index += snprintf(&fw2wbm_ring_full_hist[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->fw2wbm_ring_full_hist[i]);
        }

        HTT_STATS_PRINT(FATAL, "fw2wbm_ring_full_hist = %s\n", fw2wbm_ring_full_hist);
    } else {
        HTT_STATS_PRINT(FATAL, "INSUFFICIENT PRINT BUFFER ");
    }
}

/*
 * htt_print_tx_de_cmn_stats_tlv: display htt_tx_de_cmn_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_de_cmn_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_de_cmn_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_de_cmn_stats_tlv *htt_stats_buf =
        (htt_tx_de_cmn_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_TX_DE_CMN_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__word & 0xFF);
    /*HTT_STATS_PRINT(FATAL, "word = %u",
            ((htt_stats_buf->mac_id__word & 0xFFFFFF00) >> 8 ));*/
    HTT_STATS_PRINT(FATAL, "tcl2fw_entry_count = %u",
            htt_stats_buf->tcl2fw_entry_count);
    HTT_STATS_PRINT(FATAL, "not_to_fw = %u",
            htt_stats_buf->not_to_fw);
    HTT_STATS_PRINT(FATAL, "invalid_pdev_vdev_peer = %u",
            htt_stats_buf->invalid_pdev_vdev_peer);
    HTT_STATS_PRINT(FATAL, "tcl_res_invalid_addrx = %u",
            htt_stats_buf->tcl_res_invalid_addrx);
    HTT_STATS_PRINT(FATAL, "wbm2fw_entry_count = %u",
            htt_stats_buf->wbm2fw_entry_count);
    HTT_STATS_PRINT(FATAL, "invalid_pdev = %u",
            htt_stats_buf->invalid_pdev);
    HTT_STATS_PRINT(FATAL, "tcl_res_addrx_timeout = %u",
            htt_stats_buf->tcl_res_addrx_timeout);
    HTT_STATS_PRINT(FATAL, "invalid_vdev = %u",
            htt_stats_buf->invalid_vdev);
    HTT_STATS_PRINT(FATAL, "invalid_tcl_exp_frame_desc = %u\n",
            htt_stats_buf->invalid_tcl_exp_frame_desc);
    HTT_STATS_PRINT(FATAL, "vdev_id_mismatch_count = %u\n",
            htt_stats_buf->vdev_id_mismatch_cnt);
}

/*
 * htt_print_ring_if_stats_tlv: display htt_ring_if_stats_tlv
 * @tag_buf: buffer containing the tlv htt_ring_if_stats_tlv
 *
 * return:void
 */
static void htt_print_ring_if_stats_tlv(A_UINT32 *tag_buf)
{
    htt_ring_if_stats_tlv *htt_stats_buf =
        (htt_ring_if_stats_tlv *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                 = 0;
    A_CHAR   *low_wm_hit_count = NULL;
    A_CHAR   *high_wm_hit_count = NULL;

    low_wm_hit_count = (A_CHAR *)malloc(HTT_MAX_STRING_LEN * sizeof(A_CHAR));
    if (!low_wm_hit_count) {
        HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
        return;
    }

    high_wm_hit_count = (A_CHAR *)malloc(HTT_MAX_STRING_LEN * sizeof(A_CHAR));
    if (!high_wm_hit_count) {
        HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
        free(low_wm_hit_count);
        return;
    }

    HTT_STATS_PRINT(FATAL, "HTT_RING_IF_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "base_addr = %u",
            htt_stats_buf->base_addr);
    HTT_STATS_PRINT(FATAL, "elem_size = %u",
            htt_stats_buf->elem_size);
    HTT_STATS_PRINT(FATAL, "num_elems = %u",
            htt_stats_buf->num_elems__prefetch_tail_idx & 0xFFFF);
    HTT_STATS_PRINT(FATAL, "prefetch_tail_idx = %u",
            (htt_stats_buf->num_elems__prefetch_tail_idx & 0xFFFF0000) >> 16);
    HTT_STATS_PRINT(FATAL, "head_idx = %u",
            htt_stats_buf->head_idx__tail_idx & 0xFFFF);
    HTT_STATS_PRINT(FATAL, "tail_idx = %u",
            (htt_stats_buf->head_idx__tail_idx & 0xFFFF0000) >> 16);
    HTT_STATS_PRINT(FATAL, "shadow_head_idx = %u",
            htt_stats_buf->shadow_head_idx__shadow_tail_idx & 0xFFFF);
    HTT_STATS_PRINT(FATAL, "shadow_tail_idx = %u",
            (htt_stats_buf->shadow_head_idx__shadow_tail_idx & 0xFFFF0000) >> 16);
    HTT_STATS_PRINT(FATAL, "num_tail_incr = %u",
            htt_stats_buf->num_tail_incr);
    HTT_STATS_PRINT(FATAL, "lwm_thresh = %u",
            htt_stats_buf->lwm_thresh__hwm_thresh & 0xFFFF);
    HTT_STATS_PRINT(FATAL, "hwm_thresh = %u",
            (htt_stats_buf->lwm_thresh__hwm_thresh & 0xFFFF0000) >> 16);
    HTT_STATS_PRINT(FATAL, "overrun_hit_count = %u",
            htt_stats_buf->overrun_hit_count);
    HTT_STATS_PRINT(FATAL, "underrun_hit_count = %u",
            htt_stats_buf->underrun_hit_count);
    HTT_STATS_PRINT(FATAL, "prod_blockwait_count = %u",
            htt_stats_buf->prod_blockwait_count);
    HTT_STATS_PRINT(FATAL, "cons_blockwait_count = %u",
            htt_stats_buf->cons_blockwait_count);

    for (i = 0; i < HTT_STATS_LOW_WM_BINS; i++) {
        index += snprintf(&low_wm_hit_count[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->low_wm_hit_count[i]);
    }

    HTT_STATS_PRINT(FATAL, "low_wm_hit_count = %s ", low_wm_hit_count);

    index = 0;

    for (i = 0; i < HTT_STATS_HIGH_WM_BINS; i++) {
        index += snprintf(&high_wm_hit_count[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->high_wm_hit_count[i]);
    }

    HTT_STATS_PRINT(FATAL, "high_wm_hit_count = %s\n", high_wm_hit_count);

    free(low_wm_hit_count);
    free(high_wm_hit_count);
}

/*
 * htt_print_ring_if_cmn_tlv: display htt_ring_if_cmn_tlv
 * @tag_buf: buffer containing the tlv htt_ring_if_cmn_tlv
 *
 * return:void
 */
static void htt_print_ring_if_cmn_tlv(A_UINT32 *tag_buf)
{
    htt_ring_if_cmn_tlv *htt_stats_buf =
        (htt_ring_if_cmn_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_RING_IF_CMN_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__word & 0xFF);
    /*HTT_STATS_PRINT(FATAL, "word = %u",
            ((htt_stats_buf->mac_id__word & 0xFFFFFF00) >> 8 ));*/
    HTT_STATS_PRINT(FATAL, "num_records = %u\n",
            htt_stats_buf->num_records);
}

/*
 * htt_print_sfm_client_user_tlv_v: display htt_sfm_client_user_tlv_v
 * @tag_buf: buffer containing the tlv htt_sfm_client_user_tlv_v
 *
 * return:void
 */
static void htt_print_sfm_client_user_tlv_v(A_UINT32 *tag_buf)
{
    htt_sfm_client_user_tlv_v *htt_stats_buf =
        (htt_sfm_client_user_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                     = 0;
    A_CHAR   dwords_used_by_user_n[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                                   = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_SFM_CLIENT_USER_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&dwords_used_by_user_n[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->dwords_used_by_user_n[i]);
    }

    HTT_STATS_PRINT(FATAL, "dwords_used_by_user_n = %s\n",
            dwords_used_by_user_n);
}

/*
 * htt_print_sfm_client_tlv: display htt_sfm_client_tlv
 * @tag_buf: buffer containing the tlv htt_sfm_client_tlv
 *
 * return:void
 */
static void htt_print_sfm_client_tlv(A_UINT32 *tag_buf)
{
    htt_sfm_client_tlv *htt_stats_buf =
        (htt_sfm_client_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_SFM_CLIENT_TLV:");
    HTT_STATS_PRINT(FATAL, "client_id = %u",
            htt_stats_buf->client_id);
    HTT_STATS_PRINT(FATAL, "buf_min = %u",
            htt_stats_buf->buf_min);
    HTT_STATS_PRINT(FATAL, "buf_max = %u",
            htt_stats_buf->buf_max);
    HTT_STATS_PRINT(FATAL, "buf_busy = %u",
            htt_stats_buf->buf_busy);
    HTT_STATS_PRINT(FATAL, "buf_alloc = %u",
            htt_stats_buf->buf_alloc);
    HTT_STATS_PRINT(FATAL, "buf_avail = %u",
            htt_stats_buf->buf_avail);
    HTT_STATS_PRINT(FATAL, "num_users = %u\n",
            htt_stats_buf->num_users);
}

/*
 * htt_print_sfm_cmn_tlv: display htt_sfm_cmn_tlv
 * @tag_buf: buffer containing the tlv htt_sfm_cmn_tlv
 *
 * return:void
 */
static void htt_print_sfm_cmn_tlv(A_UINT32 *tag_buf)
{
    htt_sfm_cmn_tlv *htt_stats_buf =
        (htt_sfm_cmn_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_SFM_CMN_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__word & 0xFF);
    /*HTT_STATS_PRINT(FATAL, "word = %u",
            ((htt_stats_buf->mac_id__word & 0xFFFFFF00) >> 8 ));*/
    HTT_STATS_PRINT(FATAL, "buf_total = %u",
            htt_stats_buf->buf_total);
    HTT_STATS_PRINT(FATAL, "mem_empty = %u",
            htt_stats_buf->mem_empty);
    HTT_STATS_PRINT(FATAL, "deallocate_bufs = %u",
            htt_stats_buf->deallocate_bufs);
    HTT_STATS_PRINT(FATAL, "num_records = %u\n",
            htt_stats_buf->num_records);
}

/*
 * htt_print_sring_stats_tlv: display htt_sring_stats_tlv
 * @tag_buf: buffer containing the tlv htt_sring_stats_tlv
 *
 * return:void
 */
static void htt_print_sring_stats_tlv(A_UINT32 *tag_buf)
{
    htt_sring_stats_tlv *htt_stats_buf =
        (htt_sring_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_SRING_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__ring_id__arena__ep & 0xFF);
    HTT_STATS_PRINT(FATAL, "ring_id = %u",
            (htt_stats_buf->mac_id__ring_id__arena__ep & 0xFF00) >> 8);
    HTT_STATS_PRINT(FATAL, "arena = %u",
            (htt_stats_buf->mac_id__ring_id__arena__ep & 0xFF0000) >> 16);
    HTT_STATS_PRINT(FATAL, "ep = %u",
            (htt_stats_buf->mac_id__ring_id__arena__ep & 0x1000000) >> 24);
    HTT_STATS_PRINT(FATAL, "base_addr_lsb = 0x%x",
            htt_stats_buf->base_addr_lsb);
    HTT_STATS_PRINT(FATAL, "base_addr_msb = 0x%x",
            htt_stats_buf->base_addr_msb);
    HTT_STATS_PRINT(FATAL, "ring_size = %u",
            htt_stats_buf->ring_size);
    HTT_STATS_PRINT(FATAL, "elem_size = %u",
            htt_stats_buf->elem_size);
    HTT_STATS_PRINT(FATAL, "num_avail_words = %u",
            htt_stats_buf->num_avail_words__num_valid_words & 0xFFFF);
    HTT_STATS_PRINT(FATAL, "num_valid_words = %u",
            (htt_stats_buf->num_avail_words__num_valid_words & 0xFFFF0000) >> 16);
    HTT_STATS_PRINT(FATAL, "head_ptr = %u",
            htt_stats_buf->head_ptr__tail_ptr & 0xFFFF);
    HTT_STATS_PRINT(FATAL, "tail_ptr = %u",
            (htt_stats_buf->head_ptr__tail_ptr & 0xFFFF0000) >> 16);
    HTT_STATS_PRINT(FATAL, "consumer_empty = %u",
            htt_stats_buf->consumer_empty__producer_full & 0xFFFF);
    HTT_STATS_PRINT(FATAL, "producer_full = %u",
            (htt_stats_buf->consumer_empty__producer_full & 0xFFFF0000) >> 16);
    HTT_STATS_PRINT(FATAL, "prefetch_count = %u",
            htt_stats_buf->prefetch_count__internal_tail_ptr & 0xFFFF);
    HTT_STATS_PRINT(FATAL, "internal_tail_ptr = %u\n",
            (htt_stats_buf->prefetch_count__internal_tail_ptr & 0xFFFF0000) >> 16);
}

/*
 * htt_print_sring_cmn_tlv: display htt_sring_cmn_tlv
 * @tag_buf: buffer containing the tlv htt_sring_cmn_tlv
 *
 * return:void
 */
static void htt_print_sring_cmn_tlv(A_UINT32 *tag_buf)
{
    htt_sring_cmn_tlv *htt_stats_buf =
        (htt_sring_cmn_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_SRING_CMN_TLV:");
    HTT_STATS_PRINT(FATAL, "num_records = %u\n",
            htt_stats_buf->num_records);
}

/*
 * htt_print_tx_pdev_rate_stats_tlv: display htt_tx_pdev_rate_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_pdev_rate_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_pdev_rate_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_rate_stats_tlv *htt_stats_buf =
        (htt_tx_pdev_rate_stats_tlv *)tag_buf;
    A_UINT8  i, j;
    A_UINT16 index                       = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_CHAR   *tx_gi[HTT_TX_PEER_STATS_NUM_GI_COUNTERS];

    for (i = 0; i < HTT_TX_PEER_STATS_NUM_GI_COUNTERS; i++) {
        tx_gi[i] = (A_CHAR *)malloc(HTT_MAX_STRING_LEN);
        if (!tx_gi[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(tx_gi[j]);
           }
           return;
        }
    }

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_RATE_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__word & 0xFF);
    /*HTT_STATS_PRINT(FATAL, "word = %u",
            ((htt_stats_buf->mac_id__word & 0xFFFFFF00) >> 8 ));*/
    HTT_STATS_PRINT(FATAL, "tx_ldpc = %u",
            htt_stats_buf->tx_ldpc);
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_tx_ldpc = %u",
            htt_stats_buf->ac_mu_mimo_tx_ldpc);
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_tx_ldpc = %u",
            htt_stats_buf->ax_mu_mimo_tx_ldpc);
    HTT_STATS_PRINT(FATAL, "ofdma_tx_ldpc = %u",
            htt_stats_buf->ofdma_tx_ldpc);
    HTT_STATS_PRINT(FATAL, "rts_cnt = %u",
            htt_stats_buf->rts_cnt);
    HTT_STATS_PRINT(FATAL, "rts_success = %u",
            htt_stats_buf->rts_success);
    HTT_STATS_PRINT(FATAL, "ack_rssi = %u",
            htt_stats_buf->ack_rssi);
    HTT_STATS_PRINT(FATAL, "tx_11ax_su_ext = %u",
            htt_stats_buf->tx_11ax_su_ext);

    HTT_STATS_PRINT(FATAL, "Legacy CCK Rates: 1 Mbps: %u, 2 Mbps: %u, 5.5 Mbps: %u, 11 Mbps: %u",
            htt_stats_buf->tx_legacy_cck_rate[0], htt_stats_buf->tx_legacy_cck_rate[1],
            htt_stats_buf->tx_legacy_cck_rate[2], htt_stats_buf->tx_legacy_cck_rate[3]);

    HTT_STATS_PRINT(FATAL, "Legacy OFDM Rates: 6 Mbps: %u, 9 Mbps: %u, 12 Mbps: %u, 18 Mbps: %u\n"
                           "                   24 Mbps: %u, 36 Mbps: %u, 48 Mbps: %u, 54 Mbps: %u",
            htt_stats_buf->tx_legacy_ofdm_rate[0], htt_stats_buf->tx_legacy_ofdm_rate[1],
            htt_stats_buf->tx_legacy_ofdm_rate[2], htt_stats_buf->tx_legacy_ofdm_rate[3],
            htt_stats_buf->tx_legacy_ofdm_rate[4], htt_stats_buf->tx_legacy_ofdm_rate[5],
            htt_stats_buf->tx_legacy_ofdm_rate[6], htt_stats_buf->tx_legacy_ofdm_rate[7]);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " -2:%u,-1:%u,",
                htt_stats_buf->tx_mcs_ext_2[0],
                htt_stats_buf->tx_mcs_ext_2[1]);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_mcs[i]);
    }
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS,
                htt_stats_buf->tx_mcs_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "tx_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->ac_mu_mimo_tx_mcs[i]);
    }
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS,
                htt_stats_buf->ax_mu_mimo_tx_mcs_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_tx_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->ax_mu_mimo_tx_mcs[i]);
    }
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS,
                htt_stats_buf->ax_mu_mimo_tx_mcs_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_tx_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->ofdma_tx_mcs[i]);
    }
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS,
                htt_stats_buf->ofdma_tx_mcs_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "ofdma_tx_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_HE_SIG_B_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->ofdma_he_sig_b_mcs[i]);
    }
    HTT_STATS_PRINT(FATAL, "ofdma_he_sig_b_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->tx_nss[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ac_mu_mimo_tx_nss[i]);
    }

    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_tx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ax_mu_mimo_tx_nss[i]);
    }

    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_tx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ofdma_tx_nss[i]);
    }

    HTT_STATS_PRINT(FATAL, "ofdma_tx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_bw[i]);
    }

    index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->tx_bw_320mhz);

    HTT_STATS_PRINT(FATAL, "tx_bw = %s ", str_buf);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_REDUCED_CHAN_TYPES; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (j = 0; j < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_stats_buf->reduced_tx_bw[i][j]);
        }

        HTT_STATS_PRINT(FATAL, "%s = %s ", i == 0 ? "half_tx_bw" : "quarter_tx_bw", str_buf);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_PUNCTURED_MODE_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_su_punctured_mode[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_su_punctured_mode = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->ac_mu_mimo_tx_bw[i]);
    }

    HTT_STATS_PRINT(FATAL, "ac_mu_mimo_tx_bw = %s ", str_buf);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_REDUCED_CHAN_TYPES; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (j = 0; j < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_stats_buf->reduced_ac_mu_mimo_tx_bw[i][j]);
        }

        HTT_STATS_PRINT(FATAL, "%s = %s ", i == 0 ? "half_ac_mu_mimo_tx_bw" : "quarter_ac_mu_mimo_tx_bw", str_buf);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->ax_mu_mimo_tx_bw[i]);
    }

    HTT_STATS_PRINT(FATAL, "ax_mu_mimo_tx_bw = %s ", str_buf);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_REDUCED_CHAN_TYPES; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (j = 0; j < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_stats_buf->reduced_ax_mu_mimo_tx_bw[i][j]);
        }

        HTT_STATS_PRINT(FATAL, "%s = %s ", i == 0 ? "half_ax_mu_mimo_tx_bw" : "quarter_ax_mu_mimo_tx_bw", str_buf);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->ofdma_tx_bw[i]);
    }

    HTT_STATS_PRINT(FATAL, "ofdma_tx_bw = %s ", str_buf);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_REDUCED_CHAN_TYPES; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (j = 0; j < HTT_TX_PDEV_STATS_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_stats_buf->reduced_ax_mu_ofdma_tx_bw[i][j]);
        }

        HTT_STATS_PRINT(FATAL, "%s = %s ", i == 0 ? "half_ofdma_tx_bw" : "quarter_ofdma_tx_bw", str_buf);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_stbc[i]);
    }
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS,
                htt_stats_buf->tx_stbc_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "tx_stbc = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_PREAMBLE_TYPES; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_pream[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_pream = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "HE LTF: 1x: %u, 2x: %u, 4x: %u",
        htt_stats_buf->tx_he_ltf[1],
        htt_stats_buf->tx_he_ltf[2], htt_stats_buf->tx_he_ltf[3]);
    HTT_STATS_PRINT(FATAL, "Extra LTF: %u", htt_stats_buf->extra_eht_ltf);
    HTT_STATS_PRINT(FATAL, "Extra LTF - OFDMA: %u",
        htt_stats_buf->extra_eht_ltf_ofdma);

    /* SU GI Stats */
    for (j = 0; j < HTT_TX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;

        index += snprintf(&tx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " -2:%u,-1:%u,",
                    htt_stats_buf->tx_gi_ext_2[j][0],
                    htt_stats_buf->tx_gi_ext_2[j][1]);

        for (i = 0; i < HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
            index += snprintf(&tx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->tx_gi[j][i]);
        }
        for (i = 0; i < HTT_TX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
            index += snprintf(&tx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i + HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS,
                    htt_stats_buf->tx_gi_ext[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "tx_gi[%u] = %s ", j, tx_gi[j]);
    }

    /* AC MU-MIMO GI Stats */
    for (j = 0; j < HTT_TX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;

        for (i = 0; i < HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
            index += snprintf(&tx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->ac_mu_mimo_tx_gi[j][i]);
        }

        HTT_STATS_PRINT(FATAL, "ac_mu_mimo_tx_gi[%u] = %s ", j, tx_gi[j]);
    }

    /* AX MU-MIMO GI Stats */
    for (j = 0; j < HTT_TX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        for (i = 0; i < HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
            index += snprintf(&tx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->ax_mu_mimo_tx_gi[j][i]);
        }
        for (i = 0; i < HTT_TX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
            index += snprintf(&tx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i + HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS,
                    htt_stats_buf->ax_mu_mimo_tx_gi_ext[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "ax_mu_mimo_tx_gi[%u] = %s ", j, tx_gi[j]);
    }

    /* DL OFDMA GI Stats */
    for (j = 0; j < HTT_TX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        for (i = 0; i < HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
            index += snprintf(&tx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->ofdma_tx_gi[j][i]);
        }
        for (i = 0; i < HTT_TX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
            index += snprintf(&tx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i + HTT_TX_PDEV_STATS_NUM_MCS_COUNTERS,
                    htt_stats_buf->ofdma_tx_gi_ext[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "ofdma_tx_gi[%u] = %s ", j, tx_gi[j]);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_RU_SIZE_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %s:%u,", htt_ax_tx_ru_size_to_str(i),
                htt_stats_buf->ofdma_tx_ru_size[i]);
    }
    HTT_STATS_PRINT(FATAL, "ofdma_tx_ru_size = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_RU_SIZE_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %s:%u,", htt_ax_tx_ru_size_to_str(i),
                htt_stats_buf->ofdma_ba_ru_size[i]);
    }
    HTT_STATS_PRINT(FATAL, "ofdma_ba_ru_size = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_DCM_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->tx_dcm[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_dcm = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_11AX_TRIGGER_TYPES; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->trigger_type_11ax[i]);
    }

    HTT_STATS_PRINT(FATAL, "11ax_trigger_type = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_11BE_TRIGGER_TYPES; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->trigger_type_11be[i]);
    }

    HTT_STATS_PRINT(FATAL, "11be_trigger_type = %s", str_buf);

    HTT_STATS_PRINT(FATAL, "ax_su_embedded_trigger_data_ppdu_cnt = %u",
        htt_stats_buf->ax_su_embedded_trigger_data_ppdu);
    HTT_STATS_PRINT(FATAL, "ax_su_embedded_trigger_data_ppdu_err_cnt = %u\n",
        htt_stats_buf->ax_su_embedded_trigger_data_ppdu_err);

    for (i = 0; i < HTT_TX_PEER_STATS_NUM_GI_COUNTERS; i++) {
        free(tx_gi[i]);
    }
}

static void htt_print_tx_pdev_sawf_rate_stats_tlv(A_UINT32 *tag_buf)
{
    A_UINT16 index = 0;
    A_UINT8 i = 0;
    A_CHAR str_buf[HTT_MAX_STRING_LEN] = {0};
    htt_tx_pdev_rate_stats_sawf_tlv *htt_stats_buf =
        (htt_tx_pdev_rate_stats_sawf_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "su_burst_rate_drop_fail_cnt = %u",
        htt_stats_buf->su_burst_rate_drop_fail_cnt);

    HTT_STATS_PRINT(FATAL, "su_burst_rate_drop_cnt = %u",
        htt_stats_buf->su_burst_rate_drop_cnt);

    HTT_STATS_PRINT(FATAL, "rate_retry_mcs_drop_cnt = %u",
        htt_stats_buf->rate_retry_mcs_drop_cnt);

    HTT_STATS_PRINT(FATAL, "low_latency_rate_cnt = %u",
        htt_stats_buf->low_latency_rate_cnt);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    /* MCS -2 and -1 will be printed first */
    index = snprintf(&str_buf[index],
        HTT_MAX_STRING_LEN - index,
        " -2:%u,-1:%u,",
        htt_stats_buf->mcs_drop_rate[HTT_TX_PDEV_STATS_NUM_MCS_DROP_COUNTERS-2],
        htt_stats_buf->mcs_drop_rate[HTT_TX_PDEV_STATS_NUM_MCS_DROP_COUNTERS-1]);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_MCS_DROP_COUNTERS-2; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->mcs_drop_rate[i]);
    }
    HTT_STATS_PRINT(FATAL, "mcs_drop_rate = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < 51; i++) {
        index += snprintf(&str_buf[index],
        HTT_MAX_STRING_LEN - index,
        " %u:%u,", i, htt_stats_buf->per_histogram_cnt[i]);
    }
    HTT_STATS_PRINT(FATAL, "per_histogram_cnt = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 51; i < HTT_TX_PDEV_STATS_NUM_PER_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
        HTT_MAX_STRING_LEN - index,
        " %u:%u,", i, htt_stats_buf->per_histogram_cnt[i]);
    }
    HTT_STATS_PRINT(FATAL, " %s\n", str_buf);
}

/*
 * htt_print_tx_pdev_be_rate_stats_tlv: display htt_tx_pdev_be_rate_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_pdev_be_rate_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_pdev_be_rate_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_rate_stats_be_tlv *htt_stats_buf =
        (htt_tx_pdev_rate_stats_be_tlv *)tag_buf;

    A_UINT8  i, j;
    A_UINT16 index                       = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_CHAR   *tx_gi[HTT_TX_PEER_STATS_NUM_GI_COUNTERS];

    for (i = 0; i < HTT_TX_PEER_STATS_NUM_GI_COUNTERS; i++) {
        tx_gi[i] = (A_CHAR *)malloc(HTT_MAX_STRING_LEN);
        if (!tx_gi[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(tx_gi[j]);
           }
           return;
        }
    }

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_BE_RATE_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_tx_ldpc = %u",
            htt_stats_buf->be_mu_mimo_tx_ldpc);

    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    /* MCS -2 and -1 will be printed first */
    index = snprintf(&str_buf[index],
        HTT_MAX_STRING_LEN - index,
        " -2:%u,-1:%u,",
        htt_stats_buf->be_mu_mimo_tx_mcs[HTT_TX_PDEV_STATS_NUM_BE_MCS_COUNTERS-2],
        htt_stats_buf->be_mu_mimo_tx_mcs[HTT_TX_PDEV_STATS_NUM_BE_MCS_COUNTERS-1]);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MCS_COUNTERS-2; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->be_mu_mimo_tx_mcs[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_tx_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_mu_mimo_tx_nss[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_tx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->be_mu_mimo_tx_bw[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_mu_mimo_tx_bw = %s ", str_buf);

    /* BE MU-MIMO GI Stats */
    for (j = 0; j < HTT_TX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MCS_COUNTERS; i++) {
            index += snprintf(&tx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->be_mu_mimo_tx_gi[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "be_mu_mimo_tx_gi[%u] = %s ", j, tx_gi[j]);
    }

    for (i = 0; i < HTT_TX_PEER_STATS_NUM_GI_COUNTERS; i++) {
        free(tx_gi[i]);
    }
}

/*
 * htt_print_tx_pdev_ppdu_dur_stats_tlv : display htt_tx_pdev_ppdu_dur_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_pdev_ppdu_dur_stats_tlv
 *
 * return: void
 */
static void htt_print_tx_pdev_ppdu_dur_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_ppdu_dur_stats_tlv * htt_stats_buf =
        (htt_tx_pdev_ppdu_dur_stats_tlv *)tag_buf;
    A_UINT8  i;
    A_UINT16 index;
    A_CHAR str_buf[HTT_MAX_STRING_LEN];

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_PPDU_DUR_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "pdev_up_time_us_low = %u",
            htt_stats_buf->pdev_up_time_us_low);
    HTT_STATS_PRINT(FATAL, "pdev_up_time_us_high = %u",
            htt_stats_buf->pdev_up_time_us_high);
    HTT_STATS_PRINT(FATAL, "tx_success_time_us_low = %u",
            htt_stats_buf->tx_success_time_us_low);
    HTT_STATS_PRINT(FATAL, "tx_success_time_us_high = %u",
            htt_stats_buf->tx_success_time_us_high);
    HTT_STATS_PRINT(FATAL, "tx_fail_time_us_low = %u",
            htt_stats_buf->tx_fail_time_us_low);
    HTT_STATS_PRINT(FATAL, "tx_fail_time_us_high = %u",
            htt_stats_buf->tx_fail_time_us_high);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_PDEV_STATS_PPDU_DUR_HIST_BINS ; i++) {
        index += snprintf(&str_buf [index],
                HTT_MAX_STRING_LEN - index,
                " %u-%u : %u,",
                i * HTT_PDEV_STATS_PPDU_DUR_HIST_INTERVAL_US,
                (i + 1) * HTT_PDEV_STATS_PPDU_DUR_HIST_INTERVAL_US,
                htt_stats_buf->tx_ppdu_dur_hist[i]);
    }
    HTT_STATS_PRINT(FATAL, "tx_ppdu_dur_hist_us = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_PDEV_STATS_PPDU_DUR_HIST_BINS ; i++) {
        index += snprintf(&str_buf [index],
                HTT_MAX_STRING_LEN - index,
                " %u-%u : %u,",
                i * HTT_PDEV_STATS_PPDU_DUR_HIST_INTERVAL_US,
                (i + 1) * HTT_PDEV_STATS_PPDU_DUR_HIST_INTERVAL_US,
                htt_stats_buf->tx_ofdma_ppdu_dur_hist[i]);
    }

    HTT_STATS_PRINT(FATAL, "tx_ofdma_ppdu_dur_hist_us = %s\n", str_buf);
}

/*
 * htt_print_rx_pdev_ul_mumimo_trig_stats_tlv: display htt_rx_pdev_ul_mumimo_trig_stats_tlv
 * @tag_buf: buffer containing the tlv htt_rx_pdev_ul_mumimo_trig_stats_tlv
 *
 * return:void
 */
static void htt_print_ul_mumimo_trig_stats(A_UINT32 *tag_buf)
{
    htt_rx_pdev_ul_mumimo_trig_stats_tlv *htt_ul_mumimo_trig_stats_buf =
        (htt_rx_pdev_ul_mumimo_trig_stats_tlv *)tag_buf;

    A_CHAR str_buf[HTT_MAX_STRING_LEN];
    A_CHAR *rx_gi[HTT_RX_PDEV_STATS_NUM_GI_COUNTERS];
    A_UINT8 i, j;
    A_UINT16 index;

    for (i=0; i < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; i++) {
        rx_gi[i] = malloc(HTT_MAX_STRING_LEN);
        if (!rx_gi[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(rx_gi[j]);
           }
           return;
        }
    }

    HTT_STATS_PRINT(FATAL, "HTT_RX_PDEV_UL_MUMIMO_TRIG_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
        htt_ul_mumimo_trig_stats_buf->mac_id__word & 0xFF);

    HTT_STATS_PRINT(FATAL, "rx_11ax_ul_mumimo = %u",
            htt_ul_mumimo_trig_stats_buf->rx_11ax_ul_mumimo);

    HTT_CHECK_FOR_SPACE_ON_PRINT_BUFFER(
            HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS +
                HTT_RX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS,
            HTT_MAX_STRING_LEN);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_ul_mumimo_trig_stats_buf->ul_mumimo_rx_mcs[i]);
    }
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS,
                htt_ul_mumimo_trig_stats_buf->ul_mumimo_rx_mcs_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_mumimo_rx_mcs = %s ", str_buf);

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        memset(rx_gi[j], 0x0, HTT_MAX_STRING_LEN);
        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
            index += snprintf(&rx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i, htt_ul_mumimo_trig_stats_buf->ul_mumimo_rx_gi[j][i]);
        }
        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
            index += snprintf(&rx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i + HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS,
                    htt_ul_mumimo_trig_stats_buf->ul_mumimo_rx_gi_ext[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "ul_mumimo_rx_gi[%u] = %s ", j, rx_gi[j]);
    }

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; i++) {
        free(rx_gi[i]);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_ULMUMIMO_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_ul_mumimo_trig_stats_buf->ul_mumimo_rx_nss[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_mumimo_rx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_ul_mumimo_trig_stats_buf->ul_mumimo_rx_bw[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_mumimo_rx_bw = %s ", str_buf);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_REDUCED_CHAN_TYPES; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (j = 0; j < HTT_RX_PDEV_STATS_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_ul_mumimo_trig_stats_buf->reduced_ul_mumimo_rx_bw[i][j]);
        }

        HTT_STATS_PRINT(FATAL, "%s = %s ", i == 0 ? "half_ul_mumimo_rx_bw" : "quarter_ul_mumimo_rx_bw", str_buf);
    }

    HTT_STATS_PRINT(FATAL, "ul_mumimo_rx_stbc = %u",
            htt_ul_mumimo_trig_stats_buf->ul_mumimo_rx_stbc);
    HTT_STATS_PRINT(FATAL, "ul_mumimo_rx_ldpc = %u",
            htt_ul_mumimo_trig_stats_buf->ul_mumimo_rx_ldpc);

    for (i = 0; i < HTT_RX_PDEV_STATS_ULMUMIMO_NUM_SPATIAL_STREAMS; i++) {
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        index = 0;
        for (j = 0; j < HTT_RX_PDEV_STATS_TOTAL_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_ul_mumimo_trig_stats_buf->rx_ul_mumimo_chain_rssi_in_dbm[i][j]);
        }
        HTT_STATS_PRINT(FATAL,
                "rx_ul_mumimo_rssi_in_dbm: chain[%u] = %s ", i, str_buf);
    }

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_UL_MUMIMO_USER_STATS; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (j = 0; j <HTT_RX_PDEV_STATS_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%d,", j, htt_ul_mumimo_trig_stats_buf->rx_ul_mumimo_target_rssi[i][j]);
        }
        HTT_STATS_PRINT(FATAL, "rx_ul_mumimo_target_rssi: user[%u] = %s ", i, str_buf);
    }

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_UL_MUMIMO_USER_STATS; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (j = 0; j < HTT_RX_PDEV_STATS_ULMUMIMO_NUM_SPATIAL_STREAMS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%d,", j, htt_ul_mumimo_trig_stats_buf->rx_ul_mumimo_fd_rssi[i][j]);
        }
        HTT_STATS_PRINT(FATAL, "rx_ul_mumimo_fd_rssi: user[%u] = %s ", i, str_buf);
    }

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_UL_MUMIMO_USER_STATS; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (j = 0; j < HTT_RX_PDEV_STATS_ULMUMIMO_NUM_SPATIAL_STREAMS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%d,", j, htt_ul_mumimo_trig_stats_buf->rx_ulmumimo_pilot_evm_dB_mean[i][j]);
        }
        HTT_STATS_PRINT(FATAL, "rx_ulmumimo_pilot_evm_dB_mean: user [%u] = %s ", i, str_buf);
    }

    HTT_STATS_PRINT(FATAL, "ul_mumimo_basic_trigger_rx_qos_null_only = %u",
        htt_ul_mumimo_trig_stats_buf->ul_mumimo_basic_trigger_rx_qos_null_only);

    HTT_STATS_PRINT(FATAL, "\n");
}

/*
 * htt_print_ul_mimo_user_stats : display htt_rx_pdev_ul_mimo_user_stats_tlv
 * @tag_buf:buffer containing the tlv htt_rx_pdev_ul_mimo_user_stats_tlv
 * return:void
 */
static void htt_print_ul_mimo_user_stats(A_UINT32 *tag_buf)
{
    htt_rx_pdev_ul_mimo_user_stats_tlv *htt_ul_user_stats_buf =
        (htt_rx_pdev_ul_mimo_user_stats_tlv *)tag_buf;

    if (htt_ul_user_stats_buf->user_index < HTT_RX_PDEV_MAX_ULMUMIMO_NUM_USER) {
        if (htt_ul_user_stats_buf->user_index == 0) {
            HTT_STATS_PRINT(FATAL, "HTT_STATS_RX_PDEV_UL_MIMO_USER_STATS_TLV");
        }

        HTT_STATS_PRINT(FATAL, "rx_ulmumimo_non_data_ppdu_%u = %u ",
                htt_ul_user_stats_buf->user_index,
                htt_ul_user_stats_buf->rx_ulmumimo_non_data_ppdu);
        HTT_STATS_PRINT(FATAL, "rx_ulmumimo_data_ppdu_%u = %u ",
                htt_ul_user_stats_buf->user_index,
                htt_ul_user_stats_buf->rx_ulmumimo_data_ppdu);
        HTT_STATS_PRINT(FATAL, "rx_ulmumimo_mpdu_ok_%u = %u ",
                htt_ul_user_stats_buf->user_index,
                htt_ul_user_stats_buf->rx_ulmumimo_mpdu_ok);
        HTT_STATS_PRINT(FATAL, "rx_ulmumimo_mpdu_fail_%u = %u",
                htt_ul_user_stats_buf->user_index,
                htt_ul_user_stats_buf->rx_ulmumimo_mpdu_fail);
    }
}

/*
 * htt_print_be_ul_mimo_user_stats : display htt_rx_pdev_be_ul_mimo_user_stats_tlv
 * @tag_buf:buffer containing the tlv htt_rx_pdev_be_ul_mimo_user_stats_tlv
 * return:void
 */
static void htt_print_be_ul_mimo_user_stats(A_UINT32 *tag_buf)
{
    htt_rx_pdev_be_ul_mimo_user_stats_tlv *htt_ul_user_stats_buf =
        (htt_rx_pdev_be_ul_mimo_user_stats_tlv *)tag_buf;

    if (htt_ul_user_stats_buf->user_index < HTT_RX_PDEV_MAX_ULMUMIMO_NUM_USER) {
        if (htt_ul_user_stats_buf->user_index == 0) {
            HTT_STATS_PRINT(FATAL, "HTT_STATS_RX_PDEV_BE_UL_MIMO_USER_STATS_TLV");
        }

        HTT_STATS_PRINT(FATAL, "be_rx_ulmumimo_non_data_ppdu_%u = %u ",
                htt_ul_user_stats_buf->user_index,
                htt_ul_user_stats_buf->be_rx_ulmumimo_non_data_ppdu);
        HTT_STATS_PRINT(FATAL, "be_rx_ulmumimo_data_ppdu_%u = %u ",
                htt_ul_user_stats_buf->user_index,
                htt_ul_user_stats_buf->be_rx_ulmumimo_data_ppdu);
        HTT_STATS_PRINT(FATAL, "be_rx_ulmumimo_mpdu_ok_%u = %u ",
                htt_ul_user_stats_buf->user_index,
                htt_ul_user_stats_buf->be_rx_ulmumimo_mpdu_ok);
        HTT_STATS_PRINT(FATAL, "be_rx_ulmumimo_mpdu_fail_%u = %u",
                htt_ul_user_stats_buf->user_index,
                htt_ul_user_stats_buf->be_rx_ulmumimo_mpdu_fail);
    }
}

/*
 * htt_print_rx_pdev_ul_mumimo_trig_be_stats_tlv: display htt_rx_pdev_ul_mumimo_trig_be_stats_tlv
 * @tag_buf: buffer containing the tlv htt_rx_pdev_ul_mumimo_trig_be_stats_tlv
 *
 * return:void
 */
static void htt_print_ul_mumimo_trig_be_stats(A_UINT32 *tag_buf)
{
    htt_rx_pdev_ul_mumimo_trig_be_stats_tlv *htt_ul_mumimo_trig_be_stats_buf =
        (htt_rx_pdev_ul_mumimo_trig_be_stats_tlv *)tag_buf;

    A_CHAR str_buf[HTT_MAX_STRING_LEN];
    A_CHAR *rx_gi[HTT_RX_PDEV_STATS_NUM_GI_COUNTERS];
    A_UINT8 i, j;
    A_UINT16 index;

    for (i=0; i < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; i++) {
        rx_gi[i] = malloc(HTT_MAX_STRING_LEN);
        if (!rx_gi[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(rx_gi[j]);
           }
           return;
        }
    }

    HTT_STATS_PRINT(FATAL, "HTT_RX_PDEV_UL_MUMIMO_TRIG_BE_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
        HTT_STATS_CMN_MAC_ID_GET(htt_ul_mumimo_trig_be_stats_buf->mac_id__word));

    HTT_STATS_PRINT(FATAL, "rx_11be_ul_mumimo = %u",
            htt_ul_mumimo_trig_be_stats_buf->rx_11be_ul_mumimo);

    HTT_CHECK_FOR_SPACE_ON_PRINT_BUFFER(
            HTT_RX_PDEV_STATS_NUM_BE_MCS_COUNTERS,
            HTT_MAX_STRING_LEN);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    /* MCS -2 and -1 will be printed first */
    index = snprintf(&str_buf[index],
        HTT_MAX_STRING_LEN - index,
        " -2:%u,-1:%u,",
        htt_ul_mumimo_trig_be_stats_buf->be_ul_mumimo_rx_mcs[HTT_RX_PDEV_STATS_NUM_BE_MCS_COUNTERS-2],
        htt_ul_mumimo_trig_be_stats_buf->be_ul_mumimo_rx_mcs[HTT_RX_PDEV_STATS_NUM_BE_MCS_COUNTERS-1]);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BE_MCS_COUNTERS-2; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_ul_mumimo_trig_be_stats_buf->be_ul_mumimo_rx_mcs[i]);
    }

    HTT_STATS_PRINT(FATAL, "be_ul_mumimo_rx_mcs = %s ", str_buf);

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        memset(rx_gi[j], 0x0, HTT_MAX_STRING_LEN);
        index = snprintf(&rx_gi[j][index],
            HTT_MAX_STRING_LEN - index,
            " -2:%u,-1:%u,",
            htt_ul_mumimo_trig_be_stats_buf->be_ul_mumimo_rx_gi[j][HTT_RX_PDEV_STATS_NUM_BE_MCS_COUNTERS-2],
            htt_ul_mumimo_trig_be_stats_buf->be_ul_mumimo_rx_gi[j][HTT_RX_PDEV_STATS_NUM_BE_MCS_COUNTERS-1]);

        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BE_MCS_COUNTERS-2; i++) {
            index += snprintf(&rx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i, htt_ul_mumimo_trig_be_stats_buf->be_ul_mumimo_rx_gi[j][i]);
        }

        HTT_STATS_PRINT(FATAL, "be_ul_mumimo_rx_gi[%u] = %s ", j, rx_gi[j]);
    }

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; i++) {
        free(rx_gi[i]);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_ULMUMIMO_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_ul_mumimo_trig_be_stats_buf->be_ul_mumimo_rx_nss[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_ul_mumimo_rx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BE_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_ul_mumimo_trig_be_stats_buf->be_ul_mumimo_rx_bw[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_ul_mumimo_rx_bw = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "be_ul_mumimo_rx_stbc = %u",
            htt_ul_mumimo_trig_be_stats_buf->be_ul_mumimo_rx_stbc);
    HTT_STATS_PRINT(FATAL, "be_ul_mumimo_rx_ldpc = %u",
            htt_ul_mumimo_trig_be_stats_buf->be_ul_mumimo_rx_ldpc);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_PUNCTURED_MODE_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_ul_mumimo_trig_be_stats_buf->rx_ul_mumimo_punctured_mode[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_ul_mumimo_punctured_mode = %s ", str_buf);

    for (i = 0; i < HTT_RX_PDEV_STATS_ULMUMIMO_NUM_SPATIAL_STREAMS; i++) {
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        index = 0;
        for (j = 0; j < HTT_RX_PDEV_STATS_NUM_BE_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_ul_mumimo_trig_be_stats_buf->be_rx_ul_mumimo_chain_rssi_in_dbm[i][j]);
        }
        HTT_STATS_PRINT(FATAL,
                "be_rx_ul_mumimo_rssi_in_dbm: chain[%u] = %s ", i, str_buf);
    }

    for (i = 0; i < HTT_RX_PDEV_MAX_ULMUMIMO_NUM_USER; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (j = 0; j <HTT_RX_PDEV_STATS_NUM_BE_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%d,", j, htt_ul_mumimo_trig_be_stats_buf->be_rx_ul_mumimo_target_rssi[i][j]);
        }
        HTT_STATS_PRINT(FATAL, "be_rx_ul_mumimo_target_rssi: user[%u] = %s ", i, str_buf);
    }

    for (i = 0; i < HTT_RX_PDEV_MAX_ULMUMIMO_NUM_USER; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (j = 0; j < HTT_RX_PDEV_STATS_ULMUMIMO_NUM_SPATIAL_STREAMS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%d,", j, htt_ul_mumimo_trig_be_stats_buf->be_rx_ul_mumimo_fd_rssi[i][j]);
        }
        HTT_STATS_PRINT(FATAL, "be_rx_ul_mumimo_fd_rssi: user[%u] = %s ", i, str_buf);
    }

    for (i = 0; i < HTT_RX_PDEV_MAX_ULMUMIMO_NUM_USER; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (j = 0; j < HTT_RX_PDEV_STATS_ULMUMIMO_NUM_SPATIAL_STREAMS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%d,", j, htt_ul_mumimo_trig_be_stats_buf->be_rx_ulmumimo_pilot_evm_dB_mean[i][j]);
        }
        HTT_STATS_PRINT(FATAL, "be_rx_ulmumimo_pilot_evm_dB_mean: user [%u] = %s ", i, str_buf);
    }

    HTT_STATS_PRINT(FATAL, "be_ul_mumimo_basic_trigger_rx_qos_null_only = %u",
        htt_ul_mumimo_trig_be_stats_buf->be_ul_mumimo_basic_trigger_rx_qos_null_only);

    HTT_STATS_PRINT(FATAL, "\n");
}

/*
 * htt_print_ul_ofdma_user_stats : display htt_rx_pdev_ul_ofdma_user_stats_tlv
 * @tag_buf:buffer containing the tlv htt_rx_pdev_ul_ofdma_user_stats_tlv
 * return:void
 */
static void htt_print_ul_ofdma_user_stats(A_UINT32 *tag_buf)
{
    htt_rx_pdev_ul_ofdma_user_stats_tlv *htt_ul_user_stats_buf =
        (htt_rx_pdev_ul_ofdma_user_stats_tlv *)tag_buf;

    if (htt_ul_user_stats_buf->user_index == 0) {
        HTT_STATS_PRINT(FATAL, "HTT_RX_PDEV_UL_OFDMA_USER_STAS_TLV");
    }

    HTT_STATS_PRINT(FATAL, "rx_ulofdma_non_data_ppdu_%u = %u ",
            htt_ul_user_stats_buf->user_index,
            htt_ul_user_stats_buf->rx_ulofdma_non_data_ppdu);
    HTT_STATS_PRINT(FATAL, "rx_ulofdma_data_ppdu_%u = %u ",
            htt_ul_user_stats_buf->user_index,
            htt_ul_user_stats_buf->rx_ulofdma_data_ppdu);
    HTT_STATS_PRINT(FATAL, "rx_ulofdma_mpdu_ok_%u = %u ",
            htt_ul_user_stats_buf->user_index,
            htt_ul_user_stats_buf->rx_ulofdma_mpdu_ok);
    HTT_STATS_PRINT(FATAL, "rx_ulofdma_mpdu_fail_%u = %u",
            htt_ul_user_stats_buf->user_index,
            htt_ul_user_stats_buf->rx_ulofdma_mpdu_fail);
    HTT_STATS_PRINT(FATAL, "rx_ulofdma_non_data_nusers_%u = %u",
            htt_ul_user_stats_buf->user_index,
            htt_ul_user_stats_buf->rx_ulofdma_non_data_nusers);
    HTT_STATS_PRINT(FATAL, "rx_ulofdma_data_nusers_%u = %u",
            htt_ul_user_stats_buf->user_index,
            htt_ul_user_stats_buf->rx_ulofdma_data_nusers);
}

/*
 * htt_print_be_ul_ofdma_user_stats : display htt_rx_pdev_be_ul_ofdma_user_stats_tlv
 * @tag_buf:buffer containing the tlv htt_rx_pdev_be_ul_ofdma_user_stats_tlv
 * return:void
 */
static void htt_print_be_ul_ofdma_user_stats(A_UINT32 *tag_buf)
{
    htt_rx_pdev_be_ul_ofdma_user_stats_tlv *htt_ul_user_stats_buf =
        (htt_rx_pdev_be_ul_ofdma_user_stats_tlv *)tag_buf;

    if (htt_ul_user_stats_buf->user_index == 0) {
        HTT_STATS_PRINT(FATAL, "HTT_RX_PDEV_BE_UL_OFDMA_USER_STAS_TLV");
    }

    HTT_STATS_PRINT(FATAL, "be_rx_ulofdma_non_data_ppdu_%u = %u ",
            htt_ul_user_stats_buf->user_index,
            htt_ul_user_stats_buf->be_rx_ulofdma_non_data_ppdu);
    HTT_STATS_PRINT(FATAL, "be_rx_ulofdma_data_ppdu_%u = %u ",
            htt_ul_user_stats_buf->user_index,
            htt_ul_user_stats_buf->be_rx_ulofdma_data_ppdu);
    HTT_STATS_PRINT(FATAL, "be_rx_ulofdma_mpdu_ok_%u = %u ",
            htt_ul_user_stats_buf->user_index,
            htt_ul_user_stats_buf->be_rx_ulofdma_mpdu_ok);
    HTT_STATS_PRINT(FATAL, "be_rx_ulofdma_mpdu_fail_%u = %u",
            htt_ul_user_stats_buf->user_index,
            htt_ul_user_stats_buf->be_rx_ulofdma_mpdu_fail);
    HTT_STATS_PRINT(FATAL, "be_rx_ulofdma_non_data_nusers_%u = %u",
            htt_ul_user_stats_buf->user_index,
            htt_ul_user_stats_buf->be_rx_ulofdma_non_data_nusers);
    HTT_STATS_PRINT(FATAL, "be_rx_ulofdma_data_nusers_%u = %u",
            htt_ul_user_stats_buf->user_index,
            htt_ul_user_stats_buf->be_rx_ulofdma_data_nusers);
}

static const char *htt_ax_rx_ru_size_to_str(enum htt_ul_ofdma_user_info_ru_size ru_size)
{
    switch (ru_size) {
    case HTT_UL_OFDMA_V0_RU_SIZE_RU_26:
        return "26";
    case HTT_UL_OFDMA_V0_RU_SIZE_RU_52:
        return "52";
    case HTT_UL_OFDMA_V0_RU_SIZE_RU_106:
        return "106";
    case HTT_UL_OFDMA_V0_RU_SIZE_RU_242:
        return "242";
    case HTT_UL_OFDMA_V0_RU_SIZE_RU_484:
        return "484";
    case HTT_UL_OFDMA_V0_RU_SIZE_RU_996:
        return "996";
    case HTT_UL_OFDMA_V0_RU_SIZE_RU_996x2:
        return "996x2";
    default:
        return "unknown";
    }
}

/*
 * htt_print_rx_pdev_ul_ofdma_stats_tlv: display htt_rx_pdev_ul_trigger_stats_tlv
 * @tag_buf: buffer containing the tlv htt_rx_pdev_ul_trigger_stats_tlv
 *
 * return:void
 */
static void htt_print_ul_ofdma_trigger_stats(A_UINT32 *tag_buf)
{
    htt_rx_pdev_ul_trigger_stats_tlv *htt_trigger_stats_buf =
        (htt_rx_pdev_ul_trigger_stats_tlv *)tag_buf;

    A_CHAR   str_buf[HTT_MAX_STRING_LEN];
    A_CHAR   *rx_gi[HTT_RX_PDEV_STATS_NUM_GI_COUNTERS];
    A_UINT8  i, j;
    A_UINT16 index;

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; i++) {
        rx_gi[i] = malloc(HTT_MAX_STRING_LEN);
        if (!rx_gi[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(rx_gi[j]);
           }
           return;
        }
    }

    HTT_STATS_PRINT(FATAL, "HTT_RX_PDEV_UL_TRIGGER_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            HTT_STATS_CMN_MAC_ID_GET(htt_trigger_stats_buf->mac_id__word));
    HTT_STATS_PRINT(FATAL, "rx_11ax_ul_ofdma =%u",
        htt_trigger_stats_buf->rx_11ax_ul_ofdma);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_trigger_stats_buf->ul_ofdma_rx_mcs[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_mcs = %s ", str_buf);

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        memset(rx_gi[j], 0x0, HTT_MAX_STRING_LEN);
        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
            index += snprintf(&rx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_trigger_stats_buf->ul_ofdma_rx_gi[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_gi[%u] = %s ", j, rx_gi[j]);
    }

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; i++) {
        free(rx_gi[i]);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_trigger_stats_buf->ul_ofdma_rx_nss[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_trigger_stats_buf->ul_ofdma_rx_bw[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_bw = %s ", str_buf);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_REDUCED_CHAN_TYPES; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (j = 0; j < HTT_RX_PDEV_STATS_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_trigger_stats_buf->reduced_ul_ofdma_rx_bw[i][j]);
        }

        HTT_STATS_PRINT(FATAL, "%s = %s ", i == 0 ? "half_ul_ofdma_rx_bw" : "quarter_ul_ofdma_rx_bw", str_buf);
    }

    HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_stbc = %u",
            htt_trigger_stats_buf->ul_ofdma_rx_stbc);
    HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_ldpc = %u",
            htt_trigger_stats_buf->ul_ofdma_rx_ldpc);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_RU_SIZE_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %s:%u,", htt_ax_rx_ru_size_to_str(i),
                htt_trigger_stats_buf->rx_ulofdma_data_ru_size_ppdu[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_ulofdma_data_ru_size_ppdu = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_RU_SIZE_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %s:%u,", htt_ax_rx_ru_size_to_str(i),
                htt_trigger_stats_buf->rx_ulofdma_non_data_ru_size_ppdu[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_ulofdma_non_data_ru_size_ppdu = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_UL_MAX_UPLINK_RSSI_TRACK; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_trigger_stats_buf->uplink_sta_aid[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_rssi_track_sta_aid = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_UL_MAX_UPLINK_RSSI_TRACK; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%d,", i, htt_trigger_stats_buf->uplink_sta_target_rssi[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_sta_target_rssi = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_UL_MAX_UPLINK_RSSI_TRACK; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%d,", i, htt_trigger_stats_buf->uplink_sta_fd_rssi[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_sta_fd_rssi = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_UL_MAX_UPLINK_RSSI_TRACK; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_trigger_stats_buf->uplink_sta_power_headroom[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_sta_power_headroom = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "ul_ofdma_basic_trigger_rx_qos_null_only = %u",
        htt_trigger_stats_buf->ul_ofdma_basic_trigger_rx_qos_null_only);

    HTT_STATS_PRINT(FATAL, "\n");
}

static const char *htt_be_rx_ru_size_to_str(HTT_RX_PDEV_STATS_BE_RU_SIZE ru_size)
{
    switch (ru_size) {
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_26:
        return "26";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_52:
        return "52";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_52_26:
        return "52+26";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_106:
        return "106";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_106_26:
        return "106+26";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_242:
        return "242";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_484:
        return "484";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_484_242:
        return "484+242";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_996:
        return "996";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_996_484:
        return "996+484";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_996_484_242:
        return "996+484+242";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_996x2:
        return "996x2";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_996x2_484:
        return "996x2+484";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_996x3:
        return "996x3";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_996x3_484:
        return "996x3+484";
    case HTT_RX_PDEV_STATS_BE_RU_SIZE_996x4:
        return "996x4";
    default:
        return "unknown";
    }
}

/*
 * htt_print_rx_pdev_ul_ofdma_stats_tlv: display htt_rx_pdev_be_ul_trigger_stats_tlv
 * @tag_buf: buffer containing the tlv htt_rx_pdev_be_ul_trigger_stats_tlv
 *
 * return:void
 */
static void htt_print_be_ul_ofdma_trigger_stats(A_UINT32 *tag_buf)
{
    htt_rx_pdev_be_ul_trigger_stats_tlv *htt_trigger_stats_buf =
        (htt_rx_pdev_be_ul_trigger_stats_tlv *)tag_buf;

    A_CHAR   str_buf[HTT_MAX_STRING_LEN];
    A_CHAR   *rx_gi[HTT_RX_PDEV_STATS_NUM_GI_COUNTERS];
    A_UINT8  i, j;
    A_UINT16 index;

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; i++) {
        rx_gi[i] = malloc(HTT_MAX_STRING_LEN);
        if (!rx_gi[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(rx_gi[j]);
           }
           return;
        }
    }

    HTT_STATS_PRINT(FATAL, "HTT_RX_PDEV_BE_UL_TRIGGER_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            HTT_STATS_CMN_MAC_ID_GET(htt_trigger_stats_buf->mac_id__word));
    HTT_STATS_PRINT(FATAL, "rx_11be_ul_ofdma =%u",
        htt_trigger_stats_buf->rx_11be_ul_ofdma);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BE_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_trigger_stats_buf->be_ul_ofdma_rx_mcs[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_ul_ofdma_rx_mcs = %s ", str_buf);

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        memset(rx_gi[j], 0x0, HTT_MAX_STRING_LEN);
        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BE_MCS_COUNTERS; i++) {
            index += snprintf(&rx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_trigger_stats_buf->be_ul_ofdma_rx_gi[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "be_ul_ofdma_rx_gi[%u] = %s ", j, rx_gi[j]);
    }

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; i++) {
        free(rx_gi[i]);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_trigger_stats_buf->be_ul_ofdma_rx_nss[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_ul_ofdma_rx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BE_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_trigger_stats_buf->be_ul_ofdma_rx_bw[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_ul_ofdma_rx_bw = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "be_ul_ofdma_rx_stbc = %u",
            htt_trigger_stats_buf->be_ul_ofdma_rx_stbc);
    HTT_STATS_PRINT(FATAL, "be_ul_ofdma_rx_ldpc = %u",
            htt_trigger_stats_buf->be_ul_ofdma_rx_ldpc);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BE_RU_SIZE_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %s:%u,", htt_be_rx_ru_size_to_str(i),
                htt_trigger_stats_buf->be_rx_ulofdma_data_ru_size_ppdu[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_rx_ulofdma_data_ru_size_ppdu = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BE_RU_SIZE_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %s:%u,", htt_be_rx_ru_size_to_str(i),
                htt_trigger_stats_buf->be_rx_ulofdma_non_data_ru_size_ppdu[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_rx_ulofdma_non_data_ru_size_ppdu = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_UL_MAX_UPLINK_RSSI_TRACK; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_trigger_stats_buf->be_uplink_sta_aid[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_rx_rssi_track_sta_aid = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_UL_MAX_UPLINK_RSSI_TRACK; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%d,", i, htt_trigger_stats_buf->be_uplink_sta_target_rssi[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_rx_sta_target_rssi = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_UL_MAX_UPLINK_RSSI_TRACK; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%d,", i, htt_trigger_stats_buf->be_uplink_sta_fd_rssi[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_rx_sta_fd_rssi = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_UL_MAX_UPLINK_RSSI_TRACK; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_trigger_stats_buf->be_uplink_sta_power_headroom[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_rx_sta_power_headroom = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "be_ul_ofdma_basic_trigger_rx_qos_null_only = %u",
        htt_trigger_stats_buf->be_ul_ofdma_basic_trigger_rx_qos_null_only);

    HTT_STATS_PRINT(FATAL, "ul_mlo_send_qdepth_params_count = %u",
        htt_trigger_stats_buf->ul_mlo_send_qdepth_params_count);

    HTT_STATS_PRINT(FATAL, "ul_mlo_proc_qdepth_params_count = %u",
        htt_trigger_stats_buf->ul_mlo_proc_qdepth_params_count);

    HTT_STATS_PRINT(FATAL, "ul_mlo_proc_accepted_qdepth_params_count = %u",
        htt_trigger_stats_buf->ul_mlo_proc_accepted_qdepth_params_count);

    HTT_STATS_PRINT(FATAL, "ul_mlo_proc_discarded_qdepth_params_count = %u",
        htt_trigger_stats_buf->ul_mlo_proc_discarded_qdepth_params_count);

    HTT_STATS_PRINT(FATAL, "\n");
}

/*
 * htt_print_rx_pdev_rate_stats_tlv: display htt_rx_pdev_rate_stats_tlv
 * @tag_buf: buffer containing the tlv htt_rx_pdev_rate_stats_tlv
 *
 * return:void
 */
static void htt_print_rx_pdev_rate_stats_tlv(A_UINT32 *tag_buf)
{
    htt_rx_pdev_rate_stats_tlv *htt_stats_buf =
        (htt_rx_pdev_rate_stats_tlv *)tag_buf;
    A_UINT8  i, j;
    A_UINT16 index = 0;
    A_CHAR   *rssi_chain[HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS];
    A_CHAR   *rx_gi[HTT_RX_PDEV_STATS_NUM_GI_COUNTERS];
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_CHAR   *rx_pilot_evm_dB[HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS];

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        rssi_chain[i] = malloc(HTT_MAX_STRING_LEN);
        if (!rssi_chain[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(rssi_chain[j]);
           }
           return;
        }
    }

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; i++) {
        rx_gi[i] = malloc(HTT_MAX_STRING_LEN);
        if (!rx_gi[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(rx_gi[j]);
           }
           for (j = 0; j < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; j++) {
                free(rssi_chain[j]);
           }
           return;
        }
    }

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        rx_pilot_evm_dB[i] = malloc(HTT_MAX_STRING_LEN);
        if (!rx_pilot_evm_dB[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(rx_pilot_evm_dB[j]);
           }
           for (j = 0; j < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; j++) {
                free(rssi_chain[j]);
           }
           for (j = 0; j < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
                free(rx_gi[j]);
           }
           return;
        }
    }

    HTT_STATS_PRINT(FATAL, "HTT_RX_PDEV_RATE_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__word & 0xFF);
    /*HTT_STATS_PRINT(FATAL, "word = %u",
            ((htt_stats_buf->mac_id__word & 0xFFFFFF00) >> 8 ));*/
    HTT_STATS_PRINT(FATAL, "nsts = %u",
            htt_stats_buf->nsts);
    HTT_STATS_PRINT(FATAL, "rx_ldpc = %u",
            htt_stats_buf->rx_ldpc);
    HTT_STATS_PRINT(FATAL, "rts_cnt = %u",
            htt_stats_buf->rts_cnt);
    HTT_STATS_PRINT(FATAL, "rssi_mgmt = %u",
            htt_stats_buf->rssi_mgmt);
    HTT_STATS_PRINT(FATAL, "rssi_data = %u",
            htt_stats_buf->rssi_data);
    HTT_STATS_PRINT(FATAL, "rssi_comb = %u",
            htt_stats_buf->rssi_comb);
    HTT_STATS_PRINT(FATAL, "rssi_in_dbm = %d",
            htt_stats_buf->rssi_in_dbm);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_mcs[i]);
    }
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_EXTRA_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS,
                htt_stats_buf->rx_mcs_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->rx_nss[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_DCM_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_dcm[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_dcm = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_stbc[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_stbc = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_bw[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_bw = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "rx_evm_nss_count = %u",
            htt_stats_buf->nss_count);

    HTT_STATS_PRINT(FATAL, "rx_evm_pilot_count = %u",
            htt_stats_buf->pilot_count);

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; j++) {
        index = 0;

        for (i = 0; i < HTT_RX_PDEV_STATS_RXEVM_MAX_PILOTS_PER_NSS; i++) {
            index += snprintf(&rx_pilot_evm_dB[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%d,", i, htt_stats_buf->rx_pilot_evm_dB[j][i]);
        }

        HTT_STATS_PRINT(FATAL, "pilot_evm_dB[%u] = %s ", j, rx_pilot_evm_dB[j]);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%d,", i, htt_stats_buf->rx_pilot_evm_dB_mean[i]);
    }

    HTT_STATS_PRINT(FATAL, "pilot_evm_dB_mean = %s ", str_buf);

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; j++) {
        index = 0;

        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
            index += snprintf(&rssi_chain[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->rssi_chain[j][i]);
        }

        HTT_STATS_PRINT(FATAL, "rssi_chain[%u] = %s ", j, rssi_chain[j]);
    }

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
            index += snprintf(&rx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->rx_gi[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "rx_gi[%u] = %s ", j, rx_gi[j]);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_PREAMBLE_TYPES; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_pream[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_pream = %s", str_buf);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        free(rssi_chain[i]);
    }

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        free(rx_pilot_evm_dB[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_11ax_su_ext = %u",
            htt_stats_buf->rx_11ax_su_ext);
    HTT_STATS_PRINT(FATAL, "rx_11ac_mumimo = %u",
            htt_stats_buf->rx_11ac_mumimo);
    HTT_STATS_PRINT(FATAL, "rx_11ax_mumimo = %u",
            htt_stats_buf->rx_11ax_mumimo);
    HTT_STATS_PRINT(FATAL, "rx_11ax_ofdma = %u",
            htt_stats_buf->rx_11ax_ofdma);
    HTT_STATS_PRINT(FATAL, "txbf = %u",
            htt_stats_buf->txbf);
    HTT_STATS_PRINT(FATAL, "rx_su_ndpa = %u",
            htt_stats_buf->rx_su_ndpa);

    HTT_CHECK_FOR_SPACE_ON_PRINT_BUFFER(
            HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS, HTT_MAX_STRING_LEN);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_11ax_su_txbf_mcs[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_11ax_su_txbf_mcs = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "rx_mu_ndpa = %u",
            htt_stats_buf->rx_mu_ndpa);

    HTT_CHECK_FOR_SPACE_ON_PRINT_BUFFER(
            HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS, HTT_MAX_STRING_LEN);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_11ax_mu_txbf_mcs[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_11ax_mu_txbf_mcs = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "rx_br_poll = %u",
            htt_stats_buf->rx_br_poll);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    HTT_CHECK_FOR_SPACE_ON_PRINT_BUFFER(HTT_RX_PDEV_STATS_NUM_LEGACY_CCK_STATS,
            HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_LEGACY_CCK_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_legacy_cck_rate[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_legacy_cck_rate = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    HTT_CHECK_FOR_SPACE_ON_PRINT_BUFFER(HTT_RX_PDEV_STATS_NUM_LEGACY_OFDM_STATS,
            HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_LEGACY_OFDM_STATS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_legacy_ofdm_rate[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_legacy_ofdm_rate = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "rx_active_dur_us_low = %u",
            htt_stats_buf->rx_active_dur_us_low);
    HTT_STATS_PRINT(FATAL, "rx_active_dur_us_high = %u",
            htt_stats_buf->rx_active_dur_us_high);

    HTT_STATS_PRINT(FATAL, "rx_11ax_ul_ofdma = %u",
            htt_stats_buf->rx_11ax_ul_ofdma);

    HTT_CHECK_FOR_SPACE_ON_PRINT_BUFFER(
            HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS, HTT_MAX_STRING_LEN);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->ul_ofdma_rx_mcs[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_mcs = %s ", str_buf);

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
            index += snprintf(&rx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i, htt_stats_buf->ul_ofdma_rx_gi[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_gi[%u] = %s ", j, rx_gi[j]);
    }

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; i++) {
        free(rx_gi[i]);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->ul_ofdma_rx_nss[i]);
    }

    HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->ul_ofdma_rx_bw[i]);
    }

    HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_bw = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_stbc = %u",
            htt_stats_buf->ul_ofdma_rx_stbc);
    HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_ldpc = %u",
            htt_stats_buf->ul_ofdma_rx_ldpc);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_MAX_OFDMA_NUM_USER; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_ulofdma_non_data_ppdu[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_ulofdma_non_data_ppdu = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_MAX_OFDMA_NUM_USER; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_ulofdma_data_ppdu[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_ulofdma_data_ppdu = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_MAX_OFDMA_NUM_USER; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_ulofdma_mpdu_ok[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_ulofdma_mpdu_ok = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_MAX_OFDMA_NUM_USER; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_ulofdma_mpdu_fail[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_ulofdma_mpdu_fail = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_MAX_ULMUMIMO_NUM_USER; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_ulmumimo_non_data_ppdu[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_ulmumimo_non_data_ppdu = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_MAX_ULMUMIMO_NUM_USER; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_ulmumimo_data_ppdu[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_ulmumimo_data_ppdu = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_MAX_ULMUMIMO_NUM_USER; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_ulmumimo_mpdu_ok[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_ulmumimo_mpdu_ok = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_MAX_ULMUMIMO_NUM_USER; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_ulmumimo_mpdu_fail[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_ulmumimo_mpdu_fail = %s", str_buf);

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; j++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (i = 0; i < HTT_RX_PDEV_MAX_OFDMA_NUM_USER; i++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%d,", i, htt_stats_buf->rx_ul_fd_rssi[j][i]);
        }

        HTT_STATS_PRINT(FATAL, "rx_ul_fd_rssi: nss[%u] = %s", j, str_buf);
    }

    HTT_STATS_PRINT(FATAL, "per_chain_rssi_pkt_type = %#x",
            htt_stats_buf->per_chain_rssi_pkt_type);

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; j++) {
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        index = 0;

        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BW_COUNTERS; i++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%hhd,",
                    i, htt_stats_buf->rx_per_chain_rssi_in_dbm[j][i]);
        }

        HTT_STATS_PRINT(FATAL,
                "rx_per_chain_rssi_in_dbm[%u] = %s ", j, str_buf);
    }

    HTT_CHECK_FOR_SPACE_ON_PRINT_BUFFER(
            HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS, HTT_MAX_STRING_LEN);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_11ax_dl_ofdma_mcs[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_11ax_dl_ofdma_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    HTT_CHECK_FOR_SPACE_ON_PRINT_BUFFER(HTT_RX_PDEV_STATS_NUM_RU_SIZE_COUNTERS,
            HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_RU_SIZE_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_11ax_dl_ofdma_ru[i]);
    }

    HTT_STATS_PRINT(FATAL, "rx_11ax_dl_ofdma_ru = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_MAX_OFDMA_NUM_USER; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_ulofdma_non_data_nusers[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_ulofdma_non_data_nusers = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_MAX_OFDMA_NUM_USER; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_ulofdma_data_nusers[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_ulofdma_data_nusers = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "\n");
}

/*
 * htt_print_rx_pdev_rate_ext_stats_tlv: display htt_rx_pdev_rate_ext_stats_tlv
 * @tag_buf: buffer containing the tlv htt_rx_pdev_rate_ext_stats_tlv
 *
 * return:void
 */
static void htt_print_rx_pdev_rate_ext_stats_tlv(A_UINT32 *tag_buf)
{
    htt_rx_pdev_rate_ext_stats_tlv *htt_stats_buf =
        (htt_rx_pdev_rate_ext_stats_tlv *)tag_buf;
    A_UINT8  i, j;
    A_UINT16 index = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_CHAR   *rx_gi_ext[HTT_RX_PDEV_STATS_NUM_GI_COUNTERS];

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; i++) {
        rx_gi_ext[i] = malloc(HTT_MAX_STRING_LEN);
        if (!rx_gi_ext[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(rx_gi_ext[j]);
           }
           return;
        }
    }

    HTT_STATS_PRINT(FATAL, "rssi_mcast_in_dbm = %d",
    htt_stats_buf->rssi_mcast_in_dbm);

    HTT_STATS_PRINT(FATAL, "rssi_mgmt_in_dbm = %d",
    htt_stats_buf->rssi_mgmt_in_dbm);

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; j++) {
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        index = 0;

        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BW_EXT_COUNTERS; i++) {
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%d,", i,
                htt_stats_buf->rssi_chain_ext[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "rssi_chain_ext[%u] = %s ", j, str_buf);
    }
    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; j++) {
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        index = 0;

        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BW_EXT_COUNTERS; i++) {
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%hhd,", i,
                htt_stats_buf->rx_per_chain_rssi_ext_in_dbm[j][i]);
        }
        HTT_STATS_PRINT(FATAL,
            "rx_per_chain_rssi_ext_in_dbm[%u] = %s ", j, str_buf);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    index += snprintf(&str_buf[index],
        HTT_MAX_STRING_LEN - index,
        " -2:%u, -1:%u,",
        htt_stats_buf->rx_mcs_ext_2[0],
        htt_stats_buf->rx_mcs_ext_2[1]);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS_EXT; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->rx_mcs_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_mcs_ext = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS_EXT; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->rx_stbc_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_stbc_ext = %s ", str_buf);

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        memset(rx_gi_ext[j], 0x0, HTT_MAX_STRING_LEN);

        index += snprintf(&rx_gi_ext[j][index],
                HTT_MAX_STRING_LEN - index,
                " -2:%u, -1:%u,",
                htt_stats_buf->rx_gi_ext_2[j][0],
                htt_stats_buf->rx_gi_ext_2[j][1]);

        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS_EXT; i++) {
            index += snprintf(&rx_gi_ext[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->rx_gi_ext[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "rx_gi_ext[%u] = %s ", j, rx_gi_ext[j]);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BW_EXT2_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->rx_bw_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_bw = %s ", str_buf);

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_REDUCED_CHAN_TYPES; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (j = 0; j < HTT_RX_PDEV_STATS_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j,
                    htt_stats_buf->reduced_rx_bw[i][j]);
        }

        HTT_STATS_PRINT(FATAL, "%s = %s ", i == 0 ? "half_rx_bw" : "quarter_rx_bw", str_buf);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_PUNCTURED_MODE_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->rx_su_punctured_mode[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_su_punctured_mode = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS_EXT; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->ul_ofdma_rx_mcs_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_mcs_ext = %s ", str_buf);

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        memset(rx_gi_ext[j], 0x0, HTT_MAX_STRING_LEN);
        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS_EXT; i++) {
            index += snprintf(&rx_gi_ext[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->ul_ofdma_rx_gi_ext[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "ul_ofdma_rx_gi_ext[%u] = %s ", j, rx_gi_ext[j]);
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS_EXT; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->rx_11ax_su_txbf_mcs_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_11ax_su_txbf_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS_EXT; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->rx_11ax_mu_txbf_mcs_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_11ax_mu_txbf_mcs_ext = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_MCS_COUNTERS_EXT; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->rx_11ax_dl_ofdma_mcs_ext[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_11ax_dl_ofdma_mcs_ext = %s ", str_buf);

    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; j++) {
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        index = 0;

        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BW_EXT_2_COUNTERS; i++) {
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%d,", i,
                htt_stats_buf->rssi_chain_ext_2[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "rssi_chain_ext_2[%u] = %s ", j, str_buf);
    }
    for (j = 0; j < HTT_RX_PDEV_STATS_NUM_SPATIAL_STREAMS; j++) {
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        index = 0;

        for (i = 0; i < HTT_RX_PDEV_STATS_NUM_BW_EXT_2_COUNTERS; i++) {
            index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%hhd,", i,
                htt_stats_buf->rx_per_chain_rssi_ext_2_in_dbm[j][i]);
        }
        HTT_STATS_PRINT(FATAL,
            "rx_per_chain_rssi_ext_2_in_dbm[%u] = %s ", j, str_buf);
    }

    HTT_STATS_PRINT(FATAL, "\n");

    for (i = 0; i < HTT_RX_PDEV_STATS_NUM_GI_COUNTERS; i++) {
        free(rx_gi_ext[i]);
    }
}

/*
 * htt_print_rx_pdev_ppdu_dur_stats_tlv : display htt_rx_pdev_ppdu_dur_stats_tlv
 * @tag_buf: buffer containing the tlv htt_rx_pdev_ppdu_dur_stats_tlv
 *
 * return: void
 */
static void htt_print_rx_pdev_ppdu_dur_stats_tlv(A_UINT32 *tag_buf)
{
    htt_rx_pdev_ppdu_dur_stats_tlv * htt_stats_buf =
        (htt_rx_pdev_ppdu_dur_stats_tlv *)tag_buf;
    A_UINT8  i;
    A_UINT16 index;
    A_CHAR str_buf[HTT_MAX_STRING_LEN];

    HTT_STATS_PRINT(FATAL, "HTT_RX_PDEV_PPDU_DUR_STATS_TLV:");

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_PDEV_STATS_PPDU_DUR_HIST_BINS ; i++) {
        index += snprintf(&str_buf [index],
                HTT_MAX_STRING_LEN - index,
                " %u-%u : %u,",
                i * HTT_PDEV_STATS_PPDU_DUR_HIST_INTERVAL_US,
                (i + 1) * HTT_PDEV_STATS_PPDU_DUR_HIST_INTERVAL_US,
                htt_stats_buf->rx_ppdu_dur_hist[i]);
    }
    HTT_STATS_PRINT(FATAL, "rx_ppdu_dur_hist_us = %s\n", str_buf);
}

/*
 * htt_print_rx_soc_fw_stats_tlv: display htt_rx_soc_fw_stats_tlv
 * @tag_buf: buffer containing the tlv htt_rx_soc_fw_stats_tlv
 *
 * return:void
 */
static void htt_print_rx_soc_fw_stats_tlv(A_UINT32 *tag_buf)
{
    htt_rx_soc_fw_stats_tlv *htt_stats_buf =
        (htt_rx_soc_fw_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_RX_SOC_FW_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "fw_reo_ring_data_msdu = %u",
            htt_stats_buf->fw_reo_ring_data_msdu);
    HTT_STATS_PRINT(FATAL, "fw_to_host_data_msdu_bcmc = %u",
            htt_stats_buf->fw_to_host_data_msdu_bcmc);
    HTT_STATS_PRINT(FATAL, "fw_to_host_data_msdu_uc = %u",
            htt_stats_buf->fw_to_host_data_msdu_uc);
    HTT_STATS_PRINT(FATAL, "ofld_remote_data_buf_recycle_cnt = %u",
            htt_stats_buf->ofld_remote_data_buf_recycle_cnt);
    HTT_STATS_PRINT(FATAL, "ofld_remote_free_buf_indication_cnt = %u",
            htt_stats_buf->ofld_remote_free_buf_indication_cnt);
    HTT_STATS_PRINT(FATAL, "ofld_buf_to_host_data_msdu_uc = %u",
            htt_stats_buf->ofld_buf_to_host_data_msdu_uc);
    HTT_STATS_PRINT(FATAL, "reo_fw_ring_to_host_data_msdu_uc = %u",
            htt_stats_buf->reo_fw_ring_to_host_data_msdu_uc);
    HTT_STATS_PRINT(FATAL, "wbm_sw_ring_reap = %u",
            htt_stats_buf->wbm_sw_ring_reap);
    HTT_STATS_PRINT(FATAL, "wbm_forward_to_host_cnt = %u",
            htt_stats_buf->wbm_forward_to_host_cnt);
    HTT_STATS_PRINT(FATAL, "wbm_target_recycle_cnt = %u",
            htt_stats_buf->wbm_target_recycle_cnt);
    HTT_STATS_PRINT(FATAL, "target_refill_ring_recycle_cnt = %u",
            htt_stats_buf->target_refill_ring_recycle_cnt);
}

/*
 * htt_print_rx_soc_fw_refill_ring_empty_tlv_v: display
 *                    htt_rx_soc_fw_refill_ring_empty_tlv_v
 * @tag_buf: buffer containing the tlv htt_rx_soc_fw_refill_ring_empty_tlv_v
 *
 * return:void
 */
static void htt_print_rx_soc_fw_refill_ring_empty_tlv_v(A_UINT32 *tag_buf)
{
    htt_rx_soc_fw_refill_ring_empty_tlv_v *htt_stats_buf =
        (htt_rx_soc_fw_refill_ring_empty_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                     = 0;
    A_CHAR   refill_ring_empty_cnt[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                                   = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_RX_SOC_FW_REFILL_RING_EMPTY_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&refill_ring_empty_cnt[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->refill_ring_empty_cnt[i]);
    }

    HTT_STATS_PRINT(FATAL, "refill_ring_empty_cnt = %s\n",
            refill_ring_empty_cnt);
}

/*
 * htt_rx_soc_fw_refill_ring_num_rxdma_err_tlv_v: display
 *                    htt_rx_soc_fw_refill_ring_num_rxdma_err_tlv_v
 * @tag_buf: buffer containing the tlv htt_rx_soc_fw_refill_ring_num_rxdma_err_tlv_v
 *
 * return:void
 */
static void htt_print_rx_soc_fw_refill_ring_num_rxdma_err_tlv_v(A_UINT32 *tag_buf)
{
    htt_rx_soc_fw_refill_ring_num_rxdma_err_tlv_v *htt_stats_buf =
        (htt_rx_soc_fw_refill_ring_num_rxdma_err_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                             = 0;
    A_CHAR   rxdma_err_cnt[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                           = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_RX_SOC_FW_REFILL_RING_NUM_RXDMA_ERR_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&rxdma_err_cnt[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->rxdma_err[i]);
    }

    HTT_STATS_PRINT(FATAL, "rxdma_err = %s\n",
            rxdma_err_cnt);
}

/*
 * htt_rx_soc_fw_refill_ring_num_reo_err_tlv_v: display
 *                    htt_rx_soc_fw_refill_ring_num_reo_err_tlv_v
 * @tag_buf: buffer containing the tlv htt_rx_soc_fw_refill_ring_num_reo_err_tlv_v
 *
 * return:void
 */
static void htt_print_rx_soc_fw_refill_ring_num_reo_err_tlv_v(A_UINT32 *tag_buf)
{
    htt_rx_soc_fw_refill_ring_num_reo_err_tlv_v *htt_stats_buf =
        (htt_rx_soc_fw_refill_ring_num_reo_err_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                           = 0;
    A_CHAR   reo_err_cnt[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                         = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_RX_SOC_FW_REFILL_RING_NUM_REO_ERR_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&reo_err_cnt[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->reo_err[i]);
    }

    HTT_STATS_PRINT(FATAL, "reo_err = %s\n",
            reo_err_cnt);
}

/*
 * htt_rx_reo_debug_stats_tlv_v: display
 *                    htt_rx_reo_debug_stats_tlv_v
 * @tag_buf: buffer containing the tlv htt_rx_reo_debug_stats_tlv_v
 *
 * return:void
 */
static void htt_print_rx_reo_debug_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_rx_reo_resource_stats_tlv_v *htt_stats_buf =
        (htt_rx_reo_resource_stats_tlv_v *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_RX_REO_RESOURCE_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "sample_id = %u",
            htt_stats_buf->sample_id);
    HTT_STATS_PRINT(FATAL, "total_max = %u",
            htt_stats_buf->total_max);
    HTT_STATS_PRINT(FATAL, "total_avg = %u",
            htt_stats_buf->total_avg);
    HTT_STATS_PRINT(FATAL, "total_sample = %u",
            htt_stats_buf->total_sample);
    HTT_STATS_PRINT(FATAL, "non_zeros_avg = %u",
            htt_stats_buf->non_zeros_avg);
    HTT_STATS_PRINT(FATAL, "non_zeros_sample = %u",
            htt_stats_buf->non_zeros_sample);
    HTT_STATS_PRINT(FATAL, "last_non_zeros_max = %u",
            htt_stats_buf->last_non_zeros_max);
    HTT_STATS_PRINT(FATAL, "last_non_zeros_min %u",
            htt_stats_buf->last_non_zeros_min);
    HTT_STATS_PRINT(FATAL, "last_non_zeros_avg %u",
            htt_stats_buf->last_non_zeros_avg);
    HTT_STATS_PRINT(FATAL, "last_non_zeros_sample %u\n",
            htt_stats_buf->last_non_zeros_sample);
}

/*
 * htt_print_rx_soc_fw_refill_ring_num_refill_tlv_v: display
 *                htt_rx_soc_fw_refill_ring_num_refill_tlv_v
 * @tag_buf: buffer containing the tlv htt_rx_soc_fw_refill_ring_num_refill_tlv
 *
 * return:void
 */
static void htt_print_rx_soc_fw_refill_ring_num_refill_tlv_v(A_UINT32 *tag_buf)
{
    htt_rx_soc_fw_refill_ring_num_refill_tlv_v *htt_stats_buf =
        (htt_rx_soc_fw_refill_ring_num_refill_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                      = 0;
    A_CHAR   refill_ring_num_refill[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                                    = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_RX_SOC_FW_REFILL_RING_NUM_REFILL_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&refill_ring_num_refill[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->refill_ring_num_refill[i]);
    }

    HTT_STATS_PRINT(FATAL, "refill_ring_num_refill = %s\n",
            refill_ring_num_refill);
}

/*
 * htt_print_rx_pdev_fw_stats_tlv: display htt_rx_pdev_fw_stats_tlv
 * @tag_buf: buffer containing the tlv htt_rx_pdev_fw_stats_tlv
 *
 * return:void
 */
static void htt_print_rx_pdev_fw_stats_tlv(A_UINT32 *tag_buf)
{
    htt_rx_pdev_fw_stats_tlv *htt_stats_buf =
        (htt_rx_pdev_fw_stats_tlv *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                    = 0;
    A_CHAR   *fw_ring_mgmt_subtype = NULL;
    A_CHAR   *fw_ring_ctrl_subtype = NULL;

    fw_ring_ctrl_subtype = (A_CHAR *)malloc(HTT_MAX_STRING_LEN * sizeof(A_CHAR));
    if (!fw_ring_ctrl_subtype) {
        HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
        return;
    }

    fw_ring_mgmt_subtype = (A_CHAR *)malloc(HTT_MAX_STRING_LEN * sizeof(A_CHAR));
    if (!fw_ring_mgmt_subtype) {
       HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
       free(fw_ring_ctrl_subtype);
       return;
    }

    HTT_STATS_PRINT(FATAL, "HTT_RX_PDEV_FW_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__word & 0xFF);
    /*HTT_STATS_PRINT(FATAL, "word = %u",
            ((htt_stats_buf->mac_id__word & 0xFFFFFF00) >> 8 ));*/
    HTT_STATS_PRINT(FATAL, "ppdu_recvd = %u",
            htt_stats_buf->ppdu_recvd);
    HTT_STATS_PRINT(FATAL, "mpdu_cnt_fcs_ok = %u",
            htt_stats_buf->mpdu_cnt_fcs_ok);
    HTT_STATS_PRINT(FATAL, "mpdu_cnt_fcs_err = %u",
            htt_stats_buf->mpdu_cnt_fcs_err);
    HTT_STATS_PRINT(FATAL, "tcp_msdu_cnt = %u",
            htt_stats_buf->tcp_msdu_cnt);
    HTT_STATS_PRINT(FATAL, "tcp_ack_msdu_cnt = %u",
            htt_stats_buf->tcp_ack_msdu_cnt);
    HTT_STATS_PRINT(FATAL, "udp_msdu_cnt = %u",
            htt_stats_buf->udp_msdu_cnt);
    HTT_STATS_PRINT(FATAL, "other_msdu_cnt = %u",
            htt_stats_buf->other_msdu_cnt);
    HTT_STATS_PRINT(FATAL, "fw_ring_mpdu_ind = %u",
            htt_stats_buf->fw_ring_mpdu_ind);

    for (i = 0; i < HTT_STATS_SUBTYPE_MAX; i++) {
        index += snprintf(&fw_ring_mgmt_subtype[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->fw_ring_mgmt_subtype[i]);
    }

    HTT_STATS_PRINT(FATAL, "fw_ring_mgmt_subtype = %s ", fw_ring_mgmt_subtype);

    index = 0;

    for (i = 0; i < HTT_STATS_SUBTYPE_MAX; i++) {
        index += snprintf(&fw_ring_ctrl_subtype[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->fw_ring_ctrl_subtype[i]);
    }

    HTT_STATS_PRINT(FATAL, "fw_ring_ctrl_subtype = %s ", fw_ring_ctrl_subtype);
    HTT_STATS_PRINT(FATAL, "fw_ring_mcast_data_msdu = %u",
            htt_stats_buf->fw_ring_mcast_data_msdu);
    HTT_STATS_PRINT(FATAL, "fw_ring_bcast_data_msdu = %u",
            htt_stats_buf->fw_ring_bcast_data_msdu);
    HTT_STATS_PRINT(FATAL, "fw_ring_ucast_data_msdu = %u",
            htt_stats_buf->fw_ring_ucast_data_msdu);
    HTT_STATS_PRINT(FATAL, "fw_ring_null_data_msdu = %u",
            htt_stats_buf->fw_ring_null_data_msdu);
    HTT_STATS_PRINT(FATAL, "fw_ring_mpdu_drop = %u",
            htt_stats_buf->fw_ring_mpdu_drop);
    HTT_STATS_PRINT(FATAL, "ofld_local_data_ind_cnt = %u",
            htt_stats_buf->ofld_local_data_ind_cnt);
    HTT_STATS_PRINT(FATAL, "ofld_local_data_buf_recycle_cnt = %u",
            htt_stats_buf->ofld_local_data_buf_recycle_cnt);
    HTT_STATS_PRINT(FATAL, "drx_local_data_ind_cnt = %u",
            htt_stats_buf->drx_local_data_ind_cnt);
    HTT_STATS_PRINT(FATAL, "drx_local_data_buf_recycle_cnt = %u",
            htt_stats_buf->drx_local_data_buf_recycle_cnt);
    HTT_STATS_PRINT(FATAL, "local_nondata_ind_cnt = %u",
            htt_stats_buf->local_nondata_ind_cnt);
    HTT_STATS_PRINT(FATAL, "local_nondata_buf_recycle_cnt = %u",
            htt_stats_buf->local_nondata_buf_recycle_cnt);
    HTT_STATS_PRINT(FATAL, "fw_status_buf_ring_refill_cnt = %u",
            htt_stats_buf->fw_status_buf_ring_refill_cnt);
    HTT_STATS_PRINT(FATAL, "fw_status_buf_ring_empty_cnt = %u",
            htt_stats_buf->fw_status_buf_ring_empty_cnt);
    HTT_STATS_PRINT(FATAL, "fw_pkt_buf_ring_refill_cnt = %u",
            htt_stats_buf->fw_pkt_buf_ring_refill_cnt);
    HTT_STATS_PRINT(FATAL, "fw_pkt_buf_ring_empty_cnt = %u",
            htt_stats_buf->fw_pkt_buf_ring_empty_cnt);
    HTT_STATS_PRINT(FATAL, "fw_link_buf_ring_refill_cnt = %u",
            htt_stats_buf->fw_link_buf_ring_refill_cnt);
    HTT_STATS_PRINT(FATAL, "fw_link_buf_ring_empty_cnt = %u",
            htt_stats_buf->fw_link_buf_ring_empty_cnt);
    HTT_STATS_PRINT(FATAL, "host_pkt_buf_ring_refill_cnt = %u",
            htt_stats_buf->host_pkt_buf_ring_refill_cnt);
    HTT_STATS_PRINT(FATAL, "host_pkt_buf_ring_empty_cnt = %u",
            htt_stats_buf->host_pkt_buf_ring_empty_cnt);
    HTT_STATS_PRINT(FATAL, "mon_pkt_buf_ring_refill_cnt = %u",
            htt_stats_buf->mon_pkt_buf_ring_refill_cnt);
    HTT_STATS_PRINT(FATAL, "mon_pkt_buf_ring_empty_cnt = %u",
            htt_stats_buf->mon_pkt_buf_ring_empty_cnt);
    HTT_STATS_PRINT(FATAL, "mon_status_buf_ring_refill_cnt = %u",
            htt_stats_buf->mon_status_buf_ring_refill_cnt);
    HTT_STATS_PRINT(FATAL, "mon_status_buf_ring_empty_cnt = %u",
            htt_stats_buf->mon_status_buf_ring_empty_cnt);
    HTT_STATS_PRINT(FATAL, "mon_desc_buf_ring_refill_cnt = %u",
            htt_stats_buf->mon_desc_buf_ring_refill_cnt);
    HTT_STATS_PRINT(FATAL, "mon_desc_buf_ring_empty_cnt = %u",
            htt_stats_buf->mon_desc_buf_ring_empty_cnt);
    HTT_STATS_PRINT(FATAL, "mon_dest_ring_update_cnt = %u",
            htt_stats_buf->mon_dest_ring_update_cnt);
    HTT_STATS_PRINT(FATAL, "mon_dest_ring_full_cnt = %u",
            htt_stats_buf->mon_dest_ring_full_cnt);
    HTT_STATS_PRINT(FATAL, "rx_suspend_cnt = %u",
            htt_stats_buf->rx_suspend_cnt);
    HTT_STATS_PRINT(FATAL, "rx_suspend_fail_cnt = %u",
            htt_stats_buf->rx_suspend_fail_cnt);
    HTT_STATS_PRINT(FATAL, "rx_resume_cnt = %u",
            htt_stats_buf->rx_resume_cnt);
    HTT_STATS_PRINT(FATAL, "rx_resume_fail_cnt = %u",
            htt_stats_buf->rx_resume_fail_cnt);
    HTT_STATS_PRINT(FATAL, "rx_ring_switch_cnt = %u",
            htt_stats_buf->rx_ring_switch_cnt);
    HTT_STATS_PRINT(FATAL, "rx_ring_restore_cnt = %u",
            htt_stats_buf->rx_ring_restore_cnt);
    HTT_STATS_PRINT(FATAL, "rx_flush_cnt = %u",
            htt_stats_buf->rx_flush_cnt);
    HTT_STATS_PRINT(FATAL, "rx_recovery_reset_cnt = %u\n",
            htt_stats_buf->rx_recovery_reset_cnt);

    free(fw_ring_mgmt_subtype);
    free(fw_ring_ctrl_subtype);
}

/*
 * htt_print_rx_pdev_fw_ring_mpdu_err_tlv_v: display
 *                htt_rx_pdev_fw_ring_mpdu_err_tlv_v
 * @tag_buf: buffer containing the tlv htt_rx_pdev_fw_ring_mpdu_err_tlv_v
 *
 * return:void
 */
static void htt_print_rx_pdev_fw_ring_mpdu_err_tlv_v(A_UINT32 *tag_buf)
{
    htt_rx_pdev_fw_ring_mpdu_err_tlv_v *htt_stats_buf =
        (htt_rx_pdev_fw_ring_mpdu_err_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                                = 0;
    A_CHAR   fw_ring_mpdu_err[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_RX_PDEV_FW_RING_MPDU_ERR_TLV_V:");

    for (i = 0; i < HTT_RX_STATS_RXDMA_MAX_ERR; i++) {
        index += snprintf(&fw_ring_mpdu_err[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i,
                htt_stats_buf->fw_ring_mpdu_err[i]);
    }

    HTT_STATS_PRINT(FATAL, "fw_ring_mpdu_err = %s\n", fw_ring_mpdu_err);
}

/*
 * htt_print_rx_pdev_fw_mpdu_drop_tlv_v: display htt_rx_pdev_fw_mpdu_drop_tlv_v
 * @tag_buf: buffer containing the tlv htt_rx_pdev_fw_mpdu_drop_tlv_v
 *
 * return:void
 */
static void htt_print_rx_pdev_fw_mpdu_drop_tlv_v(A_UINT32 *tag_buf)
{
    htt_rx_pdev_fw_mpdu_drop_tlv_v *htt_stats_buf =
        (htt_rx_pdev_fw_mpdu_drop_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                            = 0;
    A_CHAR   fw_mpdu_drop[HTT_MAX_STRING_LEN] = {0};
    A_UINT32 tag_len                          = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_RX_PDEV_FW_MPDU_DROP_TLV_V:");

    for (i = 0; i < tag_len; i++) {
        index += snprintf(&fw_mpdu_drop[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->fw_mpdu_drop[i]);
    }

    HTT_STATS_PRINT(FATAL, "fw_mpdu_drop = %s\n", fw_mpdu_drop);
}

/*
 * htt_print_tx_pdev_ap_edca_params_stats_tlv_v: display htt_tx_pdev_ap_edca_params_stats_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_pdev_ap_edca_params_stats_tlv_v
 *
 * return:void
 */
static void htt_print_tx_pdev_ap_edca_params_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_pdev_ap_edca_params_stats_tlv_v *htt_stats_buf =
        (htt_tx_pdev_ap_edca_params_stats_tlv_v *)tag_buf;
    A_UINT8 i;

    HTT_STATS_PRINT(FATAL, "\nAP EDCA PARAMETERS FOR UL MUMIMO:\n");
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        HTT_STATS_PRINT(FATAL, "ul_mumimo_less_aggressive[%u] = %u",
                i, htt_stats_buf->ul_mumimo_less_aggressive[i]);
        HTT_STATS_PRINT(FATAL, "ul_mumimo_medium_aggressive[%u] = %u",
                i, htt_stats_buf->ul_mumimo_medium_aggressive[i]);
        HTT_STATS_PRINT(FATAL, "ul_mumimo_highly_aggressive[%u] = %u",
                i, htt_stats_buf->ul_mumimo_highly_aggressive[i]);
        HTT_STATS_PRINT(FATAL, "ul_mumimo_default_relaxed[%u] = %u\n",
                i, htt_stats_buf->ul_mumimo_default_relaxed[i]);
    }

    HTT_STATS_PRINT(FATAL, "AP EDCA PARAMETERS FOR UL OFDMA:\n");
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        HTT_STATS_PRINT(FATAL, "ul_muofdma_less_aggressive[%u] = %u",
                i, htt_stats_buf->ul_muofdma_less_aggressive[i]);
        HTT_STATS_PRINT(FATAL, "ul_muofdma_medium_aggressive[%u] = %u",
                i, htt_stats_buf->ul_muofdma_medium_aggressive[i]);
        HTT_STATS_PRINT(FATAL, "ul_muofdma_highly_aggressive[%u] = %u",
                i, htt_stats_buf->ul_muofdma_highly_aggressive[i]);
        HTT_STATS_PRINT(FATAL, "ul_muofdma_default_relaxed[%u] = %u\n",
                i, htt_stats_buf->ul_muofdma_default_relaxed[i]);
    }
}

/* DEPRECATED
 * htt_print_tx_pdev_muedca_params_stats_tlv_v: display htt_tx_pdev_muedca_params_stats_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_pdev_muedca_params_stats_tlv_v
 *
 * return:void
 */
static void htt_print_tx_pdev_muedca_params_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_pdev_muedca_params_stats_tlv_v *htt_stats_buf =
        (htt_tx_pdev_muedca_params_stats_tlv_v *)tag_buf;
    A_UINT8  i, j;
    A_UINT16 index       = 0;
    A_CHAR   *edca_buf   = NULL;

    edca_buf = (A_CHAR *)malloc(HTT_MAX_STRING_LEN);
    if (!edca_buf) {
        HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
        return;
    }

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_MUEDCA_PARAMS_STATS_TLV:");

    for (j = 0; j < HTT_NUM_AC_WMM; j++) {
        index = 0;
        memset(edca_buf, 0x0, HTT_MAX_STRING_LEN);
        for (i = 0; i < HTT_STATS_MUEDCA_VALUE_MAX; i++) {
            index += snprintf(&edca_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->aifs[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "aifs[%u] = %s ", j, edca_buf);

        index = 0;
        memset(edca_buf, 0x0, HTT_MAX_STRING_LEN);
        for (i = 0; i < HTT_STATS_MUEDCA_VALUE_MAX; i++) {
            index += snprintf(&edca_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->cw_min[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "cw_min[%u] = %s ", j, edca_buf);

        index = 0;
        memset(edca_buf, 0x0, HTT_MAX_STRING_LEN);
        for (i = 0; i < HTT_STATS_MUEDCA_VALUE_MAX; i++) {
            index += snprintf(&edca_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->cw_max[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "cw_max[%u] = %s ", j, edca_buf);
    }

    free(edca_buf);
}

/*
 * htt_print_tx_pdev_mu_edca_params_stats_tlv_v: display htt_tx_pdev_mu_edca_params_stats_tlv_v
 * @tag_buf: buffer containing the tlv htt_tx_pdev_mu_edca_params_stats_tlv_v
 *
 * return:void
 */
static void htt_print_tx_pdev_mu_edca_params_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_tx_pdev_mu_edca_params_stats_tlv_v *htt_stats_buf =
        (htt_tx_pdev_mu_edca_params_stats_tlv_v *)tag_buf;
    A_UINT8  i;
    A_UINT16 index     = 0;
    A_CHAR   *edca_buf = NULL;

    edca_buf = (A_CHAR *)malloc(HTT_MAX_STRING_LEN);
    if (!edca_buf) {
        HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
        return;
    }

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_MU_EDCA_PARAMS_STATS_TLV:\n");

    memset(edca_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&edca_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->relaxed_mu_edca[i]);
    }
    HTT_STATS_PRINT(FATAL, "relaxed_mu_edca = %s", edca_buf);

    index = 0;
    memset(edca_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&edca_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->mumimo_aggressive_mu_edca[i]);
    }
    HTT_STATS_PRINT(FATAL, "mumimo_aggressive_mu_edca = %s", edca_buf);

    index = 0;
    memset(edca_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&edca_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->mumimo_relaxed_mu_edca[i]);
    }
    HTT_STATS_PRINT(FATAL, "mumimo_relaxed_mu_edca = %s", edca_buf);

    index = 0;
    memset(edca_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&edca_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->muofdma_aggressive_mu_edca[i]);
    }
    HTT_STATS_PRINT(FATAL, "muofdma_aggressive_mu_edca = %s", edca_buf);
    index = 0;
    memset(edca_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&edca_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->muofdma_relaxed_mu_edca[i]);
    }
    HTT_STATS_PRINT(FATAL, "muofdma_relaxed_mu_edca = %s", edca_buf);
    index = 0;
    memset(edca_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&edca_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->latency_mu_edca[i]);
    }
    HTT_STATS_PRINT(FATAL, "latency_mu_edca = %s", edca_buf);
    index = 0;
    memset(edca_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&edca_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->psd_boost_mu_edca[i]);
    }
    HTT_STATS_PRINT(FATAL, "psd_boost_mu_edca = %s\n", edca_buf);

    free(edca_buf);
}

/*
 * htt_print_rx_pdev_fw_stats_phy_err_tlv: display htt_rx_pdev_fw_stats_phy_err_tlv
 * @tag_buf: buffer containing the tlv htt_rx_pdev_fw_stats_phy_err_tlv
 *
 * return:void
 */
static void htt_print_rx_pdev_fw_stats_phy_err_tlv(A_UINT32 *tag_buf)
{
    htt_rx_pdev_fw_stats_phy_err_tlv *htt_stats_buf =
        (htt_rx_pdev_fw_stats_phy_err_tlv *)tag_buf;
    A_UINT8  i;
    A_UINT16 index                        = 0;
    A_CHAR   phy_errs[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_RX_PDEV_FW_STATS_PHY_ERR_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id__word = %u",
            htt_stats_buf->mac_id__word);
    HTT_STATS_PRINT(FATAL, "tota_phy_err_nct = %u",
            htt_stats_buf->total_phy_err_cnt);

    for (i = 0; i < HTT_STATS_PHY_ERR_MAX; i++) {
        index += snprintf(&phy_errs[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->phy_err[i]);
    }

    HTT_STATS_PRINT(FATAL, "phy_errs = %s\n", phy_errs);
}

/*
 * htt_print_pdev_cca_stats_hist_tlv: display htt_pdev_cca_stats_hist_v1_tlv
 * @tag_buf: buffer containing the tlv htt_pdev_cca_stats_hist_v1_tlv
 *
 * return:void
 */
static void htt_print_pdev_cca_stats_hist_tlv(A_UINT32 *tag_buf)
{
    htt_pdev_cca_stats_hist_v1_tlv *htt_stats_buf =
        (htt_pdev_cca_stats_hist_v1_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "\nHTT_PDEV_CCA_STATS_HIST_TLV:");
    HTT_STATS_PRINT(FATAL, "chan_num = %u",
            htt_stats_buf->chan_num);
    HTT_STATS_PRINT(FATAL, "num_records = %u",
            htt_stats_buf->num_records);
    HTT_STATS_PRINT(FATAL, "valid_cca_counters_bitmap = 0x%x",
            htt_stats_buf->valid_cca_counters_bitmap);
    HTT_STATS_PRINT(FATAL, "collection_interval = %u\n",
            htt_stats_buf->collection_interval);

    HTT_STATS_PRINT(FATAL, "HTT_PDEV_STATS_CCA_COUNTERS_TLV:(in usec)");
    HTT_STATS_PRINT(FATAL,
            "|  tx_frame|   rx_frame|   rx_clear| my_rx_frame|pre_rx_frame|        cnt| med_rx_idle| med_tx_idle_global|   cca_obss|");
}

/*
 * htt_print_pdev_stats_cca_counters_tlv: display htt_pdev_stats_cca_counters_tlv
 * @tag_buf: buffer containing the tlv htt_pdev_stats_cca_counters_tlv
 *
 * return:void
 */
static void htt_print_pdev_stats_cca_counters_tlv(A_UINT32 *tag_buf)
{
    htt_pdev_stats_cca_counters_tlv *htt_stats_buf =
        (htt_pdev_stats_cca_counters_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL,
        "|%10u| %10u| %10u| %11u|%12u| %10u| %11u| %18u| %10u|",
        htt_stats_buf->tx_frame_usec,
        htt_stats_buf->rx_frame_usec,
        htt_stats_buf->rx_clear_usec,
        htt_stats_buf->my_rx_frame_usec,
        htt_stats_buf->pre_rx_frame_usec,
        htt_stats_buf->usec_cnt,
        htt_stats_buf->med_rx_idle_usec,
        htt_stats_buf->med_tx_idle_global_usec,
        htt_stats_buf->cca_obss_usec);
}

/*
 * htt_print_hw_stats_whal_tx_tlv: display htt_hw_stats_whal_tx_tlv
 * @tag_buf: buffer containing the tlv htt_hw_stats_pdev_errs_tlv
 *
 * return:void
 */
static void htt_print_hw_stats_whal_tx_tlv(A_UINT32 *tag_buf)
{
    htt_hw_stats_whal_tx_tlv *htt_stats_buf =
        (htt_hw_stats_whal_tx_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_HW_STATS_WHAL_TX_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__word & 0xFF);
    /*HTT_STATS_PRINT(FATAL, "word = %u",
            ((htt_stats_buf->mac_id__word & 0xFFFFFF00) >> 8 ));*/
    HTT_STATS_PRINT(FATAL, "last_unpause_ppdu_id = %u",
            htt_stats_buf->last_unpause_ppdu_id);
    HTT_STATS_PRINT(FATAL, "hwsch_unpause_wait_tqm_write = %u",
            htt_stats_buf->hwsch_unpause_wait_tqm_write);
    HTT_STATS_PRINT(FATAL, "hwsch_dummy_tlv_skipped = %u",
            htt_stats_buf->hwsch_dummy_tlv_skipped);
    HTT_STATS_PRINT(FATAL, "hwsch_misaligned_offset_received = %u",
            htt_stats_buf->hwsch_misaligned_offset_received);
    HTT_STATS_PRINT(FATAL, "hwsch_reset_count = %u",
            htt_stats_buf->hwsch_reset_count);
    HTT_STATS_PRINT(FATAL, "hwsch_dev_reset_war = %u",
            htt_stats_buf->hwsch_dev_reset_war);
    HTT_STATS_PRINT(FATAL, "hwsch_delayed_pause = %u",
            htt_stats_buf->hwsch_delayed_pause);
    HTT_STATS_PRINT(FATAL, "hwsch_long_delayed_pause = %u",
            htt_stats_buf->hwsch_long_delayed_pause);
    HTT_STATS_PRINT(FATAL, "sch_rx_ppdu_no_response = %u",
            htt_stats_buf->sch_rx_ppdu_no_response);
    HTT_STATS_PRINT(FATAL, "sch_selfgen_response = %u",
            htt_stats_buf->sch_selfgen_response);
    HTT_STATS_PRINT(FATAL, "sch_rx_sifs_resp_trigger= %u\n",
            htt_stats_buf->sch_rx_sifs_resp_trigger);
}

/*
 * htt_print_stats_whal_wsi_tlv: display htt_stats_whal_wsi_tlv
 * @tag_buf: buffer containing the tlv htt_stats_whal_wsi_tlv
 *
 * return:void
 */
static void htt_print_stats_whal_wsib_tlv(A_UINT32 *tag_buf)
{
    htt_stats_whal_wsi_tlv *htt_stats_buf = (htt_stats_whal_wsi_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_STATS_WHAL_WSI_TLV:");
    HTT_STATS_PRINT(FATAL, "wsib_event_watchdog_timeout= %u",
        htt_stats_buf->wsib_event_watchdog_timeout);
    HTT_STATS_PRINT(FATAL, "wsib_event_slave_tlv_length_error = %u",
        htt_stats_buf->wsib_event_slave_tlv_length_error);
    HTT_STATS_PRINT(FATAL, "wsib_event_slave_parity_error = %u",
        htt_stats_buf->wsib_event_slave_parity_error);
    HTT_STATS_PRINT(FATAL, "wsib_event_slave_direct_message = %u",
        htt_stats_buf->wsib_event_slave_direct_message);
    HTT_STATS_PRINT(FATAL, "wsib_event_slave_backpressure_error = %u",
        htt_stats_buf->wsib_event_slave_backpressure_error);
    HTT_STATS_PRINT(FATAL, "wsib_event_master_tlv_length_error = %u\n",
        htt_stats_buf->wsib_event_master_tlv_length_error);
}

/*
 * htt_print_pdev_stats_twt_sessions_tlv: display htt_pdev_stats_twt_sessions_tlv
 * @tag_buf: buffer containing the tlv htt_pdev_stats_twt_sessions_tlv
 *
 * return:void
 */
static void htt_print_pdev_stats_twt_sessions_tlv(A_UINT32 *tag_buf)
{
    htt_pdev_stats_twt_sessions_tlv *htt_stats_buf =
        (htt_pdev_stats_twt_sessions_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_PDEV_STATS_TWT_SESSIONS_TLV:");
    HTT_STATS_PRINT(FATAL, "pdev_id = %u",
            htt_stats_buf->pdev_id);
    HTT_STATS_PRINT(FATAL, "num_sessions = %u\n",
            htt_stats_buf->num_sessions);
}

/*
 * htt_print_pdev_stats_twt_session_tlv: display htt_pdev_stats_twt_session_tlv
 * @tag_buf: buffer containing the tlv htt_pdev_stats_twt_session_tlv
 *
 * return:void
 */
static void htt_print_pdev_stats_twt_session_tlv(A_UINT32 *tag_buf)
{
    htt_pdev_stats_twt_session_tlv *htt_stats_buf =
        (htt_pdev_stats_twt_session_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_PDEV_STATS_TWT_SESSION_TLV:");
    HTT_STATS_PRINT(FATAL, "vdev_id = %u",
            htt_stats_buf->vdev_id);
    HTT_STATS_PRINT(FATAL, "peer_mac = %02x:%02x:%02x:%02x:%02x:%02x",
            htt_stats_buf->peer_mac.mac_addr31to0 & 0xFF,
            (htt_stats_buf->peer_mac.mac_addr31to0 & 0xFF00) >> 8,
            (htt_stats_buf->peer_mac.mac_addr31to0 & 0xFF0000) >> 16,
            (htt_stats_buf->peer_mac.mac_addr31to0 & 0xFF000000) >> 24,
            (htt_stats_buf->peer_mac.mac_addr47to32 & 0xFF),
            (htt_stats_buf->peer_mac.mac_addr47to32 & 0xFF00) >> 8);
    HTT_STATS_PRINT(FATAL, "flow_id_flags = %#x",
            htt_stats_buf->flow_id_flags);
    HTT_STATS_PRINT(FATAL, "dialog_id = %u",
            htt_stats_buf->dialog_id);
    HTT_STATS_PRINT(FATAL, "wake_dura_us = %u",
            htt_stats_buf->wake_dura_us);
    HTT_STATS_PRINT(FATAL, "wake_intvl_us = %u",
            htt_stats_buf->wake_intvl_us);
    HTT_STATS_PRINT(FATAL, "sp_offset_us = %u\n",
            htt_stats_buf->sp_offset_us);
}

/*
 * htt_print_hw_war_tlv_v: display htt_hw_war_stats_tlv_v
 * @tag_buf: buffer containing the tlv htt_hw_war_stats_tlv
 *
 * return:void
 */
static void htt_print_hw_war_tlv_v(A_UINT32 *tag_buf)
{
    htt_hw_war_stats_tlv *htt_stats_buf =
        (htt_hw_war_stats_tlv *)tag_buf;

    A_UINT8  i;
    A_UINT32 tag_words = (HTT_STATS_TLV_LENGTH_GET(*tag_buf) >> 2);

    HTT_STATS_PRINT(FATAL, "HTT_HW_WAR_STATS_TLV:");

    tag_words--; /* first word beyond TLV header is for mac_id */
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__word & 0xFF);

    for (i = 0; i < tag_words; i++) {
        HTT_STATS_PRINT(FATAL, "hw_war %u = %u\n",
                i, htt_stats_buf->hw_wars[i]);
    }
}

/*
 * htt_pdev_obss_pd_stats_tlv: display htt_pdev_obss_pd_stats
 * @tag_buf: buffer containing the tlv htt_pdev_obss_pd_stats_tlv
 *
 * return:void
 */
static void htt_print_pdev_obss_pd_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_pdev_obss_pd_stats_tlv *htt_stats_buf =
        (htt_pdev_obss_pd_stats_tlv *)tag_buf;
    A_UINT8 i;
    char *access_cat_names[HTT_NUM_AC_WMM] = {
        "best effort",
        "background",
        "video",
        "voice",
    };

    HTT_STATS_PRINT(FATAL, "HTT_PDEV_OBSS_PD_STATS_TLV:");
    /*
     * Successful/Failure OBSS Transmission stats are commented out as they
     * are not supported in the current chipsets.
     * Based on MAC team suggestion, the stats will be supported in future.
     */
    #if 0
    HTT_STATS_PRINT(FATAL, "OBSS Tx success PPDU = %u",
            htt_stats_buf->num_obss_tx_ppdu_success);
    HTT_STATS_PRINT(FATAL, "OBSS Tx failures PPDU = %u\n",
            htt_stats_buf->num_obss_tx_ppdu_failure);
    #endif
    HTT_STATS_PRINT(FATAL, "num_spatial_reuse_tx = %u",
            htt_stats_buf->num_sr_tx_transmissions);
    HTT_STATS_PRINT(FATAL, "num_spatial_reuse_opportunities = %u",
            htt_stats_buf->num_spatial_reuse_opportunities);

    HTT_STATS_PRINT(FATAL, "num_non_srg_opportunities = %u",
            htt_stats_buf->num_non_srg_opportunities);
    HTT_STATS_PRINT(FATAL, "num_non_srg_ppdu_tried = %u",
            htt_stats_buf->num_non_srg_ppdu_tried);
    HTT_STATS_PRINT(FATAL, "num_non_srg_ppdu_success = %u",
            htt_stats_buf->num_non_srg_ppdu_success);

    HTT_STATS_PRINT(FATAL, "num_srg_opportunities = %u",
            htt_stats_buf->num_srg_opportunities);
    HTT_STATS_PRINT(FATAL, "num_srg_ppdu_tried = %u",
            htt_stats_buf->num_srg_ppdu_tried);
    HTT_STATS_PRINT(FATAL, "num_srg_ppdu_success = %u",
            htt_stats_buf->num_srg_ppdu_success);

    HTT_STATS_PRINT(FATAL, "num_psr_opportunities = %u",
            htt_stats_buf->num_psr_opportunities);
    HTT_STATS_PRINT(FATAL, "num_psr_ppdu_tried = %u",
            htt_stats_buf->num_psr_ppdu_tried);
    HTT_STATS_PRINT(FATAL, "num_psr_ppdu_success = %u",
            htt_stats_buf->num_psr_ppdu_success);

    HTT_STATS_PRINT(FATAL, "min_duration_check_flush_cnt = %u",
            htt_stats_buf->num_obss_min_duration_check_flush_cnt);
    HTT_STATS_PRINT(FATAL, "sr_ppdu_abort_flush_cnt = %u",
            htt_stats_buf->num_sr_ppdu_abort_flush_cnt);

    HTT_STATS_PRINT(FATAL, "HTT_PDEV_OBSS_PD_PER_AC_STATS:");

    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
         HTT_STATS_PRINT(FATAL, "Access Category %u (%s)",
                i, access_cat_names[i]);
         HTT_STATS_PRINT(FATAL, "num_non_srg_ppdu_tried = %u",
                 htt_stats_buf->num_non_srg_ppdu_tried_per_ac[i]);
         HTT_STATS_PRINT(FATAL, "num_non_srg_ppdu_success = %u",
                 htt_stats_buf->num_non_srg_ppdu_success_per_ac[i]);
         HTT_STATS_PRINT(FATAL, "num_srg_ppdu_tried = %u",
                 htt_stats_buf->num_srg_ppdu_tried_per_ac[i]);
         HTT_STATS_PRINT(FATAL, "num_srg_ppdu_success = %u",
                 htt_stats_buf->num_srg_ppdu_success_per_ac[i]);
    }
    HTT_STATS_PRINT(FATAL, "\n");
}

static void htt_print_soc_latency_prof_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_latency_prof_stats_tlv *htt_stats_buf =
        (htt_latency_prof_stats_tlv *) tag_buf;
    A_CHAR latency_prof_stat_name[HTT_STATS_MAX_PROF_STATS_NAME_LEN + 1] = {0};
    int page_fault_avg = 0;

    if (htt_stats_buf->print_header == 1) {
        HTT_STATS_PRINT(FATAL, "HTT_STATS_LATENCY_PROF_TLV:");
        HTT_STATS_PRINT(FATAL,
            "|%-32s|%8s|%15s|%15s|%8s|%15s|"
            "%8s|%8s|%15s|%8s|%15s|"
            "%15s|%26s|%8s|%8s|%8s|"
            "%10s|%17s|%6s|",
            "prof_name", "cnt","min", "min_pcycles", "max", "max_pcycles",
            "last", "tot", "tot_pcycles", "avg", "avg_pcycles",
            "hist_intvl", "hist", "pf_max", "pf_avg", "pf_tot",
            "ignoredCnt", "intHist", "intMax");
    }
    if (htt_stats_buf->cnt) {
        page_fault_avg = htt_stats_buf->page_fault_total/htt_stats_buf->cnt;
    }

    memcpy(
        latency_prof_stat_name,
        &(htt_stats_buf->latency_prof_name[0]),
        HTT_STATS_MAX_PROF_STATS_NAME_LEN);
    HTT_STATS_PRINT(FATAL,
        "|%-32s|%8u|%15u|%15u|%8u|%15u|"
        "%8u|%8u|%15u|%8u|%15u|"
        "%15u|%8u:%8u:%8u|%8u|%8u|%8u|"
        "%10u|%5u:%5u:%5u|%6u|",
        latency_prof_stat_name, htt_stats_buf->cnt,
        htt_stats_buf->min, htt_stats_buf->min_pcycles_time,
        htt_stats_buf->max, htt_stats_buf->max_pcycles_time,
        htt_stats_buf->last,
        htt_stats_buf->tot, htt_stats_buf->tot_pcycles_time,
        htt_stats_buf->avg, htt_stats_buf->avg_pcycles_time,
        htt_stats_buf->hist_intvl,
        htt_stats_buf->hist[0], htt_stats_buf->hist[1], htt_stats_buf->hist[2],
        htt_stats_buf->page_fault_max, page_fault_avg,
        htt_stats_buf->page_fault_total, htt_stats_buf->ignored_latency_count,
        htt_stats_buf->interrupts_hist[0], htt_stats_buf->interrupts_hist[1],
        htt_stats_buf->interrupts_hist[2], htt_stats_buf->interrupts_max);
}

static void htt_print_soc_latency_prof_ctx_tlv(A_UINT32 *tag_buf)
{
    htt_latency_prof_ctx_tlv *htt_stats_buf =
        (htt_latency_prof_ctx_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_STATS_LATENCY_CTX_TLV:");
    HTT_STATS_PRINT(FATAL, "duration= %u", htt_stats_buf->duration);
    HTT_STATS_PRINT(FATAL, "tx_msdu_cnt = %u", htt_stats_buf->tx_msdu_cnt);
    HTT_STATS_PRINT(FATAL, "tx_mpdu_cnt = %u", htt_stats_buf->tx_mpdu_cnt);
    HTT_STATS_PRINT(FATAL, "rx_msdu_cnt = %u", htt_stats_buf->rx_msdu_cnt);
    HTT_STATS_PRINT(FATAL, "rx_mpdu_cnt = %u", htt_stats_buf->rx_mpdu_cnt);
}

static void htt_print_latency_prof_cnt(A_UINT32 *tag_buf)
{
    htt_latency_prof_cnt_tlv *htt_stats_buf =
        (htt_latency_prof_cnt_tlv *)tag_buf;
    HTT_STATS_PRINT(FATAL,
        "prof_enable_cnt = %u", htt_stats_buf->prof_enable_cnt);
}

/*
 * htt_ring_backpressure_stats_tlv : display htt_ring_backpressure_stats
 * @tag_buf: buffer containing the tlv htt_ring_backpressure_stats_tlv
 *
 * return:void
 */
static void
htt_print_pdev_ring_backpressure_stats_tlv_v(A_UINT32 *tag_buf)
{
    htt_ring_backpressure_stats_tlv *htt_stats_buf =
        (htt_ring_backpressure_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "pdev_id = %u\n",
            htt_stats_buf->pdev_id);
    HTT_STATS_PRINT(FATAL, "Head index = %u\n",
            htt_stats_buf->current_head_idx);
    HTT_STATS_PRINT(FATAL, "Tail index = %u\n",
            htt_stats_buf->current_tail_idx);
    HTT_STATS_PRINT(FATAL, "Num Backpressure Msgs Sent = %u\n",
            htt_stats_buf->num_htt_msgs_sent);
    HTT_STATS_PRINT(FATAL, "Current Backpressure Time in Milliseconds = %u\n",
            htt_stats_buf->backpressure_time_ms);
    HTT_STATS_PRINT(FATAL, "Ring Backpressure Histogram \n");
    HTT_STATS_PRINT(FATAL,
            "100ms to 200ms = %u, 200ms to 300ms = %u, 300ms to 400ms = %u\n",
            htt_stats_buf->backpressure_hist[0],
            htt_stats_buf->backpressure_hist[1],
            htt_stats_buf->backpressure_hist[2]);
    HTT_STATS_PRINT(FATAL,
            "400ms to 500 ms = %u, Above 500ms = %u\n",
            htt_stats_buf->backpressure_hist[3],
            htt_stats_buf->backpressure_hist[4]);
}

static void htt_print_rx_fse_stats_tlv(A_UINT32 *tag_buf)
{
    htt_rx_fse_stats_tlv *htt_stats_buf =
        (htt_rx_fse_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_STATS_RX_FSE_STATS_TLV: \n");

    HTT_STATS_PRINT(FATAL, "=== Software RX FSE STATS ===\n");
    HTT_STATS_PRINT(FATAL,
            "Enable count  = %u", htt_stats_buf->fse_enable_cnt);
    HTT_STATS_PRINT(FATAL,
            "Disable count = %u\n", htt_stats_buf->fse_disable_cnt);

    HTT_STATS_PRINT(FATAL,
            "Cache Invalidate Entry Count   = %u\n",
            htt_stats_buf->fse_cache_invalidate_entry_cnt);
    HTT_STATS_PRINT(FATAL,
            "Full Cache Invalidate Count    = %u\n",
            htt_stats_buf->fse_full_cache_invalidate_cnt);

    HTT_STATS_PRINT(FATAL, "=== Hardware RX FSE STATS ===\n");
    HTT_STATS_PRINT(FATAL, "Cache hits Count = %u\n",
            htt_stats_buf->fse_num_cache_hits_cnt);
    HTT_STATS_PRINT(FATAL, "Cache No. of searches = %u\n",
            htt_stats_buf->fse_num_searches_cnt);
    HTT_STATS_PRINT(FATAL, "Cache occupancy Peak Count: \n");
    HTT_STATS_PRINT(FATAL,
             " [0] = %u [1-16] = %u [17-32] = %u "
             "[33-48] = %u [49-64] = %u [65-80] = %u "
             "[81-96] = %u [97-112] = %u [113-127] = %u "
             "[128] = %u\n",
             htt_stats_buf->fse_cache_occupancy_peak_cnt[0],
             htt_stats_buf->fse_cache_occupancy_peak_cnt[1],
             htt_stats_buf->fse_cache_occupancy_peak_cnt[2],
             htt_stats_buf->fse_cache_occupancy_peak_cnt[3],
             htt_stats_buf->fse_cache_occupancy_peak_cnt[4],
             htt_stats_buf->fse_cache_occupancy_peak_cnt[5],
             htt_stats_buf->fse_cache_occupancy_peak_cnt[6],
             htt_stats_buf->fse_cache_occupancy_peak_cnt[7],
             htt_stats_buf->fse_cache_occupancy_peak_cnt[8],
             htt_stats_buf->fse_cache_occupancy_peak_cnt[9]);
    HTT_STATS_PRINT(FATAL, "Cache occupancy Current Count: \n");
    HTT_STATS_PRINT(FATAL,
             " [0] = %u [1-16] = %u [17-32] = %u "
             "[33-48] = %u [49-64] = %u [65-80] = %u "
             "[81-96] = %u [97-112] = %u [113-127] = %u "
             "[128] = %u\n",
             htt_stats_buf->fse_cache_occupancy_curr_cnt[0],
             htt_stats_buf->fse_cache_occupancy_curr_cnt[1],
             htt_stats_buf->fse_cache_occupancy_curr_cnt[2],
             htt_stats_buf->fse_cache_occupancy_curr_cnt[3],
             htt_stats_buf->fse_cache_occupancy_curr_cnt[4],
             htt_stats_buf->fse_cache_occupancy_curr_cnt[5],
             htt_stats_buf->fse_cache_occupancy_curr_cnt[6],
             htt_stats_buf->fse_cache_occupancy_curr_cnt[7],
             htt_stats_buf->fse_cache_occupancy_curr_cnt[8],
             htt_stats_buf->fse_cache_occupancy_curr_cnt[9]);
    HTT_STATS_PRINT(FATAL, "Cache search Square Count: \n");
    HTT_STATS_PRINT(FATAL,
             " [0] = %u [1-50] = %u [51-100] = %u "
             "[101-200] = %u [201-255] = %u [256] = %u \n",
             htt_stats_buf->fse_search_stat_square_cnt[0],
             htt_stats_buf->fse_search_stat_square_cnt[1],
             htt_stats_buf->fse_search_stat_square_cnt[2],
             htt_stats_buf->fse_search_stat_square_cnt[3],
             htt_stats_buf->fse_search_stat_square_cnt[4],
             htt_stats_buf->fse_search_stat_square_cnt[5]);

    HTT_STATS_PRINT(FATAL, "Cache search Peak Pending Count:  \n");
    HTT_STATS_PRINT(FATAL,
             " [0] = %u [1-2] = %u [3-4] = %u "
             "[Greater/Equal to 5] = %u \n",
             htt_stats_buf->fse_search_stat_peak_cnt[0],
             htt_stats_buf->fse_search_stat_peak_cnt[1],
             htt_stats_buf->fse_search_stat_peak_cnt[2],
             htt_stats_buf->fse_search_stat_peak_cnt[3]);
    HTT_STATS_PRINT(FATAL, "Cache search Number of Pending Count: \n");
    HTT_STATS_PRINT(FATAL,
             "[0] = %u [1-2] = %u [3-4] = %u "
             "[Greater/Equal to 5] = %u \n",
             htt_stats_buf->fse_search_stat_search_pending_cnt[0],
             htt_stats_buf->fse_search_stat_search_pending_cnt[1],
             htt_stats_buf->fse_search_stat_search_pending_cnt[2],
             htt_stats_buf->fse_search_stat_search_pending_cnt[3]);
}

/*
 * htt_print_peer_sched_stats: display htt_peer_sched_stats_tlv
 * @tag_buf: buffer containing the tlv htt_peer_sched_stats_tlv
 *
 * return:void
 */
static void htt_print_peer_sched_stats(A_UINT32 *tag_buf)
{
    htt_peer_sched_stats_tlv *htt_stats_buf =
        (htt_peer_sched_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_PEER_SCHED_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "peer_id = %u",
            htt_stats_buf->peer_id);
    HTT_STATS_PRINT(FATAL, "num_sched_dl = %u",
            htt_stats_buf->num_sched_dl);
    HTT_STATS_PRINT(FATAL, "num_sched_ul = %u",
            htt_stats_buf->num_sched_ul);
#ifdef __KERNEL__
    HTT_STATS_PRINT(FATAL, "peer_tx_active_dur_ms = %llu",
                    qdf_do_div((htt_stats_buf->peer_tx_active_dur_us_low |
                    ((unsigned long long)htt_stats_buf->peer_tx_active_dur_us_high << 32)),
                    1000));
    HTT_STATS_PRINT(FATAL, "peer_rx_active_dur_ms = %llu",
                    qdf_do_div((htt_stats_buf->peer_rx_active_dur_us_low |
                    ((unsigned long long)htt_stats_buf->peer_rx_active_dur_us_high << 32)),
                    1000));
#else
    HTT_STATS_PRINT(FATAL, "peer_tx_active_dur_ms = %llu",
                    ((htt_stats_buf->peer_tx_active_dur_us_low |
                    ((unsigned long long)htt_stats_buf->peer_tx_active_dur_us_high << 32)) /
                    1000));
    HTT_STATS_PRINT(FATAL, "peer_rx_active_dur_ms = %llu",
                    ((htt_stats_buf->peer_rx_active_dur_us_low |
                    ((unsigned long long)htt_stats_buf->peer_rx_active_dur_us_high << 32)) /
                    1000));
#endif
    HTT_STATS_PRINT(FATAL, "peer_curr_rate_kbps = %u\n",
            htt_stats_buf->peer_curr_rate_kbps);
}

/*
 * htt_print_peer_ax_ofdma_stats: display htt_peer_ax_ofdma_stats_tlv
 * @tag_buf: buffer containing the tlv htt_peer_ax_ofdma_stats_tlv
 *
 * return:void
 */
static void htt_print_peer_ax_ofdma_stats(A_UINT32 *tag_buf)
{
    htt_peer_ax_ofdma_stats_tlv *htt_stats_buf =
        (htt_peer_ax_ofdma_stats_tlv *)tag_buf;
    int index, i;
    A_CHAR str_buf[HTT_MAX_STRING_LEN];

    HTT_STATS_PRINT(FATAL, "HTT_PEER_AX_OFDMA_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "peer_id = %u",
        htt_stats_buf->peer_id);
    HTT_STATS_PRINT(FATAL, "ax_basic_trig_count = %u",
        htt_stats_buf->ax_basic_trig_count);
    HTT_STATS_PRINT(FATAL, "ax_basic_trig_err = %u",
        htt_stats_buf->ax_basic_trig_err);
    HTT_STATS_PRINT(FATAL, "ax_bsr_trig_count = %u",
        htt_stats_buf->ax_bsr_trig_count);
    HTT_STATS_PRINT(FATAL, "ax_bsr_trig_err = %u",
        htt_stats_buf->ax_bsr_trig_err);
    HTT_STATS_PRINT(FATAL, "ax_mu_bar_trig_count = %u",
        htt_stats_buf->ax_mu_bar_trig_count);
    HTT_STATS_PRINT(FATAL, "ax_mu_bar_trig_err = %u",
        htt_stats_buf->ax_mu_bar_trig_err);
    HTT_STATS_PRINT(FATAL, "ax_basic_trig_with_per = %u",
        htt_stats_buf->ax_basic_trig_with_per);
    HTT_STATS_PRINT(FATAL, "ax_bsr_trig_with_per = %u",
        htt_stats_buf->ax_bsr_trig_with_per);
    HTT_STATS_PRINT(FATAL, "ax_mu_bar_trig_with_per = %u",
        htt_stats_buf->ax_mu_bar_trig_with_per);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < 2; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->is_airtime_large_for_dl_ofdma[i]);
    }
    HTT_STATS_PRINT(FATAL, "is_airtime_large_for_dl_ofdma = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < 2; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->is_airtime_large_for_ul_ofdma[i]);
    }
    HTT_STATS_PRINT(FATAL, "is_airtime_large_for_ul_ofdma = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->last_updated_dl_qdepth[i]);
    }
    HTT_STATS_PRINT(FATAL, "last_updated_dl_qdepth = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_buf->last_updated_ul_qdepth[i]);
    }
    HTT_STATS_PRINT(FATAL, "last_updated_ul_qdepth = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "ax_manual_ulofdma_trig_count = %u",
        htt_stats_buf->ax_manual_ulofdma_trig_count);
    HTT_STATS_PRINT(FATAL, "ax_manual_ulofdma_trig_err = %u",
        htt_stats_buf->ax_manual_ulofdma_trig_err_count);
}

/*
 * htt_print_peer_be_ofdma_stats: display htt_peer_be_ofdma_stats_tlv
 * @tag_buf: buffer containing the tlv htt_peer_be_ofdma_stats_tlv
 *
 * return:void
 */
static void htt_print_peer_be_ofdma_stats(A_UINT32 *tag_buf)
{
    htt_peer_be_ofdma_stats_tlv *htt_stats_buf =
        (htt_peer_be_ofdma_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_PEER_BE_OFDMA_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "peer_id = %u",
        htt_stats_buf->peer_id);

    HTT_STATS_PRINT(FATAL, "be_manual_ulofdma_trig_count = %u",
        htt_stats_buf->be_manual_ulofdma_trig_count);
    HTT_STATS_PRINT(FATAL, "be_manual_ulofdma_trig_err = %u\n",
        htt_stats_buf->be_manual_ulofdma_trig_err_count);
}

/*
 * htt_print_sta_ul_ofdma_stats: display htt_sta_ul_ofdma_stats_tlv values
 * @tag_buf: buffer containing the tlv htt_sta_ul_ofdma_stats_tlv
 *
 * return:void
 */

static void htt_print_sta_ul_ofdma_stats_tlv(A_UINT32 *tag_buf)
{
    A_UINT32 i, j;
    A_UINT16 index;
    char str_buf[HTT_MAX_STRING_LEN] = {0};

    htt_sta_ul_ofdma_stats_tlv *htt_stats_buf =
              (htt_sta_ul_ofdma_stats_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "==============STA UL OFDMA STATS================\n");

    HTT_STATS_PRINT(FATAL, "pdev ID = %d ", htt_stats_buf->pdev_id);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_STA_UL_OFDMA_NUM_TRIG_TYPE; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rx_trigger_type[i]);
    }
    HTT_STATS_PRINT(FATAL, "STA HW Trigger Type = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    snprintf(str_buf, HTT_MAX_STRING_LEN - index,
              " BASIC:%u, BRPOLL:%u, MUBAR:%u, MURTS:%u BSRP:%u Others:%u",
              htt_stats_buf->ax_trigger_type[0],
              htt_stats_buf->ax_trigger_type[1],
              htt_stats_buf->ax_trigger_type[2],
              htt_stats_buf->ax_trigger_type[3],
              htt_stats_buf->ax_trigger_type[4],
              htt_stats_buf->ax_trigger_type[5]);
    HTT_STATS_PRINT(FATAL, "11ax Trigger Type = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    snprintf(str_buf, HTT_MAX_STRING_LEN - index,
              " HIPRI:%u, LOWPRI:%u, BSR:%u",
              htt_stats_buf->num_data_ppdu_responded_per_hwq[0],
              htt_stats_buf->num_data_ppdu_responded_per_hwq[1],
              htt_stats_buf->num_data_ppdu_responded_per_hwq[2]);
    HTT_STATS_PRINT(FATAL, "Data PPDU Resp per HWQ = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    snprintf(str_buf, HTT_MAX_STRING_LEN - index,
              " HIPRI:%u, LOWPRI:%u, BSR:%u",
              htt_stats_buf->num_null_delimiters_responded_per_hwq[0],
              htt_stats_buf->num_null_delimiters_responded_per_hwq[1],
              htt_stats_buf->num_null_delimiters_responded_per_hwq[2]);
    HTT_STATS_PRINT(FATAL, "Null Delim Resp per HWQ = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    snprintf(str_buf, HTT_MAX_STRING_LEN - index,
              " Data:%u, NullDelim:%u",
              htt_stats_buf->num_total_trig_responses[0],
              htt_stats_buf->num_total_trig_responses[1]);
    HTT_STATS_PRINT(FATAL, "Trigger Resp Status = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "Last Trigger RX Time Interval = %u \n", htt_stats_buf->last_trig_rx_time_delta_ms);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STA_UL_OFDMA_NUM_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->ul_ofdma_tx_mcs[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_ofdma_tx_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->ul_ofdma_tx_nss[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_ofdma_tx_nss = %s ", str_buf);

    for (j = 0; j < HTT_TX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

        for (i = 0; i < HTT_STA_UL_OFDMA_NUM_MCS_COUNTERS; i++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->ul_ofdma_tx_gi[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "ul_ofdma_tx_gi[%u] = %s ", j, str_buf);
    }

    HTT_STATS_PRINT(FATAL, "ul_ofdma_tx_ldpc = %u ", htt_stats_buf->ul_ofdma_tx_ldpc);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STA_UL_OFDMA_NUM_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->ul_ofdma_tx_bw[i]);
    }
    HTT_STATS_PRINT(FATAL, "ul_ofdma_tx_bw = %s \n", str_buf);

    for (i = 0; i < HTT_STA_UL_OFDMA_NUM_REDUCED_CHAN_TYPES; i++) {
        index = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (j = 0; j < HTT_STA_UL_OFDMA_NUM_BW_COUNTERS; j++) {
            index += snprintf(&str_buf[index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", j, htt_stats_buf->reduced_ul_ofdma_tx_bw[i][j]);
        }

        HTT_STATS_PRINT(FATAL, "%s = %s \n", i == 0 ? "half_ul_ofdma_tx_bw" : "quarter_ul_ofdma_tx_bw", str_buf);
    }

    HTT_STATS_PRINT(FATAL, "Trig Based Tx PPDU = %u ", htt_stats_buf->trig_based_ppdu_tx);
    HTT_STATS_PRINT(FATAL, "RBO Based Tx PPDU = %u ", htt_stats_buf->rbo_based_ppdu_tx);
    HTT_STATS_PRINT(FATAL, "MU to SU EDCA Switch Count = %u ", htt_stats_buf->mu_edca_to_su_edca_switch_count);
    HTT_STATS_PRINT(FATAL, "MU EDCA Params Apply Count = %u \n", htt_stats_buf->num_mu_edca_param_apply_count);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->current_edca_hwq_mode[i]);
    }
    HTT_STATS_PRINT(FATAL, "current_edca_hwq_mode[AC] = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->current_cw_min[i]);
    }
    HTT_STATS_PRINT(FATAL, "current_cw_min = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->current_cw_max[i]);
    }
    HTT_STATS_PRINT(FATAL, "current_cw_max = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->current_aifs[i]);
    }
    HTT_STATS_PRINT(FATAL, "current_aifs = %s ", str_buf);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_vdev_rtt_resp_stats_tlv: display htt_vdev_rtt_resp_stats_tlv values
 * @tag_buf: buffer containing the tlv htt_vdev_rtt_resp_stats_tlv
 *
 * return:void
 */
static void htt_print_vdev_rtt_resp_stats_tlv(A_UINT32 *tag_buf)
{

    htt_vdev_rtt_resp_stats_tlv *htt_vdev_rtt_resp_stats_buf =
              (htt_vdev_rtt_resp_stats_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_VDEV_RTT_RESP_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "vdev_id = %u",
            htt_vdev_rtt_resp_stats_buf->vdev_id);
    HTT_STATS_PRINT(FATAL, "tx_ftm_suc = %u",
            htt_vdev_rtt_resp_stats_buf->tx_ftm_suc);
    HTT_STATS_PRINT(FATAL, "tx_ftm_suc_retry = %u",
            htt_vdev_rtt_resp_stats_buf->tx_ftm_suc_retry);
    HTT_STATS_PRINT(FATAL, "tx_ftm_fail = %u",
            htt_vdev_rtt_resp_stats_buf->tx_ftm_fail);
    HTT_STATS_PRINT(FATAL, "rx_ftmr_cnt = %u",
            htt_vdev_rtt_resp_stats_buf->rx_ftmr_cnt);
    HTT_STATS_PRINT(FATAL, "rx_ftmr_dup_cnt = %u",
            htt_vdev_rtt_resp_stats_buf->rx_ftmr_dup_cnt);
    HTT_STATS_PRINT(FATAL, "rx_iftmr_cnt = %u",
            htt_vdev_rtt_resp_stats_buf->rx_iftmr_cnt);
    HTT_STATS_PRINT(FATAL, "rx_iftmr_dup_cnt = %u",
            htt_vdev_rtt_resp_stats_buf->rx_iftmr_dup_cnt);
    HTT_STATS_PRINT(FATAL, "initiator_active_responder_rejected_cnt = %u",
            htt_vdev_rtt_resp_stats_buf->initiator_active_responder_rejected_cnt);
    HTT_STATS_PRINT(FATAL, "responder_terminate_cnt = %u",
            htt_vdev_rtt_resp_stats_buf->responder_terminate_cnt);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_vdev_rtt_init_stats_tlv: display htt_vdev_rtt_init_stats_tlv values
 * @tag_buf: buffer containing the tlv htt_vdev_rtt_init_stats_tlv
 *
 * return:void
 */
static void htt_print_vdev_rtt_init_stats_tlv(A_UINT32 *tag_buf)
{

    htt_vdev_rtt_init_stats_tlv *htt_vdev_rtt_init_stats_buf =
              (htt_vdev_rtt_init_stats_tlv *) tag_buf;
    HTT_STATS_PRINT(FATAL, "HTT_VDEV_RTT_INIT_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "vdev_id  = %u",
            htt_vdev_rtt_init_stats_buf->vdev_id);
    HTT_STATS_PRINT(FATAL, "tx_ftmr_cnt  = %u",
            htt_vdev_rtt_init_stats_buf->tx_ftmr_cnt);
    HTT_STATS_PRINT(FATAL, "tx_ftmr_fail  = %u",
            htt_vdev_rtt_init_stats_buf->tx_ftmr_fail);
    HTT_STATS_PRINT(FATAL, "tx_ftmr_suc_retry  = %u",
            htt_vdev_rtt_init_stats_buf->tx_ftmr_suc_retry);
    HTT_STATS_PRINT(FATAL, "rx_ftm_cnt  = %u",
            htt_vdev_rtt_init_stats_buf->rx_ftm_cnt);
    HTT_STATS_PRINT(FATAL, "initiator_terminate_cnt  = %u",
            htt_vdev_rtt_init_stats_buf->initiator_terminate_cnt);
    HTT_STATS_PRINT(FATAL, "tx_meas_req_count  = %u",
            htt_vdev_rtt_init_stats_buf->tx_meas_req_count);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_pdev_rtt_tbr_selfgen_queued_stats_tlv:
 * display htt_stats_pdev_rtt_tbr_selfgen_queued_stats_tlv values
 * @tag_buf: buffer containing the tlv htt_stats_pdev_rtt_tbr_selfgen_queued_stats_tlv
 *
 * return:void
 */
static void htt_print_pdev_rtt_tbr_selfgen_queued_stats_tlv(A_UINT32 *tag_buf)
{
    htt_stats_pdev_rtt_tbr_selfgen_queued_stats_tlv
        *htt_pdev_rtt_selfgen_queued_stats_buf =
            (htt_stats_pdev_rtt_tbr_selfgen_queued_stats_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_STATS_PDEV_RTT_TBR_SELFGEN_QUEUED_STATS_TAG:");
    HTT_STATS_PRINT(FATAL, "SU POLL = %u",
            htt_pdev_rtt_selfgen_queued_stats_buf->tbr_su_ftype_queued[HTT_TXSEND_FTYPE_SGEN_TF_POLL]);
    HTT_STATS_PRINT(FATAL, "SU SOUND = %u",
            htt_pdev_rtt_selfgen_queued_stats_buf->tbr_su_ftype_queued[HTT_TXSEND_FTYPE_SGEN_TF_SOUND]);
    HTT_STATS_PRINT(FATAL, "SU NDPA = %u",
        htt_pdev_rtt_selfgen_queued_stats_buf->tbr_su_ftype_queued[HTT_TXSEND_FTYPE_SGEN_TBR_NDPA]);
    HTT_STATS_PRINT(FATAL, "SU NDP = %u",
            htt_pdev_rtt_selfgen_queued_stats_buf->tbr_su_ftype_queued[HTT_TXSEND_FTYPE_SGEN_TBR_NDP]);
    HTT_STATS_PRINT(FATAL, "SU LMR = %u",
            htt_pdev_rtt_selfgen_queued_stats_buf->tbr_su_ftype_queued[HTT_TXSEND_FTYPE_SGEN_TBR_LMR]);
    HTT_STATS_PRINT(FATAL, "SU TF_REPORT = %u",
            htt_pdev_rtt_selfgen_queued_stats_buf->tbr_su_ftype_queued[HTT_TXSEND_FTYPE_SGEN_TF_REPORT]);
    HTT_STATS_PRINT(FATAL, "MU POLL = %u",
            htt_pdev_rtt_selfgen_queued_stats_buf->tbr_mu_ftype_queued[HTT_TXSEND_FTYPE_SGEN_TF_POLL]);
    HTT_STATS_PRINT(FATAL, "MU SOUND = %u",
            htt_pdev_rtt_selfgen_queued_stats_buf->tbr_mu_ftype_queued[HTT_TXSEND_FTYPE_SGEN_TF_SOUND]);
    HTT_STATS_PRINT(FATAL, "MU NDPA = %u",
            htt_pdev_rtt_selfgen_queued_stats_buf->tbr_mu_ftype_queued[HTT_TXSEND_FTYPE_SGEN_TBR_NDPA]);
    HTT_STATS_PRINT(FATAL, "MU NDP = %u",
            htt_pdev_rtt_selfgen_queued_stats_buf->tbr_mu_ftype_queued[HTT_TXSEND_FTYPE_SGEN_TBR_NDP]);
    HTT_STATS_PRINT(FATAL, "MU LMR = %u",
            htt_pdev_rtt_selfgen_queued_stats_buf->tbr_mu_ftype_queued[HTT_TXSEND_FTYPE_SGEN_TBR_LMR]);
    HTT_STATS_PRINT(FATAL, "MU TF_REPORT = %u",
            htt_pdev_rtt_selfgen_queued_stats_buf->tbr_mu_ftype_queued[HTT_TXSEND_FTYPE_SGEN_TF_REPORT]);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_pdev_rtt_tbr_cmd_result_stats_tlv:
 * display htt_stats_pdev_rtt_tbr_cmd_result_stats_tlv values
 * @tag_buf: buffer containing the tlv htt_stats_pdev_rtt_tbr_cmd_result_stats_tlv
 *
 * return:void
 */
void htt_print_pdev_rtt_tbr_cmd_result_stats_tlv(A_UINT32 *tag_buf)
{
    A_UINT8 j;
    htt_stats_pdev_rtt_tbr_cmd_result_stats_tlv
        *htt_pdev_rtt_cmd_result_stats_buf =
            (htt_stats_pdev_rtt_tbr_cmd_result_stats_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_STATS_PDEV_RTT_TBR_CMD_RESULT_STATS_TAG:");

    for (j=0; j<htt_pdev_rtt_cmd_result_stats_buf->tbr_num_sch_cmd_result_buckets; j++) {
        HTT_STATS_PRINT(FATAL, "SU frame_SGEN_TF_POLL : num_sch_cmd_status_%u = %u",
            j, htt_pdev_rtt_cmd_result_stats_buf->opaque_tbr_su_ftype_cmd_result[HTT_TXSEND_FTYPE_SGEN_TF_POLL][j]);

        HTT_STATS_PRINT(FATAL, "SU frame_SGEN_TF_SOUND : num_sch_cmd_status_%u = %u",
            j, htt_pdev_rtt_cmd_result_stats_buf->opaque_tbr_su_ftype_cmd_result[HTT_TXSEND_FTYPE_SGEN_TF_SOUND][j]);

        HTT_STATS_PRINT(FATAL, "SU frame_SGEN_TBR_NDPA : num_sch_cmd_status_%u = %u",
            j, htt_pdev_rtt_cmd_result_stats_buf->opaque_tbr_su_ftype_cmd_result[HTT_TXSEND_FTYPE_SGEN_TBR_NDPA][j]);

        HTT_STATS_PRINT(FATAL, "SU frame_SGEN_TBR_NDP : num_sch_cmd_status_%u = %u",
            j, htt_pdev_rtt_cmd_result_stats_buf->opaque_tbr_su_ftype_cmd_result[HTT_TXSEND_FTYPE_SGEN_TBR_NDP][j]);

        HTT_STATS_PRINT(FATAL, "SU frame_SGEN_TBR_LMR : num_sch_cmd_status_%u = %u",
            j, htt_pdev_rtt_cmd_result_stats_buf->opaque_tbr_su_ftype_cmd_result[HTT_TXSEND_FTYPE_SGEN_TBR_LMR][j]);

        HTT_STATS_PRINT(FATAL, "SU frame_SGEN_TF_REPORT : num_sch_cmd_status_%u = %u",
            j, htt_pdev_rtt_cmd_result_stats_buf->opaque_tbr_su_ftype_cmd_result[HTT_TXSEND_FTYPE_SGEN_TF_REPORT][j]);

        HTT_STATS_PRINT(FATAL, "MU frame_SGEN_TF_POLL : num_sch_cmd_status_%u = %u",
            j, htt_pdev_rtt_cmd_result_stats_buf->opaque_tbr_mu_ftype_cmd_result[HTT_TXSEND_FTYPE_SGEN_TF_POLL][j]);

        HTT_STATS_PRINT(FATAL, "MU frame_SGEN_TF_SOUND : num_sch_cmd_status_%u = %u",
            j, htt_pdev_rtt_cmd_result_stats_buf->opaque_tbr_mu_ftype_cmd_result[HTT_TXSEND_FTYPE_SGEN_TF_SOUND][j]);

        HTT_STATS_PRINT(FATAL, "MU frame_SGEN_TBR_NDPA : num_sch_cmd_status_%u = %u",
            j, htt_pdev_rtt_cmd_result_stats_buf->opaque_tbr_mu_ftype_cmd_result[HTT_TXSEND_FTYPE_SGEN_TBR_NDPA][j]);

        HTT_STATS_PRINT(FATAL, "MU frame_SGEN_TBR_NDP : num_sch_cmd_status_%u = %u",
            j, htt_pdev_rtt_cmd_result_stats_buf->opaque_tbr_mu_ftype_cmd_result[HTT_TXSEND_FTYPE_SGEN_TBR_NDP][j]);

        HTT_STATS_PRINT(FATAL, "MU frame_SGEN_TBR_LMR : num_sch_cmd_status_%u = %u",
            j, htt_pdev_rtt_cmd_result_stats_buf->opaque_tbr_mu_ftype_cmd_result[HTT_TXSEND_FTYPE_SGEN_TBR_LMR][j]);

        HTT_STATS_PRINT(FATAL, "MU frame_SGEN_TF_REPORT : num_sch_cmd_status_%u = %u",
            j, htt_pdev_rtt_cmd_result_stats_buf->opaque_tbr_mu_ftype_cmd_result[HTT_TXSEND_FTYPE_SGEN_TF_REPORT][j]);
    }

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_pdev_rtt_hw_stats_tlv: display htt_stats_pdev_rtt_hw_stats_tlv values
 * @tag_buf: buffer containing the tlv htt_stats_pdev_rtt_hw_stats_tlv
 *
 * return:void
 */
static void htt_print_pdev_rtt_hw_stats_tlv(A_UINT32 *tag_buf)
{

    htt_stats_pdev_rtt_hw_stats_tlv *htt_pdev_rtt_hw_stats_buf =
              (htt_stats_pdev_rtt_hw_stats_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_STATS_PDEV_RTT_HW_STATS_TAG:");
    HTT_STATS_PRINT(FATAL, "ista_ranging_ndpa_cnt = %u",
            htt_pdev_rtt_hw_stats_buf->ista_ranging_ndpa_cnt);
    HTT_STATS_PRINT(FATAL, "ista_ranging_ndp_cnt = %u",
            htt_pdev_rtt_hw_stats_buf->ista_ranging_ndp_cnt);
    HTT_STATS_PRINT(FATAL, "ista_ranging_i2r_lmr_cnt = %u",
            htt_pdev_rtt_hw_stats_buf->ista_ranging_i2r_lmr_cnt);
    HTT_STATS_PRINT(FATAL, "rtsa_ranging_resp_cnt = %u",
            htt_pdev_rtt_hw_stats_buf->rtsa_ranging_resp_cnt);
    HTT_STATS_PRINT(FATAL, "rtsa_ranging_ndp_cnt = %u",
            htt_pdev_rtt_hw_stats_buf->rtsa_ranging_ndp_cnt);
    HTT_STATS_PRINT(FATAL, "rsta_ranging_lmr_cnt = %u",
            htt_pdev_rtt_hw_stats_buf->rsta_ranging_lmr_cnt);
    HTT_STATS_PRINT(FATAL, "tb_ranging_cts2s_rcvd_cnt = %u",
            htt_pdev_rtt_hw_stats_buf->tb_ranging_cts2s_rcvd_cnt);
    HTT_STATS_PRINT(FATAL, "tb_ranging_ndp_rcvd_cnt = %u",
            htt_pdev_rtt_hw_stats_buf->tb_ranging_ndp_rcvd_cnt);
    HTT_STATS_PRINT(FATAL, "tb_ranging_lmr_rcvd_cnt = %u",
            htt_pdev_rtt_hw_stats_buf->tb_ranging_lmr_rcvd_cnt);
    HTT_STATS_PRINT(FATAL, "tb_ranging_tf_poll_resp_sent_cnt = %u",
            htt_pdev_rtt_hw_stats_buf->tb_ranging_tf_poll_resp_sent_cnt);
    HTT_STATS_PRINT(FATAL, "tb_ranging_tf_sound_resp_sent_cnt = %u",
            htt_pdev_rtt_hw_stats_buf->tb_ranging_tf_sound_resp_sent_cnt);
    HTT_STATS_PRINT(FATAL, "tb_ranging_tf_report_resp_sent_cnt = %u",
            htt_pdev_rtt_hw_stats_buf->tb_ranging_tf_report_resp_sent_cnt);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_pdev_rtt_resp_stats_tlv: display htt_stats_pdev_rtt_resp_stats_tlv values
 * @tag_buf: buffer containing the tlv htt_stats_pdev_rtt_resp_stats_tlv
 *
 * return:void
 */
static void htt_print_pdev_rtt_resp_stats_tlv(A_UINT32 *tag_buf)
{

    htt_stats_pdev_rtt_resp_stats_tlv *htt_pdev_rtt_resp_stats_buf =
              (htt_stats_pdev_rtt_resp_stats_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_PDEV_RTT_RESP_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "pdev_id = %u",
            htt_pdev_rtt_resp_stats_buf->pdev_id);
    HTT_STATS_PRINT(FATAL, "tx_11mc_ftm_suc = %u",
            htt_pdev_rtt_resp_stats_buf->tx_11mc_ftm_suc);
    HTT_STATS_PRINT(FATAL, "tx_11mc_ftm_suc_retry = %u",
            htt_pdev_rtt_resp_stats_buf->tx_11mc_ftm_suc_retry);
    HTT_STATS_PRINT(FATAL, "tx_11mc_ftm_fail = %u",
            htt_pdev_rtt_resp_stats_buf->tx_11mc_ftm_fail);
    HTT_STATS_PRINT(FATAL, "rx_11mc_ftmr_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->rx_11mc_ftmr_cnt);
    HTT_STATS_PRINT(FATAL, "rx_11mc_ftmr_dup_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->rx_11mc_ftmr_dup_cnt);
    HTT_STATS_PRINT(FATAL, "rx_11mc_iftmr_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->rx_11mc_iftmr_cnt);
    HTT_STATS_PRINT(FATAL, "rx_11mc_iftmr_dup_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->rx_11mc_iftmr_dup_cnt);
    HTT_STATS_PRINT(FATAL, "ftmr_drop_11mc_resp_role_not_enabled_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->ftmr_drop_11mc_resp_role_not_enabled_cnt);
    HTT_STATS_PRINT(FATAL, "tx_11az_ftm_successful = %u",
            htt_pdev_rtt_resp_stats_buf->tx_11az_ftm_successful);
    HTT_STATS_PRINT(FATAL, "tx_11az_ftm_failed = %u",
            htt_pdev_rtt_resp_stats_buf->tx_11az_ftm_failed);
    HTT_STATS_PRINT(FATAL, "rx_11az_ftmr_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->rx_11az_ftmr_cnt);
    HTT_STATS_PRINT(FATAL, "rx_11az_ftmr_dup_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->rx_11az_ftmr_dup_cnt);
    HTT_STATS_PRINT(FATAL, "rx_11az_iftmr_dup_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->rx_11az_iftmr_dup_cnt);
    HTT_STATS_PRINT(FATAL, "initiator_active_responder_rejected_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->initiator_active_responder_rejected_cnt);
    HTT_STATS_PRINT(FATAL, "malformed_ftmr = %u",
            htt_pdev_rtt_resp_stats_buf->malformed_ftmr);
    HTT_STATS_PRINT(FATAL, "ftmr_drop_ntb_resp_role_not_enabled_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->ftmr_drop_ntb_resp_role_not_enabled_cnt);
    HTT_STATS_PRINT(FATAL, "ftmr_drop_tb_resp_role_not_enabled_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->ftmr_drop_tb_resp_role_not_enabled_cnt);
    HTT_STATS_PRINT(FATAL, "responder_alloc_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->responder_alloc_cnt);
    HTT_STATS_PRINT(FATAL, "responder_alloc_failure = %u",
            htt_pdev_rtt_resp_stats_buf->responder_alloc_failure);
    HTT_STATS_PRINT(FATAL, "responder_terminate_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->responder_terminate_cnt);
    HTT_STATS_PRINT(FATAL, "active_rsta_open = %u",
            htt_pdev_rtt_resp_stats_buf->active_rsta_open);
    HTT_STATS_PRINT(FATAL, "active_rsta_mac = %u",
            htt_pdev_rtt_resp_stats_buf->active_rsta_mac);
    HTT_STATS_PRINT(FATAL, "active_rsta_mac_phy = %u",
            htt_pdev_rtt_resp_stats_buf->active_rsta_mac_phy);
    HTT_STATS_PRINT(FATAL, "pn_check_failure_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->pn_check_failure_cnt);
    HTT_STATS_PRINT(FATAL, "num_assoc_ranging_peers = %u",
            htt_pdev_rtt_resp_stats_buf->num_assoc_ranging_peers);
    HTT_STATS_PRINT(FATAL, "num_unassoc_ranging_peers = %u",
            htt_pdev_rtt_resp_stats_buf->num_unassoc_ranging_peers);
    HTT_STATS_PRINT(FATAL, "pasn_m1_auth_recv_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->pasn_m1_auth_recv_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_m1_auth_drop_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->pasn_m1_auth_drop_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_m2_auth_recv_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->pasn_m2_auth_recv_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_m2_auth_tx_fail_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->pasn_m2_auth_tx_fail_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_m3_auth_recv_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->pasn_m3_auth_recv_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_m3_auth_drop_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->pasn_m3_auth_drop_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_peer_create_request_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->pasn_peer_create_request_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_peer_created_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->pasn_peer_created_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_peer_create_timeout_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->pasn_peer_create_timeout_cnt);
    HTT_STATS_PRINT(FATAL, "sec_ranging_not_supported_mfp_not_setup = %u",
            htt_pdev_rtt_resp_stats_buf->sec_ranging_not_supported_mfp_not_setup);
    HTT_STATS_PRINT(FATAL, "non_sec_ranging_discarded_for_assoc_peer_with_mfpr_set = %u",
            htt_pdev_rtt_resp_stats_buf->non_sec_ranging_discarded_for_assoc_peer_with_mfpr_set);
    HTT_STATS_PRINT(FATAL, "open_ranging_discarded_with_URNM_MFPR_set_for_pasn_peer = %u",
            htt_pdev_rtt_resp_stats_buf->open_ranging_discarded_with_URNM_MFPR_set_for_pasn_peer);
    HTT_STATS_PRINT(FATAL, "unassoc_non_pasn_ranging_not_supported_with_URNM_MFPR = %u",
            htt_pdev_rtt_resp_stats_buf->unassoc_non_pasn_ranging_not_supported_with_URNM_MFPR);
    HTT_STATS_PRINT(FATAL, "invalid_ftm_request_params = %u",
            htt_pdev_rtt_resp_stats_buf->invalid_ftm_request_params);
    HTT_STATS_PRINT(FATAL, "requested_bw_format_not_supported = %u",
            htt_pdev_rtt_resp_stats_buf->requested_bw_format_not_supported);
    HTT_STATS_PRINT(FATAL, "ntb_unsec_unassoc_mode_ranging_peer_alloc_failed = %u",
            htt_pdev_rtt_resp_stats_buf->ntb_unsec_unassoc_mode_ranging_peer_alloc_failed);
    HTT_STATS_PRINT(FATAL, "tb_unassoc_unsec_mode_pasn_peer_creation_failed = %u",
            htt_pdev_rtt_resp_stats_buf->tb_unassoc_unsec_mode_pasn_peer_creation_failed);
    HTT_STATS_PRINT(FATAL, "num_ranging_sequences_processed = %u",
            htt_pdev_rtt_resp_stats_buf->num_ranging_sequences_processed);
    HTT_STATS_PRINT(FATAL, "ndp_rx_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->ndp_rx_cnt);
    HTT_STATS_PRINT(FATAL, "num_req_bw_20_MHz = %u",
            htt_pdev_rtt_resp_stats_buf->num_req_bw_20_MHz);
    HTT_STATS_PRINT(FATAL, "num_req_bw_40_MHz = %u",
            htt_pdev_rtt_resp_stats_buf->num_req_bw_40_MHz);
    HTT_STATS_PRINT(FATAL, "num_req_bw_80_MHz = %u",
            htt_pdev_rtt_resp_stats_buf->num_req_bw_80_MHz);
    HTT_STATS_PRINT(FATAL, "num_req_bw_160_MHz = %u",
            htt_pdev_rtt_resp_stats_buf->num_req_bw_160_MHz);
    HTT_STATS_PRINT(FATAL, "ntb_tx_ndp = %u",
            htt_pdev_rtt_resp_stats_buf->ntb_tx_ndp);
    HTT_STATS_PRINT(FATAL, "num_ntb_ranging_NDPAs_recv = %u",
            htt_pdev_rtt_resp_stats_buf->num_ntb_ranging_NDPAs_recv);
    HTT_STATS_PRINT(FATAL, "recv_lmr = %u",
            htt_pdev_rtt_resp_stats_buf->recv_lmr);
    HTT_STATS_PRINT(FATAL, "invalid_ftmr_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->invalid_ftmr_cnt);
    HTT_STATS_PRINT(FATAL, "max_time_bw_meas_exp_cnt = %u",
            htt_pdev_rtt_resp_stats_buf->max_time_bw_meas_exp_cnt);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_pdev_rtt_init_stats_tlv: display htt_stats_pdev_rtt_init_stats_tlv values
 * @tag_buf: buffer containing the tlv htt_stats_pdev_rtt_init_stats_tlv
 *
 * return:void
 */
static void htt_print_pdev_rtt_init_stats_tlv(A_UINT32 *tag_buf)
{
    A_UINT8 i;
    htt_stats_pdev_rtt_init_stats_tlv *htt_pdev_rtt_init_stats_buf =
              (htt_stats_pdev_rtt_init_stats_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "htt_pdev_rtt_init_stats_TLV:");
    HTT_STATS_PRINT(FATAL, "pdev_id = %u",
            htt_pdev_rtt_init_stats_buf->pdev_id);
    HTT_STATS_PRINT(FATAL, "tx_11mc_ftmr_cnt = %u",
            htt_pdev_rtt_init_stats_buf->tx_11mc_ftmr_cnt);
    HTT_STATS_PRINT(FATAL, "tx_11mc_ftmr_fail = %u",
            htt_pdev_rtt_init_stats_buf->tx_11mc_ftmr_fail);
    HTT_STATS_PRINT(FATAL, "tx_11mc_ftmr_suc_retry = %u",
            htt_pdev_rtt_init_stats_buf->tx_11mc_ftmr_suc_retry);
    HTT_STATS_PRINT(FATAL, "rx_11mc_ftm_cnt = %u",
            htt_pdev_rtt_init_stats_buf->rx_11mc_ftm_cnt);
    HTT_STATS_PRINT(FATAL, "rx_11az_ftm_cnt = %u",
            htt_pdev_rtt_init_stats_buf->rx_11az_ftm_cnt);
    HTT_STATS_PRINT(FATAL, "initiator_terminate_cnt = %u",
            htt_pdev_rtt_init_stats_buf->initiator_terminate_cnt);
    HTT_STATS_PRINT(FATAL, "tx_meas_req_count = %u",
            htt_pdev_rtt_init_stats_buf->tx_meas_req_count);
    HTT_STATS_PRINT(FATAL, "tx_11az_ftmr_start = %u",
            htt_pdev_rtt_init_stats_buf->tx_11az_ftmr_start);
    HTT_STATS_PRINT(FATAL, "tx_11az_ftmr_stop = %u",
            htt_pdev_rtt_init_stats_buf->tx_11az_ftmr_stop);
    HTT_STATS_PRINT(FATAL, "tx_11az_ftmr_fail = %u",
            htt_pdev_rtt_init_stats_buf->tx_11az_ftmr_fail);
    HTT_STATS_PRINT(FATAL, "ftmr_tx_failed_null_11az_peer = %u",
            htt_pdev_rtt_init_stats_buf->ftmr_tx_failed_null_11az_peer);
    HTT_STATS_PRINT(FATAL, "ftmr_retry_timeout = %u",
            htt_pdev_rtt_init_stats_buf->ftmr_retry_timeout);
    HTT_STATS_PRINT(FATAL, "ftm_parse_failure = %u",
            htt_pdev_rtt_init_stats_buf->ftm_parse_failure);
    HTT_STATS_PRINT(FATAL, "incompatible_ftm_params = %u",
            htt_pdev_rtt_init_stats_buf->incompatible_ftm_params);
    HTT_STATS_PRINT(FATAL, "ranging_negotiation_successful_cnt = %u",
            htt_pdev_rtt_init_stats_buf->ranging_negotiation_successful_cnt);
    HTT_STATS_PRINT(FATAL, "active_ista = %u",
            htt_pdev_rtt_init_stats_buf->active_ista);
    HTT_STATS_PRINT(FATAL, "init_role_not_enabled = %u",
            htt_pdev_rtt_init_stats_buf->init_role_not_enabled);
    HTT_STATS_PRINT(FATAL, "invalid_preamble = %u",
            htt_pdev_rtt_init_stats_buf->invalid_preamble);
    HTT_STATS_PRINT(FATAL, "invalid_chan_bw_format = %u",
            htt_pdev_rtt_init_stats_buf->invalid_chan_bw_format);
    HTT_STATS_PRINT(FATAL, "mgmt_buff_alloc_fail_cnt = %u",
            htt_pdev_rtt_init_stats_buf->mgmt_buff_alloc_fail_cnt);
    HTT_STATS_PRINT(FATAL, "sec_ranging_req_in_open_mode = %u",
            htt_pdev_rtt_init_stats_buf->sec_ranging_req_in_open_mode);
    HTT_STATS_PRINT(FATAL, "max_time_bw_meas_exp_cnt = %u",
            htt_pdev_rtt_init_stats_buf->max_time_bw_meas_exp_cnt);
    HTT_STATS_PRINT(FATAL, "num_tb_ranging_requests = %u",
            htt_pdev_rtt_init_stats_buf->num_tb_ranging_requests);
    HTT_STATS_PRINT(FATAL, "tb_meas_duration_expiry_cnt = %u",
            htt_pdev_rtt_init_stats_buf->tb_meas_duration_expiry_cnt);
    HTT_STATS_PRINT(FATAL, "ntbr_triggered_successfully = %u",
            htt_pdev_rtt_init_stats_buf->ntbr_triggered_successfully);
    HTT_STATS_PRINT(FATAL, "ntbr_trigger_failed = %u",
            htt_pdev_rtt_init_stats_buf->ntbr_trigger_failed);
    HTT_STATS_PRINT(FATAL,"invalid_or_no_vreg_idx = %u",
            htt_pdev_rtt_init_stats_buf->invalid_or_no_vreg_idx);
    HTT_STATS_PRINT(FATAL, "set_vreg_params_failed = %u",
            htt_pdev_rtt_init_stats_buf->set_vreg_params_failed);
    HTT_STATS_PRINT(FATAL, "sac_mismatch = %u",
            htt_pdev_rtt_init_stats_buf->sac_mismatch);
    HTT_STATS_PRINT(FATAL, "pasn_m1_auth_recv_cnt = %u",
            htt_pdev_rtt_init_stats_buf->pasn_m1_auth_recv_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_m1_auth_tx_fail_cnt = %u",
            htt_pdev_rtt_init_stats_buf->pasn_m1_auth_tx_fail_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_m2_auth_recv_cnt = %u",
            htt_pdev_rtt_init_stats_buf->pasn_m2_auth_recv_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_m2_auth_drop_cnt = %u",
            htt_pdev_rtt_init_stats_buf->pasn_m2_auth_drop_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_m3_auth_recv_cnt = %u",
            htt_pdev_rtt_init_stats_buf->pasn_m3_auth_recv_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_m3_auth_tx_fail_cnt = %u",
            htt_pdev_rtt_init_stats_buf->pasn_m3_auth_tx_fail_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_peer_create_request_cnt = %u",
            htt_pdev_rtt_init_stats_buf->pasn_peer_create_request_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_peer_created_cnt = %u",
            htt_pdev_rtt_init_stats_buf->pasn_peer_created_cnt);
    HTT_STATS_PRINT(FATAL, "pasn_peer_create_timeout_cnt = %u",
            htt_pdev_rtt_init_stats_buf->pasn_peer_create_timeout_cnt);
    HTT_STATS_PRINT(FATAL, "ntbr_ndpa_failed = %u",
            htt_pdev_rtt_init_stats_buf->ntbr_ndpa_failed);
    HTT_STATS_PRINT(FATAL, "ntbr_sequence_successful = %u",
            htt_pdev_rtt_init_stats_buf->ntbr_sequence_successful);
    HTT_STATS_PRINT(FATAL, "ntbr_ndp_failed = %u",
            htt_pdev_rtt_init_stats_buf->ntbr_ndp_failed);
    HTT_STATS_PRINT(FATAL, "num_tb_ranging_NDPAs_recv = %u",
            htt_pdev_rtt_init_stats_buf->num_tb_ranging_NDPAs_recv);
    HTT_STATS_PRINT(FATAL, "ndp_rx_cnt = %u",
            htt_pdev_rtt_init_stats_buf->ndp_rx_cnt);
    HTT_STATS_PRINT(FATAL, "num_trigger_frames_received = %u",
            htt_pdev_rtt_init_stats_buf->num_trigger_frames_received);
    for (i = 0; i<8; i++) {
        HTT_STATS_PRINT(FATAL, "num_sch_cmd_status_%d = %u",
            i, htt_pdev_rtt_init_stats_buf->sch_cmd_status_cnts[i]);
    }
    HTT_STATS_PRINT(FATAL, "num_sch_cmd_status_other_failure = %u",
            htt_pdev_rtt_init_stats_buf->sch_cmd_status_cnts[8]);
    HTT_STATS_PRINT(FATAL, "lmr_timeout = %u",
            htt_pdev_rtt_init_stats_buf->lmr_timeout);
    HTT_STATS_PRINT(FATAL, "lmr_recv = %u",
            htt_pdev_rtt_init_stats_buf->lmr_recv);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

static void htt_print_dlpager_stats(A_UINT32 *tag_buf)
{
    A_UINT32 idx;
    htt_dlpager_stats_t *htt_stats_buf =
        (htt_dlpager_stats_t *) tag_buf;
    htt_dl_pager_stats_tlv *dl_pager_stats = (htt_dl_pager_stats_tlv *) &htt_stats_buf->dl_pager_stats;
    HTT_STATS_PRINT(FATAL, "HTT_DLPAGER_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "=========================================== \n");
    HTT_STATS_PRINT(FATAL, "ASYNC locked pages = %u", HTT_DLPAGER_ASYNC_LOCK_PAGE_COUNT_GET(dl_pager_stats->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "SYNC locked pages = %u", HTT_DLPAGER_SYNC_LOCK_PAGE_COUNT_GET(dl_pager_stats->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "Total locked pages = %u", HTT_DLPAGER_TOTAL_LOCKED_PAGES_GET(dl_pager_stats->msg_dword_2));
    HTT_STATS_PRINT(FATAL, "Total free pages = %u", HTT_DLPAGER_TOTAL_FREE_PAGES_GET(dl_pager_stats->msg_dword_2));
    HTT_STATS_PRINT(FATAL, "=========================================== \n");
    HTT_STATS_PRINT(FATAL, "LOCKED PAGES HISTORY");
    HTT_STATS_PRINT(FATAL, "=========================================== \n");
    HTT_STATS_PRINT(FATAL, "last_locked_page_idx = %u", (HTT_DLPAGER_LAST_LOCKED_PAGE_IDX_GET(dl_pager_stats->msg_dword_3)) ? \
                            (HTT_DLPAGER_LAST_LOCKED_PAGE_IDX_GET(dl_pager_stats->msg_dword_3) - 1) : (HTT_DLPAGER_STATS_MAX_HIST-1));
    for (idx = 0; idx < HTT_DLPAGER_STATS_MAX_HIST; idx++) {
        HTT_STATS_PRINT(FATAL, "Index - %u : Page Number - %u : Num of pages - %u : Timestamp - %llu us",
                idx,
                dl_pager_stats->last_pages_info[HTT_STATS_PAGE_LOCKED][idx].page_num,
                dl_pager_stats->last_pages_info[HTT_STATS_PAGE_LOCKED][idx].num_of_pages,
                ((dl_pager_stats->last_pages_info[HTT_STATS_PAGE_LOCKED][idx].timestamp_lsbs) |
                 (((unsigned long long) dl_pager_stats->last_pages_info[HTT_STATS_PAGE_LOCKED][idx].timestamp_msbs) << 32)));
    }

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
    HTT_STATS_PRINT(FATAL, "UNLOCKED PAGES HISTORY");
    HTT_STATS_PRINT(FATAL, "=========================================== \n");
    HTT_STATS_PRINT(FATAL, "last_unlocked_page_idx = %u", (HTT_DLPAGER_LAST_UNLOCKED_PAGE_IDX_GET(dl_pager_stats->msg_dword_3)) ? \
            (HTT_DLPAGER_LAST_UNLOCKED_PAGE_IDX_GET(dl_pager_stats->msg_dword_3) - 1) : (HTT_DLPAGER_STATS_MAX_HIST-1));
    for (idx = 0; idx < HTT_DLPAGER_STATS_MAX_HIST; idx++) {
        HTT_STATS_PRINT(FATAL, "Index - %u : Page Number - %u : Num of pages - %u : Timestamp - %llu us",
                idx,
                dl_pager_stats->last_pages_info[HTT_STATS_PAGE_UNLOCKED][idx].page_num,
                dl_pager_stats->last_pages_info[HTT_STATS_PAGE_UNLOCKED][idx].num_of_pages,
                ((dl_pager_stats->last_pages_info[HTT_STATS_PAGE_UNLOCKED][idx].timestamp_lsbs) |
                 (((unsigned long long) dl_pager_stats->last_pages_info[HTT_STATS_PAGE_UNLOCKED][idx].timestamp_msbs) << 32)));
    }
    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_pktlog_and_htt_ring_stats_tlv: display htt_pktlog_and_htt_ring_stats_tlv values
 * @tag_buf: buffer containing the tlv htt_pktlog_and_htt_ring_stats_tlv
 *
 * return:void
 */
static void htt_print_pktlog_and_htt_ring_stats_tlv(A_UINT32 *tag_buf)
{
    htt_pktlog_and_htt_ring_stats_tlv *htt_stats_pktlog_and_htt_ring_stats_buf =
              (htt_pktlog_and_htt_ring_stats_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_PKTLOG_AND_HTT_RING_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "pktlog_lite_drop_cnt = %u",
            htt_stats_pktlog_and_htt_ring_stats_buf->pktlog_lite_drop_cnt);
    HTT_STATS_PRINT(FATAL, "pktlog_tqm_drop_cnt = %u",
            htt_stats_pktlog_and_htt_ring_stats_buf->pktlog_tqm_drop_cnt);
    HTT_STATS_PRINT(FATAL, "pktlog_ppdu_stats_drop_cnt = %u",
            htt_stats_pktlog_and_htt_ring_stats_buf->pktlog_ppdu_stats_drop_cnt);
    HTT_STATS_PRINT(FATAL, "pktlog_ppdu_ctrl_drop_cnt = %u",
            htt_stats_pktlog_and_htt_ring_stats_buf->pktlog_ppdu_ctrl_drop_cnt);
    HTT_STATS_PRINT(FATAL, "pktlog_sw_events_drop_cnt = %u",
            htt_stats_pktlog_and_htt_ring_stats_buf->pktlog_sw_events_drop_cnt);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_phy_counters_tlv: display htt_print_phy_counters_tlv values
 * @tag_buf: buffer containing the tlv htt_print_phy_counters_tlv
 *
 * return:void
 */
static void htt_print_phy_counters_tlv(A_UINT32 *tag_buf)
{
    htt_phy_counters_tlv *htt_stats_phy_counters_buf =
              (htt_phy_counters_tlv *) tag_buf;
    A_UINT8 i;

    HTT_STATS_PRINT(FATAL, "HTT_PHY_COUNTERS_TLV:");
    HTT_STATS_PRINT(FATAL, "rx_ofdma_timing_err_cnt = %u",
            htt_stats_phy_counters_buf->rx_ofdma_timing_err_cnt);
    HTT_STATS_PRINT(FATAL, "mactx_abort_cnt = %u",
            htt_stats_phy_counters_buf->mactx_abort_cnt);
    HTT_STATS_PRINT(FATAL, "macrx_abort_cnt = %u",
            htt_stats_phy_counters_buf->macrx_abort_cnt);
    HTT_STATS_PRINT(FATAL, "phytx_abort_cnt = %u",
            htt_stats_phy_counters_buf->phytx_abort_cnt);
    HTT_STATS_PRINT(FATAL, "phyrx_abort_cnt = %u",
            htt_stats_phy_counters_buf->phyrx_abort_cnt);
    HTT_STATS_PRINT(FATAL, "phyrx_defer_abort_cnt = %u",
            htt_stats_phy_counters_buf->phyrx_defer_abort_cnt);
    HTT_STATS_PRINT(FATAL, "rx_gain_adj_lstf_event_cnt = %u",
            htt_stats_phy_counters_buf->rx_gain_adj_lstf_event_cnt);
    HTT_STATS_PRINT(FATAL, "rx_gain_adj_non_legacy_cnt = %u",
            htt_stats_phy_counters_buf->rx_gain_adj_non_legacy_cnt);
    for (i = 0; i < HTT_MAX_RX_PKT_CNT; i++)
    {
        HTT_STATS_PRINT(FATAL, "rx_pkt_cnt[%d] =%u",
            i, htt_stats_phy_counters_buf->rx_pkt_cnt[i]);
    }
    for (i = 0; i < HTT_MAX_RX_PKT_CRC_PASS_CNT; i++)
    {
        HTT_STATS_PRINT(FATAL, "rx_pkt_crc_pass_cnt[%d] =%u",
            i, htt_stats_phy_counters_buf->rx_pkt_crc_pass_cnt[i]);
    }
    for (i = 0; i < HTT_MAX_PER_BLK_ERR_CNT; i++)
    {
        HTT_STATS_PRINT(FATAL, "per_blk_err_cnt[%d] =%u",
            i, htt_stats_phy_counters_buf->per_blk_err_cnt[i]);
    }
    for (i = 0; i < HTT_MAX_RX_OTA_ERR_CNT; i++)
    {
        HTT_STATS_PRINT(FATAL, "rx_ota_err_cnt[%d] =%u",
            i, htt_stats_phy_counters_buf->rx_ota_err_cnt[i]);
    }
    for (i = 0; i < HTT_MAX_RX_PKT_CNT_EXT; i++)
    {
        HTT_STATS_PRINT(FATAL, "rx_pkt_cnt_ext[%d] =%u",
            i, htt_stats_phy_counters_buf->rx_pkt_cnt_ext[i]);
    }
    for (i = 0; i < HTT_MAX_RX_PKT_CRC_PASS_CNT_EXT; i++)
    {
        HTT_STATS_PRINT(FATAL, "rx_pkt_crc_pass_cnt_ext[%d] =%u",
            i, htt_stats_phy_counters_buf->rx_pkt_crc_pass_cnt_ext[i]);
    }
    for (i = 0; i < HTT_MAX_RX_PKT_MU_CNT; i++)
    {
        HTT_STATS_PRINT(FATAL, "rx_pkt_mu_cnt[%d] =%u",
            i, htt_stats_phy_counters_buf->rx_pkt_mu_cnt[i]);
    }
    for (i = 0; i < HTT_MAX_TX_PKT_CNT; i++)
    {
        HTT_STATS_PRINT(FATAL, "tx_pkt_cnt[%d] =%u",
            i, htt_stats_phy_counters_buf->tx_pkt_cnt[i]);
    }
    for (i = 0; i < HTT_MAX_PHY_TX_ABORT_CNT; i++)
    {
        HTT_STATS_PRINT(FATAL, "phy_tx_abort_cnt[%d] =%u",
            i, htt_stats_phy_counters_buf->phy_tx_abort_cnt[i]);
    }

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_phy_stats_tlv: display htt_print_phy_stats_tlv values
 * @tag_buf: buffer containing the tlv htt_print_phy_stats_tlv
 *
 * return:void
 */
static void htt_print_phy_stats_tlv(A_UINT32 *tag_buf)
{
    htt_phy_stats_tlv *htt_stats_phy_stats_buf =
              (htt_phy_stats_tlv *) tag_buf;
    A_UINT8 i;

    HTT_STATS_PRINT(FATAL, "HTT_PHY_STATS_TLV:");
    for (i = 0; i < HTT_STATS_MAX_CHAINS; i++)
    {
        HTT_STATS_PRINT(FATAL, "bdf_nf_chain%d = %d",
            i, htt_stats_phy_stats_buf->nf_chain[i]);
    }

    for (i = 0; i < HTT_STATS_MAX_CHAINS; i++)
    {
        HTT_STATS_PRINT(FATAL, "runTime_nf_chain%d = %d",
            i, htt_stats_phy_stats_buf->runTime_nf_chain[i]);
    }

    HTT_STATS_PRINT(FATAL, "false_radar_cnt = %u / %u (mins)",
            htt_stats_phy_stats_buf->false_radar_cnt,
            htt_stats_phy_stats_buf->fw_run_time);
    HTT_STATS_PRINT(FATAL, "radar_cs_cnt = %u",
            htt_stats_phy_stats_buf->radar_cs_cnt);
    HTT_STATS_PRINT(FATAL, "ani_level = %d",
            htt_stats_phy_stats_buf->ani_level);

    HTT_STATS_PRINT(FATAL, "current_OBW = %u",
            htt_stats_phy_stats_buf->current_OBW);
    HTT_STATS_PRINT(FATAL, "current_DBW = %u",
            htt_stats_phy_stats_buf->current_DBW);
    HTT_STATS_PRINT(FATAL, "last_radar_type = %u",
            htt_stats_phy_stats_buf->last_radar_type);
    HTT_STATS_PRINT(FATAL, "dfs_reg_domain = %u",
            htt_stats_phy_stats_buf->dfs_reg_domain);
    HTT_STATS_PRINT(FATAL, "radar_mask_bit = %u",
            htt_stats_phy_stats_buf->radar_mask_bit);
    HTT_STATS_PRINT(FATAL, "radar_rssi = %d",
            htt_stats_phy_stats_buf->radar_rssi);
    HTT_STATS_PRINT(FATAL, "radar_dfs_flags = %u",
            htt_stats_phy_stats_buf->radar_dfs_flags);
    HTT_STATS_PRINT(FATAL, "band_center_frequency_OBW = %u",
            htt_stats_phy_stats_buf->band_center_frequency_OBW);
    HTT_STATS_PRINT(FATAL, "band_center_frequency_DBW = %u",
            htt_stats_phy_stats_buf->band_center_frequency_DBW);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_phy_reset_counters_tlv: display htt_phy_reset_counters_tlv values
 * @tag_buf: buffer containing the tlv htt_phy_reset_counters_tlv
 *
 * return:void
 */
static void htt_print_phy_reset_counters_tlv(A_UINT32 *tag_buf)
{
    htt_phy_reset_counters_tlv *htt_stats_phy_reset_counters_buf =
                        (htt_phy_reset_counters_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_PHY_RESET_COUNTERS_TLV:");

    HTT_STATS_PRINT(FATAL, "reset_counts : pdev_id = %u",
            htt_stats_phy_reset_counters_buf->pdev_id);

    HTT_STATS_PRINT(FATAL, "reset_counts : cf_active_low_fail_cnt = %u",
            htt_stats_phy_reset_counters_buf->cf_active_low_fail_cnt);

    HTT_STATS_PRINT(FATAL, "reset_counts : cf_active_low_pass_cnt = %u",
            htt_stats_phy_reset_counters_buf->cf_active_low_pass_cnt);

    HTT_STATS_PRINT(FATAL, "reset_counts : phy_off_through_vreg_cnt = %u",
            htt_stats_phy_reset_counters_buf->phy_off_through_vreg_cnt);

    HTT_STATS_PRINT(FATAL, "reset_counts : force_calibration_cnt = %u",
            htt_stats_phy_reset_counters_buf->force_calibration_cnt);

    HTT_STATS_PRINT(FATAL, "reset_counts : rf_mode_switch_phy_off_cnt = %u",
            htt_stats_phy_reset_counters_buf->rf_mode_switch_phy_off_cnt);

    HTT_STATS_PRINT(FATAL, "reset_counts : temperature_recal_cnt = %u",
            htt_stats_phy_reset_counters_buf->temperature_recal_cnt);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_phy_reset_stats_tlv: display htt_phy_reset_stats_tlv values
 * @tag_buf: buffer containing the tlv htt_phy_reset_stats_tlv
 *
 * return:void
 */
static void htt_print_phy_reset_stats_tlv(A_UINT32 *tag_buf)
{
    htt_phy_reset_stats_tlv *htt_stats_phy_reset_stats_buf =
                    (htt_phy_reset_stats_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_PHY_RESET_STATS_TLV:");

    HTT_STATS_PRINT(FATAL, "reset_stats : pdev_id = %u",
            htt_stats_phy_reset_stats_buf->pdev_id);

    HTT_STATS_PRINT(FATAL, "reset_stats : chan_mhz = %u",
            htt_stats_phy_reset_stats_buf->chan_mhz);

    HTT_STATS_PRINT(FATAL, "reset_stats : chan_band_center_freq1 = %u",
            htt_stats_phy_reset_stats_buf->chan_band_center_freq1);

    HTT_STATS_PRINT(FATAL, "reset_stats : chan_band_center_freq2 = %u",
            htt_stats_phy_reset_stats_buf->chan_band_center_freq2);

    HTT_STATS_PRINT(FATAL, "reset_stats : chan_phy_mode = %u",
            htt_stats_phy_reset_stats_buf->chan_phy_mode);

    HTT_STATS_PRINT(FATAL, "reset_stats : chan_flags = 0x%x",
            htt_stats_phy_reset_stats_buf->chan_flags);

    HTT_STATS_PRINT(FATAL, "reset_stats : chan_num = %u",
            htt_stats_phy_reset_stats_buf->chan_num);

    HTT_STATS_PRINT(FATAL, "reset_stats : reset_cause = 0x%x",
            htt_stats_phy_reset_stats_buf->reset_cause);

    HTT_STATS_PRINT(FATAL, "reset_stats : prev_reset_cause = 0x%x",
            htt_stats_phy_reset_stats_buf->prev_reset_cause);

    HTT_STATS_PRINT(FATAL, "reset_stats : phy_warm_reset_src = 0x%x",
            htt_stats_phy_reset_stats_buf->phy_warm_reset_src);

    HTT_STATS_PRINT(FATAL, "reset_stats : rx_gain_tbl_mode = %d",
            htt_stats_phy_reset_stats_buf->rx_gain_tbl_mode);

    HTT_STATS_PRINT(FATAL, "reset_stats : xbar_val = 0x%x",
            htt_stats_phy_reset_stats_buf->xbar_val);

    HTT_STATS_PRINT(FATAL, "reset_stats : force_calibration = %u",
            htt_stats_phy_reset_stats_buf->force_calibration);

    HTT_STATS_PRINT(FATAL, "reset_stats : phyrf_mode = %u",
            htt_stats_phy_reset_stats_buf->phyrf_mode);

    HTT_STATS_PRINT(FATAL, "reset_stats : phy_homechan = %u",
            htt_stats_phy_reset_stats_buf->phy_homechan);

    HTT_STATS_PRINT(FATAL, "reset_stats : phy_tx_ch_mask = 0x%x",
            htt_stats_phy_reset_stats_buf->phy_tx_ch_mask);

    HTT_STATS_PRINT(FATAL, "reset_stats : phy_rx_ch_mask = 0x%x",
            htt_stats_phy_reset_stats_buf->phy_rx_ch_mask);

    HTT_STATS_PRINT(FATAL, "reset_stats : phybb_ini_mask = 0x%x",
            htt_stats_phy_reset_stats_buf->phybb_ini_mask);

    HTT_STATS_PRINT(FATAL, "reset_stats : phyrf_ini_mask = 0x%x",
            htt_stats_phy_reset_stats_buf->phyrf_ini_mask);

    HTT_STATS_PRINT(FATAL, "reset_stats : phy_dfs_en_mask = 0x%x",
            htt_stats_phy_reset_stats_buf->phy_dfs_en_mask);

    HTT_STATS_PRINT(FATAL, "reset_stats : phy_sscan_en_mask = 0x%x",
            htt_stats_phy_reset_stats_buf->phy_sscan_en_mask);

    HTT_STATS_PRINT(FATAL, "reset_stats : phy_synth_sel_mask = 0x%x",
            htt_stats_phy_reset_stats_buf->phy_synth_sel_mask);

    HTT_STATS_PRINT(FATAL, "reset_stats : phy_adfs_freq = %u",
            htt_stats_phy_reset_stats_buf->phy_adfs_freq);

    HTT_STATS_PRINT(FATAL, "reset_stats : cck_fir_settings = 0x%x",
            htt_stats_phy_reset_stats_buf->cck_fir_settings);

    HTT_STATS_PRINT(FATAL, "reset_stats : phy_dyn_pri_chan = %u",
            htt_stats_phy_reset_stats_buf->phy_dyn_pri_chan);

    HTT_STATS_PRINT(FATAL, "reset_stats : cca_thresh = 0x%x",
            htt_stats_phy_reset_stats_buf->cca_thresh);

    HTT_STATS_PRINT(FATAL, "reset_stats : dyn_cca_status = %u",
            htt_stats_phy_reset_stats_buf->dyn_cca_status);

    HTT_STATS_PRINT(FATAL, "reset_stats : rxdesense_thresh_hw = 0x%x",
            htt_stats_phy_reset_stats_buf->rxdesense_thresh_hw);
    HTT_STATS_PRINT(FATAL, "reset_stats : rxdesense_thresh_sw = 0x%x",
            htt_stats_phy_reset_stats_buf->rxdesense_thresh_sw);

    HTT_STATS_PRINT(FATAL, "reset_stats : phy_bw_code = %u",
            htt_stats_phy_reset_stats_buf->phy_bw_code);
    HTT_STATS_PRINT(FATAL, "reset_stats : phy_rate_mode = %u",
            htt_stats_phy_reset_stats_buf->phy_rate_mode);
    HTT_STATS_PRINT(FATAL, "reset_stats : phy_band_code = %u",
            htt_stats_phy_reset_stats_buf->phy_band_code);

    HTT_STATS_PRINT(FATAL, "reset_stats : phy_vreg_base = 0x%x",
            htt_stats_phy_reset_stats_buf->phy_vreg_base);
    HTT_STATS_PRINT(FATAL, "reset_stats : phy_vreg_base_ext = 0x%x",
            htt_stats_phy_reset_stats_buf->phy_vreg_base_ext);

    HTT_STATS_PRINT(FATAL, "reset_stats : cur_table_index = %u",
            htt_stats_phy_reset_stats_buf->cur_table_index);

     HTT_STATS_PRINT(FATAL, "reset_stats : whal_config_flag = %u",
             htt_stats_phy_reset_stats_buf->whal_config_flag);

    HTT_STATS_PRINT(FATAL, "reset_stats : nfcal_iteration_count_home = %u",
             htt_stats_phy_reset_stats_buf->nfcal_iteration_counts[0]);

    HTT_STATS_PRINT(FATAL, "reset_stats : nfcal_iteration_count_scan = %u",
             htt_stats_phy_reset_stats_buf->nfcal_iteration_counts[1]);

    HTT_STATS_PRINT(FATAL, "reset_stats : nfcal_iteration_count_periodic = %u",
             htt_stats_phy_reset_stats_buf->nfcal_iteration_counts[2]);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_pdev_bw_mgr_stats_tlv: display htt_pdev_bw_mgr_stats_tlv values
 * @tag_buf: buffer containing the tlv htt_pdev_bw_mgr_stats_tlv
 *
 * return:void
 */
static void htt_print_pdev_bw_mgr_stats_tlv(A_UINT32 *tag_buf)
{

    htt_pdev_bw_mgr_stats_tlv *htt_stats_buf =
        (htt_pdev_bw_mgr_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_PDEV_BW_MGR_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "Mac_id = %u",
            HTT_BW_MGR_STATS_MAC_ID_GET(htt_stats_buf->mac_id__pri20_idx__freq));

    HTT_STATS_PRINT(FATAL, "Phy_mode = %u",
            HTT_BW_MGR_STATS_CHAN_PHY_MODE_GET(htt_stats_buf->phy_mode__static_pattern));

    HTT_STATS_PRINT(FATAL, "Pri20 = %uMhz, centre freq1 = %uMhz, freq2 = %uMhz",
            HTT_BW_MGR_STATS_PRI20_FREQ_GET(htt_stats_buf->mac_id__pri20_idx__freq),
            HTT_BW_MGR_STATS_CENTER_FREQ1_GET(htt_stats_buf->centre_freq1__freq2),
            HTT_BW_MGR_STATS_CENTER_FREQ2_GET(htt_stats_buf->centre_freq1__freq2)
            );

    HTT_STATS_PRINT(FATAL, "Pri20_index = %u",
            HTT_BW_MGR_STATS_PRI20_IDX_GET(htt_stats_buf->mac_id__pri20_idx__freq));

    HTT_STATS_PRINT(FATAL, "Configured Static Punctured Pattern = 0x%x",
            HTT_BW_MGR_STATS_STATIC_PATTERN_GET(htt_stats_buf->phy_mode__static_pattern));

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_phy_tpc_stats_tlv: display htt_print_phy_stats_tlv values
 * @tag_buf: buffer containing the tlv htt_print_phy_stats_tlv
 *
 * return:void
 */
static void htt_print_phy_tpc_stats_tlv(A_UINT32 *tag_buf)
{
    htt_phy_tpc_stats_tlv *htt_stats_phy_tpc_stats_buf =
              (htt_phy_tpc_stats_tlv *) tag_buf;
    A_UINT8 i, j, k;
    A_UINT16 index = 0;
    char str_buf[HTT_MAX_STRING_LEN] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_PHY_TPC_STATS_TLV:");

    HTT_STATS_PRINT(FATAL, "tpc_stats : pdev_id = %u",
            htt_stats_phy_tpc_stats_buf->pdev_id);
    HTT_STATS_PRINT(FATAL, "tpc_stats : tx_power_scale = %u",
            htt_stats_phy_tpc_stats_buf->tx_power_scale);
    HTT_STATS_PRINT(FATAL, "tpc_stats : tx_power_scale_db = %u",
            htt_stats_phy_tpc_stats_buf->tx_power_scale_db);
    HTT_STATS_PRINT(FATAL, "tpc_stats : min_negative_tx_power = %d",
            htt_stats_phy_tpc_stats_buf->min_negative_tx_power);
    HTT_STATS_PRINT(FATAL, "tpc_stats : reg_ctl_domain = %u",
            htt_stats_phy_tpc_stats_buf->reg_ctl_domain);
    HTT_STATS_PRINT(FATAL, "tpc_stats : twice_max_rd_power = %u",
            htt_stats_phy_tpc_stats_buf->twice_max_rd_power);
    HTT_STATS_PRINT(FATAL, "tpc_stats : max_tx_power = %u",
            htt_stats_phy_tpc_stats_buf->max_tx_power);
    HTT_STATS_PRINT(FATAL, "tpc_stats : home_max_tx_power = %u",
            htt_stats_phy_tpc_stats_buf->home_max_tx_power);
    HTT_STATS_PRINT(FATAL, "tpc_stats : psd_power = %d",
            htt_stats_phy_tpc_stats_buf->psd_power);
    HTT_STATS_PRINT(FATAL, "tpc_stats : eirp_power = %u",
            htt_stats_phy_tpc_stats_buf->eirp_power);
    HTT_STATS_PRINT(FATAL, "tpc_stats : power_type_6ghz = %u",
            htt_stats_phy_tpc_stats_buf->power_type_6ghz);

    for (i = 0; i < HTT_STATS_MAX_CHAINS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%d,", i,
            htt_stats_phy_tpc_stats_buf->max_reg_allowed_power[i]);
    }
    HTT_STATS_PRINT(FATAL, "tpc_stats : max_reg_allowed_power = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_MAX_CHAINS; i++){
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%d,", i,
            htt_stats_phy_tpc_stats_buf->max_reg_allowed_power_6g[i]);
    }
    HTT_STATS_PRINT(FATAL,
        "tpc_stats : max_reg_allowed_power_6g = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_MAX_CH_PWR_INFO_SIZE; i++){
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_phy_tpc_stats_buf->sub_band_cfreq[i]);
    }
    HTT_STATS_PRINT(FATAL, "tpc_stats : sub_band_cfreq = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_MAX_CH_PWR_INFO_SIZE; i++){
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i, htt_stats_phy_tpc_stats_buf->sub_band_txpower[i]);
    }
    HTT_STATS_PRINT(FATAL, "tpc_stats : sub_band_txpower = %s", str_buf);

    HTT_STATS_PRINT(FATAL, "tpc_stats : ctl_array_gain_cap_ext2_enabled = %u",
        HTT_PHY_TPC_STATS_AG_CAP_EXT2_ENABLED_GET(
            htt_stats_phy_tpc_stats_buf->ctl_args));
    HTT_STATS_PRINT(FATAL, "tpc_stats : ctl_flag = %u",
        HTT_PHY_TPC_STATS_CTL_FLAG_GET(
            htt_stats_phy_tpc_stats_buf->ctl_args));
    HTT_STATS_PRINT(FATAL,
        "tpc_stats : ctl_array_gain_cap_ext2_ctl_region_grp = %u",
        HTT_PHY_TPC_STATS_CTL_REGION_GRP_GET(
            htt_stats_phy_tpc_stats_buf->ctl_args));
    HTT_STATS_PRINT(FATAL, "tpc_stats : ctl_sub_band_index = %u",
        HTT_PHY_TPC_STATS_SUB_BAND_INDEX_GET(
            htt_stats_phy_tpc_stats_buf->ctl_args));

    i = 0;
    j = 0;
    for (k = 0; k < (HTT_STATS_MAX_CHAINS+1) * HTT_STATS_MAX_CHAINS/2; k++) {
        HTT_STATS_PRINT(FATAL,
            "tpc_stats : ctl_array_gain_cap[ntx:%d][nss:%d] = %d",
            i+1, j+1, htt_stats_phy_tpc_stats_buf->array_gain_cap[k]);
        if (j == i) {
            /* advance to the next row of the triangular matrix */
            i++;
            j = 0;
        } else {
            j++;
        }
    }

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_MAX_CHAINS; i++){
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%d,", i,
            htt_stats_phy_tpc_stats_buf->max_reg_only_allowed_power[i]);
    }
    HTT_STATS_PRINT(FATAL,
        "tpc_stats : max_reg_only_allowed_power = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_MAX_CHAINS; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            "%u:%u,\t", i + 1, htt_stats_phy_tpc_stats_buf->tx_num_chains[i]);
    }
    HTT_STATS_PRINT(FATAL, "tpc_stats : tx_num_chains = %s", str_buf);

    /* stats for negative power cases from -10 dBm to 0 dBm */
    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = HTT_MAX_NEGATIVE_POWER_LEVEL; i > 0; i--) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            "-%u:%u,\t", i, htt_stats_phy_tpc_stats_buf->tx_power_neg[i - 1]);
    }
    HTT_STATS_PRINT(FATAL, "tpc_stats : tx_power_neg = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_MAX_POWER_LEVEL; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                "%u:%u,\t", i, htt_stats_phy_tpc_stats_buf->tx_power[i]);
    }
    HTT_STATS_PRINT(FATAL, "tpc_stats : tx_power = %s\n", str_buf);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

static void htt_print_soc_txrx_stats_common_tlv(A_UINT32 *tag_buf)
{
    htt_t2h_soc_txrx_stats_common_tlv *htt_t2h_soc_txrx_stats_common_buf =
                    (htt_t2h_soc_txrx_stats_common_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_SOC_COMMON_STATS_TLV:");

    HTT_STATS_PRINT(FATAL, "soc_drop_count = 0x%08x%08x",
            htt_t2h_soc_txrx_stats_common_buf->inv_peers_msdu_drop_count_hi,
            htt_t2h_soc_txrx_stats_common_buf->inv_peers_msdu_drop_count_lo);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

static void htt_print_vdev_txrx_stats_hw_tlv(A_UINT32 *tag_buf)
{
    htt_t2h_vdev_txrx_stats_hw_stats_tlv *htt_stats_vdev_txrx_stats_hw_stats_buf =
                    (htt_t2h_vdev_txrx_stats_hw_stats_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_VDEV_TXRX_STATS_TLV:");

    HTT_STATS_PRINT(FATAL, "vdev_id = %u",
            htt_stats_vdev_txrx_stats_hw_stats_buf->vdev_id);

    HTT_STATS_PRINT(FATAL, "rx_msdu_byte_cnt = 0x%08x%08x",
            htt_stats_vdev_txrx_stats_hw_stats_buf->rx_msdu_byte_cnt_hi,
            htt_stats_vdev_txrx_stats_hw_stats_buf->rx_msdu_byte_cnt_lo);

    HTT_STATS_PRINT(FATAL, "rx_msdu_cnt = 0x%08x%08x",
            htt_stats_vdev_txrx_stats_hw_stats_buf->rx_msdu_cnt_hi,
            htt_stats_vdev_txrx_stats_hw_stats_buf->rx_msdu_cnt_lo);

    HTT_STATS_PRINT(FATAL, "tx_msdu_byte_cnt = 0x%08x%08x",
            htt_stats_vdev_txrx_stats_hw_stats_buf->tx_msdu_byte_cnt_hi,
            htt_stats_vdev_txrx_stats_hw_stats_buf->tx_msdu_byte_cnt_lo);

    HTT_STATS_PRINT(FATAL, "tx_msdu_cnt = 0x%08x%08x",
            htt_stats_vdev_txrx_stats_hw_stats_buf->tx_msdu_cnt_hi,
            htt_stats_vdev_txrx_stats_hw_stats_buf->tx_msdu_cnt_lo);

    HTT_STATS_PRINT(FATAL, "tx_msdu_excessive_retry_discard_cnt = 0x%08x%08x",
            htt_stats_vdev_txrx_stats_hw_stats_buf->tx_msdu_excessive_retry_discard_cnt_hi,
            htt_stats_vdev_txrx_stats_hw_stats_buf->tx_msdu_excessive_retry_discard_cnt_lo);

    HTT_STATS_PRINT(FATAL, "tx_msdu_cong_ctrl_drop_cnt = 0x%08x%08x",
            htt_stats_vdev_txrx_stats_hw_stats_buf->tx_msdu_cong_ctrl_drop_cnt_hi,
            htt_stats_vdev_txrx_stats_hw_stats_buf->tx_msdu_cong_ctrl_drop_cnt_lo);

    HTT_STATS_PRINT(FATAL, "tx_msdu_ttl_expire_drop_cnt = 0x%08x%08x",
            htt_stats_vdev_txrx_stats_hw_stats_buf->tx_msdu_ttl_expire_drop_cnt_hi,
            htt_stats_vdev_txrx_stats_hw_stats_buf->tx_msdu_ttl_expire_drop_cnt_lo);

    HTT_STATS_PRINT(FATAL, "tqm_bypass_frame_cnt = 0x%08x%08x",
            htt_stats_vdev_txrx_stats_hw_stats_buf->tqm_bypass_frame_cnt_hi,
            htt_stats_vdev_txrx_stats_hw_stats_buf->tqm_bypass_frame_cnt_lo);

    HTT_STATS_PRINT(FATAL, "tqm_bypass_byte_cnt = 0x%08x%08x",
            htt_stats_vdev_txrx_stats_hw_stats_buf->tqm_bypass_byte_cnt_hi,
            htt_stats_vdev_txrx_stats_hw_stats_buf->tqm_bypass_byte_cnt_lo);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

static void htt_print_tx_pdev_be_dl_mu_ofdma_sch_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_be_dl_mu_ofdma_sch_stats_tlv *htt_stats_buf =
        (htt_tx_pdev_be_dl_mu_ofdma_sch_stats_tlv *)tag_buf;
    A_UINT8 i;

    HTT_STATS_PRINT(FATAL, "\n11ax BE DL MU_OFDMA SCH STATS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "be_mu_ofdma_sch_nusers_%u = %u", i,
                htt_stats_buf->be_mu_ofdma_sch_nusers[i]);
    }
}

static void htt_print_tx_pdev_be_ul_mu_ofdma_sch_stats_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_be_ul_mu_ofdma_sch_stats_tlv *htt_stats_buf =
        (htt_tx_pdev_be_ul_mu_ofdma_sch_stats_tlv *)tag_buf;
    A_UINT8 i;

    HTT_STATS_PRINT(FATAL, "\n11ax BE UL MU_OFDMA SCH STATS:");

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_OFDMA_USER_STATS; i++) {
        HTT_STATS_PRINT(FATAL, "be_ul_mu_ofdma_basic_sch_nusers_%u = %u", i,
                htt_stats_buf->be_ul_mu_ofdma_basic_sch_nusers[i]);
        HTT_STATS_PRINT(FATAL, "be_ul_mu_ofdma_bsr_sch_nusers_%u = %u", i,
                htt_stats_buf->be_ul_mu_ofdma_bsr_sch_nusers[i]);
        HTT_STATS_PRINT(FATAL, "be_ul_mu_ofdma_bar_sch_nusers_%u = %u", i,
                htt_stats_buf->be_ul_mu_ofdma_bar_sch_nusers[i]);
        HTT_STATS_PRINT(FATAL, "be_ul_mu_ofdma_brp_sch_nusers_%u = %u\n", i,
                htt_stats_buf-> be_ul_mu_ofdma_brp_sch_nusers[i]);
    }
}

/*
 * htt_print_tx_pdev_rate_stats_be_ofdma_tlv: display htt_tx_pdev_rate_stats_tlv
 * @tag_buf: buffer containing the tlv htt_tx_pdev_rate_stats_tlv
 *
 * return:void
 */
static void htt_print_tx_pdev_rate_stats_be_ofdma_tlv(A_UINT32 *tag_buf)
{
    htt_tx_pdev_rate_stats_be_ofdma_tlv *htt_stats_buf =
        (htt_tx_pdev_rate_stats_be_ofdma_tlv *)tag_buf;
    A_UINT8  i, j;
    A_UINT16 index                       = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN] = {0};
    A_CHAR   *tx_gi[HTT_TX_PEER_STATS_NUM_GI_COUNTERS];

    for (i = 0; i < HTT_TX_PEER_STATS_NUM_GI_COUNTERS; i++) {
        tx_gi[i] = (A_CHAR *)malloc(HTT_MAX_STRING_LEN);
        if (!tx_gi[i]) {
           HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
           for (j = 0; j < i; j++) {
               free(tx_gi[j]);
           }
           return;
        }
    }

    HTT_STATS_PRINT(FATAL, "HTT_TX_PDEV_RATE_STATS_BE_OFDMA_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
            htt_stats_buf->mac_id__word & 0xFF);

    HTT_STATS_PRINT(FATAL, "be_ofdma_tx_ldpc = %u",
            htt_stats_buf->be_ofdma_tx_ldpc);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->be_ofdma_tx_mcs[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_ofdma_tx_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_EHT_SIG_MCS_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->be_ofdma_eht_sig_mcs[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_ofdma_eht_sig_mcs = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_RU_SIZE_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %s:%u,", htt_be_tx_ru_size_to_str(i),
                htt_stats_buf->be_ofdma_tx_ru_size[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_ofdma_tx_ru_size = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_AX_RU_SIZE_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %s:%u,", htt_ax_tx_ru_size_to_str(i),
                htt_stats_buf->be_ofdma_ba_ru_size[i]);
    }
    HTT_STATS_PRINT(FATAL, "be_ofdma_ba_ru_size = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_SPATIAL_STREAMS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i + 1, htt_stats_buf->be_ofdma_tx_nss[i]);
    }

    HTT_STATS_PRINT(FATAL, "be_ofdma_tx_nss = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);

    for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_BW_COUNTERS; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->be_ofdma_tx_bw[i]);
    }

    HTT_STATS_PRINT(FATAL, "be_ofdma_tx_bw = %s ", str_buf);

    /* DL OFDMA GI Stats */
    for (j = 0; j < HTT_TX_PDEV_STATS_NUM_GI_COUNTERS; j++) {
        index = 0;
        for (i = 0; i < HTT_TX_PDEV_STATS_NUM_BE_MCS_COUNTERS; i++) {
            index += snprintf(&tx_gi[j][index],
                    HTT_MAX_STRING_LEN - index,
                    " %u:%u,", i,
                    htt_stats_buf->be_ofdma_tx_gi[j][i]);
        }
        HTT_STATS_PRINT(FATAL, "be_ofdma_tx_gi[%u] = %s ", j, tx_gi[j]);
    }

    for (i = 0; i < HTT_TX_PEER_STATS_NUM_GI_COUNTERS; i++) {
        free(tx_gi[i]);
    }
}

static const char *htt_stats_ppdu_type_abbrev(HTT_STATS_PPDU_TYPE ppdu_type)
{
    const char *ppdu_type_str = "unknown";

    switch (ppdu_type) {
    case HTT_STATS_PPDU_TYPE_MODE_SU:
        ppdu_type_str = "su";
        break;

    case HTT_STATS_PPDU_TYPE_DL_MU_MIMO:
        ppdu_type_str = "dl_mu_mimo";
        break;

    case HTT_STATS_PPDU_TYPE_UL_MU_MIMO:
        ppdu_type_str = "ul_mu_mimo";
        break;

    case HTT_STATS_PPDU_TYPE_DL_MU_OFDMA:
        ppdu_type_str = "dl_mu_ofdma";
        break;

    case HTT_STATS_PPDU_TYPE_UL_MU_OFDMA:
        ppdu_type_str = "ul_mu_ofdma";
        break;

    default:
        HTT_STATS_PRINT(FATAL, "warning: encountered unexpected HTT_STATS_PPDU_TYPE: %d",
            ppdu_type);
        break;
    }

    return ppdu_type_str;
}

static const char *htt_stats_direction_abbrev(HTT_STATS_DIRECTION direction)
{
    const char *direction_str = "unknown";

    switch (direction) {
    case HTT_STATS_DIRECTION_TX:
        direction_str = "tx";
        break;

    case HTT_STATS_DIRECTION_RX:
        direction_str = "rx";
        break;

    default:
        HTT_STATS_PRINT(FATAL, "warning: encountered unexpected HTT_STATS_DIRECTION: %d",
            direction);
        direction_str = "unknown";
        break;
    }

    return direction_str;
}

static const char *htt_stats_pream_type_abbrev(HTT_STATS_PREAM_TYPE pream_type)
{
    const char *pream_type_str = "unknown";

    switch (pream_type) {
    case HTT_STATS_PREAM_OFDM:
        pream_type_str = "ofdm";
        break;

    case HTT_STATS_PREAM_CCK:
        pream_type_str = "cck";
        break;

    case HTT_STATS_PREAM_HT:
        pream_type_str = "ht";
        break;

    case HTT_STATS_PREAM_VHT:
        pream_type_str = "ac";
        break;

    case HTT_STATS_PREAM_HE:
        pream_type_str = "ax";
        break;

    case HTT_STATS_PREAM_EHT:
        pream_type_str = "be";
        break;

    default:
        HTT_STATS_PRINT(FATAL, "warning: encountered unexpected HTT_STATS_PREAM_TYPE: %d",
            pream_type);
        break;
    }

    return pream_type_str;
}

static void htt_print_ml_peer_details_stats_tlv(A_UINT32 *tag_buf)
{
    htt_ml_peer_details_tlv *htt_stats_buf =
        (htt_ml_peer_details_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_ML_PEER_DETAILS_TLV:");
    HTT_STATS_PRINT(FATAL, "========================");

    HTT_STATS_PRINT(FATAL,
        "remote_mld_mac_addr = %02x:%02x:%02x:%02x:%02x:%02x",
        (htt_stats_buf->remote_mld_mac_addr.mac_addr31to0 & 0xFF),
        (htt_stats_buf->remote_mld_mac_addr.mac_addr31to0 & 0xFF00) >> 8,
        (htt_stats_buf->remote_mld_mac_addr.mac_addr31to0 & 0xFF0000) >> 16,
        (htt_stats_buf->remote_mld_mac_addr.mac_addr31to0 & 0xFF000000) >> 24,
        (htt_stats_buf->remote_mld_mac_addr.mac_addr47to32 & 0xFF),
        (htt_stats_buf->remote_mld_mac_addr.mac_addr47to32 & 0xFF00) >> 8);

    HTT_STATS_PRINT(FATAL, "ml_peer_flags = 0x%x",
        htt_stats_buf->ml_peer_flags);

    HTT_STATS_PRINT(FATAL, "num_links = %u",
        HTT_ML_PEER_DETAILS_NUM_LINKS_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "ml_peer_id = %u",
        HTT_ML_PEER_DETAILS_ML_PEER_ID_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "primary_link_idx = %u",
        HTT_ML_PEER_DETAILS_PRIMARY_LINK_IDX_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "primary_chip_id = %u",
        HTT_ML_PEER_DETAILS_PRIMARY_CHIP_ID_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "link_init_count = %u",
        HTT_ML_PEER_DETAILS_LINK_INIT_COUNT_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "non_str = %u",
        HTT_ML_PEER_DETAILS_NON_STR_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "is_emlsr_active = %u",
        HTT_ML_PEER_DETAILS_IS_EMLSR_ACTIVE_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "is_stako = %u",
        HTT_ML_PEER_DETAILS_IS_STA_KO_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "num_local_links = %u",
        HTT_ML_PEER_DETAILS_NUM_LOCAL_LINKS_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "allocated = %u",
        HTT_ML_PEER_DETAILS_ALLOCATED_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "emlsr_support = %u",
        HTT_ML_PEER_DETAILS_EMLSR_SUPPORT_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "participating_chips_bitmap = 0x%x",
        HTT_ML_PEER_DETAILS_PARTICIPATING_CHIPS_BITMAP_GET(
            htt_stats_buf->msg_dword_2));

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

static void htt_print_ml_link_info_stats_tlv(A_UINT32 *tag_buf)
{
    htt_ml_link_info_tlv *htt_stats_buf =
        (htt_ml_link_info_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_ML_LINK_INFO_TLV:");
    HTT_STATS_PRINT(FATAL, "=====================");

    HTT_STATS_PRINT(FATAL, "valid             = %u",
        HTT_ML_LINK_INFO_VALID_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "active            = %u",
        HTT_ML_LINK_INFO_ACTIVE_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "primary           = %u",
        HTT_ML_LINK_INFO_PRIMARY_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "assoc_link        = %u",
        HTT_ML_LINK_INFO_ASSOC_LINK_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "chip_id           = %u",
        HTT_ML_LINK_INFO_CHIP_ID_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "ieee_link_id      = %u",
        HTT_ML_LINK_INFO_IEEE_LINK_ID_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "hw_link_id        = %u",
        HTT_ML_LINK_INFO_HW_LINK_ID_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "logical_link_id   = %u",
        HTT_ML_LINK_INFO_LOGICAL_LINK_ID_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "master_link       = %u",
        HTT_ML_LINK_INFO_MASTER_LINK_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "anchor_link       = %u",
        HTT_ML_LINK_INFO_ANCHOR_LINK_GET(htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "initialized       = %u",
        HTT_ML_LINK_INFO_INITIALIZED_GET(htt_stats_buf->msg_dword_1));

    HTT_STATS_PRINT(FATAL, "sw_peer_id        = %u",
        HTT_ML_LINK_INFO_SW_PEER_ID_GET(htt_stats_buf->msg_dword_2));
    HTT_STATS_PRINT(FATAL, "vdev_id           = %u",
        HTT_ML_LINK_INFO_VDEV_ID_GET(htt_stats_buf->msg_dword_2));

    HTT_STATS_PRINT(FATAL, "primary_tid_mask  = 0x%x",
        htt_stats_buf->primary_tid_mask);

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

static void htt_print_ml_peer_ext_stats_tlv(A_UINT32 *tag_buf)
{
    htt_ml_peer_ext_details_tlv *htt_stats_buf =
        (htt_ml_peer_ext_details_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_ML_PEER_EXT_TLV:");
    HTT_STATS_PRINT(FATAL, "====================");

    HTT_STATS_PRINT(FATAL, "peer_assoc_ipc_recvd    = %u",
        HTT_ML_PEER_EXT_DETAILS_PEER_ASSOC_IPC_RECVD_GET(
            htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "sched_peer_delete_recvd = %u",
        HTT_ML_PEER_EXT_DETAILS_SCHED_PEER_DELETE_RECVD_GET(
            htt_stats_buf->msg_dword_1));
    HTT_STATS_PRINT(FATAL, "mld_ast_index           = %u",
        HTT_ML_PEER_EXT_DETAILS_MLD_AST_INDEX_GET(htt_stats_buf->msg_dword_1));

    HTT_STATS_PRINT(FATAL, "=========================================== \n");
}

/*
 * htt_print_puncture_stats_tlv: display htt_pdev_puncture_stats_tlv tlv
 * @tag_buf: buffer containing the tlv htt_pdev_puncture_stats_tlv
 *
 * return:void
 */
static void htt_print_puncture_stats_tlv(A_UINT8 tag_type, A_UINT32 *tag_buf)
{
    htt_pdev_puncture_stats_tlv *htt_stats_buf =
        (htt_pdev_puncture_stats_tlv *)tag_buf;
    A_UINT8  i;
    const char *direction_str = NULL;
    const char *preamble_str = NULL;
    const char *ppdu_type_str = NULL;

    HTT_STATS_PRINT(FATAL, "HTT_PDEV_PUNCTURE_STATS_TLV:");

    direction_str = htt_stats_direction_abbrev(htt_stats_buf->direction);
    ppdu_type_str = htt_stats_ppdu_type_abbrev(htt_stats_buf->ppdu_type);
    preamble_str = htt_stats_pream_type_abbrev(htt_stats_buf->preamble);

    HTT_STATS_PRINT(FATAL, "mac_id = %u",
        HTT_PDEV_PUNCTURE_STATS_MAC_ID_GET(htt_stats_buf->mac_id__word));

    HTT_STATS_PRINT(FATAL, "%s_%s_%s_last_used_pattern_mask: 0x%08x",
        direction_str, preamble_str, ppdu_type_str, htt_stats_buf->last_used_pattern_mask);

    for (i = 0; i < HTT_PUNCTURE_STATS_MAX_SUBBAND_COUNT && i < htt_stats_buf->subband_count; ++i) {
        HTT_STATS_PRINT(FATAL, "%s_%s_%s_num_subbands_used_cnt_%02d: %u",
            direction_str, preamble_str, ppdu_type_str, i + 1, htt_stats_buf->num_subbands_used_cnt[i]);
    }
}

/*
 *  htt_print_pdev_mbssid_ctrl_frame_stats_tlv:
 *      display htt_pdev_mbssid_ctrl_frame_stats_tlv tlv
 *  @tag_buf: buffer containing the tlv htt_pdev_mbssid_ctrl_frame_stats_tlv
 *
 *  return:void
 */
static void htt_print_pdev_mbssid_ctrl_frame_stats_tlv(A_UINT32 *tag_buf)
{
    htt_pdev_mbssid_ctrl_frame_stats_tlv *htt_stats_buf =
        (htt_pdev_mbssid_ctrl_frame_stats_tlv *)tag_buf;

    HTT_STATS_PRINT(FATAL, "mac_id = %u",
        HTT_STATS_CMN_MAC_ID_GET(htt_stats_buf->mac_id__word));

    HTT_STATS_PRINT(FATAL, "basic_trigger_across_bss_count = %u ",
        htt_stats_buf->basic_trigger_across_bss);
    HTT_STATS_PRINT(FATAL, "basic_trigger_within_bss_count = %u ",
        htt_stats_buf->basic_trigger_within_bss);
    HTT_STATS_PRINT(FATAL, "bsr_trigger_across_bss_count = %u ",
        htt_stats_buf->bsr_trigger_across_bss);
    HTT_STATS_PRINT(FATAL, "bsr_trigger_within_bss_count = %u ",
        htt_stats_buf->bsr_trigger_within_bss);
    HTT_STATS_PRINT(FATAL, "MU_RTS_across_bss_count = %u ",
        htt_stats_buf->mu_rts_across_bss);
    HTT_STATS_PRINT(FATAL, "MU_RTS_within_bss_count = %u ",
        htt_stats_buf->mu_rts_within_bss);
    HTT_STATS_PRINT(FATAL, "UL_MUMIMO_trigger_across_bss_count = %u ",
        htt_stats_buf->ul_mumimo_trigger_across_bss);
    HTT_STATS_PRINT(FATAL, "UL_MUMIMO_trigger_within_bss_count = %u ",
        htt_stats_buf->ul_mumimo_trigger_within_bss);
}

/*
 * htt_print_pdev_sched_algo_ofdma_stats_tlv: display htt_pdev_sched_algo_ofdma_stats_tlv tlv
 * @tag_buf: buffer containing the tlv htt_pdev_sched_algo_stats_tlv
 *
 * return:void
 */
static void htt_print_pdev_sched_algo_ofdma_stats_tlv(A_UINT32 *tag_buf)
{
    htt_pdev_sched_algo_ofdma_stats_tlv *htt_stats_buf =
        (htt_pdev_sched_algo_ofdma_stats_tlv *)tag_buf;

    A_UINT32 i;
    A_UINT16 index;
    char str_buf[HTT_MAX_STRING_LEN] = {0};

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rate_based_dlofdma_enabled_count[i]);
    }
    HTT_STATS_PRINT(FATAL, "rate_based_dlofdma_enabled_count = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rate_based_dlofdma_disabled_count[i]);
    }
    HTT_STATS_PRINT(FATAL, "rate_based_dlofdma_disabled_count = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rate_based_dlofdma_probing_count[i]);
    }
    HTT_STATS_PRINT(FATAL, "rate_based_dlofdma_probing_count = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->rate_based_dlofdma_monitoring_count[i]);
    }
    HTT_STATS_PRINT(FATAL, "rate_based_dlofdma_monitoring_count = %s \n", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->chan_acc_lat_based_dlofdma_enabled_count[i]);
    }
    HTT_STATS_PRINT(FATAL, "chan_acc_lat_based_dlofdma_enabled_count = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->chan_acc_lat_based_dlofdma_disabled_count[i]);
    }
    HTT_STATS_PRINT(FATAL, "chan_acc_lat_based_dlofdma_disabled_count = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->chan_acc_lat_based_dlofdma_monitoring_count[i]);
    }
    HTT_STATS_PRINT(FATAL, "chan_acc_lat_based_dlofdma_monitoring_count = %s ", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->downgrade_to_dl_su_ru_alloc_fail[i]);
    }
    HTT_STATS_PRINT(FATAL, "downgrade_to_dl_su_ru_alloc_fail = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->candidate_list_single_user_disable_ofdma[i]);
    }
    HTT_STATS_PRINT(FATAL, "candidate_list_single_user_disable_ofdma = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->dl_cand_list_dropped_high_ul_qos_weight[i]);
    }
    HTT_STATS_PRINT(FATAL, "dl_cand_list_dropped_high_ul_qos_weight = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->ax_dlofdma_disabled_due_to_pipelining[i]);
    }
    HTT_STATS_PRINT(FATAL, "ax_dlofdma_disabled_due_to_pipelining = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->dlofdma_disabled_su_only_eligible[i]);
    }
    HTT_STATS_PRINT(FATAL, "dlofdma_disabled_su_only_eligible = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->dlofdma_disabled_consec_no_mpdus_tried[i]);
    }
    HTT_STATS_PRINT(FATAL, "dlofdma_disabled_consec_no_mpdus_tried = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_NUM_AC_WMM; i++) {
        index += snprintf(&str_buf[index],
                HTT_MAX_STRING_LEN - index,
                " %u:%u,", i, htt_stats_buf->dlofdma_disabled_consec_no_mpdus_success[i]);
    }
    HTT_STATS_PRINT(FATAL, "dlofdma_disabled_consec_no_mpdus_success = %s", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0 ; i < HTT_STATS_SCHED_OFDMA_TXBF_INELIGIBILITY_MAX; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index,
            " %u:%u,", i,htt_stats_buf->txbf_ofdma_ineligibility_stat[i]);
    }
    HTT_STATS_PRINT(FATAL, "sched_ofdma_txbf_ineligibility = %s\n", str_buf);

    index = 0;
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0 ; i < HTT_MAX_NUM_CHAN_ACC_LAT_INTR; i++) {
        index += snprintf(&str_buf[index],
            HTT_MAX_STRING_LEN - index, " %u:%u,",
            i, htt_stats_buf->avg_chan_acc_lat_hist[i]);
    }
    HTT_STATS_PRINT(FATAL, "avg_chan_acc_lat_hist = %s", str_buf);

    HTT_STATS_PRINT(FATAL, " "); /* add a blank line after the TLV printout */
}

/* DEPRECATED
 *
 * htt_print_latency_prof_cal_stats_tlv: display htt_latency_prof_cal_stats_tlv tlv
 * @tag_buf: buffer containing the tlv htt_latency_prof_cal_stats_tlv
 *
 * return:void
 */
static void htt_print_latency_prof_cal_stats_tlv(A_UINT32 *tag_buf)
{
    A_UINT32 i, j;
    htt_latency_prof_cal_stats_tlv *htt_stats_buf =
        (htt_latency_prof_cal_stats_tlv *) tag_buf;
    A_CHAR latency_prof_stat_name[HTT_STATS_MAX_PROF_STATS_NAME_LEN + 1] = {0};

    HTT_STATS_PRINT(FATAL, "HTT_STATS_LATENCY_PROF_CAL_TLV:");

    for (i = 1; i < HTT_STATS_MAX_PROF_CAL; i++) {
        memcpy(
            latency_prof_stat_name,
            (htt_stats_buf->latency_prof_name[i]),
            HTT_STATS_MAX_PROF_STATS_NAME_LEN);
        HTT_STATS_PRINT(FATAL,
            "%-32s",
            htt_stats_buf->latency_prof_name[i]);
        HTT_STATS_PRINT(FATAL,
            "|%-25s|%8s|%8s|%8s|%8s|%8s|%10s|%14s|%8s|%8s|%8s|",
            "cal_type","cnt","min","max","last","tot","hist_intvl",
            "hist","pf_last","pf_tot","pf_max");

        for (j = 0; j < htt_stats_buf->CalCnt[i]; j++) {
             HTT_STATS_PRINT(FATAL,
                 "|%-25s|%8u|%8u|%8u|%8u|%8u|%10u|%4u:%4u:%4u|%8u|%8u|%8u|",
                 ((A_UINT8 *)
                     htt_ctrl_path_cal_type_id_to_name(
                         HTT_CTRL_PATH_CALIBRATION_STATS_CAL_TYPE_GET(
                             htt_stats_buf->enabledCalIdx[i][j])) +
                     sizeof("HTT_CTRL_PATH_STATS_CAL_TYPE")),
                 htt_stats_buf->cnt[i][j],
                 htt_stats_buf->min[i][j],
                 htt_stats_buf->max[i][j],
                 htt_stats_buf->last[i][j],
                 htt_stats_buf->tot[i][j],
                 htt_stats_buf->hist_intvl[i][j],
                 htt_stats_buf->hist[i][j][0],
                 htt_stats_buf->hist[i][j][1],
                 htt_stats_buf->hist[i][j][2],
                 htt_stats_buf->pf_last[i][j],
                 htt_stats_buf->pf_tot[i][j],
                 htt_stats_buf->pf_max[i][j]);
         }
    }
}

/*
 * htt_print_latency_prof_cal_data_tlv: display htt_stats_latency_prof_cal_data_tlv tlv
 * @tag_buf: buffer containing the htt_stats_latency_prof_cal_data_tlv TLV
 *
 * return:void
 */
static void htt_print_latency_prof_cal_data_tlv(A_UINT32 *tag_buf)
{
    A_UINT32 i, j;
    htt_stats_latency_prof_cal_data_tlv *htt_stats_buf =
        (htt_stats_latency_prof_cal_data_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_STATS_LATENCY_PROF_CAL_DATA_TLV:");

    for (i = 1; i < HTT_STATS_MAX_PROF_CAL; i++) {
        /* ensure latency_prof_name is null-terminated */
        htt_stats_buf->latency_prof_name[i][HTT_STATS_MAX_PROF_STATS_NAME_LEN-1] = '\0';
        HTT_STATS_PRINT(FATAL,
            "%-32s",
            htt_stats_buf->latency_prof_name[i]);
        HTT_STATS_PRINT(FATAL,
            "|%-25s|%8s|%8s|%8s|%8s|%8s|%10s|%14s|%8s|%8s|%8s|",
            "cal_type","cnt","min","max","last","tot","hist_intvl",
            "hist","pf_last","pf_tot","pf_max");

        for (j = 0; j < htt_stats_buf->cal_cnt[i]; j++) {
             HTT_STATS_PRINT(FATAL,
                 "|%-25s|%8u|%8u|%8u|%8u|%8u|%10u|%4u:%4u:%4u|%8u|%8u|%8u|",
                 ((A_UINT8 *)
                     htt_ctrl_path_cal_type_id_to_name(
                         HTT_CTRL_PATH_CALIBRATION_STATS_CAL_TYPE_GET(
                             htt_stats_buf->latency_data[i][j].enabled_cal_idx)) +
                     sizeof("HTT_CTRL_PATH_STATS_CAL_TYPE")),
                 htt_stats_buf->latency_data[i][j].cnt,
                 htt_stats_buf->latency_data[i][j].min,
                 htt_stats_buf->latency_data[i][j].max,
                 htt_stats_buf->latency_data[i][j].last,
                 htt_stats_buf->latency_data[i][j].tot,
                 htt_stats_buf->latency_data[i][j].hist_intvl,
                 htt_stats_buf->latency_data[i][j].hist[0],
                 htt_stats_buf->latency_data[i][j].hist[1],
                 htt_stats_buf->latency_data[i][j].hist[2],
                 htt_stats_buf->latency_data[i][j].pf_last,
                 htt_stats_buf->latency_data[i][j].pf_tot,
                 htt_stats_buf->latency_data[i][j].pf_max);
         }
    }
}

#if 0 /* temporarily disable the below functions */
static void htt_print_mlo_umac_ssr_mlo_stats_done_mask(
    htt_mlo_umac_ssr_mlo_stats_t *p_mlo_stats)
{
    HTT_STATS_PRINT(FATAL, "done pre_reset_disable_rxdma_prefetch = %u",
        HTT_UMAC_RECOVERY_DONE_PRE_RESET_DISABLE_RXDMA_PREFETCH_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done pre_reset_pmacs_hwmlos= %u",
        HTT_UMAC_RECOVERY_DONE_PRE_RESET_PMACS_HWMLOS_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done pre_reset_global_wsi = %u",
        HTT_UMAC_RECOVERY_DONE_PRE_RESET_GLOBAL_WSI_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done pre_reset_pmacs_dmac = %u",
        HTT_UMAC_RECOVERY_DONE_PRE_RESET_PMACS_DMAC_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done pre_reset_tcl = %u",
        HTT_UMAC_RECOVERY_DONE_PRE_RESET_TCL_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done pre_reset_tqm = %u",
        HTT_UMAC_RECOVERY_DONE_PRE_RESET_TQM_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done pre_reset_wbm = %u",
        HTT_UMAC_RECOVERY_DONE_PRE_RESET_WBM_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done pre_reset_reo = %u",
        HTT_UMAC_RECOVERY_DONE_PRE_RESET_REO_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done pre_reset_host = %u",
        HTT_UMAC_RECOVERY_DONE_PRE_RESET_HOST_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done reset_prerequisites = %u",
        HTT_UMAC_RECOVERY_DONE_RESET_PREREQUISITES_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done reset_pre_ring_reset= %u",
        HTT_UMAC_RECOVERY_DONE_RESET_PRE_RING_RESET_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done reset_apply_soft_reset= %u",
        HTT_UMAC_RECOVERY_DONE_RESET_APPLY_SOFT_RESET_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done reset_post_ring_reset= %u",
        HTT_UMAC_RECOVERY_DONE_RESET_POST_RING_RESET_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done reset_fw_tqm_cmdqs = %u",
        HTT_UMAC_RECOVERY_DONE_RESET_FW_TQM_CMDQS_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done post_reset_host = %u",
        HTT_UMAC_RECOVERY_DONE_POST_RESET_HOST_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done post_reset_umac_interrupts = %u",
        HTT_UMAC_RECOVERY_DONE_POST_RESET_UMAC_INTERRUPTS_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done post_reset_wbm = %u",
        HTT_UMAC_RECOVERY_DONE_POST_RESET_WBM_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done post_reset_reo = %u",
        HTT_UMAC_RECOVERY_DONE_POST_RESET_REO_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done post_reset_tqm = %u",
        HTT_UMAC_RECOVERY_DONE_POST_RESET_TQM_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done post_reset_pmacs_dmac = %u",
        HTT_UMAC_RECOVERY_DONE_POST_RESET_PMACS_DMAC_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done post_reset_tqm_sync_cmd = %u",
        HTT_UMAC_RECOVERY_DONE_POST_RESET_TQM_SYNC_CMD_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done post_reset_global_wsi = %u",
        HTT_UMAC_RECOVERY_DONE_POST_RESET_GLOBAL_WSI_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done post_reset_pmacs_hwmlos = %u",
        HTT_UMAC_RECOVERY_DONE_POST_RESET_PMACS_HWMLOS_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done post_reset_enable_rxdma_prefetch = %u",
        HTT_UMAC_RECOVERY_DONE_POST_RESET_ENABLE_RXDMA_PREFETCH_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done post_reset_tcl = %u",
        HTT_UMAC_RECOVERY_DONE_POST_RESET_TCL_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done post_reset_host_enq = %u",
        HTT_UMAC_RECOVERY_DONE_POST_RESET_HOST_ENQ_GET(
            p_mlo_stats->umac_recovery_done_mask));
    HTT_STATS_PRINT(FATAL, "done post_reset_verify_umac_recovered = %u\n",
        HTT_UMAC_RECOVERY_DONE_POST_RESET_VERIFY_UMAC_RECOVERED_GET(
            p_mlo_stats->umac_recovery_done_mask));
}

static void
htt_print_mlo_umac_ssr_mlo_stats(htt_mlo_umac_ssr_mlo_stats_t *p_mlo_stats)
{
    HTT_STATS_PRINT(FATAL, "HTT_MLO_UMAC_SSR_MLO_STATS:");

    HTT_STATS_PRINT(FATAL, "last_mlo_htt_handshake_delta_ms = %u",
            p_mlo_stats->last_mlo_htt_handshake_delta_ms);
    HTT_STATS_PRINT(FATAL, "max_mlo_htt_handshake_delta_ms = %u",
            p_mlo_stats->max_mlo_htt_handshake_delta_ms);
    htt_print_mlo_umac_ssr_mlo_stats_done_mask(p_mlo_stats);
}

static void
htt_print_mlo_umac_ssr_kpi_stats(htt_mlo_umac_ssr_kpi_stats_t *p_kpi_stats)
{
    HTT_STATS_PRINT(FATAL, "HTT_MLO_UMAC_SSR_KPI_STATS:");

/* TODO:
 * Avoid using the struct definition compiled into the host to determine
 * the size of the kpi_delta elements (and thus the offset of the kpi_tstamp
 * array) and the size of the kpi_tstamp elements.
 * Instead, look at the TLV headers to see how much data has been provided
 * for each of the kpi_delta and kpi_tstamp elements.
 */
#if 0
    htt_print_mlo_umac_ssr_kpi_delta_stats(&(p_kpi_stats->kpi_delta_tlv.kpi_delta));
    htt_print_mlo_umac_ssr_kpi_tstamp_stats(&(p_kpi_stats->kpi_tstamp_tlv.kpi_tstamp));
#endif
}

static void
htt_print_mlo_umac_ssr_cmn_stats(htt_mlo_umac_ssr_common_stats_t *p_cmn_stats)
{
    HTT_STATS_PRINT(FATAL, "HTT_MLO_UMAC_SSR_COMMON_STATS:");

    HTT_STATS_PRINT(FATAL, "last_e2e_delta_ms = %u",
            p_cmn_stats->last_e2e_delta_ms);
    HTT_STATS_PRINT(FATAL, "max_e2e_delta_ms = %u",
            p_cmn_stats->max_e2e_delta_ms);
    HTT_STATS_PRINT(FATAL, "per_handshake_max_allowed_delta_ms = %u",
            p_cmn_stats->per_handshake_max_allowed_delta_ms);
    HTT_STATS_PRINT(FATAL, "total_success_runs_cnt = %u",
            p_cmn_stats->total_success_runs_cnt);
    HTT_STATS_PRINT(FATAL, "umac_recovery_in_progress = %u",
            p_cmn_stats->umac_recovery_in_progress);
    HTT_STATS_PRINT(FATAL, "umac_disengaged_count = %u",
            p_cmn_stats->umac_disengaged_count);
    HTT_STATS_PRINT(FATAL, "umac_soft_reset_count = %u",
            p_cmn_stats->umac_soft_reset_count);
    HTT_STATS_PRINT(FATAL, "umac_engaged_count = %u\n",
            p_cmn_stats->umac_engaged_count);
}

static void htt_print_mlo_umac_ssr_trigger_stats(
    htt_mlo_umac_ssr_trigger_stats_t *p_trigger_stats)
{
    HTT_STATS_PRINT(FATAL, "HTT_MLO_UMAC_SSR_TRIGGER_STATS:");

    HTT_STATS_PRINT(FATAL, "trigger_requests_count = %u",
        p_trigger_stats->trigger_requests_count);
    HTT_STATS_PRINT(FATAL, "trigger_count_for_umac_hang = %u",
        p_trigger_stats->trigger_count_for_umac_hang);
    HTT_STATS_PRINT(FATAL, "trigger_count_for_mlo_target_recovery_mode1 = %u",
        p_trigger_stats->trigger_count_for_mlo_target_recovery_mode1);
    HTT_STATS_PRINT(FATAL, "trigger_count_for_unknown_signature = %u",
        p_trigger_stats->trigger_count_for_unknown_signature);
    HTT_STATS_PRINT(FATAL, "total_trig_dropped = %u\n",
        p_trigger_stats->total_trig_dropped);

    HTT_STATS_PRINT(FATAL, "HTT_MLO_UMAC_SSR_TRIGGER_UMAC_HANG_STATS:");

    HTT_STATS_PRINT(FATAL, "trigger_count_for_unit_test_direct_trigger = %u",
        p_trigger_stats->trigger_count_for_unit_test_direct_trigger);
    HTT_STATS_PRINT(FATAL, "trigger_count_for_tx_de_wdg_dummy_frame_tout = %u",
        p_trigger_stats->trigger_count_for_tx_de_wdg_dummy_frame_tout);
    HTT_STATS_PRINT(FATAL,
        "trigger_count_for_peer_delete_wdg_dummy_frame_tout = %u",
        p_trigger_stats->trigger_count_for_peer_delete_wdg_dummy_frame_tout);
    HTT_STATS_PRINT(FATAL, "trigger_count_for_reo_hang = %u",
        p_trigger_stats->trigger_count_for_reo_hang);
    HTT_STATS_PRINT(FATAL, "trigger_count_for_tqm_hang = %u",
        p_trigger_stats->trigger_count_for_tqm_hang);
    HTT_STATS_PRINT(FATAL, "trigger_count_for_tcl_hang = %u",
        p_trigger_stats->trigger_count_for_tcl_hang);
    HTT_STATS_PRINT(FATAL, "trigger_count_for_wbm_hang = %u\n",
        p_trigger_stats->trigger_count_for_wbm_hang);
}
#endif /* temporarily disable the above functions */

static void htt_print_umac_ssr_stats_tlv(A_UINT32 *tag_buf)
{
    htt_umac_ssr_stats_tlv *htt_stats_buf =
        (htt_umac_ssr_stats_tlv *)tag_buf;
    HTT_STATS_PRINT(FATAL, "HTT_UMAC_SSR_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "total_done = %u",
            htt_stats_buf->stats.total_done);
    HTT_STATS_PRINT(FATAL, "trigger_requests_count = %u",
            htt_stats_buf->stats.trigger_requests_count);
    HTT_STATS_PRINT(FATAL, "total_trig_dropped = %u",
            htt_stats_buf->stats.total_trig_dropped);
    HTT_STATS_PRINT(FATAL, "umac_disengaged_count = %u",
            htt_stats_buf->stats.umac_disengaged_count);
    HTT_STATS_PRINT(FATAL, "umac_soft_reset_count = %u",
            htt_stats_buf->stats.umac_soft_reset_count);
    HTT_STATS_PRINT(FATAL, "umac_engaged_count = %u",
            htt_stats_buf->stats.umac_engaged_count);
    HTT_STATS_PRINT(FATAL, "last_trigger_request_ms = %u",
            htt_stats_buf->stats.last_trigger_request_ms);
    HTT_STATS_PRINT(FATAL, "last_start_ms = %u",
            htt_stats_buf->stats.last_start_ms);
    HTT_STATS_PRINT(FATAL, "last_start_disengage_umac_ms = %u",
            htt_stats_buf->stats.last_start_disengage_umac_ms);
    HTT_STATS_PRINT(FATAL, "last_enter_ssr_platform_thread_ms = %u",
            htt_stats_buf->stats.last_enter_ssr_platform_thread_ms);
    HTT_STATS_PRINT(FATAL, "last_exit_ssr_platform_thread_ms = %u",
            htt_stats_buf->stats.last_exit_ssr_platform_thread_ms);
    HTT_STATS_PRINT(FATAL, "last_start_engage_umac_ms = %u",
            htt_stats_buf->stats.last_start_engage_umac_ms);
    HTT_STATS_PRINT(FATAL, "post_reset_tqm_sync_cmd_completion_ms = %u",
            htt_stats_buf->stats.post_reset_tqm_sync_cmd_completion_ms);
    HTT_STATS_PRINT(FATAL, "last_done_successful_ms = %u",
            htt_stats_buf->stats.last_done_successful_ms);
    HTT_STATS_PRINT(FATAL, "last_e2e_delta_ms = %u",
            htt_stats_buf->stats.last_e2e_delta_ms);
    HTT_STATS_PRINT(FATAL, "max_e2e_delta_ms = %u",
            htt_stats_buf->stats.max_e2e_delta_ms);
    HTT_STATS_PRINT(FATAL, "trigger_count_for_umac_hang = %u",
            htt_stats_buf->stats.trigger_count_for_umac_hang);
    HTT_STATS_PRINT(FATAL, "trigger_count_for_mlo_quick_ssr = %u",
            htt_stats_buf->stats.trigger_count_for_mlo_quick_ssr);
    HTT_STATS_PRINT(FATAL, "trigger_count_for_unknown_signature = %u",
            htt_stats_buf->stats.trigger_count_for_unknown_signature);
    HTT_STATS_PRINT(FATAL, "htt_sync_mlo_initiate_umac_recovery_ms = %u",
            htt_stats_buf->stats.htt_sync_mlo_initiate_umac_recovery_ms);
    HTT_STATS_PRINT(FATAL, "htt_sync_do_pre_reset_ms = %u",
            htt_stats_buf->stats.htt_sync_do_pre_reset_ms);
    HTT_STATS_PRINT(FATAL, "htt_sync_do_post_reset_start_ms = %u",
            htt_stats_buf->stats.htt_sync_do_post_reset_start_ms);
    HTT_STATS_PRINT(FATAL, "htt_sync_do_post_reset_complete_ms = %u\n",
            htt_stats_buf->stats.htt_sync_do_post_reset_complete_ms);
}

static void htt_print_pdev_tdma_stats_tlv(A_UINT32 *tag_buf)
{
    htt_pdev_tdma_stats_tlv *htt_stats_buf =
        (htt_pdev_tdma_stats_tlv *)tag_buf;
    HTT_STATS_PRINT(FATAL, "HTT_PDEV_TDMA_STATS_TLV:");
    HTT_STATS_PRINT(FATAL, "mac_id = %u",
        HTT_STATS_TDMA_MAC_ID_GET(htt_stats_buf->mac_id__word));
    HTT_STATS_PRINT(FATAL, "num_tdma_active_schedules = %u",
            htt_stats_buf->num_tdma_active_schedules);
    HTT_STATS_PRINT(FATAL, "num_tdma_reserved_schedules = %u",
            htt_stats_buf->num_tdma_reserved_schedules);
    HTT_STATS_PRINT(FATAL, "num_tdma_restricted_schedules = %u",
            htt_stats_buf->num_tdma_restricted_schedules);
    HTT_STATS_PRINT(FATAL, "num_tdma_unconfigured_schedules = %u",
            htt_stats_buf->num_tdma_unconfigured_schedules);
    HTT_STATS_PRINT(FATAL, "num_tdma_slot_switches = %u",
            htt_stats_buf->num_tdma_slot_switches);
    HTT_STATS_PRINT(FATAL, "num_tdma_edca_switches = %u",
            htt_stats_buf->num_tdma_edca_switches);
}

static void htt_print_codel_svc_class_stats_tlv(A_UINT32 *tag_buf)
{
    htt_codel_svc_class_stats_tlv *htt_stats_buf =
        (htt_codel_svc_class_stats_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_STATS_CODEL_SVC_CLASS_TAG:");
    HTT_STATS_PRINT(FATAL, "svc class %u",
        htt_stats_buf->svc_class_id);
    HTT_STATS_PRINT(FATAL, "%u drops, %u no-drops\n",
        htt_stats_buf->codel_drops, htt_stats_buf->codel_no_drops);
}

static void htt_print_codel_msduq_stats_tlv(A_UINT32 *tag_buf)
{
    htt_codel_msduq_stats_tlv *htt_stats_buf =
        (htt_codel_msduq_stats_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_STATS_CODEL_MSDUQ_TAG:");
    HTT_STATS_PRINT(FATAL, "MSDU queue tx flow %u, svc class %u",
        HTT_CODEL_MSDUQ_STATS_TX_FLOW_NUM_GET(htt_stats_buf->id__word),
        HTT_CODEL_MSDUQ_STATS_SVC_CLASS_ID_GET(htt_stats_buf->id__word));
    HTT_STATS_PRINT(FATAL, "%u drops, %u no-drops\n",
        HTT_CODEL_MSDUQ_STATS_DROPS_GET(htt_stats_buf->stats__word),
        HTT_CODEL_MSDUQ_STATS_NO_DROPS_GET(htt_stats_buf->stats__word));
}

/*
 * htt_print_mlo_sched_stats_tlv: display mlo_sched_stats_tlv values
 * @tag_buf: buffer containing the tlv mlo_sched_stats_tlv
 *
 * return:void
 */
static void htt_print_mlo_sched_stats_tlv(A_UINT32 *tag_buf)
{
    htt_mlo_sched_stats_tlv *htt_stats_buf =
        (htt_mlo_sched_stats_tlv *)tag_buf;
    HTT_STATS_PRINT(FATAL, "HTT_STATS_MLO_SCHED_STATS:");
    HTT_STATS_PRINT(FATAL, "====================");
    HTT_STATS_PRINT(FATAL, "num_sec_link_sched: %u",
        htt_stats_buf->pref_link_num_sec_link_sched);
    HTT_STATS_PRINT(FATAL, "num_pref_link_timeout: %u",
        htt_stats_buf->pref_link_num_pref_link_timeout);
    HTT_STATS_PRINT(FATAL, "num_pref_link_sch_delay_ipc: %u",
        htt_stats_buf->pref_link_num_pref_link_sch_delay_ipc);
    HTT_STATS_PRINT(FATAL, "num_pref_link_timeout_ipc: %u",
        htt_stats_buf->pref_link_num_pref_link_timeout_ipc);
    HTT_STATS_PRINT(FATAL, "===========================================\n");
}

static void htt_print_mlo_ipc_stats_tlv(A_UINT32 *tag_buf)
{
    A_UINT8 i = 0, j = 0;
    htt_pdev_mlo_ipc_stats_tlv *htt_stats_buf =
        (htt_pdev_mlo_ipc_stats_tlv *)tag_buf;
    HTT_STATS_PRINT(FATAL, "HTT_STATS_MLO_IPC_STATS:");
    HTT_STATS_PRINT(FATAL, "====================");

    for (i = 0; i < HTT_STATS_HWMLO_MAX_LINKS; i++) {
        HTT_STATS_PRINT(FATAL, "src_link: %u", i);
        for (j = 0; j < HTT_STATS_MLO_MAX_IPC_RINGS; j++) {
            HTT_STATS_PRINT(FATAL, "mlo_ipc_ring_full_cnt[%u]: %u",
                j, htt_stats_buf->mlo_ipc_ring_full_cnt[i][j]);
        }
        HTT_STATS_PRINT(FATAL, "===========================================\n");
    }
}

/*
 * htt_print_gtx_stats_tlv: display gtx_stats_tlv values
 * @tag_buf: buffer containing the tlv gtx_stats_tlv
 *
 * return:void
 */
static void htt_print_gtx_stats_tlv(A_UINT32 *tag_buf)
{
    htt_stats_gtx_tlv *htt_stats_buf = (htt_stats_gtx_tlv *) tag_buf;
    char str_buf[HTT_MAX_STRING_LEN];

    HTT_STATS_PRINT(FATAL, "HTT_STATS_GTX_STATS:");
    HTT_STATS_PRINT(FATAL, "====================");
    HTT_STATS_PRINT(FATAL, "Green TX Enabled: %u",
        htt_stats_buf->gtx_enabled);

    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    snprintf(str_buf, HTT_MAX_STRING_LEN,
        " 0:%u 1:%u 2:%u 3:%u 4:%u 5:%u 6:%u 7:%u"
        " 8:%u 9:%u 10:%u 11:%u 12:%u 13:%u 14:%u 15:%u",
        htt_stats_buf->mcs_tpc_min[0],
        htt_stats_buf->mcs_tpc_min[1],
        htt_stats_buf->mcs_tpc_min[2],
        htt_stats_buf->mcs_tpc_min[3],
        htt_stats_buf->mcs_tpc_min[4],
        htt_stats_buf->mcs_tpc_min[5],
        htt_stats_buf->mcs_tpc_min[6],
        htt_stats_buf->mcs_tpc_min[7],
        htt_stats_buf->mcs_tpc_min[8],
        htt_stats_buf->mcs_tpc_min[9],
        htt_stats_buf->mcs_tpc_min[10],
        htt_stats_buf->mcs_tpc_min[11],
        htt_stats_buf->mcs_tpc_min[12],
        htt_stats_buf->mcs_tpc_min[13],
        htt_stats_buf->mcs_tpc_min[14],
        htt_stats_buf->mcs_tpc_min[15]);
    HTT_STATS_PRINT(FATAL, "MIN TPC (0.25 dBm) = %s", str_buf);

    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    snprintf(str_buf, HTT_MAX_STRING_LEN,
        " 0:%u 1:%u 2:%u 3:%u 4:%u 5:%u 6:%u 7:%u"
        " 8:%u 9:%u 10:%u 11:%u 12:%u 13:%u 14:%u 15:%u",
        htt_stats_buf->mcs_tpc_max[0],
        htt_stats_buf->mcs_tpc_max[1],
        htt_stats_buf->mcs_tpc_max[2],
        htt_stats_buf->mcs_tpc_max[3],
        htt_stats_buf->mcs_tpc_max[4],
        htt_stats_buf->mcs_tpc_max[5],
        htt_stats_buf->mcs_tpc_max[6],
        htt_stats_buf->mcs_tpc_max[7],
        htt_stats_buf->mcs_tpc_max[8],
        htt_stats_buf->mcs_tpc_max[9],
        htt_stats_buf->mcs_tpc_max[10],
        htt_stats_buf->mcs_tpc_max[11],
        htt_stats_buf->mcs_tpc_max[12],
        htt_stats_buf->mcs_tpc_max[13],
        htt_stats_buf->mcs_tpc_max[14],
        htt_stats_buf->mcs_tpc_max[15]);
    HTT_STATS_PRINT(FATAL, "MAX TPC (0.25 dBm) = %s", str_buf);

    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    snprintf(str_buf, HTT_MAX_STRING_LEN,
        " 0:%u 1:%u 2:%u 3:%u 4:%u 5:%u 6:%u 7:%u"
        " 8:%u 9:%u 10:%u 11:%u 12:%u 13:%u 14:%u 15:%u",
        htt_stats_buf->mcs_tpc_diff[0],
        htt_stats_buf->mcs_tpc_diff[1],
        htt_stats_buf->mcs_tpc_diff[2],
        htt_stats_buf->mcs_tpc_diff[3],
        htt_stats_buf->mcs_tpc_diff[4],
        htt_stats_buf->mcs_tpc_diff[5],
        htt_stats_buf->mcs_tpc_diff[6],
        htt_stats_buf->mcs_tpc_diff[7],
        htt_stats_buf->mcs_tpc_diff[8],
        htt_stats_buf->mcs_tpc_diff[9],
        htt_stats_buf->mcs_tpc_diff[10],
        htt_stats_buf->mcs_tpc_diff[11],
        htt_stats_buf->mcs_tpc_diff[12],
        htt_stats_buf->mcs_tpc_diff[13],
        htt_stats_buf->mcs_tpc_diff[14],
        htt_stats_buf->mcs_tpc_diff[15]);
    HTT_STATS_PRINT(FATAL, "TPC DIFF MCS (0.25 dB) = %s", str_buf);
    HTT_STATS_PRINT(FATAL, "===========================================\n");
}

/*
 * htt_print_wifi_radar_stats_tlv:
 * display htt_stats_tx_pdev_wifi_radar_tlv values
 * @tag_buf: buffer containing the htt_stats_tx_pdev_wifi_radar_tlv object
 *
 * return:void
 */
static void htt_print_wifi_radar_stats_tlv(A_UINT32 *tag_buf)
{
    int i,j;
    A_UINT16 index_cts2self                         = 0;
    A_UINT16 index_wifi_radar                       = 0;
    A_UINT16 index_rx_gain                          = 0;
    A_UINT16 index_cal_type_count                   = 0;
    A_UINT16 index_cal_fail_reason_count            = 0;
    A_CHAR   str_buf[HTT_MAX_STRING_LEN];

    htt_stats_tx_pdev_wifi_radar_tlv *htt_stats_buf =
        (htt_stats_tx_pdev_wifi_radar_tlv *) tag_buf;

    HTT_STATS_PRINT(FATAL, "HTT_STATS_TX_PDEV_WIFI_RADAR_TAG:\n");
    HTT_STATS_PRINT(FATAL, "===========================================\n");
    HTT_STATS_PRINT(FATAL, "capture_in_progress = %u",
        htt_stats_buf->capture_in_progress);
    HTT_STATS_PRINT(FATAL, "calibration_in_progress = %u",
        htt_stats_buf->calibration_in_progress);
    HTT_STATS_PRINT(FATAL, "periodicity (ms) = %u",
        htt_stats_buf->periodicity);
    HTT_STATS_PRINT(FATAL, "latest_req_timestamp (ms) = %u",
        htt_stats_buf->latest_req_timestamp);
    HTT_STATS_PRINT(FATAL, "latest_resp_timestamp (ms) = %u",
        htt_stats_buf->latest_resp_timestamp);
    HTT_STATS_PRINT(FATAL, "Full_calibration_timing (ms) = %u",
        htt_stats_buf->latest_calibration_timing);
    HTT_STATS_PRINT(FATAL, "wifi_radar_req_count = %u",
        htt_stats_buf->wifi_radar_req_count);
    HTT_STATS_PRINT(FATAL, "num_wifi_radar_pkt_success = %u",
        htt_stats_buf->num_wifi_radar_pkt_success);
    HTT_STATS_PRINT(FATAL, "num_wifi_radar_pkt_queued = %u",
        htt_stats_buf->num_wifi_radar_pkt_queued);
    HTT_STATS_PRINT(FATAL, "num_wifi_radar_cal_pkt_success = %u",
        htt_stats_buf->num_wifi_radar_cal_pkt_success);
    HTT_STATS_PRINT(FATAL, "latest_wifi_radar_cal_type = %u",
        htt_stats_buf->latest_wifi_radar_cal_type);
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_NUM_WIFI_RADAR_CAL_TYPES; i++) {
        index_cal_type_count += snprintf(
            &str_buf[index_cal_type_count],
            HTT_MAX_STRING_LEN - index_cal_type_count,
            " %u:%u,", i,
            htt_stats_buf->wifi_radar_cal_type_counts[i]);
    }
    HTT_STATS_PRINT(FATAL, "calibration_type_count = %s", str_buf);
    HTT_STATS_PRINT(FATAL, "latest_wifi_radar_cal_fail_reason = %u",
        htt_stats_buf->latest_wifi_radar_cal_fail_reason);
    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_NUM_WIFI_RADAR_CAL_FAILURE_REASONS; i++) {
        index_cal_fail_reason_count += snprintf(
            &str_buf[index_cal_fail_reason_count],
            HTT_MAX_STRING_LEN - index_cal_fail_reason_count,
            " %u:%u,", i,
            htt_stats_buf->wifi_radar_cal_fail_reason_counts[i]);
    }
    HTT_STATS_PRINT(FATAL, "wifi_radar_cal_fail_reason_count = %s", str_buf);
    HTT_STATS_PRINT(FATAL, "wifi_radar_licensed = %u",
        htt_stats_buf->wifi_radar_licensed);
    HTT_STATS_PRINT(FATAL, "wifi_radar_cal_init_tx_gain = %u",
        htt_stats_buf->wifi_radar_cal_init_tx_gain);

    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_MAX_SCH_CMD_RESULT; i++) {
        index_cts2self += snprintf(
            &str_buf[index_cts2self],
            HTT_MAX_STRING_LEN - index_cts2self,
            " %u:%u,", i,
            htt_stats_buf->cmd_results_cts2self[i]);
    }
    HTT_STATS_PRINT(FATAL, "cmd_result_cts2Self = %s", str_buf);

    memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
    for (i = 0; i < HTT_STATS_MAX_SCH_CMD_RESULT; i++) {
        index_wifi_radar += snprintf(
            &str_buf[index_wifi_radar],
            HTT_MAX_STRING_LEN - index_wifi_radar,
            " %u:%u,", i,
            htt_stats_buf->cmd_results_wifi_radar[i]);
    }
    HTT_STATS_PRINT(FATAL, "cmd_result_wifi_radar = %s", str_buf);
    for (i = 0; i < HTT_STATS_MAX_CHAINS; i++) {
        HTT_STATS_PRINT(FATAL, "Chain Pair %u Stats", i);
        HTT_STATS_PRINT(FATAL, "wifi_radar_tx_gain = %u",
            htt_stats_buf->wifi_radar_tx_gains[i]);
        HTT_STATS_PRINT(FATAL, "calibration_timing = %u",
            htt_stats_buf->calibration_timing_per_chain[i]);
        index_rx_gain = 0;
        memset(str_buf, 0x0, HTT_MAX_STRING_LEN);
        for (j = 0; j < HTT_STATS_MAX_CHAINS; j++) {
            index_rx_gain += snprintf(
                &str_buf[index_rx_gain],
                HTT_MAX_STRING_LEN - index_rx_gain,
                " %u:%u,", j,
                htt_stats_buf->wifi_radar_rx_gains[i][j]);
        }
        HTT_STATS_PRINT(FATAL, "Rx gain for chain index = %s", str_buf);
    }
    HTT_STATS_PRINT(FATAL, "===========================================\n");
}

/*
 * htt_htt_stats_print_tag: function to select the tag type and
 * print the corresponding tag structure
 * @tag_type: tag type that is to be printed
 * @tag_buf: pointer to the tag structure
 *
 * return: void
 */
void htt_htt_stats_print_tag(
        A_UINT8 tag_type,
        A_UINT32 *tag_buf)
{
    // htt_htt_stats_debug_dump(tag_buf);
    switch (tag_type) {
    case HTT_STATS_TX_PDEV_CMN_TAG:
        htt_print_tx_pdev_stats_cmn_tlv(tag_buf);
        break;
    case HTT_STATS_TX_PDEV_UNDERRUN_TAG:
        htt_print_tx_pdev_stats_urrn_tlv_v(tag_buf);
        break;
    case HTT_STATS_TX_PDEV_SIFS_TAG:
        htt_print_tx_pdev_stats_sifs_tlv_v(tag_buf);
        break;
    case HTT_STATS_TX_PDEV_FLUSH_TAG:
        htt_print_tx_pdev_stats_flush_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_PHY_ERR_TAG:
        htt_print_tx_pdev_stats_phy_err_tlv_v(tag_buf);
        break;
    case HTT_STATS_TX_PDEV_SIFS_HIST_TAG:
        htt_print_tx_pdev_stats_sifs_hist_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_TX_PPDU_STATS_TAG:
        htt_print_tx_pdev_stats_tx_ppdu_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_MU_PPDU_DIST_TAG:
        htt_print_tx_pdev_mu_ppdu_dist_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_TRIED_MPDU_CNT_HIST_TAG:
        htt_print_tx_pdev_stats_tried_mpdu_cnt_hist_tlv_v(tag_buf);
        break;

    case HTT_STATS_STRING_TAG:
        htt_print_stats_string_tlv(tag_buf);
        break;

    case HTT_STATS_TX_HWQ_CMN_TAG:
        htt_print_tx_hwq_stats_cmn_tlv(tag_buf);
        break;

    case HTT_STATS_TX_HWQ_DIFS_LATENCY_TAG:
        htt_print_tx_hwq_difs_latency_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_HWQ_CMD_RESULT_TAG:
        htt_print_tx_hwq_cmd_result_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_HWQ_CMD_STALL_TAG:
        htt_print_tx_hwq_cmd_stall_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_HWQ_FES_STATUS_TAG:
        htt_print_tx_hwq_fes_result_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_HWQ_TRIED_MPDU_CNT_HIST_TAG:
        htt_print_tx_hwq_tried_mpdu_cnt_hist_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_HWQ_TXOP_USED_CNT_HIST_TAG:
        htt_print_tx_hwq_txop_used_cnt_hist_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_TQM_GEN_MPDU_TAG:
        htt_print_tx_tqm_gen_mpdu_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_TQM_LIST_MPDU_TAG:
        htt_print_tx_tqm_list_mpdu_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_TQM_LIST_MPDU_CNT_TAG:
        htt_print_tx_tqm_list_mpdu_cnt_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_TQM_CMN_TAG:
        htt_print_tx_tqm_cmn_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_TQM_PDEV_TAG:
        htt_print_tx_tqm_pdev_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_TQM_CMDQ_STATUS_TAG:
        htt_print_tx_tqm_cmdq_status_tlv(tag_buf);
        break;

    case HTT_STATS_TX_DE_EAPOL_PACKETS_TAG:
        htt_print_tx_de_eapol_packets_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_DE_CLASSIFY_FAILED_TAG:
        htt_print_tx_de_classify_failed_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_DE_CLASSIFY_STATS_TAG:
        htt_print_tx_de_classify_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_DE_CLASSIFY_STATUS_TAG:
        htt_print_tx_de_classify_status_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_DE_ENQUEUE_PACKETS_TAG:
        htt_print_tx_de_enqueue_packets_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_DE_ENQUEUE_DISCARD_TAG:
        htt_print_tx_de_enqueue_discard_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_DE_FW2WBM_RING_FULL_HIST_TAG:
        htt_print_tx_de_fw2wbm_ring_full_hist_tlv(tag_buf);
        break;

    case HTT_STATS_TX_DE_CMN_TAG:
        htt_print_tx_de_cmn_stats_tlv(tag_buf);
        break;

    case HTT_STATS_RING_IF_TAG:
        htt_print_ring_if_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_MU_MIMO_STATS_TAG:
        htt_print_tx_pdev_mu_mimo_sch_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_MUMIMO_GRP_STATS_TAG:
        htt_print_tx_pdev_mumimo_grp_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_DL_MU_OFDMA_STATS_TAG:
        htt_print_tx_pdev_dl_mu_ofdma_sch_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_UL_MU_OFDMA_STATS_TAG:
        htt_print_tx_pdev_ul_mu_ofdma_sch_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_DL_MU_MIMO_STATS_TAG:
        htt_print_tx_pdev_dl_mu_mimo_sch_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_UL_MU_MIMO_STATS_TAG:
        htt_print_tx_pdev_ul_mu_mimo_sch_stats_tlv(tag_buf);
        break;

    case HTT_STATS_SFM_CMN_TAG:
        htt_print_sfm_cmn_tlv(tag_buf);
        break;

    case HTT_STATS_SRING_STATS_TAG:
        htt_print_sring_stats_tlv(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_FW_STATS_TAG:
        htt_print_rx_pdev_fw_stats_tlv(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_FW_RING_MPDU_ERR_TAG:
        htt_print_rx_pdev_fw_ring_mpdu_err_tlv_v(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_FW_MPDU_DROP_TAG:
        htt_print_rx_pdev_fw_mpdu_drop_tlv_v(tag_buf);
        break;

    case HTT_STATS_RX_SOC_FW_STATS_TAG:
        htt_print_rx_soc_fw_stats_tlv(tag_buf);
        break;

    case HTT_STATS_RX_SOC_FW_REFILL_RING_EMPTY_TAG:
        htt_print_rx_soc_fw_refill_ring_empty_tlv_v(tag_buf);
        break;

    case HTT_STATS_RX_SOC_FW_REFILL_RING_NUM_REFILL_TAG:
        htt_print_rx_soc_fw_refill_ring_num_refill_tlv_v(
                tag_buf);
        break;
    case HTT_STATS_RX_REFILL_RXDMA_ERR_TAG:
        htt_print_rx_soc_fw_refill_ring_num_rxdma_err_tlv_v(
                tag_buf);
        break;

    case HTT_STATS_RX_REFILL_REO_ERR_TAG:
        htt_print_rx_soc_fw_refill_ring_num_reo_err_tlv_v(
                tag_buf);
        break;

    case HTT_STATS_RX_REO_RESOURCE_STATS_TAG:
        htt_print_rx_reo_debug_stats_tlv_v(
                tag_buf);
        break;
    case HTT_STATS_RX_PDEV_FW_STATS_PHY_ERR_TAG:
        htt_print_rx_pdev_fw_stats_phy_err_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_RATE_STATS_TAG:
        htt_print_tx_pdev_rate_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_BE_RATE_STATS_TAG:
        htt_print_tx_pdev_be_rate_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_SAWF_RATE_STATS_TAG:
        htt_print_tx_pdev_sawf_rate_stats_tlv(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_RATE_STATS_TAG:
        htt_print_rx_pdev_rate_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_SCHEDULER_TXQ_STATS_TAG:
        htt_print_tx_pdev_stats_sched_per_txq_tlv(tag_buf);
        break;

    case HTT_STATS_TX_SCHED_CMN_TAG:
        htt_print_stats_tx_sched_cmn_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_MPDU_STATS_TAG:
        htt_print_tx_pdev_mu_mimo_mpdu_stats_tlv(tag_buf);
        break;

    case HTT_STATS_SCHED_TXQ_CMD_POSTED_TAG:
        htt_print_sched_txq_cmd_posted_tlv_v(tag_buf);
        break;

    case HTT_STATS_RING_IF_CMN_TAG:
        htt_print_ring_if_cmn_tlv(tag_buf);
        break;

    case HTT_STATS_SFM_CLIENT_USER_TAG:
        htt_print_sfm_client_user_tlv_v(tag_buf);
        break;

    case HTT_STATS_SFM_CLIENT_TAG:
        htt_print_sfm_client_tlv(tag_buf);
        break;

    case HTT_STATS_TX_TQM_ERROR_STATS_TAG:
        htt_print_tx_tqm_error_stats_tlv(tag_buf);
        break;

    case HTT_STATS_SCHED_TXQ_CMD_REAPED_TAG:
        htt_print_sched_txq_cmd_reaped_tlv_v(tag_buf);
        break;

    case HTT_STATS_SRING_CMN_TAG:
        htt_print_sring_cmn_tlv(tag_buf);
        break;

    case HTT_STATS_TX_SOUNDING_STATS_TAG:
        htt_print_tx_sounding_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_SELFGEN_AC_ERR_STATS_TAG:
        htt_print_tx_selfgen_ac_err_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_SELFGEN_CMN_STATS_TAG:
        htt_print_tx_selfgen_cmn_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_SELFGEN_AC_STATS_TAG:
        htt_print_tx_selfgen_ac_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_SELFGEN_AX_STATS_TAG:
        htt_print_tx_selfgen_ax_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_SELFGEN_AX_ERR_STATS_TAG:
        htt_print_tx_selfgen_ax_err_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_HWQ_MUMIMO_SCH_STATS_TAG:
        htt_print_tx_hwq_mu_mimo_sch_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_HWQ_MUMIMO_MPDU_STATS_TAG:
        htt_print_tx_hwq_mu_mimo_mpdu_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_HWQ_MUMIMO_CMN_STATS_TAG:
        htt_print_tx_hwq_mu_mimo_cmn_stats_tlv(tag_buf);
        break;

    case HTT_STATS_HW_INTR_MISC_TAG:
        htt_print_hw_stats_intr_misc_tlv(tag_buf);
        break;

    case HTT_STATS_HW_WD_TIMEOUT_TAG:
        htt_print_hw_stats_wd_timeout_tlv(tag_buf);
        break;

    case HTT_STATS_HW_PDEV_ERRS_TAG:
        htt_print_hw_stats_pdev_errs_tlv(tag_buf);
        break;

    case HTT_STATS_COUNTER_NAME_TAG:
        htt_print_counter_tlv(tag_buf);
        break;

    case HTT_STATS_TX_TID_DETAILS_TAG:
        htt_print_tx_tid_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_TID_DETAILS_V1_TAG:
        htt_print_tx_tid_stats_v1_tlv(tag_buf);
        break;

    case HTT_STATS_RX_TID_DETAILS_TAG:
        htt_print_rx_tid_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PEER_STATS_CMN_TAG:
        htt_print_peer_stats_cmn_tlv(tag_buf);
        break;

    case HTT_STATS_PEER_DETAILS_TAG:
        htt_print_peer_details_tlv(tag_buf);
        break;

    case HTT_STATS_PEER_MSDU_FLOWQ_TAG:
        htt_print_msdu_flow_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PEER_TX_RATE_STATS_TAG:
        htt_print_tx_peer_rate_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PEER_RX_RATE_STATS_TAG:
        htt_print_rx_peer_rate_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_DE_COMPL_STATS_TAG:
        htt_print_tx_de_compl_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_CCA_1SEC_HIST_TAG:
    case HTT_STATS_PDEV_CCA_100MSEC_HIST_TAG:
    case HTT_STATS_PDEV_CCA_STAT_CUMULATIVE_TAG:
        htt_print_pdev_cca_stats_hist_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_CCA_COUNTERS_TAG:
        htt_print_pdev_stats_cca_counters_tlv(tag_buf);
        break;

    case HTT_STATS_WHAL_TX_TAG:
        htt_print_hw_stats_whal_tx_tlv(tag_buf);
        break;

    case HTT_STATS_WHAL_WSI_TAG:
        htt_print_stats_whal_wsib_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_TWT_SESSIONS_TAG:
        htt_print_pdev_stats_twt_sessions_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_TWT_SESSION_TAG:
        htt_print_pdev_stats_twt_session_tlv(tag_buf);
        break;

    case HTT_STATS_SCHED_TXQ_SCHED_ORDER_SU_TAG:
        htt_print_sched_txq_sched_order_su_tlv_v(tag_buf);
        break;

    case HTT_STATS_SCHED_TXQ_SCHED_INELIGIBILITY_TAG:
        htt_print_sched_txq_sched_ineligibility_tlv_v(tag_buf);
        break;

    case HTT_STATS_PDEV_OBSS_PD_TAG:
        htt_print_pdev_obss_pd_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_HW_WAR_TAG:
        htt_print_hw_war_tlv_v(tag_buf);
        break;

    case HTT_STATS_RING_BACKPRESSURE_STATS_TAG:
        htt_print_pdev_ring_backpressure_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_LATENCY_PROF_STATS_TAG:
        htt_print_soc_latency_prof_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_LATENCY_CTX_TAG:
        htt_print_soc_latency_prof_ctx_tlv(tag_buf);
        break;

    case HTT_STATS_LATENCY_CNT_TAG:
        htt_print_latency_prof_cnt(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_UL_TRIG_STATS_TAG:
        htt_print_ul_ofdma_trigger_stats(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_UL_OFDMA_USER_STATS_TAG:
        htt_print_ul_ofdma_user_stats(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_BE_UL_OFDMA_USER_STATS_TAG:
        htt_print_be_ul_ofdma_user_stats(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_UL_MIMO_USER_STATS_TAG:
        htt_print_ul_mimo_user_stats(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_UL_MUMIMO_TRIG_STATS_TAG:
        htt_print_ul_mumimo_trig_stats(tag_buf);
        break;

    case HTT_STATS_RX_FSE_STATS_TAG:
        htt_print_rx_fse_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PEER_SCHED_STATS_TAG:
        htt_print_peer_sched_stats(tag_buf);
        break;

    case HTT_STATS_SCHED_TXQ_SUPERCYCLE_TRIGGER_TAG:
        htt_print_sched_txq_supercycle_trigger_tlv_v(tag_buf);
        break;

    case HTT_STATS_PDEV_CTRL_PATH_TX_STATS_TAG:
        htt_print_pdev_ctrl_path_tx_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PEER_CTRL_PATH_TXRX_STATS_TAG:
        htt_print_peer_ctrl_path_txrx_stats_tlv(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_RATE_EXT_STATS_TAG:
        htt_print_rx_pdev_rate_ext_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_TX_RATE_TXBF_STATS_TAG:
        htt_print_tx_peer_rate_txbf_stats_tlv(tag_buf);
        break;

    case  HTT_STATS_PER_RATE_STATS_TAG:
        htt_print_tx_per_rate_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_NDPA_STATS_TAG: /* DEPRECATED */
        htt_print_txbf_ofdma_ndpa_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_NDP_STATS_TAG: /* DEPRECATED */
        htt_print_txbf_ofdma_ndp_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_BRP_STATS_TAG: /* DEPRECATED */
        htt_print_txbf_ofdma_brp_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_STEER_STATS_TAG: /* DEPRECATED */
        htt_print_txbf_ofdma_steer_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_SELFGEN_AC_SCHED_STATUS_STATS_TAG:
        htt_print_tx_selfgen_ac_sched_status_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_SELFGEN_AX_SCHED_STATUS_STATS_TAG:
        htt_print_tx_selfgen_ax_sched_status_stats_tlv(tag_buf);
        break;

    case HTT_STATS_UNAVAILABLE_ERROR_STATS_TAG:
        htt_print_unavailable_error_stats_tlv(tag_buf);
        break;

    case HTT_STATS_UNSUPPORTED_ERROR_STATS_TAG:
        htt_print_unsupported_error_stats_tlv(tag_buf);
        break;

    case HTT_STATS_VDEV_RTT_RESP_STATS_TAG:
        htt_print_vdev_rtt_resp_stats_tlv(tag_buf);
        break;

    case HTT_STATS_VDEV_RTT_INIT_STATS_TAG:
        htt_print_vdev_rtt_init_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PKTLOG_AND_HTT_RING_STATS_TAG:
        htt_print_pktlog_and_htt_ring_stats_tlv(tag_buf);
        break;

    case HTT_STATS_DLPAGER_STATS_TAG:
        htt_print_dlpager_stats(tag_buf);
        break;

    case HTT_STATS_PHY_COUNTERS_TAG:
         htt_print_phy_counters_tlv(tag_buf);
         break;

    case HTT_STATS_PHY_STATS_TAG:
         htt_print_phy_stats_tlv(tag_buf);
         break;

    case HTT_STATS_STA_UL_OFDMA_STATS_TAG:
         htt_print_sta_ul_ofdma_stats_tlv(tag_buf);
         break;

    case HTT_STATS_PHY_RESET_COUNTERS_TAG:
         htt_print_phy_reset_counters_tlv(tag_buf);
         break;

    case HTT_STATS_PHY_RESET_STATS_TAG:
         htt_print_phy_reset_stats_tlv(tag_buf);
         break;

    case HTT_STATS_SOC_TXRX_STATS_COMMON_TAG:
        htt_print_soc_txrx_stats_common_tlv(tag_buf);
        break;

    case HTT_STATS_VDEV_TXRX_STATS_HW_STATS_TAG:
        htt_print_vdev_txrx_stats_hw_tlv(tag_buf);
        break;

    case HTT_STATS_AST_ENTRY_TAG:
        htt_print_ast_entry_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_BE_DL_MU_OFDMA_STATS_TAG:
        htt_print_tx_pdev_be_dl_mu_ofdma_sch_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_BE_UL_MU_OFDMA_STATS_TAG:
        htt_print_tx_pdev_be_ul_mu_ofdma_sch_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_RATE_STATS_BE_OFDMA_TAG:
        htt_print_tx_pdev_rate_stats_be_ofdma_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_BW_MGR_STATS_TAG:
        htt_print_pdev_bw_mgr_stats_tlv(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_UL_MUMIMO_TRIG_BE_STATS_TAG:
        htt_print_ul_mumimo_trig_be_stats(tag_buf);
        break;

    case HTT_STATS_TX_SELFGEN_BE_STATS_TAG:
        htt_print_tx_selfgen_be_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_SELFGEN_BE_ERR_STATS_TAG:
        htt_print_tx_selfgen_be_err_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_SELFGEN_BE_SCHED_STATUS_STATS_TAG:
        htt_print_tx_selfgen_be_sched_status_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_BE_UL_MU_MIMO_STATS_TAG:
        htt_print_tx_pdev_be_ul_mu_mimo_sch_stats_tlv(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_BE_UL_MIMO_USER_STATS_TAG:
        htt_print_be_ul_mimo_user_stats(tag_buf);
        break;

    case HTT_STATS_RX_RING_STATS_TAG:
        htt_print_rx_fw_ring_stats_tlv(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_BE_UL_TRIG_STATS_TAG:
        htt_print_be_ul_ofdma_trigger_stats(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_AX_NDPA_STATS_TAG:
        htt_print_txbf_ofdma_ax_ndpa_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_AX_NDP_STATS_TAG:
        htt_print_txbf_ofdma_ax_ndp_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_AX_BRP_STATS_TAG:
        htt_print_txbf_ofdma_ax_brp_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_AX_STEER_STATS_TAG:
        htt_print_txbf_ofdma_ax_steer_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_BE_NDPA_STATS_TAG:
        htt_print_txbf_ofdma_be_ndpa_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_BE_NDP_STATS_TAG:
        htt_print_txbf_ofdma_be_ndp_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_BE_BRP_STATS_TAG:
        htt_print_txbf_ofdma_be_brp_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_BE_STEER_STATS_TAG:
        htt_print_txbf_ofdma_be_steer_stats_tlv(tag_buf);
        break;

    case HTT_STATS_DMAC_RESET_STATS_TAG:
        htt_print_dmac_reset_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PHY_TPC_STATS_TAG:
         htt_print_phy_tpc_stats_tlv(tag_buf);
         break;

    case HTT_STATS_PDEV_PUNCTURE_STATS_TAG:
        htt_print_puncture_stats_tlv(tag_type, tag_buf);
        break;

    case HTT_STATS_ML_PEER_DETAILS_TAG:
        htt_print_ml_peer_details_stats_tlv(tag_buf);
        break;

    case HTT_STATS_ML_PEER_EXT_DETAILS_TAG:
        htt_print_ml_peer_ext_stats_tlv(tag_buf);
        break;

    case HTT_STATS_ML_LINK_INFO_DETAILS_TAG:
        htt_print_ml_link_info_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_PPDU_DUR_TAG:
        htt_print_tx_pdev_ppdu_dur_stats_tlv(tag_buf);
        break;

    case HTT_STATS_RX_PDEV_PPDU_DUR_TAG:
        htt_print_rx_pdev_ppdu_dur_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_SCHED_ALGO_OFDMA_STATS_TAG:
        htt_print_pdev_sched_algo_ofdma_stats_tlv(tag_buf);
        break;

    case HTT_STATS_LATENCY_PROF_CAL_STATS_TAG: /* DEPRECATED: */
        htt_print_latency_prof_cal_stats_tlv(tag_buf);
        break;

    case HTT_STATS_LATENCY_PROF_CAL_DATA_TAG:
        htt_print_latency_prof_cal_data_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_MUEDCA_PARAMS_STATS_TAG: /* DEPRECATED */
        htt_print_tx_pdev_muedca_params_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_AP_EDCA_PARAMS_STATS_TAG:
        htt_print_tx_pdev_ap_edca_params_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_AX_STEER_MPDU_STATS_TAG:
        htt_print_txbf_ofdma_ax_steer_mpdu_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TXBF_OFDMA_BE_STEER_MPDU_STATS_TAG:
        htt_print_txbf_ofdma_be_steer_mpdu_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PEER_AX_OFDMA_STATS_TAG:
        htt_print_peer_ax_ofdma_stats(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_MU_EDCA_PARAMS_STATS_TAG:
        htt_print_tx_pdev_mu_edca_params_stats_tlv_v(tag_buf);
        break;

    case HTT_STATS_ODD_PDEV_MANDATORY_TAG:
        htt_print_odd_pdev_mandatory_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_MBSSID_CTRL_FRAME_STATS_TAG:
        htt_print_pdev_mbssid_ctrl_frame_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_MLO_ABORT_TAG:
        htt_print_tx_pdev_stats_mlo_abort_tlv_v(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_MLO_TXOP_ABORT_TAG:
        htt_print_tx_pdev_stats_mlo_txop_abort_tlv_v(tag_buf);
        break;

    case HTT_STATS_UMAC_SSR_TAG:
        htt_print_umac_ssr_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PEER_BE_OFDMA_STATS_TAG:
        htt_print_peer_be_ofdma_stats(tag_buf);
        break;

    case HTT_STATS_PDEV_TDMA_TAG:
        htt_print_pdev_tdma_stats_tlv(tag_buf);
        break;

    case HTT_STATS_CODEL_SVC_CLASS_TAG:
        htt_print_codel_svc_class_stats_tlv(tag_buf);
        break;

    case HTT_STATS_CODEL_MSDUQ_TAG:
        htt_print_codel_msduq_stats_tlv(tag_buf);
        break;

    case HTT_STATS_MLO_SCHED_STATS_TAG:
        htt_print_mlo_sched_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_MLO_IPC_STATS_TAG:
        htt_print_mlo_ipc_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_RTT_RESP_STATS_TAG:
        htt_print_pdev_rtt_resp_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_RTT_INIT_STATS_TAG:
        htt_print_pdev_rtt_init_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_RTT_HW_STATS_TAG:
        htt_print_pdev_rtt_hw_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_RTT_TBR_SELFGEN_QUEUED_STATS_TAG:
        htt_print_pdev_rtt_tbr_selfgen_queued_stats_tlv(tag_buf);
        break;

    case HTT_STATS_PDEV_RTT_TBR_CMD_RESULT_STATS_TAG:
        htt_print_pdev_rtt_tbr_cmd_result_stats_tlv(tag_buf);
        break;

    case HTT_STATS_GTX_TAG:
        htt_print_gtx_stats_tlv(tag_buf);
        break;

    case HTT_STATS_TX_PDEV_WIFI_RADAR_TAG:
        htt_print_wifi_radar_stats_tlv(tag_buf);
        break;


    default:
        break;
    }
}

void htt_stats_msg_receive(
        void *data,
        A_INT32 len)
{
    static A_UINT8       *tlv_buf_head     = NULL;
    static A_UINT8       *tlv_buf_tail     = NULL;
    A_UINT32             msg_remain_len    = len;
    static A_UINT32      tlv_remain_len    = 0;
    A_UINT32             tlv_len           = 0;
    A_UINT32             *tlv_start;
    A_UINT32             *msg_word;
    static htt_tlv_tag_t tlv_type          = 0xff;

    msg_word = htt_stats_msg_get(data);

    while (msg_remain_len) {
        /*
         * if message is not a continuation of previous message
         * read the tlv type and tlv length
         */
        if (!tlv_buf_head) {
            tlv_type = HTT_STATS_TLV_TAG_GET(
                    *msg_word);
            tlv_len = HTT_STATS_TLV_LENGTH_GET(
                    *msg_word);
            tlv_remain_len = tlv_len;
        }

        if (tlv_remain_len == 0) {
            msg_remain_len = 0;

            if (tlv_buf_head) {
                free(tlv_buf_head);
                tlv_buf_head = NULL;
                tlv_buf_tail = NULL;
            }
            return;
        }
        if (!tlv_buf_head) {
            tlv_remain_len += HTT_TLV_HDR_LEN;
        }

        if ((tlv_remain_len <= msg_remain_len)) {
            /* Case 3 */
            if (tlv_buf_head) {
                memcpy(tlv_buf_tail,
                        (A_UINT8 *)msg_word,
                        tlv_remain_len);
                tlv_start = (A_UINT32 *)tlv_buf_head;
            } else {
                /* Case 1 */
                tlv_start = msg_word;
            }
            htt_htt_stats_print_tag(tlv_type, tlv_start);

            msg_remain_len -= tlv_remain_len;

            msg_word = (A_UINT32 *)
                       (((A_UINT8 *)msg_word) +
                        tlv_remain_len);

            tlv_remain_len = 0;

            if (tlv_buf_head) {
                free(tlv_buf_head);
                tlv_buf_head = NULL;
                tlv_buf_tail = NULL;
            }
        } else { /* tlv_remain_len > msg_remain_len */
            /* Case 2 & 3 */
            if (!tlv_buf_head) {
                tlv_buf_head = malloc(
                        tlv_remain_len);

                if (!tlv_buf_head) {
                    HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
                    return;
                }
                tlv_buf_tail = tlv_buf_head;
            }
            memcpy(tlv_buf_tail, (A_UINT8 *)msg_word,
                    msg_remain_len);
            tlv_remain_len -= msg_remain_len;
            tlv_buf_tail   += msg_remain_len;
            msg_remain_len  = 0;
        }
    }
}

#ifndef __KERNEL__

static void htt_stats_usage(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= USAGE =================\n");
    printf("%s %s\n", argv[0], argv[1]);
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t- ex: wifiX\n");
    printf("\t\t<cmd_id>\t- 1 - 19\n");
    printf("\t - optional args\n");
    printf("\t\t <config_param0>\t default 0 if not mentioned\n");
    printf("\t\t <config_param1>\t default 0 if not mentioned\n");
    printf("\t\t <config_param2>\t default 0 if not mentioned\n");
    printf("\t\t <config_param3>\t default 0 if not mentioned\n");
    printf("Example:\n");
    printf("\twifistats wifiX <cmd_id> <optional config_params>\n");
    printf("=========================================\n");
}

static void htt_stats_usage_peerstats(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= PEER STATS USAGE =================\n");
    printf("%s \n",                   argv[0]);
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t- ex: wifiX\n");
    printf("\t\t <cmid_id> is %d \n", HTT_DBG_EXT_STATS_PEER_INFO);
    printf("\t\t One of either two below: --peerid or--mac\n");
    printf("\t\t\t --mac   \t followed by MAC in aa:bb:cc:dd:ee:ff format\n");
    printf("\t\t\t\t -OR- \n");
    printf("\t\t \t--peerid\t followed by sw_peer_id\n");
    printf("\t - optional args\n");
    printf("\t\t --mode\t followed by peer_stats_req_mode\n");
    printf("\t\t --mask\t followed by req_type_bitmask\n");
    printf("Example:\n");
    printf("\t %s %s wifiX %d --mac aa:bb:cc:dd:ee:ff <optional args>\n", argv[0], argv[1],
            HTT_DBG_EXT_STATS_PEER_INFO);
    printf("\t\t-OR- \n");
    printf("\t %s %s wifiX %d --peerid N <optional args>\n",              argv[0], argv[1],
            HTT_DBG_EXT_STATS_PEER_INFO);
    printf("=========================================\n");
}

static void htt_stats_usage_reset(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= RESET STATS USAGE =================\n");
    printf("%s \n",                                   argv[0]);
    printf("\t - necessary args \n");
    printf("\t\t <radio_name>\t- ex: wifiX\n");
    printf("\t\t <cmid_id> is %d \n",                 HTT_DBG_EXT_STATS_RESET);
    printf("\t\t <config_param0>\t stat number to reset\n");
    printf("Example:\n");
    printf("\t %s %s wifiX %d <stat_num_to_reset>\n", argv[0], argv[1], HTT_DBG_EXT_STATS_RESET);
    printf("=========================================\n");
}

static void htt_stats_usage_vdevstats(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= VDEV STATS USAGE =================\n");
    printf("wifistats \n");
    printf("\t - necessary args \n");
    printf("\t\t <radio_name>\t\t- ex: wifiX\n");
    printf("\t\t <cmid_id>\t\t  <%d|%d|%d>\n", HTT_DBG_EXT_STATS_TX_SELFGEN_INFO,
            HTT_DBG_EXT_STATS_PDEV_TX_MU, HTT_DBG_EXT_STATS_TX_SOUNDING_INFO);
    printf("\t\t --vdevid <vdev_id>\t- ex: --vdevid <0-255>\n");
    printf("Example:\n");
    printf("\twifistats wifiX %d --vdevid 5\n", HTT_DBG_EXT_STATS_TX_SELFGEN_INFO);
    printf("\twifistats wifiX %d --vdevid 7\n", HTT_DBG_EXT_STATS_PDEV_TX_MU);
    printf("\twifistats wifiX %d --vdevid 3\n", HTT_DBG_EXT_STATS_TX_SOUNDING_INFO);
    printf("=========================================\n");
}

static void htt_stats_usage_peer_ctrl_path_txrx_stats(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= CONTROL PATH PEER TX RX STATS USAGE =================\n");
    printf("%s \n", argv[0]);
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t: wifiX\n");
    printf("\t\t<cmid_id>\t: %d\n", HTT_DBG_EXT_PEER_CTRL_PATH_TXRX_STATS);
    printf("\t - optional args\n");
    printf("\t\t<argument>\t: --mac\n");
    printf("\t\t--mac\t: followed by MAC in aa:bb:cc:dd:ee:ff format\n");
    printf("Example:\n");
    printf("1. To Display Control Path Stats\n");
    printf("\t %s %s %d\n", argv[0], argv[1], HTT_DBG_EXT_PEER_CTRL_PATH_TXRX_STATS);
    printf("2. To Configure Peer and Display Control Path Stats\n");
    printf("\t %s %s %d --mac aa:bb:cc:dd:ee:ff\n", argv[0], argv[1],
            HTT_DBG_EXT_PEER_CTRL_PATH_TXRX_STATS);
    printf("3. To Reset Control Path Stats\n");
    printf("\t %s %s 0 %d\n", argv[0], argv[1], HTT_DBG_EXT_PEER_CTRL_PATH_TXRX_STATS);
    printf("=========================================\n");
}

static void htt_stats_usage_ml_peer_stats(
        A_INT32 argc,
        A_CHAR *argv[])
{
    printf("========= ML PEER STATS USAGE =================\n");
    printf("%s \n",                   argv[0]);
    printf("\t - necessary args \n");
    printf("\t\t<radio_name>\t- ex: wifiX\n");
    printf("\t\t <cmid_id> is %d \n", HTT_DBG_EXT_STATS_ML_PEERS_INFO);
    printf("\t - optional args\n");
    printf("\t\t --mask\t followed by req_type_bitmask\n");
    printf("\t\t Bit 0 -> HTT_ML_PEER_DETAILS_TLV always enabled\n");
    printf("\t\t Bit 1 -> HTT_ML_PEER_EXT_DETAILS_TLV\n");
    printf("\t\t Bit 2 -> HTT_ML_LINK_INFO_TLV\n");
    printf("Example:\n");
    printf("\t %s %s %d <optional args>\n", argv[0], argv[1], HTT_DBG_EXT_STATS_ML_PEERS_INFO);
    printf("\t %s %s %d --mask 7\n", argv[0], argv[1], HTT_DBG_EXT_STATS_ML_PEERS_INFO);
    printf("=========================================\n");
}

void htt_stats_help(
        A_INT32 argc,
        A_CHAR *argv[])
{
    A_INT32 stats_id = atoi(argv[2]);

    if (stats_id == HTT_DBG_EXT_STATS_PEER_INFO) {
        htt_stats_usage_peerstats(argc, argv);
    } else if (stats_id == HTT_DBG_EXT_STATS_RESET) {
        htt_stats_usage_reset(argc, argv);
    } else if ((stats_id == HTT_DBG_EXT_STATS_TX_SOUNDING_INFO) ||
               (stats_id == HTT_DBG_EXT_STATS_PDEV_TX_MU) ||
               (stats_id == HTT_DBG_EXT_STATS_TX_SELFGEN_INFO))
    {
        htt_stats_usage_vdevstats(argc, argv);
    } else if (stats_id == HTT_DBG_EXT_PEER_CTRL_PATH_TXRX_STATS ) {
        htt_stats_usage_peer_ctrl_path_txrx_stats(argc, argv);
    } else if (stats_id == HTT_DBG_EXT_STATS_ML_PEERS_INFO) {
        htt_stats_usage_ml_peer_stats(argc, argv);
    } else {
        htt_stats_usage(argc, argv);
    }
}

static void *htt_stats_buff_alloc(A_INT32 *buff_len)
{
    void *buff = malloc(sizeof(struct httstats_cmd_request));

    if (!buff) {
        HTT_STATS_ERR(FATAL, "%s: %d: Failed to allocate memory", __func__, __LINE__);
        return NULL;
    }
    memset(buff, 0x0, sizeof(struct httstats_cmd_request));
    *buff_len = sizeof(struct httstats_cmd_request);

    return buff;
}

static void htt_stats_buff_free(void *buff)
{
    free(buff);
}

A_INT32 htt_vdev_stats_req_prepare(
        struct httstats_cmd_request *stats_req,
        A_INT32 argc,
        A_CHAR *argv[])
{
    A_INT8 vdev_id;

    /* To handle mu pdev stats numbers 12,17,22 */
    if (argc == 3) {
        if (stats_req->stats_id == HTT_DBG_EXT_STATS_PDEV_TX_MU)
            stats_req->config_param1 = HTT_UPLOAD_MU_STATS;
        return 0;
    }

    if ((stats_req->stats_id == HTT_DBG_EXT_STATS_PDEV_TX_MU) && (argc == 4) && (strcmp(argv[3], "--vdevid") != 0)) {
        stats_req->config_param1 = strtoul(argv[3], NULL, 0);
        return 0;
    } else {
        stats_req->config_param1 = HTT_UPLOAD_MU_STATS;
    }

    if ((argc == 5) && (strcmp(argv[3], "--vdevid") == 0)) {
        vdev_id = strtoul(argv[4], NULL, 0);

        if (vdev_id <= 255) {
            stats_req->config_param0 = HTT_DBG_EXT_STATS_SET_VDEV_MASK(vdev_id);
        } else {
            printf("Specify valid arguement value pair. Ex: --vdevid <vdev-id>\n");
            return -EIO;
        }
    } else {
        return -EIO;
    }
    return 0;
}

static A_INT32 htt_configure_debug_peer_stats_req_prepare(
        struct httstats_cmd_request *stats_req,
        A_INT32 argc,
        A_CHAR *argv[])
{
    A_CHAR mac_str[18] = {0};
    A_UINT8 mac_addr[IEEE80211_ADDR_LEN];
    A_UINT8 i = 0, id_or_mac_specified = 0;
    for (i = 3; i < argc; i += 2) {
        if (strcmp(argv[i], "--mac") == 0) {
            if (id_or_mac_specified) { /*Already specifed mac/peerid*/
                fprintf(stderr, "Specify only --mac\n");
                return -EIO;
            }
            strlcpy(mac_str, argv[i + 1], 18);

            if (extract_mac_addr(mac_addr, mac_str) != 0) {
                fprintf(stderr, "Invalid PEER MAC\n");
                return -EIO;
            }
            HTT_DBG_EXT_PEER_CTRL_PATH_TXRX_STATS_IS_MAC_ADDR_SET(stats_req->config_param0, 1);
            stats_req->config_param1 |= (mac_addr[0]         & 0x000000ff);
            stats_req->config_param1 |= ((mac_addr[1] << 8)  & 0x0000ff00);
            stats_req->config_param1 |= ((mac_addr[2] << 16) & 0x00ff0000);
            stats_req->config_param1 |= ((mac_addr[3] << 24) & 0xff000000);
            stats_req->config_param2 |= (mac_addr[4]         & 0x000000ff);
            stats_req->config_param2 |= ((mac_addr[5] << 8)  & 0x0000ff00);
            id_or_mac_specified++;
        }
    }
    return 0;
}

static A_INT32 htt_peer_stats_req_prepare(
        struct httstats_cmd_request *stats_req,
        A_INT32 argc,
        A_CHAR *argv[])
{
    A_CHAR mac_str[18] = {0};
    A_UINT8 mac_addr[IEEE80211_ADDR_LEN];
    A_UINT8 i = 0, id_or_mac_specified = 0;

    /*default values for optional params*/
    stats_req->config_param1 = 0xFFFFFFFF; /*all stats*/
    // HTT_DBG_EXT_STATS_PEER_REQ_MODE_SET(stats_req->config_param0, HTT_PEER_STATS_REQ_MODE_FLUSH_TQM);
    stats_req->config_param0 |= HTT_PEER_STATS_REQ_MODE_FLUSH_TQM << 1;

    for (i = 3; i < argc; i += 2) {
        if (strcmp(argv[i], "--mode") == 0) {
            stats_req->config_param0 &= ~HTT_DBG_EXT_STATS_PEER_REQ_MODE_M; /*clear default value entered earlier*/
            // HTT_DBG_EXT_STATS_PEER_REQ_MODE_SET(stats_req->config_param0, (strtoul(argv[i+1], NULL, 0) & 0x7FFF));
            stats_req->config_param0 |= (strtoul(argv[i + 1], NULL, 0) & 0x7FFF) << 1;
        } else if (strcmp(argv[i], "--mask") == 0) {
            stats_req->config_param1 = strtoul(argv[i + 1], NULL, 0);
        } else if (strcmp(argv[i], "--mac") == 0) {
            if (id_or_mac_specified) { /*Already specifed mac/peerid*/
                printf("Specify only one:either --peerid or --mac\n");
                return -EIO;
            }
            strlcpy(mac_str, argv[i + 1], 18);

            if (extract_mac_addr(mac_addr, mac_str) != 0) {
                printf("Invalid MAC PEER MAC\n");
                return -EIO;
            }
            HTT_DBG_EXT_STATS_PEER_INFO_IS_MAC_ADDR_SET(stats_req->config_param0, 1);
            stats_req->config_param2 |= (mac_addr[0] & 0x000000ff);
            stats_req->config_param2 |= ((mac_addr[1] << 8) & 0x0000ff00);
            stats_req->config_param2 |= ((mac_addr[2] << 16) & 0x00ff0000);
            stats_req->config_param2 |= ((mac_addr[3] << 24) & 0xff000000);
            stats_req->config_param3 |= (mac_addr[4] & 0x000000ff);
            stats_req->config_param3 |= ((mac_addr[5] << 8) & 0x0000ff00);
            id_or_mac_specified++;
        } else if (strcmp(argv[i], "--peerid") == 0) {
            if (id_or_mac_specified) { /*Already specifed mac/peerid*/
                printf("Specify only one:either --peerid or --mac\n");
                return -EIO;
            }
            HTT_DBG_EXT_STATS_PEER_INFO_IS_MAC_ADDR_SET(stats_req->config_param0, 0);
            HTT_DBG_EXT_STATS_PEER_INFO_SW_PEER_ID_SET(stats_req->config_param0,
                    (strtoul(argv[i + 1], NULL, 0) & 0xFFFF));
            id_or_mac_specified++;
        } else {
            printf("Unsupported option entered\n");
            return -EIO;
        }
    }

    if (id_or_mac_specified != 1) { /*Not specifed peerid or mac*/
        printf("Must Specify one:either --peerid or --mac\n");
        return -EIO;
    }
    return 0;
}

static A_INT32 htt_ml_peer_stats_req_prepare(
    struct httstats_cmd_request *stats_req,
    A_INT32 argc,
    A_CHAR *argv[])
{
    A_UINT8 i = 0;

    /* default values for optional params */
    stats_req->config_param0 = 0x1; /* peer details stats is enabled, link and ext stats are disabled by default */

    for (i = 3; i < argc; i += 2) {
        if (strcmp(argv[i], "--mask") == 0) {
            stats_req->config_param0 = strtoul(argv[i + 1], NULL, 0);
        } else {
            printf("Unsupported option entered\n");
            return -EIO;
        }
    }

    return 0;
}

static A_INT32 htt_stats_cookie_generate(void)
{
    static A_INT32 httstats_cookie = 0;

    if (!httstats_cookie) {
        httstats_cookie = getpid();
    }
    return httstats_cookie;
}

static A_INT32 htt_stats_input_parse(
        void *buff,
        A_INT32 argc,
        A_CHAR *argv[],
        A_INT32 *buff_len,
        A_INT32 pdev_id)
{
    struct httstats_cmd_request *stats_req = (struct httstats_cmd_request *)buff;

    stats_req->stats_id = atoi(argv[2]);
    stats_req->pid      = htt_stats_cookie_generate();

    if (stats_req->stats_id == HTT_DBG_EXT_STATS_PEER_INFO) {
        if ((argc < 4) || (argc > 9) || (argc % 2 == 0)) {
            fprintf(stderr, "Invalid commands args\n");
            return -EIO;
        }

        if (htt_peer_stats_req_prepare(stats_req, argc, argv) != 0) {
            fprintf(stderr, "Invalid commands args\n");
            return -EIO;
        }
    } else if ((stats_req->stats_id == HTT_DBG_EXT_STATS_TX_SOUNDING_INFO) ||
               (stats_req->stats_id == HTT_DBG_EXT_STATS_PDEV_TX_MU) ||
               (stats_req->stats_id == HTT_DBG_EXT_STATS_TX_SELFGEN_INFO))
    {
        if (htt_vdev_stats_req_prepare(stats_req, argc, argv) != 0) {
            return -EIO;
        }
    } else if (stats_req->stats_id == HTT_DBG_EXT_PEER_CTRL_PATH_TXRX_STATS) {
        if (argc != 3 && argc != 5) {
            fprintf(stderr, "Invalid commands args\n");
            return -EIO;
        }
        if (argc == 5) {
            if (htt_configure_debug_peer_stats_req_prepare(stats_req, argc, argv) != 0) {
                fprintf(stderr, "Invalid commands args\n");
                return -EIO;
            }
        }
    } else if (stats_req->stats_id == HTT_DBG_EXT_STATS_ML_PEERS_INFO) {
        if (((argc != 3) || (argc % 2 == 0)) &&
            ((argc != 5) || (argc % 2 == 0)))
        {
            fprintf(stderr, "Invalid commands args %d\n", argc);
            return -EIO;
        }

        if (htt_ml_peer_stats_req_prepare(stats_req, argc, argv) != 0) {
            fprintf(stderr, "Invalid commands args\n");
            return -EIO;
        }
    } else {
        if (argc >= 4) {
            stats_req->config_param0 = strtoul(argv[3], NULL, 0);
        } else {
            if (stats_req->stats_id == HTT_DBG_EXT_STATS_RESET) {
                return -EIO;
            }
        }

        if (argc >= 5) {
            stats_req->config_param1 = strtoul(argv[4], NULL, 0);
        } else {
            if (stats_req->stats_id == HTT_DBG_EXT_STATS_RESET) {
                stats_req->config_param1 = 0x1; /*default query 1 stat*/
            }
        }

        if (argc >= 6) {
            stats_req->config_param2 = strtoul(argv[5], NULL, 0);
        }

        if (argc >= 7) {
            stats_req->config_param3 = strtoul(argv[6], NULL, 0);
        }
    }

    if (stats_req->stats_id == HTT_DBG_EXT_STATS_RESET) {
        httstats.timeout = 0;
    }
    *buff_len = sizeof(struct httstats_cmd_request);
    return 0;
}

static void htt_stats_print_tlv(void *tlv_ptr)
{
    htt_tlv_tag_t tlv_type;
    A_UINT32      *msg_word = (A_UINT32 *) tlv_ptr;
    tlv_type = HTT_STATS_TLV_TAG_GET(*msg_word);

    htt_htt_stats_print_tag(tlv_type, tlv_ptr);
}

static void *htt_stats_get_buf_start(void *buff, A_INT32 *len, int *listen_done)
{
    A_UINT32 *msg_word = NULL;

    msg_word = (A_UINT32 *)buff;
    msg_word = msg_word + 3;

    *listen_done = 0;
    if (HTT_T2H_EXT_STATS_CONF_TLV_DONE_GET(*msg_word)) {
        *listen_done = 1;
    }
    msg_word++;
    *len -= (((void *)msg_word) - buff);
    return msg_word;
}

static A_INT32 htt_stats_handler(
        void *buff,
        A_INT32 len)
{
    A_UINT32 *msg_word = NULL;
    A_UINT8  done      = 0;
    A_INT32  status    = LISTEN_CONTINUE;

    msg_word = (A_UINT32 *)buff;
    msg_word = msg_word + 3;

    done = HTT_T2H_EXT_STATS_CONF_TLV_DONE_GET(*msg_word);

    if (done) {
        status = LISTEN_DONE;
    }
    msg_word++;

    /* Updated length to reduce the HTT header len as
     * htt_stats_msg_receive() do not expect HTT header*/
    htt_stats_msg_receive((void *)msg_word, (len - 16));

    return status;
}

static A_INT32 htt_stats_cookie_get(
        void *buff,
        A_INT32 len)
{
    A_UINT32 *msg_word = NULL;
    A_INT32  cookie    = 0;

    msg_word = (A_UINT32 *)buff;
    cookie   = (A_INT32)(*(msg_word + 1)); /*Cookie LSB*/

    return cookie;
}

int htt_stats_tlv_length(void *data)
{
    return (HTT_TLV_HDR_LEN +
                   HTT_STATS_TLV_LENGTH_GET(*((A_UINT32 *) data)));
}

static struct wifistats_module httstats = {
    .name                  = "htt_fw_stats",
    .help                  = htt_stats_help,
    .input_buff_alloc      = htt_stats_buff_alloc,
    .input_parse           = htt_stats_input_parse,
    .input_buff_free       = htt_stats_buff_free,
    .input_cookie_generate = htt_stats_cookie_generate,
    .output_tlv_length     = htt_stats_tlv_length,
    .output_get_buf_start  = htt_stats_get_buf_start,
    .output_print_tlv      = htt_stats_print_tlv,
    .output_handler        = htt_stats_handler,
    .output_cookie_get     = htt_stats_cookie_get,
    .timeout               = 2000,
    .output_fp             = NULL,
    .tlv_hdr_len           = HTT_TLV_HDR_LEN,
};

void httstats_init(void)
{
    wifistats_module_register(&httstats, sizeof(httstats));
}

void httstats_fini(void)
{
    wifistats_module_unregister(&httstats, sizeof(httstats));
}

#endif /* ifndef __KERNEL__ */
