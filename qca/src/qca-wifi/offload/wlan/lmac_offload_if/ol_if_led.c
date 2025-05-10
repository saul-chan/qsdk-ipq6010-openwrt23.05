/*
 * copyright (c) 2017-2022 Qualcomm Innovation Center, Inc.
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

#include <ol_if_led.h>
#include <wlan_gpio_tgt_api.h>
#include <linux/of_gpio.h>
#include "pld_common.h"

#if OL_ATH_SUPPORT_LED
static const
led_blink_rate ol_led_blink_rate_table[] = {
	{500, 130},
	{400, 100},
	{280, 70 },
	{240, 60 },
	{200, 50 },
	{160, 40 },
	{130, 30 },
	{100, 30 },
	{90, 20 },
	{80, 20 },
	{70, 20 },
	{60, 10 },
	{50, 10 },
	{40, 10 },
};

/**
 * ol_ath_led_poll_timed_out - led poll timeout handler
 *
 * @arg: Pointer to net80211 softc object
 *
 * This timeout handler function checks the state of
 * led and invokes event for led.
 *
 * Return: None
 */
static void ol_ath_led_poll_timed_out(void *arg)
{
	struct ol_ath_softc_net80211 *scn =
					(struct ol_ath_softc_net80211 *)arg;

	if (!scn || !scn->soc || !scn->soc->led_blink_rate_table) {
		qdf_err("error: on led poll timeout");
		return;
	}

	if (scn->scn_blinking != OL_BLINK_DONE)
		return;

	if ((lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCA8074) ||
	    (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCA8074V2) ||
	    (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCA9574) ||
	    (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCA5018) ||
	    (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCA5332) ||
	    (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCA6018)) {
		if( scn->scn_led_total_byte_cnt > scn->scn_led_byte_cnt)
			scn->scn_led_total_byte_cnt = 0;

		scn->scn_led_byte_cnt -= scn->scn_led_total_byte_cnt;
		scn->scn_led_total_byte_cnt += scn->scn_led_byte_cnt;
		ol_ath_led_event(scn, OL_ATH_LED_RX);
	} else {
		ol_ath_led_event(scn, OL_ATH_LED_POLL);
	}
}

/**
 * ol_ath_led_blink_timed_out - led blink timeout handler
 *
 * @arg: Pointer to net80211 softc object
 *
 * This timeout handler function is used to change the state
 * of the led between on and off with different blinking rate.
 *
 * Return: None
 */
static void ol_ath_led_blink_timed_out(void *arg)
{
	struct ol_ath_softc_net80211 *scn =
				(struct ol_ath_softc_net80211 *)arg;
	uint32_t target_type;
	struct wlan_objmgr_psoc *psoc;

	if (!scn->soc) {
	    return;
	}

	psoc = scn->soc->psoc_obj;
	if (!psoc) {
	    return;
	}

	if (!scn->soc->led_blink_rate_table ||
	    (lmac_get_tgt_type(psoc) == TARGET_TYPE_QCA8074) ||
	    (lmac_get_tgt_type(psoc) == TARGET_TYPE_QCA8074V2 && scn->scn_led_gpio == 0) ||
	    (lmac_get_tgt_type(psoc) == TARGET_TYPE_QCA6018) ||
	    (lmac_get_tgt_type(psoc) == TARGET_TYPE_QCN9224) ||
	    (lmac_get_tgt_type(psoc) == TARGET_TYPE_QCN9000)) {
		return;
	}

	target_type =  lmac_get_tgt_type(psoc);

	switch (scn->scn_blinking) {
		case OL_BLINK_ON_START:
			scn->scn_blinking = OL_BLINK_DONE;
#if OL_ATH_SUPPORT_LED_POLL
			qdf_timer_mod(&scn->scn_led_poll_timer,
				      LED_POLL_TIMER);
#endif
			break;
		case OL_BLINK_OFF_START:
			if ((target_type == TARGET_TYPE_QCA8074) ||
			    (target_type == TARGET_TYPE_QCA8074V2 && scn->scn_led_gpio == 0) ||
			    (target_type == TARGET_TYPE_QCA6018)) {
			} else if(target_type == TARGET_TYPE_QCA8074V2) {
				qal_vbus_gpio_set_value_cansleep(scn->scn_led_gpio, OL_LED_OFF);
			} else if (target_type == TARGET_TYPE_QCA9574 ||
				   target_type == TARGET_TYPE_QCA5018 ||
				   target_type == TARGET_TYPE_QCA5332) {
				ol_ath_set_led_on_off(scn, OL_LED_OFF);
			} else {
				tgt_gpio_output(psoc, scn->scn_led_gpio, 0);
			}
			scn->scn_blinking = OL_BLINK_ON_START;
			qdf_timer_mod(&scn->scn_led_blink_timer,
				      scn->scn_led_time_on);
			break;
		case OL_BLINK_STOP:
			if ((target_type == TARGET_TYPE_QCA8074) ||
			    (target_type == TARGET_TYPE_QCA8074V2 && scn->scn_led_gpio == 0) ||
			    (target_type == TARGET_TYPE_QCA6018)) {
			} else if(target_type == TARGET_TYPE_QCA8074V2) {
				qal_vbus_gpio_set_value_cansleep(scn->scn_led_gpio, OL_LED_ON);
			} else if (target_type == TARGET_TYPE_QCA9574 ||
				   target_type == TARGET_TYPE_QCA5018 ||
				   target_type == TARGET_TYPE_QCA5332) {
				ol_ath_set_led_on_off(scn, OL_LED_ON);
			} else {
				tgt_gpio_output(psoc, scn->scn_led_gpio, 1);
			}
			scn->scn_blinking = OL_BLINK_DONE;
			break;
		case OL_BLINK_DONE:
		default:
			break;
	}
}

/**
 * ol_ath_led_blink - configure the led gpio
 *
 * @scn: Pointer to net80211 softc object
 * @on: on duration
 * @off: off duration
 *
 * This function is used to configure the gpio for on and off
 * of the led.
 *
 * Return: None
 */
static void
ol_ath_led_blink(struct ol_ath_softc_net80211 *scn,
		 u_int32_t on,
		 u_int32_t off)
{
	uint32_t target_type;
	struct wlan_psoc_host_hal_reg_capabilities_ext *reg_cap;
	uint8_t pdev_idx;
	struct wlan_objmgr_psoc *psoc;

	psoc = scn->soc->psoc_obj;
	if(!psoc)
	   return;

	target_type =  lmac_get_tgt_type(psoc);
	pdev_idx = lmac_get_pdev_idx(scn->sc_pdev);
	reg_cap = ucfg_reg_get_hal_reg_cap(psoc);
	if (!reg_cap)
		return;

	if (target_type == TARGET_TYPE_QCN9000 || target_type == TARGET_TYPE_QCN9224)
		return;

	if ((target_type == TARGET_TYPE_QCA8074) ||
	    (target_type == TARGET_TYPE_QCA8074V2 && scn->scn_led_gpio == 0) ||
	    (target_type == TARGET_TYPE_QCA6018)) {
#ifdef QCA_SUPPORT_CP_STATS
		struct ieee80211com *ic = &scn->sc_ic;
		if(!pdev_cp_stats_ap_stats_tx_cal_enable_get(ic->ic_pdev_obj))
			return;
#endif

#if ATH_SUPPORT_LED_CONTROLLER
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
#ifndef CONFIG_X86
		if(reg_cap[pdev_idx].wireless_modes & WIRELESS_MODES_2G) {
			ipq_led_set_blink(HAWKEYE_2G_LED, on, off);
		} else  {
			ipq_led_set_blink(HAWKEYE_5G_LED, on, off);
		}
#endif
#endif
#endif
	} else if(target_type == TARGET_TYPE_QCA8074V2) {
		qal_vbus_gpio_set_value_cansleep(scn->scn_led_gpio, OL_LED_ON);
	} else if (target_type == TARGET_TYPE_QCA9574 ||
		   target_type == TARGET_TYPE_QCA5018 ||
		   target_type == TARGET_TYPE_QCA5332) {
		ol_ath_set_led_on_off(scn, OL_LED_ON);
	} else {
		tgt_gpio_output(psoc, scn->scn_led_gpio, 1);
	}
	scn->scn_led_time_on = on;
	if ((target_type == TARGET_TYPE_QCA8074) ||
	    (target_type == TARGET_TYPE_QCA8074V2 && scn->scn_led_gpio == 0) ||
	    (target_type == TARGET_TYPE_QCA6018)) {
#if OL_ATH_SUPPORT_LED_POLL
		scn->scn_blinking = OL_BLINK_DONE;
		qdf_timer_mod(&scn->scn_led_poll_timer, LED_POLL_TIMER);
#endif
	} else {
		scn->scn_blinking = OL_BLINK_OFF_START;
		qdf_timer_mod(&scn->scn_led_blink_timer, off);
	}
}

void
ol_ath_led_event(struct ol_ath_softc_net80211 *scn, OL_LED_EVENT event)
{
	u_int32_t led_last_time =
			CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP());
	u_int32_t map_idx;
	u_int32_t on, off;

	if (!scn || !(scn->soc->led_blink_rate_table)) {
		return;
	}

#if QCA_LTEU_SUPPORT
	if (wlan_psoc_nif_feat_cap_get(scn->soc->psoc_obj,
				       WLAN_SOC_F_LTEU_SUPPORT))
		return;
#endif
	/* Don't interrupt active blink */
	if (scn->scn_blinking != OL_BLINK_DONE)
		return;

	switch (event) {
		case OL_ATH_LED_TX:
		case OL_ATH_LED_RX:
		/* map_idx is calculated as follows.
		 * First convert bytes/ms to bits/s, ((bytes*8)/(ms*(1/1000)),
		 * which becomes ((bytes*8*1000)/ms).
		 * Then, convert bits/s to Mbps
		 * ((bytes*8*1000)/ms) * (1/(1000*1000)), this gives Mbps value.
		 * Taking x Mbps per entry from INI (to change blink rate for
		 * every x Mbps change in data rate), map_idx is calculated as,
		 * ((bytes*8*1000)/ms) * (1/(1000*1000)) * (1/x)
		 * which becomes (bytes/(ms*125*x)).
		 * Note: Default value of bw_per_idx in INI is 50Mbps, as before.
		 */
			map_idx = scn->scn_led_byte_cnt /
			((led_last_time + 1 - scn->scn_led_last_time) * 125 * scn->scn_led_bw_per_idx);

			scn->scn_led_last_time = led_last_time;
			scn->scn_led_byte_cnt = 0;
			if (map_idx < 0) {
				map_idx = 0;
			} else if (map_idx > scn->scn_led_max_blink_rate_idx) {
				map_idx = scn->scn_led_max_blink_rate_idx;
			}
			on = scn->soc->led_blink_rate_table[map_idx].time_on;
			off = scn->soc->led_blink_rate_table[map_idx].time_off;
			ol_ath_led_blink(scn, on, off);
			break;
		case OL_ATH_LED_POLL:
			ol_ath_led_blink(scn, 100, 500);
			break;
		default:
			break;
	}
}
qdf_export_symbol(ol_ath_led_event);

void ol_ath_led_init(struct ol_ath_soc_softc *soc,
		     struct ol_ath_softc_net80211 *scn,
		     uint32_t target_type)
{
	/* HAWKEYE-WAR for SOC emulation: gpio configuration is not
         * available for SOC Emulation hence call to gpio config
         * (WMI command for gpio config) is blocked
	 */
	struct wlan_objmgr_psoc *psoc;
	bool led_blink_support;
	struct wmi_unified *pdev_wmi_handle;
	struct wmi_led_blink_params params;

	psoc = scn->soc->psoc_obj;
	if (!psoc)
	    return;

	pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
	if (!pdev_wmi_handle)
		return;

	led_blink_support = cfg_get(psoc, CFG_TGT_LED_BLINK_SUPPORT);
	if (!led_blink_support) {
		return;
	}
	scn->scn_led_bw_per_idx = cfg_get(psoc, CFG_TGT_LED_BW_PER_IDX);

	if ((target_type == TARGET_TYPE_QCA8074) ||
	    (target_type == TARGET_TYPE_QCA8074V2) ||
	    (target_type == TARGET_TYPE_QCA9574) ||
	    (target_type == TARGET_TYPE_QCA6018) ||
	    (target_type == TARGET_TYPE_QCA5018) ||
	    (target_type == TARGET_TYPE_QCN6122) ||
	    (target_type == TARGET_TYPE_QCA5332) ||
	    (target_type == TARGET_TYPE_QCN9160) ||
	    (target_type == TARGET_TYPE_QCN9224) ||
	    (target_type == TARGET_TYPE_QCN9000) ||
	    (target_type == TARGET_TYPE_QCN6432)) {
	} else {
		tgt_gpio_config(psoc, scn->scn_led_gpio, 0, 0, 0, 0, 0, 0);
		tgt_gpio_output(psoc, scn->scn_led_gpio, 1);
	}
	qdf_timer_init(soc->qdf_dev, &scn->scn_led_blink_timer,
		       ol_ath_led_blink_timed_out, scn,
		       QDF_TIMER_TYPE_WAKE_APPS);
	qdf_timer_init(soc->qdf_dev, &scn->scn_led_poll_timer,
		       ol_ath_led_poll_timed_out,
		       scn, QDF_TIMER_TYPE_WAKE_APPS);
	scn->scn_blinking = OL_BLINK_DONE;
	scn->scn_led_byte_cnt = 0;
	scn->scn_led_total_byte_cnt = 0;
	scn->scn_led_last_time = CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP());
	soc->led_blink_rate_table = ol_led_blink_rate_table;
	scn->scn_led_max_blink_rate_idx =
				ARRAY_LENGTH(ol_led_blink_rate_table) - 1;

	/* Send the WMI command to transfer LED blink rate table to FW
	 * for those devices (PCI devices) for whom support is announced
	 * from FW that LED blink mechanism is taken care of by the FW.
	 */
	if (wmi_service_enabled(pdev_wmi_handle,
				wmi_service_pcie_data_rate_led_blink_support)) {

		params.pdev_id = lmac_get_pdev_idx(scn->sc_pdev);
		params.blink_enable_flag = led_blink_support;
		params.bw_per_index = scn->scn_led_bw_per_idx;
		params.num_blink_rate_table_entries = ARRAY_LENGTH(ol_led_blink_rate_table);
		params.led_blink_rate_table = ol_led_blink_rate_table;

		wmi_unified_led_blink_rate_table_cmd_send(pdev_wmi_handle, &params);
        }
}

void ol_ath_clear_led_params(struct ol_ath_softc_net80211 *scn)
{
        OS_CANCEL_TIMER(&scn->scn_led_blink_timer);
        OS_CANCEL_TIMER(&scn->scn_led_poll_timer);
        scn->scn_blinking = OL_BLINK_STOP;
        if (scn->soc->led_blink_rate_table) {
            OS_SET_TIMER(&scn->scn_led_blink_timer, 10);
        }
}

void ol_ath_set_led_on_off(struct ol_ath_softc_net80211 *scn,
			   OL_LED_STATUS value)
{
	pld_set_led_gpio(scn->scn_led_gpio, value, scn->scn_led_gpio_flags);
}
#endif
