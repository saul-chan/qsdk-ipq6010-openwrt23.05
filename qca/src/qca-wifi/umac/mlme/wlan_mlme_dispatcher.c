/*
 * Copyright (c) 2017-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 */

#include <ieee80211_var.h>
#include <ieee80211_channel.h>
#include <ieee80211_objmgr_priv.h>
#include <ieee80211_mlme_priv.h>
#include <reg_services_public_struct.h>
#include <ieee80211_rateset.h>
#include <ieee80211_node_priv.h>
#include <wlan_mlme_dispatcher.h>
#include <wlan_mlme_dbg.h>
#include <cfg_ucfg_api.h>
#include <ieee80211_ucfg.h>
#include <cdp_txrx_stats_struct.h>
#include <wlan_vdev_mgr_utils_api.h>
#include <dp_txrx.h>
#include <ol_if_athvar.h>
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include <osif_nss_wifiol_if.h>
#include <osif_nss_wifiol_vdev_if.h>
#endif
#include <wlan_mlme_vdev_mgmt_ops.h>
#if ATH_SUPPORT_WRAP
#include <dp_wrap.h>
#endif
#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_peer.h>
#include <wlan_mlo_mgr_ap.h>
#endif
#include <wlan_utility.h>
#if DBDC_REPEATER_SUPPORT
#include <qca_multi_link.h>
#endif
#include <wlan_mlme_cmn.h>
#include <telemetry_agent_wifi_driver_if.h>
#include <wlan_telemetry_agent.h>

#define MAX_L2UF_RETIRES 10

int16_t wlan_vdev_get_chan_freq(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211com *ic = NULL;

    if (vdev == NULL) {
       return -1;
    }
    ic = wlan_vdev_get_ic(vdev);

    if(ic == NULL) {
       return -1;
    }

    return ieee80211_chan2freq(ic, ic->ic_curchan);
}
qdf_export_symbol(wlan_vdev_get_chan_freq);

int16_t wlan_vdev_get_chan_freq_seg2(struct wlan_objmgr_vdev *vdev)
{
   struct ieee80211com *ic;
   struct ieee80211vap *vap;

   if (!vdev)
      return -EINVAL;

   wlan_vdev_obj_lock(vdev);
   ic = wlan_vdev_get_ic(vdev);
   vap = wlan_vdev_get_vap(vdev);
   wlan_vdev_obj_unlock(vdev);

   if(!ic || !vap)
      return -EINVAL;

   if (ieee80211_is_phymode_8080(vap->iv_cur_mode))
      return vap->iv_bsschan->ic_vhtop_freq_seg2;
   else
      return 0;
}

qdf_export_symbol(wlan_vdev_get_chan_freq_seg2);

uint8_t chwidth2convchwidth[IEEE80211_CWM_WIDTH_MAX] = {
    CH_WIDTH_20MHZ,    /*IEEE80211_CWM_WIDTH20,    */
    CH_WIDTH_40MHZ,    /*IEEE80211_CWM_WIDTH40,    */
    CH_WIDTH_80MHZ,    /*IEEE80211_CWM_WIDTH80,    */
    CH_WIDTH_160MHZ,   /*IEEE80211_CWM_WIDTH160,   */
    CH_WIDTH_80P80MHZ, /*IEEE80211_CWM_WIDTH80_80, */
#ifdef WLAN_FEATURE_11BE
    CH_WIDTH_320MHZ, /*IEEE80211_CWM_WIDTH320, */
#endif /* WLAN_FEATURE_11BE */
};

enum phy_ch_width ieee80211_get_phy_ch_width(enum ieee80211_cwm_width chwidth)
{
    if (chwidth < IEEE80211_CWM_WIDTH_MAX)
        return chwidth2convchwidth[chwidth];

    return CH_WIDTH_INVALID;
}

enum phy_ch_width wlan_vdev_get_ch_width(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211com *ic = NULL;
    enum ieee80211_cwm_width chwidth;

    if (vdev == NULL) {
       return CH_WIDTH_INVALID;
    }

    ic = wlan_vdev_get_ic(vdev);

    if(ic == NULL) {
       return CH_WIDTH_INVALID;
    }

    chwidth = ic->ic_cwm_get_width(ic);

    if (chwidth < IEEE80211_CWM_WIDTH_MAX)
        return chwidth2convchwidth[chwidth];

    return CH_WIDTH_INVALID;
}
qdf_export_symbol(wlan_vdev_get_ch_width);




uint8_t phymode2convphymode[IEEE80211_MODE_MAX] = {
    WLAN_PHYMODE_AUTO,               /* IEEE80211_MODE_AUTO */
    WLAN_PHYMODE_11A,                /* IEEE80211_MODE_11A */
    WLAN_PHYMODE_11B,                /* IEEE80211_MODE_11B */
    WLAN_PHYMODE_11G,                /* IEEE80211_MODE_11G */
    0xff,                            /* IEEE80211_MODE_FH */
    0xff,                            /* IEEE80211_MODE_TURBO_A */
    0xff,                            /* IEEE80211_MODE_TURBO_G */
    WLAN_PHYMODE_11NA_HT20,          /* IEEE80211_MODE_11NA_HT20 */
    WLAN_PHYMODE_11NG_HT20,          /* IEEE80211_MODE_11NG_HT20 */
    WLAN_PHYMODE_11NA_HT40,          /* IEEE80211_MODE_11NA_HT40PLUS */
    WLAN_PHYMODE_11NA_HT40,          /* IEEE80211_MODE_11NA_HT40MINUS */
    WLAN_PHYMODE_11NG_HT40PLUS,      /* IEEE80211_MODE_11NG_HT40PLUS */
    WLAN_PHYMODE_11NG_HT40MINUS,     /* IEEE80211_MODE_11NG_HT40MINUS */
    WLAN_PHYMODE_11NG_HT40,          /* IEEE80211_MODE_11NG_HT40 */
    WLAN_PHYMODE_11NA_HT40,          /* IEEE80211_MODE_11NA_HT40 */
    WLAN_PHYMODE_11AC_VHT20,         /* IEEE80211_MODE_11AC_VHT20 */
    WLAN_PHYMODE_11AC_VHT40,         /* IEEE80211_MODE_11AC_VHT40PLUS */
    WLAN_PHYMODE_11AC_VHT40,         /* IEEE80211_MODE_11AC_VHT40MINUS */
    WLAN_PHYMODE_11AC_VHT40,         /* IEEE80211_MODE_11AC_VHT40 */
    WLAN_PHYMODE_11AC_VHT80,         /* IEEE80211_MODE_11AC_VHT80 */
    WLAN_PHYMODE_11AC_VHT160,        /* IEEE80211_MODE_11AC_VHT160 */
    WLAN_PHYMODE_11AC_VHT80_80,      /* IEEE80211_MODE_11AC_VHT80_80 */
    WLAN_PHYMODE_11AXA_HE20,         /* IEEE80211_MODE_11AXA_HE20 */
    WLAN_PHYMODE_11AXG_HE20,         /* IEEE80211_MODE_11AXG_HE20 */
    WLAN_PHYMODE_11AXA_HE40,         /* IEEE80211_MODE_11AXA_HE40PLUS */
    WLAN_PHYMODE_11AXA_HE40,         /* IEEE80211_MODE_11AXA_HE40MINUS */
    WLAN_PHYMODE_11AXG_HE40PLUS,     /* IEEE80211_MODE_11AXG_HE40PLUS */
    WLAN_PHYMODE_11AXG_HE40MINUS,    /* IEEE80211_MODE_11AXG_HE40MINUS */
    WLAN_PHYMODE_11AXA_HE40,         /* IEEE80211_MODE_11AXA_HE40 */
    WLAN_PHYMODE_11AXG_HE40,         /* IEEE80211_MODE_11AXG_HE40 */
    WLAN_PHYMODE_11AXA_HE80,         /* IEEE80211_MODE_11AXA_HE80 */
    WLAN_PHYMODE_11AXA_HE160,        /* IEEE80211_MODE_11AXA_HE160 */
    WLAN_PHYMODE_11AXA_HE80_80,      /* IEEE80211_MODE_11AXA_HE80_80 */
#ifdef WLAN_FEATURE_11BE
    WLAN_PHYMODE_11BEA_EHT20,        /* IEEE80211_MODE_11BEA_EHT20 */
    WLAN_PHYMODE_11BEG_EHT20,        /* IEEE80211_MODE_11BEG_EHT20 */
    WLAN_PHYMODE_11BEA_EHT40,        /* IEEE80211_MODE_11BEA_EHT40PLUS */
    WLAN_PHYMODE_11BEA_EHT40,        /* IEEE80211_MODE_11BEA_EHT40MINUS */
    WLAN_PHYMODE_11BEG_EHT40PLUS,    /* IEEE80211_MODE_11BEG_EHT40PLUS */
    WLAN_PHYMODE_11BEG_EHT40MINUS,   /* IEEE80211_MODE_11BEG_EHT40MINUS */
    WLAN_PHYMODE_11BEA_EHT40,        /* IEEE80211_MODE_11BEA_EHT40 */
    WLAN_PHYMODE_11BEG_EHT40,        /* IEEE80211_MODE_11BEG_EHT40 */
    WLAN_PHYMODE_11BEA_EHT80,        /* IEEE80211_MODE_11BEA_EHT80 */
    WLAN_PHYMODE_11BEA_EHT160,       /* IEEE80211_MODE_11BEA_EHT160 */
    WLAN_PHYMODE_11BEA_EHT320,       /* IEEE80211_MODE_11BEA_EHT320 */
#endif /* WLAN_FEATURE_11BE */
};
qdf_export_symbol(phymode2convphymode);

enum wlan_phymode wlan_vdev_get_phymode(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211vap *vap = NULL;
    enum ieee80211_phymode phy_mode;

    if (vdev == NULL) {
       return IEEE80211_MODE_AUTO;
    }

    wlan_vdev_obj_lock(vdev);
    vap = wlan_vdev_get_vap(vdev);
    wlan_vdev_obj_unlock(vdev);

    if(vap == NULL) {
       return IEEE80211_MODE_AUTO;
    }
    phy_mode = vap->iv_des_mode;

    return phymode2convphymode[phy_mode];
}
qdf_export_symbol(wlan_vdev_get_phymode);

int wlan_vdev_get_sec20chan_freq_mhz(struct wlan_objmgr_vdev *vdev,
        uint16_t *sec20chan_freq)
{
    struct ieee80211com *ic = NULL;
    int offset = 0;

    if ((vdev == NULL) || (sec20chan_freq == NULL)) {
       return -1;
    }

    ic = wlan_vdev_get_ic(vdev);

    if ((ic == NULL) || (!IEEE80211_IS_CHAN_VALID(ic->ic_curchan))) {
       return -1;
    }

    offset = ieee80211_secondary20_channel_offset(ic->ic_curchan);

    if (!offset)
    {
        /* If no secondary 20 MHz is present, then we do not treat it as a
         * failure. Rather, we indicate that secondary 20 MHz is NA by setting
         * sec20chan_freq to 0.
         */
        *sec20chan_freq = 0;
    } else {
        *sec20chan_freq = (ic->ic_curchan->ic_freq + (20 * offset));
    }

    return 0;
}
qdf_export_symbol(wlan_vdev_get_sec20chan_freq_mhz);

uint32_t wlan_pdev_in_gmode(struct wlan_objmgr_pdev *pdev)
{
    struct ieee80211com *ic;
    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (ic)
        return IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan);
    else
        return 0;
}
qdf_export_symbol(wlan_pdev_in_gmode);

uint32_t wlan_pdev_in_amode(struct wlan_objmgr_pdev *pdev)
{
    struct ieee80211com *ic;
    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (ic)
        return IEEE80211_IS_CHAN_5GHZ_6GHZ(ic->ic_curchan);
    else
        return 0;
}
qdf_export_symbol(wlan_pdev_in_amode);

int wlan_pdev_get_esp_info(struct wlan_objmgr_pdev *pdev, wlan_esp_data *esp_info)
{
#if QCN_ESP_IE
    struct ieee80211com *ic;
    wlan_ba_window_size ba_window_size[] =
        {ba_window_not_used, ba_window_size_2,
         ba_window_size_4, ba_window_size_6,
         ba_window_size_8, ba_window_size_16,
         ba_window_size_32, ba_window_size_64};


    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic)
        return -EINVAL;

    /* BA Window size as entered by the user */
    if (ic->ic_esp_ba_window) {
        ic->ic_esp_ba_window =
            ba_window_size[ic->ic_esp_ba_window];
        esp_info->per_ac[WME_AC_BE].ba_window_size =
            ic->ic_esp_ba_window;
    } else {
        /* BA Window Size of 16 */
        esp_info->per_ac[WME_AC_BE].ba_window_size =
            ba_window_size_16;
    }

    /* Estimated Airtime Fraction in native format */
    if (ic->ic_esp_air_time_fraction) {
        esp_info->per_ac[WME_AC_BE].est_air_time_fraction =
            (ic->ic_esp_air_time_fraction * 100) / 255;
    } else {
        esp_info->per_ac[WME_AC_BE].est_air_time_fraction =
            ic->ic_fw_esp_air_time;
    }

    if (ic->ic_esp_ppdu_duration) {
        /* PPDU Duration in native format */
        esp_info->per_ac[WME_AC_BE].data_ppdu_dur_target =
            ic->ic_esp_ppdu_duration * MAP_PPDU_DURATION_UNITS;
    } else {
        /* Default : 250us PPDU Duration in native format */
        esp_info->per_ac[WME_AC_BE].data_ppdu_dur_target =
            MAP_DEFAULT_PPDU_DURATION * MAP_PPDU_DURATION_UNITS;
    }
#endif
    return EOK;
}

int wlan_pdev_get_multi_ap_opclass(struct wlan_objmgr_pdev *pdev,
                                   struct wlan_radio_basic_capabilities *hwcap,
                                   struct wlan_op_chan *op_chan,
                                   struct wlan_op_class *op_class,
                                   bool dfs_required)
{
    return regdmn_get_map_opclass(pdev, hwcap, op_chan, op_class, true, dfs_required);
}

void wlan_pdev_get_opclass_for_cur_hwmode(struct wlan_objmgr_pdev *pdev,
                                         wlan_radio_basic_capabilities *apcap)
{
        return regdmn_get_opclass_for_hwmode(pdev, apcap, true);
}

QDF_STATUS wlan_pdev_get_5ghz_chan_info(struct wlan_objmgr_pdev *pdev,
                                        wlan_radio_chan_list_info *cac_tlv)
{
    return ieee80211_list_5ghz_channel_info(pdev, cac_tlv);
}

void wlan_pdev_get_supp_opclass_list(struct wlan_objmgr_pdev *pdev,
                                     uint8_t *opclass_list,
                                     uint8_t *num_supp_op_class,
                                     bool global_tbl_lookup)
{
   regdmn_get_supp_opclass_list(pdev,
                                opclass_list,
                                num_supp_op_class,
                                global_tbl_lookup);
}

void wlan_vdev_get_curr_chan_and_opclass(struct wlan_objmgr_vdev *vdev,
                                         uint8_t *chan_num,
                                         uint8_t *opclass)
{
   regdmn_get_curr_chan_and_opclass(vdev, chan_num, opclass);
}

int wlan_pdev_get_current_chan_txpower(struct wlan_objmgr_pdev *pdev)
{
   return regdmn_get_current_chan_txpower(pdev);
}

uint32_t wlan_node_get_peer_chwidth(struct wlan_objmgr_peer *peer)
{
     wlan_node_t ni;

     if(peer == NULL) {
         qdf_print("%s:PEER is NULL ",__func__);
         return 0;
     }

     ni = (wlan_node_t)wlan_peer_get_mlme_ext_obj(peer);

     return (ni ? ni->ni_chwidth : IEEE80211_CWM_WIDTHINVALID);
}
qdf_export_symbol(wlan_node_get_peer_chwidth);

void wlan_node_get_peer_phy_info(struct wlan_objmgr_peer *peer, uint8_t *rxstreams, uint8_t *streams, uint8_t *cap, uint32_t *mode, uint32_t *ext_mode)
{
     wlan_node_t ni;

     if(peer == NULL) {
         qdf_print("%s:PEER is NULL ",__func__);
         return;
     }

     ni = (wlan_node_t)wlan_peer_get_mlme_ext_obj(peer);

     *rxstreams = ni->ni_rxstreams;
     *streams =   ni->ni_streams;
     *mode = ni->ni_flags;
     *ext_mode = ni->ni_ext_flags;

     *cap = 1;
}
qdf_export_symbol(wlan_node_get_peer_phy_info);

#ifdef WLAN_FEATURE_11BE
uint8_t ieee80211_get_eht_max_mcs_from_map(uint32_t eht_rxmcs_map, uint32_t eht_txmcs_map)
{
    uint8_t rx_max_mcs;
    uint8_t tx_max_mcs;

    if (IEEE80211_GET_BITS(eht_rxmcs_map, EHT_RXMCS_MAP_MCS_12_13)) {
        rx_max_mcs = 13;
    } else if (IEEE80211_GET_BITS(eht_rxmcs_map, EHT_RXMCS_MAP_MCS_10_11)) {
        rx_max_mcs = 11;
    } else if (IEEE80211_GET_BITS(eht_rxmcs_map, EHT_RXMCS_MAP_MCS_8_9)) {
        rx_max_mcs = 9;
    } else if (IEEE80211_GET_BITS(eht_rxmcs_map, EHT_RXMCS_MAP_MCS_0_7)) {
        rx_max_mcs = 7;
    } else {
        rx_max_mcs = 0;
    }
    if (IEEE80211_GET_BITS(eht_txmcs_map, EHT_TXMCS_MAP_MCS_12_13)) {
        tx_max_mcs = 13;
    } else if (IEEE80211_GET_BITS(eht_txmcs_map, EHT_TXMCS_MAP_MCS_10_11)) {
        tx_max_mcs = 11;
    } else if (IEEE80211_GET_BITS(eht_txmcs_map, EHT_TXMCS_MAP_MCS_8_9)) {
        tx_max_mcs = 9;
    } else if (IEEE80211_GET_BITS(eht_txmcs_map, EHT_TXMCS_MAP_MCS_0_7)) {
        tx_max_mcs = 7;
    } else {
        tx_max_mcs = 0;
    }

    if (tx_max_mcs > rx_max_mcs) {
        return tx_max_mcs;
    } else {
        return rx_max_mcs;
    }
}

#define EHT_MAX_NSS 8
uint8_t ieee80211_get_eht_max_nss_from_map(uint32_t eht_rxmcs_map, uint32_t eht_txmcs_map)
{

    uint8_t rx_max_nss_12_13 = 0;
    uint8_t rx_max_nss_10_11 = 0;
    uint8_t rx_max_nss_8_9 = 0;
    uint8_t rx_max_nss_0_7 = 0;
    uint8_t rx_max_nss = 0;
    uint8_t tx_max_nss_12_13 = 0;
    uint8_t tx_max_nss_10_11 = 0;
    uint8_t tx_max_nss_8_9 = 0;
    uint8_t tx_max_nss_0_7 = 0;
    uint8_t tx_max_nss = 0;

    if (IEEE80211_GET_BITS(eht_rxmcs_map, EHT_RXMCS_MAP_MCS_12_13)) {
        rx_max_nss_12_13 = IEEE80211_GET_BITS(eht_rxmcs_map, EHT_RXMCS_MAP_MCS_12_13);
    }
    if (IEEE80211_GET_BITS(eht_rxmcs_map, EHT_RXMCS_MAP_MCS_10_11)) {
        rx_max_nss_10_11 = IEEE80211_GET_BITS(eht_rxmcs_map, EHT_RXMCS_MAP_MCS_10_11);
    }
    if (IEEE80211_GET_BITS(eht_rxmcs_map, EHT_RXMCS_MAP_MCS_8_9)) {
        rx_max_nss_8_9 = IEEE80211_GET_BITS(eht_rxmcs_map, EHT_RXMCS_MAP_MCS_8_9);
    }
    if (IEEE80211_GET_BITS(eht_rxmcs_map, EHT_RXMCS_MAP_MCS_0_7)) {
        rx_max_nss_0_7 = IEEE80211_GET_BITS(eht_rxmcs_map, EHT_RXMCS_MAP_MCS_0_7);
    }

    rx_max_nss = MAX(rx_max_nss_12_13, rx_max_nss_10_11);
    rx_max_nss = MAX(rx_max_nss, rx_max_nss_8_9);
    rx_max_nss = MAX(rx_max_nss, rx_max_nss_0_7);

    if (rx_max_nss > EHT_MAX_NSS)
        rx_max_nss = 0;

    if (IEEE80211_GET_BITS(eht_txmcs_map, EHT_TXMCS_MAP_MCS_12_13)) {
        tx_max_nss_12_13 = IEEE80211_GET_BITS(eht_txmcs_map, EHT_TXMCS_MAP_MCS_12_13);
    }
    if (IEEE80211_GET_BITS(eht_txmcs_map, EHT_TXMCS_MAP_MCS_10_11)) {
        tx_max_nss_10_11 = IEEE80211_GET_BITS(eht_txmcs_map, EHT_TXMCS_MAP_MCS_10_11);
    }
    if (IEEE80211_GET_BITS(eht_txmcs_map, EHT_TXMCS_MAP_MCS_8_9)) {
        tx_max_nss_8_9 = IEEE80211_GET_BITS(eht_txmcs_map, EHT_TXMCS_MAP_MCS_8_9);
    }
    if (IEEE80211_GET_BITS(eht_txmcs_map, EHT_TXMCS_MAP_MCS_0_7)) {
        tx_max_nss_0_7 = IEEE80211_GET_BITS(eht_txmcs_map, EHT_TXMCS_MAP_MCS_0_7);
    }

    tx_max_nss = MAX(tx_max_nss_12_13, tx_max_nss_10_11);
    tx_max_nss = MAX(tx_max_nss, tx_max_nss_8_9);
    tx_max_nss = MAX(tx_max_nss, tx_max_nss_0_7);

    if (tx_max_nss > EHT_MAX_NSS)
        tx_max_nss = 0;

    if (tx_max_nss < rx_max_nss) {
        return tx_max_nss;
    } else {
        return rx_max_nss;
    }
}
#endif

#if QCA_SUPPORT_SON
int wlan_vdev_local_disassoc(struct wlan_objmgr_vdev *vdev,
			     struct wlan_objmgr_peer *peer)
{
	u_int8_t macaddr[QDF_MAC_ADDR_SIZE];
	struct ieee80211_node *ni = NULL;
	wlan_if_t vap = NULL;
#ifdef WLAN_FEATURE_11BE_MLO
	u_int8_t mldaddr[QDF_MAC_ADDR_SIZE] = {0};
#endif

	vap = wlan_vdev_get_vap(vdev);
	if (!vap)
		return -EINVAL;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (!ni) {
		return -EINVAL;
	}

	wlan_peer_obj_lock(peer);
	qdf_mem_copy(macaddr, wlan_peer_get_macaddr(peer), QDF_MAC_ADDR_SIZE);
#ifdef WLAN_FEATURE_11BE_MLO
	if ((ni->peer_obj->mldaddr) &&
	    (IEEE80211_ADDR_IS_VALID(&ni->peer_obj->mldaddr)))
		qdf_mem_copy(mldaddr, wlan_peer_mlme_get_mldaddr(peer), QDF_MAC_ADDR_SIZE);
#endif /*WLAN_FEATURE_11BE_MLO*/
	wlan_peer_obj_unlock(peer);
#ifdef WLAN_FEATURE_11BE_MLO
        if (wlan_peer_is_mlo(ni->peer_obj)) {
            wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
        }
#endif
	IEEE80211_NODE_LEAVE(ni);
#ifdef WLAN_FEATURE_11BE_MLO
	IEEE80211_DELIVER_EVENT_MLME_DISASSOC_COMPLETE(vap, macaddr,
					IEEE80211_REASON_LOCAL,
					IEEE80211_STATUS_SUCCESS, mldaddr);
#else
	IEEE80211_DELIVER_EVENT_MLME_DISASSOC_COMPLETE(vap, macaddr,
					IEEE80211_REASON_LOCAL,
					IEEE80211_STATUS_SUCCESS, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
	return EOK;
}

/**
 * @brief Get the maximum MCS supported based on PHY mode and rate
 *        information provided
 *
 * @param [in] phymode  the PHY mode the VAP/client is operating on
 * @param [in] rx_vht_mcs_map  the VHT RX rate map if supported
 * @param [in] tx_vht_mcs_map  the VHT TX rate map if supported
 * @param [in] htrates  the HT rates supported
 * @param [in] basic_rates  all other rates supported
 *
 * @return the maximum MCS supported on success; otherwise return -1
 */

int wlan_node_get_max_MCS(enum ieee80211_phymode phymode,
#ifdef WLAN_FEATURE_11BE
			  uint32_t *ehtcap_rxmcsnssmap,
			  uint32_t *ehtcap_txmcsnssmap,
#endif /* WLAN_FEATURE_11BE */
			  u_int16_t *hecap_rxmcsnssmap, u_int16_t *hecap_txmcsnssmap,
			  u_int16_t rx_vht_mcs_map, u_int16_t tx_vht_mcs_map,
			  const struct ieee80211_rateset *htrates,
			  const struct ieee80211_rateset *basic_rates)
{
	u_int8_t rx_max_MCS, tx_max_MCS, max_MCS;
	switch (phymode) {
#ifdef WLAN_FEATURE_11BE
	case IEEE80211_MODE_11BEA_EHT20:
	case IEEE80211_MODE_11BEG_EHT20:
	case IEEE80211_MODE_11BEA_EHT40PLUS:
	case IEEE80211_MODE_11BEA_EHT40MINUS:
	case IEEE80211_MODE_11BEG_EHT40PLUS:
	case IEEE80211_MODE_11BEG_EHT40MINUS:
	case IEEE80211_MODE_11BEA_EHT40:
	case IEEE80211_MODE_11BEG_EHT40:
	case IEEE80211_MODE_11BEA_EHT80:
            return ieee80211_get_eht_max_mcs_from_map(ehtcap_rxmcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX_80], ehtcap_txmcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX_80]);
	case IEEE80211_MODE_11BEA_EHT160:
            return ieee80211_get_eht_max_mcs_from_map(ehtcap_rxmcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX_160], ehtcap_txmcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX_160]);
	case IEEE80211_MODE_11BEA_EHT320:
            return ieee80211_get_eht_max_mcs_from_map(ehtcap_rxmcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX_320], ehtcap_txmcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX_320]);
#endif /* WLAN_FEATURE_11BE */
	case IEEE80211_MODE_11AXA_HE20:
	case IEEE80211_MODE_11AXG_HE20:
	case IEEE80211_MODE_11AXA_HE40PLUS:
	case IEEE80211_MODE_11AXA_HE40MINUS:
	case IEEE80211_MODE_11AXG_HE40PLUS:
	case IEEE80211_MODE_11AXG_HE40MINUS:
	case IEEE80211_MODE_11AXA_HE40:
	case IEEE80211_MODE_11AXG_HE40:
	case IEEE80211_MODE_11AXA_HE80:
		if (hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80] &&
			hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80]) {
			rx_max_MCS = hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80] & 0x03;
			tx_max_MCS = hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80] & 0x03;
			max_MCS = rx_max_MCS < tx_max_MCS ? rx_max_MCS : tx_max_MCS;
			if (max_MCS < 0x03) {
				return 7 + 2*max_MCS;
			}
		}
	case IEEE80211_MODE_11AXA_HE160:
		if (hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_160] &&
			hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_160]) {
			rx_max_MCS = hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_160] & 0x03;
			tx_max_MCS = hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_160] & 0x03;
			max_MCS = rx_max_MCS < tx_max_MCS ? rx_max_MCS : tx_max_MCS;
			if (max_MCS < 0x03) {
				return 7 + 2*max_MCS;
			}
		}
	case IEEE80211_MODE_11AXA_HE80_80:
		if (hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80_80] &&
			hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80_80]) {
			rx_max_MCS = hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80_80] & 0x03;
			tx_max_MCS = hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80_80] & 0x03;
			max_MCS = rx_max_MCS < tx_max_MCS ? rx_max_MCS : tx_max_MCS;
			if (max_MCS < 0x03) {
				return 7 + 2*max_MCS;
			}
		}
	case IEEE80211_MODE_11AC_VHT20:
	case IEEE80211_MODE_11AC_VHT40PLUS:
	case IEEE80211_MODE_11AC_VHT40MINUS:
	case IEEE80211_MODE_11AC_VHT40:
	case IEEE80211_MODE_11AC_VHT80:
	case IEEE80211_MODE_11AC_VHT160:
	case IEEE80211_MODE_11AC_VHT80_80:
		if (rx_vht_mcs_map && tx_vht_mcs_map) {
		/* Refer to IEEE P802.11ac/D7.0 Figure 8-401bs for VHT MCS Map definition */
			rx_max_MCS = rx_vht_mcs_map & 0x03;
			tx_max_MCS = tx_vht_mcs_map & 0x03;
			max_MCS = rx_max_MCS < tx_max_MCS ? rx_max_MCS : tx_max_MCS;
			if (max_MCS < 0x03) {
				return 7 + max_MCS;
			}
		}
		/* Invalid 11ac MCS, fallback to report 11n MCS */
	case IEEE80211_MODE_11NA_HT20:
	case IEEE80211_MODE_11NG_HT20:
	case IEEE80211_MODE_11NA_HT40PLUS:
	case IEEE80211_MODE_11NA_HT40MINUS:
	case IEEE80211_MODE_11NG_HT40PLUS:
	case IEEE80211_MODE_11NG_HT40MINUS:
	case IEEE80211_MODE_11NG_HT40:
	case IEEE80211_MODE_11NA_HT40:
		if (htrates && htrates->rs_nrates && (htrates->rs_nrates <= IEEE80211_RATE_MAXSIZE)) {
			return htrates->rs_rates[htrates->rs_nrates - 1];
		}
		/* Invalid 11n MCS, fallback to basic rates */
	default:
		if (basic_rates && basic_rates->rs_nrates && (basic_rates->rs_nrates <= IEEE80211_RATE_MAXSIZE)) {
			return basic_rates->rs_rates[basic_rates->rs_nrates - 1] & IEEE80211_RATE_VAL;
		}
	}

	return -1;
}

/**
 * @brief Get the maximum MCS supported by the client
 *
 * @param [in] ni  the STA to check for maximum MCS supported
 *
 * @return the maximum MCS supported by this client on success;
 *         otherwise return -1
 */
int wlan_peer_get_node_max_MCS(struct wlan_objmgr_peer *peer)
{
	struct ieee80211_node *ni = NULL;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if( !ni)
		return -EINVAL;

	return wlan_node_get_max_MCS(ni->ni_phymode,
#ifdef WLAN_FEATURE_11BE
				     ni->ni_eht.ehtcap_rxmcsnssmap,
				     ni->ni_eht.ehtcap_txmcsnssmap,
#endif /* WLAN_FEATURE_11BE */
				     ni->ni_he.hecap_rxmcsnssmap,
				     ni->ni_he.hecap_txmcsnssmap,
				     ni->ni_rx_vhtrates, ni->ni_tx_vhtrates,
				     &ni->ni_htrates, &ni->ni_rates);
}

int wlan_vdev_get_node_info(struct wlan_objmgr_vdev *vdev,
			    u_int8_t *mac, wlan_node_info *info)
{
	struct ieee80211_node *ni = NULL;
	wlan_if_t vap = NULL;
	struct ieee80211com *ic = NULL;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct wlan_objmgr_peer *peer = NULL;
	uint16_t he_rxmcsnssmap[HEHANDLE_CAP_TXRX_MCS_NSS_SIZE];
	uint16_t he_txmcsnssmap[HEHANDLE_CAP_TXRX_MCS_NSS_SIZE];
#ifdef WLAN_FEATURE_11BE
	uint32_t eht_rxmcsnssmap[EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE];
	uint32_t eht_txmcsnssmap[EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE];
#endif /* WLAN_FEATURE_11BE */
	int max_MCS = 0;
	uint8_t pdev_id;

	if (WLAN_ADDR_EQ(wlan_vdev_mlme_get_macaddr(vdev), mac) == QDF_STATUS_SUCCESS) {
		vap = wlan_vdev_get_vap(vdev);
		ic = wlan_vdev_get_ic(vdev);
		if (!vap || !ic) {
			qdf_print("%s: Req VAP %02x:%02x:%02x:%02x:%02x:%02x has invalid %s %s.",
				  __func__, mac[0], mac[1], mac[2],
				  mac[3], mac[4], mac[5],
				  vap ? "" : "vap", ic ? "" : "ic");
			return -EINVAL;
		}

		info->max_chwidth =
			ic->ic_cwm_get_width(ic);
		info->num_streams =
			ieee80211_getstreams(ic, ic->ic_tx_chainmask);
		info->phymode =
			vap->iv_cur_mode;
		/* Tx power is stored in half dBm */
		info->max_txpower =
			vap->iv_bss->ni_txpower / 2;

		/* get the intersected (user-set vs target caps)
		 * values of mcsnssmap */
		ieee80211vap_get_insctd_mcsnssmap(vap, he_rxmcsnssmap, he_txmcsnssmap);
#ifdef WLAN_FEATURE_11BE
		ieee80211vap_get_insctd_mcsnssmap_eht(vap, eht_rxmcsnssmap, eht_txmcsnssmap);
#endif /* WLAN_FEATURE_11BE */

		max_MCS = wlan_node_get_max_MCS(vap->iv_cur_mode,
#ifdef WLAN_FEATURE_11BE
					  eht_rxmcsnssmap,
					  eht_txmcsnssmap,
#endif /* WLAN_FEATURE_11BE */
					  he_rxmcsnssmap,
					  he_txmcsnssmap,
					  vap->iv_vhtcap_max_mcs.rx_mcs_set.mcs_map,
					  vap->iv_vhtcap_max_mcs.tx_mcs_set.mcs_map,
					  &ic->ic_sup_ht_rates[vap->iv_cur_mode],
					  &ic->ic_sup_rates[vap->iv_cur_mode]);
		if (max_MCS < 0) {
			qdf_print("%s: Requested VAP %02x:%02x:%02x:%02x:%02x:%02x has no valid rate info.",
				  __func__, mac[0], mac[1], mac[2],
				  mac[3], mac[4], mac[5]);
			return -EINVAL;
		} else {
			info->max_MCS = max_MCS;
		}
	} else {
		psoc = wlan_vdev_get_psoc(vdev);
		if (!psoc) {
			qdf_print("%s: Req STA %02x:%02x:%02x:%02x:%02x:%02x has invalid PSOC.",
					__func__, mac[0], mac[1], mac[2],
					mac[3], mac[4], mac[5]);
			return -EINVAL;
		}
		pdev_id =  wlan_objmgr_pdev_get_pdev_id(wlan_vdev_get_pdev(vdev));
		peer = wlan_objmgr_get_peer(psoc, pdev_id, mac, WLAN_SON_ID);
		if (!peer) {
			qdf_print("%s: Requested STA %02x:%02x:%02x:%02x:%02x:%02x has no valid rate info.",
				  __func__, mac[0], mac[1], mac[2],
				  mac[3], mac[4], mac[5]);
			return -EINVAL;
		}

		ni = wlan_peer_get_mlme_ext_obj(peer);
		if (!ni) {
			qdf_print("%s: Requested STA %02x:%02x:%02x:%02x:%02x:%02x is not "
			  "associated", __func__, mac[0], mac[1],
			  mac[2], mac[3], mac[4], mac[5]);
			wlan_objmgr_peer_release_ref(peer, WLAN_SON_ID);
			return -EINVAL;
		}

		info->max_chwidth = ni->ni_chwidth;
		info->num_streams = ni->ni_streams;
		info->phymode = ni->ni_phymode;
		info->max_txpower = ni->ni_max_txpower;
		info->is_mu_mimo_supported = (bool)(ni->ni_vhtcap & IEEE80211_VHTCAP_MU_BFORMEE);

		max_MCS = wlan_peer_get_node_max_MCS(peer);
		if (max_MCS < 0) {
			qdf_print("%s: Requested STA %02x:%02x:%02x:%02x:%02x:%02x has no valid rate info.",
				  __func__, mac[0], mac[1], mac[2],
				  mac[3], mac[4], mac[5]);
			wlan_objmgr_peer_release_ref(peer, WLAN_SON_ID);
			return -EINVAL;
		} else {
			info->max_MCS = max_MCS;
		}
		info->is_static_smps =
			((ni->ni_htcap & IEEE80211_HTCAP_C_SM_MASK) ==
			 IEEE80211_HTCAP_C_SMPOWERSAVE_STATIC);
		wlan_objmgr_peer_release_ref(peer, WLAN_SON_ID);
	}

	return EOK;
}

QDF_STATUS wlan_peer_send_null(struct wlan_objmgr_peer *peer)
{
	struct ieee80211_node *ni = NULL;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (!ni)
		return QDF_STATUS_E_INVAL;

	if (wlan_peer_mlme_flag_get(peer, WLAN_PEER_F_QOS))
		ieee80211_send_qosnulldata(ni, WME_AC_VI, false, false, NULL);
	else
		ieee80211_send_nulldata(ni, false, false, NULL);

	return QDF_STATUS_SUCCESS;
}

bool wlan_peer_update_sta_stats(struct wlan_objmgr_peer *peer,
				struct wlan_sta_stats_ind *sta_stats,
                                void *peer_stats)
{
	struct cdp_interface_peer_stats *stats = (struct cdp_interface_peer_stats *)peer_stats;

	if (!stats) {
		qdf_print("%s stats is NULL ", __func__);
		return false;
	}
        qdf_mem_copy(sta_stats->peer_stats[sta_stats->peer_count].client_addr,
                 wlan_peer_get_macaddr(peer), QDF_MAC_ADDR_SIZE);
        sta_stats->peer_stats[sta_stats->peer_count].tx_rate =
               stats->last_peer_tx_rate;
        sta_stats->peer_stats[sta_stats->peer_count].tx_packet_count =
               stats->tx_packet_count;
        sta_stats->peer_stats[sta_stats->peer_count].rx_packet_count =
               stats->rx_packet_count;
        sta_stats->peer_stats[sta_stats->peer_count].tx_byte_count =
               stats->tx_byte_count;
        sta_stats->peer_stats[sta_stats->peer_count].rx_byte_count =
               stats->rx_byte_count;
        sta_stats->peer_stats[sta_stats->peer_count].rssi = stats->peer_rssi;
        sta_stats->peer_stats[sta_stats->peer_count].per = stats->per;
        sta_stats->peer_stats[sta_stats->peer_count].ack_rssi = stats->ack_rssi;
        sta_stats->peer_count++;

        return true;
}
u_int8_t wlan_peer_get_snr(struct wlan_objmgr_peer *peer)
{
	struct ieee80211_node *ni = NULL;

	ni = wlan_peer_get_mlme_ext_obj(peer);
        if (!ni)
            return 0;

	return ni->ni_snr;
}

u_int32_t wlan_peer_get_rate(struct wlan_objmgr_peer *peer)
{
	ieee80211_rate_info rinfo;
	struct ieee80211_node *ni = NULL;

	ni = wlan_peer_get_mlme_ext_obj(peer);

	if (!ni) {
		return 0;
	}

	wlan_node_txrate_info(ni, &rinfo);
	return rinfo.lastrate;
}
struct ieee80211_ath_channel wlan_vdev_get_channel(struct wlan_objmgr_vdev *vdev)
{
	wlan_if_t vap = NULL;

	vap = wlan_vdev_get_vap(vdev);
	if (!vap)
		return empty_channel;

	return wlan_get_bss_channel(vap);
}

#if UMAC_SUPPORT_ACS
int wlan_vdev_acs_set_user_chanlist(struct wlan_objmgr_vdev *vdev,
        bool append, qdf_freq_t *chanlist)
{
	wlan_if_t vap = NULL;

	vap = wlan_vdev_get_vap(vdev);
	if (!vap)
		return -EINVAL;

	return wlan_acs_set_user_chanlist(vap, append, chanlist);
}

int wlan_vdev_acs_start_scan_report(struct wlan_objmgr_vdev *vdev , int val)
{
	wlan_if_t vap = NULL;

	vap = wlan_vdev_get_vap(vdev);
	if (!vap)
		return -EINVAL;

	return wlan_acs_start_scan_report(vap, 1, IEEE80211_START_ACS_REPORT,
					  (void *)&val);
}
#endif /* UMAC_SUPPORT_ACS */

u_int8_t wlan_vdev_get_rx_streams(struct wlan_objmgr_vdev *vdev)
{
	wlan_if_t vap = NULL;
	struct ieee80211com *ic = NULL;

	vap = wlan_vdev_get_vap(vdev);
        if (!vap)
		return 0;

	ic  = wlan_vdev_get_ic(vdev);

	if (!ic)
		return 0;

	return ieee80211_get_rxstreams(ic, vap);
}

/**
 * wlan_vdev_get_160_nss() - Get 160MHz bandwidth NSS
 * @vdev: vdev object manager handle
 *
 * This API is used to retrieve NSS value for 160MHz bandwidth
 *
 * Return: 160MHz NSS value unsigned integer on Success
 *         0 on Failure
 *
 */
u_int8_t wlan_vdev_get_160_nss(struct wlan_objmgr_vdev *vdev)
{
        wlan_if_t vap = NULL;
        struct ieee80211_bwnss_map nssmap;
        struct ieee80211com *ic = NULL;
        uint8_t txchainmask;
        uint32_t max_nss;

        vap = wlan_vdev_get_vap(vdev);
        if (!vap)
                return 0;

        ic  = wlan_vdev_get_ic(vdev);

        if (!ic)
                return 0;

        txchainmask = ieee80211com_get_tx_chainmask(ic);

        ieee80211_compute_nss(ic, txchainmask, &nssmap);

        wlan_util_vdev_mlme_get_param(vap->vdev_mlme,
                        WLAN_MLME_CFG_NSS, &max_nss);

        return QDF_MIN(max_nss, nssmap.bw_nss_160);
}

/**
 * wlan_vdev_get_80p80_nss() - Get 80+80 MHz bandwidth NSS
 * @vdev: vdev object manager handle
 *
 * This API is used to retrieve NSS value for 80+80 MHz bandwidth
 *
 * Return: 80+80 MHz NSS value unsigned integer on Success
 *         0 on Failure
 *
 */
u_int8_t wlan_vdev_get_80p80_nss(struct wlan_objmgr_vdev *vdev)
{
        return wlan_vdev_get_160_nss(vdev);
}

/**
 * wlan_vdev_get_le80_nss() - Get 20, 40, 80 MHz bandwidth NSS
 * @vdev: vdev object manager handle
 *
 * This API is used to retrieve NSS value for 80MHz and below
 *
 * Return: unsigned integer NSS value for 80MHz or below on Success
 *         0 on Failure
 *
 */
u_int8_t wlan_vdev_get_le80_nss(struct wlan_objmgr_vdev *vdev)
{
        uint32_t le80_nss;
        wlan_if_t vap = NULL;

        vap = wlan_vdev_get_vap(vdev);
        if (!vap)
                return 0;

        wlan_util_vdev_mlme_get_param(vap->vdev_mlme,
                        WLAN_MLME_CFG_NSS, &le80_nss);

        return le80_nss;
}

/**
 * wlan_vdev_get_nss() - Get NSS with current BW
 * @vdev: vdev object manager handle
 *
 * This API is used to retrieve NSS value
 *
 * Return: unsigned integer NSS value on Success
 *         0 on Failure
 *
 */
u_int8_t wlan_vdev_get_nss(struct wlan_objmgr_vdev *vdev)
{
    u_int8_t nss;
    wlan_if_t vap = NULL;
    u_int8_t ap_nss, loc_nss, chwidth;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return 0;

    if (wlan_vdev_is_sta(vdev)) {
        ap_nss = vap->iv_bss->ni_maxrxstreams;
        chwidth = wlan_vdev_get_chwidth(vdev);
        switch(chwidth) {
        case IEEE80211_CWM_WIDTH160:
            loc_nss = wlan_vdev_get_160_nss(vdev);
            break;
        case IEEE80211_CWM_WIDTH80_80:
            loc_nss = wlan_vdev_get_80p80_nss(vdev);
            break;
        case IEEE80211_CWM_WIDTH80:
            loc_nss = wlan_vdev_get_le80_nss(vdev);
            break;
        default:
            loc_nss = wlan_vdev_get_rx_streams(vdev);
            break;
        }
        if (ap_nss)
            nss = (ap_nss > loc_nss) ? loc_nss : ap_nss;
        else
            nss = loc_nss;
    } else {
        nss = wlan_vdev_get_rx_streams(vdev);
    }
    return nss;
}

u_int8_t wlan_vdev_is_sta(struct wlan_objmgr_vdev *vdev)
{
        wlan_if_t vap = NULL;

        vap = wlan_vdev_get_vap(vdev);
        if (!vap)
                return 0;

        return ieee80211_is_sta(vap);
}

/**
 * wlan_get_11axa_he_width_mask() - Get 11axa 40/80/160/80P80 widthmask
 * from the hecap phy info of ic.
 *
 * @hecap_phy_info: Pointer to hecap_phy_info fetched from ic
 * @width_info: Channel width info.
 */
static void
wlan_get_11axa_he_width_mask(uint8_t *hecap_phy_info,
			     uint8_t *width_info)
{
	uint32_t he_width_mask;

	he_width_mask = HECAP_PHY_CBW_GET_FROM_IC(hecap_phy_info);
	*width_info = he_width_mask & IEEE80211_HECAP_PHY_CHWIDTH_11AXA_HE40_HE80_HE160_HE80_80_MASK;
}

int wlan_vdev_get_apcap(struct wlan_objmgr_vdev *vdev, wlan_ap_cap *apcap)
{
	wlan_if_t vap = NULL;
	struct ieee80211com *ic = NULL;
	u_int32_t rx_streams, tx_streams;
	struct wlan_objmgr_pdev *pdev = NULL;
	qdf_freq_t low_2g=0, high_2g=0, low_5g=0, high_5g=0;
	uint16_t he_rxmcsnssmap[HEHANDLE_CAP_TXRX_MCS_NSS_SIZE];
	uint16_t he_txmcsnssmap[HEHANDLE_CAP_TXRX_MCS_NSS_SIZE];
#ifdef WLAN_FEATURE_11BE
	uint32_t eht_rxmcsnssmap[EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE];
	uint32_t eht_txmcsnssmap[EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE];
#endif /* WLAN_FEATURE_11BE */
	uint8_t i, k = 0;
	struct wlan_ap_wifi6_capabilities *wifi6cap = &apcap->wifi6cap;
	uint8_t width_info, *hecap_phy_info;
#define MAX_USER_PER_DL_MIMO_TX	4
#define MAX_USER_PER_DL_MIMO_RX	4
#define MAX_USER_PER_DL_OFDM_TX	8
#define MAX_USER_PER_DL_OFDM_RX	8

	pdev = wlan_vdev_get_pdev(vdev);
	if(!pdev)
		return -EINVAL;

	vap = wlan_vdev_get_vap(vdev);
	if (!vap)
		return -EINVAL;

	ic = vap->iv_ic;
	if (!ic)
		return -EINVAL;

	rx_streams = ieee80211_get_rxstreams(ic, vap);
	tx_streams = ieee80211_get_txstreams(ic, vap);

	/* Add HT cap */
	if (ieee80211_vap_wme_is_set(vap) &&
	    ieee80211com_has_cap(ic, IEEE80211_C_HT)) {

		apcap->wlan_ap_ht_capabilities_valid = 1;

		apcap->htcap.max_tx_nss = tx_streams;
		apcap->htcap.max_rx_nss = rx_streams;

		apcap->htcap.htcap = ic->ic_htcap;
	}

	/* Check if radio is capable of operating on 5Ghz */
	wlan_reg_get_freq_range(pdev, &low_2g, &high_2g, &low_5g, &high_5g);

	/* Add VHT cap */
	if (ieee80211_vap_wme_is_set(vap) &&
	    ieee80211com_has_cap_ext(ic, IEEE80211_CEXT_11AC) &&
	    ieee80211vap_vhtallowed(vap) &&
	    (low_5g!=0 || high_5g!=0 )) {

		apcap->wlan_ap_vht_capabilities_valid = 1;

		if (!vap->iv_set_vht_mcsmap) {
			apcap->vhtcap.supp_tx_mcs = ic->ic_vhtcap_max_mcs.tx_mcs_set.mcs_map;
			apcap->vhtcap.supp_rx_mcs = ic->ic_vhtcap_max_mcs.rx_mcs_set.mcs_map;
		} else {
			apcap->vhtcap.supp_tx_mcs = vap->iv_vhtcap_max_mcs.tx_mcs_set.mcs_map;
			apcap->vhtcap.supp_rx_mcs = vap->iv_vhtcap_max_mcs.rx_mcs_set.mcs_map;
		}

		apcap->vhtcap.max_tx_nss = tx_streams;
		apcap->vhtcap.max_rx_nss = rx_streams;

		apcap->vhtcap.vhtcap = ic->ic_vhtcap;
	}
	/* Add HE cap */
	if (ieee80211_vap_wme_is_set(vap) && ieee80211vap_heallowed(vap)) {
		apcap->wlan_ap_he_capabilities_valid = 1;

		apcap->hecap.num_mcs_entries = MAX_HE_MCS;

		/* Get HE MCS RX and TX NSS maps and
		 * store values to MAP hecap structure */
		ieee80211vap_get_insctd_mcsnssmap(vap, he_rxmcsnssmap, he_txmcsnssmap);
		for (k = 0; k < HEHANDLE_CAP_TXRX_MCS_NSS_SIZE; k++) {
			apcap->hecap.supported_he_mcs[2 * k] = he_rxmcsnssmap[k];
			apcap->hecap.supported_he_mcs[2 * k + 1] = he_txmcsnssmap[k];
		}

		apcap->hecap.max_tx_nss = tx_streams;
		apcap->hecap.max_rx_nss = rx_streams;

		apcap->hecap.he_relaxed_edca = vap->iv_he_relaxed_edca;
		apcap->hecap.he_spatial_reuse = vap->iv_he_spatial_reuse;
		apcap->hecap.he_multi_bss = vap->iv_he_multi_bss;
		apcap->hecap.he_su_ppdu_1x_ltf_800ns_gi = vap->iv_he_su_ppdu_1x_ltf_800ns_gi;
		apcap->hecap.he_su_mu_ppdu_4x_ltf_800ns_gi = vap->iv_he_su_mu_ppdu_4x_ltf_800ns_gi;
		apcap->hecap.he_ndp_4x_ltf_3200ns_gi = vap->iv_he_ndp_4x_ltf_3200ns_gi;
		apcap->hecap.he_er_su_ppdu_1x_ltf_800ns_gi = vap->iv_he_er_su_ppdu_1x_ltf_800ns_gi;
		apcap->hecap.he_er_su_ppdu_4x_ltf_800ns_gi = vap->iv_he_er_su_ppdu_4x_ltf_800ns_gi;

		apcap->hecap.he_su_bfer = vap->iv_he_su_bfer;
		apcap->hecap.he_su_bfee = vap->iv_he_su_bfee;
		apcap->hecap.he_mu_bfer = vap->iv_he_mu_bfer;
		apcap->hecap.he_mu_bfee = vap->iv_he_mu_bfee;

		apcap->hecap.he_ul_mumimo = vap->iv_he_ul_mumimo;

		apcap->hecap.he_ul_muofdma = vap->iv_he_ul_muofdma;
		apcap->hecap.he_dl_muofdma = vap->iv_he_dl_muofdma;
		apcap->hecap.he_dl_muofdma_bfer = vap->iv_he_dl_muofdma_bfer;

		apcap->hecap.he_sounding_mode = vap->iv_he_sounding_mode;
		apcap->hecap.he_muedca = vap->iv_he_muedca;

		for (i = 0; i < HEHANDLE_CAP_PHYINFO_SIZE; i++)
			apcap->hecap.phyinfo[i] = ic->ic_he.hecap_phyinfo[i];

		/// WIFI6 capability
		apcap->wlan_ap_wifi6_capabilites_valid = 1;
		// currently we can not support AP/STA on same vap
		wifi6cap->numofroles = 1;

		/// 0: wi-fi 6 support info for the ap role
		/// 1: wi-fi 6 support info for the non-ap sta role
		wifi6cap->role_cap[0].role = 0;

		hecap_phy_info = (uint8_t *) &ic->ic_he.hecap_phyinfo[IC_HECAP_PHYDWORD_IDX0];
		wlan_get_11axa_he_width_mask(hecap_phy_info, &width_info);
		/// support for he 160 mhz
		/// 0: not supported
		/// 1: supported
		if (width_info & IEEE80211_HECAP_PHY_CHWIDTH_11AXA_HE160)
			wifi6cap->role_cap[0].he160 = 1;

		/// support for he 80+80 mhz
		/// 0: not supported
		/// 1: supported
		if (width_info & IEEE80211_HECAP_PHY_CHWIDTH_11AXA_HE80_80)
			wifi6cap->role_cap[0].he80plus80 = 1;

		/* Get HE MCS RX and TX NSS maps and
		 * store values to MAP hecap structure */
		ieee80211vap_get_insctd_mcsnssmap(vap, he_rxmcsnssmap, he_txmcsnssmap);

		for (k = 0; k < HEHANDLE_CAP_TXRX_MCS_NSS_SIZE; k++) {
			wifi6cap->role_cap[0].supported_he_mcs[2 * k] = he_rxmcsnssmap[k];
			wifi6cap->role_cap[0].supported_he_mcs[2 * k + 1] = he_txmcsnssmap[k];
		}

		/// support for su beamformer.
		wifi6cap->role_cap[0].subeamformer = vap->iv_he_su_bfer;
		/// support for su beamformee
		wifi6cap->role_cap[0].subeamformee = vap->iv_he_su_bfee;
		/// support for mu beamformer status
		wifi6cap->role_cap[0].mu_beam_former_status = vap->iv_he_mu_bfer;

		/// support for beamformee sts ≤ 80 mhz
		wifi6cap->role_cap[0].beam_formee_sts_less_than_80supported =
		vap->iv_he_subfee_sts_lteq80;
		/// support for beamformee sts > 80 mhz
		wifi6cap->role_cap[0].beam_formee_sts_more_than_80supported =
		vap->iv_he_subfee_sts_gt80;
		/// support for ul mu-mimo
		wifi6cap->role_cap[0].ulmumimosupported = vap->iv_he_ul_mumimo;
		/// support for ul ofdma.
		wifi6cap->role_cap[0].ulofdmasupported = vap->iv_he_ul_muofdma;
		/// support for dl ofdma
		wifi6cap->role_cap[0].dlofdmasupported = vap->iv_he_dl_muofdma;
		/// max number of users supported per
		/// dl mu-mimo tx in an ap role
		wifi6cap->role_cap[0].maxuser_per_dl_mumimotxap = MAX_USER_PER_DL_MIMO_TX;
		/// max number of users supported per
		/// dl mu-mimo rx in an ap role
		wifi6cap->role_cap[0].max_user_per_dl_mumimorxap = MAX_USER_PER_DL_MIMO_RX;
		/// max number of users supported per dl ofdma tx in an ap role
		wifi6cap->role_cap[0].maxuserdlofdmatxap = MAX_USER_PER_DL_OFDM_TX;
		/// max number of users supported per ul ofdma rx in an ap role
		wifi6cap->role_cap[0].maxuserdlofdmarxap = MAX_USER_PER_DL_OFDM_TX;
		/// support for rts
		wifi6cap->role_cap[0].rtssupported = 1;
		/// support for mu rts
		wifi6cap->role_cap[0].murtssupported = 0;
		/// support for multi-bssid
		wifi6cap->role_cap[0].multibssidsupported = vap->iv_he_multi_bss;
		/// support for mu edca
		wifi6cap->role_cap[0].muedcasupported = vap->iv_he_muedca;
		/// support for twt requester
		wifi6cap->role_cap[0].twtrequestersupprted = vap->iv_he_twtreq;
		/// support for twt responder
		wifi6cap->role_cap[0].twtrespondersupported = (vap->iv_twt_rsp >= IEEE80211_TWT_ENABLE);
	}
#undef MAX_USER_PER_DL_MIMO_TX
#undef MAX_USER_PER_DL_MIMO_RX
#undef MAX_USER_PER_DL_OFDM_TX
#undef MAX_USER_PER_DL_OFDM_RX

#ifdef WLAN_FEATURE_11BE
	/* Add EHT cap */
	if (ieee80211_vap_wme_is_set(vap) && ieee80211vap_ehtallowed(vap)) {
		apcap->wlan_ap_eht_capabilities_valid = 1;

		/* Get EHT MCS RX and TX NSS maps and
		 * store values to MAP ehtcap structure */
		ieee80211vap_get_insctd_mcsnssmap_eht(vap, eht_rxmcsnssmap, eht_txmcsnssmap);
		for (k = 0; k < EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE; k++) {
			apcap->ehtcap.supported_eht_mcs[2 * k] = eht_rxmcsnssmap[k];
			apcap->ehtcap.supported_eht_mcs[2 * k + 1] = eht_txmcsnssmap[k];
		}

		apcap->ehtcap.num_mcs_entries = MAX_EHT_MCS;
		apcap->ehtcap.max_tx_nss = tx_streams;
		apcap->ehtcap.max_rx_nss = rx_streams;

    }
#endif /* WLAN_FEATURE_11BE */

	return EOK;
}

int wlan_vdev_add_acl_validity_timer(struct wlan_objmgr_vdev *vdev,
				     const u_int8_t *mac_addr,
				     u_int16_t validity_timer)
{
	wlan_if_t vap = NULL;
	vap = wlan_vdev_get_vap(vdev);
	if (!vap)
		return -EINVAL;

	return ieee80211_acl_add_with_validity(vap, mac_addr, validity_timer);
}

bool wlan_vdev_is_deleted_set(struct wlan_objmgr_vdev *vdev)
{
	wlan_if_t vap = NULL;

	vap = wlan_vdev_get_vap(vdev);
	if (!vap)
		return false;

	return ieee80211_vap_deleted_is_set(vap);
}

void wlan_vdev_acl_override_rssi_thresholds(struct wlan_objmgr_vdev *vdev,
					    const u_int8_t *mac_addr,
					    u_int32_t *inact_rssi_low,
					    u_int32_t *inact_rssi_high,
					    u_int32_t *low_rssi,
					    u_int32_t *low_rate_rssi,
					    u_int32_t *high_rate_rssi)
{
	wlan_if_t vap;
	struct ieee80211_node *ni = NULL;

	vap = wlan_vdev_get_vap(vdev);
	if (vap == NULL)
		return;

	ni = ieee80211_find_node(vap->iv_ic, mac_addr, WLAN_MLME_HANDLER_ID);
	if (ni == NULL)
		return;

	wlan_acl_override_rssi_thresholds(ni, inact_rssi_low,
					  inact_rssi_high,
					  low_rssi,
					  low_rate_rssi,
					  high_rate_rssi);
	ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);
}

bool wlan_vdev_acl_is_drop_mgmt_set(struct wlan_objmgr_vdev *vdev,
				    const u_int8_t *mac_addr)
{
	wlan_if_t vap = NULL;

	vap = wlan_vdev_get_vap(vdev);

	return ieee80211_acl_flag_check(vap, mac_addr,
					IEEE80211_ACL_FLAG_BLOCK_MGMT);
}

int wlan_node_get_capability(struct wlan_objmgr_peer *peer,
			     struct wlan_peer_cap *cap)
{
	struct ieee80211_node *ni = NULL;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (!ni)
		return -EINVAL;

        cap->is_BTM_Supported = (bool)(ni->ext_caps.ni_ext_capabilities & IEEE80211_EXTCAPIE_BSSTRANSITION);
	cap->is_RRM_Supported = (bool)(ni->ni_flags & IEEE80211_NODE_RRM);
	cap->is_beacon_meas_supported = (bool)(ieee80211node_has_extflag(ni, IEEE80211_NODE_BCN_MEASURE_SUPPORT));
	cap->info.max_chwidth = ni->ni_chwidth;
	cap->info.num_streams = ni->ni_streams;
	cap->info.phymode = ni->ni_phymode;
	cap->info.max_txpower = ni->ni_max_txpower;
	cap->band_cap = ni->ni_operating_bands;
	cap->info.is_static_smps =
		((ni->ni_htcap & IEEE80211_HTCAP_C_SM_MASK) ==
		 IEEE80211_HTCAP_C_SMPOWERSAVE_STATIC);
	cap->info.is_mu_mimo_supported =
		(bool)(ni->ni_vhtcap & IEEE80211_VHTCAP_MU_BFORMEE);
#if UMAC_SUPPORT_RRM
        OS_MEMCPY(cap->rrmcaps, ni->ni_rrm_caps, sizeof(cap->rrmcaps));
#endif
	IEEE80211_ADDR_COPY(cap->bssid, ni->ni_bssid);
	if (IEEE80211_NODE_USE_HT(ni)) {
		cap->htcap = ni->ni_htcap;
	} else {
		cap->htcap = 0;
	}
	if (IEEE80211_NODE_USE_VHT(ni)) {
		cap->vhtcap = ni->ni_vhtcap;
	} else {
		cap->vhtcap =  0;
	}
	if (IEEE80211_NODE_USE_HE(ni)) {
		qdf_mem_copy(&cap->hecap.he_cap_macinfo,
			     &ni->ni_he.hecap_macinfo, HECAP_MACINFO_SIZE);
		qdf_mem_copy(cap->hecap.he_cap_rxmcsnssmap,
			     &ni->ni_he.hecap_rxmcsnssmap,
			     HEHANDLE_CAP_TXRX_MCS_NSS_SIZE);
		qdf_mem_copy(cap->hecap.he_cap_txmcsnssmap,
			     &ni->ni_he.hecap_txmcsnssmap,
			     HEHANDLE_CAP_TXRX_MCS_NSS_SIZE);
		qdf_mem_copy(cap->hecap.he_cap_phyinfo,
			     &ni->ni_he.hecap_phyinfo,
			     HEHANDLE_CAP_PHYINFO_SIZE);
		cap->hecap.he_cap_ppet_numnss_m1 =
					ni->ni_he.hecap_ppet.numss_m1;
		cap->hecap.he_cap_ppet_ru_mask =
					ni->ni_he.hecap_ppet.ru_mask;
		qdf_mem_copy(cap->hecap.he_cap_ppet16_ppet8_ru3_ru0,
			     &ni->ni_he.hecap_ppet.ppet16_ppet8_ru3_ru0,
			     HE_PPET16_PPET8_SIZE);
		cap->hecap.he_cap_info_internal =
					ni->ni_he.hecap_info_internal;
	} else {
		qdf_mem_zero(&cap->hecap, sizeof(wlan_client_he_capabilities));
	}

	return EOK;
}
#endif

bool wlan_vdev_acl_is_probe_wh_set(struct wlan_objmgr_vdev *vdev,
		const u_int8_t *mac_addr, u_int8_t probe_rssi)
{
	wlan_if_t vap = NULL;
	u_int8_t rssi_hwm = 0, rssi_lwm = 0;
	u_int8_t params[] = {IEEE80211_ACL_PARAM_PROBE_RSSI_HWM,
			     IEEE80211_ACL_PARAM_PROBE_RSSI_LWM};
	u_int8_t *values[] = {&rssi_hwm,
			      &rssi_lwm};

	vap = wlan_vdev_get_vap(vdev);
	if (!vap)
		return false;

	if (ieee80211_acl_get_params_if_flag(vap, mac_addr,
					     IEEE80211_ACL_FLAG_PROBE_RESP_WH,
					     params,
					     values,
					     QDF_ARRAY_SIZE(params)) > 0) {
		if (rssi_hwm > 0 && probe_rssi > rssi_hwm)
			return true;
		else if (rssi_lwm > 0 && probe_rssi < rssi_lwm)
			return true;
		else if (rssi_hwm == 0 && rssi_lwm == 0)
			/*
			 * If hwm and lwm are not set,
			 * then it means that the decision to drop probe
			 * response only depends on the ACL flag being set
			 *
			 * We are here because the ACL flag for probe response
			 * is set
			 */
			return true;
	}
	return false;
}

u_int8_t  wlan_vdev_get_chwidth(struct wlan_objmgr_vdev *vdev)
{
	wlan_if_t vap = NULL;
	struct ieee80211com *ic = NULL;

	vap = wlan_vdev_get_vap(vdev);
        if (!vap)
		return 0;

	ic  = wlan_vdev_get_ic(vdev);

	if (ic == NULL)
		return 0;

	if(vap->iv_chwidth != IEEE80211_CWM_WIDTHINVALID)
		return vap->iv_chwidth;
	else
		return ic->ic_cwm_get_width(ic);
}

void wlan_set_peer_rate_legacy(struct wlan_objmgr_peer *peer, struct ieee80211_rateset *rates)
{
    struct ieee80211_node *ni = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        mlme_err("ni not found!\n");
        return;
    }
    qdf_mem_copy(&ni->ni_rates, rates, sizeof(struct ieee80211_rateset));
}


void wlan_set_peer_rate_ht(struct wlan_objmgr_peer *peer, struct ieee80211_rateset *rates)
{
    struct ieee80211_node *ni = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        mlme_err("ni not found!\n");
        return;
    }

    if (rates != NULL){
        qdf_mem_copy(&ni->ni_htrates, rates, sizeof(struct ieee80211_rateset));
        if (ni->ni_streams == 1 && ni->ni_htrates.rs_rates[0] > 7) {
            /* nss=1, but all MCS for nss 1 are zero, not valid, disable HT */
            mlme_info("add_client: clearing HT capability for peer\n");
            ni->ni_flags &= ~IEEE80211_NODE_HT;
        }
        if (ni->ni_streams == 2 && ni->ni_htrates.rs_rates[0] > 15) {
            /* nss=2, but MCS for NSS=1 and NSS=2 are zero, not valid, disable HT */
            mlme_info("add_client: clearing HT capability for peer\n");
            ni->ni_flags &= ~IEEE80211_NODE_HT;
        }
        if (ni->ni_streams == 3 && ni->ni_htrates.rs_rates[0] > 23) {
            /* nss=3, but MCS for NSS=1,NSS=2 and NSS=3 are zero, not valid, disable HT */
            mlme_info("add_client: clearing HT capability for peer\n");
            ni->ni_flags &= ~IEEE80211_NODE_HT;
        }
        if (IEEE80211_NODE_USE_HT(ni)) {
            qdf_mem_copy(&ni->ni_htrates, rates, sizeof(struct ieee80211_rateset));
        } else {
            qdf_mem_zero(&ni->ni_htrates, sizeof(struct ieee80211_rateset));
        }
    } else {
        qdf_mem_zero(&ni->ni_htrates, sizeof(struct ieee80211_rateset));
    }
}

void wlan_set_peer_rate_vht(struct wlan_objmgr_peer *peer, u_int16_t vhtrate_map)
{
    struct ieee80211_node *ni = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        mlme_err("ni not found!\n");
        return;
    }
    ni->ni_tx_vhtrates = vhtrate_map;
}

#ifdef WLAN_FEATURE_11BE
void wlan_set_peer_rate_eht(struct wlan_objmgr_peer *peer, u_int16_t ehtrate_map)
{
    struct ieee80211_node *ni = NULL;
    uint32_t *ni_eht_tx_mcs_set = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        mlme_err("ni not found!\n");
        return;
    }

    ni_eht_tx_mcs_set = ni->ni_eht.ehtcap_txmcsnssmap;
    switch(ni->ni_phymode) {
        case IEEE80211_MODE_11BEA_EHT320:
            ni_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_320] = ehtrate_map;
            /* fall through */
        case IEEE80211_MODE_11BEA_EHT160:
            ni_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_160] = ehtrate_map;
            /* fall through */
        default:
            ni_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_80] = ehtrate_map;
            ni_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_ONLY20] = ehtrate_map;
            break;
    }
}
#endif /* WLAN_FEATURE_11BE */

void wlan_set_peer_rate_he(struct wlan_objmgr_peer *peer, u_int16_t herate_map)
{
    struct ieee80211_node *ni = NULL;
    uint16_t *ni_he_tx_mcs_set = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        mlme_err("ni not found!\n");
        return;
    }

    ni_he_tx_mcs_set = ni->ni_he.hecap_txmcsnssmap;
    switch(ni->ni_phymode) {
        case IEEE80211_MODE_11AXA_HE80_80:
            ni_he_tx_mcs_set[HECAP_TXRX_MCS_NSS_IDX_80_80] = herate_map;
            /* fall through */
        case IEEE80211_MODE_11AXA_HE160:
            ni_he_tx_mcs_set[HECAP_TXRX_MCS_NSS_IDX_160] = herate_map;
            /* fall through */
        default:
            ni_he_tx_mcs_set[HECAP_TXRX_MCS_NSS_IDX_80] = herate_map;
            break;
    }
}

#ifdef WLAN_FEATURE_11BE
void wlan_peer_dump_rates_eht(struct wlan_objmgr_peer *peer)
{
    uint32_t *ni_eht_tx_mcs_set = NULL;
    struct ieee80211_node *ni = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        mlme_err("ni not found!\n");
        return;
    }

    if (wlan_peer_is_extflag_set(peer, IEEE80211_NODE_EHT)) {
        ni_eht_tx_mcs_set = ni->ni_eht.ehtcap_txmcsnssmap;
        switch(ni->ni_phymode) {
            case IEEE80211_MODE_11BEA_EHT320:
                mlme_info("add_client: EHT rate map=0x%x for 320HZ\n", ni_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_320]);
                /* fall through */
            case IEEE80211_MODE_11BEA_EHT160:
                mlme_info("add_client: EHT rate map=0x%x for 160HZ\n", ni_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_160]);
                /* fall through */
            default:
                mlme_info("add_client: EHT rate map=0x%x for 80HZ\n", ni_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_80]);
                break;
        }
    }
}
#else
void wlan_peer_dump_rates_eht(struct wlan_objmgr_peer *peer)
{
}
#endif /* WLAN_FEATURE_11BE */


void wlan_peer_dump_rates(struct wlan_objmgr_peer *peer)
{
    struct ieee80211_node *ni = NULL;
    int i = 0;
    uint16_t *ni_he_tx_mcs_set = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        mlme_err("ni not found!\n");
        return;
    }

    mlme_info("add_client: legacy rates\n");
    for (i=0; i<ni->ni_rates.rs_nrates; i++) {
        mlme_info("%d\n", ni->ni_rates.rs_rates[i]);
    }
    mlme_info("\n");

    if (wlan_peer_is_flag_set(peer, IEEE80211_NODE_HT)) {
        mlme_info("add_client: HT rates\n");
        for (i=0; i<ni->ni_htrates.rs_nrates; i++) {
            mlme_info("%d\n", ni->ni_htrates.rs_rates[i]);
        }
        mlme_info("\n");
    }

    if (wlan_peer_is_flag_set(peer, IEEE80211_NODE_VHT)) {
        mlme_info("add_client: VHT rate map=0x%x\n", ni->ni_tx_vhtrates);
    }

    if (wlan_peer_is_extflag_set(peer, IEEE80211_NODE_HE)) {
        ni_he_tx_mcs_set = ni->ni_he.hecap_txmcsnssmap;
        switch(ni->ni_phymode) {
            case IEEE80211_MODE_11AXA_HE80_80:
                mlme_info("add_client: HE rate map=0x%x for 80_80HZ\n", ni_he_tx_mcs_set[HECAP_TXRX_MCS_NSS_IDX_80_80]);
            case IEEE80211_MODE_11AXA_HE160:
                mlme_info("add_client: HE rate map=0x%x for 160HZ\n", ni_he_tx_mcs_set[HECAP_TXRX_MCS_NSS_IDX_160]);
            default:
                mlme_info("add_client: HE rate map=0x%x for 80HZ\n", ni_he_tx_mcs_set[HECAP_TXRX_MCS_NSS_IDX_80]);
                break;
        }
    }

    wlan_peer_dump_rates_eht(peer);
}


void wlan_deliver_mlme_evt_disassoc(struct wlan_objmgr_vdev *vdev, u_int8_t *stamac,
                                    struct wlan_objmgr_peer *peer, uint8_t is_ml_link_peer_del)
{
    struct ieee80211_node *ni = NULL;
    wlan_if_t vap;
#ifdef WLAN_FEATURE_11BE_MLO
    u_int8_t mldaddr[QDF_MAC_ADDR_SIZE] = {0};
#endif

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if (vap == NULL) {
        mlme_err("vap not found!\n");
        return;
    }

    ni = (wlan_node_t)wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        return;
    }
#ifdef WLAN_FEATURE_11BE_MLO
    if ((ni->peer_obj->mldaddr) &&
        (IEEE80211_ADDR_IS_VALID(&ni->peer_obj->mldaddr)))
        IEEE80211_ADDR_COPY(mldaddr, &ni->peer_obj->mldaddr[0]);

    if (is_ml_link_peer_del && wlan_peer_is_mlo(peer)) {
        wlan_mlo_partner_peer_disconnect_notify(peer);
    }
#endif
    IEEE80211_NODE_LEAVE(ni);

#ifdef WLAN_FEATURE_11BE_MLO
    IEEE80211_DELIVER_EVENT_MLME_DISASSOC_COMPLETE(vap, stamac,
                 IEEE80211_REASON_ASSOC_LEAVE, IEEE80211_STATUS_SUCCESS,
                 mldaddr);
#else
    IEEE80211_DELIVER_EVENT_MLME_DISASSOC_COMPLETE(vap, stamac,
                 IEEE80211_REASON_ASSOC_LEAVE, IEEE80211_STATUS_SUCCESS,
                 NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
}

int wlan_override_client_params(struct wlan_objmgr_vdev *vdev,
                                struct wlan_objmgr_peer *peer,
                                struct wlan_addclient_peer_caps *peer_caps)
{
    ieee80211_vht_rate_t vht;
    u_int16_t vhtrate_map = 0;
    u_int8_t i = 0;
    wlan_if_t vap = NULL;
    struct ieee80211_rateset overlap_rate;
    u_int8_t j;
    struct ieee80211_node *ni = NULL;
    enum ieee80211_cwm_width chwidth;

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap)
    	return -EINVAL;

    ni = (wlan_node_t)wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        return -EINVAL;
    }
#ifdef WLAN_FEATURE_11BE_MLO
    if (peer_caps->mlo)
        wlan_peer_set_mlo(peer);
    else
        wlan_peer_clear_mlo(peer);
#endif

    /* override qos flag */
    if (peer_caps->qos)
        wlan_set_peer_flag(peer, WLAN_PEER_F_QOS);
    else
        wlan_clear_peer_flag(peer, WLAN_PEER_F_QOS);

    /* Derive common legacy rateset*/
    OS_MEMSET(&overlap_rate, 0, sizeof(struct ieee80211_rateset));
    for (i=0; i<peer_caps->lrates.rs_nrates; i++) {
        for (j=0; j<ni->ni_rates.rs_nrates; j++) {
            if (peer_caps->lrates.rs_rates[i] == ni->ni_rates.rs_rates[j]) {
                overlap_rate.rs_rates[overlap_rate.rs_nrates++] = peer_caps->lrates.rs_rates[i];
                break;
            }
        }
    }
    if (overlap_rate.rs_nrates == 0) {
        qdf_debug("%s client["QDF_MAC_ADDR_FMT"] no overlap legacy rate with assoc-req\n",
                  __func__, QDF_MAC_ADDR_REF(&ni->ni_macaddr));
        return qdf_status_to_os_return(QDF_STATUS_NO_OVERLAP_LEGACY_RATES);
    }
    wlan_set_peer_rate_legacy(peer, &overlap_rate);

    if (wlan_peer_is_flag_set(peer, IEEE80211_NODE_HT)) {
        OS_MEMSET(&overlap_rate, 0, sizeof(struct ieee80211_rateset));
        for (i=0; i<peer_caps->htrates.rs_nrates; i++) {
            if (peer_caps->htrates.rs_rates[i] < ni->ni_max_streams*8) {
            /* use ni_max_streams instead of ni_steams to allow future use
             * of all rates
             */
                for (j=0; j<ni->ni_htrates.rs_nrates; j++) {
                    if (peer_caps->htrates.rs_rates[i] == ni->ni_htrates.rs_rates[j]) {
                        overlap_rate.rs_rates[overlap_rate.rs_nrates++] =
                                               peer_caps->htrates.rs_rates[i];
                    break;
                    }
                }
            }
        }
        wlan_set_peer_rate_ht(peer, &overlap_rate);
    } else {
        wlan_set_peer_rate_ht(peer, NULL);
    }


    if (wlan_peer_is_flag_set(peer, IEEE80211_NODE_VHT)) {
        qdf_mem_zero(&vht, sizeof(ieee80211_vht_rate_t));
        qdf_mem_set(&(vht.rates), MAX_VHT_STREAMS, 0xff);

        vht.num_streams = ni->ni_max_streams;
        if (vht.num_streams > MAX_VHT_STREAMS) {
            mlme_err("stream num %d beyond max VHT streams\n", vht.num_streams);
            return qdf_status_to_os_return(QDF_STATUS_MAX_VHT_STREAMS);
        }
        for(i=0; i < vht.num_streams; i++){
            vht.rates[i] = peer_caps->vhtrates;
        }
        vhtrate_map = ieee80211_get_vht_rate_map(&vht);
        wlan_set_peer_rate_vht(peer, vhtrate_map);
        /* if no VHT rate is enabled, disable VHT capability
         * to avoid FW assert.
         * */
        if (vhtrate_map == 0xffff) {
            wlan_clear_peer_flag(peer, WLAN_PEER_F_VHT);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_SPLITMAC,"add_client: clear VHT CAP\n");
        }
    } else {
        wlan_set_peer_rate_vht(peer, vhtrate_map);
    }

    if (wlan_peer_is_extflag_set(peer, IEEE80211_NODE_HE)) {
        /*
         * If HE rates are not enabled,
         * disable HE capability to avoid FW assert
         * */
        if (peer_caps->herates == 0xffff) {
            wlan_clear_peer_extflag(peer, IEEE80211_NODE_HE);
            mlme_err("add_client: clear HE CAP for vap[%d]",
                      wlan_vdev_get_id(vap->vdev_obj));
        } else
            wlan_set_peer_rate_he(peer, peer_caps->herates);
    }

#ifdef WLAN_FEATURE_11BE
    if (wlan_peer_is_extflag_set(peer, IEEE80211_NODE_EHT)) {
        wlan_set_peer_rate_eht(peer, peer_caps->ehtrates);
    }
#endif /* WLAN_FEATURE_11BE */

    /* FW assert observed when 11n/ac client joins but there is no
     * overlapping HT rate between client and AP.
     * */
    if (ni->ni_htrates.rs_nrates == 0) {
        wlan_clear_peer_flag(peer, IEEE80211_NODE_HT);
        wlan_clear_peer_flag(peer, IEEE80211_NODE_VHT);

        if (!IEEE80211_IS_CHAN_6GHZ(vap->iv_ic->ic_curchan)) {
            wlan_clear_peer_extflag(peer, IEEE80211_NODE_HE);
            ni->ni_chwidth = IEEE80211_CWM_WIDTH20; // set to 0 to be consistent with legacy
        }
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_SPLITMAC,"add_client: clear HT CAP\n");
    }
    /* make sure htcap and vhtcap, client phymode and chwidth are in sanity
     * state even if HT and VHT flag is not changed in this func, to avoid
     * FW assert
     * */
    if (wlan_peer_is_flag_set(peer, IEEE80211_NODE_HT) == 0) {
        if (!IEEE80211_IS_CHAN_6GHZ(vap->iv_ic->ic_curchan)) {
            ni->ni_streams = 1;
        }
        ni->ni_htcap = 0;
    }
    /* if no VHT rate is enabled, disable VHT capability to avoid FW assert */
    if (wlan_peer_is_flag_set(peer, IEEE80211_NODE_VHT) == 0) {
        ni->ni_vhtcap = 0;
    }

    /* if no HE rate is enabled, disable HE capability to avoid FW assert */
    if (wlan_peer_is_extflag_set(peer, IEEE80211_NODE_HE) == 0) {
        qdf_mem_zero(ni->ni_he.hecap_macinfo, sizeof(ni->ni_he.hecap_macinfo));
        qdf_mem_zero(ni->ni_he.hecap_phyinfo, sizeof(ni->ni_he.hecap_phyinfo));
    }
    /*
     * Temporarily set ni_chwidth to max_chwidth to get true chwidth
     * for client instead of that changed by Op Mode IE. This is to
     * ensure that correct client phymode is retained.
     */
    chwidth = ni->ni_chwidth;
    ni->ni_chwidth = ni->ni_max_chwidth;
    ieee80211_update_phymode(ni->ni_ic, ni);
    ni->ni_chwidth = chwidth;

    wlan_set_peer_aid(peer, peer_caps->associd);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_SPLITMAC,"add_client: ni_vhtcap=%d, ni_streams=%d, vhtrates=0x%x,"
                      "VHT rate map=0x%x, ni->ni_chwidth=%d\n, ni_max_streams=%d, ni_max_chwidth=%d",
                      ni->ni_vhtcap, ni->ni_streams, peer_caps->vhtrates, ni->ni_tx_vhtrates, ni->ni_chwidth,
                      ni->ni_max_streams, ni->ni_max_chwidth);

    wlan_peer_dump_rates(peer);

    return 0;
}

int wlan_add_client(struct wlan_objmgr_vdev *vdev,
                    struct wlan_objmgr_peer *peer,
                    struct wlan_addclient_peer_caps *peer_caps,
                    struct wlan_addclient_ml_info *ml_info)
{
    u_int8_t newassoc = 0;
    u_int16_t orig_associd = 0;
    wlan_if_t vap = NULL;
    struct ieee80211_node *ni = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni) {
        mlme_err("ni not found!\n");
        return -EINVAL;
    }

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap)
        return -EINVAL;

    if (IEEE80211_IS_CHAN_2GHZ(vap->iv_ic->ic_curchan) &&
        peer_caps->vhtrates != 0) {
        /* 2G doesn't support VHT */
        return -EINVAL;
    }

    orig_associd = wlan_get_aid(peer);
    if (orig_associd == 0) {
        newassoc = 1;
    }

    if (peer_caps->associd == 0) {
        mlme_err("Client ["QDF_MAC_ADDR_FMT"] AID is 0...check\n",
                 QDF_MAC_ADDR_REF(ni->ni_macaddr));
        return qdf_status_to_os_return(QDF_STATUS_INVALID_ASSOCID);
    }

    if (orig_associd != peer_caps->associd) {
        if (wlan_is_aid_set(vdev, peer_caps->associd)) {
            mlme_err("Client ["QDF_MAC_ADDR_FMT"] associd %d already in use.\n",
                     QDF_MAC_ADDR_REF(ni->ni_macaddr),
                     peer_caps->associd);
            return qdf_status_to_os_return(QDF_STATUS_E_EXISTS);
        }
    }

    if (orig_associd != 0) {
        mlme_info("Client ["QDF_MAC_ADDR_FMT"] replacing old associd %d with new id %d\n",
                  QDF_MAC_ADDR_REF(ni->ni_macaddr),
                  orig_associd,
                  peer_caps->associd);
        wlan_clear_aid(vdev, orig_associd, peer);
    } else {
        mlme_err("New AID is %d\n", peer_caps->associd);
    }
#ifdef WLAN_FEATURE_11BE_MLO
    /* wlan_add_client is always invoked on assoc peer, set
       peer as assoc peer
     */
    if (peer_caps->mlo)
        wlan_peer_mlme_set_assoc_peer(peer, true);
#endif

    wlan_override_client_params(vdev, peer, peer_caps);
    /* do mlme stuff for processing assoc req here */
    wlan_peer_mlme_recv_assoc_request(peer, newassoc, ml_info);

    return 0;
}

u_int16_t wlan_get_aid(struct wlan_objmgr_peer *peer)
{
    struct ieee80211_node *ni = NULL;

    ni = (wlan_node_t)wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        return -EINVAL;
    }
    return IEEE80211_AID(ni->ni_associd);
}
qdf_export_symbol(wlan_get_aid);

int wlan_is_aid_set(struct wlan_objmgr_vdev *vdev, u_int16_t associd)
{
    wlan_if_t vap;

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if (vap == NULL) {
        return -EINVAL;
    }
#ifndef WLAN_FEATURE_11BE_MLO
    return IEEE80211_AID_ISSET(vap, associd);
#else
    if (wlan_vdev_mlme_is_mlo_ap(vdev) || (vap->iv_ic->is_mbssid_enabled)) {
        return mlme_is_aid_set(vdev, associd);
    }
    else {
        return IEEE80211_AID_ISSET(vap, associd);
    }
#endif

}

void wlan_set_aid(struct wlan_objmgr_vdev *vdev, u_int16_t associd,
                  struct wlan_objmgr_peer *peer)
{
    wlan_if_t vap;

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if (vap == NULL) {
        return;
    }
#ifndef WLAN_FEATURE_11BE_MLO
    IEEE80211_AID_SET(vap, associd);
#else
    if (wlan_vdev_mlme_is_mlo_ap(vdev) || (vap->iv_ic->is_mbssid_enabled)) {
       if (wlan_peer_is_mlo(peer))
           mlo_set_aid(vdev, associd);
       else
           mlme_set_aid(vdev, associd);
    }
    else {
       IEEE80211_AID_SET(vap, associd);
    }
#endif
}

void wlan_clear_aid(struct wlan_objmgr_vdev *vdev, u_int16_t associd,
                    struct wlan_objmgr_peer *peer)
{
    wlan_if_t vap;

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if (vap == NULL) {
      return;
    }
#ifndef WLAN_FEATURE_11BE_MLO
    IEEE80211_AID_CLR(vap, associd);
#else
    if (wlan_vdev_mlme_is_mlo_ap(vdev) || (vap->iv_ic->is_mbssid_enabled)) {
       if (wlan_peer_is_mlo(peer))
           mlo_free_aid(vdev, associd);
       else
           mlme_free_aid(vdev, associd);
    }
    else {
       IEEE80211_AID_CLR(vap, associd);
    }
#endif
}

void wlan_set_peer_aid(struct wlan_objmgr_peer *peer, u_int16_t associd)
{
    struct ieee80211_node *ni = NULL;
    struct wlan_objmgr_vdev *vdev = wlan_peer_get_vdev(peer);
    wlan_if_t vap;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        return;
    }

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if (vap == NULL) {
      return;
    }

#ifndef WLAN_FEATURE_11BE_MLO
    ni->ni_associd = associd | IEEE80211_RESV_AID_BITS;
    wlan_set_aid(vdev, ni->ni_associd, peer);
#else
    if (wlan_vdev_mlme_is_mlo_ap(vdev) || (vap->iv_ic->is_mbssid_enabled)) {
        ni->ni_associd = associd;
    }
    else {
        ni->ni_associd = associd | IEEE80211_RESV_AID_BITS;
    }
     /* if peer is MLO, set aid for assoc peer only.
        for non-ML peer, set AID for peer
      */
    if (!wlan_peer_is_mlo(peer) || wlan_peer_mlme_is_assoc_peer(peer)) {
        wlan_set_aid(vdev, ni->ni_associd, peer);
    }
#endif

}

void wlan_set_peer_flag(struct wlan_objmgr_peer *peer, int flag)
{
    struct ieee80211_node *ni = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        return;
    }

    ni->ni_flags |= flag;
}

void wlan_clear_peer_flag(struct wlan_objmgr_peer *peer, int flag)
{
    struct ieee80211_node *ni = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        return;
    }

    ni->ni_flags &= ~flag;
}

int wlan_peer_is_flag_set(struct wlan_objmgr_peer *peer, int flag)
{
    struct ieee80211_node *ni = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        return -EINVAL;
    }

    return (ni->ni_flags & flag);
}

void wlan_set_peer_extflag(struct wlan_objmgr_peer *peer, int flag)
{
    struct ieee80211_node *ni = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        return;
    }

    ni->ni_ext_flags |= flag;
}

void wlan_clear_peer_extflag(struct wlan_objmgr_peer *peer, int flag)
{
    struct ieee80211_node *ni = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        return;
    }

    ni->ni_ext_flags &= ~flag;
}


int wlan_peer_is_extflag_set(struct wlan_objmgr_peer *peer, int flag)
{
    struct ieee80211_node *ni = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        return -EINVAL;
    }

    return (ni->ni_ext_flags & flag);
}

int wlan_get_peer_num_streams(struct wlan_objmgr_peer *peer)
{
    struct ieee80211_node *ni = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        return -EINVAL;
    }
    return ni->ni_streams;
}

void wlan_peer_auth(struct wlan_objmgr_vdev *vdev, u_int8_t *stamac, u_int32_t authorize)
{
    wlan_if_t vap;
    struct ieee80211_node *ni = NULL;

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if (vap == NULL) {
        return;
    }

    wlan_node_authorize(vap, authorize, stamac);

    ni = ieee80211_find_node(vap->iv_ic, stamac, WLAN_MLME_HANDLER_ID);
    if (ni == NULL) {
        return;
    }

    if(!authorize){
#ifdef WLAN_FEATURE_11BE_MLO
        if (wlan_peer_is_mlo(ni->peer_obj)) {
            wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
        }
#endif
        /* Call node leave so that its AID can be released and reused by
        * another client.
        */
        IEEE80211_NODE_LEAVE(ni);
    }
    ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);
}

void wlan_peer_mlme_recv_assoc_request(struct wlan_objmgr_peer *peer, int newassoc,
                                       struct wlan_addclient_ml_info *ml_info)
{
    struct ieee80211_node *ni = NULL;
    wbuf_t asreq_wbuf = NULL;
#ifdef WLAN_FEATURE_11BE_MLO
    uint8_t i = 0;
#endif

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        return;
    }

#ifdef WLAN_FEATURE_11BE_MLO
    if (ml_info) {
        if (ml_info->num_partner_links > (WLAN_UMAC_MLO_MAX_VDEVS - 1)) {
            mlme_err("num partner links(%d) are more than %d",
                     ml_info->num_partner_links, (WLAN_UMAC_MLO_MAX_VDEVS - 1));
            return;
        }

        if (ml_info->num_partner_links >= (WLAN_MAX_ML_BSS_LINKS - 1)) {
            mlme_err("num partner links(%d) are more than %d",
                     ml_info->num_partner_links, (WLAN_MAX_ML_BSS_LINKS- 1));
            return;
        }
         /* Copy ml info structure */
        qdf_mem_copy(&ni->addclient_ml_info, ml_info,
                     sizeof(struct wlan_addclient_ml_info));
        /* assign assoc req buffer */
        asreq_wbuf = ni->ni_asreq_buf;
         /* populate all links info */
        ni->ml_info.num_partner_links = 1;
        memcpy(ni->ml_info.partner_link_info[0].link_addr.bytes,
               ni->ni_macaddr, IEEE80211_ADDR_LEN);
        ni->ml_info.partner_link_info[0].link_id =
            wlan_vdev_get_link_id(wlan_peer_get_vdev(peer));
        for (i = 0; i < ml_info->num_partner_links; i++) {
             memcpy(ni->ml_info.partner_link_info[i+1].link_addr.bytes,
                  ml_info->partner_caps[i].stamac, IEEE80211_ADDR_LEN);
             ni->ml_info.partner_link_info[i+1].link_id =
                  ml_info->partner_caps[i].mlo_link_id;
             ni->ml_info.num_partner_links++;
        }
    }
#endif

    ieee80211_mlme_recv_assoc_request(ni, !newassoc, NULL, asreq_wbuf, NULL);
#ifdef WLAN_FEATURE_11BE_MLO
    if (ni->ni_asreq_buf) {
       wbuf_free(ni->ni_asreq_buf);
       ni->ni_asreq_buf = NULL;
    }
#endif
}

int wlan_vdev_del_key(struct wlan_objmgr_vdev *vdev, u_int16_t keyix, u_int8_t *macaddr)
{
    wlan_if_t vap;

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if (vap == NULL) {
        return -EINVAL;
    }

    wlan_del_key(vap, keyix, macaddr);

    return 0;
}

#define RXMIC_OFFSET 8
int wlan_vdev_set_key(struct wlan_objmgr_vdev *vdev, u_int8_t *macaddr, u_int8_t cipher,
                      u_int16_t keyix, u_int32_t keylen, u_int8_t *keydata)
{
    ieee80211_keyval key_val;
    wlan_if_t vap;

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if (vap == NULL) {
        return -EINVAL;
    }

    if (keylen > IEEE80211_KEYBUF_SIZE+IEEE80211_MICBUF_SIZE) {
        return -EINVAL;
    }

    if ((keyix != IEEE80211_KEYIX_NONE) &&
        (keyix >= IEEE80211_WEP_NKID) && (cipher != IEEE80211_CIPHER_AES_CMAC)) {
            return -EINVAL;
    }

    qdf_mem_zero(&key_val, sizeof(ieee80211_keyval));

    key_val.keylen  = keylen;
    if (key_val.keylen > IEEE80211_KEYBUF_SIZE) {
        key_val.keylen  = IEEE80211_KEYBUF_SIZE;
    }
    key_val.rxmic_offset = IEEE80211_KEYBUF_SIZE + RXMIC_OFFSET;
    key_val.txmic_offset =  IEEE80211_KEYBUF_SIZE;
    key_val.keytype = cipher;
    key_val.macaddr = macaddr;
    key_val.keydata = keydata;

    /* allow keys to allocate anywhere in key cache */
    wlan_set_param(vap, IEEE80211_WEP_MBSSID, 1);
    wlan_set_key(vap, keyix, &key_val);
    wlan_set_param(vap, IEEE80211_WEP_MBSSID, 0);  /* put it back to default */

    /* Zero-out local key variables */
    qdf_mem_zero(&key_val, sizeof(ieee80211_keyval));
    return 0;
}

u_int32_t wlan_ucfg_get_maxphyrate(struct wlan_objmgr_vdev *vdev)
{
	wlan_if_t vap;

	vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
	if (vap == NULL) {
		return 0;
	}

	return ieee80211_ucfg_get_maxphyrate(vap);
}

void wlan_get_bw_and_behav_limit(struct ieee80211_ath_channel *chan,
				 uint16_t *chan_width,
				 uint16_t *behav_limit)
{
	if (IEEE80211_IS_CHAN_QUARTER(chan)) {
		*chan_width = BW_5_MHZ;
		*behav_limit = BIT(BEHAV_NONE);
	} else if (IEEE80211_IS_CHAN_HALF(chan)){
		*chan_width = BW_10_MHZ;
		*behav_limit = BIT(BEHAV_NONE);
	}

	switch (ieee80211_chan2mode(chan)) {
#ifdef WLAN_FEATURE_11BE
		case IEEE80211_MODE_11BEA_EHT320:
			*chan_width = BW_320_MHZ;
			*behav_limit = BIT(BEHAV_NONE);
			break;
#endif /* WLAN_FEATURE_11BE */
		case IEEE80211_MODE_11AXA_HE160:
		case IEEE80211_MODE_11AC_VHT160:
#ifdef WLAN_FEATURE_11BE
		case IEEE80211_MODE_11BEA_EHT160:
#endif /* WLAN_FEATURE_11BE */
			*chan_width = BW_160_MHZ;
			*behav_limit = BIT(BEHAV_NONE);
			break;
		case IEEE80211_MODE_11AXA_HE80_80:
		case IEEE80211_MODE_11AC_VHT80_80:
			*chan_width = BW_80_MHZ;
			*behav_limit = BIT(BEHAV_BW80_PLUS);
			break;
#ifdef WLAN_FEATURE_11BE
		case IEEE80211_MODE_11BEA_EHT80:
#endif /* WLAN_FEATURE_11BE */
		case IEEE80211_MODE_11AXA_HE80:
		case IEEE80211_MODE_11AC_VHT80:
			*chan_width = BW_80_MHZ;
			*behav_limit = BIT(BEHAV_NONE);
			break;
#ifdef WLAN_FEATURE_11BE
		case IEEE80211_MODE_11BEA_EHT40PLUS:
		case IEEE80211_MODE_11BEG_EHT40PLUS:
#endif /* WLAN_FEATURE_11BE */
		case IEEE80211_MODE_11AXA_HE40PLUS:
		case IEEE80211_MODE_11AXG_HE40PLUS:
		case IEEE80211_MODE_11AC_VHT40PLUS:
		case IEEE80211_MODE_11NA_HT40PLUS:
		case IEEE80211_MODE_11NG_HT40PLUS:
			*chan_width = BW_40_MHZ;
			if (wlan_reg_is_6ghz_chan_freq(chan->ic_freq))
				*behav_limit = BIT(BEHAV_NONE);
			else
				*behav_limit = BIT(BEHAV_BW40_LOW_PRIMARY);
			break;
#ifdef WLAN_FEATURE_11BE
		case IEEE80211_MODE_11BEA_EHT40MINUS:
		case IEEE80211_MODE_11BEG_EHT40MINUS:
#endif /* WLAN_FEATURE_11BE */
		case IEEE80211_MODE_11AXA_HE40MINUS:
		case IEEE80211_MODE_11AXG_HE40MINUS:
		case IEEE80211_MODE_11AC_VHT40MINUS:
		case IEEE80211_MODE_11NA_HT40MINUS:
		case IEEE80211_MODE_11NG_HT40MINUS:
			*chan_width = BW_40_MHZ;
			if (wlan_reg_is_6ghz_chan_freq(chan->ic_freq))
				*behav_limit = BIT(BEHAV_NONE);
			else
				*behav_limit = BIT(BEHAV_BW40_HIGH_PRIMARY);
			break;
#ifdef WLAN_FEATURE_11BE
		case IEEE80211_MODE_11BEA_EHT40:
#endif /* WLAN_FEATURE_11BE */
		case IEEE80211_MODE_11AXA_HE40:
		case IEEE80211_MODE_11AXG_HE40:
		case IEEE80211_MODE_11NA_HT40:
		case IEEE80211_MODE_11NG_HT40:
			*chan_width = BW_40_MHZ;
			*behav_limit = BIT(BEHAV_NONE);
			break;
#ifdef WLAN_FEATURE_11BE
		case IEEE80211_MODE_11BEA_EHT20:
#endif /* WLAN_FEATURE_11BE */
		case IEEE80211_MODE_11AXA_HE20:
		case IEEE80211_MODE_11NA_HT20:
		case IEEE80211_MODE_11AC_VHT20:
			*chan_width = BW_20_MHZ;
			*behav_limit = BIT(BEHAV_NONE);
			break;
#ifdef WLAN_FEATURE_11BE
		case IEEE80211_MODE_11BEG_EHT20:
#endif /* WLAN_FEATURE_11BE */
		case IEEE80211_MODE_11AXG_HE20:
		case IEEE80211_MODE_11NG_HT20:
		case IEEE80211_MODE_11B:
		case IEEE80211_MODE_11G:
			*chan_width = BW_25_MHZ;
			*behav_limit = BIT(BEHAV_NONE);
			break;
		default:
			*chan_width = BW_20_MHZ;
			*behav_limit = BIT(BEHAV_NONE);
			break;
        }
}
qdf_export_symbol(wlan_get_bw_and_behav_limit);

uint8_t wlan_get_opclass(struct wlan_objmgr_vdev *vdev)
{
	wlan_if_t vap;
	struct ieee80211com *ic;
	uint16_t chan_width, behav_limit;
	char country_iso[4];
	uint8_t opclass = 0;

	vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);

	if (vap == NULL) {
		qdf_err("vap is NULL");
		return 0;
	}

	ic = vap->iv_ic;

	if (ic == NULL) {
		qdf_err("ic is NULL");
		return 0;
	}

	wlan_get_bw_and_behav_limit(ic->ic_curchan, &chan_width, &behav_limit);
	ieee80211_getCurrentCountryISO(vap->iv_ic, country_iso);
	opclass = wlan_reg_get_opclass_from_freq_width(country_iso,
						       ic->ic_curchan->ic_freq,
						       chan_width,
						       behav_limit);
	if (!opclass)
		qdf_err("Operating class is 0");

	return opclass;
}
qdf_export_symbol(wlan_get_opclass);

uint8_t wlan_get_opclass_from_channel(struct wlan_objmgr_vdev *vdev,
                                      struct ieee80211_ath_channel *channel)
{
	wlan_if_t vap;
	char country_iso[4];
	uint16_t chan_width, behav_limit;
	uint8_t opclass = 0;

	if(!IEEE80211_IS_CHAN_VALID(channel)) {
		qdf_err("channel is NULL");
		return 0;
	}

	vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
	if (vap == NULL) {
		qdf_err("vap is NULL");
		return 0;
	}

	wlan_get_bw_and_behav_limit(channel, &chan_width, &behav_limit);
	ieee80211_getCurrentCountryISO(vap->iv_ic, country_iso);
	opclass = wlan_reg_get_opclass_from_freq_width(country_iso,
						       channel->ic_freq,
						       chan_width,
						       behav_limit);

	if (!opclass)
		qdf_err("Operating class is 0");

	return opclass;
}

uint8_t wlan_get_prim_chan(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_channel *chan = NULL;

	if (vdev == NULL) {
		QDF_BUG(0);
		return 0;
	}
	chan = wlan_vdev_mlme_get_bss_chan(vdev);

	if (chan == NULL) {
		return 0;
	}

	return (chan->ch_ieee);
}

uint8_t wlan_get_seg1(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_channel *chan = NULL;

	if (vdev == NULL) {
		QDF_BUG(0);
		return 0;
	}
	chan = wlan_vdev_mlme_get_bss_chan(vdev);

	if (chan == NULL) {
		return 0;
	}

	return (chan->ch_freq_seg2);
}

uint16_t wlan_peer_get_beacon_interval(struct wlan_objmgr_peer *peer)
{
	struct ieee80211_node *ni;

	if (peer == NULL)
		return 0;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (ni == NULL)
		return 0;

	return ieee80211_node_get_beacon_interval(ni);
}
qdf_export_symbol(wlan_peer_get_beacon_interval);

QDF_STATUS wlan_vdev_get_elemid(struct wlan_objmgr_vdev *vdev,
				ieee80211_frame_type ftype, uint8_t *iebuf,
				uint32_t *ielen, uint32_t elem_id)
{
	struct ieee80211vap *vap;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (vdev == NULL)
		return QDF_STATUS_E_INVAL;

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (vap == NULL)
		return QDF_STATUS_E_INVAL;

	if (wlan_mlme_app_ie_get_elemid(vap, ftype, iebuf,
                ielen, elem_id)) {
		status = QDF_STATUS_E_FAILURE;
	}

	return status;
}

void wlan_set_node_peer_flag(struct wlan_objmgr_peer *peer, u_int32_t flag)
{
	struct ieee80211_node *ni;

	if (peer == NULL)
		return;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (ni == NULL)
		return;
	return ieee80211node_set_flag(ni, flag);
}

void wlan_clear_node_peer_flag(struct wlan_objmgr_peer *peer, u_int32_t flag)
{
	struct ieee80211_node *ni;

	if (peer == NULL)
		return;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (ni == NULL)
		return;

	return ieee80211node_clear_flag(ni, flag);
}

bool wlan_vdev_is_delete_in_progress(struct wlan_objmgr_vdev *vdev)
{
	wlan_if_t vap = NULL;

	vap = wlan_vdev_get_vap(vdev);
	if (!vap)
		return false;

	return wlan_vap_delete_in_progress(vap);
}

struct ieee80211_ath_channel
wlan_vdev_get_current_channel(struct wlan_objmgr_vdev *vdev,
                              bool hwChan)
{
	wlan_if_t vap = NULL;

	vap = wlan_vdev_get_vap(vdev);
	if (!vap)
		return empty_channel;

	return wlan_get_current_channel(vap, hwChan);
}

void wlan_update_hifitbl(struct wlan_objmgr_vdev *vdev, void *msg, u_int32_t len, u_int32_t flag)
{
	wlan_if_t vap = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	ol_txrx_soc_handle soc = NULL;
	struct ieee80211com *ic = NULL;
	osif_dev* osifp = NULL;
	rwlock_t *me_lock;

	vap = wlan_vdev_get_vap(vdev);
	if (!vap)
		return;

	ic = vap->iv_ic;
	if (!ic)
		return;

	osifp = (osif_dev *)vap->iv_ifp;
	if (!osifp)
		return;

	pdev = wlan_vdev_get_pdev(vdev);
	if (!pdev)
		return;

	soc = wlan_psoc_get_dp_handle(wlan_vdev_get_psoc(vdev));
	if (!soc)
		return;

	me_lock = dp_get_me_mcast_lock(soc, wlan_vdev_get_id(vdev), flag);
	if(!me_lock)
		return;

	write_lock_bh(me_lock);
	dp_me_update_mcast_table(soc, wlan_vdev_get_id(vdev),
				 wlan_objmgr_pdev_get_pdev_id(pdev), msg,
				 len, 0, flag);

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
	if (osifp->nss_wifiol_ctx && ic->nss_vops &&
	    dp_get_me_mode(soc, wlan_vdev_get_id(vdev))) {
#if ATH_SUPPORT_HYFI_ENHANCEMENTS
		ic->nss_vops->ic_osif_nss_vdev_me_reset_snooplist(osifp);
		ic->nss_vops->ic_osif_nss_vdev_me_update_hifitlb(osifp,
				 dp_get_vdev_me_handle(soc, wlan_vdev_get_id(vdev)));
#endif
	}
#endif

	write_unlock_bh(me_lock);
}

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT

void wlan_vdev_nss_ol_psta_add(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211com *ic = NULL;
    wlan_if_t vap = NULL;
       osif_dev* osifp = NULL;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;
    osifp = (osif_dev *)vap->iv_ifp;

    if (!osifp)
        return;
    ic = vap->iv_ic;
    if (osifp->osif_is_mode_offload && osifp->nss_wifiol_ctx) {
        if (ic->nss_vops) {
            ic->nss_vops->ic_osif_nss_vdev_psta_add_entry(osifp);
        }
    }
       return;
}
qdf_export_symbol(wlan_vdev_nss_ol_psta_add);

void wlan_vdev_nss_ol_psta_delete(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211com *ic = NULL;
    wlan_if_t vap = NULL;
       osif_dev* osifp = NULL;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;
    osifp = (osif_dev *)vap->iv_ifp;

    if (!osifp)
        return;
    ic = vap->iv_ic;
    if (osifp->osif_is_mode_offload && osifp->nss_wifiol_ctx) {
        if (ic->nss_vops) {
            ic->nss_vops->ic_osif_nss_vdev_psta_delete_entry(osifp);
        }
    }
    return;
}
qdf_export_symbol(wlan_vdev_nss_ol_psta_delete);

void wlan_vdev_nss_ol_set_cfg(struct wlan_objmgr_vdev *vdev, int cmd)
{
    struct ieee80211com *ic = NULL;
    wlan_if_t vap = NULL;
       osif_dev* osifp = NULL;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;
    osifp = (osif_dev *)vap->iv_ifp;

    if (!osifp)
        return;
    ic = vap->iv_ic;
    if (ic->nss_vops) {
        ic->nss_vops->ic_osif_nss_vdev_set_cfg(osifp, cmd);
    }
    return;
}
qdf_export_symbol(wlan_vdev_nss_ol_set_cfg);
#endif

void wlan_vdev_set_powersave(struct wlan_objmgr_vdev *vdev, int val)
{
    wlan_if_t vap = NULL;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;
    wlan_set_powersave(vap, val);
    return;
}
qdf_export_symbol(wlan_vdev_set_powersave);

void wlan_vdev_set_param(struct wlan_objmgr_vdev *vdev, ieee80211_param param, u_int32_t val)
{
    wlan_if_t vap = NULL;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;
    wlan_set_param(vap, param, val);
    return;
}
qdf_export_symbol(wlan_vdev_set_param);

void wlan_vdev_pwrsave_force_sleep(struct wlan_objmgr_vdev *vdev, bool val)
{
    wlan_if_t vap = NULL;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;
    wlan_pwrsave_force_sleep(vap, val);
    return;
}
qdf_export_symbol(wlan_vdev_pwrsave_force_sleep);

int wlan_send_wds_cmd(struct wlan_objmgr_vdev *vdev, uint8_t val)
{
    struct ieee80211com *ic = NULL;

    ic = wlan_vdev_get_ic(vdev);
    if (!ic)
        return -EINVAL;

    return ic->ic_send_wds_cmd(vdev, val);
}
qdf_export_symbol(wlan_send_wds_cmd);

struct wlan_objmgr_vdev *
wlan_get_stavap(struct wlan_objmgr_pdev *pdev)
{
    struct ieee80211com *ic = NULL;
    struct ieee80211vap *stavap = NULL;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic)
        return NULL;

    stavap = (struct ieee80211vap *)ic->ic_sta_vap;
    if (!stavap)
        return NULL;

    return stavap->vdev_obj;
}
qdf_export_symbol(wlan_get_stavap);

void
wlan_peer_disassoc(struct wlan_objmgr_peer *peer)
{
    struct ieee80211_node *ni = NULL;
    uint8_t peer_del_req_send = MLME_INDIVIDUAL_PEER_CLEANUP;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni)
        return;

    sta_disassoc(&peer_del_req_send, ni);
    return;
}
qdf_export_symbol(wlan_peer_disassoc);

void
wlan_pdev_update_beacon(struct wlan_objmgr_pdev *pdev)
{
    struct ieee80211com *ic = NULL;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic)
        return;

    wlan_pdev_beacon_update(ic);
    return;
}
qdf_export_symbol(wlan_pdev_update_beacon);

bool
wlan_dessired_ssid_found(struct wlan_objmgr_vdev *vdev, u_int8_t *ssid, u_int8_t ssid_len)
{
    wlan_if_t vap = NULL;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return 0;

    if(ieee80211_vap_match_ssid(vap, ssid, ssid_len))
        return 1;

    return 0;
}

bool
wlan_target_lithium(struct wlan_objmgr_pdev *pdev)
{
    struct ieee80211com *ic = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic)
        return NULL;
    psoc = wlan_pdev_get_psoc(pdev);
    return ic->ic_is_target_lithium(psoc);
}
qdf_export_symbol(wlan_target_lithium);

#if DBDC_REPEATER_SUPPORT
void
wlan_delay_stavap_conn_process_up(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211com *ic = NULL;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    struct ieee80211com *tmp_ic = NULL;
    int j;
#endif
    wlan_if_t vap = NULL;
    struct ieee80211_mlme_priv *mlme_priv = NULL;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;

    ic = vap->iv_ic;
    mlme_priv = vap->iv_mlme_priv;

    GLOBAL_IC_LOCK_BH(ic->ic_global_list);
    qca_multi_link_set_drop_sec_mcast(true);

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    for (j = 0; j < MAX_RADIO_CNT; j++) {
        tmp_ic = ic->ic_global_list->global_ic[j];
        if (tmp_ic) {
            if (tmp_ic->nss_radio_ops)
                tmp_ic->nss_radio_ops->ic_nss_ol_set_drop_secondary_mcast(tmp_ic, 1);
        }
    }
#endif
    ic->ic_global_list->num_l2uf_retries = 0;
    GLOBAL_IC_UNLOCK_BH(ic->ic_global_list);
    qdf_timer_mod(&mlme_priv->im_drop_mcast_timer,((ic->ic_global_list->delay_stavap_connection*1000/MAX_L2UF_RETIRES)));

    return;
}
qdf_export_symbol(wlan_delay_stavap_conn_process_up);

void
wlan_delay_stavap_conn_process_down(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211com *ic = NULL;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    struct ieee80211com *tmp_ic = NULL;
    int i;
#endif
    wlan_if_t vap = NULL;
    struct ieee80211_mlme_priv *mlme_priv = NULL;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;

    ic = vap->iv_ic;
    mlme_priv = vap->iv_mlme_priv;

    GLOBAL_IC_LOCK_BH(ic->ic_global_list);
    qca_multi_link_set_drop_sec_mcast(false);

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    for (i = 0; i < MAX_RADIO_CNT; i++) {
        tmp_ic = ic->ic_global_list->global_ic[i];
        if (tmp_ic) {
            if (tmp_ic->nss_radio_ops)
                tmp_ic->nss_radio_ops->ic_nss_ol_set_drop_secondary_mcast(tmp_ic, 0);
        }
    }
#endif
    GLOBAL_IC_UNLOCK_BH(ic->ic_global_list);
    qdf_timer_stop(&mlme_priv->im_drop_mcast_timer);

    return;
}
qdf_export_symbol(wlan_delay_stavap_conn_process_down);

void
wlan_dbdc_process_mac_db_up(struct wlan_objmgr_vdev *vdev, uint8_t num_sta)
{
    struct ieee80211com *ic = NULL;
    struct ieee80211com *tmp_ic = NULL;
    wlan_if_t vap = NULL;
    wlan_if_t tmp_stavap = NULL;
    osif_dev* osifp = NULL;
    int i;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;

    osifp = (osif_dev *)vap->iv_ifp;
    if (!osifp)
        return;

    ic = vap->iv_ic;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (num_sta > 1) {
        if (ic->nss_vops) {
            if (!ic->nss_vops->ic_osif_nss_vdev_wifi_mac_db_pool_entries_send(osifp)) {
                qdf_info("\n%p: Failed to send Wi-Fi MAC database entries to NSS",ic);
            }
        }
    }

    if (ic->nss_radio_ops) {
        ic->nss_radio_ops->ic_nss_ol_enable_dbdc_process(ic, 1);
    }
#endif
    for (i=0; i < MAX_RADIO_CNT; i++) {
        tmp_ic = ic->ic_global_list->global_ic[i];
        if (tmp_ic) {
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
             if (tmp_ic->nss_radio_ops) {
                 tmp_ic->nss_radio_ops->ic_nss_ol_enable_dbdc_process(tmp_ic, 1);
             }
#endif
             tmp_stavap = tmp_ic->ic_sta_vap;
             if (tmp_stavap && wlan_is_connected(tmp_stavap)) {
                 IEEE80211_DELIVER_EVENT_STA_VAP_CONNECTION_UPDATE(tmp_stavap);
             }
        }
    }

    return;
}
qdf_export_symbol(wlan_dbdc_process_mac_db_up);
#endif

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
void
wlan_nss_dbdc_process_mac_db_down(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211com *ic = NULL;
    struct ieee80211com *tmp_ic = NULL;
    wlan_if_t vap = NULL;
    osif_dev* osifp = NULL;
    struct ol_ath_softc_net80211 *tmp_scn = NULL;
    int i;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;

    osifp = (osif_dev *)vap->iv_ifp;
    if (!osifp)
        return;

    ic = vap->iv_ic;

    for (i=0; i < MAX_RADIO_CNT; i++) {
        tmp_ic = ic->ic_global_list->global_ic[i];
        if (tmp_ic) {
            tmp_scn = OL_ATH_SOFTC_NET80211(tmp_ic);
            spin_lock(&tmp_ic->ic_lock);
            if (tmp_ic->nss_vops)
                tmp_ic->nss_vops->ic_osif_nss_enable_dbdc_process(tmp_ic, 0);
            spin_unlock(&tmp_ic->ic_lock);
        }
    }

    if (ic->nss_vops) {
        ic->nss_vops->osif_nss_li_vdev_wifi_mac_db_reset_state(osifp);
    }

    if (ic->nss_radio_ops) {
        ic->nss_radio_ops->ic_nss_ol_enable_dbdc_process(ic, 0);
    }
    return;
}
qdf_export_symbol(wlan_nss_dbdc_process_mac_db_down);

void
wlan_nss_prep_mac_db_store_stavap(struct wlan_objmgr_vdev *vdev, uint8_t num_sta)
{
    struct ieee80211com *ic = NULL;
    wlan_if_t vap = NULL;
    osif_dev* osifp = NULL;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;

    osifp = (osif_dev *)vap->iv_ifp;
    if (!osifp)
        return;

    ic = vap->iv_ic;

    if (num_sta == 1) {
        if (ic->nss_vops) {
            ic->nss_vops->ic_osif_nss_vdev_prepare_wifi_mac_db(osifp);
        }
    }
#if DBDC_REPEATER_SUPPORT
    if (ic->ic_global_list->num_stavaps_up > 1) {
        if (ic->nss_vops) {
            ic->nss_vops->ic_osif_nss_store_other_pdev_stavap(ic);
        }
    }
#endif
    return;
}
qdf_export_symbol(wlan_nss_prep_mac_db_store_stavap);
#endif

#if DBDC_REPEATER_SUPPORT
void
wlan_act_update_force_cli_mcast_process_up(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211com *ic = NULL;
    struct ieee80211com *tmp_ic = NULL;
    struct ieee80211com *fastlane_ic = NULL;
    wlan_if_t vap = NULL;
    wlan_if_t tmp_vap = NULL;
    int i;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;

    ic = wlan_vdev_get_ic(vdev);
    if (!ic)
        return;

    osif_radio_activity_update(OL_ATH_SOFTC_NET80211(ic));
#if ATH_SUPPORT_WRAP
    if (ic->ic_global_list->force_client_mcast_traffic && (dp_wrap_get_mpsta_vdev(ic->ic_pdev_obj) == NULL)) {
#else
    if (ic->ic_global_list->force_client_mcast_traffic) {
#endif
    /* Do this for non-qwrap repeater modes , when always_primary flag is not set
           In case of qwrap mode, wrapd will send command to disassoc clients*/
        if (qca_multi_link_get_always_primary() == 0) {
            TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
                if ((tmp_vap->iv_opmode == IEEE80211_M_HOSTAP) &&
                    (wlan_vdev_is_up(tmp_vap->vdev_obj) == QDF_STATUS_SUCCESS)) {
                    wlan_iterate_station_list(tmp_vap, sta_disassoc, NULL);
                }
            }
            fastlane_ic = ic->fast_lane_ic;
            if (fastlane_ic) {
                TAILQ_FOREACH(tmp_vap, &fastlane_ic->ic_vaps, iv_next) {
                    if ((tmp_vap->iv_opmode == IEEE80211_M_HOSTAP) &&
                        (wlan_vdev_is_up(tmp_vap->vdev_obj) == QDF_STATUS_SUCCESS)) {
                        wlan_iterate_station_list(tmp_vap, sta_disassoc, NULL);
                    }
                }
            }
        } else {
            if (vap == ic->ic_global_list->max_priority_stavap_up) {
                for (i=0; i < MAX_RADIO_CNT; i++) {
                    tmp_ic = ic->ic_global_list->global_ic[i];
                    if (tmp_ic) {
                        TAILQ_FOREACH(tmp_vap, &tmp_ic->ic_vaps, iv_next) {
                            if ((tmp_vap->iv_opmode == IEEE80211_M_HOSTAP) &&
                                (wlan_vdev_is_up(tmp_vap->vdev_obj) == QDF_STATUS_SUCCESS)) {
                                wlan_iterate_station_list(tmp_vap, sta_disassoc, NULL);
                            }
                        }
                    }
                }
            }
        }
    }

    return;
}
qdf_export_symbol(wlan_act_update_force_cli_mcast_process_up);

void
wlan_force_cli_mcast_process_down(struct wlan_objmgr_vdev *vdev,
                                  bool *max_priority_stavap_disconnected)
{
    struct ieee80211com *ic = NULL;
    struct ieee80211com *tmp_ic = NULL;
    struct ieee80211com *fastlane_ic = NULL;
    wlan_if_t vap = NULL;
    wlan_if_t tmp_vap = NULL;
    int i;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;

    ic = wlan_vdev_get_ic(vdev);
    if (!ic)
        return;

#if ATH_SUPPORT_WRAP
    if (ic->ic_global_list->force_client_mcast_traffic && (dp_wrap_get_mpsta_vdev(ic->ic_pdev_obj) == NULL)) {
#else
    if (ic->ic_global_list->force_client_mcast_traffic) {
#endif
        /* Do this for non-qwrap repeater modes , when always_primary flag is not set
           In case of qwrap mode, wrapd will send command to disassoc clients*/
        if (qca_multi_link_get_always_primary() == 0) {
            TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
                if ((tmp_vap->iv_opmode == IEEE80211_M_HOSTAP) &&
                   (wlan_vdev_is_up(tmp_vap->vdev_obj) == QDF_STATUS_SUCCESS)) {
                    wlan_iterate_station_list(tmp_vap, sta_disassoc, NULL);
                }
            }
            fastlane_ic = ic->fast_lane_ic;
            if (fastlane_ic) {
                TAILQ_FOREACH(tmp_vap, &fastlane_ic->ic_vaps, iv_next) {
                    if ((tmp_vap->iv_opmode == IEEE80211_M_HOSTAP) &&
                        (wlan_vdev_is_up(tmp_vap->vdev_obj) == QDF_STATUS_SUCCESS)) {
                        wlan_iterate_station_list(tmp_vap, sta_disassoc, NULL);
                    }
                }
            }
        } else {
            if (*max_priority_stavap_disconnected) {
                for (i=0; i < MAX_RADIO_CNT; i++) {
                    tmp_ic = ic->ic_global_list->global_ic[i];
                    if (tmp_ic) {
                        TAILQ_FOREACH(tmp_vap, &tmp_ic->ic_vaps, iv_next) {
                            if ((tmp_vap->iv_opmode == IEEE80211_M_HOSTAP) &&
                                (wlan_vdev_is_up(tmp_vap->vdev_obj) == QDF_STATUS_SUCCESS)) {
                                wlan_iterate_station_list(tmp_vap, sta_disassoc, NULL);
                            }
                        }
                    }
                }
            }
        }
    }

    return;
}
qdf_export_symbol(wlan_force_cli_mcast_process_down);
#endif

void
wlan_rptr_send_event(struct wlan_objmgr_vdev *vdev, uint8_t *ev_data, uint32_t ev_len, uint8_t ev_flags)
{
    wlan_if_t vap = NULL;
    osif_dev* osifp = NULL;
    union iwreq_data wreq;
    struct net_device *dev = NULL;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;

    osifp = (osif_dev *)vap->iv_ifp;
    if (!osifp)
        return;

    if ((osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM, IEEE80211_EV_PREFERRED_BSSID))
        || osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM, IEEE80211_EV_PREFERRED_MLO_BSSID)) {
        dev = OSIF_TO_NETDEV(osifp);
        memset(&wreq, 0, sizeof(wreq));
        wreq.data.flags = ev_flags;
        wreq.data.length = ev_len;
        WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wreq, ev_data);
    }
    return;
}
qdf_export_symbol(wlan_rptr_send_event);

#if DBDC_REPEATER_SUPPORT
void
wlan_max_pri_stavap_process_up(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211com *ic = NULL;
    struct ieee80211com *max_priority_ic = NULL;
    wlan_if_t vap = NULL;
    wlan_if_t *max_priority_stavap_up;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;

    ic = wlan_vdev_get_ic(vdev);
    if (!ic)
        return;

    GLOBAL_IC_LOCK_BH(ic->ic_global_list);

    if (!ic->ic_global_list->dbdc_process_enable) {
        GLOBAL_IC_UNLOCK_BH(ic->ic_global_list);
        return;
    }

    max_priority_stavap_up = &ic->ic_global_list->max_priority_stavap_up;

    if ((*max_priority_stavap_up == NULL) ||
        (ieee80211_vap_deleted_is_set(*max_priority_stavap_up))) {
        *max_priority_stavap_up = vap;
    }

    max_priority_ic = (*max_priority_stavap_up)->iv_ic;
    /* max_priority_stavap_up or vap might be set as deleted at this point
     * confirm once before updating max_priority_stavap_up */
    if (qdf_unlikely(!max_priority_ic)) {
        qdf_warn("vap-%d(%p) iv_ic is NULL",
            (*max_priority_stavap_up)->iv_unit, *max_priority_stavap_up);
    } else if (qdf_unlikely(ieee80211_vap_deleted_is_set(*max_priority_stavap_up))) {
        qdf_warn("vap-%d(%p) set to deleted state",
            (*max_priority_stavap_up)->iv_unit, *max_priority_stavap_up);
    } else if (max_priority_ic->ic_radio_priority > ic->ic_radio_priority) {
        *max_priority_stavap_up = vap;
    }

    GLOBAL_IC_UNLOCK_BH(ic->ic_global_list);
    return;
}
qdf_export_symbol(wlan_max_pri_stavap_process_up);

void
wlan_max_pri_stavap_process_down(struct wlan_objmgr_vdev *vdev,
                                 bool *max_priority_stavap_disconnected)
{
    struct ieee80211com *ic = NULL;
    struct ieee80211com *tmp_ic = NULL;
    wlan_if_t vap = NULL;
    wlan_if_t tmp_stavap = NULL;
    struct ieee80211_mlme_priv *tmp_mlme_priv = NULL;
    u_int8_t max_stavap_up_priority;
    int i;

    max_stavap_up_priority = MAX_RADIO_CNT + 1;
    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return;

    ic = wlan_vdev_get_ic(vdev);
    if (!ic)
        return;

    GLOBAL_IC_LOCK_BH(ic->ic_global_list);

    if (!ic->ic_global_list->dbdc_process_enable) {
        GLOBAL_IC_UNLOCK_BH(ic->ic_global_list);
        return;
    }

    if (ic->ic_global_list->max_priority_stavap_up == NULL) {
         ic->ic_global_list->max_priority_stavap_up = vap;
    }

    if (vap == ic->ic_global_list->max_priority_stavap_up) {
        for (i=0; i < MAX_RADIO_CNT; i++) {
            tmp_ic = ic->ic_global_list->global_ic[i];
            if (tmp_ic && (tmp_ic != ic)) {
                tmp_stavap = tmp_ic->ic_sta_vap;
                if (tmp_stavap) {
                    tmp_mlme_priv = tmp_stavap->iv_mlme_priv;
                    if (tmp_mlme_priv->im_connection_up == 1) {
                        *max_priority_stavap_disconnected = 1;
                        if (max_stavap_up_priority > tmp_ic->ic_radio_priority) {
                            ic->ic_global_list->max_priority_stavap_up = tmp_ic->ic_sta_vap;
                            max_stavap_up_priority = tmp_ic->ic_radio_priority;
                        }
                    }
                }
            }
        }
    }

    GLOBAL_IC_UNLOCK_BH(ic->ic_global_list);
    return;
}
qdf_export_symbol(wlan_max_pri_stavap_process_down);

void
wlan_legacy_dbdc_flags_get(struct wlan_objmgr_pdev *pdev, struct dbdc_flags *flags)
{
    struct ieee80211com *ic = NULL;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic)
        return;

    GLOBAL_IC_LOCK_BH(ic->ic_global_list);

    flags->dbdc_process_enable = ic->ic_global_list->dbdc_process_enable;
    flags->delay_stavap_connection = ic->ic_global_list->delay_stavap_connection;

    GLOBAL_IC_UNLOCK_BH(ic->ic_global_list);
    return;
}
qdf_export_symbol(wlan_legacy_dbdc_flags_get);
#endif

#if QCA_SUPPORT_SON
static inline enum ieee80211_acl_flag convert_acl_flag(wlan_acl_flag flag)
{
    switch (flag)
    {
    case WLAN_ACL_FLAG_PROBE_RESP_WH:
        return IEEE80211_ACL_FLAG_PROBE_RESP_WH;
    case WLAN_ACL_FLAG_AUTH_ALLOW:
        return IEEE80211_ACL_FLAG_AUTH_ALLOW;
    default:
        return 0;
    }
    return 0;
}

QDF_STATUS wlan_vdev_ops(struct wlan_objmgr_vdev *vdev,
                         enum wlan_mlme_vdev_param type, void *data, void *ret)
{
    union wlan_mlme_vdev_data *in = (union wlan_mlme_vdev_data *)data;
    union wlan_mlme_vdev_data *out = (union wlan_mlme_vdev_data *)ret;
    enum ieee80211_acl_flag acl_flag;
    ieee80211_ssid *ssid;
    struct ieee80211_ath_channel chan_obj;
    wlan_chan_t chan = &chan_obj;
    wlan_if_t vap;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    struct ieee80211com *ic;
#endif
    QDF_STATUS status = QDF_STATUS_SUCCESS;

    if (!vdev)
        return QDF_STATUS_E_INVAL;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
        return QDF_STATUS_E_INVAL;

    /* All VDEV MLME operations exported to SON component */
    switch (type) {
    /* SET/CLR API start */
    case VDEV_SET_IE:
        if (!in)
            return QDF_STATUS_E_INVAL;
        wlan_mlme_app_ie_set_check(vap, in->appie.ftype,
                                   in->appie.ie, in->appie.size, in->appie.id);
        break;
    case VDEV_CLR_IE:
        if (!in)
            return QDF_STATUS_E_INVAL;
        wlan_mlme_app_ie_delete(vap, in->appie.ftype,
                                in->appie.ie);
        break;
    case VDEV_SET_ACL:
        if (!in)
            return QDF_STATUS_E_INVAL;
        acl_flag = convert_acl_flag(in->acl.flag);
        if (EOK != ieee80211_acl_set_flag(vap, in->acl.dstmac, acl_flag))
            status = QDF_STATUS_E_NOENT;
        break;
    case VDEV_CLR_ACL:
        if (!in)
            return QDF_STATUS_E_INVAL;
        acl_flag = convert_acl_flag(in->acl.flag);
        if (EOK != ieee80211_acl_clr_flag(vap, in->acl.dstmac, acl_flag))
            status = QDF_STATUS_E_NOENT;
        break;
    case VDEV_SET_ACL_TIMER:
        if (!in)
            return QDF_STATUS_E_INVAL;
        wlan_vdev_add_acl_validity_timer(vdev, in->acl.dstmac, in->acl.time_sec);
        break;
    case VDEV_SET_PEER_ACT_STATS:
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        if (!in)
            return QDF_STATUS_E_INVAL;
        ic = vap->iv_ic;
        if (!ic)
            return QDF_STATUS_E_INVAL;
        vap->iv_peer_act_stats_en = in->flag;
        if (ic->nss_vops && ic->nss_vops->ic_osif_nss_vdev_set_cfg) {
            ic->nss_vops->ic_osif_nss_vdev_set_cfg((osif_dev *)vap->iv_ifp,
                                                   OSIF_NSS_WIFI_VDEV_CFG_PEER_ACT_STATS);
        }
#endif
        break;
    case VDEV_SET_SEC_STA_WDS:
        wlan_set_param(vap, IEEE80211_CONFIG_SEC_STA_WDS, 1);
        break;
    case VDEV_SET_MEC:
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        if (!in)
            return QDF_STATUS_E_INVAL;
        ic = vap->iv_ic;
        if (!ic)
            return QDF_STATUS_E_INVAL;
        vap->iv_mec_en = in->flag;
        if (ic->nss_vops && ic->nss_vops->ic_osif_nss_vdev_set_cfg) {
            ic->nss_vops->ic_osif_nss_vdev_set_cfg((osif_dev *)vap->iv_ifp,
                                                   OSIF_NSS_WIFI_VDEV_CFG_MEC);
        }
#endif
        break;
    case VDEV_SET_MBO_IE_BSTM:
        if (!in)
            return QDF_STATUS_E_INVAL;
        vap->iv_mbo_bstm = in->flag;
        break;
    case VDEV_SET_WPS_ACL_ENABLE:
        if (!in)
            return QDF_STATUS_E_INVAL;
        vap->iv_wps_acl_en = in->flag;
        break;
    case VDEV_SET_WNM_BSS_PREF:
        if (!in)
            return QDF_STATUS_E_INVAL;
        vap->wnm->wnm_bss_pref = in->bss_pref;
        break;
    case VDEV_SET_SON_MAP_VERSION:
        if (!in)
            return QDF_STATUS_E_INVAL;
        vap->iv_map_version = in->iv_map_version;
        break;
    case VDEV_SET_MCTBL:
        if(!in)
            return QDF_STATUS_E_INVAL;
        wlan_update_hifitbl(vdev, in->mctbl.data, in->mctbl.len, DP_ME_MCTBL_SEG_LAST);
        break;

    /* GET API start */
    case VDEV_GET_NSS:
        if (!out)
            return QDF_STATUS_E_INVAL;
        out->nss = wlan_vdev_get_nss(vdev);
        break;
    case VDEV_GET_CHAN:
        if (!out)
            return QDF_STATUS_E_INVAL;
        *chan = wlan_vdev_get_channel(vdev);
        out->chan.ic_freq = chan->ic_freq;
        out->chan.ic_ieee = chan->ic_ieee;
        out->chan.ic_flags = chan->ic_flags;
        break;
    case VDEV_GET_CHAN_WIDTH:
        if (!out)
            return QDF_STATUS_E_INVAL;
        out->chan_width = wlan_vdev_get_chwidth(vdev);
        break;
    case VDEV_GET_CHAN_UTIL:
        if (!out)
            return QDF_STATUS_E_INVAL;
        out->chan_util = wlan_get_param(vap, IEEE80211_CHAN_UTIL);
        break;
    case VDEV_GET_APCAP:
        if (!out)
            return QDF_STATUS_E_INVAL;
        wlan_vdev_get_apcap(vdev, &out->apcap);
        break;
    case VDEV_GET_CONNECT_N_TX:
        if (!out)
            return QDF_STATUS_E_INVAL;
        out->flag =
            (wlan_vdev_allow_connect_n_tx(vdev) == QDF_STATUS_SUCCESS);
        break;
    case VDEV_GET_SSID:
        if (!out)
            return QDF_STATUS_E_INVAL;
        if (!ieee80211_get_desired_ssid(vap, 0, &ssid))
            memcpy(&out->ssid, ssid, sizeof(wlan_ssid));
        else
            status = QDF_STATUS_E_INVAL;
        break;
    case VDEV_GET_MAX_PHYRATE:
        if (!out)
            return QDF_STATUS_E_INVAL;
        out->rate = wlan_ucfg_get_maxphyrate(vdev);
        break;
    case VDEV_GET_ACL:
        if (!out || !in)
            return QDF_STATUS_E_INVAL;
        acl_flag = convert_acl_flag(in->acl.flag);
        out->flag =
            (ieee80211_acl_flag_check(vap, in->acl.dstmac,
                                      acl_flag) == 1);
        break;
    case VDEV_GET_ACL_RSSI_THRESHOLDS:
        if (!out || !in)
            return QDF_STATUS_E_INVAL;
        /* Set default values from input */
        out->acl_rssi.inact_rssi_low = in->acl_rssi.inact_rssi_low;
        out->acl_rssi.inact_rssi_high = in->acl_rssi.inact_rssi_high;
        out->acl_rssi.low_rssi = in->acl_rssi.low_rssi;
        out->acl_rssi.low_rate_rssi = in->acl_rssi.low_rate_rssi;
        out->acl_rssi.high_rate_rssi = in->acl_rssi.high_rate_rssi;
        /* Override the values if required */
        wlan_vdev_acl_override_rssi_thresholds(vdev, in->acl_rssi.mac,
                                               &out->acl_rssi.inact_rssi_low,
                                               &out->acl_rssi.inact_rssi_high,
                                               &out->acl_rssi.low_rssi,
                                               &out->acl_rssi.low_rate_rssi,
                                               &out->acl_rssi.high_rate_rssi);
        break;
    case VDEV_GET_NODE_CAP:
        if (!out || !in)
            return QDF_STATUS_E_INVAL;
        wlan_vdev_get_node_info(vdev, in->mac, &out->nodeinfo);
        break;
    case VDEV_GET_WDS:
        if (!out)
            return QDF_STATUS_E_INVAL;
        out->flag = wlan_rptr_vdev_is_wds(vdev) ? true : false;
        break;
    default:
        status = QDF_STATUS_E_INVAL;
    }

    return status;
}

QDF_STATUS wlan_scan_db_iterate(struct wlan_objmgr_pdev *pdev,
				scan_iterator_func handler, void *arg)
{
    return ucfg_scan_db_iterate(pdev, handler, arg);
}

QDF_STATUS wlan_pdev_ops(struct wlan_objmgr_pdev *pdev,
			 enum wlan_mlme_pdev_param type, void *data, void *ret)
{
    union wlan_mlme_pdev_data *in = (union wlan_mlme_pdev_data *)data;
    union wlan_mlme_pdev_data *out = (union wlan_mlme_pdev_data *)ret;
    struct pdev_ic_cp_stats *cps;
    struct wlan_objmgr_psoc *psoc;
    ol_txrx_soc_handle dp_soc;
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    struct ieee80211com *ic;

    if (!pdev)
	return QDF_STATUS_E_INVAL;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic)
        return QDF_STATUS_E_INVAL;

    switch (type) {
	case PDEV_SET_SEC_HM_WDS:
	    if (!in)
		return QDF_STATUS_E_INVAL;
	    /* For HK V1 only, HMWDS to be used for secondary radio */
	    ic->ic_war_hmwds = in->hmwds_en;
	    break;
	/* GET API start */
	case PDEV_GET_ESP_INFO:
	    if (!out)
		return QDF_STATUS_E_INVAL;
	    wlan_pdev_get_esp_info(pdev, &out->esp_info);
	    break;
	case PDEV_GET_DESC_POOLSIZE:
	    if (!out)
		return QDF_STATUS_E_INVAL;
	    /* Only being used for pre-lithium chips */
	    psoc = wlan_pdev_get_psoc(pdev);
	    if (!psoc)
		return QDF_STATUS_E_INVAL;
	    dp_soc = wlan_psoc_get_dp_handle(psoc);
	    out->desc_poolsize = cdp_get_free_desc_poolsize(dp_soc);
	    break;
	case PDEV_GET_PHY_ERRCNT:
	    if (!out)
		return QDF_STATUS_E_INVAL;
	    cps = wlan_get_pdev_cp_stats_ref(pdev);
	    if (!cps) {
		status = QDF_STATUS_E_EMPTY;
	    }
	    else {
		out->phy_errcnt = cps->stats.cs_phy_err_count;
	    }
	    break;
	case PDEV_GET_CAPABILITY:
	    if (!out)
		return QDF_STATUS_E_INVAL;
	    wlan_pdev_get_multi_ap_opclass(pdev, &out->cap, NULL, NULL, false);
	    break;
	case PDEV_GET_OPERABLE_CHAN:
	    if (!out || !in)
		return QDF_STATUS_E_INVAL;
	    memcpy(&out->op_chan, &in->op_chan, sizeof(struct wlan_op_chan));
	    wlan_pdev_get_multi_ap_opclass(pdev, NULL, &out->op_chan, NULL,
					   in->op_chan.dfs_required);
	    break;
	case PDEV_GET_OPERABLE_CLASS:
	    if (!out || !in)
		return QDF_STATUS_E_INVAL;
	    memcpy(&out->op_class, &in->op_class, sizeof(struct wlan_op_class));
	    wlan_pdev_get_multi_ap_opclass(pdev, NULL, NULL, &out->op_class,
					   false);
	    break;
	case PDEV_GET_OPCLASS_FOR_CUR_HWMODE:
	    if (!out)
		return QDF_STATUS_E_INVAL;
	    wlan_pdev_get_opclass_for_cur_hwmode(pdev, &out->cap);
	    break;
	case PDEV_SET_EMESH_RELAYFS_INIT:
	    wlan_telemetry_emesh_application_init_deinit_notify(AGENT_NOTIFY_EVENT_INIT);
	    return QDF_STATUS_SUCCESS;
	case PDEV_SET_EMESH_RELAYFS_DEINIT:
	    wlan_telemetry_emesh_application_init_deinit_notify(AGENT_NOTIFY_EVENT_DEINIT);
	    return QDF_STATUS_SUCCESS;
	case PDEV_GET_CAC_TLV:
	    if (!out)
		return QDF_STATUS_E_INVAL;
	    return wlan_pdev_get_5ghz_chan_info(pdev, &out->cac_tlv);
	    break;
	default:
	    status = QDF_STATUS_E_INVAL;
    }

    return status;
}

QDF_STATUS wlan_peer_ops(struct wlan_objmgr_peer *peer,
                         enum wlan_mlme_peer_param type, void *data, void *ret)
{
    union wlan_mlme_peer_data *in = (union wlan_mlme_peer_data *)data;
    union wlan_mlme_peer_data *out = (union wlan_mlme_peer_data *)ret;
    struct wlan_objmgr_vdev *vdev;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;
    wlan_if_t vap;
    struct ol_ath_softc_net80211 *scn;
    struct ol_ath_soc_softc *soc;
    struct wlan_lmac_if_tx_ops *tx_ops;
    wlan_node_t ni;
    QDF_STATUS status = QDF_STATUS_SUCCESS;

    if (!peer)
	return QDF_STATUS_E_INVAL;

    vdev = wlan_peer_get_vdev(peer);
    if (!vdev)
	return QDF_STATUS_E_INVAL;

    pdev = wlan_vdev_get_pdev(vdev);
    if (!pdev)
	return QDF_STATUS_E_INVAL;

    psoc = wlan_pdev_get_psoc(pdev);
    if (!psoc)
	return QDF_STATUS_E_INVAL;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap || !vap->iv_ic)
	return QDF_STATUS_E_INVAL;

    scn = OL_ATH_SOFTC_NET80211(vap->iv_ic);
    if (!scn)
	return QDF_STATUS_E_INVAL;

    soc = scn->soc;
    if (!soc)
	return QDF_STATUS_E_INVAL;

    /* All PEER MLME operations exported to SON component */
    switch (type) {
	/* SET/CLR API start */
	case PEER_SET_KICKOUT:
	    wlan_vdev_local_disassoc(vdev, peer);
	    break;
	case PEER_SET_KICKOUT_ALLOW:
	    if (!in)
		return QDF_STATUS_E_INVAL;
	    if (!ieee80211_node_set_kickout(vap, peer->macaddr, in->enable))
		status = QDF_STATUS_E_INVAL;
	    break;
	case PEER_SET_EXT_STATS:
	    if (!in)
		return QDF_STATUS_E_INVAL;
	    tx_ops = wlan_psoc_get_lmac_if_txops(psoc);
	    if (!tx_ops) {
		status = QDF_STATUS_E_INVAL;
		break;
	    }

	    if ((!!wlan_peer_is_flag_set(peer, IEEE80211_NODE_EXT_STATS)) != in->enable) {
		qdf_spin_lock_bh(&soc->soc_lock);
		if (tx_ops->son_tx_ops.peer_ext_stats_enable) {
		    status = tx_ops->son_tx_ops.peer_ext_stats_enable(pdev, peer->macaddr,
			    vdev, soc->peer_ext_stats_cnt,
			    in->enable);
		    if (status == QDF_STATUS_SUCCESS) {
			if (in->enable) {
			    soc->peer_ext_stats_cnt++;
			    wlan_set_node_peer_flag(peer, IEEE80211_NODE_EXT_STATS);
			}
			else {
			    if (soc->peer_ext_stats_cnt)
				soc->peer_ext_stats_cnt--;
			    wlan_clear_node_peer_flag(peer, IEEE80211_NODE_EXT_STATS);
			}
		    }
		}
		qdf_spin_unlock_bh(&soc->soc_lock);
	    }
	    break;
	case PEER_SET_VLAN_ID:
	    ni = (wlan_node_t)wlan_peer_get_mlme_ext_obj(peer);
	    if (!in)
	        return QDF_STATUS_E_INVAL;
	    ni->vlan_id = in->vlan_id;
	    break;
	case PEER_REQ_INST_STAT:
	    tx_ops = wlan_psoc_get_lmac_if_txops(psoc);
	    if (!tx_ops) {
		status = QDF_STATUS_E_INVAL;
		break;
	    }
	    if (tx_ops->son_tx_ops.son_send_null) {
		status = tx_ops->son_tx_ops.son_send_null(pdev, peer->macaddr, vdev);
	    }
	    break;
	    /* GET API start */
	case PEER_GET_CAPABILITY:
	    if (!out)
		return QDF_STATUS_E_INVAL;
	    wlan_node_get_capability(peer, &out->peercap);
	    break;
	case PEER_GET_MAX_MCS:
	    if (!out)
		return QDF_STATUS_E_INVAL;
	    out->mcs = (u_int32_t)wlan_peer_get_node_max_MCS(peer);
	    break;
	default:
	    status = QDF_STATUS_E_INVAL;
    }

    return status;
}


struct mlme_external_tx_ops mlme_tx_ops;

struct mlme_external_tx_ops * wlan_mlme_register_tx_ops(void)
{
    struct mlme_external_tx_ops *ops = &mlme_tx_ops;

    ops->peer_ops = wlan_peer_ops;
    ops->vdev_ops = wlan_vdev_ops;
    ops->pdev_ops = wlan_pdev_ops;
    ops->scan_db_iterate = wlan_scan_db_iterate;

    return ops;
}
#endif //QCA_SUPPORT_SON

struct wlan_objmgr_vdev * wlan_get_vdev_from_netdev(struct net_device *dev)
{
    wlan_if_t vap = NETDEV_TO_VAP(dev);

    if (vap == NULL) {
        return NULL;
     }

    return vap->vdev_obj;
}

bool wlan_check_rpt_max_phy(struct wlan_objmgr_pdev *pdev)
{
    struct ieee80211com *ic;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic)
        return false;

    return (ieee80211_ic_rpt_max_phy_is_set(ic));
}

bool wlan_is_6g_txable_chan_available(struct wlan_objmgr_vdev *vdev,
                                      u_int8_t pwr_mode)
{
    struct ieee80211com *ic = NULL;

    if (vdev == NULL) {
       return false;
    }
    ic = wlan_vdev_get_ic(vdev);

    if(ic == NULL) {
       return false;
    }

    return ieee80211_is_6g_txable_chan_available(ic, pwr_mode);
}
