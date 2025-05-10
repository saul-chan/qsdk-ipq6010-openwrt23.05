/*
 * Copyright (c) 2017-2020 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * Copyright (c) 2011, Atheros Communications Inc.
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
 */

#ifndef OL_IF_DCS_H
#define OL_IF_DCS_H

#include <ieee80211_channel.h>

#define DCS_PHYERR_PENALTY      (500)
#define DCS_PHYERR_THRESHOLD    (300)
#define DCS_RADARERR_THRESHOLD  (1000)
#define DCS_COCH_INTR_THRESHOLD (30) /* 30 % excessive channel utilization */
#define DCS_TXERR_THRESHOLD     (30)
#define DCS_USER_MAX_CU         (50) /* tx ch utilization due to AP tx and rx */
#define DCS_TX_MAX_CU           (30)
#define DCS_INTR_DETECTION_THR  (6)
#define DCS_SAMPLE_SIZE         (10)
/* Duration after which DCS should be enabled back after
 * disabling it due to 3 triggers in 5 minutes */
#define DCS_ENABLE_TIME         (30 * 60)
#define DCS_ENABLE_TIME_MIN     (5 * 60)
#define DCS_ENABLE_TIME_MAX     (60 * 60)
#define DCS_MAX_TRIGGERS        (3)
#define DCS_AGING_TIME          (300000) /* 5 mins */

#define OL_ATH_DCS_ENABLE(__arg1, val)  ((__arg1) |= (val))
#define OL_ATH_DCS_DISABLE(__arg1, val) ((__arg1) &= ~(val))
#define OL_ATH_DCS_SET_RUNSTATE(__arg1) ((__arg1) |= 0x80)
#define OL_ATH_DCS_CLR_RUNSTATE(__arg1) ((__arg1) &= ~0x80)
#define OL_IS_DCS_ENABLED(__arg1, val) ((__arg1) & val)
#define OL_IS_DCS_RUNNING(__arg1) ((__arg1) & 0x80)

#define OL_ATH_DCS_FREQ_TO_LIST(__list, __num, freq) ((__list)[(*__num)++] = freq)

/**
 * struct _wlan_dcs_im_host_stats - dcs wlan interference mitigation stats
 * The below stats are sent from target to host every one second
 * @prev_dcs_im_stats: The previous statistics at last known time
 * @im_intr_count: number of times the interfernce is seen continuously
 * @sample_count: int_intr_count of sample_count, the interference is seen
 */
typedef struct _wlan_dcs_im_host_stats {
	wmi_host_dcs_im_tgt_stats_t prev_dcs_im_stats;
	uint8_t im_intr_cnt; /* Interefernce detection counter */
	uint8_t im_samp_cnt; /* sample counter */
} wlan_dcs_im_host_stats_t;

typedef enum {
	DCS_DEBUG_DISABLE  = 0,
	DCS_DEBUG_CRITICAL = 1,
	DCS_DEBUG_VERBOSE  = 2,
} wlan_dcs_debug_t;

/**
 * struct _wlan_host_dcs_params - define dcs configuration parameters
 * @dcs_debug: dcs debug trace level value 0-disable, 1-critical, 2-all
 * @phy_err_penalty: phy error penalty
 * @phy_err_threshold: phy error threshold
 * @radar_err_threshold: radar error threshold
 * @coch_intr_thresh: co-channel interference threshold
 * @user_max_cu: tx channel utilization due to AP's tx and rx, tx_cu + rx_cu
 * @intr_detection_threshold: interference detection threshold
 * @intr_detection_window: interference sampling window
 * @tx_err_thresh: transmission failure rate threshold
 * @scn_dcs_im_stats: dcs wlan interference mitigation stats
 * @chan_stats: periodic channel stats
 * @dcs_enable_timer: dcs enable timer
 * @dcs_trigger_ts: dcs trigger timestamp
 * @is_enable_timer_set: check if timer is enabled/re-enabled
 * @dcs_re_enable_time: dcs re-enable time value for dcs timer
 * @dcs_trigger_count: dcs trigger count
 * @dcs_enable: dcs enabled or not, along with running state
 * @dcs_wideband_policy: dcs wideband policy for interband and intraband
 *                       channel selection
 * @dcs_random_chan_en: Enable/disable random channel selection for individual
 *                      interference types
 * @dcs_csa_tbtt: CSA TBTT count
 * @dcs_afc_trgr_delay_timer: dcs afc trigger delay timer
 * @dcs_afc_chan_sel: Enable/disable DCS trigger for various conditions
 * @dcs_afc_trgr_delay_timer_retry_cnt: dcs afc trigger delay timer retry count
 */
typedef struct _wlan_host_dcs_params {
	wlan_dcs_debug_t dcs_debug;
	uint32_t phy_err_penalty;
	uint32_t phy_err_threshold;
	uint32_t radar_err_threshold;
	uint32_t coch_intr_thresh ;
	uint32_t user_max_cu;
	uint32_t intr_detection_threshold;
	uint32_t intr_detection_window;
	uint32_t tx_err_thresh;
	wlan_dcs_im_host_stats_t scn_dcs_im_stats;
	periodic_chan_stats_t  chan_stats;
	qdf_timer_t dcs_enable_timer;
	uint32_t dcs_trigger_ts[DCS_MAX_TRIGGERS];
	bool is_enable_timer_set;
	uint16_t dcs_re_enable_time;
	uint8_t dcs_trigger_count;
	uint32_t dcs_enable;
	wlan_dcs_wideband_policy_t dcs_wideband_policy;
	uint32_t dcs_random_chan_en;
	uint32_t dcs_csa_tbtt;
	uint32_t dcs_bw_reduction_ctrl;
#ifdef CONFIG_AFC_SUPPORT
	qdf_timer_t dcs_afc_trgr_delay_timer;
	uint8_t  dcs_afc_chan_sel;
	uint8_t  dcs_afc_trgr_delay_timer_retry_cnt;
#endif /* CONFIG_AFC_SUPPORT */
	uint32_t dcs_adj_bw_block;
} wlan_host_dcs_params_t;

/**
 * ol_ath_trigger_dcs:
 * Wrapper API to trigger DCS from an external entity
 *
 * @ic          : Pointer to the ic
 * @intf_info   : Pointer to interference-specific payload
 * @trigger_type: Type of trigger/interference requested
 *
 * Return:
 * QDF_STATUS_SUCCESS: Succesfully triggered channel change
 * QDF_STAUTS_E_*    : Failed to trigger channel change
 */
QDF_STATUS ol_ath_trigger_dcs(struct ieee80211com *ic,
                              void *intf_info,
                              int trigger_type);

/**
 * ol_ath_dcs_generic_interference_handler() - wlan cw and awgn interference
 * handler
 * @scn: Pointer to net80211 softc object
 * @intf_info: Pointer to the interference information
 * @interference_type: Interference type
 *
 * Functionality of this should be the same as
 * ath_net80211_cw_interference_handler() in lmac layer of the direct
 * attach drivers. Keep this same across both.
 *
 * When the cw interference is sent from the target, kick start the scan
 * with auto channel. This is disruptive channel change. Non-discruptive
 * channel change is the responsibility of scan module.
 *
 * Return:
 * QDF_STATUS_SUCCESS: Successful
 * QDF_STATUS_E_*    : Failed
 */
QDF_STATUS ol_ath_dcs_generic_interference_handler(struct ol_ath_softc_net80211 *scn,
					     void *intf_info,
					     enum ieee80211_cap_dcs_type interference_type);

QDF_STATUS ol_ath_awgn_interference_hanlder(struct ol_ath_softc_net80211 *scn,
					     void *intf_info);


/**
 * ol_ath_wlan_interference_handler() - wlan interference handler
 * @scn: Pointer to net80211 softc object
 * @curr_stats: dcs curr stats
 * @interference_type: Interference type
 *
 * Return: none
 */
void ol_ath_wlan_interference_handler(struct ol_ath_softc_net80211 * scn,
				      wmi_host_dcs_im_tgt_stats_t *curr_stats,
				      uint32_t interference_type);

/**
 * ol_ath_reset_dcs_params() - Reset dcs parameters
 * @scn: Pointer to scn structure
 *
 * Return: none
 */
void ol_ath_reset_dcs_params(struct ol_ath_softc_net80211 *scn);

/**
 * ol_ath_disable_dcsim() - Disable DCS IM
 * @ic: ic handle
 *
 * Disable the dcs im when the intereference is detected too many times,
 * for thresholds check umac
 *
 * Return: none
 */
void ol_ath_disable_dcsim(struct ieee80211com *ic);

/**
 * ol_ath_enable_dcsim() - Enable DCS IM
 * @ic: ic handle
 *
 * Return: none
 */
void ol_ath_enable_dcsim(struct ieee80211com *ic);

/**
 * ol_ath_ctrl_dcs_awgnim() - Enable/disable DCS AWGN interference management
 * @ic    : ic handle
 * @flag  : Pointer to the flag to enable/disable AWGN
 * @enable: Flag to enable/disable AWGN interference
 *
 * Enable/Disable the DCS AWGN interference management.
 *
 * Return: None
 */
void ol_ath_ctrl_dcsawgn(struct ieee80211com *ic, uint32_t *flag, bool enable);

/**
 * ol_ath_ctrl_dcsobss() - Enable/disable DCS OBSS interference management
 * @ic    : ic handle
 * @flag  : Pointer to the flag to enable/disable OBSS
 * @enable: Flag to enable/disable OBSS interference
 *
 * Enable/Disable the DCS OBSS interference management.
 *
 * Return: None
 */
void ol_ath_ctrl_dcsobss(struct ieee80211com *ic, uint32_t *flag, bool enable);

/**
 * ol_ath_disable_dcscw() - Disable DCS CW
 * @ic: ic handle
 *
 * Disable the dcs cw when the intereference is detected too many times
 * For thresholds check umac
 *
 * Return: none
 */
void ol_ath_disable_dcscw(struct ieee80211com *ic);

/**
 * ol_ath_dcs_restore() - Restore DCS state
 * @ic: ic handle
 *
 * Turn on the dcs, use the same state as what the current
 * enabled state of dcs and reset the cw interference flag
 * Also set the run state accordingly
 *
 * Return: none
 */
void ol_ath_dcs_restore(struct ieee80211com *ic);

/**
 * ol_ath_dcs_attach() - Register the DCS functionality
 * @ic: ic handle
 *
 * Return: none
 */
void ol_ath_dcs_attach(struct ieee80211com *ic);

/**
 * ol_ath_dcs_attach() - Deregister dcs functionality
 * @ic: ic handle
 *
 * Return: none
 */
void ol_ath_dcs_dettach(struct ieee80211com *ic);

/**
 * ol_ath_soc_dcs_attach() - Register DCS with target
 * @soc: soc soft context object
 *
 * Return: none
 */
void ol_ath_soc_dcs_attach(ol_ath_soc_softc_t *soc);

/**
 * ol_ath_req_ext_dcs_trigger - Request sending DCS trigger to extenal handler
 * @scn: Pointer to net80211 softc object
 * @interference_type: Interference type
 *
 * Return: 0 on success, negative error number on failure
 */
int ol_ath_req_ext_dcs_trigger(struct ol_ath_softc_net80211 *scn,
			       uint32_t interference_type);

/**
 * wlan_dcs_im_print_stats() - Print dcs im stats
 * @prev_stats: dcs prev stats
 * @curr_stats: dcs curr stats
 *
 * Return: none
 */
void wlan_dcs_im_print_stats(wmi_host_dcs_im_tgt_stats_t *prev_stats,
			     wmi_host_dcs_im_tgt_stats_t *curr_stats);

/*
 * wlan_dcs_im_copy_stats() - Copy dcs im stats
 * @prev_stats: dcs prev stats
 * @curr_stats: dcs curr stats
 *
 * Return: none
 */
void wlan_dcs_im_copy_stats(wmi_host_dcs_im_tgt_stats_t *prev_stats,
			    wmi_host_dcs_im_tgt_stats_t *curr_stats);

/*
 * wlan_dcs_send_acs_request() - Send ACS request after DCS trigger
 * @vap: Pointer to the VAP structure
 * @allow_curchan: Flag to allow the current channel
 * @buffer: Bandwidth to avoid to the left and right of the operating channel
 *
 * Return:
 *       0: Success
 * -EINVAL: Failure
 */
int wlan_dcs_send_acs_request(struct ieee80211vap *vap,
                              bool allow_curchan,
                              uint32_t buffer);

/**
 * ol_ath_dcs_params:
 * Set/unset the DCS parameter.
 */
enum ol_ath_dcs_params {
	OL_ATH_DCS_PARAM_RANDOM_CHAN_EN,
	OL_ATH_DCS_PARAM_CSA_TBTT,
	OL_ATH_DCS_PARAM_BANDWIDTH_REDUCTION_CONTROL,
#ifdef CONFIG_AFC_SUPPORT
	OL_ATH_DCS_PARAM_AFC_CHAN_SEL_CONFIG,
#endif /* CONFIG_AFC_SUPPORT*/
	OL_ATH_DCS_PARAM_MAX,
};

/**
 * ol_ath_set_dcs_param:
 * Set miscellaneous DCS parameters.
 * @ic   : ic handle
 * @param: DCS parameter
 * @value: value to set
 *
 * Return:
 * None
 */
void ol_ath_set_dcs_param(struct ieee80211com *ic,
			  enum ol_ath_dcs_params param,
			  uint32_t value);

/**
 * ol_ath_dcs_chan_seg:
 * Different segments in the channel band.
 */
enum ol_ath_dcs_chan_seg {
	OL_ATH_DCS_SEG_PRI20       = 0x0001,
	OL_ATH_DCS_SEG_SEC20       = 0x0002,
	OL_ATH_DCS_SEG_SEC40_L     = 0x0004,
	OL_ATH_DCS_SEG_SEC40_U     = 0x0008,
	OL_ATH_DCS_SEG_SEC40       = 0x000C,
	OL_ATH_DCS_SEG_SEC80_L     = 0x0010,
	OL_ATH_DCS_SEG_SEC80_LU    = 0x0020,
	OL_ATH_DCS_SEG_SEC80_UL    = 0x0040,
	OL_ATH_DCS_SEG_SEC80_U     = 0x0080,
	OL_ATH_DCS_SEG_SEC80       = 0x00F0,
	OL_ATH_DCS_SEG_SEC160_L    = 0x0100,
	OL_ATH_DCS_SEG_SEC160_LU   = 0x0200,
	OL_ATH_DCS_SEG_SEC160_LUU  = 0x0400,
	OL_ATH_DCS_SEG_SEC160_LUUU = 0x0800,
	OL_ATH_DCS_SEG_SEC160_ULLL = 0x1000,
	OL_ATH_DCS_SEG_SEC160_ULL  = 0x2000,
	OL_ATH_DCS_SEG_SEC160_UL   = 0x4000,
	OL_ATH_DCS_SEG_SEC160_U    = 0x8000,
	OL_ATH_DCS_SEG_SEC160      = 0xFF00,
};

#define OL_ATH_DCS_GET_BITMAP_IDX(__awgn_info, __seg)                           \
	(((__awgn_info)->chan_bw_intf_bitmap) & OL_ATH_DCS_SEG_ ## __seg)

/* Default CSA TBTT count for CSA channel selection is 2 beacons */
#define DCS_CSA_TBTT_DEFAULT 2

#ifdef CONFIG_AFC_SUPPORT
/*
 * afc_chan_sel_cond_t:
 * AFC channel selection conditions
 */
enum dcs_afc_chan_sel_cond_t {
	DCS_AFC_CHAN_SEL_CUR_PWR_LT_AFC_PWR = BIT(0),
	DCS_AFC_CHAN_SEL_CUR_PWR_EQ_AFC_PWR = BIT(1),
	DCS_AFC_CHAN_SEL_CUR_PWR_GT_AFC_PWR = BIT(2),
	DCS_AFC_CHAN_SEL_ALL        = (DCS_AFC_CHAN_SEL_CUR_PWR_LT_AFC_PWR |
				       DCS_AFC_CHAN_SEL_CUR_PWR_EQ_AFC_PWR |
				       DCS_AFC_CHAN_SEL_CUR_PWR_GT_AFC_PWR),
};

/*
 * dcs_afc_trgr_delay_timer_fn:
 * Timer function to handle a delay in the AFC trigger if there are no VAPs
 * ready.
 *
 * @arg: Opaque handle to the scn pointer
 */
void dcs_afc_trgr_delay_timer_fn(void *arg);

/*
 * NOTE: Trigger delay timer will default to 70 seconds with two retries
 * totalling to 2mins 20s worst case for all the VAPs to come up.
 * From profiling, wideband radios are currently the platform which take the
 * longest to come up at 54 seconds. Adding a buffer period of 15 seconds
 * will ensure we will run AFC DCS only after all VAPs will come up.
 */
#define DCS_AFC_TRGR_DELAY_DEFAULT_TIME 70000
#define DCS_AFC_TRGR_DELAY_RETRY_CNT        2
#endif /* CONFIG_AFC_SUPPORT */
#endif /* OL_IF_DCS_H */
