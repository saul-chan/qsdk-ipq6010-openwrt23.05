/*
 * Copyright (c) 2014,2017-2018, 2020-2023 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2014 Qualcomm Atheros, Inc.  All rights reserved.
 * Qualcomm is a trademark of Qualcomm Technologies Incorporated, registered in the United
 * States and other countries.  All Qualcomm Technologies Incorporated trademarks are used with
 * permission.  Atheros is a trademark of Qualcomm Atheros, Inc., registered in
 * the United States and other countries.  Other products and brand names may be
 * trademarks or registered trademarks of their respective owners.
 */

/*
 *  WNM message handlers for AP.
 */
#include <ieee80211_var.h>
#include <ieee80211_wnm.h>
#include <ieee80211_mbo.h>
#include <ieee80211_ioctl.h>
#include <ieee80211_regdmn.h>
#include "ieee80211_rrm_proto.h"
#include "ieee80211_wnm_proto.h"
#include "ieee80211_wnm_priv.h"
#if WLAN_SUPPORT_MSCS
#include <ieee80211_ucfg.h>
#endif

#if WLAN_SUPPORT_MSCS
#include <linux/wireless.h>
#endif
#include <osif_private.h>
#include <wlan_mlme_dispatcher.h>
#include <cdp_txrx_ctrl.h>
#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_peer.h>
#if defined(WLAN_SUPPORT_SCS) && defined(SAWF_ADMISSION_CONTROL)
#include <wlan_mlo_mgr_setup.h>
#endif
#endif
#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_ap.h>
#include <ieee80211_objmgr_priv.h>
#endif
#if defined(WLAN_SUPPORT_SCS) && defined(SAWF_ADMISSION_CONTROL)
#include <ieee80211_cfg80211.h>
#endif

#if UMAC_SUPPORT_WNM
int __ieee80211_tfs_filter(struct ieee80211_node *ni, wbuf_t wbuf,
                           int ether_type, struct ip_header *ip);
/*
 * Filling up neighbor report IEs with
 * user given values instead
 * of scan table values
 */

u_int8_t *fill_nrinfo_from_user(wlan_if_t vap, u_int8_t *frm, wlan_node_t ni,
                                struct ieee80211_bstm_reqinfo *bstm_reqinfo)
{
    ieee80211_wnm_t  *wnm = &vap->wnm;
    struct ieee80211_nrresp_info info;
    int i =0 ,attr_index = 0 ,chan_index = 0,flag = false;
    struct ieee80211_mbo_attributes *ni_mbo = &ni->ni_mbo;
#ifdef WLAN_FEATURE_11BE_MLO
    int link_index = 0;
    int bssid_index = 0;
    int found_other_candidate = 0;

    (*wnm)->include_self_bss = 1;
    (*wnm)->self_bss_with_link = 0;
#endif

    for ( i = 0; i < (*wnm)->count; i++)
        {
            flag = false;
            OS_MEMSET(&info, 0,sizeof(struct ieee80211_nrresp_info));
            info.phytype = 0;
            info.regclass = 0;
            info.regclass =  (*wnm)->store_userbssidpref_in_ap[i].regclass;

            /* the only capinfo field used in the neighbor report is spectrum management
               assume this is enabled for now (although questionable if this is used by STAs) */

            info.capinfo = IEEE80211_CAPINFO_SPECTRUM_MGMT;
            /* copy over request fields */
            IEEE80211_ADDR_COPY(&info.bssid, (*wnm)->store_userbssidpref_in_ap[i].bssid);
            info.channum = (*wnm)->store_userbssidpref_in_ap[i].chan;

            if(ni_mbo && ni_mbo->num_attr) {
                for(attr_index = 0;attr_index< ni_mbo->num_attr;attr_index++) {
                    /* Don't include candidate list whose preference value is 0(i.e., Excluded BSS) */
                    if (ni_mbo->channel[attr_index].channels_preference == 0) {
                        for(chan_index = 0;chan_index < IEEE80211_MBO_NUM_NONPREF_CHANLIST;chan_index++) {
                            if(info.channum == ni_mbo->channel[attr_index].channels[chan_index]) {
                                info.preference = 0;
                                flag = true;
                                break;
                            }
                        }
                    }
                }
            }
            /* Check against supported op class information of STA, if exist */
            if (!flag && (!ni->ni_supp_op_cl.num_of_supp_class || IEEE80211_MBO_CHAN_BITMAP_IS_SET(ni->ni_supp_op_cl.channels_supported, info.channum)))
                 info.preference = (*wnm)->store_userbssidpref_in_ap[i].pref_val;

#ifdef WLAN_FEATURE_11BE_MLO
            if ((*wnm)->store_userbssidpref_in_ap[i].mld) {
                IEEE80211_ADDR_COPY(info.mld_addr, (*wnm)->store_userbssidpref_in_ap[i].mld_addr);
                info.mld = (*wnm)->store_userbssidpref_in_ap[i].mld;
                qdf_mem_set(info.link_info, sizeof(info.link_info), ML_PROBE_LINKID_INVALID);
                link_index= 0;

                if ((*wnm)->store_userbssidpref_in_ap[i].link_id != ML_PROBE_LINKID_INVALID) {
                    info.link_info_present = 1;
                    info.link_info[link_index] = (*wnm)->store_userbssidpref_in_ap[i].link_id;
                    link_index++;
                }
                if (info.link_info_present) {
                    flag = false;
                    for (bssid_index = 0; bssid_index < (*wnm)->count; bssid_index++) {
                        if (IEEE80211_ADDR_EQ((*wnm)->store_userbssidpref_in_ap[bssid_index].mld_addr, info.mld_addr) &&
                            !(IEEE80211_ADDR_EQ((*wnm)->store_userbssidpref_in_ap[bssid_index].bssid, info.bssid))) {
                            if (ni_mbo && ni_mbo->num_attr) {
                                for (attr_index = 0;attr_index< ni_mbo->num_attr;attr_index++) {
                                    /* Don't include linkid for non-preferred channel */
                                    if (ni_mbo->channel[attr_index].channels_preference == 0) {
                                        for (chan_index = 0;chan_index < IEEE80211_MBO_NUM_NONPREF_CHANLIST;chan_index++) {
                                            if ((*wnm)->store_userbssidpref_in_ap[bssid_index].chan == ni_mbo->channel[attr_index].channels[chan_index]) {
                                                flag = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            if ((*wnm)->store_userbssidpref_in_ap[bssid_index].pref_val != info.preference) {
                                flag = true;
                            }
                            if (!flag  && (link_index < IEEE80211_WNM_WLAN_MAX_MLO_VDEVS) &&
                                ((*wnm)->store_userbssidpref_in_ap[bssid_index].link_id != ML_PROBE_LINKID_INVALID)) {
                                info.link_info[link_index] = (*wnm)->store_userbssidpref_in_ap[bssid_index].link_id;
                                link_index++;
                            }
                        }
                        /* Set flag if any other candidate found */
                        if (!IEEE80211_ADDR_EQ((*wnm)->store_userbssidpref_in_ap[bssid_index].mld_addr, info.mld_addr)) {
                            found_other_candidate = 1;
                        }
                    }
                    /* Set pref to 0 for self MLD if any other candidate being added */
                    if (found_other_candidate && bstm_reqinfo->disassoc && wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj) &&
                        IEEE80211_ADDR_EQ(wlan_vdev_mlme_get_mldaddr(vap->vdev_obj),
                                          (*wnm)->store_userbssidpref_in_ap[i].mld_addr))
                        info.preference = 0;

                    if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj) &&
                        IEEE80211_ADDR_EQ(wlan_vdev_mlme_get_mldaddr(vap->vdev_obj),
                                          (*wnm)->store_userbssidpref_in_ap[i].mld_addr))
                        (*wnm)->self_bss_with_link = 1;

                } else {
                    /* Self MLD is being added without link recommdation, no need to include self BSS as candidate*/
                    if (IEEE80211_ADDR_EQ(wlan_vdev_mlme_get_mldaddr(vap->vdev_obj), info.mld_addr)) {
                        (*wnm)->include_self_bss = 0;
                        if (bstm_reqinfo->disassoc || bstm_reqinfo->bssterm_inc)
                            info.preference = 0;
                    }
                }
            }
#endif
#if UMAC_SUPPORT_RRM
            frm = ieee80211_add_nr_ie(frm, &info);
#endif
        }
    return frm;
}

/**
 * @brief Generic code to create a BSTM request, common
 * regardless of whether candidate list is specified by user
 * space, or generated by scan results
 *
 * @param vap [in] VAP to send request on
 * @param ni [in] Node to send request to
 * @param wbuf [in,out] wbuf to contain the BTM Request
 * @param frm [in,out] Pointer to the end of what has been
 *            filled in so far in the message (so extra fields
 *            start at this point)
 * @param dialogtoken [in] Dialog Token to use for the request
 *                    frame
 * @param candidate_list [in] Set to true if a candidate list
 *                       will be included
 * @param disassoc [in] Set to true if setting the
 *                 Disassociation Imminent flag
 * @param term_inc [in] Set to true if setting the BSS
 *                 Termination Included flag
 * @param abridged [in] set to true if setting the abridged
 *                      flag
 * @param disassoc_timer [in] Disassociation Timer to be
 *                       included in the request
 * @param validity_interval [in] Validity Interval to be
 *                          included in the request
 * @param bcast [in] set to true for broadcast bstm req
 *
 * @return EOK on success, negative error code on failure
 */
static int ieee80211_create_bstm_req(wlan_if_t vap, wlan_node_t ni, wbuf_t *wbuf, u_int8_t **frm,
                                     uint8_t dialogtoken,
                                     bool candidate_list,
                                     bool disassoc,
                                     bool term_inc,
                                     u_int64_t term_tsf,
                                     u_int16_t term_duration,
                                     bool abridged,
                                     bool link_remove,
                                     uint16_t disassoc_timer,
                                     uint8_t validity_interval,
                                     bool bcast)
{
    struct ieee80211_action_bstm_req *bstm_req;
    struct ieee80211_bssterm_duration *btdur;

    *wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, frm, bcast);
    if (*wbuf == NULL) {
        return -ENOMEM;
    }

    bstm_req = (struct ieee80211_action_bstm_req *)*frm;
    bstm_req->header.ia_category = IEEE80211_ACTION_CAT_WNM;
    bstm_req->header.ia_action = IEEE80211_ACTION_BSTM_REQ;
    bstm_req->dialogtoken = dialogtoken;
    bstm_req->mode_reserved = 0;
    bstm_req->mode_ess_disassoc = 0;
    bstm_req->mode_term_inc = term_inc;
    bstm_req->mode_abridged = abridged;
    bstm_req->mode_candidate_list = candidate_list;
    bstm_req->mode_disassoc = disassoc;
    bstm_req->disassoc_timer = htole16(disassoc_timer);

    bstm_req->mode_link_remove = link_remove;

    bstm_req->validity_itvl = validity_interval;

    *frm = &bstm_req->opt_ies[0];

    /* bss termination indication */
    if (bstm_req->mode_term_inc) {

        /* build BSS Termination Duration subelement */
        btdur = (struct ieee80211_bssterm_duration *)*frm;
        btdur->sub_id = IEEE80211_BSSTERM_DURATION_SUBELEMENT_ID;
        btdur->len = sizeof(struct ieee80211_bssterm_duration) - 2; /* exclude id and length */
        btdur->tsf = htole64(term_tsf);
        btdur->duration = htole16(term_duration);
        *frm += sizeof(*btdur);
    }

    return EOK;
}


/**
 * Generic code to finalize and send a BSTM request, common
 * regardless of whether candidate list is specified by user
 * space, or generated by scan results
 *
 * @param vap [in] VAP to send request on
 * @param ni [in] Node to send request to
 * @param wbuf [in] wbuf to contain the BTM Request
 * @param frm [in] Pointer to the end of what has been filled in
 *            so far in the message
 *
 */
static int ieee80211_finalize_and_send_bstm_req(wlan_if_t vap, wlan_node_t ni,
                                                wbuf_t wbuf, u_int8_t *frm)
{
    int len;

    /* Check if PMF is enabled and set security bit */
    if (ieee80211_vap_mfp_test_is_set(vap) ||
        (wlan_crypto_is_pmf_enabled(vap->vdev_obj, ni->peer_obj) &&
         ieee80211_node_is_authorized(ni))) {
        struct ieee80211_frame *wh = (struct ieee80211_frame*)wbuf_header(wbuf);
        wh->i_fc[1] |= IEEE80211_FC1_WEP;
    }

    len = frm - (u_int8_t*)wbuf_header(wbuf);
    wbuf_set_pktlen(wbuf, len);

    if (vap->wnm)
        vap->wnm->send_bstm_req++;

    return ieee80211_send_mgmt(vap, ni, wbuf, false);
}

static uint8_t get_regClassId(struct ieee80211vap *vap) {
    uint16_t chwidth, behav_lim;
    bool global_lookup = true;
    uint8_t op_class, prim_chan;
    struct ieee80211com *reported_ic = vap->iv_ic;

    wlan_get_bw_and_behav_limit(reported_ic->ic_curchan,
                        &chwidth, &behav_lim);
    wlan_reg_freq_width_to_chan_op_class_auto(reported_ic->ic_pdev_obj,
                        reported_ic->ic_curchan->ic_freq, chwidth,
                        global_lookup, behav_lim,
                        &op_class, &prim_chan);
    return op_class;
}

#ifdef WLAN_FEATURE_11BE_MLO
static u_int8_t *fill_partner_link_info(struct wlan_objmgr_vdev *vdev, u_int8_t *frm, u_int8_t preference) {
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    uint16_t num_links = 0;
    struct ieee80211_nrresp_info info;
    struct ieee80211vap *vap;
    u_int8_t bssid[QDF_MAC_ADDR_SIZE];
    struct qdf_mac_addr *mldaddr;
    u_int16_t j;

    mlo_ap_get_active_vdev_list(vdev, &num_links, vdev_list);

    if (!num_links) {
        mlo_err("Number of VDEVs under AP-MLD is reported as 0");
        return frm;
    }

    if (num_links > QDF_ARRAY_SIZE(vdev_list)) {
        mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu, investigate",num_links, QDF_ARRAY_SIZE(vdev_list));
        for (j = 0; j < QDF_ARRAY_SIZE(vdev_list); j++)
             mlo_release_vdev_ref(vdev_list[j]);
        return frm;
    }

    for (j = 0; j < num_links; j++) {
         if (vdev_list[j]){
             if (vdev != vdev_list[j]) {
                 OS_MEMSET(&info, 0, sizeof(struct ieee80211_nrresp_info));

                 vap = wlan_vdev_get_mlme_ext_obj(vdev_list[j]);
                 if (vap) {
                     wlan_vap_get_bssid(vap, bssid);
                     IEEE80211_ADDR_COPY(info.bssid, bssid);
                     mldaddr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev_list[j]);
                     IEEE80211_ADDR_COPY(info.mld_addr, mldaddr->bytes);
                     info.mld = 1;
                     info.channum = vap->iv_bsschan->ic_ieee;
                     info.regclass = get_regClassId(vap);
                     info.phytype = ieee80211_rrm_get_phy_type(vap->iv_cur_mode);
                     info.preference = preference;

                     frm = ieee80211_add_nr_ie(frm, &info);
                 }
             }
             mlo_release_vdev_ref(vdev_list[j]);
         }
    }
    return frm;
}

static u_int8_t validate_link_subsets(struct wlan_objmgr_vdev *vdev,
                                      struct recommended_link_info *linkinfo){
    u_int16_t i,j,k;
    struct ieee80211vap *vap;

    vap = wlan_vdev_get_mlme_ext_obj(vdev);

    if (vap) {
        for (i = 0; i < MAX_LINK_SUBSET; i++){
             for(j = 0; j < IEEE80211_WNM_WLAN_MAX_MLO_VDEVS; j++){
                 if (linkinfo[i].linkid[j] == ML_PROBE_LINKID_INVALID)
                     return 1;
                 for (k = 0; k < vap->mlo_info.num_mlo_links; k++){
                     if (vap->mlo_info.mlo_link_ids[k] == linkinfo[i].linkid[j])
                         break;
                 }
                 if (k == vap->mlo_info.num_mlo_links){
                     qdf_err("Invalid link id: %d",linkinfo[i].linkid[j]);
                     return 0;
                 }
             }
        }
    }
    return 1;
}

static u_int8_t *fill_recommended_link_info(struct wlan_objmgr_vdev *vdev,
                                            struct recommended_link_info *linkinfo,
                                            u_int8_t *frm) {
    struct wlan_objmgr_vdev *partner_vdev = NULL;
    struct ieee80211_nrresp_info info;
    struct ieee80211vap *vap;
    u_int8_t bssid[QDF_MAC_ADDR_SIZE];
    struct qdf_mac_addr *mldaddr;
    u_int16_t i;

    for (i = 0; i < MAX_LINK_SUBSET; i++) {
         if (linkinfo->linkid[0] == ML_PROBE_LINKID_INVALID) {
             return frm;
         }
         partner_vdev = mlo_get_vdev_by_link_id(vdev, linkinfo->linkid[0],
                                                WLAN_MLO_MGR_ID);
         if (!partner_vdev) {
             qdf_err("No MLO-link vdev found for linkid: %d", linkinfo->linkid[0]);
         }
         else{
              OS_MEMSET(&info, 0, sizeof(struct ieee80211_nrresp_info));

              vap = wlan_vdev_get_mlme_ext_obj(partner_vdev);
              if (vap) {
                  wlan_vap_get_bssid(vap, bssid);
                  IEEE80211_ADDR_COPY(info.bssid, bssid);
                  mldaddr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(partner_vdev);
                  IEEE80211_ADDR_COPY(info.mld_addr, mldaddr->bytes);
                  info.mld = 1;
                  info.channum = vap->iv_bsschan->ic_ieee;
                  info.regclass = get_regClassId(vap);
                  info.phytype = ieee80211_rrm_get_phy_type(vap->iv_cur_mode);
                  info.link_info_present = 1;
                  qdf_mem_copy(info.link_info, &linkinfo->linkid[0], sizeof(info.link_info));
                  info.preference = linkinfo->link_preference;
                  frm = ieee80211_add_nr_ie(frm, &info);
              }
              mlo_release_vdev_ref(partner_vdev);
              linkinfo++;
         }
    }
    return frm;
}
#endif

/*
 * Format and send Bss transition management request action frame
 */
int ieee80211_send_bstm_req(wlan_if_t vap, wlan_node_t ni,
                            struct ieee80211_bstm_reqinfo *bstm_reqinfo,
                            bool bcast)
{
    wbuf_t wbuf;
    u_int8_t *frm;
    int ret;
    struct ieee80211_nrresp_info info;
    u_int8_t bssid[QDF_MAC_ADDR_SIZE];
    char country_iso[4];
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_pdev *pdev = NULL;
    uint8_t prim_chan;
    uint16_t chwidth;
    uint16_t behav_lim;
    int i;
    bool link_remove_val;
#ifdef WLAN_FEATURE_11BE_MLO
    uint64_t qtimer_delta;
    uint64_t qtimer_snapshot_host;
    uint64_t usr_bssterm_tsf = 0;
#endif /* WLAN_FEATURE_11BE_MLO */

    vdev = vap->vdev_obj;
    if (!vdev) {
        return -EINVAL;
    }

    pdev = wlan_vdev_get_pdev(vdev);
    if (!pdev) {
        return -EINVAL;
    }

#ifdef WLAN_FEATURE_11BE_MLO
    if (!wlan_vdev_mlme_is_mlo_ap(vdev) && (bstm_reqinfo->link_remove ||
                                            bstm_reqinfo->partner_link)) {
        qdf_err("AP is not MLO capable, link remove %d", bstm_reqinfo->link_remove);
        return -EINVAL;
    }

    link_remove_val = bstm_reqinfo->link_remove;
#else
    link_remove_val = 0;
#endif

#ifdef WLAN_FEATURE_11BE_MLO
    /* Calculate termination tsf with user provided tsf and current Host tsf */
    if (bstm_reqinfo->bssterm_tsf) {
        if (vap->iv_tsf_offset_calc_params.fw_snapshots_valid) {
            usr_bssterm_tsf = TIME_SEC_TO_USEC(bstm_reqinfo->bssterm_tsf);
            qtimer_snapshot_host =
                    vap->iv_ic->ic_get_q6_time((OL_ATH_SOFTC_NET80211(vap->iv_ic))->soc->sc_osdev->device);
            qtimer_delta = qtimer_snapshot_host - vap->iv_tsf_offset_calc_params.qtimer_snapshot_fw;
            bstm_reqinfo->bssterm_tsf = vap->iv_tsf_offset_calc_params.tsf_snapshot_fw + qtimer_delta +
                                                usr_bssterm_tsf;
        } else {
            qdf_info("Invalid FW snapshots of qtimer and TSF, Use user provided TSF");
        }
    }
#endif /* WLAN_FEATURE_11BE_MLO */

    /* do the general bstm request fill in */
    ret = ieee80211_create_bstm_req(vap, ni, &wbuf, &frm, bstm_reqinfo->dialogtoken,
                                    bstm_reqinfo->candidate_list,
                                    bstm_reqinfo->disassoc,
                                    bstm_reqinfo->bssterm_inc,
                                    bstm_reqinfo->bssterm_tsf,
                                    bstm_reqinfo->bssterm_duration,
                                    bstm_reqinfo->abridged,
                                    link_remove_val,
                                    bstm_reqinfo->disassoc_timer,
                                    bstm_reqinfo->validity_itvl,
                                    bcast);

    if (ret != EOK) {
        return ret;
    }

    if(vap->iv_chwidth != IEEE80211_CWM_WIDTHINVALID) {
        chwidth = vap->iv_chwidth;
    } else {
        chwidth = vap->iv_ic->ic_cwm_get_width(vap->iv_ic);
    }
#ifdef WLAN_FEATURE_11BE_MLO
    if (bstm_reqinfo->partner_link && bstm_reqinfo->link_set) {
        qdf_err("Incorrect partner_link value, Use either partner_link or link_set");
        return -EINVAL;
    }
    else if (bstm_reqinfo->partner_link)
        frm = fill_partner_link_info(vap->vdev_obj, frm, bstm_reqinfo->partner_link_preference);
    else if (bstm_reqinfo->link_set){
        if (validate_link_subsets(vap->vdev_obj, bstm_reqinfo->link_info))
            frm = fill_recommended_link_info(vap->vdev_obj, bstm_reqinfo->link_info, frm);
        else
            return -EINVAL;
    }
#endif
    if (!bstm_reqinfo->num_candidates) {
        /*
         * If user has given his own bssid-pref_val pair
         * use that in neighbour report info
         */
        frm = fill_nrinfo_from_user(vap, frm, ni, bstm_reqinfo);

        /* Add nr element for own BSS only if link removal is not imminent
           And bss temination is not set */
#ifdef WLAN_FEATURE_11BE_MLO
      if (!bstm_reqinfo->partner_link && !bstm_reqinfo->link_set && vap->wnm->include_self_bss) {
#endif
        if (!link_remove_val) {
            /* Include a Neighbor Report element for the AP's own BSS */
            OS_MEMSET(&info, 0,sizeof(struct ieee80211_nrresp_info));
            info.capinfo = IEEE80211_CAPINFO_SPECTRUM_MGMT;

            wlan_vap_get_bssid(vap, bssid);
            IEEE80211_ADDR_COPY(info.bssid, bssid);
            info.channum = vap->iv_bsschan->ic_ieee;
            info.phytype = ieee80211_rrm_get_phy_type(vap->iv_cur_mode);

            if (ieee80211_vap_mbo_is_set(vap)) {
                ieee80211_getCurrentCountryISO(vap->iv_ic, country_iso);
                wlan_get_bw_and_behav_limit(vap->iv_ic->ic_curchan, &chwidth, &behav_lim);
                /* 'global_lookup' argument in below API is sent as 'false' because
                 * the API will internally set the 'global_lookup' to 'true' for 6GHz case
                 * and 'false' for non-6GHz case.
                 */
                wlan_reg_freq_width_to_chan_op_class_auto(pdev, vap->iv_ic->ic_curchan->ic_freq,
                        chwidth, false, behav_lim, &info.regclass, &prim_chan);
            } else {
                info.regclass = get_regClassId(vap);
            }

            if (!bstm_reqinfo->disassoc && !bstm_reqinfo->bssterm_inc) {
                if (ieee80211_vap_mbo_is_set(vap)) {
                    info.preference = vap->wnm->wnm_bss_pref;
                } else {
                    info.preference = 1;
                }
            }
#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj)) {
                IEEE80211_ADDR_COPY(info.mld_addr, wlan_vdev_mlme_get_mldaddr(vap->vdev_obj));
                info.mld = wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj);
                if (!info.preference && vap->wnm->self_bss_with_link) {
                    if (wlan_vdev_get_link_id(vap->vdev_obj) != ML_PROBE_LINKID_INVALID) {
                        qdf_mem_set(info.link_info, sizeof(info.link_info), ML_PROBE_LINKID_INVALID);
                        info.link_info_present = 1;
                        info.link_info[0] = wlan_vdev_get_link_id(vap->vdev_obj);
                    }
                }
            }
#endif
#if UMAC_SUPPORT_RRM
            frm = ieee80211_add_nr_ie(frm, &info);
#endif
        }
#ifdef WLAN_FEATURE_11BE_MLO
      }
#endif
    } else {
        /* add a neighbor report for each candidate BSS */
        for (i = 0; i < bstm_reqinfo->num_candidates; i++) {
             OS_MEMSET(&info, 0,sizeof(struct ieee80211_nrresp_info));
            /* fill in neighbor report info */
            info.phytype = bstm_reqinfo->candidates[i].phy_type;
            info.regclass = bstm_reqinfo->candidates[i].op_class;

            /* the only capinfo field used in the neighbor report is spectrum management
               assume this is enabled for now (although questionable if this is used by STAs) */
            info.capinfo = IEEE80211_CAPINFO_SPECTRUM_MGMT;

            /* copy over request fields */
            IEEE80211_ADDR_COPY(&info.bssid, &bstm_reqinfo->candidates[i].bssid);
#ifdef WLAN_FEATURE_11BE_MLO
            if (bstm_reqinfo->candidates[i].mld) {
                IEEE80211_ADDR_COPY(info.mld_addr, bstm_reqinfo->candidates[i].mld_addr);
                info.mld = bstm_reqinfo->candidates[i].mld;
                if (bstm_reqinfo->candidates[i].recommend_link) {
                    info.link_info_present = bstm_reqinfo->candidates[i].recommend_link;
                    qdf_mem_copy(info.link_info, bstm_reqinfo->candidates[i].linkid, sizeof(info.link_info));
                }
            }
#endif
            info.channum = bstm_reqinfo->candidates[i].channel_number;
            info.preference = bstm_reqinfo->candidates[i].preference;

#if UMAC_SUPPORT_RRM
            frm = ieee80211_add_nr_ie(frm, &info);
#endif
        }
    }

    /* Add MBO IE and its attributes */
    if ( ieee80211_vap_mbo_is_set(vap) ) {
        frm = ieee80211_setup_mbo_ie_bstmreq(vap, frm, bstm_reqinfo,ni->ni_macaddr);
    }

#ifdef WLAN_FEATURE_11BE_MLO
    if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj)) {
        if (wlan_peer_is_mlo(ni->peer_obj))
            wbuf_set_link_agno_tx(wbuf);
    }
#endif

    qdf_mem_copy(frm, bstm_reqinfo->optie_buf, bstm_reqinfo->optie_len);
    frm += bstm_reqinfo->optie_len;

    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));
    return ieee80211_finalize_and_send_bstm_req(vap, ni, wbuf, frm);
}


/*
 * Format and send Bss transition management request action frame
 * Includes a candidate list sent from user space and the preference field
 * (differs from ieee80211_send_bstm_req since that function doesn't include any candidate
 * list by default, relies on scan information, and doesn't include the preference field)
 */
int ieee80211_send_bstm_req_target(wlan_if_t vap, wlan_node_t ni,
                                   struct ieee80211_bstm_reqinfo_target * bstm_reqinfo,
                                   bool disassoc, bool bcast)
{
    wbuf_t wbuf;
    u_int8_t *frm;
    struct ieee80211_nrresp_info info;
    int i;
    int ret;
    bool link_remove_val;
    bool preferred_candidate_list;
    struct wlan_objmgr_vdev *vdev = NULL;

    vdev = vap->vdev_obj;
    if (!vdev) {
        return -EINVAL;
    }
#ifdef WLAN_FEATURE_11BE_MLO
    if (!wlan_vdev_mlme_is_mlo_ap(vdev) && (bstm_reqinfo->link_remove ||
                                            bstm_reqinfo->partner_link)) {
        qdf_err("AP is not MLO capable, link remove %d", bstm_reqinfo->link_remove);
        return -EINVAL;
    }

    link_remove_val = bstm_reqinfo->link_remove;
#else
    link_remove_val = 0;
#endif

    preferred_candidate_list = (bool)((bstm_reqinfo->preferred_candidate_list) ?
                                bstm_reqinfo->preferred_candidate_list :
                                bstm_reqinfo->num_candidates);

    /* do the general bstm request fill in */
    /* set validity interval to maximum value.
       Unused by stations, and unclear from the standard how this value should be chosen */
    /* Other fields are left with default values (don't seem to be used in general?)
       to simplify the command line interface.  Could be added back in if needed */
    ret = ieee80211_create_bstm_req(vap, ni, &wbuf, &frm, bstm_reqinfo->dialogtoken,
                                    preferred_candidate_list,
                                    (bool)bstm_reqinfo->disassoc,
                                    false /* term_inc */,
                                    0, /* term tsf */
                                    0, /* term duration */
                                    bstm_reqinfo->abridged, /* abridged */
                                    link_remove_val,
                                    bstm_reqinfo->disassoc_timer,
                                    bstm_reqinfo->validity_itvl, /*validity_interval */
                                    bcast);
    if (ret != EOK) {
        return ret;
    }
#ifdef WLAN_FEATURE_11BE_MLO
    if (bstm_reqinfo->partner_link && bstm_reqinfo->link_set){
        qdf_err("Incorrect partner_link value, Use either partner_link or link_set");
        return -EINVAL;
    }
    else if (bstm_reqinfo->partner_link)
        frm = fill_partner_link_info(vap->vdev_obj, frm, bstm_reqinfo->partner_link_preference);
    else if (bstm_reqinfo->link_set){
        if (validate_link_subsets(vap->vdev_obj, bstm_reqinfo->link_info))
            frm = fill_recommended_link_info(vap->vdev_obj, bstm_reqinfo->link_info, frm);
        else
            return -EINVAL;
    }
#endif
    /* add a neighbor report for each candidate BSS */
    for (i = 0; i<bstm_reqinfo->num_candidates; i++) {
        OS_MEMSET(&info, 0, sizeof(struct ieee80211_nrresp_info));
        /* fill in neighbor report info */
        info.phytype = bstm_reqinfo->candidates[i].phy_type;
        info.regclass = bstm_reqinfo->candidates[i].op_class;

        /* the only capinfo field used in the neighbor report is spectrum management
           assume this is enabled for now (although questionable if this is used by STAs) */
        info.capinfo = IEEE80211_CAPINFO_SPECTRUM_MGMT;

        /* copy over request fields */
        IEEE80211_ADDR_COPY(&info.bssid, &bstm_reqinfo->candidates[i].bssid);
        info.channum = bstm_reqinfo->candidates[i].channel_number;
        info.preference = bstm_reqinfo->candidates[i].preference;
#ifdef WLAN_FEATURE_11BE_MLO
            if (bstm_reqinfo->candidates[i].mld) {
                IEEE80211_ADDR_COPY(info.mld_addr, bstm_reqinfo->candidates[i].mld_addr);
                info.mld = bstm_reqinfo->candidates[i].mld;
                if (bstm_reqinfo->candidates[i].recommend_link) {
                    info.link_info_present = bstm_reqinfo->candidates[i].recommend_link;
                    qdf_mem_copy(info.link_info, bstm_reqinfo->candidates[i].linkid, sizeof(info.link_info));
                }
            }
#endif

#if UMAC_SUPPORT_RRM
        frm = ieee80211_add_nr_ie(frm, &info);
#endif
    }
        /* Adding mbo ie and attributes */
#if QCN_IE
    if ( ieee80211_vap_mbo_is_set(vap) &&
         (ni->ni_qcn_version_flag || vap->iv_mbo_bstm) ) {
#else
    if( ieee80211_vap_mbo_is_set(vap)) {
#endif
        frm = ieee80211_setup_mbo_ie_bstmreq_target(vap, frm, bstm_reqinfo,ni->ni_macaddr);
    }

#ifdef WLAN_FEATURE_11BE_MLO
    if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj)) {
        if (wlan_peer_is_mlo(ni->peer_obj))
            wbuf_set_link_agno_tx(wbuf);
    }
#endif

    ret = ieee80211_finalize_and_send_bstm_req(vap, ni, wbuf, frm);
    if ((ret == EOK) && disassoc) {
        OS_SET_TIMER(&ni->ni_btm_disassoc_timer,
                     IEEE80211_TU_TO_MS(bstm_reqinfo->disassoc_timer * ieee80211_vap_get_beacon_interval(vap)));
        ni->ni_btm_disassoc_sta = true;
    }

    return ret;
}

#if UMAC_SUPPORT_RRM
static u_int8_t get_nghbr_count_from_frame(u_int8_t *frm, int frm_len)
{
    u_int8_t *sfrm = NULL;
    u_int8_t elem_id, len;
    u_int8_t count = 0;

    sfrm = frm;
    while ((frm - sfrm) < frm_len) {
        elem_id = *frm++;
        len = *frm++;

        switch (elem_id) {
            case IEEE80211_ELEMID_NEIGHBOR_REPORT:
                count++;
                break;
            default :
                break;
        }
        frm += len;
    }
    return count;
}

static void ieee80211_neighbor_rpt_subelement(ieee80211_cust_nrresp_info_t *nr_info,
                                              u_int8_t *subelm_frm, u_int8_t subelm_len)
{
    u_int8_t remaining_subelm_len;
    u_int8_t subelm_id, len;

    remaining_subelm_len = subelm_len;

    while (remaining_subelm_len  >= (sizeof(subelm_id) + sizeof(len)) ) {
           subelm_id = *subelm_frm++;
           len = *subelm_frm++;

           switch (subelm_id) {
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_TSF:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_TSF;
                        OS_MEMCPY(&nr_info->nr_tsf, subelm_frm, sizeof(struct ieee80211_nr_resp_tsf));
                        break;
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_COUNTRY:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_COUNTRY;
                        OS_MEMCPY(&nr_info->nr_country, subelm_frm, sizeof(struct ieee80211_nr_resp_country));
                        break;
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_PREFERENCE:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_CAND_PREF;
                        OS_MEMCPY(&nr_info->nr_cand_pref, subelm_frm, sizeof(struct ieee80211_nr_cand_pref));
                        break;
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_TERM_DUR:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_TERM_DUR;
                        OS_MEMCPY(&nr_info->nr_term_duration, subelm_frm, sizeof(struct ieee80211_nr_term_duration));
                        break;
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_BEARING:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_BEARING;
                        OS_MEMCPY(&nr_info->nr_bearing, subelm_frm, sizeof(struct ieee80211_nr_bearing));
                        break;
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_WBC:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_WIDEBAND;
                        OS_MEMCPY(&nr_info->nr_wb_chan, subelm_frm, sizeof(struct ieee80211_wband_chan));
                        break;
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_HTCAP:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_HT_CAPS;
                        OS_MEMCPY(&nr_info->nr_ht_cap, subelm_frm, sizeof(struct ieee80211_nr_htcap));
                        break;
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_HTOP:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_HT_OP;
                        OS_MEMCPY(&nr_info->nr_ht_op, subelm_frm, sizeof(struct ieee80211_nr_htop));
                        break;
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_SEC_CHAN:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_SEC_CHAN_OFFSET;
                        OS_MEMCPY(&nr_info->nr_sec_chan, subelm_frm, sizeof(struct ieee80211_nr_sec_chan));
                        break;
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_PILOT:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_MEAS_PILOT;
                        OS_MEMCPY(&nr_info->nr_meas_pilot, subelm_frm, sizeof(struct ieee80211_nr_meas_pilot));
                        break;
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_RM_CAP:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_RM_EN_CAPS;
                        OS_MEMCPY(&nr_info->nr_rm_en_caps, subelm_frm, sizeof(struct ieee80211_nr_rm_en_caps));
                        break;
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_VHTCAP:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_VHT_CAPS;
                        OS_MEMCPY(&nr_info->nr_vhtcaps, subelm_frm, sizeof(struct ieee80211_nr_vhtcap));
                        break;
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_VHTOP:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_VHT_OP;
                        OS_MEMCPY(&nr_info->nr_vht_op, subelm_frm, sizeof(struct ieee80211_nr_vhtop));
                        break;
                   case IEEE80211_SUBELEMID_NEIGHBORREPORT_VENDOR:
                        nr_info->subie_pres |= IEEE80211_NR_SUBIE_PRES_VENDOR;
                        OS_MEMCPY(&nr_info->nr_vendor, subelm_frm, sizeof(struct ieee80211_beaconreq_vendor));
                        break;
                   default:
                        break;
           }
           subelm_frm += len;
           remaining_subelm_len -= sizeof(subelm_id) + sizeof(len) + len;
    }
}
#endif

static u_int8_t *get_nghbr_rpt_frm_bstm(ieee80211_cust_nrresp_info_t *nr_info,
                                         u_int8_t *frm, int frm_len, wlan_node_t ni)
{
    u_int8_t *sfrm;
    u_int8_t elem_id, len;
#if UMAC_SUPPORT_RRM
    u_int8_t *subelm_frm = NULL;
    u_int8_t subelm_len = 0;
    struct ieee80211_nr_ie *nrresp = NULL;
#endif

    sfrm = frm;
    while ((frm - sfrm) < frm_len) {
        elem_id = *frm;
        len = *(frm + 1);

        switch (elem_id) {
#if UMAC_SUPPORT_RRM
        case IEEE80211_ELEMID_NEIGHBOR_REPORT:
            if (nr_info) {
                nrresp = (struct ieee80211_nr_ie *)frm;
                IEEE80211_ADDR_COPY(nr_info->bssid, nrresp->bssid);
                nr_info->regclass = nrresp->regclass;
                nr_info->channum = nrresp->channum;
                nr_info->phy_type = nrresp->phytype;

                nr_info->bssi_info = nrresp->bsinfo0_ap_reach;
                if (nrresp->bsinfo0_security)
                    nr_info->bssi_info |= IEE80211_NR_BSSINFO_SEC_MASK;
                if (nrresp->bsinfo0_keyscope)
                    nr_info->bssi_info |= IEE80211_NR_BSSINFO_KEYSCOPE_MASK;
                if (nrresp->bsinfo0_specmgmt)
                    nr_info->bssi_info |= IEE80211_NR_BSSINFO_CAP_SPEC_MASK;
                if (nrresp->bsinfo0_qos)
                    nr_info->bssi_info |= IEE80211_NR_BSSINFO_CAP_QOS_MASK;
                if (nrresp->bsinfo0_apsd)
                    nr_info->bssi_info |= IEE80211_NR_BSSINFO_CAP_APSD_MASK;
                if (nrresp->bsinfo0_rrm)
                    nr_info->bssi_info |= IEE80211_NR_BSSINFO_CAP_RRM_MASK;

                if (nrresp->bsinfo1_dba)
                    nr_info->bssi_info |= IEE80211_NR_BSSINFO_CAP_DBA_MASK;
                if (nrresp->bsinfo1_iba)
                    nr_info->bssi_info |= IEE80211_NR_BSSINFO_CAP_IBA_MASK;
                if (nrresp->bsinfo1_mdomain)
                    nr_info->bssi_info |= IEE80211_NR_BSSINFO_MDOMAIN_MASK;
                if (nrresp->bsinfo1_ht)
                    nr_info->bssi_info |= IEE80211_NR_BSSINFO_HT_MASK;
                if (nrresp->bsinfo1_vht)
                    nr_info->bssi_info |= IEE80211_NR_BSSINFO_VHT_MASK;
                if (nrresp->bsinfo1_ftm)
                    nr_info->bssi_info |= IEE80211_NR_BSSINFO_FTM_MASK;

                if (len > NEIGHBOR_REPORT_FIXED_LEN) {
                    subelm_frm = (u_int8_t *)(&nrresp->subelm[0]);
                    subelm_len = len - NEIGHBOR_REPORT_FIXED_LEN;
                    ieee80211_neighbor_rpt_subelement(nr_info, subelm_frm, subelm_len);
                }
                nr_info++;
            }
            break;
#endif
        case IEEE80211_ELEMID_VENDOR:
            if (ni) {
                if (ismbooui(frm)) {
                    ieee80211_parse_mboie(frm, ni);
                }
            }
            break;
        default :
	    break;
        }
        /* 2 bytes for the element id and element len */
        frm += (len + 2);
    }
    return frm;
}

/*
 * Bss transition management query recv handler
 */
int ieee80211_recv_bstm_query(wlan_if_t vap, wlan_node_t ni,
                  struct ieee80211_action_bstm_query *bstm_query, int frm_len)
{
    u_int8_t ret = 0;
    u_int8_t *frm, *sfrm;
    u_int32_t bstmquery_ev_len = 0;
    struct ieee80211_bstm_reqinfo reqinfo;
    struct ieee80211_bstmquery_ev_data *bstmquery_ev_data = NULL;
    ieee80211_cust_nrresp_info_t *nr_info = NULL;
    u_int8_t num_candidate = 0;
    ieee80211_wnm_t wnm = vap->wnm;

    wnm->recv_bstm_query++;

    memset(&reqinfo, 0, sizeof(struct ieee80211_bstm_reqinfo));
    reqinfo.dialogtoken = bstm_query->dialogtoken;
    reqinfo.candidate_list = 1;
    reqinfo.disassoc_timer = 0;
    reqinfo.bssterm_inc= 0;
    reqinfo.validity_itvl = BSSTRANS_LIST_VALID_ITVL;

    sfrm = frm = &bstm_query->opt_ies[0];
#if UMAC_SUPPORT_RRM
    num_candidate = get_nghbr_count_from_frame(frm, frm_len);
#endif
    bstmquery_ev_len = (num_candidate * sizeof(ieee80211_cust_nrresp_info_t)) +
                        sizeof(struct ieee80211_bstmquery_ev_data);
    bstmquery_ev_data = (struct ieee80211_bstmquery_ev_data *)qdf_mem_malloc(bstmquery_ev_len);
    if (bstmquery_ev_data) {
        qdf_mem_zero(bstmquery_ev_data, bstmquery_ev_len);
        if (num_candidate) {
            nr_info = (ieee80211_cust_nrresp_info_t *)(&bstmquery_ev_data->canditates[0]);
            frm = get_nghbr_rpt_frm_bstm(nr_info, frm, frm_len, NULL);
        }
        bstmquery_ev_data->type = BSTM_QUERY_DATA;
        bstmquery_ev_data->token = bstm_query->dialogtoken;
        bstmquery_ev_data->num_candidate = num_candidate;
        bstmquery_ev_data->reason = bstm_query->reason;
        IEEE80211_ADDR_COPY(bstmquery_ev_data->peer_mac, ni->ni_macaddr);

        IEEE80211_DELIVER_EVENT_BSTM_AP(vap, bstmquery_ev_data, bstmquery_ev_len);
        qdf_mem_free(bstmquery_ev_data);
    }
    if (!wnm->wnm_app_filter) {
        /* send bss transition management req info */
        ret = ieee80211_send_bstm_req(vap, ni, &reqinfo, FALSE);
    }
    return ret;
}

/*
 * Bss transition management response recv handler
 */
int ieee80211_recv_bstm_resp(wlan_if_t vap, wlan_node_t ni,
                     struct ieee80211_action_bstm_resp *bstm_resp, int frm_len)
{
    u_int8_t num_candidate = 0;
    u_int8_t *frm, *sfrm;
    char bssid[QDF_MAC_ADDR_SIZE];
    u_int32_t bstmrsp_ev_len = 0;
    ieee80211_cust_nrresp_info_t *nr_info = NULL;
    struct ieee80211_bstmresp_ev_data *bstmrsp_ev_data = NULL;
    ieee80211_wnm_t wnm = vap->wnm;

    wnm->recv_bstm_resp++;

    if (bstm_resp->status == 0) {
        /* Station Transition to other BSS */
        if (frm_len >= sizeof(struct ieee80211_action_bstm_resp) + QDF_MAC_ADDR_SIZE) {
            sfrm = frm = &bstm_resp->opt_ies[QDF_MAC_ADDR_SIZE];
            memcpy(bssid, &bstm_resp->opt_ies[0], QDF_MAC_ADDR_SIZE);
        } else {
            sfrm = frm = &bstm_resp->opt_ies[0];
            memset(bssid, 0, QDF_MAC_ADDR_SIZE);
        }
    } else {
            /* Station Not Transition to other BSS */
            sfrm = frm = &bstm_resp->opt_ies[0];
            memset(bssid, 0, QDF_MAC_ADDR_SIZE);
    }

#if UMAC_SUPPORT_RRM
    num_candidate = get_nghbr_count_from_frame(frm, frm_len);
#endif
    bstmrsp_ev_len = (num_candidate * sizeof(ieee80211_cust_nrresp_info_t)) +
                      sizeof(struct ieee80211_bstmresp_ev_data);
    bstmrsp_ev_data = (struct ieee80211_bstmresp_ev_data *)qdf_mem_malloc(bstmrsp_ev_len);
    if (bstmrsp_ev_data) {
        qdf_mem_zero(bstmrsp_ev_data, bstmrsp_ev_len);
        if (num_candidate) {
            nr_info = (ieee80211_cust_nrresp_info_t *)(&bstmrsp_ev_data->canditates[0]);
            frm = get_nghbr_rpt_frm_bstm(nr_info, frm, frm_len, ni);
        }
        bstmrsp_ev_data->type = BSTM_RESP_DATA;
        bstmrsp_ev_data->status = bstm_resp->status;
        bstmrsp_ev_data->reject_code = IEEE80211_BSTM_REJECT_REASON_INVALID;
        if(bstm_resp->status != 0)
            bstmrsp_ev_data->reject_code = ni->ni_mbo.trans_reject_code; //Populate this reject code by accessing MBO Reject Code in BSTM Response //
        bstmrsp_ev_data->token = bstm_resp->dialogtoken;
        bstmrsp_ev_data->termination_delay = bstm_resp->bssterm_delay;
        bstmrsp_ev_data->num_candidate = num_candidate;
        IEEE80211_ADDR_COPY(bstmrsp_ev_data->peer_mac, ni->ni_macaddr);
        IEEE80211_ADDR_COPY(bstmrsp_ev_data->target_mac, bssid);

        IEEE80211_DELIVER_EVENT_BSTM_AP(vap, bstmrsp_ev_data, bstmrsp_ev_len);
        qdf_mem_free(bstmrsp_ev_data);
    }

   IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Received BSS Transmision "
           "Managemnet Response (len = %d): dialog_token %d, status %d, "
           "delay %d, bssid %s\n", frm_len, bstm_resp->dialogtoken,
           bstm_resp->status, bstm_resp->bssterm_delay,
           bstm_resp->status ? "n/a" : ether_sprintf(bssid));

   return 0;
}

static
void ieee80211_wnm_bss_validate_inactivity_iter_cb(void *arg,
                                                   struct ieee80211_node *ni)
{
    struct ieee80211vap *vap;
    systime_t time_diff;
    u_int32_t idle_period;
    u_int32_t time_msec;

    vap = ni->ni_vap;

    if (vap->iv_opmode != IEEE80211_M_HOSTAP ||
        !ieee80211_vap_wnm_is_set(vap) ||
        IEEE80211_ADDR_EQ(ni->ni_macaddr, vap->iv_myaddr) || !ni->ni_associd)
        return;

    if (ieee80211node_has_flag(ni, IEEE80211_NODE_DELAYED_CLEANUP)) {
        return;
    }

    if (ieee80211_wnm_bss_is_set(vap->wnm)) {
        time_diff = OS_GET_TIMESTAMP() - ni->ni_wnm->last_rcvpkt_tstamp;
        time_msec = CONVERT_SYSTEM_TIME_TO_MS(time_diff);
        /*
         * Convert msec to Max BSS Idle Period
         *
         *     1 TU == 1024 usec.
         *     Max BSS Idle Period is in the unit of 1000 TU.
         *
         * The compiler will optimize the divid w/ shift.
         */
        idle_period = time_msec / TIME_UNIT_TO_MICROSEC;

        if ((idle_period >= vap->wnm->wnm_bss_max_idle_period)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Disassociate %s "
                      "due to max bss idle period %d (1000TU) reached. "
                      "Time difference is %u msec\n",
                      ether_sprintf(ni->ni_macaddr),
                      vap->wnm->wnm_bss_max_idle_period, time_msec);

            ieee80211_try_mark_node_for_delayed_cleanup(ni);
            wlan_mlme_disassoc_request(ni->ni_vap, ni->ni_macaddr,
                                       IEEE80211_REASON_ASSOC_EXPIRE);
        }
    }
}

int ieee80211_wnm_bss_validate_inactivity(struct ieee80211com *ic)
{
    wlan_mlme_iterate_node_list(ic,
                        ieee80211_wnm_bss_validate_inactivity_iter_cb, NULL,
                        IEEE80211_NODE_ITER_F_ASSOC_STA);

    return 0;
}

void ieee80211_wnm_bssmax_updaterx(struct ieee80211_node *ni, int secured)
{
    if (ni->ni_wnm == NULL) {
        return;
    }

    /* Protected Keep Alive */
    if (ni->wnm_bss_idle_option & WNM_BSS_IDLE_PROTECTED_KEEPALIVE) {
        if (secured) {
            ni->ni_wnm->last_rcvpkt_tstamp = OS_GET_TIMESTAMP();
        }
    } else {
        ni->ni_wnm->last_rcvpkt_tstamp = OS_GET_TIMESTAMP();
    }
}


u_int8_t * ieee80211_add_bssmax(u_int8_t *frm, struct ieee80211vap *vap,
        int prot)
{
    *frm++ = IEEE80211_ELEMID_BSSMAX_IDLE_PERIOD;
    *frm++ = 3;
    *(u_int16_t *)frm = htole16(vap->wnm->wnm_bss_max_idle_period);
    frm += 2;
    *(u_int8_t *)frm = prot;
    frm += 1;

    return frm;
}

/* Flexible Multicast Service(FMS) Routines */
static int ieee80211_fms_counter_free(ieee80211_fms_t fms, int cid)
{
    if (cid >= IEEE80211_FMS_MAX_COUNTERS)
        return -1;

    if (fms->counters[cid].numfms > 0) {
        fms->counters[cid].numfms--;
    }

    if (fms->counters[cid].numfms == 0) {
        fms->counters[cid].inuse = 0;
        fms->counters[cid].current_count = 0;
        fms->counters[cid].del_itvl = 0;
    }

    return 0;
}

static int ieee80211_fms_counter_create(ieee80211_fms_t fms, int del_itvl, int *counterid)
{
    int cid;

    for (cid = 0; cid < IEEE80211_FMS_MAX_COUNTERS; cid++) {
        /* Allocate first free counter */
        if (fms->counters[cid].inuse == 0) {
            fms->counters[cid].inuse = 1;
            break;
        }
    }

    if (cid < IEEE80211_FMS_MAX_COUNTERS) {
        fms->counters[cid].numfms++;
        *counterid = cid;
    } else
        return -1;

    return 0;
}

static bool ieee80211_fms_is_tclass_equal(struct tclas_element *tclass1,
                                   struct tclas_element *tclass2)
{
    u_int8_t filter_len;

    if (tclass1 == NULL || tclass2 == NULL) {
        return false;
    }

    if (tclass1->type != tclass2->type) {
        return false;
    } else {
        if (tclass1->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE3) {
            filter_len = 0;
            if (tclass1->tclas.type3.filter_offset != tclass2->tclas.type3.filter_offset ||
                OS_MEMCMP(tclass1->tclas.type3.filter_value,
                    tclass2->tclas.type3.filter_value, filter_len) ||
                OS_MEMCMP(tclass1->tclas.type3.filter_mask,
                    tclass2->tclas.type3.filter_mask, filter_len))
            {
                return false;
            }
        } else if (tclass1->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE1 ||
                   tclass1->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4)
        {
            if (tclass1->tclas.type14.type14_v4.version !=
                tclass2->tclas.type14.type14_v4.version)
            {
                return false;
            }

            if (tclass1->tclas.type14.type14_v4.version == IEEE80211_WNM_TCLAS_CLAS14_VERSION_4) {
                if (OS_MEMCMP(tclass1->tclas.type14.type14_v4.src_ip,
                        tclass2->tclas.type14.type14_v4.src_ip, IEEE80211_IPV4_LEN) ||
                    OS_MEMCMP(tclass1->tclas.type14.type14_v4.dst_ip,
                        tclass2->tclas.type14.type14_v4.dst_ip, IEEE80211_IPV4_LEN) ||
                    tclass1->tclas.type14.type14_v4.src_port !=
                        tclass2->tclas.type14.type14_v4.src_port ||
                    tclass1->tclas.type14.type14_v4.dst_port !=
                        tclass2->tclas.type14.type14_v4.dst_port ||
                    tclass1->tclas.type14.type14_v4.dscp !=
                        tclass2->tclas.type14.type14_v4.dscp ||
                    tclass1->tclas.type14.type14_v4.protocol !=
                        tclass2->tclas.type14.type14_v4.protocol)
                {
                    return false;
                }
            }

            if (tclass1->tclas.type14.type14_v6.version == IEEE80211_WNM_TCLAS_CLAS14_VERSION_6) {
                if (OS_MEMCMP(tclass1->tclas.type14.type14_v6.src_ip,
                        tclass2->tclas.type14.type14_v6.src_ip, IEEE80211_IPV6_LEN) ||
                    OS_MEMCMP(tclass1->tclas.type14.type14_v6.dst_ip,
                        tclass2->tclas.type14.type14_v6.dst_ip, IEEE80211_IPV6_LEN) ||
                    tclass1->tclas.type14.type14_v6.src_port !=
                        tclass2->tclas.type14.type14_v6.src_port ||
                    tclass1->tclas.type14.type14_v6.dst_port !=
                        tclass2->tclas.type14.type14_v6.dst_port ||
                    OS_MEMCMP(tclass1->tclas.type14.type14_v6.flow_label,
                        tclass2->tclas.type14.type14_v6.flow_label, 3))
                {
                    return false;
                }

                if (tclass1->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4) {
                    if (tclass1->tclas.type14.type14_v6.type4_dscp !=
                        tclass2->tclas.type14.type14_v6.type4_dscp ||
                        tclass1->tclas.type14.type14_v6.type4_next_header !=
                        tclass2->tclas.type14.type14_v6.type4_next_header)
                    {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

static bool ieee80211_fms_subelement_match(ieee80211_fms_stream_t *fms_stream,
        ieee80211_fmsreq_subele_t *fmsreq_subele)
{
    struct tclas_element *tclass1, *tclass2;
    bool tclass_found = false;

    if (fms_stream->num_tclas != fmsreq_subele->num_tclas) {
        return false;
    }
    if (fms_stream->del_itvl != fmsreq_subele->del_itvl ||
        fms_stream->max_del_itvl !=  fmsreq_subele->max_del_itvl ||
        OS_MEMCMP(&fms_stream->rate_id, &fmsreq_subele->rate_id,
            sizeof(ieee80211_fms_rate_identifier_t)))
    {
        return false;
    }

    TAILQ_FOREACH(tclass2, &fmsreq_subele->tclas_head, tclas_next) {
        tclass_found = false;

        TAILQ_FOREACH(tclass1, &fms_stream->tclas_head, tclas_next) {
            if (ieee80211_fms_is_tclass_equal(tclass1, tclass2)) {
                /* tclass found to go next tclass */
                tclass_found = true;
                break;
            }
        }
        if (tclass_found == false) {
            break;
        }
    }

    if (tclass2 == NULL && tclass_found == true) {
        return true;
    }

    return false;
}

static bool ieee80211_fms_subelement_tclass_match(void *head1, void *head2)
{
    struct tclas_element *tclass1, *tclass2;
    bool tclass_found = false;
    TAILQ_HEAD(, tclas_element) *fmsstrm_tclas;
    TAILQ_HEAD(, tclas_element) *fmsreq_tclas;

    fmsstrm_tclas = head1;
    fmsreq_tclas = head2;

    TAILQ_FOREACH(tclass2, fmsreq_tclas, tclas_next) {
        tclass_found = false;

        TAILQ_FOREACH(tclass1, fmsstrm_tclas, tclas_next) {
            if (ieee80211_fms_is_tclass_equal(tclass1, tclass2)) {
                /* tclass found to go next tclass */
                tclass_found = true;
                break;
            }
        }
        if (tclass_found == false) {
            break;
        }
    }

    if (tclass2 == NULL && tclass_found == true) {
        return true;
    }

    return false;
}

static ieee80211_fms_stream_t* ieee80211_fms_stream_add(wlan_if_t vap,
        wlan_node_t ni, ieee80211_fmsreq_subele_t *fmsreq_subele,
        ieee80211_fms_subelement_status_t *fmssubele_status)
{
    ieee80211_fms_t fms;
    ieee80211_fms_counter_t *fms_counter;
    ieee80211_fms_stream_t *new_fms_stream;
    int i, counter_id, rv;

    fms = vap->wnm->wnm_fms_data;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " Enter Function %s\n", __func__);

    for (i = 0; i < IEEE80211_FMS_MAX_COUNTERS; i++) {
        if ((fms->counters[i].inuse == 1) &&
            (fms->counters[i].del_itvl == fmsreq_subele->del_itvl))
        {
            /* Reusing the same counter i */
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Reusing counter id - "
                    "%d for del_itvl %d\n", i, fmsreq_subele->del_itvl);
            counter_id = i;
            break;
        }
    }

    if (i == IEEE80211_FMS_MAX_COUNTERS) {
        rv = ieee80211_fms_counter_create(fms, fmsreq_subele->del_itvl, &counter_id);
        if (rv != 0) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "%s : line %d no free "
                    "counters available\n", __func__, __LINE__);
            return NULL;
        }
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "New counter id %d for "
                "del_itvl %d\n", counter_id, fmsreq_subele->del_itvl);
        fms_counter = &fms->counters[counter_id];
        fms_counter->counter_id = counter_id;
        fms_counter->del_itvl = fmsreq_subele->del_itvl;
        fms_counter->current_count = fmsreq_subele->del_itvl;
    } else {
        fms_counter = &fms->counters[counter_id];
    }

    new_fms_stream = OS_MALLOC(vap->iv_ic->ic_osdev, sizeof(ieee80211_fms_stream_t), 0);
    if (new_fms_stream == NULL) {
        qdf_nofl_info("%s:%d OS_MALLOC failed !\n", __func__, __LINE__);
        return NULL;
    }

    OS_MEMSET(new_fms_stream, 0, sizeof(ieee80211_fms_stream_t));
    new_fms_stream->counter_id = fms_counter->counter_id;
    new_fms_stream->fmsid = ++fms->fmsid;
    new_fms_stream->del_itvl = fmsreq_subele->del_itvl;
    new_fms_stream->max_del_itvl = fmsreq_subele->max_del_itvl;
    new_fms_stream->rate_id.mask = fmsreq_subele->rate_id.mask;
    new_fms_stream->rate_id.mcs_idx = fmsreq_subele->rate_id.mcs_idx;
    new_fms_stream->rate_id.rate = fmsreq_subele->rate_id.rate;
    new_fms_stream->num_tclas = fmsreq_subele->num_tclas;
    OS_MEMCPY(new_fms_stream->mcast_addr, fmsreq_subele->mcast_addr, 6);
    TAILQ_INIT(&new_fms_stream->tclas_head);
    TAILQ_CONCAT(&new_fms_stream->tclas_head, &fmsreq_subele->tclas_head, tclas_next);
    OS_MEMCPY(&new_fms_stream->tclasprocess, &fmsreq_subele->tclasprocess,
            sizeof(struct ieee80211_tclas_processing));

    fmsreq_subele->accepted = 1;

    fmssubele_status->fmsid = new_fms_stream->fmsid;
    fmssubele_status->fms_counter = (fms_counter->counter_id << 5) |
                                    fms_counter->current_count;
    TAILQ_INSERT_TAIL(&fms_counter->fms_stream_head, new_fms_stream, stream_next);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "*** Exiting function %s ***\n", __func__);

    return new_fms_stream;
}

static int ieee80211_recv_fmssubele_parse(wlan_if_t vap, wlan_node_t ni,
        void *head, u_int8_t *frm, u_int8_t fmsreq_ie_len)

{
   u_int8_t tclass_len;
   ieee80211_fmsreq_subele_t *fmsreq_subele;
   TAILQ_HEAD(, ieee80211_fmsreq_subele_s) *fmssubele_head;
   struct fmsresp_tclas_subele_status tclas_status_msg;
   int rv;
   u_int16_t rate;
   u_int8_t mcast_addr[6] = {0x01, 0x00, 0x5e, 0, 0, 0};

   fmssubele_head = head;
   TAILQ_INIT(fmssubele_head);

   IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " Enter Function %s\n", __func__);
   while (fmsreq_ie_len > 0) {
       fmsreq_subele = OS_MALLOC(vap->iv_ic->ic_osdev,
               sizeof(ieee80211_fmsreq_subele_t), 0);
       if (fmsreq_subele == NULL) {
           qdf_nofl_info("%s:%d OS_MALLOC failed!\n", __func__, __LINE__);
           return ENOMEM;
       }
       fmsreq_subele->elem_id = *frm++;
       IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Subelement Id %x\n", fmsreq_subele->elem_id);
       if (IEEE80211_FMS_SUBELEMENT_ID == fmsreq_subele->elem_id) {
           fmsreq_subele->len = *frm++;
           fmsreq_subele->del_itvl = *frm++;
           fmsreq_subele->max_del_itvl = *frm++;
           fmsreq_subele->rate_id.mask = *frm++;
           fmsreq_subele->rate_id.mcs_idx = *frm++;
           memcpy(&rate,frm, 2);
           fmsreq_subele->rate_id.rate = be16toh(rate);
           frm += 2;

           tclass_len = fmsreq_subele->len - 6;
           IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "len %d di %d max di %d\n",
                   fmsreq_subele->len, fmsreq_subele->del_itvl,
                   fmsreq_subele->max_del_itvl);

           if (tclass_len > 0) {
               rv = ieee80211_tclass_element_parse(vap,
                       &fmsreq_subele->tclas_head,
                       &fmsreq_subele->tclasprocess,
                       &tclas_status_msg, &frm, tclass_len);
               if (rv) {
                   OS_FREE(fmsreq_subele);
                   IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "%s:%d failed\n", __func__, __LINE__);
                   return rv;
               }
               if (tclas_status_msg.ismcast != 1) {
                   IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "%s:%d failed\n", __func__, __LINE__);
                   OS_FREE(fmsreq_subele);
                   return -EINVAL;
               } else {
                   mcast_addr[3] = (tclas_status_msg.mcast_ipaddr >> 16) & 0x7f;
                   mcast_addr[4] = (tclas_status_msg.mcast_ipaddr >> 8) & 0xff;
                   mcast_addr[5] = (tclas_status_msg.mcast_ipaddr & 0xff);
                   memcpy(&fmsreq_subele->mcast_addr, mcast_addr, 6);
               }
               IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "clas_status_msg."
                                 "num_tclas_elements %d\n",
                                 tclas_status_msg.num_tclas_elements);
               fmsreq_subele->num_tclas = tclas_status_msg.num_tclas_elements;
           }
           IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "fmsreq_ie_len %d\n", fmsreq_ie_len);
           fmsreq_ie_len = fmsreq_ie_len - (fmsreq_subele->len + 3);
           IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "fmsreq_ie_len %d\n", fmsreq_ie_len);
           TAILQ_INSERT_TAIL(fmssubele_head, fmsreq_subele, fmssubele_next);
       } else {
           qdf_nofl_info("%s: Unknown Element ID %d\n", __func__, fmsreq_subele->elem_id);
           OS_FREE(fmsreq_subele);
           return -EINVAL;
       }
   }
   IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " Exit Function %s\n", __func__);
   return 0;
}

/*
 * Sends unsolicit response to the group to indicate
 * the change in FMS stream parameters.
 */
static int ieee80211_send_unsol_fmsrsp(wlan_if_t vap, wlan_node_t ni,
                                ieee80211_fms_subelement_status_t *fmssubele_status)
{
    wbuf_t wbuf;
    u_int8_t *frm;
    u_int8_t *len;
    struct ieee80211_frame *wh;
    struct ieee80211_fmsrsp *fmsrsp;
    u_int16_t rate;
    /*u_int8_t bcast_addr[6] = {0xff,0xff,0xff,0xff,0xff, 0xff};*/

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "%s: status %d del_itvl=%d "
            "max_del_itvl=%d\n", __func__, fmssubele_status->element_status,
            fmssubele_status->del_itvl,fmssubele_status->max_del_itvl);
    fmsrsp = ni->ni_wnm->fmsrsp;

    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm, 0);
    if (wbuf == NULL) {
        return -ENOMEM;
    }

    *frm++ = IEEE80211_ACTION_CAT_WNM;
    *frm++ = IEEE80211_ACTION_FMS_RESP;
    *frm++ = ni->ni_wnm->fmsreq->dialog_token;

    *frm++ = IEEE80211_ELEMID_FMS_RESPONSE;
    len = frm;
    *len = 1;
    frm++;

    *frm++ = 0; /* fms_token */
    /* setup the wireless header */
    wh = (struct ieee80211_frame *)wbuf_header(wbuf);
#if 1
    ieee80211_send_setup(vap, ni, wh,
                         IEEE80211_FC0_TYPE_MGT | IEEE80211_FC0_SUBTYPE_ACTION,
                         vap->iv_myaddr, fmssubele_status->mcast_addr, ni->ni_bssid);
#else
    /* Multicast FMS request frames application
       using broadcast for testing */
    ieee80211_send_setup(vap, ni, wh,
                         IEEE80211_FC0_TYPE_MGT | IEEE80211_FC0_SUBTYPE_ACTION,
                         vap->iv_myaddr, bcast_addr, ni->ni_bssid);
#endif

    *frm++ = IEEE80211_FMS_STATUS_SUBELE;
    *frm++ = 15;
    *frm++ = fmssubele_status->element_status;
    *frm++ = fmssubele_status->del_itvl;
    *frm++ = fmssubele_status->max_del_itvl;
    *frm++ = fmssubele_status->fmsid;
    *frm++ = fmssubele_status->fms_counter;
    *frm++ = fmssubele_status->rate_id.mask;
    *frm++ = fmssubele_status->rate_id.mcs_idx;
    rate  = htobe16(fmssubele_status->rate_id.rate);
    OS_MEMCPY(frm, &rate, 2);
    frm += 2;

    OS_MEMCPY(frm, &fmssubele_status->mcast_addr, 6);
    frm += 6;
    *len += 17;

    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " Exit Function %s\n", __func__);
    return ieee80211_send_mgmt(vap, ni, wbuf, false);
}

static int ieee80211_fms_stream_modify(wlan_if_t vap, wlan_node_t ni,
                                ieee80211_fms_stream_t *fms_stream_match,
                                ieee80211_fmsreq_subele_t *fmsreq_subele,
                                ieee80211_fms_subelement_status_t *fmssubele_status)
{
   ieee80211_fms_t fms = vap->wnm->wnm_fms_data;
   u_int8_t new_del_itvl, new_max_del_itvl;
   u_int8_t old_del_itvl, old_max_del_itvl;
   u_int8_t rx_del_itvl, rx_max_del_itvl;
   ieee80211_fms_subelement_status_t unsol_subele_status;
   int unsol_rsp_send = 1;
   ieee80211_fms_counter_t *fms_counter, *new_counter;
   int unsol_fms_status=-1;
   int i, counter_id, rv;

   IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "*** Entering function %s\n", __func__);
   /* FMS stream delivery interval has changed */
   old_del_itvl = fms_stream_match->del_itvl;
   old_max_del_itvl = fms_stream_match->max_del_itvl;

   rx_del_itvl = fmsreq_subele->del_itvl;
   rx_max_del_itvl = fmsreq_subele->max_del_itvl;

   if ((old_max_del_itvl == 0 && rx_max_del_itvl != 0) ||
       (old_max_del_itvl != 0 && rx_max_del_itvl != 0 && rx_max_del_itvl < old_max_del_itvl))
   {
       new_max_del_itvl = rx_max_del_itvl;
       if (old_del_itvl > rx_max_del_itvl) {
           unsol_fms_status = IEEE80211_WNM_FMSSUBELE_ALTE_MAX_DEL_ITVL_CHG;
           new_del_itvl = rx_max_del_itvl;
       } else {
           if (old_del_itvl < rx_del_itvl) {
               new_del_itvl = rx_del_itvl;
           } else if (old_del_itvl >= rx_del_itvl) {
               new_del_itvl = old_del_itvl;
           }
       }
   } else if ((old_max_del_itvl != 0 && rx_max_del_itvl == 0) ||
              (old_max_del_itvl != 0 && rx_max_del_itvl !=0 && rx_max_del_itvl > old_max_del_itvl))
   {
       new_max_del_itvl = old_max_del_itvl;
       if (rx_del_itvl > old_max_del_itvl) {
           new_del_itvl = old_max_del_itvl;
       } else {
           if (rx_del_itvl >= old_del_itvl) {
               new_del_itvl = rx_del_itvl;
           } else if (rx_del_itvl < old_del_itvl) {
               new_del_itvl = old_del_itvl;
           }
       }
   } else if (rx_max_del_itvl == old_max_del_itvl) {
       new_max_del_itvl = rx_max_del_itvl;
       if (old_del_itvl < rx_del_itvl) {
           new_del_itvl = rx_del_itvl;
       } else if (old_del_itvl >= rx_del_itvl) {
           new_del_itvl = old_del_itvl;
       }
   } else {
       IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM,
               "new_del_itvl/new_max_del_itvl computation failed\n");
       return -EINVAL;
   }

   IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "old %d.%d rx %d.%d new %d.%d\n",
           old_del_itvl, old_max_del_itvl, rx_del_itvl,
           rx_max_del_itvl, new_del_itvl, new_max_del_itvl);

   if (new_del_itvl != old_del_itvl) {
       IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "new_del_itvl %d old_del_itvl"
              " %d creating new counter\n", new_del_itvl, old_del_itvl);
       for (i = 0; i < IEEE80211_FMS_MAX_COUNTERS; i++) {
           if ((fms->counters[i].inuse == 1) &&
               (fms->counters[i].del_itvl == new_del_itvl))
           {
               /* Reusing the same counter i */
               IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Reusing counter id "
                       "- %d for del_itvl %d\n", i, fmsreq_subele->del_itvl);
               counter_id = i;
               break;
           }
       }

       if (i == IEEE80211_FMS_MAX_COUNTERS) {
           rv = ieee80211_fms_counter_create(fms, new_del_itvl, &counter_id);
           if (rv != 0) {
               IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "%s : line %d no "
                       "free counters available\n", __func__, __LINE__);
               return ENOMEM;
           }
           IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "New counter id %d for "
                   "del_itvl %d\n", counter_id, new_del_itvl);
           new_counter = &fms->counters[counter_id];
           new_counter->counter_id = counter_id;
           new_counter->del_itvl = new_del_itvl;
           new_counter->current_count = new_del_itvl;
       } else {
           new_counter = &fms->counters[counter_id];
       }

       fms_counter = &fms->counters[fms_stream_match->counter_id];
       TAILQ_REMOVE(&fms_counter->fms_stream_head, fms_stream_match, stream_next);

       if (TAILQ_EMPTY(&fms_counter->fms_stream_head)) {
           fms_counter->inuse = 0;
       }

       fms_stream_match->counter_id = counter_id;
       fms_stream_match->del_itvl = new_del_itvl;
       fms_stream_match->max_del_itvl = new_max_del_itvl;
       /* Moving the stream to new counter */
       TAILQ_INSERT_TAIL(&new_counter->fms_stream_head, fms_stream_match, stream_next);
       fmssubele_status->fms_counter = (new_counter->counter_id << 5 | new_counter->current_count);
       fmssubele_status->fmsid = fms_stream_match->fmsid;
   }

   if (new_max_del_itvl != old_max_del_itvl && new_del_itvl == old_del_itvl)  {
       unsol_fms_status = IEEE80211_WNM_FMSSUBELE_ALTE_MAX_DEL_ITVL_CHG;
   } else if (new_del_itvl != old_del_itvl && new_max_del_itvl == old_max_del_itvl) {
       unsol_fms_status = IEEE80211_WNM_FMSSUBELE_ALTE_DEL_ITVL_CHANGE;
   } else if (new_del_itvl != old_del_itvl && new_max_del_itvl != old_max_del_itvl) {
       unsol_fms_status = IEEE80211_WNM_FMSSUBELE_ALTE_MAX_DEL_ITVL_CHG;
   } else if (OS_MEMCMP(&fms_stream_match->rate_id, &fmsreq_subele->rate_id,
               sizeof(ieee80211_fms_rate_identifier_t))) {
       unsol_fms_status = IEEE80211_WNM_FMSSUBELE_ALTE_MCAST_RATE_CHANGE;
   } else {
       unsol_rsp_send = 0;
   }

   if (unsol_rsp_send == 1) {
       OS_MEMSET(&unsol_subele_status, 0, sizeof(ieee80211_fms_subelement_status_t));
       unsol_subele_status.element_status = unsol_fms_status;
       unsol_subele_status.del_itvl = new_del_itvl;
       unsol_subele_status.max_del_itvl = new_max_del_itvl;
       unsol_subele_status.fmsid = fms_stream_match->fmsid;
       OS_MEMCPY(unsol_subele_status.mcast_addr,fms_stream_match->mcast_addr, 6);
       OS_MEMCPY(&unsol_subele_status.rate_id, &fms_stream_match->rate_id,
               sizeof(ieee80211_fms_rate_identifier_t));
       ieee80211_send_unsol_fmsrsp(vap, ni, &unsol_subele_status);
   }

   if (new_max_del_itvl != rx_max_del_itvl) {
       fmssubele_status->element_status = IEEE80211_WNM_FMSSUBELE_ALTE_MAX_DEL_ITVL_CHG;
   } else if (new_del_itvl != rx_del_itvl) {
       fmssubele_status->element_status = IEEE80211_WNM_FMSSUBELE_ALTE_DEL_ITVL;
   } else if (new_del_itvl == rx_del_itvl) {
       fmssubele_status->element_status = IEEE80211_WNM_FMSSUBELE_ACCEPT;
   } else {
       fmssubele_status->element_status = IEEE80211_WNM_FMSSUBELE_DENY_UNSPECIFIED;
   }
   fmssubele_status->del_itvl = new_del_itvl;
   fmssubele_status->max_del_itvl = new_max_del_itvl;
   IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "*** Exiting function %s\n", __func__);

   return 0;
}

static int ieee80211_fmsreq_subelement_modify(wlan_if_t vap, wlan_node_t ni,
        void *head1, void *head2, void *head3)
{
    struct tclas_element *tclas, *tclas_elem_next;
    TAILQ_HEAD(, ieee80211_fmsreq_subele_s) *fmssubele_head;
    TAILQ_HEAD(, ieee80211_fms_status_subelement_s) *status_subele_head;
    TAILQ_HEAD(, ieee80211_fms_act_stream_ptr) *fmsact_strmptr_head;
    ieee80211_fmsreq_subele_t          *fmsreq_subele;
    ieee80211_fms_stream_t             *fms_stream=NULL, *fms_stream_match=NULL;
    ieee80211_fms_subelement_status_t  *fmssubele_status;
    ieee80211_fms_act_stream_ptr_t     *fmsstream_ptr = NULL;
    ieee80211_fms_status_subelement_t  *status_subele=NULL;
    ieee80211_fms_counter_t            *fms_counter = NULL, *new_counter;
    ieee80211_fms_t                    fms;
    int                                exact_match,stream_updated;
    int i, rv;
    int stream_deleted, counter_id;

    fmssubele_head = head1;
    status_subele_head = head2;
    fmsact_strmptr_head = head3;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " Enter Function %s\n", __func__);
    fms = vap->wnm->wnm_fms_data;

    TAILQ_INIT(status_subele_head);

    TAILQ_FOREACH(fmsreq_subele, fmssubele_head, fmssubele_next) {
        stream_updated = 0;
        stream_deleted = 0;
        exact_match = 0;

        /* If it is not new FMS request */
        TAILQ_FOREACH(fmsstream_ptr, fmsact_strmptr_head, strm_ptr_next) {
            fms_stream_match = NULL;
            fms_stream = fmsstream_ptr->stream;

            if (ieee80211_fms_subelement_match(fms_stream, fmsreq_subele)) {
                /*
                 * Received old FMS request with same FMS stream from the same
                 * client do not increment num_sta counter
                 */
                fms_stream_match = fms_stream;
                exact_match = 1;
                break;
            } else if (ieee80211_fms_subelement_tclass_match(&fms_stream->tclas_head,
                        &fmsreq_subele->tclas_head))
            {
                fms_stream_match = fms_stream;

                if (fmsreq_subele->del_itvl == 0) {
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Delivery interval "
                            "is zero - deleting stream from node\n");
                    stream_deleted = 1;
                    TAILQ_REMOVE(fmsact_strmptr_head, fmsstream_ptr, strm_ptr_next);
                    OS_FREE(fmsstream_ptr);

                    fms_stream->num_sta--;

                    if (fms_stream->num_sta == 0) {
                        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Deleting stream "
                                "from the database\n");
                        TAILQ_FOREACH_SAFE(tclas, &fms_stream->tclas_head, tclas_next, tclas_elem_next) {
                            TAILQ_REMOVE(&fms_stream->tclas_head, tclas, tclas_next);
                            OS_FREE(tclas);
                        }
                        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "%s: freeing "
                                "counter %d\n", __func__, fms_stream->counter_id);
                        ieee80211_fms_counter_free(fms, fms_stream->counter_id);
                        fms_counter = &fms->counters[fms_stream->counter_id];
                        TAILQ_REMOVE(&fms_counter->fms_stream_head, fms_stream, stream_next);
                        OS_FREE(fms_stream);
                    }
                    break;
                } else {
                    stream_updated = 1;
                    break;
                }
            }
        }

        if (fms_stream_match == NULL) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Received new FMS stream "
                    "in the old FMS request\n");
            continue;
        }

        if (stream_deleted == 1) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Old stream deleted - "
                    "continue processing next subelement\n");
            continue;
        }

        status_subele = OS_MALLOC(vap->iv_ic->ic_osdev,
                sizeof(ieee80211_fms_status_subelement_t), 0);
        if (status_subele == NULL) {
            qdf_nofl_info("%s:%d OS_MALLOC failed!\n", __func__, __LINE__);
            return ENOMEM;
        }

        fmssubele_status = OS_MALLOC(vap->iv_ic->ic_osdev,
                sizeof(ieee80211_fms_subelement_status_t), 0);
        if (fmssubele_status == NULL) {
            qdf_nofl_info("%s:%d OS_MALLOC failed!\n", __func__, __LINE__);
            OS_FREE(status_subele);
            return ENOMEM;
        }

        OS_MEMSET(fmssubele_status, 0, sizeof(ieee80211_fms_subelement_status_t));
        OS_MEMSET(status_subele, 0, sizeof(ieee80211_fms_subelement_status_t));

        if (exact_match == 1 && fms_stream != NULL) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Found exact match\n");
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "fms_stream->num_sta %d\n",
                    fms_stream->num_sta);
            fms_counter = &fms->counters[fms_stream->counter_id];
            fmssubele_status->element_status = IEEE80211_WNM_FMSSUBELE_ACCEPT;
            fmssubele_status->del_itvl = fms_stream->del_itvl;
            fmssubele_status->max_del_itvl = fms_stream->max_del_itvl;
            fmssubele_status->fmsid = fms_stream->fmsid;
            fmssubele_status->fms_counter = (fms_stream->counter_id << 5 | fms_counter->current_count);
            memcpy(&fmssubele_status->rate_id, &fms_stream->rate_id,
                    sizeof(ieee80211_fms_rate_identifier_t));
            memcpy(fmssubele_status->mcast_addr, fms_stream->mcast_addr, 6);
        } else if (stream_updated == 1 && fms_stream_match != NULL) {
            if (fms_stream_match->num_sta == 1) {
                /*
                 * If this is the only station uses the stream
                 * move the stream to other counter.
                 */
                fms_counter = &fms->counters[fms_stream_match->counter_id];
                TAILQ_REMOVE(&fms_counter->fms_stream_head, fms_stream_match, stream_next);

                if (TAILQ_EMPTY(&fms_counter->fms_stream_head)) {
                    fms_counter->inuse = 0;
                }

                for (i = 0; i < IEEE80211_FMS_MAX_COUNTERS; i++)
                {
                    if ((fms->counters[i].inuse == 1) &&
                        (fms->counters[i].del_itvl == fmsreq_subele->del_itvl))
                    {
                        /* Reusing the same counter i */
                        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Reusing counter "
                                "id - %d for del_itvl %d\n", i, fmsreq_subele->del_itvl);
                        counter_id = i;
                        break;
                    }
                }
                if (i == IEEE80211_FMS_MAX_COUNTERS) {
                    rv = ieee80211_fms_counter_create(fms, fmsreq_subele->del_itvl, &counter_id);
                    if (rv != 0) {
                        OS_FREE(fmssubele_status);
                        OS_FREE(status_subele);
                        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " %s : line %d "
                                "no free counters available\n", __func__, __LINE__);
                        return ENOMEM;
                    }
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "New counter id %d "
                            "for del_itvl %d\n", counter_id, fmsreq_subele->del_itvl);
                    new_counter = &fms->counters[counter_id];
                    new_counter->counter_id = counter_id;
                    new_counter->del_itvl = fmsreq_subele->del_itvl;
                    new_counter->current_count = fmsreq_subele->del_itvl;
                } else {
                    new_counter = &fms->counters[counter_id];
                }

                fms_stream_match->counter_id = counter_id;
                fms_stream_match->del_itvl = fmsreq_subele->del_itvl;
                fms_stream_match->max_del_itvl = fmsreq_subele->max_del_itvl;
                OS_MEMCPY(&fms_stream_match->rate_id, &fmsreq_subele->rate_id,
                        sizeof(ieee80211_fms_rate_identifier_t));
                OS_MEMCPY(fms_stream_match->mcast_addr, fmsreq_subele->mcast_addr, 6);

                /* Moving the stream to new counter */
                TAILQ_INSERT_TAIL(&new_counter->fms_stream_head, fms_stream_match, stream_next);
                fmssubele_status->del_itvl = fmsreq_subele->del_itvl;
                fmssubele_status->max_del_itvl = fmsreq_subele->max_del_itvl;
                fmssubele_status->fms_counter = new_counter->counter_id << 5 |
                                                new_counter->current_count;
                fmssubele_status->fmsid = fms_stream_match->fmsid;
                fmssubele_status->element_status = IEEE80211_WNM_FMSSUBELE_ACCEPT;
            } else {
                rv = ieee80211_fms_stream_modify(vap, ni, fms_stream_match,
                                                 fmsreq_subele, fmssubele_status);
                /*
                 * Remove the stream ptr from the node if it fails to modify
                 * the existing stream due to conflicts station will send new
                 * request with correct params.
                */
                if (fmssubele_status->element_status != IEEE80211_WNM_FMSSUBELE_ACCEPT) {
                    fms_stream_match->num_sta--;
                    TAILQ_REMOVE(fmsact_strmptr_head, fmsstream_ptr, strm_ptr_next);
                    OS_FREE(fmsstream_ptr);
                }
            }
            OS_MEMCPY(&fmssubele_status->rate_id, &fmsreq_subele->rate_id,
                    sizeof(ieee80211_fms_rate_identifier_t));
            OS_MEMCPY(fmssubele_status->mcast_addr, fmsreq_subele->mcast_addr, 6);
        } else {
            OS_FREE(fmssubele_status);
            OS_FREE(status_subele);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Invalid FMS request\n");
            return -EINVAL;
        }

        status_subele->subelementid = IEEE80211_FMS_STATUS_SUBELE;
        status_subele->subele_status = (void *)fmssubele_status;
        TAILQ_INSERT_TAIL(status_subele_head, status_subele, status_subele_next);
    }

    return 0;
}

static int ieee80211_fmsreq_subelement_process(wlan_if_t vap, wlan_node_t ni,
        void *head1, void *head2, void *head3)
{
    TAILQ_HEAD(, ieee80211_fmsreq_subele_s) *fmssubele_head;
    TAILQ_HEAD(, ieee80211_fms_status_subelement_s) *status_subele_head;
    TAILQ_HEAD(, ieee80211_fms_act_stream_ptr) *fmsact_strmptr_head;
    ieee80211_fmsreq_subele_t          *fmsreq_subele;
    ieee80211_fms_stream_t             *fms_stream=NULL;
    ieee80211_fms_stream_t             *fms_stream_match=NULL;
    ieee80211_fms_subelement_status_t  *fmssubele_status;
    ieee80211_fms_act_stream_ptr_t     *fmsstream_ptr = NULL;
    ieee80211_fms_status_subelement_t  *status_subele;
    ieee80211_fms_counter_t            *fms_counter = NULL;
    ieee80211_fms_t                    fms;
    int                                exact_match,num_matches, partial_match;
    int                                cid, rv, old_del_itvl;

    fmssubele_head = head1;
    status_subele_head = head2;
    fmsact_strmptr_head = head3;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " Enter Function %s\n", __func__);
    fms = vap->wnm->wnm_fms_data;

    TAILQ_INIT(status_subele_head);

    TAILQ_FOREACH(fmsreq_subele, fmssubele_head, fmssubele_next) {
        exact_match = 0;
        partial_match = 0;
        num_matches = 0;
        old_del_itvl = 0;

        status_subele = OS_MALLOC(vap->iv_ic->ic_osdev,
                sizeof(ieee80211_fms_status_subelement_t), 0);
        if (status_subele == NULL) {
            qdf_nofl_info("%s:%d OS_MALLOC failed!\n", __func__, __LINE__);
            return ENOMEM;
        }

        fmssubele_status = OS_MALLOC(vap->iv_ic->ic_osdev,
                sizeof(ieee80211_fms_subelement_status_t), 0);
        if (fmssubele_status == NULL) {
            qdf_nofl_info("%s:%d OS_MALLOC failed!\n", __func__, __LINE__);
            OS_FREE(status_subele);
            return ENOMEM;
        }

        OS_MEMSET(fmssubele_status, 0, sizeof(ieee80211_fms_subelement_status_t));
        OS_MEMSET(status_subele, 0, sizeof(ieee80211_fms_subelement_status_t));

        /* Search the global stream list when received a new FMS request */
        for (cid = 0; cid < IEEE80211_FMS_MAX_COUNTERS; cid++) {
            fms_counter = &fms->counters[cid];
            if (fms_counter->inuse == 1) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "fms_counter id %d is in use\n", cid);

                TAILQ_FOREACH(fms_stream, &fms_counter->fms_stream_head, stream_next) {
                    if (ieee80211_fms_subelement_match(fms_stream, fmsreq_subele)) {
                        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Found exact match\n");
                        exact_match = 1;
                        break;
                    }

                    if (ieee80211_fms_subelement_tclass_match(&fms_stream->tclas_head,
                                &fmsreq_subele->tclas_head))
                    {
                        num_matches++;
                        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "num_matches %d\n", num_matches);
                        if (num_matches > 1) {
                            /*
                             * Deny FMS stream with all tclass classifiers
                             * matching two streams with different delivery
                             * intervals.
                             */
                            if (old_del_itvl != fms_stream->del_itvl) {
                                IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM,
                                        "FMS stream matches two streams with "
                                        "different del_itvls\n");
                                break;
                            }
                        }
                        old_del_itvl = fms_stream->del_itvl;

                        if ((fms_stream->num_tclas != fmsreq_subele->num_tclas) &&
                            (fms_stream->del_itvl == fmsreq_subele->del_itvl))
                        {
                            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Partial match\n");
                            partial_match = 1;
                        }
                        fms_stream_match = fms_stream;
                    }
                }
            }
            if (exact_match || num_matches) {
                break;
            }
        }

        if (exact_match == 1 && fms_counter != NULL && fms_stream != NULL) {
            fmssubele_status->element_status = IEEE80211_WNM_FMSSUBELE_ACCEPT;
            fmssubele_status->del_itvl = fms_stream->del_itvl;
            fmssubele_status->max_del_itvl = fms_stream->max_del_itvl;
            fmssubele_status->fmsid = fms_stream->fmsid;
            fmssubele_status->fms_counter = fms_counter->counter_id << 5 |
                                            fms_counter->current_count;
            memcpy(&fmssubele_status->rate_id, &fms_stream->rate_id,
                    sizeof(ieee80211_fms_rate_identifier_t));
            memcpy(fmssubele_status->mcast_addr, fms_stream->mcast_addr, 6);
        } else if (num_matches > 1) {
            /*
             * Deny FMS stream with all tclass classifiers matching
             * two streams with different delivery intervals.
             */
            fmssubele_status->element_status = IEEE80211_WNM_FMSSUBELE_DENY_CLASSIFIER;
            fmssubele_status->del_itvl = fmsreq_subele->del_itvl;
            fmssubele_status->max_del_itvl = fmsreq_subele->max_del_itvl;
            fmssubele_status->fmsid = 0;
            fmssubele_status->fms_counter = 0;
            memcpy(&fmssubele_status->rate_id, &fmsreq_subele->rate_id,
                    sizeof(ieee80211_fms_rate_identifier_t));
            memcpy(fmssubele_status->mcast_addr, fmsreq_subele->mcast_addr, 6);
        } else if (num_matches == 1 && fms_stream_match != NULL) {
            rv = ieee80211_fms_stream_modify(vap, ni, fms_stream_match,
                    fmsreq_subele, fmssubele_status);
            fms_stream = fms_stream_match;
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Creating new FMS stream\n");
            fms_stream = ieee80211_fms_stream_add(vap, ni, fmsreq_subele, fmssubele_status);
            if (fms_stream == NULL) {
                fmssubele_status->element_status = IEEE80211_WNM_FMSSUBELE_TERM_LACK_RESOURCES;
            } else {
                fmssubele_status->element_status = IEEE80211_WNM_FMSSUBELE_ACCEPT;
            }

            fmssubele_status->del_itvl = fmsreq_subele->del_itvl;
            fmssubele_status->max_del_itvl = fmsreq_subele->max_del_itvl;
            fmssubele_status->rate_id.mask = fmsreq_subele->rate_id.mask;
            fmssubele_status->rate_id.mcs_idx = fmsreq_subele->rate_id.mcs_idx;
            fmssubele_status->rate_id.rate = fmsreq_subele->rate_id.rate;
            memcpy(fmssubele_status->mcast_addr, fmsreq_subele->mcast_addr, 6);
        }

        if (fmssubele_status->element_status == IEEE80211_WNM_FMSSUBELE_ACCEPT) {
            fms_stream->num_sta++;
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Number of clients for "
                    "stream %d = %d\n", fms_stream->fmsid, fms_stream->num_sta);
            fmsstream_ptr = OS_MALLOC(vap->iv_ic->ic_osdev,
                    sizeof(ieee80211_fms_act_stream_ptr_t), 0);
            if (fmsstream_ptr == NULL) {
                qdf_nofl_info("%s:%d OS_MALLOC failed!\n", __func__, __LINE__);
                OS_FREE(status_subele);
                OS_FREE(fmssubele_status);
                return ENOMEM;
            }

            fmsstream_ptr->stream = fms_stream;
            TAILQ_INSERT_TAIL(fmsact_strmptr_head, fmsstream_ptr, strm_ptr_next);
        }

        status_subele->subelementid = IEEE80211_FMS_STATUS_SUBELE;
        status_subele->subele_status = (void *)fmssubele_status;
        TAILQ_INSERT_TAIL(status_subele_head, status_subele, status_subele_next);
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " Exit Function %s\n", __func__);
    return 0;
}

static int ieee80211_recv_fmsreq_process(wlan_if_t vap, wlan_node_t ni,
                                  struct ieee80211_fmsreq *fmsreq,
                                  struct ieee80211_fmsrsp *fmsrsp)
{
    ieee80211_fms_response_t *fmsrsp_ie;
    ieee80211_fms_request_t *fmsreq_ie;
    ieee80211_fmsreq_active_element_t *fmsact_ie, *fmsact_ie_next;
    ieee80211_fms_t fms;
    int rv = 0;

    fms = vap->wnm->wnm_fms_data;

    TAILQ_INIT(&fmsrsp->fmsrsp_head);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " Enter Function %s\n", __func__);

    TAILQ_FOREACH(fmsreq_ie, &fmsreq->fmsreq_head, fmsreq_next) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Processing req element %pK\n", fmsreq_ie);
        fmsrsp_ie = OS_MALLOC(vap->iv_ic->ic_osdev, (sizeof(ieee80211_fms_response_t)), 0);
        if (fmsrsp_ie == NULL) {
            qdf_nofl_info("%s:%d OS_MALLOC failed!\n", __func__, __LINE__);
            ieee80211_wnm_free_fmsrsp(&fmsrsp->fmsrsp_head);
            return ENOMEM;
        }

        if (fmsreq_ie->fms_token == 0) {
            fmsact_ie = OS_MALLOC(vap->iv_ic->ic_osdev,
                    sizeof(ieee80211_fmsreq_active_element_t), 0);
            if (fmsact_ie == NULL) {
                OS_FREE(fmsrsp_ie);
                ieee80211_wnm_free_fmsrsp(&fmsrsp->fmsrsp_head);
                qdf_nofl_info("%s:%d OS_MALLOC failed!\n", __func__, __LINE__);
                return ENOMEM;
            }

            TAILQ_INIT(&fmsact_ie->fmsact_strmptr_head);
            rv = ieee80211_fmsreq_subelement_process(vap, ni,
                    &fmsreq_ie->fmssubele_head,
                    &fmsrsp_ie->status_subele_head,
                    &fmsact_ie->fmsact_strmptr_head);

            /*
             * Assign the unique fms_token if at least one
             * fms subelement is accepted.
             */
            if (!TAILQ_EMPTY(&fmsact_ie->fmsact_strmptr_head)) {
                fmsrsp_ie->fms_token = ++fms->fms_token;
                fmsact_ie->fms_token = fmsrsp_ie->fms_token;
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Adding token %d "
                        "fmsact_ie %pK to ni %pK\n", fmsact_ie->fms_token,fmsact_ie, ni);
                TAILQ_INSERT_TAIL(&ni->ni_wnm->fmsreq_act_head, fmsact_ie, fmsreq_act_next);
            } else {
                OS_FREE(fmsact_ie);
            }
        } else {
            TAILQ_FOREACH_SAFE(fmsact_ie, &ni->ni_wnm->fmsreq_act_head, fmsreq_act_next, fmsact_ie_next) {
                 IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "received token %d "
                         "saved %d fmsact_ie %pK ni %pK\n",
                         fmsreq_ie->fms_token, fmsact_ie->fms_token, fmsact_ie, ni);
                 if (fmsact_ie->fms_token == fmsreq_ie->fms_token) {
                     rv = ieee80211_fmsreq_subelement_modify(vap, ni,
                             &fmsreq_ie->fmssubele_head,
                             &fmsrsp_ie->status_subele_head,
                             &fmsact_ie->fmsact_strmptr_head);
                     if (TAILQ_EMPTY(&fmsact_ie->fmsact_strmptr_head)) {
                         TAILQ_REMOVE(&ni->ni_wnm->fmsreq_act_head, fmsact_ie, fmsreq_act_next);
                         OS_FREE(fmsact_ie);
                     } else {
                         fmsrsp_ie->fms_token = fmsact_ie->fms_token;
                     }
                 }
             }
        }

        if (rv != 0) {
            OS_FREE(fmsrsp_ie);
            ieee80211_wnm_free_fmsrsp(&fmsrsp->fmsrsp_head);
            qdf_nofl_info("%s:%d subelement processing failed!\n", __func__, __LINE__);
            return ENOMEM;
        }
        if (!TAILQ_EMPTY(&fmsrsp_ie->status_subele_head)) {
            TAILQ_INSERT_TAIL(&fmsrsp->fmsrsp_head, fmsrsp_ie, fmsrsp_next);
        } else {
            OS_FREE(fmsrsp_ie);
        }
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " Exit Function %s\n", __func__);
    return rv;
}

static int ieee80211_send_fms_rsp(wlan_if_t vap, wlan_node_t ni)
{
    wbuf_t wbuf;
    u_int8_t *frm;
    u_int8_t *len;
    struct ieee80211_fmsrsp *fmsrsp;
    ieee80211_fms_response_t *fmsrsp_ele;
    ieee80211_fms_status_subelement_t *status_subele;
    u_int16_t rate;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " Enter Function %s\n", __func__);
    fmsrsp = ni->ni_wnm->fmsrsp;

    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm, 0);
    if (wbuf == NULL) {
        return -ENOMEM;
    }

    /* Check if PMF is enabled and set security bit */
    if (wlan_crypto_is_pmf_enabled(vap->vdev_obj, ni->peer_obj) &&
        ieee80211_node_is_authorized(ni)) {
        struct ieee80211_frame *wh;
        wh = (struct ieee80211_frame*)wbuf_header(wbuf);
        wh->i_fc[1] |= IEEE80211_FC1_WEP;
    }

    *frm++ = IEEE80211_ACTION_CAT_WNM;
    *frm++ = IEEE80211_ACTION_FMS_RESP;
    *frm++ = ni->ni_wnm->fmsreq->dialog_token;

    TAILQ_FOREACH(fmsrsp_ele, &fmsrsp->fmsrsp_head,  fmsrsp_next) {
        *frm++ = IEEE80211_ELEMID_FMS_RESPONSE;
        len = frm;
        *len = 1;
        frm++;
        *frm++ = fmsrsp_ele->fms_token;

        TAILQ_FOREACH(status_subele, &fmsrsp_ele->status_subele_head, status_subele_next) {
            if (status_subele->subelementid == IEEE80211_FMS_STATUS_SUBELE) {
                ieee80211_fms_subelement_status_t *fmssubele_status;

                fmssubele_status = status_subele->subele_status;
                *frm++ = IEEE80211_FMS_STATUS_SUBELE;
                *frm++ = 15;
                *frm++ = fmssubele_status->element_status;
                *frm++ = fmssubele_status->del_itvl;
                *frm++ = fmssubele_status->max_del_itvl;
                *frm++ = fmssubele_status->fmsid;
                *frm++ = fmssubele_status->fms_counter;
                *frm++ = fmssubele_status->rate_id.mask;
                *frm++ = fmssubele_status->rate_id.mcs_idx;
                rate  = htobe16(fmssubele_status->rate_id.rate);
                OS_MEMCPY(frm, &rate, 2);
                frm += 2;
                OS_MEMCPY(frm, &fmssubele_status->mcast_addr, 6);
                frm += 6;
                *len += 17;
            } else if (status_subele->subelementid == IEEE80211_TCLASS_STATUS_SUBELE) {
                struct tclas_element *tclass_element;
                ieee80211_tclass_subelement_status_t *tclas_status;
                u_int8_t *tclas_len;

                tclas_status = status_subele->subele_status;
                *frm++ = IEEE80211_TCLASS_STATUS_SUBELE;
                tclas_len = frm;
                *tclas_len = 1;
                frm += 1;
                *frm++ = tclas_status->fmsid;

                TAILQ_FOREACH(tclass_element, &tclas_status->tclas_head, tclas_next) {
                    *frm++ = tclass_element->elemid;
                    *frm++ = tclass_element->len;
                    *frm++ = tclass_element->up;
                    *frm++ = tclass_element->type;
                    *frm++ = tclass_element->mask;
                    *tclas_len += 5;

                    if (tclass_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE3) {
                        *(u_int16_t *)frm++ = tclass_element->tclas.type3.filter_offset;
                        *tclas_len += 2;
                        memcpy(frm, tclass_element->tclas.type3.filter_value,
                                tclass_element->tclas.type3.filter_len);
                        frm += tclass_element->tclas.type3.filter_len;
                        *tclas_len += tclass_element->tclas.type3.filter_len;
                        memcpy(frm, tclass_element->tclas.type3.filter_mask,
                                tclass_element->tclas.type3.filter_len);
                        frm += tclass_element->tclas.type3.filter_len;
                        *tclas_len += tclass_element->tclas.type3.filter_len;
                    }

                    if (tclass_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE1 ||
                        tclass_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4)
                    {
                        *frm++ = tclass_element->tclas.type14.type14_v4.version;
                        if (tclass_element->tclas.type14.type14_v4.version ==
                            IEEE80211_WNM_TCLAS_CLAS14_VERSION_4)
                        {
                            memcpy(frm, tclass_element->tclas.type14.type14_v4.src_ip,
                                    IEEE80211_IPV4_LEN);
                            frm += IEEE80211_IPV4_LEN;
                            memcpy(frm, tclass_element->tclas.type14.type14_v4.dst_ip,
                                    IEEE80211_IPV4_LEN);
                            frm += IEEE80211_IPV4_LEN;
                            memcpy(frm, &tclass_element->tclas.type14.type14_v4.src_port, 2);
                            frm += 2;
                            memcpy(frm, &tclass_element->tclas.type14.type14_v4.dst_port, 2);
                            frm += 2;
                            *frm++= tclass_element->tclas.type14.type14_v4.dscp;
                            *frm++= tclass_element->tclas.type14.type14_v4.protocol;
                            *frm++= 0; /* Reserved byte */
                            *tclas_len += 15;
                        } else if (tclass_element->tclas.type14.type14_v6.version ==
                                   IEEE80211_WNM_TCLAS_CLAS14_VERSION_6)
                        {
                            memcpy(frm, tclass_element->tclas.type14.type14_v6.src_ip,
                                    IEEE80211_IPV6_LEN);
                            frm += IEEE80211_IPV6_LEN;
                            memcpy(frm, tclass_element->tclas.type14.type14_v6.dst_ip,
                                    IEEE80211_IPV6_LEN);
                            frm += IEEE80211_IPV6_LEN;
                            memcpy(frm, &tclass_element->tclas.type14.type14_v4.src_port, 2);
                            frm += 2;
                            memcpy(frm, &tclass_element->tclas.type14.type14_v4.dst_port, 2);
                            frm += 2;
                            if (tclass_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4) {
                                *frm++= tclass_element->tclas.type14.type14_v6.type4_dscp;
                                *frm++= tclass_element->tclas.type14.type14_v6.type4_next_header;
                            }
                            memcpy(frm, tclass_element->tclas.type14.type14_v6.flow_label, 3);
                            frm += 3;
                            *tclas_len += 41;
                        }
                    }
                }
                *len += *tclas_len;
                *len += 2;
            }
        }
    }
    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " Exit Function %s\n", __func__);

    return ieee80211_send_mgmt(vap, ni, wbuf, false);
}

int ieee80211_recv_wnm_req(wlan_if_t vap, wlan_node_t ni, u_int8_t *frm, int frm_len)
{
    struct ieee80211_wnmreq wnmreq;

    frm = frm + 2;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MBO, " Enter Function %s\n", __func__);
    wnmreq.dialog_token = *frm++;
    wnmreq.type = *frm++;
    if (wnmreq.type == IEEE80211_ELEMID_VENDOR){
        wnmreq.elem_id = *(frm);
        if(IEEE80211_ELEMID_VENDOR == wnmreq.elem_id) {
            ieee80211_parse_wnm_mbo_subelem(frm,ni);
        }
    }
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MBO, " Exit Function %s\n", __func__);
    return 0;
}

int ieee80211_recv_fms_req(wlan_if_t vap, wlan_node_t ni, u_int8_t *frm, int frm_len)
{
    u_int8_t *sfrm;
    ieee80211_fms_request_t *fmsreq_ie;
    struct ieee80211_fmsreq *fmsreq;
    struct ieee80211_fmsrsp *fmsrsp;
    int rv;
    u_int8_t subelem_len;

    fmsreq = ni->ni_wnm->fmsreq;
    fmsrsp = ni->ni_wnm->fmsrsp;

    TAILQ_INIT(&fmsreq->fmsreq_head);

    sfrm = frm;
    frm = frm + 2;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " Enter Function %s\n", __func__);

    fmsreq->dialog_token = *frm++;

    while ((frm - sfrm) < frm_len) {
        fmsreq_ie = OS_MALLOC(vap->iv_ic->ic_osdev, sizeof(ieee80211_fms_request_t), 0);
        if (fmsreq_ie == NULL) {
            qdf_nofl_info("%s:%d OS_MALLOC failed!\n", __func__, __LINE__);
            ieee80211_wnm_free_fmsreq(&fmsreq->fmsreq_head);
            return ENOMEM;
        }

        fmsreq_ie->elem_id = *frm++;

        if (IEEE80211_ELEMID_FMS_REQUEST == fmsreq_ie->elem_id) {
            subelem_len = *frm++;
            fmsreq_ie->fms_token = *frm++;

            if (subelem_len > 0) {
                rv = ieee80211_recv_fmssubele_parse(vap, ni,
                        &fmsreq_ie->fmssubele_head, frm, subelem_len);
                if (rv) {
                    OS_FREE(fmsreq_ie);
                    ieee80211_wnm_free_fmsreq(&fmsreq->fmsreq_head);
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM,
                                      "function %s : %d ieee80211_recv_fmssubele_parse failed !\n",
                                      __func__, __LINE__);
                    return ENOMEM;
                }
            }

            frm = frm + subelem_len - 1;
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Adding req element %pK\n", fmsreq_ie);
            TAILQ_INSERT_TAIL(&fmsreq->fmsreq_head, fmsreq_ie, fmsreq_next);
        } else {
           OS_FREE(fmsreq_ie);
           ieee80211_wnm_free_fmsreq(&fmsreq->fmsreq_head);
           qdf_nofl_info("%s:%d unknown request type\n", __func__, __LINE__);
           return -EINVAL;
        }
    }

    rv = ieee80211_recv_fmsreq_process(vap, ni, fmsreq, fmsrsp);
    if (rv) {
        ieee80211_wnm_free_fmsreq(&fmsreq->fmsreq_head);
        qdf_nofl_info("%s:%d ieee80211_recv_fmsreq_process failed!\n", __func__, __LINE__);
        return rv;
    }

    ieee80211_wnm_free_fmsreq(&fmsreq->fmsreq_head);
    if (!TAILQ_EMPTY(&fmsrsp->fmsrsp_head)) {
        rv = ieee80211_send_fms_rsp(vap, ni);
    }
    ieee80211_wnm_free_fmsrsp(&fmsrsp->fmsrsp_head);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " Exiting Function %s\n", __func__);

    return rv;
}

void ieee80211_fms_filter(struct ieee80211_node *ni, wbuf_t wbuf,
        int ether_type, struct ip_header *ip,int hdrsize)
{
    ieee80211_fms_counter_t *fms_counter;
    ieee80211_fms_stream_t *fms_stream;
    ieee80211_fms_t fms;
    int filter_len, i;
    unsigned char *bp;
    u_int32_t saddr, daddr;
    int tclas_count = 0;
    int tclas_match = 0;
    int filter_match = 1;
    int status = 0;
    int buf_len = 0;
    struct tclas_element *tclas_element;
    u_int8_t protocol;
    u_int8_t dscp;
    u_int8_t sport_match;
    u_int8_t dport_match;
    u_int16_t sport;
    u_int16_t dport;
    u_int16_t *tcpudphdr;
    u_int16_t offset;

    fms = ni->ni_vap->wnm->wnm_fms_data;

    for (i = 0; i < IEEE80211_FMS_MAX_COUNTERS; i++) {
        fms_counter = &fms->counters[i];

        TAILQ_FOREACH(fms_stream, &fms_counter->fms_stream_head, stream_next) {
            tclas_count = 0;
            tclas_match = 0;

            if (fms_stream->del_itvl_changed == 1) {
                continue;
            }

            TAILQ_FOREACH(tclas_element, &fms_stream->tclas_head, tclas_next) {
                tclas_count++;

                if (tclas_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE3) {
                    offset = tclas_element->tclas.type3.filter_offset;
                    filter_len = (tclas_element->len - 5) / 2;
                    buf_len = wbuf_get_pktlen(wbuf);
                    if (buf_len < (filter_len)) {
                        filter_match = 0;
                        break;
                    }
                    bp = (u_int8_t *)(wbuf->data);
                    for (i = 0; i < filter_len; i++) {
                        if ((bp[offset + i] &
                             tclas_element->tclas.type3.filter_mask[i]) !=
							(tclas_element->tclas.type3.filter_value[i] &
							 tclas_element->tclas.type3.filter_mask[i]))
                        {
                            filter_match = 0;
                            break;
                        }
                    }
                    if (filter_match) {
                        if (fms_stream->tclasprocess.tclas_process == 1) {
                            tclas_match = tclas_count;
                            break;
                        }
  	                    tclas_match++;
                    }
                } else if (tclas_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE1 ||
                           tclas_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4)
                {
                    if ((ether_type == htons(ETHERTYPE_IP)) &&
                        (tclas_element->tclas.type14.type14_v4.version ==
                         IEEE80211_WNM_TCLAS_CLAS14_VERSION_4))
                    {
                        qdf_net_iphdr_t *hdr;

                        hdr = (qdf_net_iphdr_t *)ip;
                        sport = tclas_element->tclas.type14.type14_v4.src_port;
                        dport = tclas_element->tclas.type14.type14_v4.dst_port;
                        protocol = tclas_element->tclas.type14.type14_v4.protocol;
                        dscp = tclas_element->tclas.type14.type14_v4.dscp & ~INET_ECN_MASK;

                        memcpy(&saddr, tclas_element->tclas.type14.type14_v4.src_ip,
                                IEEE80211_IPV4_LEN);
                        memcpy(&daddr, tclas_element->tclas.type14.type14_v4.dst_ip,
                                IEEE80211_IPV4_LEN);

                        if ((hdr->ip_proto == protocol) &&
                            (saddr == hdr->ip_saddr) &&
                            (daddr == hdr->ip_daddr) &&
                            (dscp == (hdr->ip_tos & ~INET_ECN_MASK)))
                        {
                            tcpudphdr = (u_int16_t *)((u_int32_t *)hdr + hdr->ip_hl);
                            sport_match = 1;
                            if (sport && (sport != *tcpudphdr)) {
                                sport_match = 0;
                            }
                            dport_match = 1;
                            if (dport && (dport != *(tcpudphdr + 1))) {
                                dport_match = 0;
                            }

                            if (dport_match && sport_match) {
                                if (fms_stream->tclasprocess.tclas_process == 1) {
                                    tclas_match = tclas_count;
                                    break;
                                }
                                tclas_match++;
                            }
                        }
                    } else if ((ether_type == htons(ETHERTYPE_IPV6)) &&
                               (tclas_element->tclas.type14.type14_v6.version ==
                                IEEE80211_WNM_TCLAS_CLAS14_VERSION_6))
                    {
                        qdf_net_ipv6hdr_t *ipv6;
                        unsigned long ver_pri_flowlabel;
                        unsigned long pri;
                        unsigned long *hdr;
                        u_int8_t *src_ip;
                        u_int8_t *dst_ip;
                        u_int8_t *flowlbl;
                        u_int8_t tos;
                        u_int8_t nexthdr;

                        hdr = (unsigned long *)ip;
                        ipv6 = (qdf_net_ipv6hdr_t *)hdr;
                        ver_pri_flowlabel = *hdr;
                        sport = tclas_element->tclas.type14.type14_v4.src_port;
                        dport = tclas_element->tclas.type14.type14_v4.dst_port;
                        flowlbl = tclas_element->tclas.type14.type14_v6.flow_label;
                        if (tclas_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4) {
                            dscp = tclas_element->tclas.type14.type14_v6.type4_dscp & ~INET_ECN_MASK;
                            nexthdr = tclas_element->tclas.type14.type14_v6.type4_next_header;
                        }
                        pri = (ver_pri_flowlabel & IPV6_PRIORITY_MASK) >> IPV6_PRIORITY_SHIFT;
                        tos = (pri & ~INET_ECN_MASK) >> IP_PRI_SHIFT;
                        src_ip = tclas_element->tclas.type14.type14_v6.src_ip;
                        dst_ip = tclas_element->tclas.type14.type14_v6.dst_ip;

                        if ((tclas_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE1 ||
                            ((tclas_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4) &&
                             (ipv6->ipv6_nexthdr == nexthdr) && (tos == dscp))) &&
                            !OS_MEMCMP(ipv6->ipv6_flow_lbl, flowlbl, 3) &&
                            !OS_MEMCMP(src_ip, ipv6->ipv6_saddr.s6_addr, IEEE80211_IPV6_LEN) &&
                            !OS_MEMCMP(dst_ip, ipv6->ipv6_daddr.s6_addr, IEEE80211_IPV6_LEN))
                        {
                            tcpudphdr = (u_int16_t *)(ipv6 + 1);
                            sport_match = 1;
                            if (sport && (sport != *tcpudphdr)) {
                                sport_match = 0;
                            }

                            dport_match = 1;
                            if (dport && (dport != *(tcpudphdr + 1))) {
                                dport_match = 0;
                            }

                            if (dport_match && sport_match) {
                                if (fms_stream->tclasprocess.tclas_process == 1) {
                                    tclas_match = tclas_count;
                                    break;
                                }
                                tclas_match++;
                            }
                        }
                    } else {
                        tclas_match++;
                    }
                }
            }
            if (tclas_count == tclas_match) {
                 status = 1;
                 break;
            }
        }

        if (status) {
            wbuf_set_fmsstream(wbuf);
            wbuf_set_fmsqid(wbuf, fms_counter->counter_id);
            break;
        }
    }
}

/*
 * Creates the FMS descriptor to be sent out in the beacon.
 * On return, fmsie points to the desc or NULL, fmsie_len
 * points to len of desc.
 */
int ieee80211_wnm_setup_fmsdesc_ie(struct ieee80211_node *ni, int dtim, u_int8_t **fmsie,
                                   u_int8_t *fmsie_len, u_int32_t *fms_counter_mask)
{
    struct tclas_element *tclas, *tclas_elem_next;
    struct ieee80211vap *vap = ni->ni_vap;
    ieee80211_wnm_t wnm = vap->wnm;
    ieee80211_fms_t fms = vap->wnm->wnm_fms_data;
    ieee80211_fms_counter_t *fms_counter;
    ieee80211_fms_stream_t *fms_stream, *fms_stream_next;
    struct ieee80211_fms_desc_ie *fmsdesc;
    u_int8_t *fmsid;
    int fms_desc_len = 1; /* at least 1, if present */
    int i;
    int numcounters = 0;

    if (!ieee80211_vap_wnm_is_set(vap) || !wnm ||
        (vap->iv_opmode != IEEE80211_M_HOSTAP))
    {
        *fmsie = NULL;
        *fmsie_len = 0;
        return 0;
    }

    OS_MEMZERO(wnm->wnm_fms_desc_ie, sizeof(wnm->wnm_fms_desc_ie));
    fmsdesc = (struct ieee80211_fms_desc_ie *)wnm->wnm_fms_desc_ie;
    fmsid = (u_int8_t *)fmsdesc;
    fmsid += 2; /* elemid+len */
    OS_MEMZERO(fmsdesc, sizeof(wnm->wnm_fms_desc_ie));

    fmsdesc->id = IEEE80211_ELEMID_FMS_DESCRIPTOR;

    for (i = 0; i < IEEE80211_FMS_MAX_COUNTERS; i++) {
        fms_counter = &fms->counters[i];

        if (fms_counter->inuse == 1) {
            numcounters++;
            fmsdesc->fmscounters[i].id = fms_counter->counter_id;

            if (dtim)
                fms_counter->current_count--;

            fmsdesc->fmscounters[i].count = fms_counter->current_count;
            fms_desc_len++; /* id, count are in the same byte */
        }
    }

    fmsdesc->numcounters = numcounters;
    fmsid += fms_desc_len;

    for (i = 0; i < IEEE80211_FMS_MAX_COUNTERS; i++) {
        fms_counter = &fms->counters[i];

        if (fms_counter->inuse == 1 && fms_counter->current_count == 0) {
            /*
             * Append FMSIDs of the streams when associated counter becomes
             * zero and resetting fms counter to delivery interval.
             */
            *fms_counter_mask |= (1 << i);

            TAILQ_FOREACH_SAFE(fms_stream, &fms_counter->fms_stream_head, stream_next, fms_stream_next) {
                *fmsid = fms_stream->fmsid;
                fmsid++;
                fms_desc_len++;

                if (fms_stream->del_itvl_changed == 1) {
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "%s : Deleting "
                            "stream %d and free counter %d\n",
                            __func__, fms_stream->fmsid, fms_stream->counter_id);
                    ieee80211_fms_counter_free(fms, fms_stream->counter_id);
                    TAILQ_FOREACH_SAFE(tclas, &fms_stream->tclas_head, tclas_next, tclas_elem_next) {
                        TAILQ_REMOVE(&fms_stream->tclas_head, tclas, tclas_next);
                        OS_FREE(tclas);
                    }

                    TAILQ_REMOVE(&fms_counter->fms_stream_head, fms_stream, stream_next);
                    OS_FREE(fms_stream);
                }
            }

            fms_counter->current_count = fms_counter->del_itvl;
        }
    }

    fmsdesc->len = fms_desc_len;
    *fmsie = (u_int8_t*)fmsdesc;
    *fmsie_len = (fms_desc_len + 2); /* plus elem id and len */

    return 0;
}

#ifdef WLAN_SUPPORT_SCS

#define IEEE80211_TCLAS10_ESP_FILTER_LEN             4
#define IEEE80211_TCLAS10_UDP_ENCAP_IPSEC_FILTER_LEN 12

/* SCS Procedure related functionalities */

/* Check if SCSID is valid - This function will check
 * if the given SCSID is present in the AP database
 * If present, then it will return the index of the entry
 * If absent, then it will be return a fixed incorrect value,
 * with which the caller will come to know if the entry is
 * not there.
 * @param ni - Node data structure
 * @param id - ID to be checked
 * return uint8_t  - index of the entry
 */
uint8_t
ieee80211_check_for_existing_scsid(struct ieee80211_node *ni,
        uint8_t scs_id)
{
    uint8_t idx = 0;
    while (idx < ni->ni_scs_sessions && idx < IEEE80211_SCS_MAX_SIZE) {
        if(scs_id == ni->ni_scs[idx]->scsid)
            return idx;
        idx++;
    }

    return (IEEE80211_SCS_MAX_SIZE + 1);
}

/* Copy TCLAS type 4 version 4 parameters - This function
 * copies the TCLAS parameters with Type 4 and having IPv4
 * parameters to SCS descriptor.
 * @param scs_data - SCS descriptor pointer
 * @param tclas_element - TCLAS linked list element
 * @param tclas_idx - TCLAS entry number for given SCS entry number.
 */
void
ieee80211_tclas_copy_type4_v4_params(struct ieee80211_scs_data *scs_data,
        struct tclas_element *tclas, uint8_t tclas_idx)
{
    qdf_mem_copy(scs_data->tclas_tuple[tclas_idx].tclas.type4.src_ip.ipv4,
                tclas->tclas.type14.type14_v4.src_ip,4);
    qdf_mem_copy(scs_data->tclas_tuple[tclas_idx].tclas.type4.dst_ip.ipv4,
                tclas->tclas.type14.type14_v4.dst_ip, 4);
    scs_data->tclas_tuple[tclas_idx].tclas.type4.src_port =
                tclas->tclas.type14.type14_v4.src_port;
    scs_data->tclas_tuple[tclas_idx].tclas.type4.dst_port =
                tclas->tclas.type14.type14_v4.dst_port;
    scs_data->tclas_tuple[tclas_idx].tclas.type4.dscp =
                tclas->tclas.type14.type14_v4.dscp;
    scs_data->tclas_tuple[tclas_idx].tclas.type4.next_header =
                tclas->tclas.type14.type14_v4.protocol;
}

/* Copy TCLAS type 4 version 6 parameters - This function
 * copies the TCLAS parameters with Type 4 and having IPv6
 * parameters to SCS descriptor.
 * @param scs_data - SCS descriptor pointer
 * @param tclas_element - TCLAS linked list element
 * @param tclas_idx - TCLAS entry number for given SCS entry number.
 */
void ieee80211_tclas_copy_type4_v6_params(struct ieee80211_scs_data *scs_data,
        struct tclas_element *tclas, uint8_t tclas_idx)
{
    qdf_mem_copy(scs_data->tclas_tuple[tclas_idx].tclas.type4.src_ip.ipv6,
                tclas->tclas.type14.type14_v6.src_ip, IEEE80211_IPV6_LEN);
    qdf_mem_copy(scs_data->tclas_tuple[tclas_idx].tclas.type4.dst_ip.ipv6,
                tclas->tclas.type14.type14_v6.dst_ip, IEEE80211_IPV6_LEN);
    scs_data->tclas_tuple[tclas_idx].tclas.type4.src_port =
                tclas->tclas.type14.type14_v6.src_port;
    scs_data->tclas_tuple[tclas_idx].tclas.type4.dst_port =
                tclas->tclas.type14.type14_v6.dst_port;
    scs_data->tclas_tuple[tclas_idx].tclas.type4.dscp =
                tclas->tclas.type14.type14_v6.type4_dscp;
    scs_data->tclas_tuple[tclas_idx].tclas.type4.next_header =
                tclas->tclas.type14.type14_v6.type4_next_header;
}

/* Copy TCLAS type 10 parameters - This function
 * copies the TCLAS parameters with Type 10
 * parameters to SCS descriptor.
 * @param scs_data - SCS descriptor pointer
 * @param tclas_element - TCLAS linked list element
 * @param tclas_idx - TCLAS entry number for given SCS entry number.
 */
uint8_t ieee80211_tclas_copy_type10_params(struct ieee80211_scs_data *scs_data,
        struct tclas_element *tclas, uint8_t tclas_idx)
{
    int left_over_len = (tclas->len-4)/2;
    uint8_t protocol_no = tclas->tclas.type10.protocol;
    uint64_t *mask, *value;

    /* ECM supports TCLAS10 elements only for UDP encapsulated IPSEC and ESP */
    if (((protocol_no != IPPROTO_UDP) && (protocol_no != IPPROTO_ESP)) ||
        ((protocol_no == IPPROTO_ESP) && (left_over_len != IEEE80211_TCLAS10_ESP_FILTER_LEN)) ||
        ((protocol_no == IPPROTO_UDP) && (left_over_len != IEEE80211_TCLAS10_UDP_ENCAP_IPSEC_FILTER_LEN)))
        return IEEE80211_SCS_REQUEST_DECLINED;

    /* ECM supports only SPI based rule matching for TCLAS10 */
    if (protocol_no == IPPROTO_UDP) {
        mask = (uint64_t *)tclas->tclas.type10.filter_mask;
        value = (uint64_t *)tclas->tclas.type10.filter_value;
        if (*mask || *value)
            return IEEE80211_SCS_REQUEST_DECLINED;
    }

    scs_data->tclas_tuple[tclas_idx].tclas.type10.protocol_instance =
            tclas->tclas.type10.prot_instance;
    scs_data->tclas_tuple[tclas_idx].tclas.type10.protocol_number =
            tclas->tclas.type10.protocol;
    scs_data->tclas_tuple[tclas_idx].tclas.type10.filter_len = left_over_len;
    scs_data->tclas_tuple[tclas_idx].tclas.type10.filter_val
                = tclas->tclas.type10.filter_value;

    scs_data->tclas_tuple[tclas_idx].tclas.type10.filter_mask
                = tclas->tclas.type10.filter_mask;
    return IEEE80211_SCS_SUCCESS;
}

uint8_t ieee80211_tclas_copy_params(struct ieee80211_scs_data *scs_data,
                                    struct tclas_element *tclas)
{
    uint8_t type, status;
    uint8_t tclas_elements = scs_data->tclas_elements;
    uint8_t tclas_idx = tclas_elements;

    /* Boundary check for no. of tclas elements in 1 SCS descriptor */
    if (tclas_elements >= IEEE80211_SCS_TCLAS_MAX_SIZE)
        return IEEE80211_SCS_INSUFFICIENT_TCLAS_PROCESSING_RESOURCES;

    type = tclas->type;
    scs_data->tclas_tuple[tclas_idx].type = type;

    switch (type) {
        case IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4: {

            uint8_t version = tclas->tclas.type14.type14_v4.version;
            scs_data->tclas_tuple[tclas_idx].tclas.type4.mask = tclas->mask;
            scs_data->tclas_tuple[tclas_idx].tclas.type4.version = version;

            switch (version) {
                case IEEE80211_WNM_TCLAS_CLAS14_VERSION_4:
                    ieee80211_tclas_copy_type4_v4_params(scs_data, tclas,
                                                         tclas_idx);
                    break;

                default:
                    ieee80211_tclas_copy_type4_v6_params(scs_data, tclas,
                                                         tclas_idx);
                    break;
            }
            return IEEE80211_SCS_SUCCESS;
        }

        case IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE10:
            status = ieee80211_tclas_copy_type10_params(scs_data, tclas,
                                                        tclas_idx);
            return status;

        default:
            return IEEE80211_SCS_REQUESTED_TCLAS_NOT_SUPPORTED;
    }
}

void ieee80211_scs_desc_mem_free(struct ieee80211_scs_data *scs_data)
{
    uint8_t index = 0;
    struct tclas_tuple *tclas_elem = NULL;

    for (index = 0; index < scs_data->tclas_elements; index++) {
        tclas_elem = &(scs_data->tclas_tuple[index]);
        if (tclas_elem->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE10) {
            OS_FREE(tclas_elem->tclas.type10.filter_mask);
            OS_FREE(tclas_elem->tclas.type10.filter_val);
        }
    }

#ifdef WLAN_FEATURE_11BE
    if (scs_data->qos_attr) {
        qdf_mem_free(scs_data->qos_attr);
        scs_data->qos_attr = NULL;
    }
#endif

    OS_FREE(scs_data);
}

/* SCS Delete stream - This function is to delete a given
 * SCS stream/rule. Identified by it's SCSID, this function
 * deletes the corresponding entry from the database.
 * @param ni - Node data base
 * @param scs_id - SCSID
 * return - PROCESSING Terminated message
 */
uint16_t
ieee80211_scs_del_stream(struct ieee80211_node *ni,
        uint8_t scs_id)
{
    uint8_t idx = scs_id;
    uint8_t sessions = ni->ni_scs_sessions;
    if (idx > sessions-1) {
        qdf_err("Invalid entry to be removed");
        return IEEE80211_SCS_REQUEST_DECLINED;
    }

    ieee80211_scs_desc_mem_free(ni->ni_scs[idx]);

    while (idx < (sessions - 1)) {
            ni->ni_scs[idx] = ni->ni_scs[idx + 1];
            idx++;
    }
    ni->ni_scs_sessions--;
    ni->ni_scs[idx] = NULL;

    return IEEE80211_SCS_TCLAS_PROCESSING_TERMINATED;
}

#ifdef QCA_SUPPORT_WDS_EXTENDED
void ieee80211_update_scs_rules(struct ieee80211vap *vap, struct net_device *dev)
{
    uint8_t i = 0;
    struct ieee80211_node *ni = NULL;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *assoc_node = NULL;
    osif_peer_dev *osifp  = ath_netdev_priv(dev);
#ifdef WLAN_FEATURE_11BE_MLO
    bool flag = false;
#endif

    if (!ic)
        return;

    ni = ieee80211_find_node(ic, osifp->peer_mac_addr, WLAN_MLME_SB_ID);
    if (!ni)
        return;

#ifdef WLAN_FEATURE_11BE_MLO
    if (wlan_peer_is_mlo(ni->peer_obj) && !wlan_peer_mlme_is_assoc_peer(ni->peer_obj)) {
        assoc_node = ieee80211_ucfg_get_mlo_assoc_link_node(ni->peer_obj->mlo_peer_ctx);
        if (!assoc_node)
            goto release_ref;

        flag = true;
    } else
#endif
    {
        assoc_node = ni;
    }

    qdf_spin_lock_bh(&assoc_node->ni_scs_lock);

    for (i = 0; i < assoc_node->ni_scs_sessions; i++) {
#ifdef WLAN_FEATURE_11BE
        if (ieee80211_get_scs_qos_attr_direction(assoc_node, i) == IEEE80211_QOS_ATTR_DIRECTION_DOWNLINK)
#endif
            ieee80211_send_scs_rule_config_event(assoc_node, i, IEEE80211_SCS_CHANGE_RULE);
    }

    qdf_spin_unlock_bh(&assoc_node->ni_scs_lock);

#ifdef WLAN_FEATURE_11BE_MLO
    if (flag)
        wlan_objmgr_peer_release_ref(assoc_node->peer_obj, WLAN_MLME_NB_ID);
#endif

release_ref:
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);
}
#endif

/* Populate SCS response -
 * This function will populate the response with
 * SCSID and Status code tuple information
 */
uint8_t*
ieee80211_populate_scs_resp(struct ieee80211_node *ni, uint8_t *frm)
{
    uint8_t index = 0;
    uint8_t req_idx = ni->ni_scs_resp.num_scs_resp;

    *frm++ = ni->ni_scs_resp.dialog_token;    /* Dialog Token */
    *frm++ = req_idx;    /* No. of SCS responses */
    scs_trace("Dialog_token: %d, SCS Response Count: %d",
              ni->ni_scs_resp.dialog_token, req_idx);
    while (req_idx != 0) {
        *frm++ = ni->ni_scs_resp.scsid[index];
        *frm++ = ni->ni_scs_resp.status[index] & 0x000000ff;
        *frm++ = (ni->ni_scs_resp.status[index] & 0x0000ff00) >> 8;
        scs_trace("Index:%d SCS ID:%d, Status:%d", index,
                  ni->ni_scs_resp.scsid[index],
                  ni->ni_scs_resp.status[index]);
        index++;
        req_idx--;
    }
    return frm;
}

int
ieee80211_send_scs_resp(struct ieee80211_node *ni)
{
    struct ieee80211_action_mgt_args scs_args = {0};
    scs_args.category = IEEE80211_ACTION_CAT_ROBUST_AV_STREAM;
    scs_args.action = IEEE80211_ACTION_SCS_RESPONSE;

    ieee80211_send_action(ni, &scs_args, NULL);

    return 0;
}

/* Send SCS response -
 * AP sends SCS response to the STA for a given SCS request.
 * In this response, it sends a tuple for every SCSID, along with
 * the status code.
 */
int
ieee80211_send_scs_unsolicited_resp(struct ieee80211_node *ni,
        u_int8_t dialog_token, uint8_t scsid, uint8_t req_type)
{
    uint8_t is_valid_scsid = 0;

    qdf_spin_lock_bh(&ni->ni_scs_lock);
    qdf_mem_zero(&ni->ni_scs_resp, sizeof(struct ieee80211_scs_resp));
    ni->ni_scs_resp.dialog_token = dialog_token;
    if (req_type == IEEE80211_SCS_REMOVE_RULE) {
        /* SCS rule to be deleted in peer db as well */
        is_valid_scsid = ieee80211_check_for_existing_scsid(ni, scsid);
        if (is_valid_scsid > IEEE80211_SCS_MAX_SIZE) {
            qdf_err("Invalid entry to be removed");
            goto fail;
        }
        ieee80211_send_scs_rule_config_event(ni, is_valid_scsid,
                                             IEEE80211_SCS_REMOVE_RULE);
        ieee80211_scs_del_stream(ni, is_valid_scsid);
        ieee80211_prepare_scs_resp(ni, scsid, IEEE80211_SCS_TCLAS_PROCESSING_TERMINATED);
        ieee80211_send_scs_resp(ni);
    }

fail:
    qdf_spin_unlock_bh(&ni->ni_scs_lock);

    return 0;
}


/* ieee80211_recv_scs_req - Receive SCS Request to parse, process and respond.
 * AP receives the SCS Request sent by the STA. In this request,
 * the STA adds the SCS Descriptor element, wherein details like SCS identifier,
 * intra access priority element, TCLAS elements, TCLAS processing element, QOS
 * attributes are present. The AP processes these parameters and stores the
 * params inside the node data structure. Then the AP sends the SCS response.
 * @param ni - Node data structure
 * @param frm - SCS request Action frame
 * @dialog_token - dialog token from SCS req frame
 * @frame_length - Entire frame length of SCS req frame
 *
 * @return - void
 */

void ieee80211_recv_scs_req(struct ieee80211_node *ni, uint8_t *frm,
                            uint8_t dialog_token, uint32_t frame_length)
{
    u_int8_t len = 0;
    struct ieee80211com *ic;
    struct ieee80211vap *vap;
    uint8_t scs_id;
    uint16_t status = 0;
    u_int8_t elem;
    uint8_t idx = 0;
    struct ieee80211_node *assoc_ni = ni;
    struct ieee80211_scs_data *scs_temp = NULL;
    bool resp_pending = false, skip_response = false;
#ifdef WLAN_FEATURE_11BE_MLO
    bool flag = false;
#endif

#ifdef SAWF_ADMISSION_CONTROL
    uint8_t ml_grp_id;
    uint16_t link_id;
    struct wlan_objmgr_psoc *psoc;
    struct ieee80211_scs_adm_ctrl_info *scs_info = NULL;
    struct ieee80211_scs_adm_ctrl_intf scs_adm_ctrl_intf = {0};
#endif

    vap = ni->ni_vap;
    ic = vap->iv_ic;

    if (!ic)
        return;

#ifdef SAWF_ADMISSION_CONTROL
    psoc = wlan_pdev_get_psoc(ni->ni_vap->iv_ic->ic_pdev_obj);
    mlo_psoc_get_grp_id(psoc, &ml_grp_id);
    link_id = ieee80211_mlo_get_pdev_hw_link_id(ni->ni_vap->iv_ic->ic_pdev_obj);
    scs_adm_ctrl_intf.hw_link_id = ((ml_grp_id << 8) | link_id);
    qdf_mem_copy(scs_adm_ctrl_intf.peer_mac_addr, ni->ni_macaddr, QDF_MAC_ADDR_SIZE);
    scs_adm_ctrl_intf.dialog_token = dialog_token;
#endif

    scs_info("SCS request frame dump - Frame Length:%d", frame_length);
    qdf_trace_hex_dump(QDF_MODULE_ID_SCS, QDF_TRACE_LEVEL_INFO, frm, frame_length);
    scs_info("Dialog token:%d", dialog_token);

#ifdef WLAN_FEATURE_11BE_MLO
    /* Maintain SCS database on assoc_node for MLO peer */
    if (wlan_peer_is_mlo(ni->peer_obj)) {
#ifdef SAWF_ADMISSION_CONTROL
        scs_adm_ctrl_intf.is_mlo_peer = 1;
        qdf_mem_copy(scs_adm_ctrl_intf.peer_mld_mac_addr,
                     wlan_peer_mlme_get_mldaddr(ni->peer_obj),
                     QDF_MAC_ADDR_SIZE);
#endif
        if (!wlan_peer_mlme_is_assoc_peer(ni->peer_obj)) {
            assoc_ni = ieee80211_ucfg_get_mlo_assoc_link_node(ni->peer_obj->mlo_peer_ctx);
            if (!assoc_ni)
                return;

            qdf_spin_lock_bh(&assoc_ni->ni_scs_lock);
            flag = true;
        }
    }
#endif

#ifdef SAWF_ADMISSION_CONTROL
    if (assoc_ni->request_processing) {
        scs_err("SCS req rcvd while processing previous request, dropping new req");

        skip_response = true;
        goto skip_processing;
    }
#endif

    qdf_mem_zero(&ni->ni_scs_resp, sizeof(struct ieee80211_scs_resp));
    ni->ni_scs_resp.dialog_token = dialog_token;

    while (frame_length > 0) {
        resp_pending = false;
        status = 0;
        elem = *frm;
        if (elem != IEEE80211_ELEMID_SCS_IE)
            break;

        if (idx >= IEEE80211_SCS_MAX_SIZE) {
            qdf_err("Rcvd more than %d SCS requests in 1 Action Frame", IEEE80211_SCS_MAX_SIZE);
            qdf_err("Processing only first %d SCS requests and ignoring the remaining ones", IEEE80211_SCS_MAX_SIZE);
            break;
        }

        len = *(frm + 1);
        scs_id = IEEE80211_SCS_GET_SCSID(frm);

        /* Allocate scs_temp struture for the looping scs descriptor */
        scs_temp = (struct ieee80211_scs_data*)OS_MALLOC(ic->ic_osdev,
                        (sizeof(struct ieee80211_scs_data)), 0);

        if (!scs_temp) {
            status = IEEE80211_SCS_INSUFFICIENT_TCLAS_PROCESSING_RESOURCES;
            goto next;
        }

        /* Set the initial state */
        scs_temp->scs_state = SCS_STATE_INIT;

        /* Parse SCS frame attributes and copy to scs_temp structure */
        status = ieee80211_parse_scs_req(assoc_ni, frm, scs_temp, len);
        if (status != IEEE80211_SCS_SUCCESS) {
            if (scs_temp) {
                scs_trace("Parse: Clearing scs_temp memory, status:%d", status);
                ieee80211_scs_desc_mem_free(scs_temp);
             }
             goto next;
        }

        /* Process the SCS request frame based on the request type*/
        status = ieee80211_process_scs_req(assoc_ni, scs_temp);

#ifdef SAWF_ADMISSION_CONTROL
        if (status == SCS_CONFIRMATION_PENDING) {
            if (scs_temp->qos_attr) {
                resp_pending = true;
                skip_response = true;
                assoc_ni->request_processing = true;

                scs_info = &scs_adm_ctrl_intf.scs_info[scs_adm_ctrl_intf.num_scs_reqs];
                scs_info->scsid = scs_temp->scsid;
                scs_info->tid = (scs_temp->access_priority & IEEE80211_SCS_OUTPUT_TID_MASK);
                scs_info->ac = TID_TO_WME_AC(scs_info->tid);
                scs_info->service_interval = scs_temp->qos_attr->resultant_service_interval;
                scs_info->burst_size = scs_temp->qos_attr->resultant_burst_size;
                scs_info->delay_bound = scs_temp->qos_attr->delay_bound;
                scs_info->min_data_rate = scs_temp->qos_attr->min_data_rate;

                scs_adm_ctrl_intf.num_scs_reqs++;
                goto next;
            }
        }
#endif

        /* For SCS Add fail after scs_temp is attached to the node, do not clear
         * the memory as it is already freed inside.
         * For SCS Del, return value is always != Success and memory should be
         * cleared. Below case takes care of that.
         */
        if (status != IEEE80211_SCS_SUCCESS) {
            if (status != SCS_REQUEST_DECLINED_DATA_FREED &&
                status != SCS_TCLAS_PROCESSING_TERMINATED_DATA_FREED) {
                if (scs_temp) {
                    scs_trace("Process: Clearing scs_temp memory, status:%d, "
                              "Request type:%d", status,
                              scs_temp->request_type);
                    ieee80211_scs_desc_mem_free(scs_temp);
                }
            }
        }

        /* When scs_temp data already freed case, in case of failure in add
         * after node attach, return IEEE80211_SCS_REQUEST_DECLINED code.
         * In case of failure during update after node attach, return
         * IEEE80211_SCS_TCLAS_PROCESSING_TERMINATED code.
         */
        if (status == SCS_REQUEST_DECLINED_DATA_FREED)
            status = IEEE80211_SCS_REQUEST_DECLINED;
        else if (status == SCS_TCLAS_PROCESSING_TERMINATED_DATA_FREED)
            status = IEEE80211_SCS_TCLAS_PROCESSING_TERMINATED;

next:
        frm += (len + 2);
        frame_length -= (len + 2);

        /* Populate response in buffer only when confirmation is not
           required from RM */
        if (!resp_pending) {
            /*** Populate SCS response in this database ***/
            ieee80211_prepare_scs_resp(ni, scs_id, status);
        }

        idx++;
    }

#ifdef SAWF_ADMISSION_CONTROL
    if (scs_adm_ctrl_intf.num_scs_reqs) {
        ni->transaction_key = dialog_token;
        wlan_cfg80211_send_scs_adm_ctrl_req(ni->ni_vap, &scs_adm_ctrl_intf);
        /* Start wait timer of 600 milliseconds for RM confirmation */
        qdf_timer_start(&ni->ni_scs_timer, 600);
    }
#endif

#ifdef SAWF_ADMISSION_CONTROL
skip_processing:
#endif
#ifdef WLAN_FEATURE_11BE_MLO
    if (flag) {
        /* Release lock and reference on assoc_node */
        qdf_spin_unlock_bh(&assoc_ni->ni_scs_lock);
        wlan_objmgr_peer_release_ref(assoc_ni->peer_obj, WLAN_MLME_NB_ID);
    }
#endif

    /* Send response to STA if no confirmation required from RM.
       In case, if RM confirmation is required, then response shall be sent to
       STA either on reception of response from RM or on timer expiry */
    if (!skip_response) {
        /* Generate scsid and status tuple*/
        ieee80211_send_scs_resp(ni);
    }
}

/**
 * ieee80211_scs_clear_stream(): API to clear scs stream on rule population
 *                               failure.
 * @ic: Handle to ic
 * @rule_id: 32 bit rule_id
 *
 * Return: void
 */
void ieee80211_scs_clear_stream(struct ieee80211com *ic, uint32_t rule_id)
{
    uint16_t peer_id = 0;
    uint8_t scs_id = 0;
    uint8_t peer_mac[QDF_MAC_ADDR_SIZE];
    u_int8_t idx;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc_obj;

    struct ieee80211_node *ni = NULL;

    pdev = ic->ic_pdev_obj;
    if (!pdev) {
       scs_err("pdev is NULL");
       return;
    }

    psoc_obj = wlan_pdev_get_psoc(pdev);
    if (!psoc_obj) {
       scs_err("psoc obj is NULL");
       return;
    }

    peer_id = IEEE80211_FETCH_ID_FROM_SCS_RULE_ID(rule_id, PEER_ID);

    scs_id = IEEE80211_FETCH_ID_FROM_SCS_RULE_ID(rule_id, SCS_ID);

    cdp_get_peer_mac_from_peer_id(wlan_psoc_get_dp_handle(psoc_obj),
                                  peer_id, peer_mac);

    ni = ieee80211_find_node(ic, peer_mac, WLAN_MLME_NB_ID);
    if (!ni) {
       scs_err("Node not found");
       return;
    }

    qdf_spin_lock_bh(&ni->ni_scs_lock);
    idx = ieee80211_check_for_existing_scsid(ni, scs_id);

    if (idx > IEEE80211_SCS_MAX_SIZE) {
        goto free_node;
    }

    qdf_mem_zero(&ni->ni_scs_resp, sizeof(struct ieee80211_scs_resp));

    ieee80211_send_scs_rule_config_event(ni, idx, IEEE80211_SCS_REMOVE_RULE);

    ieee80211_scs_del_stream(ni, idx);

    ieee80211_prepare_scs_resp(ni, scs_id,
                               IEEE80211_SCS_TCLAS_PROCESSING_TERMINATED);

    ieee80211_send_scs_resp(ni);

free_node:
    qdf_spin_unlock_bh(&ni->ni_scs_lock);

    ieee80211_free_node(ni, WLAN_MLME_NB_ID);
}

#ifdef SAWF_ADMISSION_CONTROL
void ieee80211_deprioritize_scs_flows(struct ieee80211_node *ni, uint8_t svc_id)
{
    uint8_t scs_id;
    bool response = false;

    qdf_spin_lock_bh(&ni->ni_scs_lock);
    qdf_mem_zero(&ni->ni_scs_resp, sizeof(struct ieee80211_scs_resp));

    if (ni->ni_scs_sessions) {
        uint8_t session_idx = 1;
        uint8_t sessions = ni->ni_scs_sessions;
        uint8_t idx = sessions - 1;

        /* Delete SCS streams in revere order to avoid shifting overhead */
        while (idx >= 0 && session_idx <= sessions) {
            if (ni->ni_scs[idx]->qos_attr && ni->ni_scs[idx]->qos_attr->service_class_id == svc_id) {
                scs_id = ni->ni_scs[idx]->scsid;
                ieee80211_send_scs_rule_config_event(ni, idx,
                                                     IEEE80211_SCS_REMOVE_RULE);
                ieee80211_scs_del_stream(ni, idx);
                ieee80211_prepare_scs_resp(ni, scs_id, IEEE80211_SCS_TCLAS_PROCESSING_TERMINATED);
                response = true;
            }
            idx--;
            session_idx++;
        }
    }

    if (response)
        ieee80211_send_scs_resp(ni);

    qdf_spin_unlock_bh(&ni->ni_scs_lock);
}
#endif /* SAWF_ADMISSION_CONTROL */
#else
void ieee80211_deprioritize_scs_flows(struct ieee80211_node *ni, uint8_t svc_id)
{}
#endif /* WLAN_SUPPORT_SCS */

#if WLAN_SUPPORT_MSCS
/* MSCS Procedure related functionalities */


/* Send MSCS information to Datapath -
 * AP sends the MSCS information from Control path to Data path
 * In this process, two types of Databases are maintained -
 * Control Database and Flow Database.
 * This function sends the information to Data path, which
 * later creates a flow database, based on this information.
 * @param ni - Node data structure
 * @param retval - Return code,0 if successful
 */
int ieee80211_send_mscs_info(struct ieee80211_node *ni,
    u_int16_t retval)
{
    u_int8_t ctr = 0;
    bool active = 1;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic;
    struct net_device *dev;
    ol_txrx_soc_handle soc_txrx_handle;
    struct wlan_objmgr_psoc *psoc;
    struct cdp_mscs_params mscs_params;
    union iwreq_data wrqu;
    osif_dev *osifp;

    if (!vap)
        return 0;

    ic = vap->iv_ic;
    if (!ic)
        return 0;

    dev = ((osif_dev *)vap->iv_ifp)->netdev;
    if (!dev)
        return 0;

    osifp = ath_netdev_priv(dev);
    if (!osifp)
        return 0;

    if (retval == IEEE80211_MSCS_SUCCESS || retval == IEEE80211_MSCS_TCLAS_PROCESSING_TERMINATED ||
        retval == IEEE80211_MSCS_INSUFFICIENT_TCLAS_PROCESSING_RESOURCES ||
        vap->iv_mscs == 0) {
        /* The parameters are valid and the classifier mask,
         * classifier type, User priority control
         * fields are recorded and sent to the Data path */
        if (retval == IEEE80211_MSCS_TCLAS_PROCESSING_TERMINATED ||
            retval == IEEE80211_MSCS_INSUFFICIENT_TCLAS_PROCESSING_RESOURCES) {
            ni->ni_mscs->mscs_active = false;
            active = 0;
        }
        if(!vap->iv_mscs){
            active = 0;
        }
        psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
        soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
        mscs_params.user_pri_bitmap = ni->ni_mscs->node_stats.user_priority_bitmap;
        mscs_params.user_pri_limit = ni->ni_mscs->node_stats.user_priority_limit;
        mscs_params.classifier_type = ni->ni_mscs->node_service[ctr].classifier_type;
        mscs_params.classifier_mask = ni->ni_mscs->node_service[ctr].classifier_mask;
        cdp_record_vdev_mscs_params(soc_txrx_handle,ni->peer_obj->macaddr,
            wlan_vdev_get_id(vap->vdev_obj),
            &mscs_params,
            active);
        /*
         * Enable Hlos tid override globally on VAP
         * per peer tid override will be taken care in datapath
         */
        ieee80211_ucfg_set_hlos_tid_override(osifp, 1, true);

#ifdef WLAN_FEATURE_11BE_MLO
    /* Enable hlos_tidoverride on partner links also if vap is MLD AP */
    if (ieee80211_mlo_is_mld_ap(vap->vdev_obj)) {
        uint8_t i = 0;
        uint16_t num_links = 0;
        osif_dev *partner_osifp = NULL;
        struct net_device *partner_dev = NULL;
        struct ieee80211vap *partner_vap = NULL;
        struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};

        ieee80211_mlo_ap_get_vdev_list(vap->vdev_obj, &num_links, vdev_list);

        for (i = 0; i < num_links; i++) {
            if (vap->vdev_obj == vdev_list[i])
                goto release_ref;

            partner_vap = wlan_vdev_mlme_get_ext_hdl(vdev_list[i]);
            if (!partner_vap)
                goto release_ref;

            partner_dev = ((osif_dev *)partner_vap->iv_ifp)->netdev;
            if (!partner_dev)
                goto release_ref;

            partner_osifp = ath_netdev_priv(partner_dev);
            if (!partner_osifp)
                goto release_ref;

            ieee80211_ucfg_set_hlos_tid_override(partner_osifp, 1, true);

release_ref:
            ieee80211_mlo_release_vdev_ref(vdev_list[i]);
        }
    }
#endif
        if (retval != IEEE80211_MSCS_TCLAS_PROCESSING_TERMINATED &&
            retval != IEEE80211_MSCS_INSUFFICIENT_TCLAS_PROCESSING_RESOURCES) {
            /* Flush existing connection rule in ECM for this node */
            memset(&wrqu, 0, sizeof(wrqu));
            IEEE80211_ADDR_COPY(wrqu.addr.sa_data, ni->peer_obj->macaddr);
            wrqu.addr.sa_family = ARPHRD_ETHER;
            WIRELESS_SEND_EVENT(dev, IWEVEXPIRED, &wrqu, NULL);
        }
    }

    return 0;
}

/* Send MSCS Response -
 * AP sends the MSCS Response corresponding to the MSCS request
 * sent by the STA. By sending the MSCS response, the AP agrees
 * or disagrees the request put up by the STA, by specifying the
 * return code.
 * @param ni - Node data structure
 * @param retval - Return code,0 if successful
 * @param dialog_token - Dialog token that the STA had sent in MSCS request
 */
int ieee80211_send_mscs_resp(struct ieee80211_node *ni,
    u_int16_t retval, u_int8_t dialog_token)
{
    struct ieee80211_action_mgt_args mscs_args;
    mscs_args.category = IEEE80211_ACTION_CAT_ROBUST_AV_STREAM;
    mscs_args.action = IEEE80211_ACTION_MSCS_RESPONSE;
    mscs_args.arg1 = dialog_token;
    mscs_args.arg2 = (u_int16_t)retval;

    if (ni->ni_mscs)
        ieee80211_send_mscs_info(ni, retval);
    else
        qdf_err("MSCS memory is NULL");

    ieee80211_send_action(ni, &mscs_args, NULL);

    return 0;
}

/* Receive MSCS Request -
 * AP recives the MSCS Request sent by the STA. In this request,
 * the STA adds the MSCS Descriptor element, wherein details like
 * user priority control field, stream timeout, TCLAS mask elements
 * are present. The AP processes these parameters and stores the params
 * inside the node data structure. Then the AP sends the MSCS response.
 * @param ni - Node data structure
 * @param mscs_req - MSCS request Action frame
 */

int ieee80211_recv_mscs_req(struct ieee80211_node *ni,
    struct ieee80211_action_mscs *mscs_req)
{
    /* Processing of MSCS request to be done here */
    u_int16_t retval = 0;
    u_int8_t dialog_token = mscs_req->mscs_dialogtoken;
    retval = ieee80211_parse_mscs_ie(ni,&mscs_req->mscs_ie);

    /* Send MSCS_response
     * The current implementation sends response without any
     * MSCS descriptor element, which means AP
     * will accept all the ADD/CHANGE valid requests
     * without any rejection, provided it is valid */

    ieee80211_send_mscs_resp(ni,retval,dialog_token);
    return 0;
}
#endif
uint8_t
ieee80211_find_node_in_mscs_disallow_list(struct ieee80211_mscs_disallow_db
        *disallow_mscs_list, u_int8_t *macaddr)
{
    uint8_t idx = 0;
    uint8_t retval = 1;
    uint8_t max_count = disallow_mscs_list->count;

    while(idx < max_count) {
        if (macaddr == disallow_mscs_list->candidate[idx].bssid)
            return retval;
        idx++;
    }
    retval = 0;
    return retval;
}

uint8_t
ieee80211_handle_incoming_mscs_sessions(struct ieee80211_node *ni,
        struct ieee80211_mscs_disallow_db *disallow_mscs_list, uint8_t
        dialog_token)
{
    uint8_t status = 0;
    uint8_t retval = 0;

    retval = ieee80211_find_node_in_mscs_disallow_list(disallow_mscs_list,
            ni->ni_bssid);
    if (retval) {
        ieee80211_send_mscs_resp(ni, IEEE80211_MSCS_REQUEST_DECLINED,
                dialog_token);
        return status;
    }
    status = 1;
    return status;
}

uint8_t
ieee80211_handle_existing_mscs_sessions(struct ieee80211vap *vap)
{
    uint8_t idx = 0,retval = 0;
    struct ieee80211_node *ni = NULL;
    uint8_t *macaddr = NULL;
    uint8_t status = 0;
    u_int8_t dialog_token = 123; /* Random Dialog token # */

    while(idx < vap->mscs_disallow_db.count) {
        macaddr = vap->mscs_disallow_db.candidate[idx].bssid;
        ni = ieee80211_vap_find_node(vap, macaddr, WLAN_WNM_ID);
        if (ni && ni->ni_mscs) {
            /* Check if the node has mscs active */
            if (ni->ni_mscs->mscs_active) {
                /* terminate the session */
                qdf_mem_zero(ni->ni_mscs, sizeof(struct ieee80211_mscs_data));
                status = IEEE80211_MSCS_TCLAS_PROCESSING_TERMINATED;
                retval = ieee80211_send_mscs_resp(ni, status, dialog_token);
            }
            ieee80211_free_node(ni, WLAN_WNM_ID);
        }
        idx++;
    }
    /* For STAs with inactive sessions, they will be dealt when
     * the request is sent
     */
    if (ni != NULL)
        ieee80211_free_node(ni, WLAN_WNM_ID);
    return retval;
}

static
int check_if_bssid_exists_in_mscs_disallow_db (uint8_t *macaddr,
        struct ieee80211_mscs_disallow_db mscs_disallow_db)
{
    uint8_t idx = 0;

    while (idx < mscs_disallow_db.count) {
        if (qdf_mem_cmp(macaddr, mscs_disallow_db.candidate[idx].bssid,
                    QDF_MAC_ADDR_SIZE))
            return 1;
        idx++;
    }

    return 0;
}

int
ieee80211_disallow_mscs(struct ieee80211vap *vap,
        struct ieee80211_mscs_disallow_db *disallow_mscs_list)
{
    uint8_t idx = 0;
    uint8_t retval = 0;
    uint8_t current_idx = vap->mscs_disallow_db.count;
    uint8_t no_of_entries = disallow_mscs_list->count;
    if (disallow_mscs_list->type == IEEE80211_MSCS_DISALLOW_GET_LIST)
        /* we need to populate the candidate list which have been
         * refrained MSCS sessions from the AP
         * and send this to the requested upper layer application
         * as an QCA_NL80211 event
         */
        /* To be implemented! */
        return retval;
    vap->mscs_disallow_db.is_true = 1; /* Use this db to filter out incoming
                                           session requests */
    /* Check if the vap table is populated */
    if ((current_idx >= IEEE80211_MSCS_MAX_DISALLOW_LIST) ||
            ((current_idx + no_of_entries)
             >= IEEE80211_MSCS_MAX_DISALLOW_LIST)) {
        qdf_info("VAP cannot disallow more STAs, max limit reached, remove existing entries first");
        return -ENOMEM;
    }

    while (idx < no_of_entries) {
        if (!check_if_bssid_exists_in_mscs_disallow_db(disallow_mscs_list->candidate[idx].bssid,
                    vap->mscs_disallow_db)) {
            qdf_info("New bssid entry added to disallow list");
            qdf_mem_copy(vap->mscs_disallow_db.candidate[current_idx + idx].bssid,
                    disallow_mscs_list->candidate[idx].bssid,
                    QDF_MAC_ADDR_SIZE);
        }
            idx++;
    }
    vap->mscs_disallow_db.count += no_of_entries;

    retval = ieee80211_handle_existing_mscs_sessions(vap);
    qdf_info("\t\tSTAs barred from MSCS sessions\n\n");
    idx = 0;
    while (idx < vap->mscs_disallow_db.count) {
        qdf_info("\t\t%d=====>%pM\n",idx, vap->mscs_disallow_db.candidate[idx].bssid);
        idx++;
    }

    return retval;
}

#if WLAN_SUPPORT_DAP
/*
 * Check existing policy () -
 * Function to check if the given policy exists.
 * If it does, then the index of that entry is
 * returned.
 * @param vap - VAP struct
 * @param policy_id - Policy_ID to be checked
 * @return pidx - if success else max value.
 */
uint8_t
ieee80211_check_existing_policy (struct ieee80211vap *vap,
        uint8_t policy_id)
{
    uint8_t pidx = 0;
    while (pidx != vap->iv_qos.no_of_policies) {
        if(policy_id ==
                vap->iv_qos.qos_mgmt_attr[pidx].policy_attr.policy_id)
            return pidx;
        pidx++;
    }

    return (IEEE80211_DSCP_ACTION_MAX_POLICIES + 1);
}

uint8_t*
ieee80211_populate_dap_header(struct ieee80211vap *vap,
        uint8_t *frm, struct ieee80211_action_mgt_args *actionargs)
{
    struct ieee80211_action_vendor_specific *ven;

    ven = (struct ieee80211_action_vendor_specific*)frm;
    ven->ia_category = actionargs->category;
    ven->vendor_oui[0] = 0x50;
    ven->vendor_oui[1] = 0x6f;
    ven->vendor_oui[2] = 0x9a;
    frm += sizeof(struct ieee80211_action_vendor_specific);

    /*** OUI Type ***/
    *frm++ = IEEE80211_DAP_REQ_OUI_TYPE;
    /*** OUI SubType ***/
    *frm++ = IEEE80211_DSCP_ACTION_POLICY_REQ;
    /*** Dialog Token ***/
    *frm++ = actionargs->arg1;
    *frm = 0;
    /*** Request control ***/
    *frm++ |= (vap->iv_qos.reset << 1);

    return frm;
}
/*
 * recv_and_store_dap_resp() - Function to
 * store DAP response from the STA in a
 * node database consisting of -
 * {PID, Status}
 *
 * @param ni - node struct
 * @param frm - response frm buffer
 * @param count - No.of {PID,Status} tuples
 * in the response.
 * return *frm - frm ptr
 */
uint8_t*
ieee80211_recv_and_store_dap_resp(struct ieee80211_node *ni,
        uint8_t *frm, uint8_t count)
{
    uint8_t pidx = 0;

    while (pidx < count) {
        ni->ni_dscp_action->response_table[pidx].policy_id = *frm++;
        ni->ni_dscp_action->response_table[pidx].status_code = *frm++;
        pidx++;
    }
    return frm;
}

/*
 * Parse DSCP Action Query frm() -
 * Function to parse query frm sent by the STA.
 * Attribute ID is checked to see if any
 * Policy attribute/TCLAS attribute/Domain name attribute/
 * Port range attribute is present in the frm.
 *
 * @param ni - Node Data struct
 * @param frm - Query frm buf
 *
 * Currently the scope is to only check if it's a
 * wildcard Query frm, if so, the AP will send
 * all of it's existing configured policies to the
 * STA.
 */
int
ieee80211_parse_dscp_query_frm(struct ieee80211_node *ni,
        uint8_t *frm)
{
    struct ieee80211vap *vap = ni->ni_vap;
    uint8_t attr_id;

    if (!frm)
        return 0;

    frm += sizeof(struct qos_mgmt_hdr);
    /* Get the Attr ID */

    attr_id = *frm++;

    if (attr_id < 2)
        return 0;

    /* Check if the AP has any existing policies configured
     * If yes, search for it (currently not in scope)
     * If no, return with zero policies - not a WILDCARD query
     */
    if (vap->iv_qos.no_of_policies == 0)
        return 0;

    return 0;
}

/* add_dscp_policy() -
 * Function to add a new dscp policy/
 * modify existing policy by rewriting it.
 *
 * @param vap - VAP struct
 * @param qos_ie - User configured QoS Mgmt IE params
 * consisting of policy elements.
 * The policies are configured by the
 * administrator, usually while bringing up
 * the AP. This can be done in user space.
 */
int
ieee80211_add_dscp_policy (struct ieee80211vap *vap,
        struct ieee80211_qos_mgmt_ie *qos_ie)
{
    uint8_t policy_id = 0;
    uint8_t existing_policies = 0;
    uint8_t pidx = 0;

    if(vap->iv_qos.no_of_policies >= IEEE80211_DSCP_ACTION_MAX_POLICIES) {
        qdf_err("Cannot add/change any policies, delete existing ones first");
        return -ENOMEM;
    }
    /* Get the number of existing policies */
    existing_policies = vap->iv_qos.no_of_policies;

    /* Check if policy ID exists */
    policy_id = qos_ie->qos_mgmt_attr[0].policy_attr.policy_id;


    if (policy_id == 0) {
        vap->iv_qos.reset = qos_ie->reset;
        return 0;
    }

    /* Check if the policy exists */
    pidx = ieee80211_check_existing_policy(vap, policy_id);

    if(pidx >= IEEE80211_DSCP_ACTION_MAX_POLICIES) {
        /* New policy, assign the latest index value */
        pidx = existing_policies;
        vap->iv_qos.no_of_policies++;
    }

    /* Copy policy attributes */
    vap->iv_qos.qos_mgmt_attr[pidx].policy_attr.policy_id =
        qos_ie->qos_mgmt_attr[0].policy_attr.policy_id;

    vap->iv_qos.qos_mgmt_attr[pidx].policy_attr.dscp =
        qos_ie->qos_mgmt_attr[0].policy_attr.dscp;


    /* Copy port range attributes if present */
    if (qos_ie->qos_mgmt_attr[0].port_range_present) {

        vap->iv_qos.qos_mgmt_attr[pidx].port_range_present = 1;
        vap->iv_qos.qos_mgmt_attr[pidx].port_range =
            qos_ie->qos_mgmt_attr[0].port_range;
    }

    if (qos_ie->qos_mgmt_attr[0].tclas_attr_present)
        /*Copy TCLAS attributes */
        ieee80211_copy_tclas_attr(vap, qos_ie, pidx);

        /*Copy Domain name attributes */
    if (qos_ie->qos_mgmt_attr[0].domain_name_present) {
        vap->iv_qos.qos_mgmt_attr[pidx].domain_name_present = 1;
        vap->iv_qos.qos_mgmt_attr[pidx].domain_name =
            qos_ie->qos_mgmt_attr[0].domain_name;
    }
    return 0;
}

/*
 * send_dscp_action_req() -
 * Main function to send DAP request to STA.
 * Based on the existing/add policies the AP sends request
 * to the STA.
 * It populates the attributes based on whatever is present
 * in that entry (keyed by the Policy ID) and adds them to the
 * request.
 * Usually policy_id_list tells which policies to be sent.
 * But if that is NULL, the AP considers the request as a
 * wildcard request and sends all the policies, that it has
 * been configured with.
 * @param ni - Node data struct
 * @param qos_ie - QoS Mgmt IE struct
 * @policies - Total # of policies to be sent
 * @policy_id_list - Which policies to be sent
 */
int ieee80211_send_dscp_action_req(struct ieee80211_node *ni,
        struct ieee80211_qos_mgmt_ie *qos_ie,
        uint8_t policies, uint8_t *policy_id_list)
{
    /* Setting up QoS mgmt attributes and sending the request */
    struct ieee80211vap *vap;
    uint8_t num_policies = 0;
    uint8_t pidx = 0;
    uint8_t req_type = 0;
    uint8_t policy_num = 0;
    uint8_t ptrx = 0;
    uint8_t hdr_len, policy_len,tclas_len, domain_name_len,
            tclas_attr_present, request_type, port_range_len;
    struct ieee80211_action_mgt_args dscp_action_req_args;
    struct ieee80211_action_mgt_buf dscp_action_mgmt_buf;
    struct qos_mgmt_policy_attr *qos_mgmt_policy_attr;
    struct qos_mgmt_hdr *qos_mgmt_hdr;
    struct qos_mgmt_port_attr *port_range;

    vap = ni->ni_vap;

    if (ieee80211_is_pmf_enabled(ni->ni_vap, ni)){
            dscp_action_req_args.category = IEEE80211_ACTION_CAT_VENDOR_PROTECTED;
    } else {
        dscp_action_req_args.category = IEEE80211_ACTION_CAT_VENDOR;
    }
    dscp_action_req_args.action = IEEE80211_ELEMID_VENDOR;
    if (policies == 0) {
        /* We need to send all the policies */
        dscp_action_req_args.arg1 = ni->ni_dscp_action->dialog_token;
        policies = qos_ie->no_of_policies;
    } else {
        dscp_action_req_args.arg1 = 123; /*Default dialog token # */
    }

    while (num_policies != policies) {

        tclas_len = 0;
        hdr_len = 0;
        req_type = 0;
        policy_len = 0;
        port_range_len = 0;
        domain_name_len = 0;
        qos_mgmt_hdr = (struct qos_mgmt_hdr *) &dscp_action_mgmt_buf.buf[ptrx];
        hdr_len = ieee80211_qos_mgmt_fill_header(qos_mgmt_hdr);
        qos_mgmt_policy_attr =
            (struct qos_mgmt_policy_attr *) &dscp_action_mgmt_buf.buf[ptrx +
            hdr_len];

        /* Get the policy number from the array */
        if (policy_id_list == NULL) {
            policy_num = 0;
        } else {
            policy_num = policy_id_list[num_policies];
        }
        if (policy_num == 0) {
            /* Wildcard query, we need to send all the policies */
            pidx = num_policies;
        } else {
            /* Check if the entry is present in the vap table */
            pidx = ieee80211_check_existing_policy(vap, policy_num);
        }

        if (pidx >= IEEE80211_DSCP_ACTION_MAX_POLICIES) {
            /* Entry is not present, request type to be set to remove */
            req_type = IEEE80211_DSCP_ACTION_POLICY_REMOVE;
        }

        /*** =================== POLICY ATTRIBUTE ======================***/

        policy_len =
            ieee80211_qos_mgmt_fill_policy_attr(qos_mgmt_policy_attr,
                    qos_ie, pidx, policy_num, req_type);
        request_type = qos_mgmt_policy_attr->req_type;

        if (request_type == IEEE80211_DSCP_ACTION_POLICY_REMOVE ||
                pidx >= IEEE80211_DSCP_ACTION_MAX_POLICIES) {
            dscp_action_mgmt_buf.buf[ptrx + 1] += hdr_len + policy_len - 2;
            ptrx += hdr_len + policy_len;
            num_policies++;
            continue;
        }
        /***
         * Now, for the existing policies, we need to formulate
         * their corresponding Port Range,
         * TCLAS and Domain name attributes.
         */
        /*** =============== PORT RANGE ATTRIBUTE =================***/
        if (qos_ie->qos_mgmt_attr[pidx].port_range_present == 1) {
            port_range =
                (struct qos_mgmt_port_attr *)&dscp_action_mgmt_buf.buf[ptrx
                + hdr_len + policy_len];
            port_range_len =
                ieee80211_qos_mgmt_fill_port_range_attr(port_range,
                        qos_ie, pidx);
        }

        /*** =============== TCLAS ATTRIBUTE ======================***/
        tclas_attr_present =
            qos_ie->qos_mgmt_attr[pidx].tclas_attr_present;

        if(qos_ie->qos_mgmt_attr[pidx].tclas_attr_present) {
            tclas_len = ieee80211_qos_mgmt_fill_tclas_attr(
                &dscp_action_mgmt_buf.buf[ptrx +
                hdr_len + policy_len + port_range_len],
                qos_ie, tclas_attr_present, pidx);
        }

        /*** ================ DOMAIN NAME ATTRIBUTE ================***/
        if (qos_ie->qos_mgmt_attr[pidx].domain_name_present) {
            domain_name_len = ieee80211_qos_mgmt_fill_domain_name_attr(
                    &dscp_action_mgmt_buf.buf[ptrx +
                    hdr_len + policy_len + tclas_len + port_range_len],
                    &qos_ie->qos_mgmt_attr[pidx].domain_name);
        }

        num_policies++;
        dscp_action_mgmt_buf.buf[ptrx + 1] +=
            hdr_len - 2 + policy_len + tclas_len + domain_name_len +
            port_range_len;
        ptrx += hdr_len + policy_len + tclas_len +
            domain_name_len + port_range_len;
    }
    dscp_action_req_args.arg2 = ptrx;
    ieee80211_send_action(ni, &dscp_action_req_args, &dscp_action_mgmt_buf);
    return 0;
}

/*
 * delete_dscp_policy() - Deletes an existing
 * policy.
 *
 * @param vap - VAP struct
 * @policy_id - Policy ID
 * return 0 if success
 */
int
ieee80211_del_dscp_policy (struct ieee80211vap *vap,
        uint8_t policy_id)
{
    uint8_t policies = vap->iv_qos.no_of_policies;
    uint8_t pidx = 0;

    if (policies > IEEE80211_DSCP_ACTION_MAX_POLICIES) {
        qdf_info("Something is wrong, flush out existing entries");
        policies = IEEE80211_DSCP_ACTION_MAX_POLICIES ;
    }
    /* Check if the policy to be deleted is present */
    pidx = ieee80211_check_existing_policy(vap, policy_id);

    if (pidx >= IEEE80211_DSCP_ACTION_MAX_POLICIES) {
        qdf_err("Policy to be deleted is not present");
        return -EINVAL;
    }

    while (pidx < policies-1) {
        qdf_mem_zero(&vap->iv_qos.qos_mgmt_attr[pidx], sizeof(struct ieee80211_qos_mgmt_attr));
        vap->iv_qos.qos_mgmt_attr[pidx] = vap->iv_qos.qos_mgmt_attr[pidx + 1];
        pidx++;
    }
    qdf_mem_zero(&vap->iv_qos.qos_mgmt_attr[pidx], sizeof(struct ieee80211_qos_mgmt_attr));
    vap->iv_qos.no_of_policies--;

    return 0;
}

/*
 * send_dscp_policy() - Interface function
 * to send an existing policy, which in turn
 * goes to the next step of formulating a DAP
 * request.
 * @param vap - VAP struct
 * @param ni - node struct
 * @param qos_req - QoS request struct containing
 * policy ID list.
 */
int
ieee80211_send_dscp_policy (struct ieee80211vap *vap,
        struct ieee80211_node *ni,
        struct ieee80211_qos_req *qos_req)
{
    uint8_t policies = 0;
    struct ieee80211_qos_mgmt_ie *qos_ie;

    if (!qos_req) {
        policies = 0;
    } else {
        policies = qos_req->no_of_policies;
    }

    if (qos_req) {
        if (qos_req->reset == 2)
            vap->iv_qos.reset = 0;

        if (qos_req->reset == 1)
            vap->iv_qos.reset = 1;
    }

    if (policies > IEEE80211_DSCP_ACTION_MAX_POLICIES) {
        qdf_err("Number of policies to be sent is more than MAX SIZE");
        return -EINVAL;
    }
    qos_ie = &vap->iv_qos;

    if (qos_req != NULL) {
        ieee80211_send_dscp_action_req(ni, qos_ie, policies, qos_req->policy_id_list);
    } else {
        ieee80211_send_dscp_action_req(ni, qos_ie, 0, NULL);
    }
    return 0;
}
#endif

int ieee80211_recv_tfs_req(wlan_if_t vap, wlan_node_t ni, u_int8_t *frm, int frm_len)
{
    u_int8_t status = -1;

    if (frm_len == 3) {
        ieee80211_tfs_request *tfsreq;

        tfsreq = TAILQ_FIRST(&ni->ni_wnm->tfsreq->tfs_req_head);
        if (tfsreq) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM,
                    "Delete current TFS filter: Id %d\n", tfsreq->tfs_id);
            wlan_wnm_free_tfs(&ni->ni_wnm->tfsreq->tfs_req_head);
        }
        return 1;
    }

    frm = frm + 2;

    ni->ni_wnm->tfsreq->dialogtoken = *frm++;
    frm = ieee80211_add_tfs_req(vap, ni, frm, frm_len);

    if (frm != NULL) {
        status = ieee80211_send_tfs_rsp(vap, ni);
        if (status == 0) {
            wlan_wnm_free_tfsrsp(&ni->ni_wnm->tfsrsp->tfs_rsp_head);
        }
    }
    return 1;
}

u_int8_t *ieee80211_add_tfs_req(wlan_if_t vap, wlan_node_t ni, u_int8_t *frm, int frm_len)
{
    uint8_t *filter_value;
    uint8_t *filter_mask;
    int tfs_length;
    int subele_length;
    TAILQ_HEAD(, ieee80211_tfs_request) tfs_req_head;
    TAILQ_HEAD(, ieee80211_tfs_response) tfs_rsp_head;
    struct ieee80211_tfs_subelement_req *tfs_subele;
    struct ieee80211_tfs_subelement_rsp *tfs_subrsp = NULL;
    struct ieee80211_tfs_request *tfs_req = NULL;
    struct ieee80211_tfs_response *tfs_rsp;
    struct tclas_element *tclas = NULL, *temp_tclas = NULL;
    int filter_len;
    u_int8_t *sfrm;

    TAILQ_INIT(&tfs_req_head);
    TAILQ_INIT(&tfs_rsp_head);

    sfrm = frm;

    while ((frm - sfrm) <= frm_len) {
        if (*frm != IEEE80211_ELEMID_TFS_REQUEST) {
            break;
        }
        tfs_req = OS_MALLOC(vap->iv_ic->ic_osdev,
                sizeof(struct ieee80211_tfs_request), 0);
        if (tfs_req == NULL) {
            goto bad;
        }

        tfs_rsp = OS_MALLOC(vap->iv_ic->ic_osdev,
                sizeof(struct ieee80211_tfs_response), 0);
        if (tfs_rsp == NULL) {
            OS_FREE(tfs_req);
            goto bad;
        }

        tfs_req->tfs_elemid = *frm++;
        tfs_req->length = *frm++;
        tfs_length = tfs_req->length;
        tfs_req->tfs_id = *frm++;
        tfs_req->tfs_action_code = *frm++;

        TAILQ_INIT(&tfs_req->tfs_req_sub);
        TAILQ_INIT(&tfs_rsp->tfs_rsp_sub);

        tfs_length = tfs_length - 2;

        while (tfs_length > 0) {
            tfs_subele = OS_MALLOC(vap->iv_ic->ic_osdev,
                    sizeof(struct ieee80211_tfs_subelement_req), 0);
            if (tfs_subele == NULL) {
                OS_FREE(tfs_req);
                OS_FREE(tfs_rsp);
                goto bad;
            }

            tfs_subrsp = OS_MALLOC(vap->iv_ic->ic_osdev,
                    sizeof(struct ieee80211_tfs_subelement_rsp), 0);
            if (tfs_subrsp == NULL) {
                OS_FREE(tfs_req);
                OS_FREE(tfs_rsp);
                OS_FREE(tfs_subele);
                goto bad;
            }
            tfs_subrsp->status = IEEE80211_TFS_ACCEPT;

            tfs_subele->elem_id = *frm++;
            tfs_subele->length = *frm++;
            subele_length = tfs_subele->length;
            tfs_length = tfs_length - (subele_length + 2);

            TAILQ_INIT(&tfs_subele->tclas_head);

            while (subele_length != 0) {

                if (*frm == IEEE80211_ELEMID_TCLAS) {

                    tclas = OS_MALLOC(vap->iv_ic->ic_osdev,
                            sizeof(struct tclas_element), 0);
                    if (tclas == NULL) {
                        OS_FREE(tfs_req);
                        OS_FREE(tfs_rsp);
                        OS_FREE(tfs_subele);
                        OS_FREE(tfs_subrsp);
                        goto bad;
                    }

                    tclas->elemid = *frm++;
                    tclas->len = *frm++;
                    tclas->up = *frm++;
                    tclas->type = *frm++;
                    tclas->mask = *frm++;
                    if (tclas->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE3) {
                        filter_len = (tclas->len - 5) / 2;
                        filter_value = OS_MALLOC(vap->iv_ic->ic_osdev,
                                filter_len, GFP_KERNEL);
                        if (filter_value == NULL) {
                            OS_FREE(tfs_req);
                            OS_FREE(tfs_rsp);
                            OS_FREE(tfs_subele);
                            OS_FREE(tfs_subrsp);
                            OS_FREE(tclas);
                            goto bad;
                        }
                        filter_mask = OS_MALLOC(vap->iv_ic->ic_osdev,
                                filter_len, GFP_KERNEL);
                        if (filter_mask == NULL) {
                            OS_FREE(tfs_req);
                            OS_FREE(tfs_rsp);
                            OS_FREE(tfs_subele);
                            OS_FREE(tfs_subrsp);
                            OS_FREE(tclas);
                            OS_FREE(filter_value);
                            goto bad;
                        }
                        tclas->tclas.type3.filter_offset = be16toh(*(u_int16_t* )frm);
                        frm += 2;
                        memcpy(filter_value, frm , filter_len);
                        tclas->tclas.type3.filter_value = filter_value;
                        frm += filter_len;
                        memcpy(filter_mask, frm, filter_len);
                        tclas->tclas.type3.filter_mask = filter_mask;
                        frm += filter_len;
                    } else if (tclas->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE1 ||
                               tclas->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4)
                    {
                        if (*frm == IEEE80211_WNM_TCLAS_CLAS14_VERSION_4) {
                            tclas->tclas.type14.type14_v4.version = *frm++;
                            memcpy(tclas->tclas.type14.type14_v4.src_ip, frm,
                                    IEEE80211_IPV4_LEN);
                            frm += IEEE80211_IPV4_LEN;
                            memcpy(tclas->tclas.type14.type14_v4.dst_ip, frm,
                                    IEEE80211_IPV4_LEN);
                            frm += IEEE80211_IPV4_LEN;
                            tclas->tclas.type14.type14_v4.src_port =
                                        be16toh(*(u_int16_t* )frm);
                            frm += 2;
                            tclas->tclas.type14.type14_v4.dst_port =
                                        be16toh(*(u_int16_t* )frm);
                            frm += 2;
                            tclas->tclas.type14.type14_v4.dscp = *frm++;
                            tclas->tclas.type14.type14_v4.protocol = *frm++;
                            tclas->tclas.type14.type14_v4.reserved = *frm++;
                        } else if (*frm == IEEE80211_WNM_TCLAS_CLAS14_VERSION_6) {
                            tclas->tclas.type14.type14_v6.version = *frm++;
                            memcpy(tclas->tclas.type14.type14_v6.src_ip, frm,
                                    IEEE80211_IPV6_LEN);
                            frm += IEEE80211_IPV6_LEN;
                            memcpy(tclas->tclas.type14.type14_v6.dst_ip, frm,
                                    IEEE80211_IPV6_LEN);
                            frm += IEEE80211_IPV6_LEN;
                            tclas->tclas.type14.type14_v6.src_port =
                                        be16toh(*(u_int16_t* )frm);
                            frm += 2;
                            tclas->tclas.type14.type14_v6.dst_port =
                                        be16toh(*(u_int16_t* )frm);
                            frm += 2;
                            if (tclas->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4) {
                                tclas->tclas.type14.type14_v6.type4_dscp = *frm++;
                                tclas->tclas.type14.type14_v6.type4_next_header = *frm++;
                            }
                            memcpy(tclas->tclas.type14.type14_v6.flow_label, frm, 3);
                            frm += 3;
                        }
                    } else {
                        tfs_subrsp->status = IEEE80211_TFS_INVALID_CLASSIFIER;
                        break;
                    }
                } else if (*frm == IEEE80211_ELEMID_TCLAS_PROCESS) {
                    tfs_subele->tclasprocess.elem_id = *frm++;
                    tfs_subele->tclasprocess.length = *frm++;
                    tfs_subele->tclasprocess.tclas_process = *frm++;
                    break;
                } else {
                    tfs_subrsp->status = IEEE80211_TFS_REASON_UNKNOWN;
                    break;
                }
                TAILQ_INSERT_TAIL(&tfs_subele->tclas_head, tclas, tclas_next);
                subele_length = subele_length - (tclas->len + 2);
            }

            if (tfs_length > 255) {
                tfs_subrsp->status = IEEE80211_TFS_REASON_UNKNOWN;
            }

            if (tfs_subrsp->status == IEEE80211_TFS_ACCEPT) {
                TAILQ_INSERT_TAIL(&tfs_req->tfs_req_sub, tfs_subele, tsub_next);
            } else {
                /* As tfs_subele has not been used, it should be freed */
                TAILQ_FOREACH_SAFE(tclas, &tfs_subele->tclas_head, tclas_next,
                                   temp_tclas) {
                    if (tclas->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE3) {
                        OS_FREE(tclas->tclas.type3.filter_value);
                        OS_FREE(tclas->tclas.type3.filter_mask);
                    }
                    TAILQ_REMOVE(&tfs_subele->tclas_head, tclas, tclas_next);
                    OS_FREE(tclas);
                }
                OS_FREE(tfs_subele);
                wlan_wnm_delete_tfs_req(ni, tfs_req->tfs_id);
            }

    	    tfs_subrsp->tfsid = tfs_req->tfs_id;
            TAILQ_INSERT_TAIL(&tfs_rsp->tfs_rsp_sub, tfs_subrsp, tsub_next);
        }
        if (TAILQ_FIRST(&tfs_req->tfs_req_sub) != NULL) {
            TAILQ_INSERT_TAIL(&tfs_req_head, tfs_req, tfs_req_next);
        } else {
            qdf_info("tfs_req_sub tailqueue is empty ");
            /* As tfs_req has not been used, free it */
            OS_FREE(tfs_req);
        }
        tfs_rsp->tfs_elemid  = IEEE80211_ELEMID_TFS_RESPONSE;
		TAILQ_INSERT_TAIL(&tfs_rsp_head, tfs_rsp, tfs_rsp_next);
    }

    wlan_wnm_free_tfsrsp(&ni->ni_wnm->tfsrsp->tfs_rsp_head);
    TAILQ_CONCAT(&ni->ni_wnm->tfsrsp->tfs_rsp_head, &tfs_rsp_head, tfs_rsp_next);

    if (TAILQ_FIRST(&ni->ni_wnm->tfsrsp->tfs_rsp_head) != NULL) {
        wlan_wnm_free_tfs(&ni->ni_wnm->tfsreq->tfs_req_head);
        TAILQ_CONCAT(&ni->ni_wnm->tfsreq->tfs_req_head, &tfs_req_head, tfs_req_next);

        if((tfs_subrsp != NULL) &&(tfs_req != NULL) && (tclas != NULL))  {
            if (tfs_subrsp->status == IEEE80211_TFS_ACCEPT)
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "Install new TFS filter: "
                        "Id %d, TCLAS Type %d\n", tfs_req->tfs_id, tclas->type);
            else
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, "TFS Reply %d to Request",
                        tfs_subrsp->status);
        }
    } else {
        if (TAILQ_FIRST(&ni->ni_wnm->tfsreq->tfs_req_head) != NULL) {
            wlan_wnm_free_tfs(&ni->ni_wnm->tfsreq->tfs_req_head);
        }
    }

    if (TAILQ_FIRST(&ni->ni_wnm->tfsreq->tfs_req_head) == NULL) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WNM, " TFS REQ NULL \n");
    }

    return frm;

bad:
    wlan_wnm_free_tfs(&tfs_req_head);
    wlan_wnm_free_tfsrsp(&tfs_rsp_head);
    return NULL;
}

u_int8_t *
ieee80211_add_tfsrsp_ie(wlan_if_t vap, wlan_node_t ni, u_int8_t *frm)
{
    struct ieee80211_tfs_response *tfs_rsp;
    u_int8_t *len;

    TAILQ_FOREACH(tfs_rsp, &ni->ni_wnm->tfsrsp->tfs_rsp_head, tfs_rsp_next) {
        struct ieee80211_tfs_subelement_rsp *tfs_subrsp;

        *frm++ = tfs_rsp->tfs_elemid;
        len = frm;
        *len = 0;
        frm++;
        TAILQ_FOREACH(tfs_subrsp, &tfs_rsp->tfs_rsp_sub, tsub_next) {
            *frm++ = tfs_subrsp->elem_id = 1;
            *frm++ = 2;
            *frm++ = tfs_subrsp->status;
            *frm++ = tfs_subrsp->tfsid;
            *len += 4;
        }
    }

    return frm;
}

int ieee80211_send_tfs_rsp(wlan_if_t vap, wlan_node_t ni)
{
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL;

    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm, 0);
    if (wbuf == NULL) {
        return -ENOMEM;
    }

    /* Check if PMF is enabled and set security bit */
    if (wlan_crypto_is_pmf_enabled(vap->vdev_obj, ni->peer_obj) &&
        ieee80211_node_is_authorized(ni)) {
        struct ieee80211_frame *wh;
        wh = (struct ieee80211_frame*)wbuf_header(wbuf);
        wh->i_fc[1] |= IEEE80211_FC1_WEP;
    }

    *frm++ = IEEE80211_ACTION_CAT_WNM;
    *frm++ = IEEE80211_ACTION_TFS_RESP;
    *frm++ = ni->ni_wnm->tfsreq->dialogtoken;

    frm = ieee80211_add_tfsrsp_ie(vap, ni, frm);
    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));

    return ieee80211_send_mgmt(vap, ni, wbuf, false);
}

/**
 * ieee80211_apply_tfs_filter() - Helper function for TFS
 * @ni - Node handler
 * @wbuf - Wbuf
 *
 * Helper function for TFS
 *
 * return 0 on success, non-zero value on failure.
 */
static inline int ieee80211_apply_tfs_filter(struct ieee80211_node *ni,
                                             wbuf_t wbuf)
{
    struct ether_header *eh;
    struct ip_header *ip;

    eh = (struct ether_header *)wbuf_header(wbuf);
    ip = (struct ip_header *)(wbuf_header(wbuf) + sizeof(*eh));
    return (__ieee80211_tfs_filter(ni, wbuf, eh->ether_type, ip));
}

#ifdef WLAN_FEATURE_11BE_MLO
/**
 * ieee80211_mlo_tfs_filter() - Apply TFS for all partner peers of MLD STA
 * @vap - VAP handle
 * @daddr - Destination addr of MLD STA
 * @wbuf - Wbuf
 *
 * This function iterates through MLD peer context and applies traffic
 * filter for all partner peers.
 *
 * return 1 if filter match, 0 otherwise.
 */
static int ieee80211_mlo_tfs_filter(struct ieee80211vap *vap,
                                    u_int8_t *daddr, wbuf_t wbuf)
{

    struct wlan_mlo_peer_context *mlo_peer_ctx = NULL;
    struct wlan_mlo_link_peer_entry *mlo_peer = NULL;
    struct qdf_mac_addr mac;
    struct ieee80211_node *ni;
    int status = 1, i;

    qdf_mem_copy(mac.bytes, daddr, QDF_MAC_ADDR_SIZE);
    mlo_peer_ctx = wlan_mlo_get_mlpeer(vap->vdev_obj->mlo_dev_ctx, &mac);

    /* This is not MLO peer */
    if (!mlo_peer_ctx) {
        ni = ieee80211_find_txnode(vap, daddr, WLAN_WNM_ID);
        if (ni && (vap->iv_bss != ni))
            status = ieee80211_apply_tfs_filter(ni, wbuf);
        if (ni)
            ieee80211_free_node(ni, WLAN_WNM_ID);
         return status;
    }

    for (i = 0; i < mlo_peer_ctx->link_peer_cnt; i++) {
        mlo_peer = &mlo_peer_ctx->peer_list[i];
        if (!mlo_peer)
            return status;
        ni = ieee80211_find_txnode(vap, mlo_peer->link_addr.bytes, WLAN_WNM_ID);
        if (ni && (vap->iv_bss != ni))
            status = ieee80211_apply_tfs_filter(ni, wbuf);
        if (ni)
            ieee80211_free_node(ni, WLAN_WNM_ID);
        if (!status)
            return status;
    }
    return status;
}
#endif

int ieee80211_tfs_filter(struct ieee80211vap *vap, wbuf_t wbuf)
{
    struct ether_header *eh;
    u_int8_t *daddr;
    struct ieee80211_node *ni;
    int status = 1;

    if(wbuf != NULL){
        eh = (struct ether_header *)wbuf_header(wbuf);
    }
    else{
        return status;
    }

    if ((eh->ether_type == htons(ETHERTYPE_PAE)) ||
        (vap->iv_opmode != IEEE80211_M_HOSTAP))
        return status;

    daddr = eh->ether_dhost;
#ifdef WLAN_FEATURE_11BE_MLO
    if (ieee80211_mlo_is_mld_ap(vap->vdev_obj))
        status = ieee80211_mlo_tfs_filter(vap, daddr, wbuf);
    else
#endif
    {
        ni = ieee80211_find_txnode(vap, daddr, WLAN_WNM_ID);
        if (ni && (vap->iv_bss != ni))
            status = ieee80211_apply_tfs_filter(ni, wbuf);
        if (ni)
            ieee80211_free_node(ni, WLAN_WNM_ID);
    }
    return status;
}

int
__ieee80211_tfs_filter(struct ieee80211_node *ni, wbuf_t wbuf,
                       int ether_type, struct ip_header *ip)
{
    struct ieee80211_tfs_request *tfs_req, *temp_treq;
    int filter_len, i;
    unsigned char *bp;
    u_int32_t saddr, daddr;
    int tclas_count = 0;
    int tclas_match = 0;
    int filter_match = 1;
    int status = 1;
    int status_count = 0;
    int buf_len = 0;
    u_int8_t tfsid[10];
    u_int8_t tfsid_count = 0;
    struct ieee80211_tfs_subelement_req *tfs_subele;
    struct tclas_element *tclas_element;
    u_int8_t protocol;
    u_int8_t dscp;
    u_int8_t sport_match;
    u_int8_t dport_match;
    u_int16_t sport;
    u_int16_t dport;
    u_int16_t *tcpudphdr;
    u_int16_t offset;

    TAILQ_FOREACH_SAFE(tfs_req, &ni->ni_wnm->tfsreq->tfs_req_head, tfs_req_next, temp_treq) {
        TAILQ_FOREACH(tfs_subele, &tfs_req->tfs_req_sub, tsub_next) {
            tclas_count = 0;
            tclas_match = 0;

            TAILQ_FOREACH(tclas_element, &tfs_subele->tclas_head, tclas_next) {
                tclas_count++;
                if (tclas_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE3) {
                    struct llc llc;
                    unsigned char *bp_llc;
                    int useBTEP;
                    struct ether_header *eh;

                    eh = (struct ether_header *)wbuf_header(wbuf);
                    useBTEP = (eh->ether_type == htons(ETHERTYPE_AARP)) ||
                              (eh->ether_type == htons(ETHERTYPE_IPX));
                    offset = tclas_element->tclas.type3.filter_offset;
                    filter_len = (tclas_element->len - 5) / 2;
                    buf_len = wbuf_get_pktlen(wbuf);

                    if (buf_len < filter_len) {
                        filter_match = 0;
                        break;
                    }
                    bp_llc = (u_int8_t *)&llc;
                    llc.llc_dsap = llc.llc_ssap = LLC_SNAP_LSAP;
                    llc.llc_control = LLC_UI;
                    if (useBTEP) {
                        llc.llc_snap.org_code[0] = BTEP_SNAP_ORGCODE_0; /* 0x0 */
                        llc.llc_snap.org_code[1] = BTEP_SNAP_ORGCODE_1; /* 0x0 */
                        llc.llc_snap.org_code[2] = BTEP_SNAP_ORGCODE_2; /* 0xf8 */
                    } else {
                        llc.llc_snap.org_code[0] = RFC1042_SNAP_ORGCODE_0; /* 0x0 */
                        llc.llc_snap.org_code[1] = RFC1042_SNAP_ORGCODE_1; /* 0x0 */
                        llc.llc_snap.org_code[2] = RFC1042_SNAP_ORGCODE_2; /* 0x0 */
                    }
                    llc.llc_snap.ether_type = eh->ether_type;
                    bp = (u_int8_t *) (wbuf_header(wbuf) + sizeof(struct ether_header));

                    for (i = 0; i < filter_len; i++) {
                        if ((offset + i) < sizeof(llc)) {
                            if ((bp_llc[offset + i] &
                                 tclas_element->tclas.type3.filter_mask[i]) !=
                                (tclas_element->tclas.type3.filter_value[i] &
                                 tclas_element->tclas.type3.filter_mask[i]))
                            {
                              filter_match = 0;
                              break;
                            }
                        } else if ((bp[offset + i - sizeof(llc)] &
                                    tclas_element->tclas.type3.filter_mask[i]) !=
                                   (tclas_element->tclas.type3.filter_value[i] &
                                    tclas_element->tclas.type3.filter_mask[i]))
                        {
                            filter_match = 0;
                            break;
                        }
                    }
                    if (filter_match) {
                        if (tfs_subele->tclasprocess.tclas_process == 1) {
                            tclas_match = tclas_count;
                            break;
                        }
  	                    tclas_match++;
                    } else {
                        filter_match = 1;
                    }
                } else if (tclas_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE1 ||
                           tclas_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4)
                {
                    if ((ether_type == htons(ETHERTYPE_IP)) &&
                        (tclas_element->tclas.type14.type14_v4.version ==
                         IEEE80211_WNM_TCLAS_CLAS14_VERSION_4))
                    {
                        qdf_net_iphdr_t *hdr;

                        hdr = (qdf_net_iphdr_t *)ip;
                        sport = tclas_element->tclas.type14.type14_v4.src_port;
                        dport = tclas_element->tclas.type14.type14_v4.dst_port;
                        protocol = tclas_element->tclas.type14.type14_v4.protocol;
                        dscp = tclas_element->tclas.type14.type14_v4.dscp & ~INET_ECN_MASK;

                        memcpy(&saddr, tclas_element->tclas.type14.type14_v4.src_ip,
                                IEEE80211_IPV4_LEN);
                        memcpy(&daddr, tclas_element->tclas.type14.type14_v4.dst_ip,
                                IEEE80211_IPV4_LEN);

                        if ((hdr->ip_proto == protocol) &&
                            (!saddr || saddr == hdr->ip_saddr) &&
                            (!daddr || daddr == hdr->ip_daddr) /* &&
                            (dscp == (hdr->ip_tos & ~INET_ECN_MASK)) */)
                        {
                            tcpudphdr = (u_int16_t *)((u_int32_t *)hdr + hdr->ip_hl);
                            sport_match = 1;
                            if (sport && (sport != *tcpudphdr)) {
                                sport_match = 0;
                            }
                            dport_match = 1;
                            if (dport && (dport != *(tcpudphdr + 1))) {
                                dport_match = 0;
                            }

                            if (dport_match && sport_match) {
                                if (tfs_subele->tclasprocess.tclas_process == 1) {
                                    tclas_match = tclas_count;
                                    break;
                                }
                                tclas_match++;
                            }
                        }
                    } else if ((ether_type == htons(ETHERTYPE_IPV6)) &&
                               (tclas_element->tclas.type14.type14_v6.version ==
                                IEEE80211_WNM_TCLAS_CLAS14_VERSION_6))
                    {
                        qdf_net_ipv6hdr_t *ipv6;
                        unsigned long ver_pri_flowlabel;
                        unsigned long pri;
                        unsigned long *hdr;
                        u_int8_t *src_ip;
                        u_int8_t *dst_ip;
                        u_int8_t *flowlbl;
                        u_int8_t tos;
                        u_int8_t nexthdr;

                        hdr = (unsigned long *)(ip);
                        ipv6 = (qdf_net_ipv6hdr_t *)hdr;
                        ver_pri_flowlabel = *hdr;
                        sport = tclas_element->tclas.type14.type14_v4.src_port;
                        dport = tclas_element->tclas.type14.type14_v4.dst_port;
                        flowlbl = tclas_element->tclas.type14.type14_v6.flow_label;
                        if (tclas_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4) {
                            dscp = tclas_element->tclas.type14.type14_v6.type4_dscp & ~INET_ECN_MASK;
                            nexthdr = tclas_element->tclas.type14.type14_v6.type4_next_header;
                        }
                        pri = (ver_pri_flowlabel & IPV6_PRIORITY_MASK) >> IPV6_PRIORITY_SHIFT;
                        tos = (pri & ~INET_ECN_MASK) >> IP_PRI_SHIFT;
                        src_ip = tclas_element->tclas.type14.type14_v6.src_ip;
                        dst_ip = tclas_element->tclas.type14.type14_v6.dst_ip;

                        if ((tclas_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE1 ||
                            ((tclas_element->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4) &&
                             (ipv6->ipv6_nexthdr == nexthdr) && (tos == dscp))) &&
                            !OS_MEMCMP(ipv6->ipv6_flow_lbl, flowlbl, 3) &&
                            !OS_MEMCMP(src_ip, ipv6->ipv6_saddr.s6_addr, IEEE80211_IPV6_LEN) &&
                            !OS_MEMCMP(dst_ip, ipv6->ipv6_daddr.s6_addr, IEEE80211_IPV6_LEN))
                        {
                            tcpudphdr = (u_int16_t *)(ipv6 + 1);

                            sport_match = 1;
                            if (sport && (sport != *tcpudphdr)) {
                                sport_match = 0;
                            }
                            dport_match = 1;
                            if (dport && (dport != *(tcpudphdr + 1))) {
                                dport_match = 0;
                            }
                            if (dport_match && sport_match) {
                                if (tfs_subele->tclasprocess.tclas_process == 1) {
                                    tclas_match = tclas_count;
                                    break;
                                }
                                tclas_match++;
                            }
                        }
                    } else {
                        tclas_match++;
                    }
                }
            }
            if (tclas_count == tclas_match) {
                status = 1;
                status_count++;
                break;
            } else {
                status = 0;
            }
        }
        if (status) {
            if (IEEE80211_TFS_NOTIFY & tfs_req->tfs_action_code) {
                tfsid[tfsid_count++] = tfs_req->tfs_id;
            }
            if (IEEE80211_TFS_DELETE & tfs_req->tfs_action_code) {
                wlan_wnm_delete_tfs_req(ni, tfs_req->tfs_id);
            }
        }
    }

    /* Send TFS Notify only when the client has PM bit set */
    if ((tfsid_count != 0) && (tfsid_count < 10)) {
        u_int8_t sleep;

        sleep = ni->ps_state;
        if (sleep ) {
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_WNM,
                    "Send TFS Notify to %s, tfs_count %d\n",
                    ether_sprintf(ni->ni_macaddr), tfsid_count);
            ieee80211_tfs_notify(ni, tfsid_count, tfsid);
        }
    }

    if (status_count > 0) {
        status = 1;
    }

    return status;
}

int ieee80211_tfs_notify(struct ieee80211_node *ni,u_int8_t tfs_count, u_int8_t  *tfsid)
{
    struct ieee80211vap *vap = ni->ni_vap;
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL, i;

    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm, 0);
    if (wbuf == NULL) {
        return -ENOMEM;
    }

    *frm++ = IEEE80211_ACTION_CAT_WNM;
    *frm++ = IEEE80211_ACTION_TFS_NOTIFY;
    *frm++ = tfs_count;

    for (i = 0; i < tfs_count; i++) {
        *frm++ = tfsid[i];
    }

    wbuf_set_pktlen(wbuf, (frm - (u_int8_t *)wbuf_header(wbuf)));
    return ieee80211_send_mgmt(vap, ni, wbuf, false);
}


int ieee80211_recv_tim_req(wlan_if_t vap, wlan_node_t ni, u_int8_t *frm, int frm_len)
{
    u_int8_t *efrm;
    int status;

    efrm = frm + frm_len;
    frm += 2;

    ni->ni_wnm->timbcast_dialogtoken = *frm++;

    status = ieee80211_parse_timreq_ie(frm, frm + frm_len, ni);

    if (status == 0) {
        status = ieee80211_send_tim_response(ni->ni_vap, ni);
    }

    return status;
}

int ieee80211_parse_timreq_ie(u_int8_t *frm, u_int8_t *efrm, wlan_node_t ni)
{
    int status = -EINVAL;

    ni->ni_wnm->timbcast_status = IEEE80211_WNM_TIMREQUEST_DENIED;

    while ((frm + 1 < efrm) && (frm + frm[1] + 1 < efrm)) {
        switch (frm[0]) {
            case IEEE80211_ELEMID_TIM_BCAST_REQUEST:
                ni->ni_wnm->timbcast_interval = frm[2];
                status = 0;
            break;
        }
        frm += frm[1] + 2;
    }

    return status;
}

static
void wlan_calculate_timbcast_interval_iter_cb(void *arg,
                                              struct ieee80211_node *ni)
{
    int *found = (int *)arg;

    if ((*found) == 1)
        return;

    /* Compare wnm_timbcast_interval of vap and ni */
    if (ni->ni_vap->wnm->wnm_timbcast_interval == ni->ni_wnm->timbcast_interval)
        *found = 1;
}

int wlan_calculate_timbcast_interval(wlan_if_t vap)
{
    int found = 0;

    wlan_iterate_all_sta_list(vap, wlan_calculate_timbcast_interval_iter_cb,
                              (void *)&found);

    if (found == 0) {
        vap->wnm->wnm_timbcast_interval = 0;
    }

    return vap->wnm->wnm_timbcast_interval;
}

int ieee80211_send_tim_response(wlan_if_t vap, wlan_node_t ni)
{
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL;

    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm, 0);
    if (wbuf == NULL) {
        return -ENOMEM;
    }

    /* Check if PMF is enabled and set security bit */
    if (wlan_crypto_is_pmf_enabled(vap->vdev_obj, ni->peer_obj) &&
        ieee80211_node_is_authorized(ni)) {
        struct ieee80211_frame *wh;
        wh = (struct ieee80211_frame*)wbuf_header(wbuf);
        wh->i_fc[1] |= IEEE80211_FC1_WEP;
    }

    *frm++ = IEEE80211_ACTION_CAT_WNM;
    *frm++ = IEEE80211_ACTION_TIM_RESP;
    *frm++ = ni->ni_wnm->timbcast_dialogtoken;

    frm = ieee80211_add_timresp_ie(vap, ni, frm);

    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));

    return ieee80211_send_mgmt(vap, ni, wbuf, false);
}

u_int8_t *ieee80211_add_timresp_ie(wlan_if_t vap, wlan_node_t ni, u_int8_t *frm)
{
    int status = IEEE80211_WNM_TIMREQUEST_DENIED;
    int tim_interval = 0;
    u_int8_t *len;
    int i;
    u_int32_t basicrate = 0;

    *frm++ = IEEE80211_ELEMID_TIM_BCAST_RESPONSE;
    len = frm;
    frm++;

    if (ni->ni_wnm->timbcast_interval) {
        vap->wnm->wnm_timbcast_interval = wlan_calculate_timbcast_interval(vap);
        if (vap->wnm->wnm_timbcast_interval) {
            if (vap->wnm->wnm_timbcast_interval >= ni->ni_wnm->timbcast_interval) {
                if (!(vap->wnm->wnm_timbcast_interval % ni->ni_wnm->timbcast_interval)) {
                    status = IEEE80211_WNM_TIMREQUEST_ACCEPT;
                    vap->wnm->wnm_timbcast_interval = ni->ni_wnm->timbcast_interval;
                    tim_interval = vap->wnm->wnm_timbcast_interval;
                }
            } else {
                if (!(ni->ni_wnm->timbcast_interval % vap->wnm->wnm_timbcast_interval)) {
                    status = IEEE80211_WNM_TIMREQUEST_OVERRIDDEN;
                    tim_interval = vap->wnm->wnm_timbcast_interval;
                }
            }
        } else {
            tim_interval = vap->wnm->wnm_timbcast_interval = ni->ni_wnm->timbcast_interval;
            status = IEEE80211_WNM_TIMREQUEST_ACCEPT;
        }
    } else {
        if (vap->wnm->wnm_timbcast_interval) {
            tim_interval = wlan_calculate_timbcast_interval(vap);
            ni->ni_wnm->timbcast_interval = tim_interval;
            vap->wnm->wnm_timbcast_interval = tim_interval;
            status = IEEE80211_WNM_TIMREQUEST_ACCEPT;
        }
    }

    if (status == IEEE80211_WNM_TIMREQUEST_DENIED) {
        *frm++ = status;
        *len = 1;
    } else {
        *frm++ = status;
        if (ni->ni_wnm->timbcast_interval) {
            *frm++ = tim_interval;
            *(u_int32_t *)frm = htole32(vap->wnm->wnm_timbcast_offset);
            frm += 4;
            if (vap->wnm->wnm_timbcast_enable & IEEE80211_WNM_TIM_HIGHRATE_ENABLE) {
                *(u_int16_t *)frm = htole16(vap->wnm->wnm_timbcast_highrate);
            } else {
                memset(frm, 0, sizeof(u_int16_t));
            }
            frm += 2;
            if (vap->wnm->wnm_timbcast_enable & IEEE80211_WNM_TIM_LOWRATE_ENABLE) {
                for (i = 0; i < ni->ni_rates.rs_nrates; i++) {
                    if (basicrate == 0 ||
                        (basicrate & IEEE80211_RATE_VAL) >
                         (ni->ni_rates.rs_rates[i] & IEEE80211_RATE_VAL))
                    {
                        basicrate = ni->ni_rates.rs_rates[i];
                    }
                }
                vap->wnm->wnm_timbcast_lowrate = basicrate & IEEE80211_RATE_VAL;
                *(u_int16_t *)frm = htole16(vap->wnm->wnm_timbcast_lowrate);
            } else {
                memset(frm, 0, sizeof(u_int16_t));
            }
            frm += 2;
            *len = 10;
        } else {
            *len = 1;
        }
        ni->ni_wnm->timbcast_status = IEEE80211_WNM_TIMREQUEST_ACCEPT;
    }

    return frm;
}

#endif /* UMAC_SUPPORT_WNM */
