/*
 * Copyright (c) 2011-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011-2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * Copyright (c) 2008, Atheros Communications Inc.
 * All Rights Reserved.
 */
#ifdef IPA_OFFLOAD
#include "wlan_ipa_ucfg_api.h"
#include "wlan_psoc_mlme_api.h"
#endif
#include "ieee80211_api.h"
#include "ieee80211_mlme_priv.h"    /* Private to MLME module */
#include "base/ieee80211_node_priv.h"
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include <ol_if_athvar.h>
#include <osif_nss_wifiol_if.h>
#include <osif_nss_wifiol_vdev_if.h>
#endif
#include <osif_private.h>
#include <wlan_utility.h>
#if WLAN_SUPPORT_SPLITMAC
#include <wlan_splitmac.h>
#endif
#include <dp_txrx.h>
#if DBDC_REPEATER_SUPPORT
#include <qca_multi_link.h>
#endif
#include <ieee80211_ucfg.h>
#include <ol_if_athvar.h>
#include <wlan_mlme_vdev_mgmt_ops.h>
#include <include/wlan_psoc_mlme.h>
#include <wlan_rnr.h>
#if WLAN_SUPPORT_GREEN_AP
#include <wlan_green_ap_api.h>
#endif
#include <wlan_objmgr_global_obj_i.h>
#include <target_if.h>
#if WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_ap.h>
#include <wlan_mlo_mgr_peer.h>
#include <wlan_mlo_mgr_setup.h>
#endif

#if WLAN_FEATURE_11BE_MLO || UMAC_SUPPORT_CFG80211
#include <ieee80211_cfg80211.h>
#endif

#include "cfg_ol.h"
#include "cfg_ucfg_api.h"

#if ATH_SUPPORT_WRAP
#include <dp_wrap.h>
#endif
#include <wlan_repeater_api.h>
#include <wlan_mlo_mgr_sta.h>
#include <ieee80211_wds.h>
#include <cfg80211_external.h>
#include <wlan_mlme_if.h>

#ifdef WLAN_FEATURE_11BE
#include <wlan_mlo_t2lm.h>
#endif
#if QCA_AIRTIME_FAIRNESS
#include <wlan_atf_utils_api.h>
#endif

/* Local function prototypes */
static OS_TIMER_FUNC(timeout_callback);

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
static OS_TIMER_FUNC(stacac_timeout_callback);
#endif

#if DBDC_REPEATER_SUPPORT
static OS_TIMER_FUNC(drop_mcast_timeout_callback);
#endif

static void mlme_timeout_callback(struct ieee80211vap *vap, IEEE80211_STATUS  ieeeStatus);
void sta_deauth(void *arg, struct ieee80211_node *ni);
#define MAX_L2UF_RETIRES 10
extern void
osif_hardstart_l2uf(os_handle_t osif, u_int8_t *macaddr);

static void ieee80211_determine_max_non_transmit_vaps(
                            struct ieee80211com *ic) {
    uint8_t idx                    = 0;
    uint16_t used_space_in_cur_pos = 0;
    uint16_t node_size             = 0;
    uint16_t node_vendor_ie_size   = 0;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    bool is_mbssid_enabled = ic->is_mbssid_enabled;
    bool is_ema_enabled    = wlan_pdev_nif_feat_ext_cap_get(ic->ic_pdev_obj,
                             WLAN_PDEV_FEXT_EMA_AP_ENABLE);
    uint16_t rsvd_bcn_space_ntx_profiles;
    ol_ath_ema_pdev_ctx_t *ema_ctx = &scn->ema_ctx;
    ol_ath_ema_ctx_t *soc_ema_ctx = &scn->soc->ema_ctx;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
    uint8_t grp_id = IEEE80211_MBSSID_DEFAULT_GROUP_ID;

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss = &mbss_ctx->ic_mbss[grp_id];

    /* Irreconverable - this condition is already sanitized in
     * init_deinit_get_ema_max_pp()
     */
    qdf_assert(soc_ema_ctx->max_mgmt_frm_sz >
              (ema_ctx->beacon_common_part_size +
               ema_ctx->rnr_field_size_limit) +
               soc_ema_ctx->mbssie_headroom);

    rsvd_bcn_space_ntx_profiles =
                           (soc_ema_ctx->max_mgmt_frm_sz -
                           (ema_ctx->beacon_common_part_size +
                            ema_ctx->rnr_field_size_limit) +
                            soc_ema_ctx->mbssie_headroom);

    if (is_mbssid_enabled) {
        if (is_ema_enabled) {
            ic_mbss->max_non_transmit_vaps = ema_ctx->num_max_vaps - 1;
        } else {
            mbss_info("pool size: %d max_bssid: %d",
                    MBSSID_SUB_GROUP_SIZE(ic), mbss_ctx->max_bssid);
            for (idx = 0; idx < MBSSID_SUB_GROUP_SIZE(ic) - 1; idx++) {
                node_vendor_ie_size =
                        (idx < IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX) ?
                    IEEE80211_EMA_GET_VENDOR_IE_SIZE_FROM_NTX_IDX(
                                ema_ctx->vendor_ie_config_low, idx) :
                    IEEE80211_EMA_GET_VENDOR_IE_SIZE_FROM_NTX_IDX(
                                ema_ctx->vendor_ie_config_high,
                                (idx - IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX));

                /* Override for WPS */
                if (ema_ctx->support_wps_6ghz)
                    node_vendor_ie_size = ema_ctx->vendor_ie_size_limit;

                node_size = node_vendor_ie_size +
                                ema_ctx->optional_ie_size +
                                ema_ctx->max_non_tx_profile_sz_with_rsn;

                if (node_size <
                        (rsvd_bcn_space_ntx_profiles - used_space_in_cur_pos)) {
                    used_space_in_cur_pos += node_size;
                    ic_mbss->max_non_transmit_vaps++;
                } else
                    break;
            } /* end for */
        }

        /* All vaps are non-tx vap till tx vap designation */
        ic_mbss->max_non_transmit_vaps++;
    } /* end if (is_mbssid_enabled) */

    mbss_info(":< %d", ic_mbss->max_non_transmit_vaps);
}

/* assign beacon position for each node in cache */
static void ieee80211_allocate_beacon_pos_for_mbss_cache_nodes(
                            struct ieee80211vap *vap) {
    struct ieee80211com           *ic = vap->iv_ic;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ieee80211_mbss_ie_cache_node *node;
    uint8_t curr_pos               = 0;
    uint8_t idx                    = 0;
    uint16_t used_space_in_cur_pos = 0;
    uint16_t node_size             = 0;
    uint16_t node_vendor_ie_size   = 0;
    uint16_t rsvd_bcn_space_ntx_profiles;
    ol_ath_ema_pdev_ctx_t *ema_ctx = &scn->ema_ctx;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;

    if (!ic->ic_mbss_ctx)
        return;

    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss = &mbss_ctx->ic_mbss[vap->iv_mbss.group_id];

    /* Irreconverable - this condition is already sanitized in
     * init_deinit_get_ema_max_pp()
     */
    qdf_assert(scn->soc->ema_ctx.max_mgmt_frm_sz >
                  (ema_ctx->beacon_common_part_size +
                       ema_ctx->rnr_field_size_limit));

    rsvd_bcn_space_ntx_profiles = ema_ctx->max_non_tx_sz;
    ic->ic_mbss_ctx->non_tx_profile_size = ema_ctx->max_non_tx_sz_limit -
                                      ema_ctx->max_non_tx_profile_sz_with_rsn;

    /* This function should get called only in
     * case of EMA AP and in that case mbss_offset
     * can't be NULL
     */
    qdf_assert(ic_mbss->mbss_offset);
    ic_mbss->mbss_offset[0] = 0;

    mbss_info("config_low: 0x%x", ema_ctx->vendor_ie_config_low);
    mbss_info("config_high: 0x%x", ema_ctx->vendor_ie_config_high);

    for (idx = 0; idx < ic->ic_mbss_ctx->grp_sz - 1; idx++) {
        /* Is there enough slot left in the current
         * beacon pos to accomodate the IEs correspo
         * nding to this slot?
         */
        node = &((struct ieee80211_mbss_ie_cache_node *)
                                ic_mbss->mbss_cache)[idx];
        if (!ema_ctx->support_wps_6ghz || !ema_ctx->ext_enabled) {
            node_vendor_ie_size = (idx < IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX) ?
                                IEEE80211_EMA_GET_VENDOR_IE_SIZE_FROM_NTX_IDX(
                                ema_ctx->vendor_ie_config_low, idx) :
                                IEEE80211_EMA_GET_VENDOR_IE_SIZE_FROM_NTX_IDX(
                                ema_ctx->vendor_ie_config_high,
                                (idx - IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX));
        } else {
            node_vendor_ie_size = IEEE80211_EMA_VENDOR_IE_SECTION_BOUND_WITH_WPS;
        }

        node_size = node_vendor_ie_size +
                        ema_ctx->max_non_tx_profile_sz_with_rsn +
                        ema_ctx->optional_ie_size;

        mbss_debug("node_size: %u", node_size);
        mbss_debug("rsvd_bcn_space_ntx_profiles: %u", rsvd_bcn_space_ntx_profiles);
        mbss_debug("used_space_in_cur_pos: %u", used_space_in_cur_pos);

        if (node_size <
                (rsvd_bcn_space_ntx_profiles - used_space_in_cur_pos)) {
            used_space_in_cur_pos += node_size;
        } else {
            used_space_in_cur_pos = node_size;
            curr_pos++;
            ic_mbss->mbss_offset[curr_pos] = (uint8_t *)node -
                                    (uint8_t *) ic_mbss->mbss_cache;
        }
        node->pos = curr_pos;
        mbss_info("beacon position for mbss cache node with"
                       " bssidx: %d is %d", idx+1, node->pos);
    } /* end for */

    mbss_debug(":<");
    /* sanity check for position assignment */
    qdf_assert(++curr_pos == ic_mbss->max_pp);
}

static int ieee80211_mbss_cache_alloc(struct ieee80211com *ic)
{
    uint8_t gp_id;
    struct mbssid_context   *mbss_ctx;
    size_t mbss_cache_size;
    bool is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                             WLAN_PDEV_F_MBSS_IE_ENABLE);

    mbss_debug(":>");

    if (is_mbssid_enabled) {

        if (!ic->ic_mbss_ctx) {
            return -EINVAL;
        } else {
            mbss_ctx = ic->ic_mbss_ctx;
        }

        mbss_cache_size =
            sizeof(struct ieee80211_mbss_ie_cache_node) * (mbss_ctx->grp_sz);
        /* Allocate cache space to both ap & sec groups */
        for (gp_id = 0; gp_id < mbss_ctx->max_ngroups; gp_id++) {
            struct mbssid_info *ic_mbss = &mbss_ctx->ic_mbss[gp_id];

            ic_mbss->mbss_cache = qdf_mem_malloc(mbss_cache_size);
            mbss_debug("mbss_ctx->mbss_cache: %pK", ic_mbss->mbss_cache);

            if (!ic_mbss->mbss_cache)
                return -ENOMEM;

            qdf_mem_zero(ic_mbss->mbss_cache, mbss_cache_size);
            /* create lock to protect mbss_cache */
            qdf_spinlock_create(&ic_mbss->mbss_cache_lock);
        }
    }

    mbss_debug(":<");
    return 0;
}

static int ieee80211_mbss_init_ctx(struct ieee80211com *ic)
{
    int ret = -1;
    struct mbssid_context *mbss_ctx = ic->ic_mbss_ctx;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct target_psoc_info *psoc_info;
    bool is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                WLAN_PDEV_F_MBSS_IE_ENABLE);
    ol_ath_ema_pdev_ctx_t *ema_ctx;
    target_resource_config* tgt_cfg;

    psoc_info = wlan_psoc_get_tgt_if_handle(scn->soc->psoc_obj);
    if (!psoc_info) {
        qdf_mem_free(mbss_ctx);
        ic->ic_mbss_ctx = NULL;
        goto exit;
    }

    tgt_cfg = target_psoc_get_wlan_res_cfg(psoc_info);
    if (!tgt_cfg) {
        qdf_mem_free(mbss_ctx);
        ic->ic_mbss_ctx = NULL;
        goto exit;
    }

    if (is_mbssid_enabled && mbss_ctx) {
        ema_ctx = &scn->ema_ctx;
        mbss_ctx->max_ap_ngroups = ema_ctx->max_ngroups;
        /* Sanitize max ap groups */
        if (mbss_ctx->max_ap_ngroups > ema_ctx->num_max_vaps/2)
            mbss_ctx->max_ap_ngroups = ema_ctx->num_max_vaps/2;

        mbss_ctx->max_ngroups = mbss_ctx->max_ap_ngroups + IEEE80211_EMA_MAX_N_SEC_GROUPS;

        mbss_ctx->ic_mbss = qdf_mem_malloc(sizeof(struct mbssid_info)
                            * mbss_ctx->max_ngroups);
        if (!mbss_ctx->ic_mbss) {
            ret = -ENOMEM;
            qdf_mem_free(mbss_ctx);
            ic->ic_mbss_ctx = NULL;
            goto exit;
        }
        qdf_mem_zero(mbss_ctx->ic_mbss,
                    (sizeof(struct mbssid_info) * mbss_ctx->max_ngroups));
        mbss_info("max_bssid_indicator: %d", mbss_ctx->max_bssid);
        mbss_ctx->max_bssid = qdf_min(tgt_cfg->max_bssid_indicator,
                                     (uint32_t)ilog2(ema_ctx->num_max_vaps));
        mbss_ctx->grp_sz    = ema_ctx->grp_sz;

        qdf_mem_zero((void *)&mbss_ctx->sec_group_bitmap,
                     sizeof(mbss_ctx->sec_group_bitmap));
        mbss_ctx->sec_group_bitmap = ~(mbss_ctx->sec_group_bitmap |
                                       IEEE80211_EMA_SEC_GRP_BMASK);
        mbss_info("num_max_vaps:%d max_ap_ngroups:%d max_ngroups:%d grp_sz:%d",
                  ema_ctx->num_max_vaps, mbss_ctx->max_ap_ngroups,
                  mbss_ctx->max_ngroups, ema_ctx->grp_sz);

        ret = ieee80211_mbss_cache_alloc(ic);

        if (!ret) {
            ic->is_mbssid_enabled = is_mbssid_enabled;
            mbss_ctx->non_inherit_enable = true;
            mbss_ctx->non_tx_profile_size = ema_ctx->max_non_tx_sz_limit -
                                            ema_ctx->max_non_tx_profile_sz_with_rsn;
        }
    }

exit:
   return ret;
}

static int ieee80211_mbss_attach(struct ieee80211com *ic)
{
    int ret = 0;
    struct mbssid_context *mbss_ctx;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    bool is_mbssid_enabled;
    ol_ath_ema_pdev_ctx_t *ema_ctx;
    ol_ath_ema_ctx_t *soc_ema_ctx;
    uint8_t gp_id;
    qdf_freq_t low_freq, high_freq;
    QDF_STATUS status;

    mbss_info(":>");
    if (!ic) {
        ret = -EINVAL;
        goto exit;
    }

    is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                            WLAN_PDEV_F_MBSS_IE_ENABLE);

    mbss_ctx = ic->ic_mbss_ctx;
    if (is_mbssid_enabled) {
        ema_ctx = &scn->ema_ctx;
        soc_ema_ctx = &scn->soc->ema_ctx;
        if (!ic->ic_mbss_ctx) {
            mbss_ctx = qdf_mem_malloc(sizeof(struct mbssid_context));
            if (!mbss_ctx) {
                ret = -ENOMEM;
                goto exit;
            } else {
                ic->ic_mbss_ctx = mbss_ctx;
                ret = ieee80211_mbss_init_ctx(ic);
            }
        }

        if (!ret) {
            for (gp_id = 0; gp_id < mbss_ctx->max_ngroups; gp_id++) {
                struct mbssid_info *ic_mbss = &mbss_ctx->ic_mbss[gp_id];

                ic_mbss->max_pp   = soc_ema_ctx->max_pp;
                ic_mbss->group_id = gp_id;

                ic_mbss->mbss_offset = (uint16_t *) qdf_mem_malloc(
                        ic_mbss->max_pp * sizeof(ic_mbss->mbss_offset));

                if (ic_mbss->mbss_offset) {
                    qdf_mem_zero(ic_mbss->mbss_offset,
                            ic_mbss->max_pp * sizeof(ic_mbss->mbss_offset));

                    /* initialize current pp */
                    ic_mbss->current_pp = 1;

                    ic_mbss->max_bssid = ilog2(mbss_ctx->grp_sz);

                    ic_mbss->max_non_transmit_vaps = mbss_ctx->grp_sz - 1;

                    /* bcn_bo_mbss_ie & prb_po_mbss_ie to be allocated &
                     * used for AP groups only */
                    if (gp_id < mbss_ctx->max_ap_ngroups) {
                        ic_mbss->bcn_bo_mbss_ie = qdf_mem_malloc(
                                                  ema_ctx->max_non_tx_sz +
                                                  soc_ema_ctx->mbssie_headroom);
                        if (ic_mbss->bcn_bo_mbss_ie) {
                            ic_mbss->prb_po_mbss_ie = qdf_mem_malloc(
                                                      ema_ctx->max_non_tx_sz +
                                                      soc_ema_ctx->mbssie_headroom);
                        } else {
                            qdf_mem_free(ic_mbss->mbss_offset);
                            ic_mbss->mbss_offset = NULL;
                            ic_mbss->prb_po_mbss_ie = NULL;
                        }

                        if (!ic_mbss->prb_po_mbss_ie) {
                            qdf_mem_free(ic_mbss->mbss_offset);
                            ic_mbss->mbss_offset = NULL;
                            qdf_mem_free(ic_mbss->bcn_bo_mbss_ie);
                            ic_mbss->bcn_bo_mbss_ie = NULL;
                        } else {
                            qdf_mem_set(&ic_mbss->vdev_id_gp, sizeof(ic_mbss->vdev_id_gp),
                                        IEEE80211_MBSSID_INVALID_VDEV_ID);
                            ic_mbss->mbssid_init = true;
                        }
                    } else {
                        qdf_mem_set(&ic_mbss->vdev_id_gp, sizeof(ic_mbss->vdev_id_gp),
                                    IEEE80211_MBSSID_INVALID_VDEV_ID);
                        ic_mbss->mbssid_init = true;
                    }
                } else {
                    qdf_mem_free(ic_mbss->mbss_cache);
                    ic_mbss->mbss_cache = NULL;
                }
            } /* end for */
            if (mbss_ctx->max_ap_ngroups == 1)
                ieee80211_determine_max_non_transmit_vaps(ic);
        } /* end !ret */ else {
            if (ic->ic_mbss_ctx) {
                qdf_mem_free(ic->ic_mbss_ctx);
                ic->ic_mbss_ctx = NULL;
                ic->is_mbssid_enabled = false;
                goto exit;
            }
        }
    } /* end is_mbssid_enabled */

    /* The intent is not to return error value from
     * wlan_reg_get_freq_range() function.
     * Failure in this only hamper the OOB RNR logic.
     * But other functionalities are fine.
     */
    status = wlan_reg_get_freq_range(ic->ic_pdev_obj, NULL, NULL, &low_freq, &high_freq);
    if (status) {
        qdf_err("Error in wlan_reg_get_freq_range Err:%d",status);
        goto exit;
    }

    if (wlan_reg_is_range_overlap_6g(low_freq, high_freq) || wlan_reg_is_range_only6g(low_freq, high_freq)){
        wlan_global_6ghz_pdev_set(scn->soc->soc_idx,
                        wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj));
    }

    if(!(wlan_reg_is_range_only6g(low_freq, high_freq) || wlan_reg_is_range_overlap_6g(low_freq, high_freq))){
        if(ic->is_mbssid_enabled){
            ic->ic_enable_mlo_mbssid_lb = true;
            qdf_info("Enable Lower Band MLO MBSSID\n");
        }
    }

exit:
    mbss_info(":<ret:%d",ret);
    return ret;
}

static int ieee80211_mbss_detach(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn;
    struct mbssid_context *mbss_ctx;
    bool is_mbssid_enabled;
    uint8_t gp_id;

    mbss_info(":>");
    if (ic) {
        scn = OL_ATH_SOFTC_NET80211(ic);
        is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                WLAN_PDEV_F_MBSS_IE_ENABLE);

        if (is_mbssid_enabled && ic->ic_mbss_ctx) {
            mbss_ctx = ic->ic_mbss_ctx;
            for (gp_id = 0; gp_id < mbss_ctx->max_ngroups; gp_id++) {
                struct mbssid_info *ic_mbss = &mbss_ctx->ic_mbss[gp_id];

                if (ic_mbss->mbssid_init) {
                    qdf_mem_free(ic_mbss->mbss_offset);
                    ic_mbss->mbss_offset = NULL;
                    qdf_mem_free(ic_mbss->bcn_bo_mbss_ie);
                    ic_mbss->bcn_bo_mbss_ie = NULL;
                    qdf_mem_free(ic_mbss->prb_po_mbss_ie);
                    ic_mbss->prb_po_mbss_ie = NULL;
                    qdf_mem_free(ic_mbss->mbss_cache);
                    ic_mbss->mbss_cache = NULL;
                    qdf_spinlock_destroy(&ic_mbss->mbss_cache_lock);
                    qdf_mem_set(&ic_mbss->vdev_id_gp, sizeof(ic_mbss->vdev_id_gp),
                            IEEE80211_MBSSID_INVALID_VDEV_ID);
                }
            }

            if (mbss_ctx->ic_mbss) {
                qdf_mem_free(mbss_ctx->ic_mbss);
                mbss_ctx->ic_mbss = NULL;
            }

            qdf_mem_free(ic->ic_mbss_ctx);
            ic->ic_mbss_ctx = NULL;
        }
        wlan_global_6ghz_pdev_destroy(scn->soc->soc_idx,
                wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj));
    }
    mbss_info(":<");

    return 0;
}

/* We could have these for optimized resources allocation
 * per group at the time any entry gets added to the group.
 * De-alloc could happen when the last entry in the group
 * is removed. But if the group formation/dissolve happen
 * quite frequently then it may lead to memory fragmentation
 * due to frequent alloc/dealloc. So, we keep the per-group
 * alloc/dealloc in mbss_attach/detach function
 */
static int ieee80211_mbss_group_attach(struct ieee80211com *ic, uint8_t gp_id)
{
    return 0;
}

static int ieee80211_mbss_group_detach(struct ieee80211com *ic, uint8_t gp_id)
{
    return 0;
}

/*
 * Public MLME APIs (within UMAC, ieee80211_mlme_*)
 */
int ieee80211_mlme_attach(struct ieee80211com *ic)
{
    return ieee80211_mbss_attach(ic);
}

int ieee80211_mlme_detach(struct ieee80211com *ic)
{
    return ieee80211_mbss_detach(ic);
}

int ieee80211_mlme_vattach(struct ieee80211vap *vap)
{
    struct ieee80211com     *ic = vap->iv_ic;
    ieee80211_mlme_priv_t    mlme_priv;

#if 0
    vap->iv_debug |= IEEE80211_MSG_MLME;
#endif

    if (vap->iv_mlme_priv) {
        ASSERT(vap->iv_mlme_priv == 0);
        return -1; /* already attached ? */
    }

    mlme_priv = (ieee80211_mlme_priv_t) OS_MALLOC(ic->ic_osdev, (sizeof(struct ieee80211_mlme_priv)),0);

    if (mlme_priv == NULL) {
       return -ENOMEM;
    } else {
        vap->iv_mlme_priv = mlme_priv;
        OS_MEMZERO(mlme_priv, sizeof(*mlme_priv));
        mlme_priv->im_vap = vap;
        mlme_priv->im_osdev = ic->ic_osdev;
        OS_INIT_TIMER(ic->ic_osdev, &mlme_priv->im_timeout_timer,
                      timeout_callback, vap, QDF_TIMER_TYPE_WAKE_APPS);

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
        OS_INIT_TIMER(ic->ic_osdev, &mlme_priv->im_stacac_timeout_timer,
                      stacac_timeout_callback, vap, QDF_TIMER_TYPE_WAKE_APPS);
#endif

#if DBDC_REPEATER_SUPPORT
        qdf_timer_init(ic->ic_osdev, &mlme_priv->im_drop_mcast_timer,
                      drop_mcast_timeout_callback, vap, QDF_TIMER_TYPE_WAKE_APPS);
#endif
        /* Default configuration values */
        mlme_priv->im_disassoc_timeout = MLME_DEFAULT_DISASSOCIATION_TIMEOUT;
        switch(vap->iv_opmode) {
        case IEEE80211_M_STA:
            mlme_sta_vattach(vap);
            break;
        default:
            break;
        }


        return 0;
    }
}

int ieee80211_mlme_vdetach(struct ieee80211vap *vap)
{
    ieee80211_mlme_priv_t    mlme_priv = vap->iv_mlme_priv;
    int                      ftype;

    if (mlme_priv == NULL) {
        ASSERT(mlme_priv);
        return -1; /* already detached ? */
    }

    OS_CANCEL_TIMER(&mlme_priv->im_timeout_timer);
    OS_FREE_TIMER(&mlme_priv->im_timeout_timer);

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
    OS_CANCEL_TIMER(&mlme_priv->im_stacac_timeout_timer);
    OS_FREE_TIMER(&mlme_priv->im_stacac_timeout_timer);
#endif
#if DBDC_REPEATER_SUPPORT
    qdf_timer_free(&mlme_priv->im_drop_mcast_timer);
#endif
    switch(vap->iv_opmode) {
    case IEEE80211_M_STA:
        mlme_sta_vdetach(vap);
        break;
    default:
        break;
    }
    qdf_mem_free(mlme_priv);
    vap->iv_mlme_priv = NULL;

    /* Make sure we have release all the App IE */
    for (ftype = 0; ftype < IEEE80211_FRAME_TYPE_MAX; ftype++) {
        ASSERT(STAILQ_EMPTY(&vap->iv_app_ie_list[ftype]));
    }

    /* Free opt ie buffer */
    if (vap->iv_opt_ie.ie) {
        OS_FREE(vap->iv_opt_ie.ie);
        vap->iv_opt_ie.ie = NULL;
        vap->iv_opt_ie.length = 0;
    }

    if (vap->iv_beacon_copy_buf) {
        void *pTmp = vap->iv_beacon_copy_buf;
        vap->iv_beacon_copy_buf = NULL;
        vap->iv_beacon_copy_len = 0;
        OS_FREE(pTmp);
    }

    return 0;
}

#ifdef WLAN_FEATURE_11BE_MLO
QDF_STATUS wlan_mlme_mlo_alloc_primary_umac(struct ieee80211vap *vap,
                                            struct ieee80211_node *ni)
{
    struct wlan_mlo_dev_context *ml_dev;
    struct wlan_mlo_peer_context *ml_peer = NULL;
    struct wlan_objmgr_vdev *link_vdev;
    struct wlan_objmgr_vdev *tmp_link_vdevs[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    struct wlan_mlo_link_peer_entry *peer_entry;
    uint8_t i = 0, j = 0;

    ml_dev = vap->vdev_obj->mlo_dev_ctx;
    if (!ml_dev) {
        mlo_debug("ML dev ctx is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    ml_peer = ni->peer_obj->mlo_peer_ctx;
    if (!ml_peer) {
        mlo_debug("ML peer not found");
        return QDF_STATUS_E_NULL_VALUE;
    }

    if (!ml_dev->ap_ctx || !ml_dev->ap_ctx->mlo_link_reject) {
        mlo_err("Link rej disabled, skip PUMAC alloc");
        return QDF_STATUS_E_INVAL;
    }

    mlo_peer_lock_acquire(ml_peer);

    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        if (!peer_entry->link_peer)
            continue;

        /* If peer is rejected then skip that link */
        if (wlan_peer_is_assoc_rejected(peer_entry->link_peer)) {
            mlo_info("Skip rejected peer, skip " QDF_MAC_ADDR_FMT,
                     QDF_MAC_ADDR_REF(peer_entry->link_peer->macaddr));
            continue;
        }

        link_vdev = wlan_peer_get_vdev(peer_entry->link_peer);
        tmp_link_vdevs[j++] = link_vdev;
    }

    mlo_peer_allocate_primary_umac(ml_dev, ml_peer, tmp_link_vdevs);

    /* Assign primary umac for partner peers */
    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        if (!peer_entry->link_peer)
            continue;

        if (peer_entry->link_peer == ni->peer_obj)
            continue;

        if (wlan_peer_is_assoc_rejected(peer_entry->link_peer))
            continue;

        mlo_peer_assign_primary_umac(ml_peer, peer_entry);
    }

    mlo_peer_lock_release(ml_peer);

    wlan_cfg80211_prim_netdev_notification(vap, ni->peer_obj);
    return QDF_STATUS_SUCCESS;
}
#endif

#ifdef WLAN_SUPPORT_FILS
QDF_STATUS wlan_mlme_auth_fils(struct wlan_objmgr_vdev *vdev,
                struct ieee80211req_fils_aad *fils_aad, uint8_t *macaddr)
{
    struct wlan_crypto_req_key req_key;
    struct wlan_objmgr_peer *peer = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;
    uint8_t pdev_id;
    QDF_STATUS status = QDF_STATUS_SUCCESS;

    if (!vdev || !fils_aad) {
        qdf_print("%s: Invalid Input", __func__);
        return QDF_STATUS_E_INVAL;
    }
    pdev_id = wlan_objmgr_pdev_get_pdev_id(wlan_vdev_get_pdev(vdev));

    if ((!fils_aad->kek_len) ||
        (fils_aad->kek_len > WLAN_MAX_WPA_KEK_LEN)) {
        qdf_print("%s: Invalid Key length (%d)", __func__, fils_aad->kek_len);
        return QDF_STATUS_E_INVAL;
    }

    psoc = wlan_vdev_get_psoc(vdev);
    if (!psoc) {
        qdf_print("%s: Psoc is NULL", __func__);
        return QDF_STATUS_E_INVAL;
    }

    peer = wlan_objmgr_get_peer_by_mac_n_vdev(psoc, pdev_id,
                wlan_vdev_mlme_get_macaddr(vdev), macaddr, WLAN_MGMT_SB_ID);
    if (!peer) {
        qdf_print("%s: Invalid peer", __func__);
        return QDF_STATUS_E_INVAL;
    }

    qdf_mem_zero(&req_key, sizeof(struct wlan_crypto_req_key));
    req_key.type = WLAN_CRYPTO_CIPHER_FILS_AEAD;
    req_key.keyix = WLAN_CRYPTO_KEYIX_NONE;
    req_key.flags = (WLAN_CRYPTO_KEY_XMIT | WLAN_CRYPTO_KEY_RECV);
    qdf_mem_copy(req_key.macaddr, macaddr, QDF_MAC_ADDR_SIZE);
    qdf_mem_copy(&req_key.filsaad, fils_aad, sizeof(struct wlan_crypto_fils_aad_key));

    status = wlan_crypto_setkey(vdev, &req_key);
    if (status != QDF_STATUS_SUCCESS) {
        qdf_print("%s: FILS crypto reg Failed", __func__);
        wlan_crypto_set_peer_fils_aead(peer, 0);
    } else {
        wlan_crypto_set_peer_fils_aead(peer, 1);
    }
    wlan_objmgr_peer_release_ref(peer, WLAN_MGMT_SB_ID);

    /* Zero-out local key variables */
    qdf_mem_zero(&req_key, sizeof(struct wlan_crypto_req_key));
    return status;
}
#endif

int wlan_mlme_auth(wlan_if_t vaphandle, u_int8_t *macaddr, u_int16_t seq, u_int16_t status,
                   u_int8_t *challenge_txt, u_int8_t challenge_len,
                   struct ieee80211_app_ie_t* optie)
{
    struct ieee80211vap      *vap = vaphandle;
    struct ieee80211_node    *ni;
    int                      error = 0;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s status %d \n", __func__, status);


    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_HANDLER_ID);
    if (ni == NULL || (ni->ni_ic == NULL)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s ni->ni_ic is NULL for ni(%pK)\n",__func__,ni);
        error = -ENOMEM;
        goto exit;
    }

    /* Send auth frame */
    if (status != IEEE80211_STATUS_SUCCESS) {
        ni->ni_authstatus = status;
    }

    IEEE80211_DELIVER_EVENT_MLME_AUTH_COMPLETE(vap, ni->ni_macaddr, ni->ni_authstatus);

    ieee80211_send_auth(ni, seq, ni->ni_authstatus, challenge_txt, challenge_len, NULL, optie);

    if (ni->ni_authstatus != IEEE80211_STATUS_SUCCESS) {
        /* auth is not success, remove the node from node table */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s auth failed with status %d \n", __func__, ni->ni_authstatus);
#ifdef WLAN_FEATURE_11BE_MLO
        if (wlan_peer_is_mlo(ni->peer_obj)) {
            wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
        }
#endif
        IEEE80211_NODE_LEAVE(ni);
        error = -EIO;
    }

exit :
    /* claim node immediately */
    if (ni)
        ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);

    return error;
}

#ifdef WLAN_FEATURE_11BE_MLO
static bool is_link_peer_adjacent(struct wlan_mlo_peer_context *ml_peer)
{
    struct wlan_mlo_link_peer_entry *peer_entry;
    struct wlan_objmgr_peer *link_peer;
    uint8_t psoc_id[MAX_MLO_LINK_PEERS];
    uint8_t is_adjacent;
    uint8_t i, ix = 0;

    if (!ml_peer)
        return true;

    mlo_peer_lock_acquire(ml_peer);

    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        link_peer = peer_entry->link_peer;

        if (!link_peer) {
            continue;
        }

        if (wlan_peer_is_assoc_rejected(link_peer))
            continue;

        psoc_id[ix] = wlan_peer_get_psoc_id(link_peer);

        ix++;
    }

    mlo_peer_lock_release(ml_peer);

    /* Peers are adjacent if link peers are more than 2 */
    if (ix != WLAN_NUM_TWO_LINK_PSOC)
        return true;

    if ((QDF_STATUS_SUCCESS == mlo_chip_adjacent(psoc_id[0], psoc_id[1], &is_adjacent)) &&
        is_adjacent)
        return true;
    else
        qdf_info("Adjacency check failed");

    return false;
}

static void update_partner_link_assoc_rsp_buf_with_status(struct ieee80211_node *ni)
{
    struct wlan_objmgr_vdev *link_vdev;
    struct wlan_objmgr_peer *link_peer;
    struct ieee80211_node *link_ni;
    struct wlan_mlo_peer_context *ml_peer = ni->peer_obj->mlo_peer_ctx;
    struct wlan_mlo_link_peer_entry *peer_entry;
    wlan_if_t partner_vap;
    qdf_nbuf_t assocnbuf;
    uint8_t i, vdev_link_id, *assocbuf,  *frm;
    size_t assocbuf_len;
    struct ieee80211_frame *wh;

    if (!ml_peer)
        return;

    mlo_peer_lock_acquire(ml_peer);

    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        link_peer = peer_entry->link_peer;

        if (!link_peer) {
            continue;
        }

        if (!mlo_peer_is_assoc_peer(ml_peer, link_peer)) {
            link_vdev = wlan_peer_get_vdev(link_peer);
            if (!link_vdev)
                continue;

            link_ni = wlan_peer_get_mlme_ext_obj(link_peer);
            if (!link_ni)
                continue;

            /* Donot update frame assoc status if accepted by driver and hostapd */
            if (!wlan_peer_is_assoc_rejected(link_peer))
                continue;

            if (!ieee80211_try_ref_node(link_ni, WLAN_MLME_HANDLER_ID))
                continue;

            if (link_ni->ni_assocstatus == IEEE80211_STATUS_SUCCESS)
                link_ni->ni_assocstatus = IEEE80211_STATUS_UNSPECIFIED;

            vdev_link_id = wlan_vdev_get_link_id(link_vdev);
            partner_vap = wlan_vdev_get_mlme_ext_obj(link_vdev);

            mlo_peer_lock_release(ml_peer);
            assocnbuf = mlo_peer_get_link_peer_assoc_resp_buf(ml_peer, vdev_link_id);
            if (assocnbuf && partner_vap) {
                assocbuf = qdf_nbuf_data(assocnbuf);
                assocbuf_len = qdf_nbuf_len(assocnbuf);
                wh = (struct ieee80211_frame *)assocbuf;
                frm  = (uint8_t *)&wh[1];
                frm += 2;

                /* Assoc frame status code update */
                *(u_int16_t *)frm = htole16(link_ni->ni_assocstatus);

                qdf_info("Updated STA partner link status %d\n", link_ni->ni_assocstatus);
             }
            ieee80211_free_node(link_ni, WLAN_MLME_HANDLER_ID);
            mlo_peer_lock_acquire(ml_peer);
        }
    }

    mlo_peer_lock_release(ml_peer);
}

static void cleanup_rejected_partner_links(struct wlan_mlo_peer_context *ml_peer)
{
    struct wlan_objmgr_vdev *link_vdev;
    struct wlan_objmgr_peer *link_peer;
    struct ieee80211_node *link_ni;
    struct wlan_mlo_link_peer_entry *peer_entry;
    uint8_t i;

    if (!ml_peer)
        return;

    mlo_peer_lock_acquire(ml_peer);

    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        link_peer = peer_entry->link_peer;

        if (!link_peer)
            continue;

        if (!mlo_peer_is_assoc_peer(ml_peer, link_peer)) {
            link_vdev = wlan_peer_get_vdev(link_peer);
            if (!link_vdev)
                continue;

            link_ni = wlan_peer_get_mlme_ext_obj(link_peer);
            if (!link_ni)
                continue;

            if (!wlan_peer_is_assoc_rejected(link_peer))
                continue;

            if (!ieee80211_try_ref_node(link_ni, WLAN_MLME_HANDLER_ID))
                continue;

            mlo_peer_lock_release(ml_peer);
            qdf_info("Cleanup STA partner link: assoc status %d", link_ni->ni_assocstatus);
            IEEE80211_NODE_LEAVE(link_ni);
            ieee80211_free_node(link_ni, WLAN_MLME_HANDLER_ID);
            mlo_peer_lock_acquire(ml_peer);
        }
    }

    mlo_peer_lock_release(ml_peer);
}

static void mlo_partner_peers_rearrange(struct wlan_mlo_peer_context *ml_peer)
{
    struct wlan_mlo_link_peer_entry tmp_peer_list[MAX_MLO_LINK_PEERS];
    struct wlan_mlo_link_peer_entry rej_peer_list[MAX_MLO_LINK_PEERS];
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    uint8_t rej_cnt = 0;
    uint8_t cnt = 0;
    uint8_t i;

    if (!ml_peer)
        return;

    mlo_peer_lock_acquire(ml_peer);

    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        if (!peer_entry->link_peer)
            continue;

        if (wlan_peer_is_assoc_rejected(peer_entry->link_peer)) {
            rej_peer_list[rej_cnt] = *peer_entry;
            rej_cnt++;
            continue;
        }

        tmp_peer_list[cnt] = *peer_entry;
        cnt++;
    }

    /* Move the rejected peers to the end of the list, before sending assoc command
     * to FW and to avoid holes once freed */
    if ((rej_cnt + cnt) <= MAX_MLO_LINK_PEERS) {
        qdf_mem_copy(&ml_peer->peer_list[0], &tmp_peer_list[0], sizeof(struct wlan_mlo_link_peer_entry) * cnt);
        qdf_mem_copy(&ml_peer->peer_list[cnt], &rej_peer_list[0], sizeof(struct wlan_mlo_link_peer_entry) * rej_cnt);
    }

    mlo_peer_lock_release(ml_peer);
}
#endif

int wlan_mlme_assoc_resp(wlan_if_t vaphandle, u_int8_t *macaddr, IEEE80211_REASON_CODE reason, int reassoc,
                         struct ieee80211_app_ie_t* optie)
{
    struct ieee80211com      *ic = NULL;
    struct ieee80211vap      *vap = vaphandle;
    struct ieee80211_node    *ni;
    int                      error = 0;
#if WLAN_SUPPORT_SPLITMAC
    struct wlan_objmgr_vdev *vdev = vap->vdev_obj;
#endif
    int is_splitmac_enable = 0;
#ifdef WLAN_FEATURE_11BE_MLO
    int retval = 0;
    bool mlo_link_reject = false;
#endif
#ifdef WLAN_FEATURE_11BE
    struct wlan_mlo_peer_context *ml_peer = NULL;
    enum wlan_t2lm_resp_frm_type t2lm_resp_type = WLAN_T2LM_RESP_TYPE_INVALID;
#endif /* WLAN_FEATURE_11BE */

    ic = vaphandle->iv_ic;

#if WLAN_SUPPORT_SPLITMAC
    is_splitmac_enable = splitmac_is_enabled(vdev);
#else
    is_splitmac_enable = 0;
#endif

    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_HANDLER_ID);
    if (ni == NULL) {
        /* In case the assoc resp is temp rejet and the node is not found, use bss peer
         * to transmit assoc resp frame. This is to handle a scenario where auth is received
         * with mac address same as MLD mac and SA query is required to be sent before deleting
         * older entry
         */
        if (wlan_vap_is_pmf_enabled(vap) && vap->iv_opmode == IEEE80211_M_HOSTAP &&
            reason == IEEE80211_STATUS_REJECT_TEMP) {
            wbuf_t wbuf;
            ni = vap->iv_bss;
            mlme_debug("Send assoc resp using BSS peer");
            wbuf = ieee80211_setup_assocresp(ni, NULL, reassoc, reason, optie, NULL, NULL, macaddr);
            if (!wbuf)
                return ENOMEM;
            error = ieee80211_send_mgmt(vap,ni, wbuf,false);
            return error;
        } else {
            error = -EIO;
            return error;
        }
    }

    IEEE80211_NOTE(vap, IEEE80211_MSG_MLME, ni,
                   "%s reason %d reassoc %d \n", __func__,
                   reason, reassoc);

    ni->ni_assocstatus = reason;
#if WLAN_SUPPORT_SPLITMAC
    if (is_splitmac_enable) {
        u_int16_t aid;

        IEEE80211_NODE_STATE_LOCK(ni);
        if (ni->ni_associd == 0) {
#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_vdev_mlme_is_mlo_ap(vdev) || (ic->is_mbssid_enabled)) {
                if (!wlan_peer_is_mlo(ni->peer_obj)) {
                    aid = mlme_get_aid(vap->vdev_obj);
                    if (aid == (uint16_t)-1) {
                        IEEE80211_NODE_STATE_UNLOCK(ni);
                        ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);
                        return -EIO;
                    }
                    else {
                        ni->ni_associd = aid;
                    }
                }
            }
            else
#endif
            {
                aid = ieee80211_get_vap_start_aid(ic, vap);
                /*
                 * It would be good to search the bitmap
                 * more efficiently, but this will do for now.
                 */
                for (; aid < vap->iv_max_aid; aid++) {
                    if (!IEEE80211_AID_ISSET(vap, aid))
                        break;
                }

                if (aid >= vap->iv_max_aid) {
                    /*
                     * Keep stats on this situation.
                     */
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC, "aid (%d)"
                            " greater than max aid (%d)\n", aid,
                            vap->iv_max_aid);
                    IEEE80211_NODE_STATE_UNLOCK(ni);
                    IEEE80211_NODE_LEAVE(ni);
                    ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);
                    return -1;
                }

                ni->ni_associd = aid | IEEE80211_RESV_AID_BITS;
                IEEE80211_AID_SET(vap, ni->ni_associd);
            }
        }
        IEEE80211_NODE_STATE_UNLOCK(ni);
    }
#endif /* WLAN_SUPPORT_SPLITMAC */

    /*
     * The ni might have changed before this assoc resp has come from hostapd.
     * Check this using the associd and return here
     */
    if (ni && (ni != vap->iv_bss) && (ni->ni_associd == 0)) {
        /*
         * During intra band roaming, the hostapd initiates SA Query to the
         * initially connected Vap and sends assoc reject temp to the currently
         * roaming AP. Also, The ni_associd will be Zero to currently roamed vap
         * since we will avoid creatingl peer because it is already exists. So,
         * in case intraband roamingvoid dropping the assoc resp if status is
         * reject temporary.
         */
#ifdef WLAN_FEATURE_11BE_MLO
        if (!(wlan_peer_is_mlo(ni->peer_obj) && ni->ni_ml_peer_created &&
              reason == IEEE80211_STATUS_REJECT_TEMP))
#endif
        {
            ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);
            error = -EIO;
            return error;
        }
    }

#ifdef WLAN_FEATURE_11BE_MLO
#ifdef WLAN_FEATURE_11BE
    ml_peer = ni->peer_obj->mlo_peer_ctx;
#endif

    if (vap->vdev_obj->mlo_dev_ctx &&
        vap->vdev_obj->mlo_dev_ctx->ap_ctx &&
        vap->vdev_obj->mlo_dev_ctx->ap_ctx->mlo_link_reject)
        mlo_link_reject = true;

    /* Check the adjacency of accepted link peers and discard association if not adjacent */
    if (ni->ni_assocstatus == IEEE80211_STATUS_SUCCESS &&
        ml_peer && mlo_link_reject && !is_link_peer_adjacent(ml_peer)) {
        ni->ni_assocstatus = IEEE80211_STATUS_UNSPECIFIED;
        reason = ni->ni_assocstatus;
    }

    if (ni->ni_assocstatus == IEEE80211_STATUS_SUCCESS) {
        if(!(ieee80211_is_pmf_enabled(vap, ni)) ||  !(ieee80211node_has_flag(ni,IEEE80211_NODE_AUTH))){
            u_int8_t newassoc = !ieee80211node_has_extflag(ni, IEEE80211_NODE_ASSOC_RESP);

            if (((ic->ic_newassoc != NULL) && !(ni->is_ft_reassoc)) && !is_splitmac_enable) {
#ifdef WLAN_FEATURE_11BE
                if (ml_peer) {
                    t2lm_resp_type = ml_peer->t2lm_policy.ongoing_tid_to_link_mapping.t2lm_resp_type;

                    if (wlan_peer_mlme_is_assoc_peer(ni->peer_obj) &&
                            (t2lm_resp_type == WLAN_T2LM_RESP_TYPE_SUCCESS)) {
                        /* Copy the ongoing T2LM info to successfully negotiated info
                         * structure.
                         */
                        ieee80211_copy_negotiated_t2lm_info_to_peer_ctx(ml_peer,
                                &ml_peer->t2lm_policy.ongoing_tid_to_link_mapping, false);
                    }
                }
#endif /* WLAN_FEATURE_11BE */
                mlo_partner_peers_rearrange(ml_peer);

              /* Allocate primary UMAC */
                if (newassoc)
                    wlan_mlme_mlo_alloc_primary_umac(vap, ni);

                retval = ic->ic_newassoc(ni, newassoc);
                if (retval) {
                    qdf_err("node %s, failed to send peer assoc command, disconnect the STA", ether_sprintf(ni->ni_macaddr));
                    ieee80211_send_assocresp(ni, reassoc, IEEE80211_STATUS_INVALID_PARAM, optie);

#ifdef WLAN_FEATURE_11BE_MLO
                    if (wlan_peer_is_mlo(ni->peer_obj)) {
                        wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
                    }
#endif
                    if(IEEE80211_NODE_LEAVE(ni)) {
#ifdef WLAN_FEATURE_11BE_MLO
                       IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                 ni->ni_macaddr, ni->ni_associd, IEEE80211_STATUS_UNSPECIFIED, &ni->peer_obj->mldaddr[0]);
#else
                       IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                 ni->ni_macaddr, ni->ni_associd, IEEE80211_STATUS_UNSPECIFIED, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
                    }
                    ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);
                    error = -EIO;
                    return error;
                }

                if (wlan_peer_is_mlo(ni->peer_obj))
                    wlan_mlo_partner_peer_assoc_post(ni->peer_obj);

                /* In case of OMN in Asoc req, phymode is not updated
                 * and therefore we explicitly send chwidth in OMN
                 * to target. This is confirmed by checking if chan width
                 * obtained from phymode and peer ch width differ.
                 */
                if (ni->ni_chwidth != get_chwidth_phymode(ni->ni_phymode))
                    ic->ic_chwidth_change(ni);
                ieee80211node_set_extflag(ni, IEEE80211_NODE_ASSOC_RESP);
                ieee80211_update_noderates(ni);
            }
        }
    }
    /*
     * Node is pmf enabled, authorized, and ml_peer is created for that node
     * during previous assoc req processing. During duplicate assoc req cases,
     * it is expected that SA Query would fail and assoc resp is sent with
     * failure.
     *
     * Hostapd sent SUCCESS for this assoc request, which should not happen.
     * Hence, dropping this frame.
     */
    if ((wlan_peer_is_mlo(ni->peer_obj)) && (ni->peer_obj->mlo_peer_ctx)) {
        if ((!ni->ni_ml_peer_created) &&
            (reason == IEEE80211_STATUS_SUCCESS)) {
            reason = IEEE80211_STATUS_UNSPECIFIED;
            qdf_err("dropping assoc as frame only processed in hostapd with reason code: %d \n",
                    reason);
        }
    }

    /* Iterate and update the frame with assoc status
     * for partner links */
    if (reason == IEEE80211_STATUS_SUCCESS && wlan_peer_is_mlo(ni->peer_obj) && ml_peer
        && mlo_link_reject)
        update_partner_link_assoc_rsp_buf_with_status(ni);
#endif

    /* Send assoc frame */
    error = ieee80211_send_assocresp(ni, reassoc, reason, optie);

#ifdef WLAN_FEATURE_11BE_MLO
    /* Iterate and free the assoc rejected links */
    if (reason == IEEE80211_STATUS_SUCCESS && wlan_peer_is_mlo(ni->peer_obj) && ml_peer
        && mlo_link_reject)
        cleanup_rejected_partner_links(ml_peer);
#endif

    if (error || !((reason == IEEE80211_STATUS_SUCCESS) ||
                   (reason == IEEE80211_STATUS_ANTI_CLOGGING_TOKEN_REQ))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s assoc failed with status %d \n", __func__, reason);

	if (!ieee80211_node_is_authorized(ni)) {
            /* Remove the node from node table only if node is not authorized */
#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_peer_is_mlo(ni->peer_obj)) {
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
            }
#endif
            /* Remove the node from node table only in non-PMF assoc
             * or reason is other than REJECT_TEMP and INVALID_PMKID
             */
            if(IEEE80211_NODE_LEAVE(ni)) {
#ifdef WLAN_FEATURE_11BE_MLO
                IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                 macaddr, ni->ni_associd, IEEE80211_STATUS_UNSPECIFIED, &ni->peer_obj->mldaddr[0]);
#else
                IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                 macaddr, ni->ni_associd, IEEE80211_STATUS_UNSPECIFIED, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
            }
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s STA already authorized not removing node for reason %d\n",
                                 __func__, reason);
        }
    }
#ifdef WLAN_FEATURE_11BE_MLO
    if (!error && ni->ni_assocstatus == IEEE80211_STATUS_SUCCESS) {
        if(!(ieee80211_is_pmf_enabled(vap, ni)) ||  !(ieee80211node_has_flag(ni,IEEE80211_NODE_AUTH))) {
            if (!(ni->is_ft_reassoc) && !is_splitmac_enable) {
                ieee80211node_set_extflag(ni, IEEE80211_NODE_ASSOC_RESP);
                ieee80211_update_noderates(ni);
            }
        }
    }
#endif
    /* claim node immediately */
    ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);

    return error;
}

#ifdef WLAN_FEATURE_11BE_MLO
/**
 * wlan_initialize_default_mld_values() - Initialize MLD cap values in ML Peer
 * @ml_peer: Pointer to ML Peer
 * Return: Nothing
 */
static void wlan_initialize_default_mld_values(struct wlan_mlo_peer_context *ml_peer)
{
    struct ieee80211_node *ni = NULL;

    ni = ieee80211_ucfg_get_mlo_assoc_link_node(ml_peer);
    if (!ni || !ml_peer)
        return;

    qdf_mem_copy(&ml_peer->mlpeer_mldcap, &ni->mlpeer_mld_cap,
                 sizeof(struct wlan_mlo_mld_cap));
    wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
}

/**
 * wlan_initialize_default_eml_values() - Initialize EML values in ML Peer
 * @ml_peer: Pointer to ML Peer
 * Return: Nothing
 */
static void wlan_initialize_default_eml_values(struct wlan_mlo_peer_context *ml_peer)
{
    struct ieee80211_node *ni = NULL;

    ni = ieee80211_ucfg_get_mlo_assoc_link_node(ml_peer);
    if (!ni || !ml_peer)
        return;

    qdf_mem_copy(&ml_peer->mlpeer_emlcap, &ni->mlpeer_eml_cap,
                 sizeof(struct wlan_mlo_eml_cap));
    wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
}

/**
 * wlan_initialize_default_nstr_values() - Initialize NSTR info values in ML Peer
 * @ml_peer: Pointer to ML Peer
 * Return: Nothing
 */
static void wlan_initialize_default_nstr_values(struct wlan_mlo_peer_context *ml_peer)
{
    struct ieee80211_node *ni = NULL;

    if (!ml_peer)
        return;

    ni = ieee80211_ucfg_get_mlo_assoc_link_node(ml_peer);
    if (ni) {
        qdf_mem_copy(&ml_peer->mlpeer_nstrinfo, &ni->ml_info.nstr_info,
                     sizeof(struct mlo_nstr_info) * WLAN_UMAC_MLO_MAX_VDEVS);
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
    }
}

void wlan_mlme_peer_assoc_resp(struct wlan_objmgr_peer *peer)
{
    struct ieee80211_node *ni;
    int error;
    int retval = 0;
    struct ieee80211vap *vap;
    int is_splitmac_enable = 0;
    struct ieee80211com *ic;
    struct wlan_mlo_peer_context *ml_peer;
    struct wlan_mlo_dev_context *ml_dev;

    if (!wlan_peer_is_mlo(peer))
        return;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni)
        return;

    vap = ni->ni_vap;
    if (!wlan_peer_mlme_is_assoc_peer(peer)) {
        mlo_err("vdev%d: This is not assoc peer, so returning for MAC addr "QDF_MAC_ADDR_FMT,
                 vap->iv_unit, QDF_MAC_ADDR_REF(peer->mldaddr));
        return;
    }

    ic = vap->iv_ic;
    ni->ni_ml_link_peer_created = 1;

    ml_peer = peer->mlo_peer_ctx;
    if (!ml_peer)
        return;

    wlan_initialize_default_mld_values(ml_peer);
    wlan_initialize_default_t2lm_values(ml_peer);

    ml_dev = vap->vdev_obj->mlo_dev_ctx;
    if ((wlan_vdev_mlme_get_opmode(vap->vdev_obj) == QDF_SAP_MODE) &&
        ml_dev && !ml_dev->ap_ctx->mlo_link_reject)
        wlan_cfg80211_prim_netdev_notification(vap, peer);

    wlan_initialize_default_eml_values(ml_peer);
    wlan_initialize_default_nstr_values(ml_peer);

    if (wlan_mlo_peer_is_nawds(ml_peer)) {
        wlan_mlnawds_assoc_peer(vap, ni, wlan_peer_get_macaddr(peer));
        if (wlan_peer_is_mlo(peer))
            wlan_mlo_partner_peer_assoc_post(peer);
        return;
    }

    if (wlan_mlo_peer_is_mesh(ml_peer) &&
        (wlan_mlo_peer_is_assoc_done(ml_peer) != QDF_STATUS_SUCCESS)) {
        wlan_mlmesh_assoc_peer(vap, ni, wlan_peer_get_macaddr(peer));
        if (wlan_peer_is_mlo(peer))
            wlan_mlo_partner_peer_assoc_post(peer);
        return;
    }

#if WLAN_SUPPORT_SPLITMAC
    is_splitmac_enable = splitmac_is_enabled(vap->vdev_obj);
#endif

    if ((is_splitmac_enable) &&
        (wlan_mlo_peer_is_assoc_done(ml_peer) != QDF_STATUS_SUCCESS)) {
       if (!(ieee80211_is_pmf_enabled(vap, ni)) ||
             !(ni->ni_flags & IEEE80211_NODE_AUTH)) {
          u_int8_t newassoc = !ieee80211node_has_extflag(ni, IEEE80211_NODE_ASSOC_RESP);
          if ((ic->ic_newassoc != NULL) && !(ni->is_ft_reassoc)) {
              retval = ic->ic_newassoc(ni, newassoc);
              if (retval) {
                   qdf_err("node %s, failed to send peer assoc command, disconnect the STA", ether_sprintf(ni->ni_macaddr));
                   ieee80211_send_assocresp(ni, ni->is_ft_reassoc, IEEE80211_STATUS_INVALID_PARAM, NULL);

#ifdef WLAN_FEATURE_11BE_MLO
                    if (wlan_peer_is_mlo(ni->peer_obj)) {
                        wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
                    }
#endif
                   if (IEEE80211_NODE_LEAVE(ni)) {
                       IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                 ni->ni_macaddr, ni->ni_associd, IEEE80211_STATUS_UNSPECIFIED, &ni->peer_obj->mldaddr[0]);
                   }
                   return;
              }

             if (wlan_peer_is_mlo(peer))
                 wlan_mlo_partner_peer_assoc_post(peer);
             /* In case of OMN in Asoc req, phymode is not updated
              * and therefore we explicitly send chwidth in OMN
              * to target. This is confirmed by checking if chan width
              * obtained from phymode and peer ch width differ.
              */
             if (ni->ni_chwidth != get_chwidth_phymode(ni->ni_phymode))
                 ic->ic_chwidth_change(ni);
          }
       }
    }

    if (ni->ni_hostapd_asresp_trig) {
       if (is_splitmac_enable) {
           /* initiate assoc resp */
          wlan_send_mgmt(vap, ni->ni_macaddr, wbuf_header(ni->ni_assoc_resp_wbuf),
                         wbuf_get_pktlen(ni->ni_assoc_resp_wbuf));
          wbuf_free(ni->ni_assoc_resp_wbuf);
          ieee80211node_set_extflag(ni, IEEE80211_NODE_ASSOC_RESP);
          ni->ni_assoc_resp_wbuf = NULL;
       } else {
          if (ni->ni_assocstatus == IEEE80211_STATUS_SUCCESS) {
              if(!(ieee80211_is_pmf_enabled(vap, ni)) ||  !(ieee80211node_has_flag(ni,IEEE80211_NODE_AUTH))){
                  u_int8_t newassoc = !ieee80211node_has_extflag(ni, IEEE80211_NODE_ASSOC_RESP);

                  if (((ic->ic_newassoc != NULL) && !(ni->is_ft_reassoc)) && !is_splitmac_enable) {
                      retval = ic->ic_newassoc(ni, newassoc);
                      if (retval) {
                          qdf_err("node %s, failed to send peer assoc command, disconnect the STA", ether_sprintf(ni->ni_macaddr));
                          ieee80211_send_assocresp(ni, ni->is_ft_reassoc, IEEE80211_STATUS_INVALID_PARAM, NULL);

#ifdef WLAN_FEATURE_11BE_MLO
                          if (wlan_peer_is_mlo(ni->peer_obj)) {
                              wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
                           }
#endif
                          if (IEEE80211_NODE_LEAVE(ni)) {
                              IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                         ni->ni_macaddr, ni->ni_associd, IEEE80211_STATUS_UNSPECIFIED, &ni->peer_obj->mldaddr[0]);
                          }
                          return;
                      }

                      if (wlan_peer_is_mlo(peer))
                          wlan_mlo_partner_peer_assoc_post(peer);

                      /* In case of OMN in Asoc req, phymode is not updated
                       * and therefore we explicitly send chwidth in OMN
                       * to target. This is confirmed by checking if chan width
                       * obtained from phymode and peer ch width differ.
                       */
                      if (ni->ni_chwidth != get_chwidth_phymode(ni->ni_phymode))
                          ic->ic_chwidth_change(ni);

                      ieee80211node_set_extflag(ni, IEEE80211_NODE_ASSOC_RESP);
                      ieee80211_update_noderates(ni);
                  }
              }
          }
          error = ieee80211_send_assocresp(ni, ni->ni_ml_reassoc,
                                           ni->ni_ml_assoc_resp_reason,
                                           &ni->ni_assoc_resp_optie);

          if (error)  {
              IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s assoc failed with status %d \n", __func__,
                                ni->ni_ml_assoc_resp_reason);
              if (!ieee80211_node_is_authorized(ni)) {
                  /* Remove the node from node table only if node is not authorized */
                  if (wlan_peer_is_mlo(peer)) {
                      wlan_mlo_partner_peer_disconnect_notify(peer);
                  }
                  /* Remove the node from node table only in non-PMF assoc
                   * or reason is other than REJECT_TEMP and INVALID_PMKID
                   */
                  if(IEEE80211_NODE_LEAVE(ni)) {
                      IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                       ni->ni_macaddr, ni->ni_associd, IEEE80211_STATUS_UNSPECIFIED, &ni->peer_obj->mldaddr[0]);
                  }
              } else {
                  IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s STA already authorized not removing node for reason %d\n",
                                    __func__, ni->ni_ml_assoc_resp_reason);
              }
          }
          qdf_mem_free(ni->ni_assoc_resp_optie.ie);
          ni->ni_assoc_resp_optie.ie = NULL;
          ni->ni_assoc_resp_optie.length = 0;

          if (!error && ni->ni_assocstatus == IEEE80211_STATUS_SUCCESS) {
              if(!(ieee80211_is_pmf_enabled(vap, ni)) ||  !(ieee80211node_has_flag(ni,IEEE80211_NODE_AUTH))) {
                  if (!(ni->is_ft_reassoc) && !is_splitmac_enable) {
                      ieee80211node_set_extflag(ni, IEEE80211_NODE_ASSOC_RESP);
                      ieee80211_update_noderates(ni);
                  }
              }
          }
       }
    }
}
#else
static void wlan_initialize_default_mld_values(struct wlan_mlo_peer_context *ml_peer)
{
}
static void wlan_initialize_default_eml_values(struct wlan_mlo_peer_context *ml_peer)
{
}
static void wlan_initialize_default_nstr_values(struct wlan_mlo_peer_context *ml_peer)
{
}
#endif //WLAN_FEATURE_11BE_MLO

/*
 * Public MLME APIs (external to UMAC, wlan_mlme_*)
 */

int wlan_mlme_deauth_request(wlan_if_t vaphandle, u_int8_t *macaddr, IEEE80211_REASON_CODE reason)
{
    struct ieee80211vap      *vap = vaphandle;
    struct ieee80211_node    *ni;
    int                      error = 0;
    bool delayed_cleanup = false;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    /*
     * if a node exist with the given address already , use it.
     * if not use bss node.
     */
    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_HANDLER_ID);
    if (ni == NULL) {
        if(vap->iv_opmode == IEEE80211_M_STA){
            if(!wlan_vap_is_pmf_enabled(vap)){
                error = -ENOMEM;
                goto exit;
            }
        } else {
            if (!IEEE80211_ADDR_EQ(macaddr, IEEE80211_GET_BCAST_ADDR(vap->iv_ic)))
            {
                error = -EIO;
                goto exit;
            }
        }

        ni = ieee80211_try_ref_node(vap->iv_bss, WLAN_MLME_HANDLER_ID);
        if (!ni) {
            error = -EFAULT;
            goto exit;
        }

    }

    /* Send deauth frame */
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_AUTH, "%s: sending DEAUTH to %s, mlme deauth reason %d\n",
            __func__, ether_sprintf(ni->ni_macaddr), reason);

    delayed_cleanup = ieee80211node_has_flag(ni, IEEE80211_NODE_DELAYED_CLEANUP);
    error = ieee80211_send_deauth(ni, reason);

    /* Node needs to be removed from table as well, do it only for AP/IBSS now */
    if (vap->iv_opmode == IEEE80211_M_HOSTAP && ni != vap->iv_bss) {
        if (!delayed_cleanup ||
            ieee80211node_has_extflag(ni, IEEE80211_NODE_DELAYED_CLEANUP_FAIL)) {

#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_peer_is_mlo(ni->peer_obj)) {
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
            }
#endif
            IEEE80211_NODE_LEAVE(ni);
#ifdef WLAN_FEATURE_11BE_MLO
            IEEE80211_DELIVER_EVENT_MLME_DEAUTH_COMPLETE(vap,macaddr,IEEE80211_STATUS_SUCCESS,ni->ni_associd,reason, &ni->peer_obj->mldaddr[0]);
#else
            IEEE80211_DELIVER_EVENT_MLME_DEAUTH_COMPLETE(vap,macaddr,IEEE80211_STATUS_SUCCESS,ni->ni_associd,reason, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
        }
    }

    /* claim node immediately */
    ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);

    if (error) {
        goto exit;
    }

exit:
    return error;
}

#ifdef WLAN_FEATURE_11BE
/**
 * ieee80211_pmf_get_action_offset() - Get the action frame offset for PMF
 * frames
 * @wbuf: Pointer to wbuf
 *
 * Return: Pointer to action frame offset
 */
static struct ieee80211_action *ieee80211_pmf_get_action_offset(wbuf_t wbuf)
{
    uint16_t buflen = 0, len = 0;
    uint8_t *data, *ivp = NULL, *data_ptr;
    struct ieee80211_frame *wh = (struct ieee80211_frame *)wbuf_header(wbuf);

    data = (uint8_t *)qdf_nbuf_data(wbuf);
    buflen = qdf_nbuf_len(wbuf);
    data_ptr = data + sizeof(struct ieee80211_frame);

    if (wh->i_fc[1] & IEEE80211_FC1_ORDER) {
        data_ptr += IEEE80211_HT_CTRL_LEN;
        len = IEEE80211_HT_CTRL_LEN;
    }

    if (buflen > (sizeof(struct ieee80211_frame) + WLAN_HDR_EXT_IV_LEN))
        ivp = data + sizeof(struct ieee80211_frame) + len;

    /* Set data_ptr based on EXT IV bit
     * if EXT IV bit set, CCMP using PMF 8 bytes of IV is present
     * else for WEP using PMF, 4 bytes of IV is present
     */
    if (ivp && (ivp[WLAN_HDR_IV_LEN] & WLAN_HDR_EXT_IV_BIT)) {
        if (buflen <= (sizeof(struct ieee80211_frame)
                    + IEEE80211_CCMP_HEADERLEN)) {
            t2lm_err("Incorrect buflen (%d)", buflen);
            return NULL;
        }

        data_ptr += IEEE80211_CCMP_HEADERLEN;
    } else {
        if (buflen <= (sizeof(struct ieee80211_frame)
                    + WLAN_HDR_EXT_IV_LEN)) {
            t2lm_err("Incorrect buflen (%d)", buflen);
            return NULL;
        }

        data_ptr += WLAN_HDR_EXT_IV_LEN;
    }

    return (struct ieee80211_action *)data_ptr;
}

void ieee80211_handle_t2lm_on_tx_frame_completion(struct ieee80211_node *ni,
        wbuf_t wbuf, int ts_flags)
{
    struct ieee80211_frame *wh = (struct ieee80211_frame *)wbuf_header(wbuf);
    struct wlan_mlo_peer_context *ml_peer = NULL;
    enum wlan_t2lm_resp_frm_type t2lm_resp_type = WLAN_T2LM_RESP_TYPE_INVALID;
    uint8_t *frm;
    struct ieee80211_action *ia;
    struct ieee80211com *ic = ni->ni_ic;
    int subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
    struct ieee80211vap *vap = ni->ni_vap;
    struct wlan_objmgr_vdev *vdev;

    if (!vap) {
        t2lm_err("null vap");
        return;
    }

    ml_peer = ni->peer_obj->mlo_peer_ctx;
    if (!ml_peer)
        return;

    if (!ic->ic_send_mlo_peer_tid_to_link_map) {
        t2lm_err("ic_send_mlo_peer_tid_to_link_map is null");
        return;
    }

    /* Send the T2LM frame Tx completion status to FW when,
     * - MLD peer/STA supports the T2LM feature
     * - Assoc response Tx completion received from FW
     * - AP MLD sends T2LM response frame to MLD peer with response status as
     *   success.
     * - AP MLD sends T2LM request frame to MLD peer
     * - AP MLD sends T2LM teardown frame to MLD peer
     */


    if (subtype != IEEE80211_FC0_SUBTYPE_ACTION)
        return;

    if ((wh->i_fc[1] & IEEE80211_FC1_WEP) &&
         !qdf_is_macaddr_group((struct qdf_mac_addr *)wh->i_addr1) &&
         !qdf_is_macaddr_broadcast((struct qdf_mac_addr *)wh->i_addr1)) {
        ia = ieee80211_pmf_get_action_offset(wbuf);
    } else {
        frm = IEEE80211_IS_HTC_PRESENT(wh) ?
            ((u_int8_t *)&wh[1] + IEEE80211_HT_CTRL_LEN) : (u_int8_t *)&wh[1];
        ia = (struct ieee80211_action *) frm;
    }

    if (!ia || (ia->ia_category != IEEE80211_ACTION_CAT_PROTECTED_EHT))
        return;

    t2lm_resp_type = ml_peer->t2lm_policy.ongoing_tid_to_link_mapping.t2lm_resp_type;

    t2lm_debug("ts_flags:%d t2lm_resp_type:%d ia_action:%d",
            ts_flags, t2lm_resp_type, ia->ia_action);

    if ((ia->ia_action == IEEE80211_ACTION_T2LM_TEARDOWN) ||
        ((ia->ia_action == IEEE80211_ACTION_T2LM_RESPONSE) &&
        (t2lm_resp_type == WLAN_T2LM_RESP_TYPE_SUCCESS))) {
        /* To be done, notify the RM APP about Tx completion status */
        wlan_mlme_assoc_t2lm_params(ni->peer_obj, false,
                                    &ml_peer->t2lm_policy.ongoing_tid_to_link_mapping,
                                    QCA_WLAN_VENDOR_T2LM_CATEGORY_NONE);

    }

    if ((!ts_flags) &&
        (((ia->ia_action == IEEE80211_ACTION_T2LM_RESPONSE) &&
          (t2lm_resp_type == WLAN_T2LM_RESP_TYPE_SUCCESS)) ||
         (ia->ia_action == IEEE80211_ACTION_T2LM_TEARDOWN))) {
        /* T2LM negotiation successfull, send the negotiated T2LM
         * info to FW.
         */
        ieee80211_copy_negotiated_t2lm_info_to_peer_ctx(ml_peer,
                &ml_peer->t2lm_policy.ongoing_tid_to_link_mapping, true);

        vdev = vap->vdev_obj;
        if ((ia->ia_action == IEEE80211_ACTION_T2LM_TEARDOWN) &&
             vdev && vdev->mlo_dev_ctx &&
             vdev->mlo_dev_ctx->t2lm_ctx.established_t2lm.t2lm.expected_duration_present) {
            ic->ic_send_mlo_peer_established_mapping(vdev, ml_peer);
        } else {
            ic->ic_send_mlo_peer_tid_to_link_map(ml_peer, true);
        }
    }
}
#else
void ieee80211_handle_t2lm_on_tx_frame_completion(struct ieee80211_node *ni,
        wbuf_t wbuf, int ts_flags)
{
    return;
}
#endif /* WLAN_FEATURE_11BE */

void ieee80211_mlme_null_frame_complete_handler(wlan_if_t vap, wbuf_t wbuf,void *arg,
        u_int8_t *dst_addr, u_int8_t *src_addr, u_int8_t *bssid,
        ieee80211_xmit_status *ts)
{
    bool      tx_status;
    u_int64_t cookie;
    struct ieee80211_node *ni     = (struct ieee80211_node *)arg;
    struct wlan_objmgr_vdev *vdev = vap->vdev_obj;

    if (!ni) {
        qdf_err("Error ni is NULL for mac_addr:%s",ether_sprintf(dst_addr));
        return;
    }
    if (vap->iv_cfg80211_create && vap->iv_opmode == IEEE80211_M_HOSTAP) {
        tx_status = (!ts->ts_flags) ? true : false;
        if (tx_status == true) {
            qdf_debug("Got ACK for STA MAC:%s", ether_sprintf(dst_addr));
            cookie = wbuf_get_txrx_cookie(wbuf);
            cfg80211_probe_status(vdev->vdev_nif.osdev->wdev->netdev, dst_addr, cookie,
                                                        tx_status, 1, true, GFP_ATOMIC);
        } else {
            qdf_err("Tx Error: ACK not received from STA MAC:%s", ether_sprintf(dst_addr));
            cookie = wbuf_get_txrx_cookie(wbuf);
            cfg80211_probe_status(vdev->vdev_nif.osdev->wdev->netdev, dst_addr, cookie,
                                                        tx_status, 0, false, GFP_ATOMIC);
        }
    }
}

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
static void ieee80211_mlo_tx_status_translate_addr(struct ieee80211vap *vap, struct ieee80211_node *ni,
                                      wbuf_t wbuf)
{
    struct ieee80211_frame *wh;

    wh = (struct ieee80211_frame *) wbuf_header(wbuf);

    if (!wlan_peer_is_mlo(ni->peer_obj))
        return;

    if (qdf_is_macaddr_zero(&vap->mld_mac_addr))
        return;

    if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
        qdf_mem_copy(wh->i_addr1, ni->peer_obj->mldaddr, QDF_MAC_ADDR_SIZE);
    } else if (vap->iv_opmode == IEEE80211_M_STA) {
        qdf_mem_copy(wh->i_addr1, ni->peer_obj->mldaddr, QDF_MAC_ADDR_SIZE);
        qdf_mem_copy(wh->i_addr2, &vap->mld_mac_addr, QDF_MAC_ADDR_SIZE);
        qdf_mem_copy(wh->i_addr3, &vap->mld_mac_addr, QDF_MAC_ADDR_SIZE);
    }
}
#endif

void ieee80211_mlme_frame_complete_handler(wlan_if_t vap, wbuf_t wbuf,void *arg,
        u_int8_t *dst_addr, u_int8_t *src_addr, u_int8_t *bssid,
        ieee80211_xmit_status *ts)
{
    struct ieee80211_node *ni = (struct ieee80211_node *)arg;
    struct ieee80211_frame *wh = (struct ieee80211_frame *)wbuf_header(wbuf);
    int subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
    bool delayed_cleanup = false;
    bool leave_ongoing = false;
#if defined(WLAN_SUPPORT_SPLITMAC) || defined(IPA_OFFLOAD)
    struct wlan_objmgr_vdev *vdev;
#endif
    int is_splitmac_enable;
    struct ieee80211com *ic;
#ifndef WLAN_FEATURE_11BE_MLO
    u_int8_t newassoc;
    int retval = 0;
#endif
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    int link_id = -1;
#endif
#ifdef IPA_OFFLOAD
    enum wlan_ipa_wlan_event wlan_event;
    struct wlan_mlo_peer_context *ml_peer_ctx;
    struct wlan_mlo_link_peer_entry *link_peer;
    struct wlan_objmgr_psoc *psoc;
    uint8_t i;
#endif
#if UMAC_SUPPORT_CFG80211
    struct wireless_dev *wdev;
    osif_dev *osifp = (osif_dev *)vap->iv_ifp;

    if (!osifp)
        return;

    wdev = get_cfg80211_notification_wdev(osifp);

    if (!wdev)
        return;
#endif
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    if (ieee80211_mlo_is_mld_ap(vap->vdev_obj))
        link_id = vap->link_id;
#endif

    if (!ni) {
        qdf_print("%s Error ni is NULL for subtype:%d mac_addr:%s",__func__,subtype, ether_sprintf(dst_addr));
        return;
    }

    delayed_cleanup = ieee80211node_has_flag(ni, IEEE80211_NODE_DELAYED_CLEANUP);
    leave_ongoing = ieee80211node_has_flag(ni, IEEE80211_NODE_LEAVE_ONGOING);
    IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_ASSOC, dst_addr,
            "%s: frame %d send to %s, delayed_clean: %d, leve_ongoing: %d ts flags %08x",
            __func__, subtype, ether_sprintf(dst_addr), delayed_cleanup, leave_ongoing, ts->ts_flags);

    switch (subtype) {
        case IEEE80211_FC0_SUBTYPE_DISASSOC:
        case IEEE80211_FC0_SUBTYPE_DEAUTH:
            if(!(IEEE80211_ADDR_EQ(dst_addr, ieee80211broadcastaddr)) && delayed_cleanup) {
                ieee80211node_clear_flag(ni, IEEE80211_NODE_DELAYED_CLEANUP);
                /*
                 * Prevent _ieee80211_node_leave() from reentry which would mess
                 * up the value of iv_sta_assoc. Before AP received the tx ack
                 * for "disassoc request", it may have received the "auth
                 * (not SUCCESS status)" to do node leave. With the flag,
                 * follow-up cleanup wouldn't call _ieee80211_node_leave() again
                 * when execuating the tx_complete handler.
                 */
                if (!leave_ongoing && (ni != ni->ni_bss_node)) {
#if ATH_SUPPORT_MGMT_TX_STATUS
                    /*
                     * Providing only ack status as success,
                     * as we are providing DISASSOC_COMPLETE EVENT
                     * with sucess always
                     */
                    IEEE80211_DELIVER_EVENT_MGMT_TX_STATUS(vap,1,wbuf);
#endif
                    if (subtype == IEEE80211_FC0_SUBTYPE_DISASSOC) {
                        if (!vap->iv_roam.iv_roam_disassoc)
#ifdef WLAN_FEATURE_11BE_MLO
                            IEEE80211_DELIVER_EVENT_MLME_DISASSOC_COMPLETE(vap,
                                    dst_addr, ni->ni_reason_code,
                                    IEEE80211_STATUS_SUCCESS,
                                    &ni->peer_obj->mldaddr[0]);
#else
                            IEEE80211_DELIVER_EVENT_MLME_DISASSOC_COMPLETE(vap,
                                    dst_addr, ni->ni_reason_code,
                                    IEEE80211_STATUS_SUCCESS,
                                    NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
                        else
                            vap->iv_roam.iv_roam_disassoc = 0;
                    } else if (subtype == IEEE80211_FC0_SUBTYPE_DEAUTH)
#ifdef WLAN_FEATURE_11BE_MLO
                         IEEE80211_DELIVER_EVENT_MLME_DEAUTH_COMPLETE(vap,
                                dst_addr, ts->ts_flags , ni->ni_associd,
                                ni->ni_reason_code, &ni->peer_obj->mldaddr[0]);
                    if (wlan_peer_is_mlo(ni->peer_obj) &&
                        (wlan_mlo_partner_peer_delete_is_allowed(ni->peer_obj))) {
                        wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
                    }
#else
                       IEEE80211_DELIVER_EVENT_MLME_DEAUTH_COMPLETE(vap,
                                dst_addr, ts->ts_flags , ni->ni_associd,
                                ni->ni_reason_code, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
                    IEEE80211_NODE_LEAVE(ni);
                }
            }
            else if (IEEE80211_ADDR_EQ(dst_addr, ieee80211broadcastaddr))
            {
#ifdef WLAN_FEATURE_11BE_MLO
                wlan_iterate_station_list(vap, cleanup_sta_partner_bridge_peer, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
                wlan_iterate_station_list(vap, cleanup_sta_peer, NULL);
                wlan_iterate_unassoc_sta_list(vap, cleanup_sta_peer, NULL);
            }
            break;

        case IEEE80211_FC0_SUBTYPE_ASSOC_RESP:
        case IEEE80211_FC0_SUBTYPE_REASSOC_RESP:
#ifdef WLAN_FEATURE_11BE_MLO
           if (wlan_peer_is_mlo(ni->peer_obj)) {
                ni->ni_ml_link_peer_created = 0;
                ni->ni_hostapd_asresp_trig = 0;
                wlan_mlo_peer_free_all_link_assoc_resp_buf(ni->peer_obj);
           }
#endif

            if( delayed_cleanup || leave_ongoing ) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,
                        "%s: Error Assoc resp when peer delete in progress ni: 0x%pK, mac: %s,vapid: %d delayed_cleanup:%d leave_ongoing:%d\n",
                        __func__, ni, ether_sprintf(ni->ni_macaddr),ni->ni_vap->iv_unit,delayed_cleanup,leave_ongoing);
                if (ni->ni_assocstatus != IEEE80211_STATUS_SUCCESS) {
                    ieee80211node_clear_flag(ni, IEEE80211_NODE_DELAYED_CLEANUP);

#ifdef WLAN_FEATURE_11BE_MLO
                    if (wlan_peer_is_mlo(ni->peer_obj)) {
                        wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
                    }
#endif
                    IEEE80211_NODE_LEAVE(ni);
                }
#if UMAC_SUPPORT_CFG80211
                if(vap->iv_cfg80211_create) {
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
                    ieee80211_mlo_tx_status_translate_addr(vap, ni, wbuf);
                    cfg80211_mgmt_tx_status(wdev, 0, qdf_nbuf_data(wbuf), qdf_nbuf_len(wbuf), false, link_id, GFP_ATOMIC);
#else
                    cfg80211_mgmt_tx_status(wdev, 0, qdf_nbuf_data(wbuf), qdf_nbuf_len(wbuf), false, GFP_ATOMIC);
#endif
                }
                return;
#endif
            } else if (ni->ni_assocstatus == IEEE80211_STATUS_SUCCESS && ni != vap->iv_bss) {
                /* Check if ni is not bss peer because for bss peer ni->ni_assocstatus will not
                 * be set but for duplicate auth/assoc, resp is sent using bss peer. Hence, completion
                 * will be recived on bss peer but peer assoc is not needed.
                 */
                ic = vap->iv_ic;
#if WLAN_SUPPORT_SPLITMAC
                vdev = vap->vdev_obj;
                is_splitmac_enable = splitmac_is_enabled(vdev);
#else
                is_splitmac_enable = 0;
#endif
                ni->ni_last_authorize_rst_time = OS_GET_TIMESTAMP();
#ifndef WLAN_FEATURE_11BE_MLO
                if(!(ieee80211_is_pmf_enabled(vap, ni)) ||  !(ieee80211node_has_flag(ni,IEEE80211_NODE_AUTH))){
                        newassoc = !ieee80211node_has_extflag(ni, IEEE80211_NODE_ASSOC_RESP);

                        if (((ic->ic_newassoc != NULL) && !(ni->is_ft_reassoc)) && !is_splitmac_enable) {
                            retval = ic->ic_newassoc(ni, newassoc);
                            if (retval) {
                                qdf_err("node %s, failed to send peer assoc command, disconnect the STA", ether_sprintf(ni->ni_macaddr));
                                ieee80211_send_assocresp(ni, newassoc, IEEE80211_STATUS_INVALID_PARAM, NULL);

#ifdef WLAN_FEATURE_11BE_MLO
                                if (wlan_peer_is_mlo(ni->peer_obj)) {
                                    wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
                                }
#endif
                                if (IEEE80211_NODE_LEAVE(ni)) {
                                    IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                               ni->ni_macaddr, ni->ni_associd, IEEE80211_STATUS_UNSPECIFIED, NULL);
                                }
                                return;
                            }

                                /* In case of OMN in Asoc req, phymode is not updated
                                 * and therefore we explicitly send chwidth in OMN
                                 * to target. This is confirmed by checking if chan width
                                 * obtained from phymode and peer ch width differ.
                                */
                                if (ni->ni_chwidth != get_chwidth_phymode(ni->ni_phymode))
                                        ic->ic_chwidth_change(ni);
                                ieee80211node_set_extflag(ni, IEEE80211_NODE_ASSOC_RESP);
                                ieee80211_update_noderates(ni);
                        }
                }
#endif  /* WLAN_FEATURE_11BE_MLO */

#ifdef IPA_OFFLOAD
                if (ucfg_ipa_is_enabled()) {
                    ic = vap->iv_ic;

                    if (wlan_peer_is_mlo(ni->peer_obj)) {
                        /* peer is mlo capable peer, send client_connect only for mld peer with primary netdev */
                        ml_peer_ctx = ni->peer_obj->mlo_peer_ctx;

                        if (!ml_peer_ctx) {
                            qdf_err("ML Peer is NULL !!!");
                            return;
                        }

                        mlo_peer_lock_acquire(ml_peer_ctx);

                        for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
                            link_peer = &ml_peer_ctx->peer_list[i];
                            if (!link_peer->link_peer)
                                continue;

                            if (!link_peer->is_primary) {
                                 /* Send client connect for Secondary link to enable WDI pipes */
                                wlan_event = WLAN_IPA_MLO_CLIENT_CONNECT_EX;
                                vdev = link_peer->link_peer->peer_objmgr.vdev;
                                psoc = wlan_vdev_get_psoc(vdev);
                                if (!psoc) {
                                        mlo_peer_lock_release(ml_peer_ctx);
                                        qdf_err("Psoc is NULL !!!");
                                        return;
                                }
                                ucfg_ipa_event_wq(psoc, &link_peer->link_addr, vdev,
                                                  wlan_event);
                            } else {
                                /* Send client connect for primary link with mld peer mac address */
                                vdev = link_peer->link_peer->peer_objmgr.vdev;
                                wlan_event = WLAN_IPA_CLIENT_CONNECT_EX;
                                psoc = wlan_vdev_get_psoc(vdev);
                                if (!psoc) {
                                        mlo_peer_lock_release(ml_peer_ctx);
                                        qdf_err("Psoc is NULL !!!");
                                        return;
                                }
                                ucfg_ipa_event_wq(psoc, &ml_peer_ctx->peer_mld_addr,
                                                  vdev, wlan_event);
                            }
                        }

                        mlo_peer_lock_release(ml_peer_ctx);
                    } else {
                        /* Send cleint connect for non-mlo client */
                        wlan_event = WLAN_IPA_CLIENT_CONNECT_EX;
                        vdev = vap->vdev_obj;
                        psoc = wlan_vdev_get_psoc(vdev);
                        if (!psoc) {
                                qdf_err("Psoc is NULL !!!");
                                return;
                        }
                        ucfg_ipa_event_wq(psoc, ni->ni_macaddr, vdev, wlan_event);
                    }
                }
#endif

#if WLAN_SUPPORT_GREEN_AP
                if ((vap->iv_opmode == IEEE80211_M_HOSTAP) && (!ni->is_assoc_resp_completed) && (!ts->ts_flags) && (ni->ni_associd > 0)) {
                    wlan_green_ap_add_sta(ic->ic_pdev_obj);

                    if(wlan_node_get_max_nss(ni) > 1)
                        wlan_green_ap_add_multistream_sta(ic->ic_pdev_obj);
#ifdef WLAN_FEATURE_11BE_MLO
                    if (ieee80211_mlo_is_mld_ap(vap->vdev_obj) && wlan_peer_is_mlo(ni->peer_obj))
                        ieee80211_mlo_green_ap_add_sta(vap->vdev_obj, ni->ni_associd);
#endif
                    ni->is_assoc_resp_completed = 1;
                }
#endif
            }
            break;

        default:
            break;
    }

    ieee80211_handle_t2lm_on_tx_frame_completion(ni, wbuf, ts->ts_flags);

#if UMAC_SUPPORT_CFG80211
    if((subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
       (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP) ||
       (subtype == IEEE80211_FC0_SUBTYPE_DISASSOC) ||
       (subtype == IEEE80211_FC0_SUBTYPE_DEAUTH)||
       (subtype == IEEE80211_FC0_SUBTYPE_ACTION)) {
        if(vap->iv_cfg80211_create) {
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
            ieee80211_mlo_tx_status_translate_addr(vap, ni, wbuf);
            cfg80211_mgmt_tx_status(wdev, 0, qdf_nbuf_data(wbuf), qdf_nbuf_len(wbuf), (!ts->ts_flags) ? true : false, link_id, GFP_ATOMIC);
#else
            cfg80211_mgmt_tx_status(wdev, 0, qdf_nbuf_data(wbuf), qdf_nbuf_len(wbuf), (!ts->ts_flags) ? true : false, GFP_ATOMIC);
#endif
        }
    }
#endif
    return;
}
/*
 * Routine to transmit a Disassoc request frame.
 */
int wlan_mlme_disassoc_request(wlan_if_t vaphandle, u_int8_t *macaddr, IEEE80211_REASON_CODE reason)
{
    int retval;
    retval = wlan_mlme_disassoc_request_with_callback(vaphandle, macaddr, reason, NULL, NULL);
    return retval;
}

int wlan_mlme_mark_delayed_node_cleanup(wlan_if_t vaphandle, u_int8_t *macaddr)
{
    struct ieee80211vap      *vap = vaphandle;
    struct ieee80211_node    *ni;

    if (IEEE80211_ADDR_EQ(macaddr, IEEE80211_GET_BCAST_ADDR(vap->iv_ic))) {
        if (vap->iv_opmode == IEEE80211_M_STA) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,
                              "%s: unexpected station vap with all 0xff mac address", __func__);
            return -EINVAL;
        }
    }
    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_HANDLER_ID);
    if (ni != NULL) {
        ieee80211_try_mark_node_for_delayed_cleanup(ni);
        ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);
    }

    return 0;
}

/*
 * Routine to transmit a Disassoc request frame with a completion callback when done.
 */
int wlan_mlme_disassoc_request_with_callback(
    wlan_if_t vaphandle,
    u_int8_t *macaddr,
    IEEE80211_REASON_CODE reason,
    wlan_vap_complete_buf_handler handler,
    void *arg
    )
{
    struct ieee80211vap      *vap = vaphandle;
    struct ieee80211_node    *ni;
    int                      error = 0;
    bool delayed_cleanup = false;
#if QCA_SUPPORT_SON
    struct wlan_assoc_disassoc_event_info info;
#endif
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_rx_ops *rx_ops = NULL;
#ifdef WLAN_FEATURE_11BE_MLO
    u_int8_t mldaddr[QDF_MAC_ADDR_SIZE] = {0};
#endif

    psoc = wlan_vdev_get_psoc(vap->vdev_obj);
    if (psoc) {
        rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

#if QCA_SUPPORT_SON
    qdf_mem_zero(&info, sizeof(info));
    qdf_mem_copy(info.macaddr, macaddr, QDF_MAC_ADDR_SIZE);
    info.flag = ACTION_DISASSOC;
    info.reason = reason;
    if (rx_ops && rx_ops->son_rx_ops.deliver_event)
        rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, NULL, MLME_EVENT_ASSOC_DISASSOC, &info);
#endif

    /* Broadcast Addr - disassociate all stations */
    if (IEEE80211_ADDR_EQ(macaddr, IEEE80211_GET_BCAST_ADDR(vap->iv_ic))) {
        if (vap->iv_opmode == IEEE80211_M_STA) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,
                              "%s: unexpected station vap with all 0xff mac address", __func__);
            ASSERT(0);
            goto exit;
		} else {
            /* Iterate station list only when PMF is not enabled */
            if (!wlan_vap_is_pmf_enabled(vap)) {
                wlan_iterate_station_list(vap, sta_disassoc, NULL);
                goto exit;
            }
        }
    }

    /*
     * if a node exist with the given address already , use it.
     * if not use bss node.
     */
    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_HANDLER_ID);
    if (ni == NULL) {
        if(vap->iv_opmode == IEEE80211_M_STA){
            if(!wlan_vap_is_pmf_enabled(vap)){
                error = -ENOMEM;
                goto exit;
            }
        } else {
            if (!IEEE80211_ADDR_EQ(macaddr, IEEE80211_GET_BCAST_ADDR(vap->iv_ic)))
            {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,
                        "%s: client %s does not exist, skip sending disassoc\n",
                        __func__, ether_sprintf(macaddr));
                error = -EIO;
                goto exit;
            }
        }
        ni = ieee80211_try_ref_node(vap->iv_bss, WLAN_MLME_HANDLER_ID);
        if (!ni) {
            error = -EINVAL;
            goto exit;
        }
    }
    if ((ieee80211node_has_flag(ni, IEEE80211_NODE_DELAYED_CLEANUP) && (handler == NULL)) ||
       IEEE80211_ADDR_EQ(macaddr, IEEE80211_GET_BCAST_ADDR(vap->iv_ic))) {
        handler = ieee80211_mlme_frame_complete_handler;
        ni->ni_reason_code = reason;
        arg = (void *)ni;
    }
    wlan_node_set_peer_state(ni, WLAN_DISCONNECT_STATE);
    /* Send disassoc frame */
    delayed_cleanup = ieee80211node_has_flag(ni, IEEE80211_NODE_DELAYED_CLEANUP);
    error = ieee80211_send_disassoc_with_callback(ni, reason, handler, arg);

#ifdef WLAN_FEATURE_11BE_MLO
    if ((ni->peer_obj->mldaddr) &&
        (IEEE80211_ADDR_IS_VALID(ni->peer_obj->mldaddr)))
        IEEE80211_ADDR_COPY(mldaddr, &ni->peer_obj->mldaddr[0]);
#endif
    /* Node needs to be removed from table as well, do it only for AP now */
    if (vap->iv_opmode == IEEE80211_M_HOSTAP  && ni != vap->iv_bss) {
        if (!delayed_cleanup ||
            ieee80211node_has_extflag(ni, IEEE80211_NODE_DELAYED_CLEANUP_FAIL))
        {
#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_peer_is_mlo(ni->peer_obj)) {
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
            }
#endif
            IEEE80211_NODE_LEAVE(ni);
        }
    }
    /* claim node immediately */
    ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);

    /*
     * Call MLME confirmation handler => mlme_disassoc_complete
     * This should reflect the tx completion status of the disassoc frame,
     * but since we don't have per frame completion, we'll always indicate success here.
     */
    if (!delayed_cleanup ||
        ieee80211node_has_extflag(ni, IEEE80211_NODE_DELAYED_CLEANUP_FAIL)) {
        if (!vap->iv_roam.iv_roam_disassoc)
#ifdef WLAN_FEATURE_11BE_MLO
            IEEE80211_DELIVER_EVENT_MLME_DISASSOC_COMPLETE(vap, macaddr, reason, IEEE80211_STATUS_SUCCESS, mldaddr);
#else
            IEEE80211_DELIVER_EVENT_MLME_DISASSOC_COMPLETE(vap, macaddr, reason, IEEE80211_STATUS_SUCCESS, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
        else
            vap->iv_roam.iv_roam_disassoc = 0;
    }

exit:
    return error;
}

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
bool wlan_mlme_is_stacac_running(wlan_if_t vaphandle)
{
    struct ieee80211vap           *vap = vaphandle;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    return (mlme_priv->im_is_stacac_running);
}

#ifdef WLAN_FEATURE_11BE_MLO
QDF_STATUS wlan_mlme_mlo_is_stacac_running(struct wlan_objmgr_vdev *vdev)
{
   uint16_t vdev_count = 0, i = 0;
   struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
   wlan_if_t vap = NULL;
   QDF_STATUS status = QDF_STATUS_E_FAILURE;

   mlo_get_ml_vdev_list(vdev, &vdev_count, vdev_list);
   for (i = 0; i< vdev_count; i++) {
        if (status == QDF_STATUS_SUCCESS) {
            mlo_release_vdev_ref(vdev_list[i]);
            continue;
        }
        vap = wlan_vdev_mlme_get_ext_hdl(vdev_list[i]);
        if (!vap) {
            mlo_release_vdev_ref(vdev_list[i]);
            continue;
        }
        if (wlan_mlme_is_stacac_running(vap))
            status = QDF_STATUS_SUCCESS;
        mlo_release_vdev_ref(vdev_list[i]);
   }

   return status;
}
#endif
#endif

int wlan_mlme_start_bss(wlan_if_t vaphandle, u_int8_t restart)
{
    struct ieee80211vap           *vap = vaphandle;
    int                           error = 0;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    if (wlan_vdev_chan_config_valid(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
        /* bss is already started  */
        error = EBUSY;
        return error;
    }

    switch(vap->iv_opmode) {
    case IEEE80211_M_MONITOR:
    case IEEE80211_M_HOSTAP:
    case IEEE80211_M_BTAMP:
        /*
         * start the AP . the channel/ssid should have been setup already.
         */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Create infrastructure(AP) bss\n", __func__);
        IEEE80211_RADAR_FOUND_LOCK(vap->iv_ic);
        error = mlme_create_infra_bss(vap, restart);
        IEEE80211_RADAR_FOUND_UNLOCK(vap->iv_ic);
        break;
    default:
        ASSERT(0);
    }
    /* In DA, create_infra will return 0 on success.
     * In Offload mode, create_infra will return EBUSY
     * as it needs to wait for vdev start response from FW
     */
    if(!error  || error == EBUSY) {
        /*
         * Save the time when the vap interface is brought up
         */
        vap->vap_uptime = OS_GET_TICKS();
    }

    return error;
}

QDF_STATUS wlan_mlme_restart_bss(struct wlan_objmgr_vdev *vdev)
{
    wlan_mlo_ap_vdev_quiet_set(vdev);

    if (!wlan_vdev_mlme_is_special_vdev(vdev)) {
        return wlan_vdev_mlme_sm_deliver_evt(vdev,
                                             WLAN_VDEV_SM_EV_SUSPEND_RESTART,
                                             0, NULL);
    } else {
        return wlan_vdev_mlme_sm_deliver_evt(vdev,
                                             WLAN_VDEV_SM_EV_FW_VDEV_RESTART,
                                             0, NULL);
    }
}

bool wlan_coext_enabled(wlan_if_t vaphandle)
{
    struct ieee80211com    *ic = NULL;

    if(vaphandle->iv_ic != NULL)
        ic = vaphandle->iv_ic;
    else
        return FALSE;

    return (ic->ic_flags & IEEE80211_F_COEXT_DISABLE) ? FALSE : TRUE;
}

void wlan_determine_cw(wlan_if_t vaphandle, wlan_chan_t channel)
{
    struct ieee80211com    *ic = vaphandle->iv_ic;
    int is_chan_40;


    if(!IEEE80211_IS_CHAN_VALID(channel))
        return;

    is_chan_40 = IEEE80211_IS_CHAN_BW_40MHZ(channel);

    if (!is_chan_40 || !IEEE80211_IS_CHAN_2GHZ(channel)) {
        return;
    }

    if (ic->ic_flags & IEEE80211_F_COEXT_DISABLE)
        return;

    if (IEEE80211_IS_CHAN_40INTOL(ic, channel)) {
        ic->ic_bss_to20(ic);
    } else {
        ic->ic_bss_to40(ic);
    }
}

void peer_delete_req_status_set(struct ieee80211_node *ni, void *arg)
{
    u_int8_t *peer_del_req = (u_int8_t *)arg;
    if (peer_del_req) {
        if ((*peer_del_req) == MLME_INDIVIDUAL_PEER_CLEANUP)
            qdf_atomic_set(&(ni->ni_peer_del_req_enable), 1);
        else if ((*peer_del_req) == MLME_MULTIPLE_PEER_CLEANUP)
            qdf_atomic_set(&(ni->ni_peer_del_req_enable), 0);
    }
}

void
update_peer_nss_based_on_cap(void *arg, struct ieee80211_node *ni)
{
#define MAX_PEER_EXT_NSS_UPDATE_COUNT 128
    struct ieee80211vap    *vap = ni->ni_vap;
    struct ieee80211com    *ic = vap->iv_ic;
    struct ieee80211_ath_channel ni_chan;
    struct chan_params chan_p = {0};
    uint8_t *peer_update_count = (uint8_t *)arg;

    chan_p.freq = ni->ni_ch_param.ni_freq;
    chan_p.cfreq2 = ni->ni_ch_param.ni_cfreq2;
    ni_chan = ieee80211_find_dot11_channel(ic, &chan_p,
                                               ni->ni_ch_param.ni_bss_phymode);

    /*
     * VHT rate should be set only if VHT capabilities are sent by the client.
     * A 6G DUT does not support VHT rates. VHT rates are currently
     * supported by 5G 11ac and 11ax APs only.
     * Process the VHT caps, if the node channel is 11ax supported and not a 6G
     * channel or if it is a vht supported channel.
     */
    if ((IEEE80211_IS_CHAN_11AX_SUPPORTED(&ni_chan) &&
         !IEEE80211_IS_CHAN_6GHZ(&ni_chan)) ||
        IEEE80211_IS_CHAN_VHT(&ni_chan)) {
        ieee80211_setup_vht_rates(ni, NULL, IEEE80211_F_DOFRATE
                                  | IEEE80211_F_DOXSECT | IEEE80211_F_DOBRS);
    }

    /* Tx/Rx MCS NSS is part of 11AX HE Cap and need not be processed
     * for non 11AX clients. This has to be updated in the future to
     * include 11BE.
     */
    if (ieee80211_is_phymode_11ax(ni->ni_phymode)
#ifdef WLAN_FEATURE_11BE
        || ieee80211_is_phymode_11be(ni->ni_phymode)
#endif /* WLAN_FEATURE_11BE */
       )
        ieee80211_calc_nss_based_on_txrx_mcsnss_requirement(ic, ni);

    if (*peer_update_count < MAX_PEER_EXT_NSS_UPDATE_COUNT)
        ic->ic_ext_nss_change(ni, peer_update_count);
}

void
cleanup_sta_peer(void *arg, struct ieee80211_node *ni)
{
    struct ieee80211vap    *vap = ni->ni_vap;
    u_int8_t macaddr[6];
    u_int16_t associd = 0;
    bool notify_deauth = true;
    bool mlo_reconfig = false;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct ieee80211vap *vap1 = NULL;
    struct wlan_mlo_peer_context *ml_peer = NULL;
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct wlan_objmgr_peer *link_peer = NULL;
    bool bridge_peer_present = false;
    int i;
#endif

#ifdef WLAN_FEATURE_11BE_MLO
    /*
     *  Skip sta peer cleanup for MLO peers if the request is coming from
     *  DFS_CAC_WAIT state.
     */
    if ((wlan_mlo_peer_delete_is_not_allowed(vap->vdev_obj)) &&
        (wlan_peer_is_mlo(ni->peer_obj) && (ni->peer_obj->mlo_peer_ctx))) {
        return;
    }
#endif

    IEEE80211_NOTE(vap, IEEE80211_MSG_MLME, ni, "%s: cleanup station",
                      __func__);
    IEEE80211_ADDR_COPY(macaddr, ni->ni_macaddr);
    peer_delete_req_status_set(ni, arg);

    associd = ni->ni_associd;

    mlo_reconfig = wlan_vdev_mlme_op_flags_get(vap->vdev_obj, WLAN_VDEV_OP_MLO_STOP_LINK_DEL) ||
                   (wlan_vdev_mlme_op_flags_get(vap->vdev_obj, WLAN_VDEV_OP_MLO_LINK_TBTT_COMPLETE));
#ifdef WLAN_FEATURE_11BE_MLO
    ml_peer = ni->peer_obj->mlo_peer_ctx;
    if (!wlan_peer_is_mlo(ni->peer_obj) || !ml_peer)
         goto skip_hw_war;

    /* Search if bridge peer exists */
    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        if (!peer_entry)
            continue;

        link_peer = peer_entry->link_peer;
        if (!link_peer)
            continue;

        if (wlan_peer_get_peer_type(link_peer) == WLAN_PEER_MLO_BRIDGE) {
            bridge_peer_present = true;
            break;
        }
    }

    /*
     * Handling MLO address translation issues in Be
     * Indicate controller through NL event to enable AMSDUs for 3rd party root-AP
     * if only single link peer if left after sta peer cleanup
     * Check if mlo_reconfig is enabled and check one of two possible conditions
     * First where 2 links and no bridge peer is present and one link is going down
     * second when 3 links including a bridge peer is present and since one link
     * is going down, so we will be left with single link MLO
     */
    if (mlo_reconfig && (((ml_peer->link_peer_cnt == 2) && !bridge_peer_present) ||
        ((ml_peer->link_peer_cnt == 3) && bridge_peer_present))) {
        for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
            peer_entry = &ml_peer->peer_list[i];
            if (!peer_entry)
                continue;

            link_peer = peer_entry->link_peer;
            if (!link_peer)
                continue;

            if ((link_peer != ni->peer_obj) &&
                (wlan_peer_get_peer_type(link_peer) != WLAN_PEER_MLO_BRIDGE)) {
                    vdev = wlan_peer_get_vdev(link_peer);
                    vap1 = wlan_vdev_get_mlme_ext_obj(vdev);
                    psoc = wlan_pdev_get_psoc(wlan_vdev_get_pdev(vdev));
                    if ((!cfg_get(psoc, CFG_OL_ADV_RAW_MODE_WAR_ENABLE) ||
                        (qdf_mem_cmp(vdev->vdev_mlme.macaddr, vdev->vdev_mlme.mldaddr,
                        QDF_MAC_ADDR_SIZE) == 0)) && (vap1 &&
                        vap1->iv_tx_encap_type == htt_cmn_pkt_type_raw))
                        /*
                         * For cases when ini enable_advance_rawmode_war is true, then send enable
                         * event only when peer mld mac is same as peer link mac
                         */
                        wlan_cfg80211_hw_status_notification(vap1, link_peer, true);
	    }
        }
    }

skip_hw_war:
    if (wlan_peer_is_mlo(ni->peer_obj)) {
        if (wlan_mlo_partner_peer_delete_is_allowed(ni->peer_obj)) {
           if (wlan_peer_get_peer_type(ni->peer_obj) == WLAN_PEER_MLO_BRIDGE) {
               wlan_mlo_peer_deauth_init(ni->peer_obj->mlo_peer_ctx, ni->peer_obj, 1);
               return;
           } else {
               wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
           }
        }
        else
           notify_deauth = false;
    }
#endif
    IEEE80211_NODE_LEAVE(ni);
    if (notify_deauth)
#ifdef WLAN_FEATURE_11BE_MLO 
        IEEE80211_DELIVER_EVENT_MLME_DEAUTH_INDICATION(vap, macaddr, associd,
                                                       IEEE80211_REASON_AUTH_LEAVE,
                                                       &ni->peer_obj->mldaddr[0]);
#else
        IEEE80211_DELIVER_EVENT_MLME_DEAUTH_INDICATION(vap, macaddr, associd,
                                                       IEEE80211_REASON_AUTH_LEAVE,
                                                       NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
}

bool
ieee80211_try_mark_node_for_delayed_cleanup(struct ieee80211_node *ni)
{
    struct ieee80211vap *vap;
    bool marked = false;

    if (ni) {
        vap = ni->ni_vap;
        if ((ni != ni->ni_bss_node) || (vap->iv_opmode == IEEE80211_M_STA)) {
            ieee80211node_set_flag(ni, IEEE80211_NODE_DELAYED_CLEANUP);
            marked = true;
        }
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_AUTH | IEEE80211_MSG_ASSOC,
                "%s: ni: 0x%pK, mac: %s, marked:%d, vapid: %d\n",
                __func__, ni, ether_sprintf(ni->ni_macaddr), marked,
                ni->ni_vap->iv_unit);
    } else
        qdf_print("%s: null ni", __func__);

    return marked;
}

#ifdef WLAN_FEATURE_11BE_MLO
void cleanup_sta_partner_bridge_peer(void *arg, struct ieee80211_node *ni)
{
    struct ieee80211_node *bridge_ni;
    struct wlan_mlo_peer_context *ml_peer = ni->peer_obj->mlo_peer_ctx;
    struct wlan_objmgr_peer *bridge_peer = NULL;

    if (!wlan_peer_is_mlo(ni->peer_obj) || !ml_peer)
        return;

    if (wlan_peer_get_peer_type(ni->peer_obj) == WLAN_PEER_MLO_BRIDGE)
        return;

    bridge_peer = wlan_mlo_peer_get_bridge_peer(ml_peer);
    if (bridge_peer && ml_peer->link_peer_cnt <= 2) {
        bridge_ni = wlan_peer_get_mlme_ext_obj(bridge_peer);
        IEEE80211_NODE_LEAVE(bridge_ni);
    }
}
#endif

void
sta_deauth(void *arg, struct ieee80211_node *ni)
{
    struct ieee80211vap *vap = ni->ni_vap;
    u_int8_t macaddr[6];
    u_int16_t associd = 0;
    bool delayed_cleanup = false;
    bool mlo_reconfig = false;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct ieee80211vap *vap1 = NULL;
    struct wlan_mlo_peer_context *ml_peer = NULL;
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct wlan_objmgr_peer *link_peer = NULL;
    bool bridge_peer_present = false;
    int i;
#endif

#ifdef WLAN_FEATURE_11BE_MLO
    /*
     *  Skip sending deauth for MLO peers if the request is coming from
     *  DFS_CAC_WAIT state.
     */
    if ((wlan_mlo_peer_delete_is_not_allowed(vap->vdev_obj)) &&
        (wlan_peer_is_mlo(ni->peer_obj) && (ni->peer_obj->mlo_peer_ctx))) {
        return;
    }
#endif

    if (ieee80211node_has_flag(ni, IEEE80211_NODE_DELAYED_CLEANUP)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: station %s "
                "already marked for cleanup. aid: %d\n", __func__,
                ether_sprintf(ni->ni_macaddr), ni->ni_associd);
        return;
    }
    mlo_reconfig = wlan_vdev_mlme_op_flags_get(vap->vdev_obj, WLAN_VDEV_OP_MLO_STOP_LINK_DEL) ||
                   (wlan_vdev_mlme_op_flags_get(vap->vdev_obj, WLAN_VDEV_OP_MLO_LINK_TBTT_COMPLETE));
#ifdef WLAN_FEATURE_11BE_MLO
    if ((mlo_reconfig || (wlan_peer_get_peer_type(ni->peer_obj) == WLAN_PEER_MLO_BRIDGE))
        && wlan_mlo_partner_peer_delete_is_allowed(ni->peer_obj)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME | IEEE80211_MSG_AUTH,
                          "%s: sending DEAUTH %s on partner link, reason %d\n",
                           __func__, ether_sprintf(ni->ni_macaddr), IEEE80211_REASON_AUTH_LEAVE);
        wlan_mlo_peer_deauth_init(ni->peer_obj->mlo_peer_ctx, ni->peer_obj, 0);
        return;
    }

    ml_peer = ni->peer_obj->mlo_peer_ctx;
    if (!wlan_peer_is_mlo(ni->peer_obj) || !ml_peer)
         goto skip_hw_war;

    /* Search if bridge peer exists */
    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        if (!peer_entry)
            continue;

        link_peer = peer_entry->link_peer;
        if (!link_peer)
            continue;

        if (wlan_peer_get_peer_type(link_peer) == WLAN_PEER_MLO_BRIDGE) {
            bridge_peer_present = true;
            break;
        }
    }

    /*
     * Handling MLO address translation issues in Be
     * Indicate controller through NL event to enable AMSDUs for 3rd party root-AP
     * if only single link peer if left after deauth
     * Check if mlo_reconfig is enabled and check one of two possible conditions
     * First where 2 links and no bridge peer is present and one link is going down
     * second when 3 links including a bridge peer is present and since one link
     * is going down, so we will be left with single link MLO
     */
    if (mlo_reconfig && (((ml_peer->link_peer_cnt == 2) && !bridge_peer_present) ||
        ((ml_peer->link_peer_cnt == 3) && bridge_peer_present))) {
        for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
            peer_entry = &ml_peer->peer_list[i];
            if (!peer_entry)
                continue;

            link_peer = peer_entry->link_peer;
            if (!link_peer)
                continue;

            if ((link_peer != ni->peer_obj) &&
                (wlan_peer_get_peer_type(link_peer) != WLAN_PEER_MLO_BRIDGE)) {
                    vdev = wlan_peer_get_vdev(link_peer);
                    vap1 = wlan_vdev_get_mlme_ext_obj(vdev);
                    psoc = wlan_pdev_get_psoc(wlan_vdev_get_pdev(vdev));
                    if ((!cfg_get(psoc, CFG_OL_ADV_RAW_MODE_WAR_ENABLE) ||
                        (qdf_mem_cmp(vdev->vdev_mlme.macaddr, vdev->vdev_mlme.mldaddr,
                        QDF_MAC_ADDR_SIZE) == 0)) && (vap1 &&
                        vap1->iv_tx_encap_type == htt_cmn_pkt_type_raw))
                        /*
                         * For cases when ini enable_advance_rawmode_war is true, then send enable
                         * event only when peer mld mac is same as peer link mac
                         */
                        wlan_cfg80211_hw_status_notification(vap1, link_peer, true);
	    }
        }
    }
skip_hw_war:
#endif

    IEEE80211_ADDR_COPY(macaddr, ni->ni_macaddr);
    associd = ni->ni_associd;
    peer_delete_req_status_set(ni, arg);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME | IEEE80211_MSG_AUTH,
            "%s: sending DEAUTH to %s, reason %d\n",
            __func__, ether_sprintf(ni->ni_macaddr), IEEE80211_REASON_AUTH_LEAVE);

    /* For link removal case, do not send deauth */
    if (!mlo_reconfig) {
        delayed_cleanup = ieee80211_try_mark_node_for_delayed_cleanup(ni);
        ieee80211_send_deauth(ni, IEEE80211_REASON_AUTH_LEAVE);
    }
    if (!delayed_cleanup ||
        ieee80211node_has_extflag(ni, IEEE80211_NODE_DELAYED_CLEANUP_FAIL)) {

#ifdef WLAN_FEATURE_11BE_MLO
        if (wlan_peer_is_mlo(ni->peer_obj) && !mlo_reconfig) {
            wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
        }
#endif
        IEEE80211_NODE_LEAVE(ni);
        if (!mlo_reconfig) {
#ifdef WLAN_FEATURE_11BE_MLO
             IEEE80211_DELIVER_EVENT_MLME_DEAUTH_INDICATION(vap, macaddr,
                                                            associd,
                                                            IEEE80211_REASON_AUTH_LEAVE,
                                                            &ni->peer_obj->mldaddr[0]);
#else
             IEEE80211_DELIVER_EVENT_MLME_DEAUTH_INDICATION(vap, macaddr,
                                                            associd,
                                                            IEEE80211_REASON_AUTH_LEAVE,
                                                            NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
        }
    }
}

void
sta_disassoc(void *arg, struct ieee80211_node *ni)
{
    struct ieee80211vap    *vap = ni->ni_vap;
    u_int8_t macaddr[6];
    bool delayed_cleanup = false;
    bool mlo_reconfig = false;
    bool disassoc_peer = true;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_objmgr_psoc *psoc = NULL;
    u_int8_t mldaddr[QDF_MAC_ADDR_SIZE] = {0};
    struct wlan_objmgr_vdev *vdev = NULL;
    struct ieee80211vap *vap1 = NULL;
    struct wlan_mlo_peer_context *ml_peer = NULL;
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct wlan_objmgr_peer *link_peer = NULL;
    bool bridge_peer_present = false;
    int i;
#endif

#ifdef WLAN_FEATURE_11BE_MLO
    /*
     *  Skip sending deauth for MLO peers if the request is coming from
     *  DFS_CAC_WAIT state.
     */
    if ((wlan_mlo_peer_delete_is_not_allowed(vap->vdev_obj)) &&
        (wlan_peer_is_mlo(ni->peer_obj) && (ni->peer_obj->mlo_peer_ctx))) {
        return;
    }
#endif

    if (ieee80211node_has_flag(ni, IEEE80211_NODE_DELAYED_CLEANUP)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: station %s "
                "already marked for cleanup, aid: %d\n", __func__,
                ether_sprintf(ni->ni_macaddr), ni->ni_associd);
        return;
    }

    mlo_reconfig = wlan_vdev_mlme_op_flags_get(vap->vdev_obj, WLAN_VDEV_OP_MLO_STOP_LINK_DEL) ||
                   (wlan_vdev_mlme_op_flags_get(vap->vdev_obj, WLAN_VDEV_OP_MLO_LINK_TBTT_COMPLETE));
#ifdef WLAN_FEATURE_11BE_MLO
    if ((mlo_reconfig || (wlan_peer_get_peer_type(ni->peer_obj) == WLAN_PEER_MLO_BRIDGE))
        && wlan_mlo_partner_peer_delete_is_allowed(ni->peer_obj)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: disassoc station %s on partner link, aid: %d\n",
                          __func__,ether_sprintf(ni->ni_macaddr), ni->ni_associd);
        wlan_mlo_peer_deauth_init(ni->peer_obj->mlo_peer_ctx, ni->peer_obj, 1);
        return;
    }
    disassoc_peer = (!(mlo_reconfig && wlan_peer_is_mlo(ni->peer_obj)));
    ml_peer = ni->peer_obj->mlo_peer_ctx;
    if (!wlan_peer_is_mlo(ni->peer_obj) || !ml_peer)
        goto skip_hw_war;

    /* Search if bridge peer exists */
    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        if (!peer_entry)
            continue;

        link_peer = peer_entry->link_peer;
        if (!link_peer)
            continue;

        if (wlan_peer_get_peer_type(link_peer) == WLAN_PEER_MLO_BRIDGE) {
            bridge_peer_present = true;
            break;
        }
    }

    /*
     * Handling MLO address translation issues in Be
     * Indicate controller through NL event to enable AMSDUs for 3rd party root-AP
     * if only single link peer if left after disassoc
     * Check if mlo_reconfig is enabled and check one of two possible conditions
     * First where 2 links and no bridge peer is present and one link is going down
     * second when 3 links including a bridge peer is present and since one link
     * is going down, so we will be left with single link MLO
     */
    if (mlo_reconfig && (((ml_peer->link_peer_cnt == 2) && !bridge_peer_present) ||
        ((ml_peer->link_peer_cnt == 3) && bridge_peer_present))) {
        for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
            peer_entry = &ml_peer->peer_list[i];
            if (!peer_entry)
                continue;

            link_peer = peer_entry->link_peer;
            if (!link_peer)
                continue;

            if ((link_peer != ni->peer_obj) &&
                (wlan_peer_get_peer_type(link_peer) != WLAN_PEER_MLO_BRIDGE)) {
                    vdev = wlan_peer_get_vdev(link_peer);
                    vap1 = wlan_vdev_get_mlme_ext_obj(vdev);
                    psoc = wlan_pdev_get_psoc(wlan_vdev_get_pdev(vdev));
                    if ((!cfg_get(psoc, CFG_OL_ADV_RAW_MODE_WAR_ENABLE) ||
                        (qdf_mem_cmp(vdev->vdev_mlme.macaddr, vdev->vdev_mlme.mldaddr,
                        QDF_MAC_ADDR_SIZE) == 0)) && (vap1 &&
                        vap1->iv_tx_encap_type == htt_cmn_pkt_type_raw))
                        /*
                         * For cases when ini enable_advance_rawmode_war is true, then send enable
                         * event only when peer mld mac is same as peer link mac
                         */
                        wlan_cfg80211_hw_status_notification(vap1, link_peer, true);
	    }
        }
    }
skip_hw_war:
#endif

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: disassoc station %s, aid: %d\n",
                      __func__,ether_sprintf(ni->ni_macaddr), ni->ni_associd);
    IEEE80211_ADDR_COPY(macaddr, ni->ni_macaddr);
    peer_delete_req_status_set(ni, arg);

    /* Do not send disassoc to MLO peers during link removal */
    if (ni->ni_associd && disassoc_peer) {
        delayed_cleanup = ieee80211_try_mark_node_for_delayed_cleanup(ni);
        ieee80211_send_disassoc_with_callback(ni,
                IEEE80211_REASON_ASSOC_LEAVE,
                ieee80211_mlme_frame_complete_handler, ni);
    }
    if (!delayed_cleanup ||
        ieee80211node_has_extflag(ni, IEEE80211_NODE_DELAYED_CLEANUP_FAIL)) {

#ifdef WLAN_FEATURE_11BE_MLO
        if ((ni->peer_obj->mldaddr) &&
            (IEEE80211_ADDR_IS_VALID(ni->peer_obj->mldaddr)))
            IEEE80211_ADDR_COPY(mldaddr, &ni->peer_obj->mldaddr[0]);

        if (wlan_peer_is_mlo(ni->peer_obj) && !mlo_reconfig)
               wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
#endif
        IEEE80211_NODE_LEAVE(ni);
        if (!mlo_reconfig) {
#ifdef WLAN_FEATURE_11BE_MLO
             IEEE80211_DELIVER_EVENT_MLME_DISASSOC_COMPLETE(vap, macaddr,
                   IEEE80211_REASON_ASSOC_LEAVE, IEEE80211_STATUS_SUCCESS,
                   mldaddr);
#else
             IEEE80211_DELIVER_EVENT_MLME_DISASSOC_COMPLETE(vap, macaddr,
                   IEEE80211_REASON_ASSOC_LEAVE, IEEE80211_STATUS_SUCCESS,
                   NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
        }
    }
}

int wlan_mlme_stop_bss(wlan_if_t vaphandle, int flags)
{
#define WAIT_RX_INTERVAL 10000
#define WAIT_RX_COUNT 10
    u_int32_t                       elapsed_time = 0;
    struct ieee80211vap             *vap = vaphandle;
    int                             error = 0;
    enum wlan_vdev_state state;
    enum wlan_vdev_state substate;
    uint8_t iter_count = 0;
    systime_t start_time;

	if ( vap == NULL ) {
		return EINVAL;
	}

    ieee80211_vap_stop_bss_set(vap);

    /*
     * Reset the vap interface uptime when it is brought down
     */
    vap->vap_uptime = 0;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,
            "%s flags = 0x%x, vdevid:%d: opmode:%d, g_unicast_deauth_on_stop:%d\n",
            __func__, flags, vap->iv_unit, vap->iv_opmode, g_unicast_deauth_on_stop);
    /*
     * Wait for current rx path to finish. Assume only one rx thread.
     */
    if (flags & WLAN_MLME_STOP_BSS_F_WAIT_RX_DONE) {
        start_time = OS_GET_TIMESTAMP();
        do {
            if (OS_ATOMIC_CMPXCHG(&vap->iv_rx_gate, 0, 1) == 0) {
                break;
            }

            OS_SLEEP(WAIT_RX_INTERVAL);
            elapsed_time += WAIT_RX_INTERVAL;

            if (elapsed_time > (100 * WAIT_RX_INTERVAL)) {
                /* Log once every second and assert on timeout of 10 seconds */
               ieee80211_note (vap, IEEE80211_MSG_MLME,
                    "%s: Rx pending count stuck. Investigate!!!\n", __func__);
               elapsed_time = 0;
               iter_count++;
               if (iter_count > WAIT_RX_COUNT) {
                   qdf_err("Time elapsed :%useconds",
                           CONVERT_SYSTEM_TIME_TO_SEC(OS_GET_TIMESTAMP() - start_time));
                   QDF_BUG(0);
               }
            }
        } while (1);
    }

    state = wlan_vdev_mlme_get_state(vap->vdev_obj);
    substate = wlan_vdev_mlme_get_substate(vap->vdev_obj);
     /* For STA VDEV, Don't trigger EV_DOWN, if it is in START/RESTART substates
        or INIT states */
    if (!((vap->iv_opmode == IEEE80211_M_STA) &&
          (((state == WLAN_VDEV_S_START) &&
           ((substate == WLAN_VDEV_SS_START_START_PROGRESS) ||
           (substate == WLAN_VDEV_SS_START_RESTART_PROGRESS))))))
        wlan_vdev_mlme_sm_deliver_evt(vap->vdev_obj, WLAN_VDEV_SM_EV_DOWN,
                                      0, NULL);
    /*
     * Release the rx mutex.
     */
    if (flags & WLAN_MLME_STOP_BSS_F_WAIT_RX_DONE) {
        (void) OS_ATOMIC_CMPXCHG(&vap->iv_rx_gate, 1, 0);
    }
    ieee80211_vap_stop_bss_clear(vap);

    return error;
#undef WAIT_RX_INTERVAL
}

/* return true if an mlme operation is in progress */
bool wlan_mlme_operation_in_progress(wlan_if_t vaphandle)
{
    struct ieee80211vap           *vap = vaphandle;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;

    return (mlme_priv->im_request_type != MLME_REQ_NONE);
}


/* Cancel any pending MLME request */
int wlan_mlme_cancel(wlan_if_t vaphandle)
{
    struct ieee80211vap           *vap = vaphandle;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    /* Cancel pending timer */
    if (OS_CANCEL_TIMER(&mlme_priv->im_timeout_timer) &&
        (mlme_priv->im_request_type != MLME_REQ_NONE))
    {
        /* Invoke the timeout routine */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,"%s", "Trigger early timeout\n");
        mlme_priv->im_join_is_timeout = 1;
        mlme_timeout_callback(vap, IEEE80211_STATUS_CANCEL);

    }

    return 0;
}

/* Reset Connection */
int wlan_mlme_connection_reset(wlan_if_t vaphandle,
                               enum conn_reset_reason reset_reason)
{
    struct ieee80211vap           *vap = vaphandle;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    enum wlan_vdev_state state;
    enum wlan_vdev_state substate;


    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    if ((reset_reason != SM_REPEATER_CAC_STATE) &&
        (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS))
        reset_reason = SM_CONNECTED_STATE;

    /* There should be no mlme requests pending */
    ASSERT(vap->iv_mlme_priv->im_request_type == MLME_REQ_NONE);

    /* Reset state variables */
    mlme_priv->im_connection_up = 0;

    state = wlan_vdev_mlme_get_state(vap->vdev_obj);
    substate = wlan_vdev_mlme_get_substate(vap->vdev_obj);
    if (reset_reason != SM_REPEATER_CAC_STATE) {
             /* Association failed, put underlying H/W back to init state.
                this event would move VDEV SM into stopping state, VDEV SM
                moves to INIT state, on receiving event from connection_sm */
         if (!((state == WLAN_VDEV_S_START) &&
             ((substate == WLAN_VDEV_SS_START_START_PROGRESS) ||
             (substate == WLAN_VDEV_SS_START_RESTART_PROGRESS) ||
             (substate == WLAN_VDEV_SS_START_DISCONN_PROGRESS))))
               wlan_vdev_mlme_sm_deliver_evt(vap->vdev_obj, WLAN_VDEV_SM_EV_DOWN,
                                             0, NULL);
    }

    return 0;
}

/* Start monitor mode */
int wlan_mlme_start_monitor(wlan_if_t vaphandle)
{
    struct ieee80211vap    *vap = vaphandle;
    struct ieee80211com    *ic  = vap->iv_ic;
    ieee80211_reset_request  req;
    int hash;
    wlan_if_t tmpvap;
    struct ieee80211_ath_channel chan_obj = empty_channel;
    struct ieee80211_ath_channel *chan = &chan_obj;
    struct wlan_channel *iter_vdev_chan = NULL;
    struct chan_params chan_p = {0};

    ASSERT(vap->iv_opmode == IEEE80211_M_MONITOR);

    IEEE80211_VAP_LOCK(vap);
    for (hash = 0; hash < IEEE80211_NODE_HASHSIZE; hash++)
        LIST_INIT(&vap->mac_filter_hash[hash]);
    IEEE80211_VAP_UNLOCK(vap);


    OS_MEMZERO(&req, sizeof(req));
    req.reset_hw = 1;
    req.type = IEEE80211_RESET_TYPE_INTERNAL;
    req.no_flush = 0;
    wlan_reset_start(vap, &req);
    wlan_reset(vap, &req);
    wlan_reset_end(vap, &req);

    /* Monitor mode vap is always initialized with base channel (1 or 36), if no
     * channel is configured or no other vap is beaconing, it can operate on
     * that channel. Otherwise, it should check other VDEVs' channel and start
     * beaconing in those channels,
     * Below logic iterates through all VDEVs and finds the operating channel,
     * if no VDEV is operating, it would use this VDEV's channel as operating
     * channel
     */
    if (IEEE80211_IS_CHAN_VALID(vap->iv_des_chan[vap->iv_des_mode])) {
       *chan = *vap->iv_des_chan[vap->iv_des_mode];
    }
    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
       iter_vdev_chan = wlan_vdev_get_active_channel(tmpvap->vdev_obj);
       if (iter_vdev_chan) {
           break;
       }
    }

    if (iter_vdev_chan) {
        wlan_chan_copy(vap->vdev_obj->vdev_mlme.des_chan, iter_vdev_chan);
    }
    else {
        if (!IEEE80211_IS_CHAN_VALID(chan))
            *chan = *ic->ic_curchan;

        if (!IEEE80211_IS_CHAN_VALID(chan))
            *chan = ieee80211_find_dot11_channel(ic, &chan_p, vap->iv_des_mode);

        if (!IEEE80211_IS_CHAN_VALID(chan)) {
            qdf_err("Channel is NULL for monitor vdev");
            wlan_mlme_release_vdev_req(vap->vdev_obj,
                                       WLAN_SER_CMD_VDEV_START_BSS, EINVAL);
            return 0;
        }

        ieee80211_update_vdev_chan(vap->vdev_obj->vdev_mlme.des_chan, chan);
    }

    wlan_vdev_mlme_sm_deliver_evt(vap->vdev_obj, WLAN_VDEV_SM_EV_START, 0, NULL);
    return 0;
}

int wlan_mlme_parse_appie(struct ieee80211vap *vap, ieee80211_frame_type ftype,const  u_int8_t *buf, u_int16_t buflen)
{
    struct ieee80211_ie_header *ie = (struct ieee80211_ie_header *)buf;
    int final_buflen = 0;
    int sta_count = 0;
    if (ftype == IEEE80211_FRAME_TYPE_BEACON) {
        while ((u_int8_t *)ie < buf + buflen) {
#if ATH_SUPPORT_HS20
            if (ie->element_id == IEEE80211_ELEMID_INTERWORKING) {
                u_int8_t *val;
                val = (u_int8_t *)(ie + 1);
                vap->iv_access_network_type = val[0] & 0xF;
                if (ie->length == 7)
                    IEEE80211_ADDR_COPY(vap->iv_hessid, val + 1);
                if (ie->length == 9)
                    IEEE80211_ADDR_COPY(vap->iv_hessid, val + 3);
            }
#endif
            if (ie->element_id == IEEE80211_ELEMID_XCAPS) {
                /* copy xcaps and delete ie from appie */
                u_int8_t *delbuf = (u_int8_t *)ie;
		int is_beacon_prot_extflag_set;
                struct ieee80211_ie_ext_cap *elem_extcap =
                    (struct ieee80211_ie_ext_cap *)ie;

#if ATH_SUPPORT_HS20
                vap->iv_hotspot_xcaps = le32toh(elem_extcap->ext_capflags);
                if (elem_extcap->elem_len > sizeof(vap->iv_hotspot_xcaps)) {
                    vap->iv_hotspot_xcaps2 = le32toh(elem_extcap->ext_capflags2);
                }
#endif

		if ((elem_extcap->elem_len >= 11) && (elem_extcap->ext_capflags5 & IEEE80211_EXTCAPIE_BEACON_PROTECTION))
			is_beacon_prot_extflag_set = 1;
		else
			is_beacon_prot_extflag_set = 0;

		if (vap->iv_beacon_prot ^ is_beacon_prot_extflag_set) {
		    sta_count = wlan_iterate_all_sta_list(vap, NULL, NULL);
		    if (sta_count) {
			qdf_err("Beacon  proto value shouldnt be changed when sta is connected for %s \n",vap->iv_netdev_name);
		    }
		    else {
			vap->iv_beacon_prot = is_beacon_prot_extflag_set;
			if (vap->iv_beacon_prot)
				qdf_err("%s Beacon Protection is enabled\n", vap->iv_netdev_name);
			else
				qdf_err("%s Beacon Protection is disabled\n", vap->iv_netdev_name);
                    }
		}

                buflen -= ie->length + 2;
                OS_MEMMOVE(delbuf, delbuf + ie->length + 2, buflen - final_buflen);
                ie = (struct ieee80211_ie_header *)delbuf;
                continue;
            }
/*
            if (ie->element_id == IEEE80211_ELEMID_TIME_ADVERTISEMENT) {
                struct ieee80211com    *ic = vap->iv_ic;
                val = (u_int8_t *)(ie + 1);
                if (val[0] == 2 && ie->length > 10) {
                    ieee80211_vap_tsf_offset tsf_offset_info;
                    u_int64_t tsf = ic->ic_get_TSF64(ic);
                    u_int32_t ie_msecs, msecs;

                    ieee80211_vap_get_tsf_offset(vap, &tsf_offset_info);

                    if (tsf_offset_info.offset_negative) {
                        tsf -= tsf_offset_info.offset;
                    } else {
                        tsf += tsf_offset_info.offset;
                    }
#define MSECS_IN_HOUR (60 * 60 * 1000)
#define MSECS_IN_MIN  (60 * 1000)
#define MSECS_IN_SEC  (1000)
                    ie_msecs = val[5] * MSECS_IN_HOUR  + val[6] * MSECS_IN_MIN + val[7] * MSECS_IN_SEC + le16toh(*(u_int16_t *)(val + 8));
                    msecs = tsf / 1000;
                    if (ie_msecs > msecs) {
                        ie_msecs -= msecs;
                        val[5] = ie_msecs / MSECS_IN_HOUR;
                        ie_msecs -= val[5] * MSECS_IN_HOUR;
                        val[6] = ie_msecs / MSECS_IN_MIN;
                        ie_msecs -= val[6] * MSECS_IN_MIN;
                        val[7] = ie_msecs / MSECS_IN_SEC;
                        ie_msecs -= val[7] * MSECS_IN_SEC;
                        val[8] = htole16((u_int16_t)ie_msecs) >> 8;
                        val[9] = htole16((u_int16_t)ie_msecs) & 0xFF;
                    }
                    else {
                        qdf_nofl_info("FIXME: Time Advt IE crossing day boundary\n");
                    }
#undef MSECS_IN_HOUR
#undef MSECS_IN_MIN
#undef MSECS_IN_SEC
                }
            }
*/
            final_buflen += ie->length + 2;
            ie = (struct ieee80211_ie_header *)((u_int8_t *)ie + ie->length + 2);
        }
    }
    else if ((ftype == IEEE80211_FRAME_TYPE_PROBEREQ) ||
             (ftype == IEEE80211_FRAME_TYPE_PROBERESP) ||
             (ftype == IEEE80211_FRAME_TYPE_ASSOCREQ) ||
             (ftype == IEEE80211_FRAME_TYPE_ASSOCRESP))  {
        while ((u_int8_t *)ie < buf + buflen) {
            if (ie->element_id == IEEE80211_ELEMID_XCAPS) {
                /* delete this ie from appie */
                u_int8_t *delbuf = (u_int8_t *)ie;
		int is_beacon_prot_extflag_set;
                struct ieee80211_ie_ext_cap *elem_extcap =
                    (struct ieee80211_ie_ext_cap *)ie;
                if ((elem_extcap->elem_len >= 11) && (elem_extcap->ext_capflags5 & IEEE80211_EXTCAPIE_BEACON_PROTECTION))
                    is_beacon_prot_extflag_set = 1;
                else
                    is_beacon_prot_extflag_set = 0;

                if (vap->iv_beacon_prot ^ is_beacon_prot_extflag_set) {
		    sta_count = wlan_iterate_all_sta_list(vap, NULL, NULL);
                    if (sta_count)
			qdf_err("Beacon proto value shouldnt be changed when sta is connected for %s \n",vap->iv_netdev_name);
                    else {
                        vap->iv_beacon_prot = is_beacon_prot_extflag_set;
			if (vap->iv_beacon_prot)
				qdf_err("%s Beacon Protection is enabled\n", vap->iv_netdev_name);
			else
				qdf_err("%s Beacon Protection is disabled\n", vap->iv_netdev_name);
                    }
                }
                buflen -= ie->length + 2;
                OS_MEMMOVE(delbuf, delbuf + ie->length + 2, buflen - final_buflen);
                ie = (struct ieee80211_ie_header *)delbuf;
                continue;
            }
            final_buflen += ie->length + 2;
            ie = (struct ieee80211_ie_header *)((u_int8_t *)ie + ie->length + 2);
        }
    }
    else {
        final_buflen = buflen;
    }
    return final_buflen;
}

/*
 * ieee80211_delete_duplicate_ies: Delete the IE passed from list
 *
 * @vap        : VAP handle
 * @ftype      : Frame type to which duplicate IEs are to be removed
 * @key_ie     : IE whose duplicate to be removed
 * @key_ie_len : Length of the key_ie
 */
static inline void ieee80211_delete_duplicate_ies(struct ieee80211vap *vap,
        ieee80211_frame_type ftype, uint8_t *key_ie, uint32_t key_ie_len)
{
    struct app_ie_entry *ie_entry, *next_entry;

    STAILQ_FOREACH_SAFE(ie_entry, &vap->iv_app_ie_list[ftype], link_entry, next_entry) {
        if (ie_entry->app_ie.ie == NULL || ie_entry->app_ie.length <= 0) {
            return;
        }

        if (ie_entry->app_ie.ie[0] == key_ie[0]) {

            /* Check for vendor and extension IEs */
            if(key_ie[0] != IEEE80211_ELEMID_VENDOR
                    || !(OS_MEMCMP(&ie_entry->app_ie.ie[2],&key_ie[2],4))) {
                STAILQ_REMOVE(&vap->iv_app_ie_list[ftype], ie_entry, app_ie_entry, link_entry);
                vap->iv_app_ie_list[ftype].total_ie_len -= ie_entry->app_ie.length;
                qdf_mem_free(ie_entry->app_ie.ie);
                qdf_mem_free(ie_entry);
                return;
            }
        }
    }
}

/* Set application defined IEs */
int wlan_mlme_set_appie(wlan_if_t vaphandle, ieee80211_frame_type ftype, const u_int8_t *buf, u_int16_t buflen)
{
    struct ieee80211vap    *vap = vaphandle;
    u_int8_t               *iebuf = NULL;
    int                    i = 0;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s, ftype=%x, ie_len=%x\n", __func__,ftype, buflen ) ;

    ASSERT(ftype < IEEE80211_FRAME_TYPE_MAX);

    if (ftype >= IEEE80211_FRAME_TYPE_MAX) {
        return -EINVAL;
    }

    buflen = wlan_mlme_parse_appie(vap, ftype, buf, buflen);

    if (buflen==0) {
	return 0;
    }

    iebuf = qdf_mem_malloc(buflen*sizeof(u_int8_t));
    if (iebuf == NULL) {
        return -ENOMEM;
    }
    qdf_mem_copy(iebuf, buf, buflen);

    IEEE80211_VAP_LOCK(vap);
    IEEE80211_VAP_APPIE_UPDATE_DISABLE(vap);
    while ((i+1 < buflen) && (iebuf[i+1]+1 < buflen)) {
        if (IEEE80211_VAP_IS_HIDESSID_ENABLED(vap)
                && (ftype == IEEE80211_FRAME_TYPE_BEACON) && iswpsoui(&iebuf[i])) {
            i += iebuf[i+1] + 2;
            continue;
        } else {
            struct app_ie_entry *ie_entry;

            /* Remove duplicates */
            ieee80211_delete_duplicate_ies(vap, ftype, &iebuf[i], iebuf[i+1] + 2);

            ie_entry = (struct app_ie_entry *)qdf_mem_malloc(sizeof(struct app_ie_entry));
            if (!ie_entry) {
                IEEE80211_VAP_APPIE_UPDATE_ENABLE(vap);
                IEEE80211_VAP_UNLOCK(vap);
                return -ENOMEM;
            }
            if (iswpsoui(&iebuf[i]))
                wlan_set_param(vap, IEEE80211_WPS_MODE, 1);

            ie_entry->app_ie.ie = &iebuf[i];
            ie_entry->app_ie.length = iebuf[i+1] + 2;
            ie_entry->identifier = DEFAULT_IDENTIFIER;
            STAILQ_INSERT_TAIL(&vap->iv_app_ie_list[ftype], ie_entry, link_entry);
            vap->iv_app_ie_list[ftype].total_ie_len += ie_entry->app_ie.length;
            vap->iv_app_ie_list[ftype].changed = true;

            i += iebuf[i+1] + 2;
        }
    }

    IEEE80211_VAP_APPIE_UPDATE_ENABLE(vap);
    IEEE80211_VAP_UNLOCK(vap);

    if (ftype == IEEE80211_FRAME_TYPE_BEACON) {
        vap->iv_reinit_beacon = true;
        wlan_vdev_beacon_update(vap);
    }
    return 0;
}

/* Get application defined IEs */
int wlan_mlme_get_appie(wlan_if_t vaphandle, ieee80211_frame_type ftype, u_int8_t *buf, u_int32_t *ielen, u_int32_t buflen, u_int8_t identifier)
{
    struct ieee80211vap    *vap = vaphandle;
    int                    error = 0;
    struct app_ie_entry *ie_entry, *next_entry;
    *ielen = 0;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    ASSERT(ftype < IEEE80211_FRAME_TYPE_MAX);

    if (ftype >= IEEE80211_FRAME_TYPE_MAX) {
        error = -EINVAL;
        goto exit;
    }
    if(vap->vie_handle == NULL)
    {
        qdf_nofl_info("vie handle is NULL");
        error = -EINVAL;
        goto exit;
    }

    IEEE80211_VAP_LOCK(vap);

    STAILQ_FOREACH_SAFE(ie_entry, &vap->iv_app_ie_list[ftype], link_entry,next_entry)
    {
        if(ie_entry->identifier == identifier)
        {
            if(ie_entry->app_ie.length != 0)
            {
                if(buflen < (*ielen + ie_entry->app_ie.length))
                {
                    qdf_nofl_info("buffer length exceeded");
                    error = -EINVAL;
                    IEEE80211_VAP_UNLOCK(vap);
                    goto exit;
                }
                OS_MEMCPY(&buf[*ielen], ie_entry->app_ie.ie, ie_entry->app_ie.length);
                (*ielen) += ie_entry->app_ie.length;
            }
        }
    }

    IEEE80211_VAP_UNLOCK(vap);

exit:
    return error;
}

/* Set optional application defined IEs */
int wlan_mlme_set_optie(wlan_if_t vaphandle, u_int8_t *buf, u_int16_t buflen)
{
    struct ieee80211vap    *vap = vaphandle;
    struct ieee80211com    *ic = vap->iv_ic;
    int                    error = 0;
    u_int8_t               *iebuf = NULL;
    bool                   alloc_iebuf = FALSE;
    u_int8_t *rsn_ie = NULL;
    u_int8_t *wpa_ie = NULL;
    u_int8_t *ptr = buf;
    struct wlan_crypto_params crypto_params = {0} ;
    int status = IEEE80211_STATUS_SUCCESS;
    unsigned int args[2] = {0,0};

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    if (buflen > vap->iv_opt_ie_maxlen) {
        /* Allocate ie buffer */
        iebuf = OS_MALLOC(ic->ic_osdev, buflen, 0);

        if (iebuf == NULL) {
            error = -ENOMEM;
            goto exit;
        }

        alloc_iebuf = TRUE;
    }

    IEEE80211_VAP_LOCK(vap);

    /* Free existing buffer */
    if (alloc_iebuf == TRUE) {
        vap->iv_opt_ie_maxlen = buflen;
        if (vap->iv_opt_ie.ie) {
            OS_FREE(vap->iv_opt_ie.ie);
        }
        vap->iv_opt_ie.ie = iebuf;
    }

    vap->iv_opt_ie.length = buflen;

    if (buflen) {
        ASSERT(buf);
        if (buf == NULL) {
            IEEE80211_VAP_UNLOCK(vap);
            error = -EINVAL;
            goto exit;
        }

        /* Copy app ie contents and save pointer/length */
        OS_MEMCPY(vap->iv_opt_ie.ie, buf, buflen);
    }
    IEEE80211_VAP_UNLOCK(vap);

    while (((ptr + 1) < buf + buflen) && (ptr + ptr[1] + 1 < buf + buflen)) {
        if (ptr[0] == WLAN_ELEMID_RSN && ptr[1] >= 20 ){
            rsn_ie = ptr;
        } else if (ptr[0] == WLAN_ELEMID_VENDOR && iswpaoui(ptr) ){
            wpa_ie = ptr;
        }
        ptr += ptr[1] + 2;
    }

    if (wpa_ie != NULL)
    {
        status = wlan_crypto_wpaie_check((struct wlan_crypto_params *)&crypto_params, wpa_ie);
        if (status != QDF_STATUS_SUCCESS) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_CRYPTO,"wpa ie unavailable\n");
        }
    } else if (rsn_ie != NULL) {
        status = wlan_crypto_rsnie_check((struct wlan_crypto_params *)&crypto_params, rsn_ie);
        if (status != QDF_STATUS_SUCCESS) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_CRYPTO,"rsn ie unavailable\n");
        }
    }

    if ((!wpa_ie && !rsn_ie) || status != QDF_STATUS_SUCCESS)
        crypto_params.rsn_caps = 0;
    ieee80211_ucfg_setparam(vap,IEEE80211_PARAM_RSNCAPS, crypto_params.rsn_caps ,(char*)args);

exit:
    return error;
}


/* Get optional application defined IEs */
int wlan_mlme_get_optie(wlan_if_t vaphandle, u_int8_t *buf, u_int32_t *ielen, u_int32_t buflen)
{
    struct ieee80211vap    *vap = vaphandle;
    int                    error = 0;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    *ielen = vap->iv_opt_ie.length;

    /* verify output buffer is large enough */
    if (buflen < vap->iv_opt_ie.length) {
        error = -EOVERFLOW;
        goto exit;
    }

    IEEE80211_VAP_LOCK(vap);
    /* copy opt ie contents to output buffer */
    if (*ielen) {
        OS_MEMCPY(buf, vap->iv_opt_ie.ie, vap->iv_opt_ie.length);
    }
    IEEE80211_VAP_UNLOCK(vap);

exit:
    return error;
}

#if DBDC_REPEATER_SUPPORT && ATH_SUPPORT_WRAP
static int vap_is_psta(struct ieee80211vap *vap)
{
    if( !wlan_rptr_vdev_is_mpsta(vap->vdev_obj) && wlan_rptr_vdev_is_psta(vap->vdev_obj)) {
        return 1;
    } else {
        return 0;
    }
}
#endif

#if DBDC_REPEATER_SUPPORT
#ifdef WLAN_FEATURE_11BE_MLO
static void wlan_set_dbdc_primary_in_mlo(struct ieee80211vap *vap) {
    struct pdev_osif_priv *pdev_ospriv = NULL, *pdev_ospriv_temp = NULL;
    struct wiphy *wiphy = NULL, *primary_wiphy = NULL, *wiphy_temp = NULL;
    struct wlan_objmgr_pdev *pdev = NULL, *pdev_temp = NULL;
    struct wlan_mlo_peer_context *ml_peer = NULL;
    struct ieee80211_node *ni = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;
    uint8_t primary_psoc_id = 0, psoc_id = 0, i = 0;
    struct net_device *primary_umac_sta_dev = NULL;
    struct ieee80211com *ic = NULL;

    /* Get primary PSOC ID from ml_peer */
    ni = vap->iv_bss;
    if (wlan_peer_is_mlo(ni->peer_obj)) {
        ml_peer = ni->peer_obj->mlo_peer_ctx;
        primary_psoc_id = ml_peer->primary_umac_psoc_id;
    }

    psoc = wlan_pdev_get_psoc(wlan_vdev_get_pdev(vap->vdev_obj));
    psoc_id = psoc->soc_objmgr.psoc_id;

    /* Check if current PSOC is same as the primary PSOC */
    if (ml_peer && (primary_psoc_id == psoc_id)) {
        pdev = wlan_vdev_get_pdev(vap->vdev_obj);
        pdev_ospriv = wlan_pdev_get_ospriv(pdev);
        wiphy = pdev_ospriv->wiphy;
        primary_wiphy = qca_multi_link_get_primary_radio();

        /* Check if the primary radio is set for primary PSOC */
        if (primary_wiphy && (primary_wiphy != wiphy)) {
            if (vap->vdev_obj->mlo_dev_ctx) {
                mlo_dev_lock_acquire(vap->vdev_obj->mlo_dev_ctx);

                /* Loop through the various vdevs present in the mlo_dev_ctx
                 * and check if the DBDC primary radio is set for any of the
                 * PDEV (excluding the current PDEV) in that list. If primary
                 * radio is set of any one in that list, make the PDEV of the
                 * primary PSOC also as DBDC primary radio.
                 */

                for (i =  0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
                     if (!vap->vdev_obj->mlo_dev_ctx->wlan_vdev_list[i]) {
                         qdf_info("No vdev in vdev list\n");
                         continue;
                     } else {
                         pdev_temp = wlan_vdev_get_pdev(vap->vdev_obj->mlo_dev_ctx->wlan_vdev_list[i]);
                         if (pdev_temp != pdev) {
                             pdev_ospriv_temp = wlan_pdev_get_ospriv(pdev_temp);
                             wiphy_temp = pdev_ospriv_temp->wiphy;

                             if (primary_wiphy == wiphy_temp) {
                                 primary_umac_sta_dev = qca_multi_link_get_station_vap(wiphy);
                                 qdf_info("Setting %s as Multi link primary radio\n", primary_umac_sta_dev->name);
                                 qca_multi_link_set_primary_radio(wiphy);
                                 ic = wlan_vdev_get_ic(vap->vdev_obj);
                                 if (!ic) {
                                     qdf_err("ic is NULL for %s radio", primary_umac_sta_dev->name);
                                     mlo_dev_lock_release(vap->vdev_obj->mlo_dev_ctx);
                                     return;
                                 }
                                 break;
                             }
                         }
                     }
                }
                mlo_dev_lock_release(vap->vdev_obj->mlo_dev_ctx);
                if (ic) {
                    GLOBAL_IC_LOCK_BH(ic->ic_global_list);
                    if (ic->ic_global_list->dbdc_process_enable) {
                        vap->iv_ic->ic_radio_priority = 1;
                        ic->ic_global_list->max_priority_stavap_up = vap;
                    }
                    GLOBAL_IC_UNLOCK_BH(ic->ic_global_list);
                }
            }
        }
    }
}
#endif /* WLAN_FEATURE_11BE_MLO */
#endif /* DBDC_REPEATER_SUPPORT */

/* Notify connection state (up/down)
 *
 * Mlme will not indicate node assoc/disassoc until the connection state
 * is set to "up".
 *
 * For example, on Vista, the driver would indicate to the OS that the
 * connection is "up" and then notify mlme that the connection is "up".
 * This prevents mlme from indicating node assoc, before the OS connection
 * is established.
 *
 */
void wlan_mlme_connection_up(wlan_if_t vaphandle)
{
    struct ieee80211vap           *vap = vaphandle;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    ieee80211_vap_event           evt;
    struct ieee80211vap           *first_vap = NULL;
#if DBDC_REPEATER_SUPPORT
    struct ieee80211com           *ic = vap->iv_ic;
    osif_dev  *osdev = (osif_dev *)vap->iv_ifp;
    struct net_device *dev = OSIF_TO_NETDEV(osdev);
#endif

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

#if ATH_SUPPORT_WRAP
    if ((vap->iv_opmode == IEEE80211_M_STA) && wlan_rptr_vdev_is_psta(vap->vdev_obj)
	    && (mlme_priv->im_connection_up == 0)) {
	dp_wrap_mat_params_update(vap->vdev_obj);
    }
#endif
#if DBDC_REPEATER_SUPPORT
    if ((vap->iv_opmode == IEEE80211_M_STA) && (mlme_priv->im_sta_connection_up == 0)
#if ATH_SUPPORT_WRAP
    && !vap_is_psta(vap)
#endif
    ) {
        mlme_priv->im_connection_up = 1;
        mlme_priv->im_sta_connection_up = 1;

        GLOBAL_IC_LOCK_BH(ic->ic_global_list);
        ic->ic_global_list->num_stavaps_up++;
        GLOBAL_IC_UNLOCK_BH(ic->ic_global_list);
        osif_vap_activity_update(vap);
        osif_vap_tx_activity_update(vap);
#ifdef WLAN_FEATURE_11BE_MLO
        wlan_set_dbdc_primary_in_mlo(vap);
#endif

        /* For MLO, update dbdc process with assoc link vdev only */
        if (!wlan_vdev_mlme_is_mlo_vdev(vap->vdev_obj) ||
            wlan_vdev_mlme_is_assoc_sta_vdev(vap->vdev_obj))
            wlan_rptr_conn_up_dbdc_process(vap->vdev_obj, dev);

    } else {
        mlme_priv->im_connection_up = 1;
        osif_vap_tx_activity_update(vap);
    }
#else
    mlme_priv->im_connection_up = 1;
#endif

    if( vap->iv_opmode != IEEE80211_M_STA || !ieee80211_auth_mode_needs_upper_auth(vap) ){
        evt.type = IEEE80211_VAP_AUTH_COMPLETE;
        ieee80211_vap_deliver_event(vap, &evt);
    }

    if(ieee80211_is_phymode_auto(vap->iv_des_mode) && vap->iv_opmode == IEEE80211_M_HOSTAP) {
        TAILQ_FOREACH(first_vap, &(vap->iv_ic)->ic_vaps, iv_next) {
            if (!first_vap->iv_diff_mode)
                break;
        }
        if (first_vap != NULL && first_vap != vap) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: changing mode from %d to %d\n", __func__, vap->iv_des_mode, first_vap->iv_des_mode);
            wlan_set_desired_phymode(vap, first_vap->iv_des_mode);
        }
    }
}

void wlan_mlme_connection_down(wlan_if_t vaphandle)
{
    struct ieee80211vap           *vap = vaphandle;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
#if DBDC_REPEATER_SUPPORT
    struct ieee80211com           *ic = vap->iv_ic;
    osif_dev  *osdev = (osif_dev *)vap->iv_ifp;
    struct net_device *dev = OSIF_TO_NETDEV(osdev);
#endif
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

#if DBDC_REPEATER_SUPPORT
    if ((vap->iv_opmode == IEEE80211_M_STA) && (mlme_priv->im_sta_connection_up == 1)
#if ATH_SUPPORT_WRAP
       && !vap_is_psta(vap)
#endif
    ) {
        mlme_priv->im_connection_up = 0;
        mlme_priv->im_sta_connection_up = 0;

        GLOBAL_IC_LOCK_BH(ic->ic_global_list);
        ic->ic_global_list->num_stavaps_up--;
        GLOBAL_IC_UNLOCK_BH(ic->ic_global_list);

        /* For MLO, update dbdc process with assoc link vdev only */
        if (!wlan_vdev_mlme_is_mlo_vdev(vap->vdev_obj) ||
            wlan_vdev_mlme_is_assoc_sta_vdev(vap->vdev_obj))
            wlan_rptr_conn_down_dbdc_process(vap->vdev_obj, dev);

#if QCA_AIRTIME_FAIRNESS
        /* Trigger peer join leave for atf peers */
        if (vap->iv_bss)
            wlan_atf_peer_join_leave(vap->iv_bss->peer_obj, 0);
#endif
#if REPEATER_SAME_SSID
        wlan_rptr_s_ssid_vdev_down_process(vap->vdev_obj);
#endif
    } else {
        mlme_priv->im_connection_up = 0;
    }
#else
    mlme_priv->im_connection_up = 0;
#endif
}

int ieee80211_mlme_recv_auth(struct ieee80211_node *ni,
                              u_int16_t algo, u_int16_t seq, u_int16_t status_code,
                              u_int8_t *challenge, u_int8_t challenge_length, wbuf_t wbuf,
                              const struct ieee80211_rx_status *rs)

{
    struct ieee80211vap           *vap = ni->ni_vap;
    int ret_val = EOK;

    switch (vap->iv_opmode) {
    case IEEE80211_M_STA:
        if (algo == IEEE80211_AUTH_ALG_PASN)
            return ret_val;

        ret_val = mlme_recv_auth_sta(ni,algo,seq,status_code,challenge,challenge_length,wbuf);
        break;

    case IEEE80211_M_HOSTAP:
        ret_val = mlme_recv_auth_ap(ni,algo,seq,status_code,challenge,challenge_length,wbuf,rs);
        break;

    default:
        break;
    }
#ifdef QCA_SUPPORT_CP_STATS
    vdev_cp_stats_mlme_auth_attempt_inc(vap->vdev_obj, 1);
#endif

  return ret_val;
}

void ieee80211_mlme_recv_deauth(struct ieee80211_node *ni, u_int16_t reason_code)
{
    struct ieee80211vap           *vap = ni->ni_vap;
    u_int16_t associd;
#if QCA_SUPPORT_SON
    struct wlan_client_disconnect_ind event_data = {0};
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_rx_ops *rx_ops = NULL;
#endif

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    switch(vap->iv_opmode) {
    case IEEE80211_M_HOSTAP:
    case IEEE80211_M_BTAMP:
        if (ni != vap->iv_bss) {
            if (!ieee80211_try_ref_node(ni, WLAN_MLME_HANDLER_ID))
                return;

#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_peer_is_mlo(ni->peer_obj)) {
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
            }
#endif
            associd = ni->ni_associd;
#if QCA_SUPPORT_SON
            wlan_node_get_disassoc_stats(vap, ni->peer_obj, &event_data.sta_stats_event_data);
#endif
            if(IEEE80211_NODE_LEAVE(ni)) {
#if QCA_SUPPORT_SON
                psoc = wlan_vdev_get_psoc(vap->vdev_obj);
                if (psoc) {
                    rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
                }
#endif
                /* Call MLME indication handler if node is in associated state */
#ifdef WLAN_FEATURE_11BE_MLO
                IEEE80211_DELIVER_EVENT_MLME_DEAUTH_INDICATION(vap,
                                                               ni->ni_macaddr,
                                                               associd,
                                                               reason_code,
                                                               &ni->peer_obj->mldaddr[0]);
#else
                IEEE80211_DELIVER_EVENT_MLME_DEAUTH_INDICATION(vap,
                                                               ni->ni_macaddr,
                                                               associd,
                                                               reason_code,
                                                               NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
#if ATH_PARAMETER_API
                ieee80211_papi_send_assoc_event(vap, ni, PAPI_STA_DISASSOCIATION);
#endif
#if QCA_SUPPORT_SON
                qdf_mem_copy(event_data.disconnect_event_data.client_addr, ni->ni_macaddr, QDF_MAC_ADDR_SIZE);
                event_data.disconnect_event_data.source = BSTEER_SOURCE_REMOTE;
                event_data.disconnect_event_data.type = BSTEER_DISASSOC;
                event_data.disconnect_event_data.reason = reason_code;
                if (rx_ops && rx_ops->son_rx_ops.deliver_event)
                    rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, ni->peer_obj, MLME_EVENT_CLIENT_DISCONNECTED, &event_data);
#endif

            }
            ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);
        }
        break;

    default:
        /* Call MLME indication handler */
#ifdef WLAN_FEATURE_11BE_MLO
        IEEE80211_DELIVER_EVENT_MLME_DEAUTH_INDICATION(vap, ni->ni_bssid, 0,
                                                       reason_code,
                                                       &ni->peer_obj->mldaddr[0]);
#else
        IEEE80211_DELIVER_EVENT_MLME_DEAUTH_INDICATION(vap, ni->ni_bssid, 0,
                                                       reason_code, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
    }
}

void ieee80211_mlme_recv_disassoc(struct ieee80211_node *ni, u_int32_t reason_code)
{
    struct ieee80211vap           *vap = ni->ni_vap;
    u_int16_t associd = 0;
#if QCA_SUPPORT_SON
    struct wlan_client_disconnect_ind event_data = {0};
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_rx_ops *rx_ops = NULL;
#endif

    IEEE80211_NOTE(vap, IEEE80211_MSG_MLME, ni, "%s\n", __func__);

    switch(vap->iv_opmode) {
    case IEEE80211_M_HOSTAP:
    case IEEE80211_M_BTAMP:
        if (ni != vap->iv_bss) {
            if (!ieee80211_try_ref_node(ni, WLAN_MLME_HANDLER_ID))
                return;

#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_peer_is_mlo(ni->peer_obj)) {
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
            }
#endif
            associd = ni->ni_associd;
#if QCA_SUPPORT_SON
            wlan_node_get_disassoc_stats(vap, ni->peer_obj, &event_data.sta_stats_event_data);
#endif
            if(IEEE80211_NODE_LEAVE(ni)) {
#if QCA_SUPPORT_SON
                psoc = wlan_vdev_get_psoc(vap->vdev_obj);
                if (psoc) {
                    rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
                }
#endif
                /* Call MLME indication handler if node is in associated state */
#ifdef WLAN_FEATURE_11BE_MLO
                IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap, ni->ni_macaddr, associd, reason_code, &ni->peer_obj->mldaddr[0]);
#else

                IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap, ni->ni_macaddr, associd, reason_code, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
#if ATH_PARAMETER_API
                ieee80211_papi_send_assoc_event(vap, ni, PAPI_STA_DISASSOCIATION);
#endif
#if QCA_SUPPORT_SON
                qdf_mem_copy(event_data.disconnect_event_data.client_addr, ni->ni_macaddr, QDF_MAC_ADDR_SIZE);
                event_data.disconnect_event_data.source = BSTEER_SOURCE_REMOTE;
                event_data.disconnect_event_data.type = BSTEER_DEAUTH;
                event_data.disconnect_event_data.reason = reason_code;
                if (rx_ops && rx_ops->son_rx_ops.deliver_event)
                    rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, ni->peer_obj, MLME_EVENT_CLIENT_DISCONNECTED, &event_data);
#endif
                /* Flush frames from backlog queue */
                ieee80211_flush_peer_mgmt_queue(ni);
            }
            ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);
        }
        break;
        default:
                /* Call MLME indication handler */
                {
                        u_int8_t *macaddr;
                        if (vap->iv_opmode == IEEE80211_M_STA)
                                macaddr = ni->ni_bssid;
                        else
                                macaddr = ni->ni_macaddr;
#ifdef WLAN_FEATURE_11BE_MLO
                        IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap, macaddr, 0, reason_code, &ni->peer_obj->mldaddr[0]);
#else
                        IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap, macaddr, 0, reason_code, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
                }
    }
}
void ieee80211_mlme_recv_csa(struct ieee80211_node *ni, u_int32_t csa_delay, bool disconnect)
{
    struct ieee80211vap    *vap = ni->ni_vap;
    u_int16_t associd = 0;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    switch(vap->iv_opmode) {
    case IEEE80211_M_STA:
         IEEE80211_NODE_STATE_LOCK(ni);
        if (ni->ni_table) {
            IEEE80211_DELIVER_EVENT_MLME_RADAR_DETECTED(vap, csa_delay);
            associd = ni->ni_associd;

            /* Call MLME indication handler */
#ifdef WLAN_FEATURE_11BE_MLO
            IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap, ni->ni_macaddr, associd, IEEE80211_STATUS_UNSPECIFIED, &ni->peer_obj->mldaddr[0]);
#else
            IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap, ni->ni_macaddr, associd, IEEE80211_STATUS_UNSPECIFIED, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
        }
         IEEE80211_NODE_STATE_UNLOCK(ni);
        break;
    default:
        break;
    }
}


/*
 * heart beat timer to handle any timeouts.
 * called every IEEE80211_INACT_WAIT seconds.
 */
void mlme_inact_timeout(struct ieee80211vap *vap)
{
    /*
     * send keep alive packets (null frames) for station vap so that
     * the AP does not kick us out because of inactivity.
     */
    switch(vap->iv_opmode) {
    case IEEE80211_M_STA:
        ieee80211_inact_timeout_sta(vap);
        break;
    case IEEE80211_M_HOSTAP:
        ieee80211_inact_timeout_ap(vap);
        break;
    default:
        break;

    }
}

/*
 * Data structure and routine used for verifying whether any port has an
 * active connection involving the specified BSS entry.
 */
struct ieee80211_mlme_find_connection {
    struct scan_cache_entry    *scan_entry;
    bool                           connection_found;
};

/*
 * Local functions
 */

static OS_TIMER_FUNC(timeout_callback)
{
    struct ieee80211vap    *vap;
    OS_GET_TIMER_ARG(vap, struct ieee80211vap *);

    mlme_timeout_callback(vap, IEEE80211_STATUS_TIMEOUT);
}

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
static OS_TIMER_FUNC(stacac_timeout_callback)
{
}
#endif

#if DBDC_REPEATER_SUPPORT
static OS_TIMER_FUNC(drop_mcast_timeout_callback)
{
    struct ieee80211vap    *vap;
    struct ieee80211com    *ic;
    struct ieee80211_mlme_priv    *mlme_priv;
    osif_dev *osdev;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    struct ieee80211com    *tmp_ic;
    int i;
#endif
    u_int8_t *macaddr;

    OS_GET_TIMER_ARG(vap, struct ieee80211vap *);
    mlme_priv = vap->iv_mlme_priv;
    ic = vap->iv_ic;
    osdev = (osif_dev *)vap->iv_ifp;
    GLOBAL_IC_LOCK_BH(ic->ic_global_list);
    macaddr = wlan_vdev_mlme_get_mldaddr(vap->vdev_obj);
    if (!wlan_vdev_mlme_is_mlo_vdev(vap->vdev_obj) || qdf_is_macaddr_zero((struct qdf_mac_addr *)macaddr))
        macaddr = vap->iv_myaddr;

    if (ic->ic_global_list->num_l2uf_retries < MAX_L2UF_RETIRES) {
        osif_hardstart_l2uf(osdev, macaddr);
        ic->ic_global_list->num_l2uf_retries++;
        GLOBAL_IC_UNLOCK_BH(ic->ic_global_list);
        qdf_timer_mod(&mlme_priv->im_drop_mcast_timer,((ic->ic_global_list->delay_stavap_connection*1000/MAX_L2UF_RETIRES)));
        return;
    }

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (ic->nss_vops) {
        if (!ic->nss_vops->ic_osif_nss_vdev_wifi_mac_db_ready(osdev) &&
            ic->ic_global_list->dbdc_process_enable) {
            GLOBAL_IC_UNLOCK_BH(ic->ic_global_list);
            qdf_timer_mod(&mlme_priv->im_drop_mcast_timer,((ic->ic_global_list->delay_stavap_connection*100)));
            return;
        }
    }
#endif

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

    qdf_info("drop mcast expired on sta vap: %s",ether_sprintf(macaddr));
}
#endif

void wlan_mlme_external_auth_timer_fn(void *arg)
{
    struct ieee80211vap    *vap = arg;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;

    qdf_timer_stop(&vap->iv_sta_external_auth_timer);

    if (mlme_priv->im_request_type == MLME_REQ_AUTH) {
        mlme_priv->im_request_type = MLME_REQ_NONE;
        IEEE80211_DELIVER_EVENT_MLME_AUTH_COMPLETE(vap, NULL, IEEE80211_STATUS_TIMEOUT);
    }
    qdf_info("external auth timer expire");
}

#ifdef WLAN_FEATURE_11BE_MLO
void wlan_mlme_ml_presp_wait_timer_fn(void *arg)
{
    struct ieee80211vap *vap = arg;
    struct ml_prbreq_connect_info *info = &vap->iv_ml_prbreq_info;
    struct cfg80211_scan_info scan_info = {
        .aborted = true
    };

    mlo_debug("ML Probe: Probe response timer expired after %ums",
              vap->iv_ic->ic_ml_prbresp_update_time);

    /* If timer is still running, ML probe response is not received yet.
     * Update scan_done to cfg80211 as `aborted`.
     *
     * Since scan request is converted to a probe request tx, cfg80211 scan
     * should be cleaned up on failure.
     */
    qdf_timer_stop(&info->prbresp_update_time);

    qdf_spin_lock(&info->status_lock);
    info->send_req = 0;
    qdf_spin_unlock(&info->status_lock);

    cfg80211_scan_done(info->cfg_scan_req, &scan_info);

    return;
}
#endif /* WLAN_FEATURE_11BE_MLO */

static void mlme_timeout_callback(struct ieee80211vap *vap, IEEE80211_STATUS  ieeeStatus)
{
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    int                           mlme_request_type = mlme_priv->im_request_type;
#ifdef OL_ATH_SMART_LOGGING
    struct ol_ath_softc_net80211 *scn = NULL;
    struct ol_if_offload_ops *ol_if_ops = NULL;

    if (vap->iv_ic != NULL) {
        scn = OL_ATH_SOFTC_NET80211(vap->iv_ic);
    }

    /*
     * We stop at extraction of ol_if_ops and do not go beyond this, since the
     * individual function pointers to be accessed under ol_if_ops may vary as
     * more functionality is added.
     */
    if (scn && scn->soc) {
        ol_if_ops = scn->soc->ol_if_ops;
    }
#endif /* OL_ATH_SMART_LOGGING */

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s. Request type = %d status %d\n",
                       __func__, mlme_request_type, ieeeStatus);

    /* Request complete */
    mlme_priv->im_request_type = MLME_REQ_NONE;

    switch(mlme_request_type) {
    case MLME_REQ_JOIN_INFRA:
        /*
         * Cancel the Join operation if it has not already completed
         */
        if(MLME_STOP_WAITING_FOR_JOIN(mlme_priv) == TRUE){
            if(mlme_priv->im_join_is_timeout == 0){
                /*Send a bcast probe.3rd party hotspot sometime respond only to Brocast
                  probe request instead of unicast in join time. If no response received
                  to unicast probe request, try broadcast to solicit probe response */
                struct ieee80211_node         *ni = vap->iv_bss;
                u_int8_t    bcast_addr[QDF_MAC_ADDR_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                mlme_priv->im_request_type = MLME_REQ_JOIN_INFRA;
                ieee80211_send_probereq(ni, vap->iv_myaddr, bcast_addr,
                        bcast_addr, ni->ni_essid, 0,
                        vap->iv_opt_ie.ie, vap->iv_opt_ie.length);
                mlme_priv->im_join_is_timeout = 1;
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, " Bcast probe timeout %d ms\n", mlme_priv->im_timeout);
                OS_SET_TIMER(&mlme_priv->im_timeout_timer, mlme_priv->im_timeout);
                /* Set the appropriate filtering function and wait for Join Beacon */
                MLME_WAIT_FOR_BSS_JOIN(mlme_priv);

            }else{
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,"%s", "Cancelled the Join Operation as it took too long\n");

                IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_INFRA(vap, ieeeStatus);
                mlme_priv->im_join_is_timeout = 0;
            }
        }
        break;
    case MLME_REQ_AUTH:
        /*
         * Cancel the auth operation if it has not already completed
         */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,"%s", "Cancelled the Auth Operation as it took too long\n");
        mlme_priv->im_expected_auth_seq_number = 0;
        IEEE80211_DELIVER_EVENT_MLME_AUTH_COMPLETE(vap, NULL, ieeeStatus);

#ifdef OL_ATH_SMART_LOGGING
        if (ol_if_ops && ol_if_ops->smart_log_connection_fail_start) {
            /*
             * Currently, we will not alter the rest of the state machine flow
             * based on whether we succeeded or failed in starting connection
             * failure logging. Hence we do not check for failure. We rely on
             * the API to print failure messages if any. Engineering action
             * would have to be taken separately to analyze logging API
             * failures.
             * We also rely on the API to determine whether the logging is
             * required.
             */
            ol_if_ops->smart_log_connection_fail_start(scn);
        }
#endif /* OL_ATH_SMART_LOGGING */

        break;
    case MLME_REQ_ASSOC:
        /*
         * Cancel the assoc operation if it has not already completed
         */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,"%s", "Cancelled the Assoc Operation as it took too long\n");

        IEEE80211_DELIVER_EVENT_MLME_ASSOC_COMPLETE(vap, ieeeStatus, 0, NULL);

#ifdef OL_ATH_SMART_LOGGING
        if (ol_if_ops && ol_if_ops->smart_log_connection_fail_start) {
            /*
             * Currently, we will not alter the rest of the state machine flow
             * based on whether we succeeded or failed in starting connection
             * failure logging. Hence we do not check for failure. We rely on
             * the API to print failure messages if any. Engineering action
             * would have to be taken separately to analyze logging API
             * failures.
             * We also rely on the API to determine whether the logging is
             * required.
             */
            ol_if_ops->smart_log_connection_fail_start(scn);
        }
#endif /* OL_ATH_SMART_LOGGING */

        break;
    case MLME_REQ_REASSOC:
        /*
         * Cancel the reassoc operation if it has not already completed
         */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,"%s", "Cancelled the Reassoc Operation as it took too long\n");

        IEEE80211_DELIVER_EVENT_MLME_REASSOC_COMPLETE(vap, ieeeStatus, 0, NULL);

#ifdef OL_ATH_SMART_LOGGING
        if (ol_if_ops && ol_if_ops->smart_log_connection_fail_start) {
            /*
             * Currently, we will not alter the rest of the state machine flow
             * based on whether we succeeded or failed in starting connection
             * failure logging. Hence we do not check for failure. We rely on
             * the API to print failure messages if any. Engineering action
             * would have to be taken separately to analyze logging API
             * failures.
             * We also rely on the API to determine whether the logging is
             * required.
             */
            ol_if_ops->smart_log_connection_fail_start(scn);
        }
#endif /* OL_ATH_SMART_LOGGING */

        break;
    case MLME_REQ_TXCSA:
        /*
         * Cancel the TXCSA operation if it has not already completed
         */
        if(ieeeStatus == IEEE80211_STATUS_TIMEOUT) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,"%s", "Canceller the Tx CSA Operation as it took too long\n");
            IEEE80211_DELIVER_EVENT_MLME_TXCHANSWITCH_COMPLETE(vap,1 /*FAILED*/);
        }
        break;
    case MLME_REQ_REPEATER_CAC:
        /*
         * Cancel the REPEATER_CAC operation if it has not already completed
         */
        if(ieeeStatus == IEEE80211_STATUS_TIMEOUT) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,"%s", "Canceller the REPEATER  CAC Operation as it took too long\n");
            IEEE80211_DELIVER_EVENT_MLME_REPEATER_CAC_COMPLETE(vap,1 /*FAILED*/);
        }
        break;
    case MLME_REQ_NONE:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,"%s", "mlme_request_type is MLME_REQ_NONE, do nothing.\n");
        break;
    default:
        ASSERT(0);
        break;
    }
}

u_int32_t mlme_dot11rate_to_bps(u_int8_t  rate)
{
    return 500000 * rate;
}

void
ieee80211_mlme_node_pwrsave(struct ieee80211_node *ni, int enable)
{
    struct ieee80211com     *ic = ni->ni_vap->iv_ic;

    if(ni->ni_vap->iv_opmode != IEEE80211_M_HOSTAP)  {
        return;
    }
    if (ic->ic_node_psupdate) {
        ic->ic_node_psupdate(ni, enable, 0);
    }
}

void
ieee80211_mlme_node_leave(struct ieee80211_node *ni)
{
    if(ni->ni_vap->iv_opmode != IEEE80211_M_HOSTAP) return;
    ieee80211_mlme_node_leave_ap(ni);
}

u_int32_t ieee80211_mlme_sta_swbmiss_timer_alloc_id(struct ieee80211vap *vap, int8_t *requestor_name)
{
    return mlme_sta_swbmiss_timer_alloc_id(vap,requestor_name);
}

int ieee80211_mlme_sta_swbmiss_timer_disable(struct ieee80211vap *vap, u_int32_t id)
{
    return mlme_sta_swbmiss_timer_disable(vap,id);
}

void ieee80211_mlme_sta_bmiss_ind(struct ieee80211vap *vap)
{
    mlme_sta_bmiss_ind(vap);
}

void ieee80211_mlme_reset_bmiss(struct ieee80211vap *vap)
{
    mlme_sta_reset_bmiss(vap);
}

/**
 * register a mlme  event handler.
 * @param vap        : handle to vap object
 * @param evhandler  : event handler function.
 * @param arg        : argument passed back via the event handler
 * @return EOK if success, EINVAL if failed, ENOMEM if runs out of memory.
 * allows more than one event handler to be registered.
 */
int ieee80211_mlme_register_event_handler(ieee80211_vap_t vap,ieee80211_mlme_event_handler evhandler, void *arg)
{
    int i;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;

    /* unregister if there exists one already */
    ieee80211_mlme_unregister_event_handler(vap,evhandler,arg);

    IEEE80211_VAP_LOCK(vap);
    for (i=0;i<IEEE80211_MAX_MLME_EVENT_HANDLERS; ++i) {
        if ( mlme_priv->im_event_handler[i] == NULL ) {
             mlme_priv->im_event_handler[i] = evhandler;
             mlme_priv->im_event_handler_arg[i] = arg;
             IEEE80211_VAP_UNLOCK(vap);
             return EOK;
        }
    }
    IEEE80211_VAP_UNLOCK(vap);
    return ENOMEM;
}

/**
 * unregister a mlme  event handler.
 * @param vap        : handle to vap object
 * @param evhandler  : event handler function.
 * @param arg        : argument passed back via the evnt handler
 * @return EOK if success, EINVAL if failed.
 */
int ieee80211_mlme_unregister_event_handler(ieee80211_vap_t vap,ieee80211_mlme_event_handler evhandler, void *arg)
{
    int i;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;

    IEEE80211_VAP_LOCK(vap);
    for (i=0;i<IEEE80211_MAX_MLME_EVENT_HANDLERS; ++i) {
        if ( mlme_priv->im_event_handler[i] == evhandler &&  mlme_priv->im_event_handler_arg[i] == arg ) {
             mlme_priv->im_event_handler[i] = NULL;
             mlme_priv->im_event_handler_arg[i] = NULL;
             IEEE80211_VAP_UNLOCK(vap);
             return EOK;
        }
    }
    IEEE80211_VAP_UNLOCK(vap);
    return EINVAL;
}

void ieee80211_mlme_deliver_event(struct ieee80211_mlme_priv *mlme_priv, ieee80211_mlme_event *event)
{
    int i;
    void *arg;
    ieee80211_mlme_event_handler evhandler;
    struct ieee80211vap    *vap = mlme_priv->im_vap;

    IEEE80211_VAP_LOCK(vap);
    for(i=0;i<IEEE80211_MAX_MLME_EVENT_HANDLERS; ++i) {
        if (mlme_priv->im_event_handler[i]) {
            evhandler =  mlme_priv->im_event_handler[i];
            arg = mlme_priv->im_event_handler_arg[i];
            IEEE80211_VAP_UNLOCK(vap);
            (* evhandler) (vap, event,arg);
            IEEE80211_VAP_LOCK(vap);
        }
    }
    IEEE80211_VAP_UNLOCK(vap);
}

/*
 * Calculate maximum allowed scan_entry age, in ms.
 * Reference_time specifies the timestamp of the oldest accepted entry.
 */
u_int32_t ieee80211_mlme_maximum_scan_entry_age(wlan_if_t vaphandle,
                                                systime_t reference_time)
{
#define IEEE80211_SCAN_LATENCY_TIME                 1000
    u_int32_t    maximum_age  = 0;
    systime_t    current_time = OS_GET_TIMESTAMP();

    if (reference_time == 0) {
        /* Make all entries old if there's no record of the last scan */
        maximum_age = 0;
    }
    else {
        maximum_age = CONVERT_SYSTEM_TIME_TO_MS(current_time - reference_time);

        /*
         * Make all entries in the table "old" by setting the maximum age
         * to 0 if last scan occurred too long ago. This can happen when
         * system is resuming from S3/S4.
         */
        if (maximum_age > IEEE80211_SCAN_ENTRY_EXPIRE_TIME) {
            maximum_age = IEEE80211_SCAN_ENTRY_EXPIRE_TIME;
        }
    }

    /*
     * Add a latency time to account for the delay from the time the
     * maximum age is calculated to the time it's actually used.
     * Failing to account for this latency time can cause the oldest
     * entries in the scan list to be skipped.
     */
    if (maximum_age > 0) {
        maximum_age += IEEE80211_SCAN_LATENCY_TIME;
    }

    return maximum_age;
#undef IEEE80211_SCAN_LATENCY_TIME
}

int wlan_mlme_num_apvap_running(wlan_if_t vaphandle)
{
    struct ieee80211com      *ic = vaphandle->iv_ic;
    return ieee80211_num_apvap_running(ic);
}
bool wlan_mlme_is_vap_main_sta(wlan_if_t vaphandle)
{
    struct ieee80211com      *ic = vaphandle->iv_ic;
    return (ic->ic_sta_vap == vaphandle);
}

bool wlan_mlme_is_txcsa_set(wlan_if_t vaphandle)
{
    struct ieee80211com      *ic = vaphandle->iv_ic;
    return  IEEE80211_IS_CSH_CSA_APUP_BYSTA_ENABLED(ic);
}
bool wlan_mlme_is_repeater_cac_set(wlan_if_t vaphandle)
{
    struct ieee80211com      *ic = vaphandle->iv_ic;
    return  IEEE80211_IS_CSH_CAC_APUP_BYSTA_ENABLED(ic);
}
bool wlan_mlme_is_scanentry_dfs(struct ieee80211_ath_channel *scan_entry_chan)
{
    return (IEEE80211_IS_CHAN_DFS(scan_entry_chan) ||
             ((IEEE80211_IS_CHAN_11AC_VHT160(scan_entry_chan) || IEEE80211_IS_CHAN_11AC_VHT80_80(scan_entry_chan))
             && IEEE80211_IS_CHAN_DFS_CFREQ2(scan_entry_chan)));
}
bool wlan_mlme_is_chosen_diff_from_curchan(wlan_if_t vaphandle,struct ieee80211_ath_channel *scan_entry_chan)
{
    /* check if the chosen channel is the same as the curchan */
    return (!IEEE80211_ARE_CHANS_EQUAL(vaphandle->iv_ic->ic_curchan, scan_entry_chan));
}

bool wlan_mlme_is_chan_radar(struct ieee80211com *ic, struct scan_cache_entry *scan_entry)
{
    struct ieee80211_ath_channel scan_chan;

    scan_chan = wlan_util_scan_entry_channel(ic, scan_entry, false);
    return IEEE80211_IS_CHAN_RADAR(ic, &scan_chan);
}

int wlan_mlme_set_ftie(wlan_if_t vaphandle, u_int16_t md, u_int8_t *buf, u_int16_t buflen)
{
    struct ieee80211vap    *vap = vaphandle;
    u_int8_t               *ftbuf = NULL;
    u_int16_t              ftbuf_len = 0;
#define MAX_FT_IES_LEN (MAX_RSN_IE_LEN + MD_IE_LEN + MAX_FT_IE_LEN)
#define MIN_FT_IES_LEN (MIN_RSN_IE_LEN + MD_IE_LEN + MIN_FT_IE_LEN)

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    if (!buf || !buflen) {
        return -EINVAL;
    }

    /* Supplicant provides RSNE, MDE, FTE in that order.
     */

    if (buflen > MAX_FT_IES_LEN || buflen < MIN_FT_IES_LEN) {
        return -EINVAL;
    }

    /* Allocate ft buf */
    ftbuf_len = buflen;
    ftbuf = qdf_mem_malloc(ftbuf_len);
    if (!ftbuf) {
        return -ENOMEM;
    }

    OS_MEMZERO(ftbuf, ftbuf_len);

    /* copy ft ie contents */
    qdf_mem_copy(ftbuf, buf, ftbuf_len);
    wlan_mlme_app_ie_set_check(vap, IEEE80211_FRAME_TYPE_ASSOCREQ, (u_int8_t *)ftbuf, ftbuf_len, HOSTAPD_IE);
    wlan_mlme_app_ie_set_check(vap, IEEE80211_FRAME_TYPE_AUTH, (u_int8_t *)ftbuf, ftbuf_len, HOSTAPD_IE);
    qdf_mem_free(ftbuf);
#undef MAX_FT_IES_LEN
#undef MIN_FT_IES_LEN
    return 0;
}

#ifdef WLAN_FEATURE_11BE_MLO
struct mlo_mlme_ext_ops mlo_mlme_glbl_ops = {
    .mlo_mlme_ext_validate_conn_req = osif_vap_validate_link_sta,
    .mlo_mlme_get_link_assoc_req = ieee80211_mlo_get_link_assoc,
    .mlo_mlme_ext_create_link_vdev = NULL,
    .mlo_mlme_ext_peer_create = wlan_mlme_peer_create,
    .mlo_mlme_ext_bridge_peer_create = wlan_mlme_bridge_peer_create,
    .mlo_mlme_ext_peer_assoc = wlan_mlme_peer_assoc,
    .mlo_mlme_ext_peer_assoc_fail = wlan_mlme_peer_assoc_fail,
    .mlo_mlme_ext_peer_delete = wlan_mlme_peer_delete,
    .mlo_mlme_ext_assoc_resp = wlan_mlme_peer_assoc_resp,
    .mlo_mlme_ext_deauth = wlan_mlme_peer_deauth,
    .mlo_mlme_ext_clone_security_param = osif_mlo_sta_clone_security_param,
    .mlo_mlme_ext_sta_op_class = osif_update_op_class_from_mlie,
#ifdef UMAC_MLO_AUTH_DEFER
    .mlo_mlme_ext_peer_process_auth = wlan_mlme_peer_process_auth,
#endif
    .mlo_mlme_ext_peer_reassoc = wlan_mlme_peer_reassoc,
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    .mlo_mlme_ext_connect_get_partner_info = osif_mlo_sta_connect_get_partner_info,
#endif
};

struct mlo_mlme_ext_ops *mlme_mlo_get_mlme_ops(void)
{
    return &mlo_mlme_glbl_ops;
}

int wlan_mlme_mlo_link_rej_init(struct wlan_objmgr_vdev *vdev,
                                enum mlo_link_rej_cfg value)
{
    int splitmac_enabled = 0;

#if WLAN_SUPPORT_SPLITMAC
    splitmac_enabled = splitmac_is_enabled(vdev);
#else
    splitmac_enabled = 0;
#endif

    if (!vdev->mlo_dev_ctx || !vdev->mlo_dev_ctx->ap_ctx)
        return -1;

    if (splitmac_enabled) {
        vdev->mlo_dev_ctx->ap_ctx->mlo_link_reject = 0;
        qdf_err("Splitmac enabled, link reject disabled");
    } else {
        vdev->mlo_dev_ctx->ap_ctx->mlo_link_reject = value;
    }

    return vdev->mlo_dev_ctx->ap_ctx->mlo_link_reject;
}
#endif
