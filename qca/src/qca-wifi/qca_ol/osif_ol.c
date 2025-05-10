/*
 * Copyright (c) 2016-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include "osif_private.h"
#include "target_type.h"
#include <wlan_opts.h>
#include <ieee80211_var.h>
#include <dp_extap.h>
#include <ieee80211_api.h>
#include <ieee80211_acfg.h>
#include <acfg_drv_event.h>
#include <wlan_vdev_mlme_api.h>
#include <ieee80211_cfg80211.h>
#if MESH_MODE_SUPPORT
#include <if_meta_hdr.h>
#endif
#include <qdf_perf.h>
#include "ath_netlink.h"

#include "ieee80211_ev.h"
#include <ol_txrx_api.h>
#include <cdp_txrx_ctrl.h>
#include <qdf_trace.h>
#include <wlan_utility.h>

#include <linux/seq_file.h>

#include <ieee80211_vi_dbg.h>

#ifdef UMAC_SUPPORT_PROXY_ARP
int wlan_proxy_arp(wlan_if_t vap, wbuf_t wbuf);
#endif

#include <linux/ethtool.h>

#include <ieee80211_nl.h>
#include <qdf_nbuf.h> /* qdf_nbuf_map_single */

#if ATH_SUPPORT_WRAP
#include "ieee80211_api.h"
#endif
#if ATH_PERF_PWR_OFFLOAD
#include <ol_cfg_raw.h>
#include <osif_rawmode.h>
#include <ol_if_athvar.h>
#endif /* ATH_PERF_PWR_OFFLOAD */
#include "ol_ath.h"
#include <linux/ethtool.h>
#include <osif_ol.h>
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include <osif_nss_wifiol_if.h>
#include <osif_nss_wifiol_vdev_if.h>
#endif
#if QCA_NSS_PLATFORM
#include <nss_api_if.h>
#endif
#if QCA_AIRTIME_FAIRNESS
#include <target_if_atf.h>
#endif
#include <init_deinit_lmac.h>
#include <wlan_utility.h>
#if DBDC_REPEATER_SUPPORT
#include <qca_multi_link.h>
#endif
#include <dp_txrx.h>

#if QCA_SUPPORT_RAWMODE_PKT_SIMULATION
#include <rawsim_api_defs.h>
#endif /* QCA_SUPPORT_RAWMODE_PKT_SIMULATION */

#if (MESH_MODE_SUPPORT)
#include <mesh_util.h>
#endif /* MESH_MODE_SUPPORT*/

#if ATH_SUPPORT_WRAP
#include "dp_wrap.h"
#endif
#include <wlan_repeater_api.h>
#include "wlan_mlo_mgr_peer.h"
#include <cfg_ucfg_api.h>
#include <wlan_mlo_mgr_sta.h>

#define DP_LAG_SEC_VAP_SEND true
#define DP_LAG_SEC_SKIP_VAP_SEND false
#define DP_XMIT_LINK 0 /* Transmit link of packet is non-MLO */

#if UMAC_VOW_DEBUG
extern void update_vow_dbg_counters(osif_dev  *osifp,
                        qdf_nbuf_t msdu, unsigned long *vow_counter, int rx, int peer);
#endif

#if QCA_NSS_PLATFORM
extern void osif_send_to_nss(os_if_t osif, struct sk_buff *skb, void *nss_redir_ctx,
                             struct net_device *dev);
#elif QCA_PPE_VP
extern void osif_send_to_ppe_vp(os_if_t osif, struct sk_buff *skb, int32_t ppe_vp_num,
                             struct net_device *dev);
#endif

#ifdef QCA_PARTNER_PLATFORM
extern bool osif_pltfrm_deliver_data(os_if_t osif, wbuf_t wbuf);
#endif
#if MESH_MODE_SUPPORT
extern void
os_if_tx_free_ext(struct sk_buff *skb);
#endif

#if UMAC_VOW_DEBUG
static inline void
osif_ol_hadrstart_vap_vow_debug(osif_dev  *osdev, struct sk_buff *skb){

    if(osdev->vow_dbg_en) {
        //This needs to be changed if multiple skbs are sent
        struct ether_header *eh = (struct ether_header *)skb->data;
        int i=0;

        for( i = 0; i < MAX_VOW_CLIENTS_DBG_MONITOR; i++ )
        {
            if( eh->ether_dhost[4] == osdev->tx_dbg_vow_peer[i][0] &&
                    eh->ether_dhost[5] == osdev->tx_dbg_vow_peer[i][1] ) {
                update_vow_dbg_counters(osdev, (qdf_nbuf_t) skb, &osdev->tx_dbg_vow_counter[i], 0, i);
                break;
            }
        }
    }
    return;
}

#endif /* UMAC_VOW_DEBUG*/


#if QCA_NSS_PLATFORM
#if UMAC_VOW_DEBUG || UMAC_SUPPORT_VI_DBG
#define OLE_HEADER_PADDING 2
int
transcap_nwifi_hdrsize(qdf_nbuf_t msdu)
{
    struct ieee80211_frame_addr4 *wh;
    uint32_t hdrsize;
    uint8_t fc1;

    wh = (struct ieee80211_frame_addr4 *)qdf_nbuf_data(msdu);
    fc1 = wh->i_fc[1] & IEEE80211_FC1_DIR_MASK;

    /* Native Wifi header  give only 80211 non-Qos packets */
    if (fc1 == IEEE80211_FC1_DIR_DSTODS )
    {
        hdrsize = sizeof(struct ieee80211_frame_addr4);

        /* In case of WDS frames, , padding is enabled by default
         * in Native Wifi mode, to make ipheader 4 byte aligned
         */
        hdrsize = hdrsize + OLE_HEADER_PADDING;
    } else {
        hdrsize = sizeof(struct ieee80211_frame);
    }

    /*
     *
     * header size (wifhdrsize + llchdrsize )
     */
    hdrsize +=sizeof(struct llc);
    return hdrsize;
}
#endif
#else
int transcap_nwifi_hdrsize(qdf_nbuf_t msdu) {
    return 0;
};
#endif

#if UMAC_VOW_DEBUG
#define VOW_DBG_RX_OFFSET 14
#define VOW_DBG_INVALID_IDX -1
#define    UMAC_VOW_NSSRX_DELIVER_DEBUG(_osif, _skb) \
        {\
            osif_dev  *osifp = (osif_dev *) _osif; \
            int rx_offset =0;\
            if (osifp->vow_dbg_en) { \
                update_vow_dbg_counters(osifp, (qdf_nbuf_t)_skb, &osifp->umac_vow_counter, rx_offset, \
                        VOW_DBG_INVALID_IDX); \
            } \
        }
#elif UMAC_SUPPORT_VI_DBG
#define UMAC_VOW_NSSRX_DELIVER_DEBUG(_osif, _skb) \
{\
        osif_dev  *osifp = (osif_dev *) _osif; \
        if (osifp->vi_dbg) { \
                ieee80211_vi_dbg_input(osifp->os_if, _skb); \
        } \
}
#else /* UMAC_VOW_DEBUG */
#define    UMAC_VOW_NSSRX_DELIVER_DEBUG(osif, skb)
#endif /* UMAC_VOW_DEBUG */


#if ATH_SUPPORT_VLAN
#if LINUX_VERSION_CODE <  KERNEL_VERSION(3,1,0)
#error "KERNEL_VERSION less then 3.1.0 not supported in NWIFI offload"
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
#define ATH_ADD_VLAN_TAG(_osif, _skb) \
{ \
    osif_dev  *osifp = (osif_dev *) _osif; \
    if ( osifp->vlanID != 0) { \
        __vlan_hwaccel_put_tag(_skb, osifp->vlanID); \
    } \
}
#define ATH_RX_VLAN_TAG(_osif, _skb) \
{ \
    osif_dev  *osifp = (osif_dev *) _osif; \
    if ( osifp->vlanID != 0 && osifp->vlgrp != NULL) { \
        vlan_hwaccel_rx(_skb, osifp->vlgrp, osifp->vlanID); \
    } \
}

#else
#define ATH_ADD_VLAN_TAG(_osif, _skb) \
{ \
    osif_dev  *osifp = (osif_dev *) _osif; \
    if ( osifp->vlanID != 0) { \
        __vlan_hwaccel_put_tag(_skb, htons(ETH_P_8021Q), osifp->vlanID); \
    } \
}
#define ATH_RX_VLAN_TAG(_osif, _skb)
#endif
#else
#define ATH_ADD_VLAN_TAG(_osif, _skb)
#define ATH_RX_VLAN_TAG(_osif, _skb)
#endif /*ATH_SUPPORT_VLAN*/


#if UMAC_VOW_DEBUG
#define  OL_TX_LL_UMAC_VAP_HARDSTART_VOW_DEBUG(_osdev, _skb) osif_ol_hadrstart_vap_vow_debug(_osdev, _skb)
#elif UMAC_SUPPORT_VI_DBG
#define  OL_TX_LL_UMAC_VAP_HARDSTART_VOW_DEBUG(_osdev, _skb) \
{\
	osif_dev  *osifp = (osif_dev *) _osdev; \
			   if (osifp->vi_dbg) { \
				   ieee80211_vi_dbg_input(osifp->os_if, _skb); \
			   } \
}
#else
#define OL_TX_LL_UMAC_VAP_HARDSTART_VOW_DEBUG(_osdev, _skb)
#endif /* UMAC_VOW_DEBUG */

#if ATH_SUPPORT_WRAP
/*
 * osif_ol_wrap_tx_process()
 *  wrap tx process
 */
int
osif_ol_wrap_tx_process(osif_dev **osifp, struct ieee80211vap *vap, struct sk_buff **skb)
{
    struct net_device *dev = (*osifp)->netdev;
    /*
     * The below addiitonal check is to make compilation when the ATH_WRAP_TX Macro is disabled.
     * the bad label will come as unused variable
     */
    if (vap == NULL) {
        goto bad;
    }
    if (dp_wrap_tx_process(&dev, vap->vdev_obj, skb)) {
          goto bad;
    } else {
        *osifp = ath_netdev_priv(dev);
    }
    return 0;
bad :
    return 1;
}
#else /* ATH_SUPPORT_WRAP */

#define OL_WRAP_TX_PROCESS(_osdev, _vap, _skb)
int
osif_ol_wrap_tx_process(osif_dev **osifp, struct ieee80211vap *vap, struct sk_buff **skb)
{
    return 0;
}
#endif /* ATH_SUPPORT_WRAP */


#if ATH_DATA_RX_INFO_EN || MESH_MODE_SUPPORT
u_int32_t osif_rx_status_dump(void* rs)
{
#if ATH_DATA_RX_INFO_EN
    struct per_msdu_data_recv_status *rs1 = rs;
#else
    struct mesh_recv_hdr_s *rs1 = rs;
    u_int32_t count;
#endif
    u_int32_t rate1 = rs1->rs_ratephy1;
    u_int32_t rate2 = rs1->rs_ratephy2 & 0xFFFFFF;
//    u_int32_t rate3 = rs->rs_ratephy3 & 0x1FFFFFFF;
    u_int32_t rate1_1 = ((rate1 & 0xFFFFFF0) >> 4);

    if(!(rs1->rs_flags & IEEE80211_RX_FIRST_MSDU)){
        /*Only for the 1st msdu, we populate the rx status struct,
          check unified_rx_desc_update_pkt_info() */
        return 0;
    }

    /*We save receive status info in skb->cb[48] after struct cvg_nbuf_cb,
      so we have struct cvg_nbuf_cb + struct per_msdu_data_recv_status in cb.
      Since, sizeof(struct cvg_nbuf_cb)=44, only 48-44=4 bytes left for rs here,
      we only have space to save the 1st int(rs_flags) of the struct per_msdu_data_recv_status.
    */
    qdf_info("%s: rs_flags=0x%x ",__FUNCTION__, rs1->rs_flags);
#if MESH_MODE_SUPPORT
    qdf_info("%s: frame is decrypted=0x%x keyix %d", __FUNCTION__, (rs1->rs_flags & MESH_RX_DECRYPTED)
            ? 1 : 0, rs1->rs_keyix);
#endif


    /*Below fields only valid when skb->cb has enough space to store them*/
    qdf_info("%s: rs_rssi=0x%x ",__FUNCTION__, rs1->rs_rssi);
    qdf_info("%s: rs_snr=0x%x ", __FUNCTION__, rs1->rs_snr);
    qdf_info("%s: rs_ratephy1=0x%x ",__FUNCTION__, rs1->rs_ratephy1);
    qdf_info("%s: rs_ratephy2=0x%x ",__FUNCTION__, rs1->rs_ratephy2);
    qdf_info("%s: rs_ratephy3=0x%x ",__FUNCTION__, rs1->rs_ratephy3);

#if MESH_MODE_SUPPORT
    qdf_info("%s: rs_band=0x%x", __FUNCTION__, rs1->rs_band);
    qdf_info("%s: rs_channel=0x%x ",__FUNCTION__,  rs1->rs_channel);
    qdf_info("%s: rs_key=",__FUNCTION__);
    for (count = 0; count < 32; count++) {
        qdf_info("0x%x ", rs1->rs_decryptkey[count]);
    }
    qdf_print("\n");
    if ((rs1->rs_flags & MESH_RXHDR_VER) == MESH_RXHDR_VER1) {
        qdf_info("pkt type %d ", (rs1->rs_ratephy1 >> 16) & 0xFF);
        qdf_info("mcs %d ", rs1->rs_ratephy1 & 0xFF);
        qdf_info("nss %d ", (rs1->rs_ratephy1 >> 8) & 0xFF);
        qdf_info("bw %d", (rs1->rs_ratephy1 >> 24) & 0xFF);
    } else
#endif
    {
        switch (rate1 & 0xF) // preamble
        {
            case 0: //CCK
                {
                    switch(rate1_1) //l_sig_rate
                    {
                        case 0x1:  //long 1M
                            qdf_info("CCK 1 Mbps long preamble");
                            break;
                        case 0x2: //long 2M
                            qdf_info("CCK 2 Mbps long preamble");
                            break;
                        case 0x3: //long 5.5M
                            qdf_info("CCK 5.5 Mbps long preamble");
                            break;
                        case 0x4: //long 11M
                            qdf_info("CCK 11 Mbps long preamble");
                            break;
                        case 0x5: //short 2M
                            qdf_info("CCK 2 Mbps short preamble");
                            break;
                        case 0x6: //short 5.5M
                            qdf_info("CCK 5.5 Mbps short preamble");
                            break;
                        case 0x7: //short 11M
                            qdf_info("CCK 11 Mbps short preamble");
                            break;
                    }
                }
                break;
            case 1: //OFDM
                {
                    switch(rate1_1) //l_sig_rate
                    {
                        case 0x8:
                            qdf_info("OFDM 48 Mbps");
                            break;

                        case 0x9:
                            qdf_info("OFDM 24 Mbps");
                            break;

                        case 0xa:
                            qdf_info("OFDM 12 Mbps");
                            break;

                        case 0xb:
                            qdf_info("OFDM 6 Mbps");
                            break;

                        case 0xc:
                            qdf_info("OFDM 54 Mbps");
                            break;

                        case 0xd:
                            qdf_info("OFDM 36 Mbps");
                            break;

                        case 0xe:
                            qdf_info("OFDM 18 Mbps");
                            break;

                        case 0xf:
                            qdf_info("OFDM 9 Mbps");
                            break;
                    }
                }
                break;
            case 2:
                {
                    if(rate1_1 & 0x80) //HT40
                    {
                        qdf_info("HT40 MCS%c", '0' + (rate1_1 & 0x1f));
                    }
                    else // HT20
                    {
                        qdf_info("HT20 MCS%c", '0' + (rate1_1 & 0x1f));
                    }
                }
                break;
            case 3:
                switch (rate1_1 & 0x3)
                {
                    case 0x0: // VHT20
                        qdf_info("VHT20 NSS%c MCS%c", '1' + ((rate1_1 >> 10) & 0x3),
                                '0' + ((rate2 >> 4) & 0xf));

                        break;

                    case 0x1: // VHT40
                        qdf_info("VHT40 NSS%c MCS%c", '1' + ((rate1_1 >> 10) & 0x3),
                                '0' + ((rate2 >> 4) & 0xf));

                        break;

                    case 0x2: // VHT80
                        qdf_info("VHT80 NSS%c MCS%c", '1' + ((rate1_1 >> 10) & 0x3),
                                '0' + ((rate2 >> 4) & 0xf));

                        break;
                }
                break;
        }
    }
    return 0;
}
qdf_export_symbol(osif_rx_status_dump);
#endif /*end of ATH_DATA_RX_INFO_EN*/

#if QCA_NSS_PLATFORM
bool osif_ol_no_nss_accel(os_if_t osif, struct sk_buff *skb,
                          struct ol_txrx_nbuf_classify *nbuf_class)
{
    struct net_device *dev = OSIF_TO_NETDEV(osif);

    if (nbuf_class->is_ipv4 &&
            (nss_ipv4_dscp_action_get(nbuf_class->pkt_dscp)
             != NSS_IPV4_DSCP_MAP_ACTION_DONT_ACCEL)) {
        return 0;
    }
    else if (nbuf_class->is_ipv6 &&
            (nss_ipv6_dscp_action_get(nbuf_class->pkt_dscp)
             != NSS_IPV6_DSCP_MAP_ACTION_DONT_ACCEL)) {
        return 0;
    }

    skb->protocol = eth_type_trans(skb, dev);
    skb->dev = dev;
    nbuf_debug_del_record(skb);
    netif_receive_skb(skb);
    return 1;
}
#else
bool osif_ol_no_nss_accel(os_if_t osif, struct sk_buff *skb,
                          struct ol_txrx_nbuf_classify *nbuf_class)
{
    return 0;
}
#endif

#ifdef QCA_PEER_EXT_STATS
void osif_update_rx_delay_stats(ol_txrx_soc_handle soc_txrx_handle,
                                struct sk_buff *skb, uint8_t vdev_id)
{
    cdp_config_param_type value = {0};

    cdp_txrx_get_psoc_param(soc_txrx_handle, CDP_CFG_PEER_EXT_STATS, &value);
    if (qdf_likely(!value.cdp_psoc_param_pext_stats)) {
        return;
    }

    cdp_calculate_delay_stats(soc_txrx_handle, vdev_id, skb);
}
#else
static inline void osif_update_rx_delay_stats(ol_txrx_soc_handle soc_txrx_handle,
                                              struct sk_buff *skb, uint8_t vdev_id)
{
}
#endif

bool osif_ol_delay_processing(os_if_t osif, struct sk_buff *skb)
{
    ol_txrx_soc_handle soc_txrx_handle;
    struct net_device *comdev;
    struct ol_ath_softc_net80211 *scn;
    cdp_config_param_type value = {0};
    osif_dev  *osifp = (osif_dev *) osif;

    comdev = ((osif_dev *)osif)->os_comdev;
    scn = ath_netdev_priv(comdev);

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    cdp_txrx_get_pdev_param(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
            CDP_CONFIG_DELAY_STATS, &value);

    /*
     * Classify packet for Video and query nss driver action whether
     * to accelerate the packet or drop it
     */
    if (qdf_unlikely(value.cdp_pdev_param_cfg_delay_stats)) {
        struct ol_txrx_nbuf_classify nbuf_class;
        cdp_txrx_classify_and_update(soc_txrx_handle,
                wlan_vdev_get_id(osifp->ctrl_vdev),
                skb, rx_direction, &nbuf_class);
        cdp_calculate_delay_stats(soc_txrx_handle, wlan_vdev_get_id(osifp->ctrl_vdev), skb);
        if (osif_ol_no_nss_accel(osif, skb, &nbuf_class))
           return 0;
    }

    osif_update_rx_delay_stats(soc_txrx_handle, skb, wlan_vdev_get_id(osifp->ctrl_vdev));
    return 1;
}

#if CONFIG_DP_TRACE
void osif_ol_dp_trace(os_if_t osif, struct sk_buff *skb) {
    struct net_device *comdev = ((osif_dev *)osif)->os_comdev;
    struct ol_ath_softc_net80211 *scn = ath_netdev_priv(comdev);
    uint8_t pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);

    qdf_nbuf_classify_pkt(skb);
    qdf_dp_trace_log_pkt(0, skb, QDF_RX, pdev_id);
    qdf_dp_trace_set_track(skb, QDF_RX);
    QDF_NBUF_CB_RX_PACKET_TRACK(skb) = QDF_NBUF_RX_PKT_DATA_TRACK;
    DPTRACE(qdf_dp_trace(skb, QDF_DP_TRACE_HDD_RX_PACKET_RECORD, pdev_id,
                (uint8_t *)skb->data, qdf_nbuf_len(skb), QDF_RX));

    if (qdf_nbuf_len(skb) > QDF_DP_TRACE_RECORD_SIZE) {
        DPTRACE(qdf_dp_trace(skb, QDF_DP_TRACE_HDD_RX_PACKET_RECORD,
                    pdev_id, (uint8_t *)&skb->data[QDF_DP_TRACE_RECORD_SIZE],
                    (qdf_nbuf_len(skb) - QDF_DP_TRACE_RECORD_SIZE), QDF_RX));
    }
}
#else
void osif_ol_dp_trace(os_if_t osif, struct sk_buff *skb) {}
#endif

#if ATH_DATA_RX_INFO_EN || MESH_MODE_SUPPORT
void osif_ol_dump_rx_info(wlan_if_t vap, struct sk_buff *skb)
{
    enum cb_ftype ftype = qdf_nbuf_get_rx_ftype(skb);

    /*
     * provide callback to collect rx_status here if required.
     * The data will no longer be available after this point
     */
    if ((ftype == CB_FTYPE_RX_INFO)
#if MESH_MODE_SUPPORT
       || ((vap->mdbg & 0x2) && (ftype == CB_FTYPE_MESH_RX_INFO))
#endif
    ){
        /*
         * provide callback to collect rx_status here if required.
         * The data will no longer be available after this point
         */
        osif_rx_status_dump((void *)qdf_nbuf_get_rx_fctx(skb));
    }

    if ((qdf_nbuf_get_rx_ftype(skb) == CB_FTYPE_RX_INFO) ||
            (qdf_nbuf_get_rx_ftype(skb) == CB_FTYPE_MESH_RX_INFO)) {
        qdf_mem_free((void *)qdf_nbuf_get_rx_fctx(skb));
        qdf_nbuf_set_rx_fctx_type(skb, 0, CB_FTYPE_INVALID);
    }
}
#else
void osif_ol_dump_rx_info(wlan_if_t vap, struct sk_buff *skb) {}
#endif

int osif_deliver_data_ol_extended(os_if_t *osif, struct net_device **dev,
	                          wlan_if_t vap, struct sk_buff *skb)
{
    osif_dev  *osdev = (osif_dev *)*osif;
    struct wlan_objmgr_vdev *vdev = osdev->ctrl_vdev;
#if DBDC_REPEATER_SUPPORT
    struct ol_ath_softc_net80211 *pscn = NULL;
    struct net_device *prim_dev = NULL;
#endif
#if ATH_SUPPORT_WRAP
    int rv = 0;
#endif


#if QCA_OL_VLAN_WAR
    OL_RX_VLAN_WAR(skb, (struct ol_ath_softc_net80211 *)vap->iv_ic);
#endif
    osif_ol_dump_rx_info(vap, skb);
    osif_ol_dp_trace(*osif, skb);

#if ATH_SUPPORT_WRAP
    if (*dev) {
        rv = dp_wrap_rx_process(dev, vap->vdev_obj, skb);
        if (qdf_likely(rv == 0)) {
            *osif = (os_if_t)ath_netdev_priv(*dev);
        } else if (rv < 0) {
            qdf_nbuf_free(skb);
            return 0;
        } else {
            return 0;
        }
    }
#endif

    UMAC_VOW_NSSRX_DELIVER_DEBUG(*osif, skb);

    if (ADP_EXT_AP_RX_PROCESS(vdev, skb)) {
        qdf_nbuf_free(skb);
        return 0;
    }

#if DBDC_REPEATER_SUPPORT
    pscn = (struct ol_ath_softc_net80211 *)vap->iv_ic;

    if ((lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCA8074) ||
        (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCA8074V2) ||
        (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCA9574) ||
        (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCN9000) ||
        (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCN9224) ||
        (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCA5332) ||
        (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCN6432) ||
        (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCA5018) ||
        (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCA6018)) {
        if (wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE) {
            if (qca_multi_link_sta_rx(*dev, skb, &prim_dev,
                                      osif_get_mld_netdev(osdev)) == 0) {
                if (prim_dev) {
                    skb->dev = prim_dev;
                    skb->protocol = eth_type_trans(skb, prim_dev);
                    nbuf_debug_del_record(skb);
                    netif_receive_skb(skb);
                    return 0;
                }
            } else {
                return 0;
            }
        }
    }
#endif

#ifdef QCA_PARTNER_PLATFORM
    if (osif_pltfrm_deliver_data(*osif, skb))
        return 0;
#endif

    ATH_ADD_VLAN_TAG(*osif, skb);
    ATH_RX_VLAN_TAG(*osif, skb);

    if (!osif_ol_delay_processing(*osif, skb))
        return 0;

    return 1;
}

#ifdef QCA_SUPPORT_EAPOL_OVER_CONTROL_PORT
void osif_deliver_eapol_over_nl(os_if_t osif, struct sk_buff *skb_list)
{
    struct net_device *dev;
    osif_dev  *osdev = (osif_dev *)osif;
    wlan_if_t vap = osdev->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    struct ethhdr *eth;
    uint8_t eapol_type;
    uint8_t *src = NULL, *dst = NULL;
    bool pairwise;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_mlo_peer_context *mlo_peer_ctx;
    struct wlan_objmgr_peer *assoc_peer;
    struct wlan_objmgr_vdev *assoc_vdev;
    wlan_if_t assoc_vap;
#endif
    struct wireless_dev *wdev;
    osif_dev* assoc_osdev = osdev;
    dev = get_cfg80211_notification_ndev(osdev);

    while (skb_list) {
        struct sk_buff *skb;
        skb = skb_list;
        skb_list = skb_list->next;
        skb->dev = dev;
        skb->next = NULL;

        /* Forward EAPOL frames over NL socket for lithium chipsets */
        if (ic->enable_eapol_over_nl && qdf_nbuf_is_ipv4_eapol_pkt(skb)){
            uint8_t *pkt_type;
            pkt_type = qdf_nbuf_data(skb) + (2 * QDF_MAC_ADDR_SIZE);
	    dst = qdf_nbuf_data(skb);
            src = qdf_nbuf_data(skb) + QDF_MAC_ADDR_SIZE;

            if (*(uint16_t *)pkt_type == htons(QDF_ETH_TYPE_8021Q)) {
                struct vlan_ethhdr veth_hdr;
                struct vlan_ethhdr *veh = (struct vlan_ethhdr *)skb->data;

                /*
                 * Extract VLAN header of 4 bytes:
                 * Frame Format : {dst_addr[6], src_addr[6], 802.1Q header[4], EtherType[2], Payload}
                 * Before Removal : xx xx xx xx xx xx xx xx xx xx xx xx 81 00 00 02 08 00 45 00 00...
                 * After Removal  : xx xx xx xx xx xx xx xx xx xx xx xx 08 00 45 00 00...
                 */
                 qdf_mem_copy(&veth_hdr, veh, sizeof(veth_hdr));
                 qdf_nbuf_pull_head(skb, 4);
                 veh = (struct vlan_ethhdr *)skb->data;
                 qdf_mem_copy(veh, &veth_hdr, 2 * QDF_MAC_ADDR_SIZE);
            }

            eth = (struct ethhdr *)qdf_nbuf_data(skb);
            skb->protocol = eth->h_proto;
            eapol_type = qdf_nbuf_get_eapol_subtype(skb);

            if (eapol_type == QDF_PROTO_EAPOL_M1)
                vap->iv_m1_packet_cnt++;
            else if (eapol_type == QDF_PROTO_EAPOL_M2)
                vap->iv_m2_packet_cnt++;
            else if (eapol_type == QDF_PROTO_EAPOL_M3)
                vap->iv_m3_packet_cnt++;
            else if (eapol_type == QDF_PROTO_EAPOL_M4)
                vap->iv_m4_packet_cnt++;
            else if (eapol_type == QDF_PROTO_EAPOL_G1)
                vap->iv_g1_packet_cnt++;
            else if (eapol_type == QDF_PROTO_EAPOL_G2)
                vap->iv_g2_packet_cnt++;

            pairwise = (eapol_type == QDF_PROTO_EAPOL_G1 || eapol_type == QDF_PROTO_EAPOL_G2) ? 0:1;
            eapol_type = (eapol_type == QDF_PROTO_EAPOL_G1) ?
                         1 : ((eapol_type == QDF_PROTO_EAPOL_G2) ? 2 : eapol_type);
            skb->mac_header = skb->data - skb->head;

#ifdef WLAN_FEATURE_11BE_MLO
            /* Override VAP with Assoc VAP in MLO case */
            if (ieee80211_mlo_is_mld_ap(vap->vdev_obj) || mlo_is_mld_sta(vap->vdev_obj) ||
                IEEE80211_IS_MLO_BRIDGE_VAP(vap)) {
                mlo_peer_ctx = wlan_mlo_get_mlpeer_by_mld_mac(vap->vdev_obj->mlo_dev_ctx,
                                                              (struct qdf_mac_addr *)src);
                if (mlo_peer_ctx) {
                    assoc_peer = wlan_mlo_peer_get_assoc_peer(mlo_peer_ctx);
                    if (assoc_peer) {
                        assoc_vdev = wlan_peer_get_vdev(assoc_peer);
                        assoc_vap = wlan_vdev_get_mlme_ext_obj(assoc_vdev);

                        if (assoc_vap) {
                            assoc_osdev = (osif_dev*)(assoc_vap->iv_ifp);
                            dev = get_cfg80211_notification_ndev(assoc_osdev);
                            skb->dev = dev;
                        }
                    }
                }
            }
#endif
            /*
             * Invoke cfg80211_control_port_tx_status() with RX buffer to notify
             * iw utility to dump the EAPOL M2 and M4 frames.
             */
            wdev = get_cfg80211_notification_wdev(assoc_osdev);
	    if (wdev) {
                cfg80211_control_port_tx_status(wdev, 0, qdf_nbuf_data(skb),
                                                skb->len, true, GFP_ATOMIC);
            } else {
                qdf_err("null wiphy dereference can happen at kernel avoid invoke cfg80211_control_port_tx_status");
                if (dst && src) {
                    qdf_err(" destination address is %s \n",ether_sprintf(dst));
                    qdf_err(" source address is %s \n",ether_sprintf(src));
                 }
             }
            /*
              Hostapd expects only EAPOL data and skb data contains DA+SA+Ethertype+EAPOL data
              So, removing DA(6)+SA(6)+ethertype(2) from skb data.
            */
            qdf_nbuf_pull_head(skb, sizeof(qdf_ether_header_t));

            if (qdf_unlikely(!dev)) {
                qdf_err("dev is NULL dropping packet EAPOL M%d from %s",
                        eapol_type, ether_sprintf(src));
                qdf_nbuf_free(skb);
                return;
            }
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
            cfg80211_rx_control_port(dev, skb, 0, -1);
#else
#ifdef CFG80211_MLO_SINGLE_NETDEV_API
            cfg80211_rx_control_port(dev, skb, 0, -1);
#else
            cfg80211_rx_control_port(dev, skb, 0);
#endif
#endif
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_CFG80211, "%s EAPOL %s%d from :%s\n", __func__,
                              pairwise ? "M":"G", eapol_type, ether_sprintf(src));
            qdf_nbuf_free(skb);
        } else
            qdf_nbuf_free(skb);
    }

    return;
}
#endif

#ifdef QCA_WIFI_IPQ4019
#ifdef QCA_SUPPORT_WDS_EXTENDED
void osif_deliver_wds_ext_data_ol(os_if_t osif, struct sk_buff *skb_list)
{
}
#endif

void
osif_deliver_data_ol(os_if_t osif, struct sk_buff *skb_list)
{
    struct net_device *dev = OSIF_TO_NETDEV(osif);
#if ATH_SUPPORT_VLAN || UMAC_VOW_DEBUG
    osif_dev  *osifp = (osif_dev *) osif;
#endif
    osif_dev  *osdev = (osif_dev *)osif;
    struct wlan_objmgr_vdev *vdev = osdev->ctrl_vdev;
    wlan_if_t vap = osdev->os_if;
#if ATH_RXBUF_RECYCLE
    struct net_device *comdev;
    struct ath_softc_net80211 *scn;
    struct ath_softc *sc;
#endif /* ATH_RXBUF_RECYCLE */

#if QCA_OL_VLAN_WAR || CONFIG_DP_TRACE
    ol_txrx_soc_handle soc_txrx_handle;
    struct net_device *comdev_war;
    struct ol_ath_softc_net80211 *scn_war;
#if UMAC_VOW_DEBUG
    struct ol_txrx_nbuf_classify nbuf_class;
#endif
    uint8_t pdev_id = 0;
    uint8_t vdev_id = wlan_vdev_get_id(osdev->ctrl_vdev);
#if ATH_SUPPORT_WRAP
    int rv =0;
#endif
    cdp_config_param_type value = {0};

    comdev_war = ((osif_dev *)osif)->os_comdev;
    scn_war = ath_netdev_priv(comdev_war);
    soc_txrx_handle = wlan_psoc_get_dp_handle(scn_war->soc->psoc_obj);
    pdev_id =  wlan_objmgr_pdev_get_pdev_id(scn_war->sc_pdev);
    cdp_txrx_get_pdev_param(soc_txrx_handle, pdev_id, CDP_CONFIG_DELAY_STATS, &value);
#endif /* QCA_OL_VLAN_WAR || CONFIG_DP_TRACE */

#if ATH_RXBUF_RECYCLE
    comdev = ((osif_dev *)osif)->os_comdev;
    scn = ath_netdev_priv(comdev);
    sc = ATH_DEV_TO_SC(scn->sc_dev);
#endif /* ATH_RXBUF_RECYCLE */

    while (skb_list) {
        struct sk_buff *skb;

        skb = skb_list;
        skb_list = skb_list->next;

        skb->dev = dev;
        /*
         * SF#01368954
         * Thanks to customer for the fix.
         *
         * Each skb of the list is delivered to the OS.
         * Thus, we need to unlink each skb.
         * Otherwise, the OS processes the linked skbs, as well,
         * which results in sending the same skb twice to the LAN driver.
         * This, also leads to unpredictable frame drops.
         * Note that this (most likely) only occurs when sending frames
         * using dev_queue_xmit().
         * Delivering linked skbs through netif_rx() seems not to be a problem.
         */
        skb->next = NULL;

#if ATH_DATA_RX_INFO_EN
        if (qdf_nbuf_get_rx_ftype(skb) == CB_FTYPE_RX_INFO)
        /*
         * provide callback to collect rx_status here if required.
         * The data will no longer be available after this point
         */
            osif_rx_status_dump((void *)qdf_nbuf_get_rx_fctx(skb));
#elif MESH_MODE_SUPPORT
        if ((vap->mdbg & 0x2) && (qdf_nbuf_get_rx_ftype(skb) == CB_FTYPE_MESH_RX_INFO)) {
        /*
         * provide callback to collect rx_status here if required.
         * The data will no longer be available after this point
         */
            osif_rx_status_dump((void *)qdf_nbuf_get_rx_fctx(skb));
        }
#endif

#if ATH_DATA_RX_INFO_EN
        if (qdf_nbuf_get_rx_ftype(skb) == CB_FTYPE_RX_INFO){
            qdf_mem_free((void *)qdf_nbuf_get_rx_fctx(skb));
            qdf_nbuf_set_rx_fctx_type(skb, 0, CB_FTYPE_INVALID);
       }
#elif MESH_MODE_SUPPORT
       if (qdf_nbuf_get_rx_ftype(skb) == CB_FTYPE_MESH_RX_INFO) {
            qdf_mem_free((void *)qdf_nbuf_get_rx_fctx(skb));
            qdf_nbuf_set_rx_fctx_type(skb, 0, CB_FTYPE_INVALID);
       }
#endif

#if QCA_OL_VLAN_WAR
    OL_RX_VLAN_WAR(skb, (struct ol_ath_softc_net80211 *)vap->iv_ic);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)) && MIPS_TP_ENHC
    prefetch(skb->data);
    prefetch(&skb->protocol);
    prefetch(&skb->__pkt_type_offset);
#endif

#if CONFIG_DP_TRACE
    qdf_nbuf_classify_pkt(skb);
    qdf_dp_trace_log_pkt(0, skb, QDF_RX, pdev_id);
    qdf_dp_trace_set_track(skb, QDF_RX);
    QDF_NBUF_CB_TX_PACKET_TRACK(skb) = QDF_NBUF_TX_PKT_DATA_TRACK;
    DPTRACE(qdf_dp_trace(skb, QDF_DP_TRACE_HDD_RX_PACKET_RECORD, pdev_id,
        (uint8_t *)skb->data, qdf_nbuf_len(skb), QDF_RX));
    if (qdf_nbuf_len(skb) > QDF_DP_TRACE_RECORD_SIZE) {
        DPTRACE(qdf_dp_trace(skb, QDF_DP_TRACE_HDD_RX_PACKET_RECORD,
            pdev_id, (uint8_t *)&skb->data[QDF_DP_TRACE_RECORD_SIZE],
            (qdf_nbuf_len(skb)-QDF_DP_TRACE_RECORD_SIZE), QDF_RX));
    }
#endif

#if ATH_SUPPORT_WRAP
    rv = dp_wrap_rx_process(&dev,vap->vdev_obj, skb);
    if (qdf_likely(rv == 0)) {
        osif = (os_if_t)ath_netdev_priv(dev);
    } else if(rv < 0) {
        qdf_nbuf_free(skb);
        return;
    } else {
        return;
    }
#endif

    if (ADP_EXT_AP_RX_PROCESS(vdev, skb)) {
        qdf_nbuf_free(skb);
        return;
    }

#ifdef HOST_OFFLOAD
        /* For the Full Offload solution, diverting the data packet into the
           offload stack for further processing and hand-off to Host processor */
        atd_rx_from_wlan(skb);
        continue;
#endif

#ifdef QCA_PARTNER_PLATFORM
        if ( osif_pltfrm_deliver_data (osif, skb))
            continue;
#endif

#if UMAC_VOW_DEBUG && QCA_OL_VLAN_WAR
	/*  Classify packet for Video */
	if (qdf_unlikely(value.cdp_pdev_param_cfg_delay_stats))
		cdp_txrx_classify_and_update(soc_txrx_handle,
			wlan_vdev_get_id(vdev), skb, rx_direction, &nbuf_class);
#endif

        skb->protocol = eth_type_trans(skb, dev);

#if ATH_RXBUF_RECYCLE
	    /*
	     * Do not recycle the received mcast frame b/c it will be cloned twice
	     */
        if (sc->sc_osdev->rbr_ops.osdev_wbuf_collect && !(wbuf_is_cloned(skb)))
        {
            sc->sc_osdev->rbr_ops.osdev_wbuf_collect((void *)sc, (void *)skb);
        }
#endif /* ATH_RXBUF_RECYCLE */

#if UMAC_VOW_DEBUG
#define VOW_DBG_RX_OFFSET 14 /*RX packet ethernet header is stripped off. Need to adjust offset accordingly*/
#define VOW_DBG_INVALID_IDX -1
        if(osifp->vow_dbg_en) {
            update_vow_dbg_counters(osifp, (qdf_nbuf_t)skb, &osifp->umac_vow_counter, VOW_DBG_RX_OFFSET,
                    VOW_DBG_INVALID_IDX);
        }
#endif

#if ATH_SUPPORT_VLAN
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,1,0)
        if ( osifp->vlanID != 0)
        {
            /* attach vlan tag */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
            __vlan_hwaccel_put_tag(skb, osifp->vlanID);
#else
            __vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), osifp->vlanID);
#endif
        }
#else
        if ( osifp->vlanID != 0 && osifp->vlgrp != NULL)
        {
            /* attach vlan tag */
            vlan_hwaccel_rx(skb, osifp->vlgrp, osifp->vlanID);
        }
        else  /*XXX NOTE- There is an else here. Be careful while adding any code below */
#endif
#endif

#if QCA_OL_VLAN_WAR
        if (qdf_unlikely(value.cdp_pdev_param_cfg_delay_stats)) {
            cdp_calculate_delay_stats(soc_txrx_handle, vdev_id, skb);
        }
#endif  /* QCA_OL_VLAN_WAR */

        nbuf_debug_del_record(skb);
        netif_rx(skb);
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
    dev->last_rx = jiffies;
#endif
}

#else
#if defined(VDEV_PEER_PROT_COUNT) && !defined(QCA_SUPPORT_WDS_EXTENDED)
#define osif_rx_peer_protocol_count(osifp, soc, vdev_id, skb) \
{ \
    if ((osifp)->peer_protocol_count) { \
        cdp_txrx_peer_protocol_cnt((soc), (vdev_id), (skb),\
                               CDP_VDEV_PEER_PROT_IS_EGRESS, \
                               CDP_VDEV_PEER_PROT_IS_RX); \
    } \
}
#else
#define osif_rx_peer_protocol_count(osifp, soc, vdev_id, skb)
#endif

#if QCA_NSS_PLATFORM
#define osif_send_data(osif, skb, osdev, dev) osif_send_to_nss(osif, skb, osdev->nss_redir_ctx, dev)
#elif QCA_PPE_VP
#define osif_send_data(osif, skb, osdev, dev) osif_send_to_ppe_vp(osif, skb, osdev->ppe_vp_num, dev)
#else
static inline void
osif_send_data(os_if_t osif, struct sk_buff *skb, void *osdev, struct net_device *dev)
{
    osif_rx_peer_protocol_count(osif, soc_txrx_handle,
                          wlan_vdev_get_id(osif->ctrl_vdev), skb);
    skb->protocol = eth_type_trans(skb, dev);
    nbuf_debug_del_record(skb);
    netif_receive_skb(skb);
    return;
}
#endif

#ifdef QCA_SUPPORT_WDS_EXTENDED
void osif_deliver_wds_ext_data_ol(os_if_t osif, struct sk_buff *skb_list)
{
    osif_peer_dev  *osdev = (osif_peer_dev *)osif;

    while (skb_list) {
        struct sk_buff *skb;
        skb = skb_list;
        skb_list = skb_list->next;
	if (skb_list) {
		prefetch(skb_list);
		qdf_prefetch(&skb_list->_skb_refdst);
		prefetch(&skb_list->__pkt_type_offset);
		prefetch(&skb_list->head);
		prefetch(skb_list->data);
	}
        skb->next = NULL;
        qdf_prefetch(skb_shinfo(skb));
        osif_send_data(osif, skb, osdev,
                       ((osif_peer_dev *)(osif))->netdev);
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
    dev->last_rx = jiffies;
#endif
}
#endif /* QCA_SUPPORT_WDS_EXTENDED */

void osif_deliver_data_ol(os_if_t osif, struct sk_buff *skb_list)
{
    struct net_device *dev = OSIF_TO_NETDEV(osif);
    osif_dev  *osdev = (osif_dev *)osif;
    wlan_if_t vap = osdev->os_if;

    while (skb_list) {
        struct sk_buff *skb;
        skb = skb_list;
        skb_list = skb_list->next;
	if (skb_list) {
		prefetch(skb_list);
		prefetch(&skb_list->__pkt_type_offset);
		prefetch(&skb_list->head);
		prefetch(skb_list->data);
	}
        skb->next = NULL;

        if (qdf_unlikely(!osdev->wifi3_0_rx_fast_path))
            if (!osif_deliver_data_ol_extended(&osif, &dev, vap, skb))
                continue;

        osdev = (osif_dev *)osif;
        qdf_prefetch(skb_shinfo(skb));
#ifdef WLAN_FEATURE_11BE_MLO
        if ((IEEE80211_M_HOSTAP == osdev->os_opmode) &&
            qdf_unlikely(MLD_AP_HYBRID_NONBOND_MODE ==
                         cfg_get(wlan_vdev_get_psoc(vap->vdev_obj),
                                 CFG_OL_MLDEV_AP_MODE))) {
            if (CDP_RX_GET_ML_PEER_VALID(skb))
                osif_send_data(osif, skb, osdev, osdev->rx_dev);
            else
                osif_send_data(osif, skb, osdev, dev);
        } else
#endif
            osif_send_data(osif, skb, osdev, osdev->rx_dev);
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
    dev->last_rx = jiffies;
#endif
}
#endif

#if !(QCA_NSS_PLATFORM || QCA_PPE_VP)
#ifdef QCA_SUPPORT_WDS_EXTENDED
void osif_send_wds_ext_data(os_if_t osif,
		       struct sk_buff *skb,
		       struct net_device *dev)
{
    osif_peer_dev *osifp;
    struct net_device *netdev;
    osif_dev *osif_parent = NULL;
    wlan_if_t vap = NULL;

    nbuf_debug_add_record(skb);
    if(dev == NULL) {
        qdf_nofl_info(KERN_CRIT "%s , netdev is NULL, freeing skb", __func__);
        wbuf_free(skb);
        return;
    }

    netdev = (struct net_device *)dev;
    qdf_net_if_hold_dev((struct qdf_net_if *)netdev);
    osifp = ath_netdev_priv(netdev);

    if (osifp->netdev != netdev) {
        qdf_nofl_info(KERN_CRIT "%s , netdev incorrect, freeing skb", __func__);
        wbuf_free(skb);
        qdf_net_if_release_dev((struct qdf_net_if *)netdev);
        return;
    }

    osif_parent = osifp->parent_osif;
    vap = osif_parent->os_if;

#if QCA_OL_VLAN_WAR
    OL_RX_VLAN_WAR(skb, (struct ol_ath_softc_net80211 *)vap->iv_ic);
#endif
    osif_ol_dump_rx_info(vap, skb);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)) && MIPS_TP_ENHC
    prefetch(skb->data);
    prefetch(&skb->protocol);
    prefetch(&skb->__pkt_type_offset);
#endif

#ifdef HOST_OFFLOAD
    /* For the Full Offload solution, diverting the data packet into the
       offload stack for further processing and hand-off to Host processor */
    atd_rx_from_wlan(skb);
    return;
#endif

#ifdef QCA_PARTNER_PLATFORM
    if (osif_pltfrm_deliver_data(osif, skb))
        return;
#endif

    skb->dev = netdev;
    skb->protocol = eth_type_trans(skb, netdev);
#if ATH_SUPPORT_VLAN
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,1,0)
    if ( osif_parent->vlanID != 0)
    {
        /* attach vlan tag */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
        __vlan_hwaccel_put_tag(skb, osif_parent->vlanID);
#else
        __vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), osif_parent->vlanID);
#endif
    }
#else
    if ( osif_parent->vlanID != 0 && osif_parent->vlgrp != NULL)
    {
        /* attach vlan tag */
        vlan_hwaccel_rx(skb, osif_parent->vlgrp,
                        osif_parent->vlanID);
    }
#endif
#endif
    nbuf_debug_del_record(skb);

    if (in_irq())
        netif_rx(skb);
    else
        netif_receive_skb(skb);

    qdf_net_if_release_dev((struct qdf_net_if *)netdev);

    return;
}

void
osif_deliver_wds_ext_data_wifi2(os_if_t osif,
                                struct sk_buff *skb_list)
{
	struct net_device *dev = ((osif_peer_dev *)(osif))->netdev;

	 while (skb_list) {
		 struct sk_buff *skb;
		 skb = skb_list;
		 skb_list = skb_list->next;
		 skb->dev = dev;
		 skb->next = NULL;
		 osif_send_wds_ext_data(osif, skb, ((osif_peer_dev *)(osif))->netdev);
	 }

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
    dev->last_rx = jiffies;
#endif
}
#endif

#endif /*QCA_NSS_PLATFORM*/

#ifdef IPA_OFFLOAD
void osif_deliver_data_ol_ipa_offload(struct sk_buff *skb_list, struct net_device *dev)
{
        osif_dev  *osdev = ath_netdev_priv(dev);
        wlan_if_t vap = osdev->os_if;
        os_if_t osif;

        osif = vap->iv_ifp;
        nbuf_debug_add_record(skb_list);
        osif_deliver_data_ol(osif, skb_list);
}
#endif
#ifdef WLAN_FEATURE_FASTPATH
/*
 * TODO: Move this to a header file
 */
extern void
ol_tx_stats_inc_map_error(ol_txrx_vdev_handle vdev,
                             uint32_t num_map_error);
/**
 * osif_ol_process_tx() - Process common Tx operations for legacy and lithium.
 *
 * @skb - buffer from network stack
 * @dev - net device handle
 *
 * Return - 0 on failure, 1 on success
 */
static int osif_ol_process_tx(struct sk_buff *skb, osif_dev *osdev)
{
    struct net_device *dev = osdev->netdev;
    wlan_if_t vap = osdev->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    struct ol_ath_softc_net80211 *pscn = (struct ol_ath_softc_net80211 *)ic;
    ol_txrx_soc_handle soc_txrx_handle;
    struct wlan_objmgr_vdev *vdev = osdev->ctrl_vdev;
#if QCA_AIRTIME_FAIRNESS
    struct ether_header *eh;
#endif
    cdp_config_param_type value = {0};
#if (MESH_MODE_SUPPORT)
    struct mesh_params params = {0};
#endif

    soc_txrx_handle = wlan_psoc_get_dp_handle(pscn->soc->psoc_obj);

    if (qdf_unlikely(dev && ((dev->flags & (IFF_RUNNING|IFF_UP)) != (IFF_RUNNING|IFF_UP)))) {
        goto bad;
    }

    if (qdf_unlikely((osdev->os_opmode == IEEE80211_M_MONITOR) ||
                      IEEE80211_IS_SMART_MON_VAP(vap))) {
        goto bad;
    }

    /* Allow TX for multilink MLO vaps during cac timeout */
    if (qdf_unlikely(wlan_vdev_chan_config_valid(vap->vdev_obj) != QDF_STATUS_SUCCESS)) {
        /* In case non mlo drop the packet */
        if (!wlan_vdev_mlme_is_mlo_vdev(vdev))
            goto bad;

        if (!wlan_is_ap_cac_timer_running(ic))
            goto bad;
#ifdef WLAN_FEATURE_11BE_MLO
        /* In SLO case drop the packet */
        if (vdev->mlo_dev_ctx && (vdev->mlo_dev_ctx->wlan_vdev_count <= 1))
            goto bad;
#endif
    }

    if (qdf_unlikely(!vap->iv_dfs_tx_allowed))
        goto bad;

#if (MESH_MODE_SUPPORT)
    if (vap->iv_mesh_vap_mode && (vap->mdbg & 0x1)) {
        if (OL_CFG_NONRAW_TX_LIKELINESS(vap->iv_tx_encap_type != osif_pkt_type_raw)) {
            params.mhdr = vap->mhdr;
            params.mdbg = vap->mdbg;
            params.mhdr_len = vap->mhdr_len;
            if (add_mesh_meta_hdr(skb, &params)) {
                goto bad;
            }
            vap->mhdr = params.mhdr;
        }
    }
#endif

#if QCA_AIRTIME_FAIRNESS
    eh = (struct ether_header *)(wbuf_header(skb) + vap->mhdr_len);
    if (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_AR9888 &&
        lmac_get_tgt_version(pscn->soc->psoc_obj) == AR9888_REV2_VERSION) {
        if (target_if_atf_is_tx_traffic_blocked(vap->vdev_obj, eh->ether_dhost, skb)) {
            goto bad;
        }
    }
#endif

#if UMAC_SUPPORT_WNM
    if (wlan_wnm_tfs_filter(vap, (wbuf_t) skb)) {
        goto bad;
    }
#endif

    return 1;

bad:
    value.cdp_pdev_param_osif_drop = 1;
    cdp_txrx_set_pdev_param(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(vap->iv_ic->ic_pdev_obj), CDP_OSIF_DROP, value);
    qdf_nbuf_free(skb);
    return 0;
}

/**
 * osif_ol_process_tx_synchronous() - Process common Tx operations for legacy and lithium.
 *
 * @skb - buffer from network stack
 * @dev - net device handle
 *
 * This function is called with lock held.
 *
 * Return - 0 on failure, 1 on success
 */
static int osif_ol_process_tx_synchronous(struct sk_buff **skb, osif_dev **osdev,
                                          struct cdp_tx_exception_metadata *tx_exc_param,
                                          bool *is_exception)
{
    wlan_if_t vap = (*osdev)->os_if;
    struct wlan_objmgr_vdev *vdev = (*osdev)->ctrl_vdev;
    uint8_t src_mac[IEEE80211_ADDR_LEN];
    struct wlan_objmgr_peer *peer = NULL;
    qdf_ether_header_t *eh;
    bool extap_repeater = false;
    struct cdp_ast_entry_info ast_entry_info = {0};
    int ast_entry_found = 0;
#ifdef QCA_OL_DMS_WAR
    int ret;
    uint8_t peer_addr[QDF_MAC_ADDR_SIZE];
#endif
    struct net_device *comdev = (*osdev)->os_comdev;
    struct ol_ath_softc_net80211 *scn = ath_netdev_priv(comdev);
    uint8_t pdev_id =  wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);
    ol_txrx_soc_handle soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    cdp_config_param_type value = {0};

    if ((OL_CFG_RAW_TX_LIKELINESS(vap->iv_tx_encap_type == osif_pkt_type_raw))
	    || (vap->iv_tx_encap_type == osif_pkt_type_native_wifi)) {
        /* In Raw Mode, the payload normally comes encrypted by an external
         * Access Controller and we won't have the keys. Besides, the format
         * isn't 802.3/Ethernet II.
         * Hence, VLAN WAR, Ext AP functionality, VoW debug and Multicast to
         * Unicast conversion aren't applicable, and are skipped.
         *
         * Additionally, TSO and nr_frags based scatter/gather are currently
         * not required and thus not supported with Raw Mode.
         *
         * Error conditions are handled internally by the below function.
         */
        OL_TX_LL_UMAC_RAW_PROCESS((*osdev), skb);
        return 0;
    }
#if MESH_MODE_SUPPORT
    if (!vap->iv_mesh_vap_mode)
#endif
    {


        /*
         * Perform DBDC tx process before the EXTAP TX
         */
        if (qdf_unlikely(wlan_rptr_vdev_is_extap(vdev))
            && (wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE)) {
            if (((lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCA8074V2) ||
                 (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCA9574) ||
                 (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCN9000) ||
                 (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCN9224) ||
                 (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCA5332) ||
                 (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCA5018) ||
                 (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCN6122) ||
                 (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCN9160) ||
                 (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCA6018) ||
                 (lmac_get_tgt_type(scn->soc->psoc_obj) == TARGET_TYPE_QCN6432))) {
                qdf_mem_copy(src_mac, ((*skb)->data + vap->mhdr_len + ETH_ALEN), ETH_ALEN);
                extap_repeater = true;
#if DBDC_REPEATER_SUPPORT
                /*
                 * QCA Repeater Processing.
                 */
                if (qca_multi_link_sta_tx((*osdev)->netdev, *skb,
                                          osif_get_mld_netdev(*osdev))) {
                    return 0;
                }
#endif
            }
        }

        /* Raw mode or native wifi mode not
         * supported in qwrap , revisit later
         */
        if (ADP_EXT_AP_TX_PROCESS(vdev, skb, vap->mhdr_len, NULL)) {
            goto bad;
        }

        /*
         * For HKv2, add HM_SEC ast entry for EXTAP Repeater ethernet backend
         */
        if (qdf_unlikely(extap_repeater)) {
            if (wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE) {
                eh = (qdf_ether_header_t *)((*skb)->data + vap->mhdr_len);
                peer = wlan_vdev_get_bsspeer(vdev);
                if (IEEE80211_IS_MULTICAST(eh->ether_dhost) && peer) {
                    ast_entry_found = cdp_peer_get_ast_info_by_soc(soc_txrx_handle, src_mac,
                                                                    &ast_entry_info);
                    if (!ast_entry_found) {
                        cdp_peer_add_ast(soc_txrx_handle, wlan_vdev_get_id(vdev), peer->macaddr,
                                         src_mac, CDP_TXRX_AST_TYPE_WDS_HM_SEC, IEEE80211_NODE_F_WDS_HM);
                    }
                }
            }
        }

#if QCA_OL_VLAN_WAR
	if(OL_TX_VLAN_WAR(skb, scn))
	    goto bad;
#endif /* QCA_OL_VLAN_WAR */

#ifdef QCA_OL_DMS_WAR
	if (vap->dms_amsdu_war) {
            if (ol_check_valid_dms(skb, vap, &peer_addr[0])) {
                if (vap->iv_hw_dms_support_enabled)
                    ret = OL_DMS_AMSDU(soc_txrx_handle, wlan_vdev_get_id(vdev),
                                       &peer_addr[0], tx_exc_param);
                else
                    ret = OL_DMS_AMSDU_WAR(soc_txrx_handle, skb, &peer_addr[0],
                                           tx_exc_param, vap->iv_myaddr);
                if (ret)
                    goto bad;
                *is_exception = true;
            }
        }
#endif

        (*skb)->next = NULL;

        OL_TX_LL_UMAC_VAP_HARDSTART_VOW_DEBUG(*osdev, *skb);
    }

#if CONFIG_DP_TRACE
    qdf_nbuf_classify_pkt(*skb);
    qdf_dp_trace_log_pkt(0, *skb, QDF_TX, pdev_id);
    QDF_NBUF_CB_TX_PACKET_TRACK(*skb) = QDF_NBUF_TX_PKT_DATA_TRACK;
    QDF_NBUF_UPDATE_TX_PKT_COUNT(*skb, QDF_NBUF_TX_PKT_HDD);
    qdf_dp_trace_set_track(*skb, QDF_TX);
    DPTRACE(qdf_dp_trace(*skb, QDF_DP_TRACE_HDD_TX_PACKET_RECORD, pdev_id,
            (uint8_t *)(*skb)->data, qdf_nbuf_len(*skb), QDF_TX));
    if (qdf_nbuf_len(*skb) > QDF_DP_TRACE_RECORD_SIZE) {
        DPTRACE(qdf_dp_trace(*skb, QDF_DP_TRACE_HDD_TX_PACKET_RECORD,
            pdev_id, (uint8_t *)&(*skb)->data[QDF_DP_TRACE_RECORD_SIZE],
            (qdf_nbuf_len(*skb)-QDF_DP_TRACE_RECORD_SIZE), QDF_TX));
    }
#endif

    return 1;

bad:
    value.cdp_pdev_param_osif_drop = 1;
    cdp_txrx_set_pdev_param(soc_txrx_handle, pdev_id, CDP_OSIF_DROP, value);
    if ((*skb) != NULL)
        qdf_nbuf_free(*skb);
    return 0;
}

#ifdef VDEV_PEER_PROT_COUNT
#define osif_tx_peer_protocol_cnt(osdev, soc, vdev_id, skb) \
{ \
    if ((osdev)->peer_protocol_cnt) { \
        cdp_txrx_peer_protocol_cnt((soc), (vdev_id), (skb), \
                               CDP_VDEV_PEER_PROT_IS_INGRESS, \
                               CDP_VDEV_PEER_PROT_IS_TX); \
    } \
}
#else
#define osif_tx_peer_protocol_cnt(osdev, soc, vdev_id, skb)
#endif

#ifndef IPA_OFFLOAD
static inline
struct sk_buff *osif_dp_xmit(struct sk_buff *skb, osif_dev  *osdev,
	     ol_txrx_soc_handle soc_txrx_handle,
	     uint8_t fast_flags)
{
	if (qdf_likely(qdf_nbuf_fast_xmit(skb) && (fast_flags & WIFI_3_0_ULTRA_FASTPATH)))
		skb = ((ol_txrx_tx_fast_fp)osdev->iv_vap_fast_send)(soc_txrx_handle, wlan_vdev_get_id(osdev->ctrl_vdev), skb);
	else
		skb = ((ol_txrx_tx_fp)osdev->iv_vap_send)(soc_txrx_handle, wlan_vdev_get_id(osdev->ctrl_vdev), skb);

	return skb;
}
#else
static inline
struct sk_buff *osif_dp_xmit(struct sk_buff *skb, osif_dev  *osdev,
	     ol_txrx_soc_handle soc_txrx_handle,
	     uint8_t fast_flags)
{
	skb = ((ol_txrx_tx_fp)osdev->iv_vap_send)(soc_txrx_handle, wlan_vdev_get_id(osdev->ctrl_vdev), skb);

	return skb;
}
#endif

static inline __attribute__((always_inline)) int
osif_ol_vap_send_wifi3_fast(struct sk_buff *skb, osif_dev *osdev, uint8_t fast_flags)
{
    struct net_device *dev = osdev->netdev;
    wlan_if_t vap = osdev->os_if;
    struct wlan_objmgr_vdev *vdev = osdev->ctrl_vdev;
    ol_txrx_soc_handle soc_txrx_handle = NULL;

    nbuf_debug_add_record(skb);
    if (qdf_unlikely(dev && ((dev->flags & (IFF_RUNNING|IFF_UP)) != (IFF_RUNNING|IFF_UP)))) {
        goto bad;
    }

    if (qdf_unlikely(vap == NULL))
        goto bad;


    /* Allow TX for multilink MLO vaps during cac timeout */
    if (qdf_unlikely(wlan_vdev_chan_config_valid(vap->vdev_obj) != QDF_STATUS_SUCCESS)) {
        /* In case non mlo drop the packet */
        if (!wlan_vdev_mlme_is_mlo_vdev(vdev))
            goto bad;

        if (!wlan_is_ap_cac_timer_running(vap->iv_ic))
            goto bad;

#ifdef WLAN_FEATURE_11BE_MLO
        /* In SLO case drop the packet */
        if (vdev->mlo_dev_ctx && (vdev->mlo_dev_ctx->wlan_vdev_count <= 1))
            goto bad;
#endif
    }

    if (qdf_unlikely(!vap->iv_dfs_tx_allowed))
        goto bad;

    soc_txrx_handle = wlan_psoc_get_dp_handle(wlan_pdev_get_psoc(wlan_vdev_get_pdev(vdev)));

    skb = osif_dp_xmit(skb, osdev, soc_txrx_handle, fast_flags);

bad:
    if (skb != NULL) {
        qdf_nbuf_free(skb);
    }
    return 0;
}


static inline __attribute__((always_inline)) int
osif_ol_vap_send_wifi3(struct sk_buff *skb, osif_dev *osdev, struct cdp_tx_exception_metadata *tx_exc_param, bool is_exception)
{
#if defined(ATH_SUPPORT_WRAP) || defined(DBDC_REPEATER_SUPPORT)
    struct net_device *dev = osdev->netdev;
#endif
    wlan_if_t vap = osdev->os_if;
    void *soc;
    struct wlan_objmgr_vdev *vdev = osdev->ctrl_vdev;
#ifdef SKB_GET_SAWF_TAG
    uint32_t nw_delay;
#endif
    ol_txrx_soc_handle soc_txrx_handle = wlan_psoc_get_dp_handle(wlan_pdev_get_psoc(wlan_vdev_get_pdev(vdev)));
    cdp_config_param_type value = {0};

#if defined(CONFIG_WIFI_EMULATION_WIFI_3_0) && defined (BUILD_X86) && !defined (QCA_WIFI_QCN9000)
    uint32_t lowmem_alloc_tries = 0;
    struct sk_buff *skb_orig;

    /* Hawkeye M2M emulation cannot handle memory addresses below 0x50000000
     * Though we are trying to reserve low memory upfront to prevent this,
     * we sometimes see SKBs allocated from low memory.
     */
    skb_orig = skb;
    while (virt_to_phys(skb->data) < 0x50000040) {
        lowmem_alloc_tries++;
        if (lowmem_alloc_tries > 100) {
            if (skb_orig->sk) {
                /* account system memory */
                atomic_sub(skb_orig->truesize,
                    &(skb_orig->sk->sk_wmem_alloc));
            }
            return 0;
        } else {
            skb = skb_copy(skb_orig, GFP_KERNEL);
        }
    }
#endif
    nbuf_debug_add_record(skb);

    if (qdf_unlikely(vap == NULL)) {
        qdf_nbuf_free(skb);
        return 0;
    }

    osif_tx_peer_protocol_cnt(osdev, soc_txrx_handle,
                          wlan_vdev_get_id(osdev->ctrl_vdev), skb);

    value.cdp_pdev_param_ingrs_stats = 1;
    cdp_txrx_set_pdev_param(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(vap->iv_ic->ic_pdev_obj), CDP_INGRESS_STATS, value);

#ifdef SKB_GET_SAWF_TAG
    if (qdf_unlikely((SKB_GET_SAWF_TAG(skb->mark) == SKB_SAWF_VALID_TAG))) {
        /* Calculate n/w-delay only if n/w timestamp is valid */
        if (qdf_nbuf_get_timestamp(skb)) {
            nw_delay = (uint32_t)qdf_nbuf_get_timedelta_us(skb);
            qdf_nbuf_set_tx_fctx_type(skb, (void *)((uintptr_t)nw_delay),
                                      CB_FTYPE_SAWF);
        }
    }
#endif

    qdf_nbuf_set_timestamp(skb);

    if (!osif_ol_process_tx(skb, osdev)) {
        return 0;
    }

    spin_lock_bh(&osdev->tx_lock);

    soc = wlan_psoc_get_dp_handle(wlan_pdev_get_psoc(vap->iv_ic->ic_pdev_obj));
#if DBDC_REPEATER_SUPPORT
    /*
     *Skip dbdc process when vap is mpsta or if extap is enabled
     */
#if ATH_SUPPORT_WRAP
    if (qdf_unlikely(wlan_rptr_vdev_is_mpsta(vap->vdev_obj))) {
    }
#endif
    if (qdf_unlikely(wlan_rptr_vdev_is_extap(vdev))) {
	    goto skip_lag_tx_process;
    }

    /*
     * QCA Repeater Processing.
     */
    if (wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE) {
        if (dev && qca_multi_link_sta_tx(dev, skb, osif_get_mld_netdev(osdev))) {
            spin_unlock_bh(&osdev->tx_lock);
            return 0;
        }
    }
#endif
#if DBDC_REPEATER_SUPPORT
skip_lag_tx_process:
#endif
#if ATH_SUPPORT_WRAP
#if MESH_MODE_SUPPORT
    if (!vap->iv_mesh_vap_mode) {
#endif
       spin_unlock_bh(&osdev->tx_lock);
       if (dev) {
           if (dp_wrap_tx_process(&dev, vap->vdev_obj, &skb)) {
               goto bad;
           } else {
               osdev = ath_netdev_priv(dev);
           }
       }

       spin_lock_bh(&osdev->tx_lock);
#if MESH_MODE_SUPPORT
    }
#endif
#endif

    if (!osif_ol_process_tx_synchronous(&skb, &osdev, tx_exc_param, &is_exception)) {
        spin_unlock_bh(&osdev->tx_lock);
        return 0;
    }

    spin_unlock_bh(&osdev->tx_lock);

    if (is_exception) {
        /*
         * Additional reference taken here so that on MSDU completions nbuf is not freed.
         * And on PPDU completions the ppdu cookie can be given back to the customer stack
         */
        if (tx_exc_param->is_tx_sniffer)
            qdf_nbuf_ref(skb);

        skb = ((ol_txrx_tx_exc_fp)osdev->iv_vap_send_exc)(soc,  wlan_vdev_get_id(osdev->ctrl_vdev), skb, tx_exc_param);

        /*
         * Release the additional reference taken for sniffer
         */
        if (skb != NULL && tx_exc_param->is_tx_sniffer)
            qdf_nbuf_free(skb);
    } else {
        skb = ((ol_txrx_tx_fp)osdev->iv_vap_send)(soc_txrx_handle,  wlan_vdev_get_id(osdev->ctrl_vdev), skb);
    }

#if ATH_SUPPORT_WRAP
bad:
#endif
    if (skb != NULL) {
        qdf_nbuf_free(skb);
    }
    return 0;
}

static inline void
osif_ol_vap_init_exception_metadata(struct cdp_tx_exception_metadata *tx_exc_param)
{
    memset(tx_exc_param, 0, sizeof(struct cdp_tx_exception_metadata));
    tx_exc_param->tx_encap_type = CDP_INVALID_TX_ENCAP_TYPE;
    tx_exc_param->sec_type = CDP_INVALID_SEC_TYPE;
    tx_exc_param->peer_id = CDP_INVALID_PEER;
    tx_exc_param->tid = CDP_INVALID_TID;
}

int
osif_ol_vap_send_exception_wifi3(struct sk_buff *skb, struct net_device *dev, void *mdata)
{
    struct tx_sniffer_meta_hdr *mhdr = (struct tx_sniffer_meta_hdr *)mdata;
    struct cdp_tx_exception_metadata tx_exc_param;

    osif_ol_vap_init_exception_metadata(&tx_exc_param);
    tx_exc_param.is_tx_sniffer = 1;
    tx_exc_param.ppdu_cookie = mhdr->ppdu_cookie;
    return osif_ol_vap_send_wifi3(skb, ath_netdev_priv(dev), &tx_exc_param, true);
}

int
osif_ol_vap_hardstart_wifi3_wrapper(struct sk_buff *skb, void *os_dev)
{
    osif_dev *osdev = (osif_dev *)os_dev;
    struct cdp_tx_exception_metadata tx_exc_param;
    uint8_t fast_flags = osdev->wifi3_0_fast_path;
    qdf_ether_header_t *eh;

    if (qdf_likely(qdf_nbuf_fast_xmit(skb) && (fast_flags & WIFI_3_0_ULTRA_FASTPATH) &&
                   !qdf_nbuf_pkt_type_is_mcast(skb)))
        return osif_ol_vap_send_wifi3_fast(skb, osdev, fast_flags);

    if (!qdf_nbuf_fast_xmit(skb)) {
        eh = (qdf_ether_header_t *) skb->data;
        if (IEEE80211_IS_BROADCAST(eh->ether_dhost) && !(qdf_nbuf_pkt_type_is_bcast(skb)))
            skb->pkt_type = PACKET_BROADCAST;
    }
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MCAST_MLO)
    if (osdev->mldev && wlan_vdev_mlme_is_ap(osdev->ctrl_vdev)) {
        eh = (qdf_ether_header_t *) skb->data;

        if (IEEE80211_IS_MULTICAST(eh->ether_dhost)) {
            wlan_if_t vap = osdev->os_if;
            ol_txrx_soc_handle soc;
            uint8_t vdev_id;

            if ((qdf_nbuf_get_tx_ftype(skb) == CB_FTYPE_MLO_MCAST)
#if MESH_MODE_SUPPORT
                && !(vap->iv_mesh_vap_mode && (vap->mdbg & 0x1))
#endif
            ) {
                osif_ol_vap_init_exception_metadata(&tx_exc_param);
                tx_exc_param.is_mlo_mcast = 1;
                return osif_ol_vap_send_wifi3(skb, osdev, &tx_exc_param, true);
            }

            soc = wlan_psoc_get_dp_handle(wlan_vdev_get_psoc(vap->vdev_obj));
            vdev_id = wlan_vdev_get_id(vap->vdev_obj);

            if ((MC_ME_DISABLE == dp_get_me_mode(soc, vdev_id))
#if MESH_MODE_SUPPORT
                && !(vap->iv_mesh_vap_mode && (vap->mdbg & 0x1))
#endif
            ) {
                  /* Dont allow mcast packet xmit on link netdev if the
                   * vdev is operating in MLO AP non-mesh mode with ME
                   * disabled.
                   */
                  nbuf_debug_add_record(skb);
                  qdf_nbuf_free(skb);
                  return 0;
            }
        }
    }
#endif

    if (qdf_likely(fast_flags && !qdf_nbuf_pkt_type_is_mcast(skb))) {
        return osif_ol_vap_send_wifi3_fast(skb, osdev, fast_flags);
    }

    osif_ol_vap_init_exception_metadata(&tx_exc_param);
    return osif_ol_vap_send_wifi3(skb, osdev, &tx_exc_param, false);
}
qdf_export_symbol(osif_ol_vap_hardstart_wifi3_wrapper);

int
osif_ol_vap_hardstart_wifi3(struct sk_buff *skb, struct net_device *dev)
{
    /*Set cb packet xmit type to DP_XMIT_LINK*/
    qdf_nbuf_set_vdev_xmit_type(skb, DP_XMIT_LINK);
    return osif_ol_vap_hardstart_wifi3_wrapper(skb, (void *)ath_netdev_priv(dev));
}

#ifdef IPA_OFFLOAD
QDF_STATUS
osif_ol_vap_hardstart_wifi3_ipa_offload(struct sk_buff *skb, struct net_device *dev)
{
        int ret = 0;
        ret = osif_ol_vap_hardstart_wifi3(skb, dev);
        return qdf_status_from_os_return(ret);
}
#endif

#ifdef QCA_SUPPORT_WDS_EXTENDED
int
osif_wds_ext_peer_hardstart_wifi3(struct sk_buff *skb, struct net_device *dev)
{
    struct cdp_tx_exception_metadata tx_exc_param;
    osif_dev  *parent_osdev = NULL;
    osif_peer_dev *osdev = ath_netdev_priv(dev);
    struct ether_header *eh = NULL;
    uint8_t is_mcast;
    uint8_t fast_flags;
#ifdef SKB_GET_SAWF_TAG
    uint32_t nw_delay;
#endif
    osif_peer_dev *osifp  = ath_netdev_priv(dev);

    /*
     * WDS_ext interfaces will be created for MLO/Non-MLO clients, based on the
     * is_mlo, the ingress stats traffic will be accumulated at respective index
     */
    nbuf_debug_add_record(skb);
    qdf_nbuf_set_vdev_xmit_type(skb, osifp->is_mlo);
    dev = osdev->parent_netdev;
    parent_osdev = osdev->parent_osif;
    fast_flags = parent_osdev->wifi3_0_fast_path;

    if (qdf_likely(qdf_nbuf_fast_xmit(skb) && (fast_flags & WIFI_3_0_ULTRA_FASTPATH) &&
                   !qdf_nbuf_pkt_type_is_mcast(skb))) {
        nbuf_debug_del_record(skb);
        return osif_ol_vap_send_wifi3_fast(skb, parent_osdev, fast_flags);
    }

    eh = (struct ether_header *)skb->data;
    is_mcast = IEEE80211_IS_MULTICAST(eh->ether_dhost);

    if (qdf_likely(!is_mcast)) {
#ifdef SKB_GET_SAWF_TAG
        if (qdf_unlikely((SKB_GET_SAWF_TAG(skb->mark) == SKB_SAWF_VALID_TAG))) {
            /* Calculate n/w-delay only if n/w timestamp is valid */
            if (qdf_nbuf_get_timestamp(skb)) {
                nw_delay = (uint32_t)qdf_nbuf_get_timedelta_us(skb);
                qdf_nbuf_set_tx_fctx_type(skb, (void *)((uintptr_t)nw_delay),
                                      CB_FTYPE_SAWF);
            }
        }
#endif
        qdf_nbuf_set_timestamp(skb);
        nbuf_debug_del_record(skb);
        return osif_ol_vap_send_wifi3_fast(skb, parent_osdev, fast_flags);
    }

    if (!qdf_nbuf_fast_xmit(skb)) {
        if (IEEE80211_IS_BROADCAST(eh->ether_dhost) && !(qdf_nbuf_pkt_type_is_bcast(skb)))
            skb->pkt_type = PACKET_BROADCAST;
    }

    /*
     * WDS Ext Tx handling
     */
    osif_ol_vap_init_exception_metadata(&tx_exc_param);
    tx_exc_param.peer_id = osdev->peer_id;
    tx_exc_param.is_wds_extended_mc_bc = 1;

    if (qdf_unlikely(dev && ((dev->flags & (IFF_RUNNING|IFF_UP)) != (IFF_RUNNING|IFF_UP)))) {
        qdf_nbuf_free(skb);
        return 0;
    }

    nbuf_debug_del_record(skb);
    return osif_ol_vap_send_wifi3(skb, parent_osdev, &tx_exc_param, true);
}
#endif

/*
 * OS entry point for Fast Path 11AC offload data-path
 * NOTE : It is unlikely that we need lock protection
 * here, since we are called under OS lock (HARD_TX_LOCK()))
 * in linux. So this function is can be called by a single
 * at a given instance of time.

 * TODO : This function does not implement full fledged packet
 * batching. This function receives a single packet and calls
 * the underlying API. The subsequent OL layer API's however,
 * can operate on batch of packets and hence is provided a packet
 * array (special case, array size = 1).
 */
int
osif_ol_legacy_vap_hardstart(struct sk_buff *skb,
			 struct net_device *dev,
			 struct cdp_tx_exception_metadata *tx_exc_param,
			 bool is_exception)
{

    osif_dev  *osdev = ath_netdev_priv(dev);
    wlan_if_t vap = osdev->os_if;
    ol_txrx_soc_handle soc_txrx_handle =
        wlan_psoc_get_dp_handle(wlan_pdev_get_psoc(vap->iv_ic->ic_pdev_obj));
    struct cdp_host_stats_ops *stats_ops = soc_txrx_handle->ops->host_stats_ops;
#if QCA_PARTNER_DIRECTLINK_TX
    struct ether_header *eh;
#endif
#if UMAC_VOW_DEBUG
    struct ol_txrx_nbuf_classify nbuf_class;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)) && MIPS_TP_ENHC
    prefetch(&skb->len);
    prefetch(skb->data);
#endif
    if (stats_ops->txrx_update_vdev_stats) {
        stats_ops->txrx_update_vdev_stats(soc_txrx_handle,  wlan_vdev_get_id(osdev->ctrl_vdev), skb,
                                          DP_VDEV_STATS_PKT_CNT_ONLY,
                                          DP_XMIT_LINK);
    }

    nbuf_debug_add_record(skb);
    qdf_nbuf_set_ext_cb(skb, NULL);
    if (!osif_ol_process_tx(skb, osdev)) {
        return 0;
    }

    /*
     * Classify packet for Video and query nss driver action whether
     * to accelerate the packet or drop it
     */
#if UMAC_VOW_DEBUG
    if (qdf_unlikely(osdev->carrier_vow_config)) {
        cdp_txrx_classify_and_update(soc_txrx_handle,  wlan_vdev_get_id(osdev->ctrl_vdev), skb, tx_direction, &nbuf_class);
    }
#endif
    OSIF_VAP_TX_LOCK(soc_txrx_handle, osdev);

    /* Update packet count */
    /* Note: it is for the user to use skb_unshare with caution */

#if QCA_PARTNER_DIRECTLINK_TX
    if (qdf_unlikely(osdev->is_directlink)) {
        OSIF_VAP_TX_UNLOCK(soc_txrx_handle, osdev);
        ol_tx_partner(skb, dev, CDP_INVALID_PEER);
        return 0;
    } else
#endif /* QCA_PARTNER_DIRECTLINK_TX */

    {
#if MESH_MODE_SUPPORT
        if (!vap->iv_mesh_vap_mode)
#endif
        {
#if ATH_SUPPORT_WRAP
            /* QWRAP componentization support is not added for
               legacy platforms */
#endif
        }

        if (!osif_ol_process_tx_synchronous(&skb, &osdev, tx_exc_param, &is_exception)) {
            goto out;
        }

        if (qdf_unlikely(is_exception)) {
            skb = ((ol_txrx_tx_exc_fp)osdev->iv_vap_send_exc)(soc_txrx_handle, wlan_vdev_get_id(osdev->ctrl_vdev), skb, tx_exc_param);
        } else {
            skb = ((ol_txrx_tx_fp)osdev->iv_vap_send)(soc_txrx_handle, wlan_vdev_get_id(osdev->ctrl_vdev), skb);
        }

        if (skb != NULL)
            goto bad;

out:
        OSIF_VAP_TX_UNLOCK(soc_txrx_handle, osdev);
        return 0;
    }

bad:
    OSIF_VAP_TX_UNLOCK(soc_txrx_handle, osdev);

    if (skb != NULL)
        qdf_nbuf_free(skb);
    return 0;
}

int
osif_ol_ll_vap_hardstart(struct sk_buff *skb, struct net_device *dev)
{
    struct cdp_tx_exception_metadata tx_exc_param = {0};

	return osif_ol_legacy_vap_hardstart(skb, dev, &tx_exc_param, false);
}

#if QCA_SUPPORT_WDS_EXTENDED && !(QCA_NSS_PLATFORM || QCA_PPE_VP)
int
osif_wds_ext_peer_hardstart_wifi2(struct sk_buff *skb, struct net_device *dev)
{
    struct cdp_tx_exception_metadata tx_exc_param = {0};
    osif_peer_dev *osdev = ath_netdev_priv(dev);
    struct ether_header *eh = NULL;
    uint8_t is_mcast;
    bool is_exception = false;
    int retval;

    dev = osdev->parent_netdev;
    eh = (struct ether_header *)skb->data;
    is_mcast = IEEE80211_IS_MULTICAST(eh->ether_dhost);

    /*
     * WDS Ext Tx handling
     */
    if (qdf_likely(is_mcast)) {
        is_exception = true;
        tx_exc_param.tx_encap_type = CDP_INVALID_TX_ENCAP_TYPE;
        tx_exc_param.sec_type = CDP_INVALID_SEC_TYPE;
        tx_exc_param.peer_id = osdev->peer_id;
        tx_exc_param.tid = CDP_INVALID_TID;
        tx_exc_param.is_wds_extended_mc_bc = 1;
    }

    retval = osif_ol_legacy_vap_hardstart(skb, dev,
                                          &tx_exc_param,
                                          is_exception);

    return retval;
}
#endif
#endif /* WLAN_FEATURE_FASTPATH */

#if ATH_PERF_PWR_OFFLOAD
#ifdef UMAC_SUPPORT_PROXY_ARP
extern int do_proxy_arp(wlan_if_t vap, qdf_nbuf_t netbuf);
#endif /* UMAC_SUPPORT_PROXY_ARP */
extern void osif_receive_monitor_80211_base (os_if_t osif, wbuf_t wbuf,
                                        ieee80211_recv_status *rs);

QDF_STATUS osif_getkey_ol(osif_dev *osdev, uint8_t *key_buf, uint8_t *mac_addr, uint8_t keyix)
{
    wlan_if_t vap = osdev->os_if;
    ieee80211_keyval k = {0};

    k.keydata = key_buf;
    if (wlan_get_key(vap, keyix, mac_addr, &k,
                     IEEE80211_KEYBUF_SIZE+IEEE80211_MICBUF_SIZE,
                     GET_PN_DISABLE)) {
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}

#ifdef UMAC_SUPPORT_PROXY_ARP
int
osif_proxy_arp_ol(os_if_t osif, qdf_nbuf_t netbuf)
{
    osif_dev  *osdev = (osif_dev *)osif;
    wlan_if_t vap = osdev->os_if;

    return(do_proxy_arp(vap, netbuf));
}
#endif /* UMAC_SUPPORT_PROXY_ARP */

#if ATH_SUPPORT_WAPI
extern bool osif_wai_check(os_if_t osif,
                struct sk_buff *skb_list_head, struct sk_buff *skb_list_tail);
#endif /* ATH_SUPPORT_WAPI */

void osif_vap_setup_ol (struct ieee80211vap *vap, osif_dev *osifp) {
        struct ol_txrx_ops ops = {0};
        struct ieee80211com *ic = vap->iv_ic;
        ol_txrx_soc_handle soc_txrx_handle;
        uint8_t vdev_id = wlan_vdev_get_id(vap->vdev_obj);

        soc_txrx_handle = wlan_psoc_get_dp_handle(wlan_pdev_get_psoc(ic->ic_pdev_obj));
        /*
         * This function registers rx and monitor functions,
         * and a callback handle.
         * It fills in the transmit handler to be called from shim.
         */
        ops.rx.rx = (ol_txrx_rx_fp) osif_deliver_data_ol;
#ifdef QCA_SUPPORT_EAPOL_OVER_CONTROL_PORT
        ops.rx.rx_eapol = (ol_txrx_rx_fp) osif_deliver_eapol_over_nl;
#endif
        ops.get_key = (ol_txrx_get_key_fp) osif_getkey_ol;
#if QCA_SUPPORT_RAWMODE_PKT_SIMULATION

        ops.rx.rsim_rx_decap = wlan_rawsim_api_get_rx_decap();

#endif
#if MESH_MODE_SUPPORT
	ops.tx.tx_free_ext = (ol_txrx_tx_free_ext_fp)os_if_tx_free_ext;
#endif

#if ATH_SUPPORT_WAPI
        ops.rx.wai_check = (ol_txrx_rx_check_wai_fp) osif_wai_check;
#endif
        ops.rx.mon = (ol_txrx_rx_mon_fp) osif_receive_monitor_80211_base;
#ifdef UMAC_SUPPORT_PROXY_ARP
        ops.proxy_arp = (ol_txrx_proxy_arp_fp) osif_proxy_arp_ol;
#endif
        cdp_vdev_register(soc_txrx_handle,
            vdev_id, (void *) osifp, &ops);
        osifp->iv_vap_send = ops.tx.tx;
        osifp->iv_vap_fast_send = ops.tx.tx_fast;
        osifp->iv_vap_send_exc = ops.tx.tx_exception;

        osifp->is_ar900b = cdp_is_target_ar900b(soc_txrx_handle);
        return;
}
#endif /* ATH_PER_PWR_OFFLOAD */



#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
int osif_nss_ol_extap_rx(struct net_device *dev, struct sk_buff *skb)
{
    osif_dev  *osdev = ath_netdev_priv(dev);
    struct wlan_objmgr_vdev *vdev = osdev->ctrl_vdev;

    return ADP_EXT_AP_RX_PROCESS(vdev, skb);
}

int osif_nss_ol_extap_tx(struct net_device *dev, struct sk_buff *skb)
{
    osif_dev  *osdev = ath_netdev_priv(dev);
    wlan_if_t vap = osdev->os_if;
    int status;
    struct wlan_objmgr_vdev *vdev = osdev->ctrl_vdev;
    struct dp_extap_nssol extap_nssol;
    uint16_t ip_version;
    uint8_t src_mac[IEEE80211_ADDR_LEN];

    OS_MEMSET(&extap_nssol, 0, sizeof(struct dp_extap_nssol));

    qdf_mem_copy(src_mac, (skb->data + vap->mhdr_len + ETH_ALEN), ETH_ALEN);
    status = ADP_EXT_AP_TX_PROCESS(vdev, &skb, vap->mhdr_len, &extap_nssol);

    ip_version = extap_nssol.ip_version;
    if ((ip_version != 0) && osdev->nss_wifiol_ctx && vap->iv_ic->nss_vops) {
        vap->iv_ic->nss_vops->ic_osif_nss_vdev_extap_table_entry_add(osdev, ip_version,
                (uint8_t *)&extap_nssol.ip, extap_nssol.mac);
    }

    return status;
}

int
osif_nss_ol_vap_hardstart(struct sk_buff *skb, struct net_device *dev)
{
    osif_dev  *osdev ;
    wlan_if_t vap ;
    struct wlan_objmgr_vdev *vdev;
    ol_txrx_soc_handle soc_txrx_handle;
    struct cdp_host_stats_ops *stats_ops;
#if DBDC_REPEATER_SUPPORT || QCA_OL_VLAN_WAR
    struct ieee80211com *ic = NULL;
    struct ol_ath_softc_net80211 *pscn = NULL;
#endif
    uint8_t vdev_id;
#if (MESH_MODE_SUPPORT)
    struct mesh_params params = {0};
#endif

    nbuf_debug_add_record(skb);
    qdf_nbuf_set_ext_cb(skb, NULL);
    if (qdf_unlikely((dev->flags & (IFF_RUNNING|IFF_UP)) != (IFF_RUNNING|IFF_UP))) {
        qdf_nbuf_free(skb);
        return 0;
    }

    osdev = ath_netdev_priv(dev);

    if (qdf_unlikely(osdev->os_opmode == IEEE80211_M_MONITOR)) {
        qdf_nbuf_free(skb);
        return 0;
    }

    vap = osdev->os_if;
    vdev = osdev->ctrl_vdev;

    soc_txrx_handle = wlan_psoc_get_dp_handle(
		              wlan_pdev_get_psoc(vap->iv_ic->ic_pdev_obj));
    vdev_id = wlan_vdev_get_id(vap->vdev_obj);
    stats_ops = soc_txrx_handle->ops->host_stats_ops;
    if (stats_ops->txrx_update_vdev_stats) {
        stats_ops->txrx_update_vdev_stats(soc_txrx_handle, vdev_id, skb,
                                          DP_VDEV_STATS_PKT_CNT_ONLY,
                                          DP_XMIT_LINK);
    }

    if ((wlan_vdev_chan_config_valid(vap->vdev_obj) != QDF_STATUS_SUCCESS) ||
                    (IEEE80211_IS_CHAN_RADAR(vap->iv_ic, vap->iv_bsschan))) {
        qdf_nbuf_free(skb);
        return 0;
    }

    if (qdf_unlikely(!qdf_nbuf_is_tso(skb) != 0)) {
        skb = qdf_nbuf_unshare(skb);
        if (skb == NULL)
            return 0;
    }

#if UMAC_SUPPORT_WNM
    if (wlan_wnm_tfs_filter(vap, (wbuf_t) skb)) {
        goto bad;
    }
#endif

#if DBDC_REPEATER_SUPPORT || QCA_OL_VLAN_WAR
        ic = vap->iv_ic;
        pscn = (struct ol_ath_softc_net80211 *)ic;
#endif

#if ATH_SUPPORT_WRAP
    if (wlan_rptr_vdev_is_mpsta(vap->vdev_obj) || wlan_rptr_vdev_is_psta(vap->vdev_obj)) {
#if DBDC_REPEATER_SUPPORT
        if ((lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCA8074) ||
            (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCA8074V2) ||
            (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCA9574) ||
            (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCA5332) ||
            (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCA5018) ||
            (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCN9000) ||
            (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCN9224) ||
            (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCN6122) ||
            (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCN9160) ||
            (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCA6018) ||
            (lmac_get_tgt_type(pscn->soc->psoc_obj) == TARGET_TYPE_QCN6432)) {
            if (qca_multi_link_sta_tx(dev, skb, osif_get_mld_netdev(osdev))) {
                return 0;
            }
        }
#endif
        if (osdev->nss_wifiol_ctx && vap->iv_ic->nss_vops) {
            vap->iv_ic->nss_vops->ic_osif_nss_vdev_process_mpsta_tx(dev, skb);
        }
        return 0;
    }
#endif
    if (OL_CFG_RAW_TX_LIKELINESS(vap->iv_tx_encap_type == osif_pkt_type_raw)) {
        /* In Raw Mode, the payload normally comes encrypted by an external
         * Access Controller and we won't have the keys. Besides, the format
         * isn't 802.3/Ethernet II.
         * Hence, VLAN WAR, Ext AP functionality, VoW debug and Multicast to
         * Unicast conversion aren't applicable, and are skipped.
         *
         * Additionally, TSO and nr_frags based scatter/gather are currently
         * not required and thus not supported with Raw Mode.
         *
         * Error conditions are handled internally by the below function.
         */
        OL_TX_LL_UMAC_RAW_PROCESS(osdev, &skb);
        goto out;
    }

#if (MESH_MODE_SUPPORT)
    if (vap->iv_mesh_vap_mode && (vap->mdbg & MESH_DEBUG_ENABLED)) {
        params.mhdr = vap->mhdr;
        params.mdbg = vap->mdbg;
        params.mhdr_len = vap->mhdr_len;
        if (add_mesh_meta_hdr(skb, &params)) {
            goto bad;
        }
        vap->mhdr = params.mhdr;
    }
#endif

    if (wlan_rptr_vdev_is_extap(vdev) &&
            (wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE)) {
#if DBDC_REPEATER_SUPPORT
        if ((lmac_get_tgt_type(pscn->soc->psoc_obj) != TARGET_TYPE_QCA8074) &&
            (lmac_get_tgt_type(pscn->soc->psoc_obj) != TARGET_TYPE_QCA8074V2) &&
            (lmac_get_tgt_type(pscn->soc->psoc_obj) != TARGET_TYPE_QCA9574) &&
            (lmac_get_tgt_type(pscn->soc->psoc_obj) != TARGET_TYPE_QCA5332) &&
            (lmac_get_tgt_type(pscn->soc->psoc_obj) != TARGET_TYPE_QCA5018) &&
            (lmac_get_tgt_type(pscn->soc->psoc_obj) != TARGET_TYPE_QCN9000) &&
            (lmac_get_tgt_type(pscn->soc->psoc_obj) != TARGET_TYPE_QCN9224) &&
            (lmac_get_tgt_type(pscn->soc->psoc_obj) != TARGET_TYPE_QCN6122) &&
            (lmac_get_tgt_type(pscn->soc->psoc_obj) != TARGET_TYPE_QCN9160) &&
            (lmac_get_tgt_type(pscn->soc->psoc_obj) != TARGET_TYPE_QCA6018) &&
            (lmac_get_tgt_type(pscn->soc->psoc_obj) != TARGET_TYPE_QCN6432)) {
            vap = osdev->os_if;
        } else {
            /*
             * QCA Repeater Processing.
             */
            if (qca_multi_link_sta_tx(dev, skb, osif_get_mld_netdev(osdev))) {
                return 0;
            }
        }
#endif

        if (osdev->nss_wifiol_ctx && vap->iv_ic->nss_vops)
            vap->iv_ic->nss_vops->ic_osif_nss_vdev_process_extap_tx(dev, skb);

        return 0;
    }

#if QCA_OL_VLAN_WAR
    if(OL_TX_VLAN_WAR(&skb, pscn))
        goto bad;
#endif /* QCA_OL_VLAN_WAR */

    if (osdev->nss_wifiol_ctx && vap->iv_ic->nss_vops && vap->iv_ic->nss_vops->ic_osif_nss_vap_xmit(osdev, skb)) {
        goto bad;
    }

out:
    return 0;

bad:
    if (skb != NULL) {
        qdf_nbuf_free(skb);
    }
    return 0;
}
qdf_export_symbol(osif_nss_ol_vap_hardstart);

#ifdef QCA_SUPPORT_WDS_EXTENDED
int osif_nss_ext_vdev_hardstart(struct sk_buff *skb, struct net_device *dev)
{
    struct ieee80211com *ic = NULL;
    osif_peer_dev *osifp = NULL;
    osif_dev  *osdev = NULL;
    wlan_if_t vap = NULL;

    if (skb)
        nbuf_debug_add_record(skb);

    osifp = ath_netdev_priv(dev);
    if (!osifp || !osifp->parent_netdev) {
        goto free;
    }

    if (osifp->wdev.iftype != NL80211_IFTYPE_AP_VLAN) {
        goto free;
    }

    osdev = ath_netdev_priv(osifp->parent_netdev);
    vap = osdev->os_if;
    ic = vap->iv_ic;

    /*
     * Transfer the buffer to NSS
     */
    if (osifp->nss_wifiol_ctx && ic->nss_ext_vops && ic->nss_ext_vops->ic_osif_nss_ext_vdev_xmit(osifp, skb)) {
        goto free;
    }

    return 0;
free:
    if (skb != NULL) {
        qdf_nbuf_free(skb);
    }

    return 0;
}
qdf_export_symbol(osif_nss_ext_vdev_hardstart);
#endif /* QCA_SUPPORT_WDS_EXTENDED */
#endif /* QCA_NSS_WIFI_OFFLOAD_SUPPORT */

void osif_get_peer_mac_from_peer_id(struct wlan_objmgr_pdev *pdev, uint32_t peer_id, uint8_t *peer_mac)
{
    struct wlan_objmgr_psoc *psoc = NULL;
    ol_txrx_soc_handle soc_txrx_handle = NULL;

    if (!pdev) {
        return;
    }
    psoc = wlan_pdev_get_psoc(pdev);
    if (!psoc) {
        return;
    }

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    if (peer_mac) {
        cdp_get_peer_mac_from_peer_id(soc_txrx_handle, peer_id, peer_mac);
    }
}

void osif_deliver_tx_capture_data(osif_dev *osifp, struct sk_buff *skb)
{
    skb->dev = osifp->netdev;
    skb->pkt_type = PACKET_USER;
    skb->ip_summed = CHECKSUM_UNNECESSARY;
    skb->protocol = eth_type_trans(skb, osifp->netdev);
    nbuf_debug_del_record(skb);
    netif_rx(skb);
}

/**
 * osif_deliver_lite_mon_data() - Deliver data to stack in lite mon
 *
 * @osifp: osif_dev *
 * @skb:  struct sk_buff *
 */
void osif_deliver_lite_mon_data(osif_dev *osifp, struct sk_buff *skb)
{
    skb->protocol = __constant_htons(ETH_P_80211_RAW);
    skb->dev = osifp->netdev;
    skb_reset_mac_header(skb);
    skb->pkt_type = PACKET_OTHERHOST;
    skb->ip_summed = CHECKSUM_NONE;
    nbuf_debug_del_record(skb);
    netif_rx(skb);
}
