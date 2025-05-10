/*
* Copyright (c) 2018 Qualcomm Innovation Center, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Innovation Center, Inc.
*
*/

/*
 *  Copyright (c) 2011 Atheros Communications Inc.
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

#include <ieee80211_var.h>
#include <ieee80211_objmgr_priv.h>
#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_ap.h>
#endif
#include <wlan_mbss.h>

#if UMAC_SUPPORT_QUIET

/*
 * Add quiet information element to a frame.
 */
u_int8_t *
ieee80211_add_quiet_ie(u_int8_t *frm, struct ieee80211vap *vap)
{
    struct ieee80211_ath_quiet_ie *quiet = (struct ieee80211_ath_quiet_ie *) frm;
    struct ieee80211_quiet_param *vap_quiet = vap->iv_quiet;

    if(!quiet)
        return frm;

    memset(quiet, 0, sizeof(struct ieee80211_ath_quiet_ie));
    quiet->ie = IEEE80211_ELEMID_QUIET;
    quiet->len = sizeof(struct ieee80211_ath_quiet_ie) - 2;
    quiet->tbttcount = vap_quiet->tbttcount;
    quiet->period = vap_quiet->period;
    quiet->duration = htole16(vap_quiet->duration);
    quiet->offset = htole16(vap_quiet->offset);
    return frm + sizeof(struct ieee80211_ath_quiet_ie);
}

/*
 * Update vap quiet parameters
 */
void
ieee80211_quiet_param_update(struct ieee80211vap *vap, struct ieee80211com *ic)
{
    struct ieee80211_quiet_param *iv_quiet = vap->iv_quiet;
    struct ieee80211_quiet_param *ic_quiet = ic->ic_quiet;
    u_int8_t tbttcount = ic_quiet->tbttcount;
    u_int16_t offset = ic_quiet->beacon_offset + ic_quiet->offset;

    if (offset < iv_quiet->beacon_offset) {
        tbttcount = (tbttcount -1) ? (tbttcount -1) : ic_quiet->period;
        offset += ic->ic_intval - iv_quiet->beacon_offset;
    }
    else {
        if (iv_quiet->beacon_offset < ic_quiet->beacon_offset) {
            tbttcount = (tbttcount -1) ? (tbttcount -1) : ic_quiet->period;
        }
        offset -= iv_quiet->beacon_offset;
    }
    iv_quiet->tbttcount = tbttcount;
    iv_quiet->period = ic_quiet->period;
    iv_quiet->duration = ic_quiet->duration;
    iv_quiet->offset = offset;
}

/* Count vaps with Quiet-enabled. Disable it on each vap
 * while iterating in case of MBSSID
 */
static void ieee80211_iter_vap_quiet(void *arg, struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    u_int16_t *count = (u_int16_t *)arg;
    bool is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                WLAN_PDEV_F_MBSS_IE_ENABLE);
    ASSERT(vap->iv_quiet);
    if (vap->iv_quiet->is_enabled) {
        ++(*count);
        if (is_mbssid_enabled)
            vap->iv_quiet->is_enabled = 0;
    }
}


/*
 * Add the quiet ie and update beacon offset
 */
u_int8_t *
ieee80211_quiet_beacon_setup(struct ieee80211vap *vap, struct ieee80211com *ic,
                        struct ieee80211_beacon_offsets *bo,
                        u_int8_t *frm)
{
    ASSERT(ic->ic_quiet);
    ASSERT(vap->iv_quiet);
    if(!vap->iv_bcn_offload_enable) {
        if (ic->ic_quiet->is_enabled &&
                vap->iv_quiet->is_enabled) {
            bo->bo_quiet = frm;
            ieee80211_quiet_param_update(vap, ic);
            frm = ieee80211_add_quiet_ie(frm, vap);
        }
    }
    return frm;
}

/*
 * update the quiet ie
 */
void
ieee80211_quiet_beacon_update(struct ieee80211vap *vap, struct ieee80211com *ic,
                               struct ieee80211_beacon_offsets *bo)
{
    ASSERT(ic->ic_quiet);
    ASSERT(vap->iv_quiet);
    if(!(vap->iv_bcn_offload_enable)) {
        if (ic->ic_quiet->is_enabled &&
                vap->iv_quiet->is_enabled &&
                bo->bo_quiet) {
            ieee80211_quiet_param_update(vap, ic);
            ieee80211_add_quiet_ie(bo->bo_quiet, vap);
        }
    }
}

/*
 * Add quiet ie
 */
u_int8_t *
ieee80211_add_quiet(struct ieee80211vap *vap, struct ieee80211com *ic, u_int8_t *frm)
{
    ASSERT(ic->ic_quiet);
    ASSERT(vap->iv_quiet);
    if (ic->ic_quiet->is_enabled &&
        vap->iv_quiet->is_enabled) {
        frm = ieee80211_add_quiet_ie(frm, vap);
    }
    return frm;
}

void wlan_init_quiet_params(struct ieee80211com *ic)
{
    struct ieee80211_quiet_param *ic_quiet = ic->ic_quiet;

    OS_MEMZERO(ic_quiet, sizeof(struct ieee80211_quiet_param));
    ic_quiet->period = WLAN_QUIET_PERIOD_DEF;
    ic_quiet->tbttcount = WLAN_QUIET_PERIOD_DEF;
    ic_quiet->duration = WLAN_QUIET_DURATION_DEF;
    ic_quiet->offset = WLAN_QUIET_OFFSET_DEF;
}

int ieee80211_quiet_attach(struct ieee80211com *ic)
{
    struct ieee80211_quiet_param *ic_quiet;
    if (ic->ic_quiet) {
        ASSERT(ic->ic_quiet == 0);
        return -1;
    }
    ic_quiet = (struct ieee80211_quiet_param *) OS_MALLOC(ic->ic_osdev,
                       (sizeof(struct ieee80211_quiet_param)),0);
    if (ic_quiet == NULL) {
       return -ENOMEM;
    }
    ic->ic_quiet = ic_quiet;
    wlan_init_quiet_params(ic);

    return 0;
}

int ieee80211_quiet_detach(struct ieee80211com *ic)
{
    if (ic->ic_quiet == NULL) {
        ASSERT(ic->ic_quiet);
        return -1; /* already detached */
    }
    OS_FREE(ic->ic_quiet);
    ic->ic_quiet = NULL;
    return 0;
}


int ieee80211_quiet_vattach(struct ieee80211vap *vap)
{
    struct ieee80211com     *ic = vap->iv_ic;
    struct ieee80211_quiet_param *iv_quiet;
    if (vap->iv_quiet) {
        ASSERT(vap->iv_quiet == 0);
        return -1; /* already attached ? */
    }
    iv_quiet = (struct ieee80211_quiet_param *) OS_MALLOC(ic->ic_osdev,
                       (sizeof(struct ieee80211_quiet_param)),0);
    if (iv_quiet == NULL) {
       return -ENOMEM;
    }
    vap->iv_quiet = iv_quiet;
    OS_MEMZERO(iv_quiet, sizeof(struct ieee80211_quiet_param));
    return 0;
}

int ieee80211_quiet_vdetach(struct ieee80211vap *vap)
{
    if (vap->iv_quiet == NULL) {
        ASSERT(vap->iv_quiet);
        return -1; /* already detached */
    }
    OS_FREE(vap->iv_quiet);
    vap->iv_quiet = NULL;
    return 0;
}

#ifdef WLAN_FEATURE_11BE_MLO
void wlan_mlo_ap_vdev_quiet_set(struct wlan_objmgr_vdev *vdev)
{
    mlo_ap_vdev_quiet_set(vdev);
}

void wlan_mlo_ap_vdev_quiet_clear(struct wlan_objmgr_vdev *vdev)
{
    mlo_ap_vdev_quiet_clear(vdev);
}

#endif /* WLAN_FEATURE_11BE_MLO */

int wlan_quiet_set_param(struct ieee80211vap *vap, u_int32_t val) {
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_quiet_param *iv_quiet = vap->iv_quiet;
    struct ieee80211_quiet_param *ic_quiet = ic->ic_quiet;
    uint8_t num_ap, num_vdev;
    bool is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                WLAN_PDEV_F_MBSS_IE_ENABLE);
    u_int16_t count = 0;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;

    ASSERT(iv_quiet);
    ASSERT(ic_quiet);

    if (is_mbssid_enabled) {
        if (!ic->ic_mbss_ctx) {
            mbss_err("mbssid context is null in mbssid mode");
            return -EINVAL;
        }
        mbss_ctx = ic->ic_mbss_ctx;
        ic_mbss  = &mbss_ctx->ic_mbss[vap->iv_mbss.group_id];
    }

    /* Do not allow Quiet when CSA is inprogress */
    if (ic->ic_flags & IEEE80211_F_CHANSWITCH) {
        qdf_err("CSA is inprogress, try later");
        return -EINVAL;
    }

    /* Initialize the Quiet params to default values if Quiet tbttcount is 0,
     * else Quiet announcement is inprogress so do not update the Quiet params.
     */
    if (!iv_quiet->tbttcount) {
        wlan_init_quiet_params(ic);
        iv_quiet->tbttcount = ic_quiet->tbttcount;
    }

    if (val) {
        /* if the curret VAP is HOSTAP mode and all the
         * other VAPs also in HOSTAP then enable this feature
         */
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            num_ap = wlan_mbss_num_ap(ic->ic_pdev_obj);
            num_vdev = wlan_mbss_num_vdev(ic->ic_pdev_obj);
            if (num_vdev == num_ap) {
                /* Set WLAN_QUIET_FLAG_INFO_IN_SWBA_START_STOP flag to get
                 * Quiet count from FW as part of SWBA event.
                 */
                val |= WLAN_QUIET_FLAG_INFO_IN_SWBA_START_STOP;
                iv_quiet->is_enabled = val;
                ic_quiet->is_enabled = 1;
                if (vap->iv_bcn_offload_enable) {
                    /* In case of MBSSID, multiplex quiet-ie
                     * enablement to Tx-vap
                     */
                    if (is_mbssid_enabled) {
                        if (!ic_mbss->transmit_vap) {
                            qdf_err("Tx-vap is NULL - can't enable quiet_ie");
                            return -EINVAL;
                        }
                        /* get tx_vap reference */
                        vap = ic_mbss->transmit_vap;
                        vap->iv_quiet->is_enabled = val;
                    }
                    vap->iv_ic->ic_set_bcn_offload_quiet_mode(vap, vap->iv_ic);
                }
            }
            else {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,
                                  "Not all vaps are in HOSTAP mode total vaps %d ap vaps %d \n",
                                  num_vdev,
                                  num_ap);
                return -EINVAL;
            }
        }
        else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,
                                  "vap mode is not HOSTAP opmode %x \n",
                                  vap->iv_opmode);
            return -EINVAL;
        }
    }
    else {
        iv_quiet->is_enabled = 0;
        if (vap->iv_bcn_offload_enable) {
            if (is_mbssid_enabled) {
                if (!ic_mbss->transmit_vap) {
                    qdf_err("Tx-vap is NULL - operation failed");
                    return -EINVAL;
                }
                vap = ic_mbss->transmit_vap;
                vap->iv_quiet->is_enabled = 0;
            }
            ic->ic_set_bcn_offload_quiet_mode(vap, ic);
        }
        wlan_iterate_vap_list(ic, ieee80211_iter_vap_quiet,
            (void *)&count);
        if (is_mbssid_enabled || (count == 0)) {
            /* quiet IE is not enabled for all VAPs
             * disable quiet element support in the ic
             */
            ic_quiet->is_enabled = 0;
        }
    }
    return 0;
}

u_int32_t wlan_quiet_get_param(struct ieee80211vap *vap) {
    ASSERT(vap->iv_quiet);
    return vap->iv_quiet->is_enabled;
}

#endif /* UMAC_SUPPORT_QUIET */
