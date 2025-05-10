/*
 * Copyright (c) 2011, 2017-2018 Qualcomm Innovation Center, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 *  Copyright (c) 2008 Atheros Communications Inc.  All rights reserved.
 * \brief 802.11n compliant Transmit Beamforming
 */

#include <ieee80211_var.h>
#include <ieee80211_channel.h>
#include "ol_if_athvar.h"

#ifdef ATH_SUPPORT_TxBF
extern void ieee80211_send_setup( struct ieee80211vap *vap, struct ieee80211_node *ni,
    struct ieee80211_frame *wh, u_int8_t type, const u_int8_t *sa, const u_int8_t *da,
                           const u_int8_t *bssid);

enum {
    NOT_SUPPORTED         = 0,
    DELAYED_FEEDBACK      = 1,
    IMMEDIATE_FEEDBACK    = 2,
    DELAYED_AND_IMMEDIATE = 3,
};
void 
ieee80211_set_TxBF_keycache(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    //struct ieee80211vap *vap = ni->ni_vap;
	 
    //IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"==>%s:keyid: %d, cipher %x,flags %x",__func__,ni->ni_ucastkey.wk_keyix
    //        ,ni->ni_ucastkey.wk_cipher->ic_cipher,ni->ni_ucastkey.wk_flags);

    if (ni->ni_ucastkey.wk_keyix == IEEE80211_KEYIX_NONE){  //allocate new key
        ni->ni_ucastkey.wk_keyix=ic->ic_txbf_alloc_key(ic, ni);
    }
    if (( ni->ni_ucastkey.wk_keyix != IEEE80211_KEYIX_NONE)&& 
        (ni->ni_ucastkey.wk_cipher->ic_cipher == IEEE80211_CIPHER_NONE)){ 
        ic->ic_txbf_set_key(ic, ni);   // update staggered sounding ,cec , mmss into key cache.
    }
}

void
ieee80211_del_TxBF_keycache(struct ieee80211vap *vap, struct ieee80211_key *key, struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;

    if (( ni->ni_ucastkey.wk_keyix != IEEE80211_KEYIX_NONE)&&
            (ni->ni_ucastkey.wk_cipher->ic_cipher == IEEE80211_CIPHER_NONE)) {
        ic->ic_txbf_del_key(ic,ni);
    }
}

void
ieee80211_match_txbfcapability(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    union ieee80211_hc_txbf *bfer, *bfee;
    bool previous_txbf_active = 0;

    bfer = &ic->ic_txbf;
    bfee = &ni->ni_txbf;

    /* set previous link txbf status*/
    if (ni->ni_explicit_compbf || ni->ni_explicit_noncompbf || ni->ni_implicit_bf){
        previous_txbf_active = 1;
    }
    ni->ni_explicit_compbf = FALSE;
    ni->ni_explicit_noncompbf = FALSE;
    ni->ni_implicit_bf = FALSE;

    /* XXX need multiple chains to do BF?*/
    /* Skipping the TxBF, if single chain is enabled 
     * (chain 0 or chain 1 or chain 2, and so on)
     */ 
    if ((ic->ic_tx_chainmask & (ic->ic_tx_chainmask - 1)) == 0)
        return;
    /*
     * We would prefer to do explicit BF if possible.
     * So the following checks are order sensitive.
     */
     /* suppot both delay report and immediately report*/
     
    if (bfer->explicit_comp_steering && (bfee->explicit_comp_bf != 0)) {
        ni->ni_explicit_compbf = TRUE;
        
        /* re-initial TxBF timer for previous link is not TxBF link*/
        if (previous_txbf_active == 0){
            ieee80211_init_txbf(ic, ni);
        }        
        return;
    }

    if (bfer->explicit_noncomp_steering && (bfee->explicit_noncomp_bf != 0)){
        ni->ni_explicit_noncompbf = TRUE;
        if (previous_txbf_active == 0){
            ieee80211_init_txbf(ic, ni);
        }  
        return;
    }

    if (bfer->implicit_txbf_capable && bfee->implicit_rx_capable) {
        ni->ni_implicit_bf = TRUE;
        if (previous_txbf_active == 0){
            ieee80211_init_txbf(ic, ni);
        }  
        return;
    }
}

u_int8_t *
ieee80211_add_htc(wbuf_t wbuf, int use4addr)
{
    struct ieee80211_frame *wh;
    u_int8_t *htc;

    wh = (struct ieee80211_frame *)wbuf_header(wbuf);

    /* set order bit in frame control to enable HTC field; */
    wh->i_fc[1] |= IEEE80211_FC1_ORDER;	

    if (!use4addr) {
        htc = ((struct ieee80211_qosframe_htc *)wh)->i_htc;
    } else {              					
        htc= ((struct ieee80211_qosframe_htc_addr4 *)wh)->i_htc;
    }

    htc[0] = htc[1] = htc[2] = htc[3] = 0;

    return htc;
}

void
ieee80211_request_cv_update(struct ieee80211com *ic,struct ieee80211_node *ni, wbuf_t wbuf, int use4addr)
{
    u_int8_t *htc;
    /* struct ieee80211vap *vap = ni->ni_vap;
	        
     *IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"==>%s:\n",__func__);*/

    htc = ieee80211_add_htc(wbuf, use4addr);

    /* clear update flags */
    ni->ni_bf_update_cv = 0;
    if (ni->ni_explicit_compbf) {
        /* set CSI=3 to request compressed ExBF */
        htc[2] = IEEE80211_HTC2_CSI_COMP_BF;
    } else if (ni->ni_explicit_noncompbf) {
        /* set CSI=2 to request non-compressed ExBF */
        htc[2] = IEEE80211_HTC2_CSI_NONCOMP_BF;
    }
    return;
}

//for TxBF RC
/*
 * Send a Crlx frame of Beamforming to the specified node.
	Cal_type : 0	 ==> Cal 1
			   or	 ==> Cal 3

 */

OS_TIMER_FUNC(txbf_cv_timeout)
{
    struct ieee80211_node *ni;
    OS_GET_TIMER_ARG(ni, struct ieee80211_node *);

    ni->ni_allow_cv_update = 1; // time up, allow cv update.
}

OS_TIMER_FUNC(txbf_cv_report_timeout)
{
    struct ieee80211_node *ni;

    OS_GET_TIMER_ARG(ni, struct ieee80211_node *);

    ni->ni_bf_update_cv = 1; // time up, request cv update immediately.

    if (ni->ni_sw_cv_timeout){ 
        // reset sw timer
        OS_CANCEL_TIMER(&(ni->ni_cv_timer));
        OS_SET_TIMER(&ni->ni_cv_timer, ni->ni_sw_cv_timeout);
    }
}

void ieee80211_init_txbf(struct ieee80211com *ic,struct ieee80211_node *ni)
{
    if (ni->ni_explicit_compbf || ni->ni_explicit_noncompbf || ni->ni_implicit_bf) {
        ni->ni_bf_update_cv = 0;
        ni->ni_allow_cv_update = 0;
        ic->ic_init_sw_cv_timeout(ic, ni);
        //IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"==>%s: this node support TxBF, set initial BF ",__func__);
        if (ni->ni_txbf_timer_initialized){
            OS_FREE_TIMER(&ni->ni_cv_timer);
            OS_FREE_TIMER(&ni->ni_report_timer);
        }        
        OS_INIT_TIMER(ic->ic_osdev, &ni->ni_cv_timer, txbf_cv_timeout, ni, QDF_TIMER_TYPE_WAKE_APPS);
        OS_INIT_TIMER(ic->ic_osdev, &ni->ni_report_timer, txbf_cv_report_timeout, ni, QDF_TIMER_TYPE_WAKE_APPS);
        ni->ni_txbf_timer_initialized = 1;
    }
} 

void
ieee80211_tx_bf_completion_handler(struct ieee80211_node *ni,  struct ieee80211_tx_status *ts)
{

    /* should check status validation, not zero and TxBF mode first to avoid EV#82661*/
    if (ni->ni_explicit_compbf || ni->ni_explicit_noncompbf || ni->ni_implicit_bf ) {   // at Bfer state
        if (ts->ts_txbfstatus & TxBF_Valid_SW_Status) {    // valid sw status.
            if (ts->ts_txbfstatus & TxBF_STATUS_Sounding_Complete){
                ni->ni_hw_cv_requested = 0; /* clear flag after sounding sent*/
                if ( ts->ts_tstamp > ni->ni_cvtstamp){
                    /* report not received yet, expected to receive in TXBF_CV_REPORT_TIMEOUT time*/
                    if (ni->ni_cvretry < TXBF_CV_RETRY_LIMIT ){
                        OS_CANCEL_TIMER(&ni->ni_report_timer);
                        OS_SET_TIMER(&ni->ni_report_timer, TXBF_CV_REPORT_TIMEOUT);
                        ni->ni_cvretry ++;
                    }
                }
            } else if (ts->ts_txbfstatus & TxBF_STATUS_Sounding_Request){
                if (ni->ni_sw_cv_timeout){ // reset sw timer
                    OS_CANCEL_TIMER(&(ni->ni_cv_timer));
                    OS_SET_TIMER(&ni->ni_cv_timer, ni->ni_sw_cv_timeout);
                }
                ni->ni_bf_update_cv = 1;    // request to update CV cache
            }
        } else {
            if (((ts->ts_flags &  IEEE80211_TX_ERROR) == 0) &&
                    ((ts->ts_flags & IEEE80211_TX_XRETRY) == 0)) // tx frame ok
            {
                if ((ts->ts_txbfstatus & AR_TxBF_Valid_HW_Status)!= AR_Expired){
                    if (ni->ni_hw_cv_requested == 0){
                        ni->ni_bf_update_cv = 1;    // request to update CV cache for HW status is not AR_Expired.
                        ni->ni_hw_cv_requested = 1; /* set flag to avoid send sounding repeatedly*/
                    }
                }
                else {
                    // if sw cv timeout setting is not equal to zero ,use S/W timer to trigger CV update
                    // otherwise use H/W timer directly.
                    if (ni->ni_sw_cv_timeout) {
                        if (ni->ni_allow_cv_update) {
                            ni->ni_allow_cv_update = 0;
                            ni->ni_bf_update_cv = 1;    // request to update CV cache
                            OS_SET_TIMER(&ni->ni_cv_timer, ni->ni_sw_cv_timeout);
                            ni->ni_cvretry = 0;
                            //IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"==>%s: request CV update and trigger S/W timer!\n",__func__);
                        }
                    } else {
                        ni->ni_bf_update_cv = 1;    // request to update CV cache
                        //IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"==>%s: txbf status %x,request cv_update!\n",__func__,ts->ts_txbfstatus);
                    }
                }
            }
        }
    }

}
#endif /* ATH_SUPPORT_TxBF */

#if QCA_STANDALONE_SOUNDING_TRIGGER
#define CBF_STREAMFS_MAX_SUBBUF  1250
#define CBF_STREAMFS_NUM_SUBBUF  100

QDF_STATUS txbf_cbf_streamfs_init(struct wlan_objmgr_pdev *pdev)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic;
    struct txbf_cbf_ctx *cbf_ctx;
    char *devname;
    char folder[32];

    if (!pdev) {
        qdf_err("PDEV is NULL\n");
        return QDF_STATUS_E_FAILURE;
    }
    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return QDF_STATUS_E_FAILURE;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return QDF_STATUS_E_FAILURE;
    }

    cbf_ctx  = &scn->cbf_ctx;
    devname = ic->ic_netdev->name;
    if (!devname) {
        qdf_err("devname is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    snprintf(folder, sizeof(folder), "cbf_%s", devname);
    cbf_ctx->dir_ptr = qdf_streamfs_create_dir((const char *)folder, NULL);
    if (!cbf_ctx->dir_ptr) {
        qdf_err("Directory create failed");
        return QDF_STATUS_E_FAILURE;
    }

    cbf_ctx->subbuf_size = CBF_STREAMFS_MAX_SUBBUF;
    cbf_ctx->num_subbufs = CBF_STREAMFS_NUM_SUBBUF;
    cbf_ctx->chan_ptr = qdf_streamfs_open("cbf_dump", cbf_ctx->dir_ptr,
            cbf_ctx->subbuf_size, cbf_ctx->num_subbufs, NULL);
    if (!cbf_ctx->chan_ptr) {
        qdf_err("Chan create failed");
        qdf_streamfs_remove_dir_recursive(cbf_ctx->dir_ptr);
        cbf_ctx->dir_ptr = NULL;
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS txbf_cbf_streamfs_deinit(struct wlan_objmgr_pdev *pdev)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic = NULL;
    struct txbf_cbf_ctx *cbf_ctx = NULL;

    if (!pdev) {
        qdf_err("PDEV is NULL\n");
        return QDF_STATUS_E_FAILURE;
    }
    ic =  wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return QDF_STATUS_E_FAILURE;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return QDF_STATUS_E_FAILURE;
    }

    cbf_ctx  = &scn->cbf_ctx;
    if (cbf_ctx) {
        if (cbf_ctx->chan_ptr) {
            qdf_streamfs_close(cbf_ctx->chan_ptr);
            cbf_ctx->chan_ptr = NULL;
        }

        if (cbf_ctx->dir_ptr) {
            qdf_streamfs_remove_dir_recursive(cbf_ctx->dir_ptr);
            cbf_ctx->dir_ptr = NULL;
        }

    } else {
        qdf_err("cbf_ctx is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS
cbf_streamfs_write(struct txbf_cbf_ctx *cbf_ctx, const void *write_data,
                   size_t write_len)
{
    if (cbf_ctx->chan_ptr) {
    /* write to channel buffer */
        qdf_streamfs_write(cbf_ctx->chan_ptr, write_data, write_len);
    } else {
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS cbf_streamfs_flush(struct txbf_cbf_ctx *cbf_ctx)
{
    if (cbf_ctx->chan_ptr) {

    /* Flush the data write to channel buffer */
        qdf_streamfs_flush(cbf_ctx->chan_ptr);
    } else
        return QDF_STATUS_E_FAILURE;

 return QDF_STATUS_SUCCESS;
}

QDF_STATUS txbf_cbf_send_cv_status(struct ieee80211com *ic,
                                   struct txbf_cv_status *cv_status)
{
    struct ol_ath_softc_net80211 *scn;
    struct txbf_cbf_ctx *cbf_ctx = NULL;
    QDF_STATUS status;

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    /* send error status to relayfs */
    cbf_ctx  = &scn->cbf_ctx;
    if (cbf_ctx) {
        cv_status->start_magic_num  =  0xDEADBEAF;

        status = cbf_streamfs_write(cbf_ctx, cv_status, sizeof(struct txbf_cv_status));
        if (status != QDF_STATUS_SUCCESS) {
            qdf_err("cbf cv status relay write failed");
            return QDF_STATUS_E_FAILURE;
        }
        status = cbf_streamfs_flush(cbf_ctx);
        if (status != QDF_STATUS_SUCCESS) {
            qdf_err("cbf cv status relay flush failed");
            return QDF_STATUS_E_FAILURE;
        }
    }  else {
        qdf_err("cbf_ctx is NULL");
        return QDF_STATUS_E_FAILURE;
    }

return QDF_STATUS_SUCCESS;
}

QDF_STATUS
txbf_cbf_send_cv_data(struct ieee80211com *ic, struct txbf_cv_report *report)
{
    struct ol_ath_softc_net80211 *scn;
    struct txbf_cv_hdr *hdr;
    struct txbf_cbf_ctx *cbf_ctx = NULL;
    QDF_STATUS status;

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    cbf_ctx  = &scn->cbf_ctx;
    if (cbf_ctx) {
        hdr = &report->cv_hdr;
        if (hdr) {
            qdf_trace_hex_dump(QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_DEBUG,
                    (unsigned char *)hdr, sizeof(struct txbf_cv_hdr));
            status = cbf_streamfs_write(cbf_ctx, hdr, sizeof(struct txbf_cv_hdr));
            if (status != QDF_STATUS_SUCCESS) {
                qdf_err("cbf cv hdr relay write failed");
                return QDF_STATUS_E_FAILURE;
            }
            if (hdr->asnr_len) {
                status = cbf_streamfs_write(cbf_ctx, report->asnr_data, hdr->asnr_len);
                if (status != QDF_STATUS_SUCCESS) {
                    qdf_err("cbf cv asnr data relay write failed");
                    return QDF_STATUS_E_FAILURE;
                }
            }
            if (hdr->dsnr_len) {
                status = cbf_streamfs_write(cbf_ctx, report->dsnr_data, hdr->dsnr_len);
                if (status != QDF_STATUS_SUCCESS) {
                    qdf_err("cbf cv dsnr data relay write failed");
                    return QDF_STATUS_E_FAILURE;
                }
            }
            status = cbf_streamfs_flush(cbf_ctx);
            if (status != QDF_STATUS_SUCCESS) {
                qdf_err("cbf cv upload relay flush failed");
                return QDF_STATUS_E_FAILURE;
            }
        } else {
            qdf_err("cv_hdr is NULL");
            return QDF_STATUS_E_FAILURE;
        }
    }  else {
        qdf_err("cbf_ctx is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}
#endif /* QCA_STANDALONE_SOUNDING_TRIGGER */
