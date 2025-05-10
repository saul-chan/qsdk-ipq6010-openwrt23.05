/*
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 */

/**
 * DOC: This file contains implementations of routines for 802.11be Multi-Link
 * Operation (MLO) protocol processing.
 */
#include <ieee80211_proto_mlo.h>
#include <ol_if_athvar.h>

#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_utility.h>
#include <utils_mlo.h>
#include <wlan_mlo_mgr_public_structs.h>
#include <wlan_mlo_mgr_ap.h>
#include <wlan_mlo_mgr_cmn.h>
#include <wlan_mlo_mgr_peer.h>
#include <ieee80211_objmgr_priv.h>
#include <wlan_mlme_dispatcher.h>
#include <ieee80211_mlme_dfs_dispatcher.h>
#include <wlan_vdev_mgr_utils_api.h>
#include <ieee80211_quiet_priv.h>
#include <ieee80211_channel.h>
#include "cfg_ucfg_api.h"

/**
 * ieee80211_multilink_ie_skipsection() - Helper function to skip given protocol
 * section(s) inwithin a containing protocol section in a Multi-Link IE, and
 * update buffer and length for the containing protocol section. This is a
 * simple helper which is only intended to help with a basic check of the
 * remaining space in the containing section, and the skip itself. It does not
 * handle complexities such as fragmentation, etc., and the caller/caller chain
 * should take care of these complexities.
 *
 * Since this is a static helper function and we would like to avoid redundant
 * checks, it is the caller's/caller chain's responsibility to validate that all
 * pointer arguments are non-NULL (and that in the case of double pointers, all
 * levels of pointers are non-NULL).
 *
 * @param ctningsectionbuf: Pointer to buffer for containing protocol section,
 * which when passed to the function points to buffer starting from the protocol
 * section(s) to be skipped, and which after successful execution of function is
 * incremented by the number of bytes of protocol section(s) skipped
 * @param remctningsectionbuflen: Pointer to length of containing protocol
 * section, which when passed to the function points to value of length starting
 * from the protocol section(s) to be skipped, and which after successful
 * execution of function is decremented by the number of bytes of protocol
 * section(s) skipped
 * @param ctningsectionname: Name of containing protocol section
 * @skipsectionlen: Number of bytes of protocol section(s) to be skipped
 * @skipsectionname: Name(s) of protocol section(s) to be skipped
 *
 * @return QDF_STATUS_SUCCESS on success, other QDF_STATUS value in case of
 * error.
 */
static inline QDF_STATUS ieee80211_multilink_ie_skipsection(
        u_int8_t **ctningsectionbuf, size_t *remctningsectionbuflen,
        const char *ctningsectionname, size_t skipsectionlen,
        const char *skipsectionname)
{
    if (*remctningsectionbuflen < skipsectionlen) {
        mloie_err("Insufficient size remaining in %s for %s, required: %zu octets, remaining: %zu octets",
                ctningsectionname, skipsectionname, skipsectionlen,
                *remctningsectionbuflen);
        return QDF_STATUS_E_NOMEM;
    }

    *ctningsectionbuf += skipsectionlen;
    *remctningsectionbuflen -= skipsectionlen;

    return QDF_STATUS_SUCCESS;
}

/**
 * ieee80211_gen_basic_multilink_ie_completestaprof() - Helper function to
 * generate contents of STA profile in Link Info field in Basic variant Multi
 * Link element. This helper does not take care of ML element fragmentation, per
 * STA profile fragmentation/size checks - the caller/caller chain is expected
 * to take care of this. This helper function is intended to be used for
 * (re)association request/response and those ML probe responses which carry
 * complete information.
 *
 * Since this is a static helper function and we would like to avoid redundant
 * checks, it is the caller's/caller chain's responsibility to validate that
 * - All pointer arguments are non-NULL
 * - This helper is called for the right subtype
 *
 * @param reportingstabuf: Pointer to buffer which contains elements for the
 * reporting STA
 * @param reportingstabufsize: Length of the buffer which contains elements for
 * the reporting STA
 * @param reportedstabuf: Pointer to buffer which contains elements for the
 * reported STA
 * @param reportedstabufsize: Length of the buffer which contains elements for
 * the reported STA
 * @param staprofbuf: Buffer into which the STA profile should be populated
 * @param staprofbufsize: Size of buffer into which the STA profile should be
 * populated
 * @param subtype: Frame subtype
 * @param extractx: Extra framing context. This can be NULL.
 * @param vap: Pointer to vap structure
 *
 * @return Positive integer value giving number of octets added to staprofbuf in
 * the case of success, -1 in the case of failure
 */
static int ieee80211_gen_basic_multilink_ie_completestaprof(
        u_int8_t *reportingstabuf, size_t reportingstabufsize,
        u_int8_t *reportedstabuf, size_t reportedstabufsize,
        u_int8_t *staprofbuf, size_t staprofbufsize, u_int8_t subtype,
        struct ieee80211_framing_extractx *extractx,
        struct ieee80211vap *vap)
{
    u_int8_t *curr_rptingbufpos = NULL;
    size_t rptingbufsize_remaining = 0;
    u_int8_t *curr_rptedbufpos = NULL;
    size_t rptedbufsize_remaining = 0;
    bool discard_fragseq_sp;
    u_int8_t *curr_spbufpos = NULL;
    size_t spbufspace_remaining = 0;
    u_int8_t *rptingbuf_iestartpos = NULL;
    size_t rptingbuf_iesectionlen = 0;
    u_int8_t *rptedbuf_iestartpos = NULL;
    size_t rptedbuf_iesectionlen = 0;
    u_int8_t *ninherit_ie = NULL;
    size_t ninherit_ielen = 0;
    u_int8_t ninherit_elemlist[IEEE80211_MAX_IE_LEN - 1] = {0};
    size_t ninherit_elemlist_len = 0;
    u_int8_t ninherit_elemextlist[IEEE80211_MAX_IE_LEN - 1] = {0};
    size_t ninherit_elemextlist_len = 0;
    u_int8_t *curr_ninherit_ie_pos = NULL;
    size_t tot_elemsize = 0;
    struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vap->vdev_obj);
    u_int8_t *staprofbuf_iestartpos = NULL;
    size_t staprofbuf_iesectionlen = 0;
    struct ieee80211_ath_quiet_ie *quiet_ie = NULL;
    struct ieee80211_quiet_param *iv_quiet = vap->iv_quiet;
    uint16_t bcn_intval = ieee80211_node_get_beacon_interval(vap->iv_bss);
    unsigned long cur_time = 0;
    struct ieee80211_max_chan_switch_time_ie *max_cst_ie = NULL;
    uint32_t max_switch_time_in_ms = 0;
    uint32_t max_switch_time_in_tu = 0;
    uint8_t eid = 0;
    int i = 0;

    if (!pdev)
        return -1;

    curr_rptingbufpos = reportingstabuf;
    rptingbufsize_remaining = reportingstabufsize;
    curr_rptedbufpos = reportedstabuf;
    rptedbufsize_remaining = reportedstabufsize;
    curr_spbufpos = staprofbuf;
    spbufspace_remaining = staprofbufsize;

    /* Skip over the IEEE 802.11 header in the reporting and reported STA
     * buffers
     */
    if (ieee80211_multilink_ie_skipsection(&curr_rptingbufpos,
                &rptingbufsize_remaining, "reporting STA buffer",
                sizeof(struct ieee80211_frame), "IEEE 802.11 header") !=
            QDF_STATUS_SUCCESS) {
        return -1;
    }

    if (ieee80211_multilink_ie_skipsection(&curr_rptedbufpos,
                &rptedbufsize_remaining, "reported STA buffer",
                sizeof(struct ieee80211_frame), "IEEE 802.11 header") !=
            QDF_STATUS_SUCCESS) {
        return -1;
    }

    /* Note: In case ML probe response is replaced by a new frame type in the
     * standard, change below accordingly.
     */
    if ((subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP) &&
            (extractx != NULL) && (extractx->fectx_is_ml_proberesp)) {
        /* Skip over the Timestamp and Beacon Interval fields in the reporting
         * and reported STA buffers.
         */
        if (ieee80211_multilink_ie_skipsection(&curr_rptingbufpos,
                    &rptingbufsize_remaining, "reporting STA buffer",
                    (WLAN_TIMESTAMP_LEN + WLAN_BEACONINTERVAL_LEN),
                    "Timestamp and Beacon Interval fields") !=
                QDF_STATUS_SUCCESS) {
            return -1;
        }

        if (ieee80211_multilink_ie_skipsection(&curr_rptedbufpos,
                    &rptedbufsize_remaining, "reported STA buffer",
                    (WLAN_TIMESTAMP_LEN + WLAN_BEACONINTERVAL_LEN),
                    "Timestamp and Beacon Interval fields") !=
                QDF_STATUS_SUCCESS) {
            return -1;
        }
    }

    /* Skip over the Capability Information field in reporting STA buffer. */
    if (ieee80211_multilink_ie_skipsection(&curr_rptingbufpos,
                &rptingbufsize_remaining, "reporting STA buffer",
                WLAN_CAPABILITYINFO_LEN, "Capability Information") !=
            QDF_STATUS_SUCCESS) {
        return -1;
    }

    /* Copy Capability Information field from reported STA buffer into STA
     * profile buffer. As of IEEE802.11be D1.0, there is no facility to apply
     * inheritance to this field.
     */
    if (spbufspace_remaining < WLAN_CAPABILITYINFO_LEN) {
        mloie_err("Insufficient space remaining in STA profile buffer for Capability Information, required: %u octets, remaining: %zu octets",
                WLAN_CAPABILITYINFO_LEN, spbufspace_remaining);
        return -1;
    }

    if (rptedbufsize_remaining < WLAN_CAPABILITYINFO_LEN) {
        mloie_err("Insufficient size remaining in reported STA buffer for Capability Information, required: %u octets, remaining: %zu octets",
                WLAN_CAPABILITYINFO_LEN, rptedbufsize_remaining);
        return -1;
    }

    memcpy(curr_spbufpos, curr_rptedbufpos, WLAN_CAPABILITYINFO_LEN);
    curr_spbufpos += WLAN_CAPABILITYINFO_LEN;
    spbufspace_remaining -= WLAN_CAPABILITYINFO_LEN;
    curr_rptedbufpos += WLAN_CAPABILITYINFO_LEN;
    rptedbufsize_remaining -= WLAN_CAPABILITYINFO_LEN;

    if ((subtype == IEEE80211_FC0_SUBTYPE_ASSOC_REQ) ||
            (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ)) {
        /* The Listen Interval field and, for reassociation request, the Current
         * AP Address field are not included in the STA Profile. Skip over these
         * in reporting and reported STA buffers.
         */
        if (ieee80211_multilink_ie_skipsection(&curr_rptingbufpos,
                    &rptingbufsize_remaining, "reporting STA buffer",
                    WLAN_LISTENINTERVAL_LEN, "Listen Interval") !=
                QDF_STATUS_SUCCESS) {
            return -1;
        }

        if (ieee80211_multilink_ie_skipsection(&curr_rptedbufpos,
                    &rptedbufsize_remaining, "reported STA buffer",
                    WLAN_LISTENINTERVAL_LEN, "Listen Interval") !=
                QDF_STATUS_SUCCESS) {
            return -1;
        }

        if (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ) {
            if (ieee80211_multilink_ie_skipsection(&curr_rptingbufpos,
                        &rptingbufsize_remaining, "reporting STA buffer",
                        QDF_MAC_ADDR_SIZE, "Current AP Address") !=
                    QDF_STATUS_SUCCESS) {
                return -1;
            }

            if (ieee80211_multilink_ie_skipsection(&curr_rptedbufpos,
                        &rptedbufsize_remaining, "reported STA buffer",
                        QDF_MAC_ADDR_SIZE, "Current AP Address") !=
                    QDF_STATUS_SUCCESS) {
                return -1;
            }
        }
    } else if ((subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
            (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP)) {
        /* Skip over the Status Code field in reporting STA buffer. */
        if (ieee80211_multilink_ie_skipsection(&curr_rptingbufpos,
                    &rptingbufsize_remaining, "reporting STA buffer",
                    WLAN_STATUSCODE_LEN, "Status Code") !=
                QDF_STATUS_SUCCESS) {
            return -1;
        }

        /* Copy Status Code field from reported STA buffer into STA profile
         * buffer. As of IEEE802.11be D1.0, there is no facility to apply
         * inheritance to this field.
         */
        if (spbufspace_remaining < WLAN_STATUSCODE_LEN) {
            mloie_err("Insufficient space remaining in STA profile buffer for Status Code, required: %u octets, remaining: %zu octets",
                    WLAN_STATUSCODE_LEN, spbufspace_remaining);
            return -1;
        }

        if (rptedbufsize_remaining < WLAN_STATUSCODE_LEN) {
            mloie_err("Insufficient size remaining in reported STA buffer for Status Code, required: %u octets, remaining: %zu octets",
                    WLAN_STATUSCODE_LEN, rptedbufsize_remaining);
            return -1;
        }

        memcpy(curr_spbufpos, curr_rptedbufpos, WLAN_STATUSCODE_LEN);
        curr_spbufpos += WLAN_STATUSCODE_LEN;
        spbufspace_remaining -= WLAN_STATUSCODE_LEN;
        curr_rptedbufpos += WLAN_STATUSCODE_LEN;
        rptedbufsize_remaining -= WLAN_STATUSCODE_LEN;

        /* Skip over the AID field in the reporting and reported STA buffers.
         */
        if (ieee80211_multilink_ie_skipsection(&curr_rptingbufpos,
                    &rptingbufsize_remaining, "reporting STA buffer",
                    WLAN_AID_LEN, "AID field") != QDF_STATUS_SUCCESS) {
            return -1;
        }

        if (ieee80211_multilink_ie_skipsection(&curr_rptedbufpos,
                    &rptedbufsize_remaining, "reported STA buffer",
                    WLAN_AID_LEN, "AID field") != QDF_STATUS_SUCCESS) {
            return -1;
        }
    }

    /* Mark the position where the IEs start being populated in the reporting
     * and reported STA buffers.
     */
    rptingbuf_iestartpos = curr_rptingbufpos;
    rptingbuf_iesectionlen = rptingbufsize_remaining;
    rptedbuf_iestartpos = curr_rptedbufpos;
    rptedbuf_iesectionlen = rptedbufsize_remaining;

    staprofbuf_iestartpos=curr_spbufpos;
    staprofbuf_iesectionlen = 0;

    discard_fragseq_sp = false;

    /* Parse through IEs in reported STA buffer and determine whether to add
     * them to STA profile.
     */
    while (rptedbufsize_remaining > 0) {
        /* Note: Support for fragments, combinations involving multiple
         * instances of elements having the same element ID/element ID extension
         * will be handled later after getting some further clarifications in
         * the standard.
         */

        /* Pointer to current element in reported STA buffer */
        const u_int8_t *curr_rptedelem = NULL;
        /* Pointer to element in reporting STA buffer having same element
         * ID/element ID extension as current element of reported STA buffer.
         */
        const u_int8_t *curr_rptingelem = NULL;

        if (rptedbufsize_remaining < IEEE80211_IE_HDR_LEN) {
            mloie_err("Insufficient size remaining in reported STA buffer for IE header, required: %u octets, remaining: %zu octets",
                    IEEE80211_IE_HDR_LEN, rptedbufsize_remaining);
            return -1;
        }

        curr_rptedelem = curr_rptedbufpos;

        if ((rptedbufsize_remaining - IEEE80211_IE_HDR_LEN) <
                curr_rptedelem[TAG_LEN_POS]) {
            mloie_err("Insufficient size remaining in reported STA buffer for IE content, required: %u octets, remaining: %zu octets",
                    curr_rptedelem[TAG_LEN_POS],
                    (rptedbufsize_remaining - IEEE80211_IE_HDR_LEN));
            return -1;
        }

        tot_elemsize = IEEE80211_IE_HDR_LEN + curr_rptedelem[TAG_LEN_POS];

        if (discard_fragseq_sp) {
            /* Discard potential trailing fragment sequence for a 'fragmentable'
             * element excluded from STA profile.
             */
            if (curr_rptedelem[ID_POS] == IEEE80211_ELEMID_FRAG) {
                mloie_debug("Not including elem ID %u with len %zu in STA profile since it is part of a frag sequence for an excluded element",
                        curr_rptedelem[ID_POS], tot_elemsize);
                curr_rptedbufpos += tot_elemsize;
                rptedbufsize_remaining -= tot_elemsize;

                if (curr_rptedelem[TAG_LEN_POS] < IEEE80211_MAX_IE_LEN) {
                    /* We have come to the end of the fragment sequence to be
                     * discarded since there cannot be any more fragments in the
                     * sequence if the length of the current fragment is not the
                     * maximum possible.
                     */
                    discard_fragseq_sp = false;
                }

                continue;
            } else {
                    /* We have come to the end of the fragment sequence to be
                     * discarded, or if this is the first element after the
                     * excluded element then there was no fragment sequence for
                     * the excluded element.
                     */
                    discard_fragseq_sp = false;
            }
        }

        if ((subtype == IEEE80211_FC0_SUBTYPE_ASSOC_REQ) ||
                (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ)) {
            /* The SSID element and Multi Link element are not included in the
             * STA Profile field. Skip these.
             *
             * In case of future standards requirements to skip any other
             * elements, add to below.
             */
            if (curr_rptedelem[ID_POS] == IEEE80211_ELEMID_SSID) {
                mloie_debug("Not including elem ID %u with len %zu in STA profile",
                        curr_rptedelem[ID_POS], tot_elemsize);
                curr_rptedbufpos += tot_elemsize;
                rptedbufsize_remaining -= tot_elemsize;
                continue;
            }

            if ((curr_rptedelem[ID_POS] == IEEE80211_ELEMID_EXTN) &&
                (curr_rptedelem[IDEXT_POS] == IEEE80211_ELEMID_EXT_MULTILINK)) {
                mloie_debug("Not including elem ID %u, elem ID extn %u with len %zu in STA profile",
                            curr_rptedelem[ID_POS], curr_rptedelem[IDEXT_POS],
                            tot_elemsize);
                curr_rptedbufpos += tot_elemsize;
                rptedbufsize_remaining -= tot_elemsize;

                if (curr_rptedelem[TAG_LEN_POS] == IEEE80211_MAX_IE_LEN)
                    discard_fragseq_sp = true;

                continue;
            }
        }

        if ((subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
                (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP) ||
                ((subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP) &&
                 (extractx != NULL) && (extractx->fectx_is_ml_proberesp))) {

            /* As per IEEE 802.11be/D3.0:
             * Need to add SSID element for the case where Basic Multi Link element
             * has complete profile and is contained in a multi-link probe response
             * transmitted by the transmitted BSSID in a multiple BSSID set in
             * response to a multi-link probe request directed to the nontransmitted
             * BSSID in the same multiple BSSID set.
             */
            if (extractx && extractx->fectx_is_ml_proberesp && extractx->non_tx_vap &&
                (curr_rptedelem[ID_POS] == IEEE80211_ELEMID_SSID)) {

                if (spbufspace_remaining < tot_elemsize) {
                    mloie_err("Insufficient space remaining in STA profile buffer for IE, required: %zu octets, remaining: %zu octets",
                              tot_elemsize, spbufspace_remaining);
                    return -1;
                }

                mloie_debug("Including elem ID %u with len %zu in STA profile",
                            curr_rptedelem[ID_POS], tot_elemsize);
                qdf_mem_copy(curr_spbufpos, curr_rptedelem, tot_elemsize);
                curr_spbufpos += tot_elemsize;
                spbufspace_remaining -= tot_elemsize;
                curr_rptedbufpos += tot_elemsize;
                rptedbufsize_remaining -= tot_elemsize;
                staprofbuf_iesectionlen += tot_elemsize;
                continue;
            }

            /* The SSID element, TIM element, BSS Max Idle Period element,
             * Neighbor Report, MBSSID ,Reduced Neighbor report, Multiple
             * BSSID-Index element, Multiple BSSID Configuration element and
             * Multi-Link element are not included in the STA Profile field.
             * Skip these. Note that these elements may not necessarily be
             * included in some of the subtypes considered in the if check.
             * However, we use the if check in a generic manner for flexibility
             * in case of future changes in the standard, since the standard
             * broadly specifies that if the reporting STA is an AP STA, then
             * these elements are not to be included.
             */
            if ((curr_rptedelem[ID_POS] == IEEE80211_ELEMID_SSID) ||
                    (curr_rptedelem[ID_POS] == IEEE80211_ELEMID_TIM) ||
                    (curr_rptedelem[ID_POS] ==
                     IEEE80211_ELEMID_BSSMAX_IDLE_PERIOD) ||
                     (curr_rptedelem[ID_POS] == IEEE80211_ELEMID_NEIGHBOR_RPT) ||
                     (curr_rptedelem[ID_POS] == IEEE80211_ELEMID_MBSSID) ||
                     (curr_rptedelem[ID_POS] == IEEE80211_ELEMID_REDUCED_NBR_RPT) ||
                     (curr_rptedelem[ID_POS] == IEEE80211_ELEMID_MBSSID_INDEX)) {
                mloie_debug("Not including elem ID %u with len %zu in STA profile",
                        curr_rptedelem[ID_POS], tot_elemsize);
                curr_rptedbufpos += tot_elemsize;
                rptedbufsize_remaining -= tot_elemsize;
                continue;
            }

            if ((curr_rptedelem[ID_POS] == IEEE80211_ELEMID_EXTN) &&
                ((curr_rptedelem[IDEXT_POS] == IEEE80211_ELEMID_EXT_MBSSID_CONFIG) ||
                (curr_rptedelem[IDEXT_POS] == IEEE80211_ELEMID_EXT_MULTILINK) ||
                (curr_rptedelem[IDEXT_POS] == IEEE80211_ELEMID_EXT_T2LM))) {

                mloie_debug("Not including elem ID %u, elem ID extn %u with len %zu in STA profile",
                            curr_rptedelem[ID_POS], curr_rptedelem[IDEXT_POS],
                            tot_elemsize);

                curr_rptedbufpos += tot_elemsize;
                rptedbufsize_remaining -= tot_elemsize;

                if ((curr_rptedelem[IDEXT_POS] ==
                        IEEE80211_ELEMID_EXT_MULTILINK) &&
                        (curr_rptedelem[TAG_LEN_POS] == IEEE80211_MAX_IE_LEN))
                    discard_fragseq_sp = true;

                continue;
            }
        }

        /* We tentatively copy all vendor IEs from the reported STA buffer to
         * the STA profile. Note this is not optimal and not necessarily
         * standards conformant. This will be changed once we have the
         * full-fledged implementation handling multiple instances of elements.
         */
        if (curr_rptedelem[ID_POS] == IEEE80211_ELEMID_VENDOR) {

            if (spbufspace_remaining < tot_elemsize) {
                mloie_err("Insufficient space remaining in STA profile buffer for IE, required: %zu octets, remaining: %zu octets",
                        tot_elemsize, spbufspace_remaining);
                return -1;
            }

            mloie_debug("Including elem ID %u with len %zu in STA profile",
                        curr_rptedelem[ID_POS], tot_elemsize);
            memcpy(curr_spbufpos, curr_rptedelem, tot_elemsize);
            curr_spbufpos += tot_elemsize;
            spbufspace_remaining -= tot_elemsize;
            curr_rptedbufpos += tot_elemsize;
            rptedbufsize_remaining -= tot_elemsize;
            staprofbuf_iesectionlen += tot_elemsize;

            continue;
        }

        /* Find whether an element exists in the reporting STA buffer which has
         * the same element ID/element ID extension as the current element in
         * the reported STA buffer.
         * Note: As of now, we do not optimize the element search process using
         * assumptions such as fixed order of elements in reported and reporting
         * STA buffers, since such assumptions may not necessarily hold true for
         * some proprietary implementation variations. This can be revisited if
         * required.
         */
        if (curr_rptedelem[ID_POS] != IEEE80211_ELEMID_EXTN) {
            curr_rptingelem = wlan_get_ie_ptr_from_eid(curr_rptedelem[ID_POS],
                    rptingbuf_iestartpos, rptingbuf_iesectionlen);
        } else {
            if (curr_rptedelem[TAG_LEN_POS] < 1) {
                mloie_err("Incorrect element length");
                return -1;
            }

            curr_rptingelem = wlan_get_ext_ie_ptr_from_ext_id(
                    &curr_rptedelem[IDEXT_POS], 1, rptingbuf_iestartpos,
                    rptingbuf_iesectionlen);
        }

        /* Note: For simplicity and code reuse, we compare everything after the
         * IE length field even in the case of element ID extension.
         */
        if ((curr_rptingelem == NULL) ||
                (curr_rptingelem[TAG_LEN_POS] != curr_rptedelem[TAG_LEN_POS]) ||
                qdf_mem_cmp(curr_rptingelem + IEEE80211_IE_HDR_LEN,
                             curr_rptedelem + IEEE80211_IE_HDR_LEN,
                             curr_rptedelem[TAG_LEN_POS])) {
            /* Element not found in reporting STA buffer. Copy current element
             * in reported STA buffer into STA profile.
             */
            tot_elemsize = IEEE80211_IE_HDR_LEN +
                curr_rptedelem[TAG_LEN_POS];

            if (spbufspace_remaining < tot_elemsize) {
                mloie_err("Insufficient space remaining in STA profile buffer for IE, required: %zu octets, remaining: %zu octets",
                        tot_elemsize, spbufspace_remaining);
                return -1;
            }

            if (curr_rptedelem[ID_POS] != IEEE80211_ELEMID_EXTN) {
                mloie_debug("Including elem ID %u with len %zu in STA profile",
                            curr_rptedelem[ID_POS], tot_elemsize);
            } else {
                mloie_debug("Including elem ID %u, elem ID extn %u with len %zu in STA profile",
                            curr_rptedelem[ID_POS], curr_rptedelem[IDEXT_POS],
                            tot_elemsize);
            }

            memcpy(curr_spbufpos, curr_rptedelem, tot_elemsize);
            curr_spbufpos += tot_elemsize;
            spbufspace_remaining -= tot_elemsize;
            staprofbuf_iesectionlen += tot_elemsize;
        } else {
             if (curr_rptedelem[ID_POS] != IEEE80211_ELEMID_EXTN) {
                mloie_debug("Not including elem ID %u with len %zu in STA profile",
                            curr_rptedelem[ID_POS], tot_elemsize);
            } else {
                mloie_debug("Not including elem ID %u, elem ID extn %u with len %zu in STA profile",
                            curr_rptedelem[ID_POS], curr_rptedelem[IDEXT_POS],
                            tot_elemsize);
            }
        }

        curr_rptedbufpos += tot_elemsize;
        rptedbufsize_remaining -= tot_elemsize;
    }

    /* Parse through IEs in reported STA buffer and determine whether to add
     * them to non-inheritance element.
     */
    while (rptingbufsize_remaining > 0) {
        /* Note: Support for fragments, combinations involving multiple
         * instances of elements having the same element ID/element ID extension
         * will be handled later after getting some further clarifications in
         * the standard.
         */

        /* Pointer to current element in reporting STA buffer */
        const u_int8_t *curr_rptingelem = NULL;
        /* Pointer to element in reported STA buffer having same element
         * ID/element ID extension as current element of reported STA buffer.
         */
        const u_int8_t *curr_rptedelem = NULL;

        if (rptingbufsize_remaining < IEEE80211_IE_HDR_LEN) {
            mloie_err("Insufficient size remaining in reporting STA buffer for IE header, required: %u octets, remaining: %zu octets",
                    IEEE80211_IE_HDR_LEN, rptingbufsize_remaining);
            return -1;
        }

        curr_rptingelem = curr_rptingbufpos;

        if (rptingbufsize_remaining < curr_rptingelem[TAG_LEN_POS]) {
            mloie_err("Insufficient size remaining in reporting STA buffer for IE content, required: %u octets, remaining: %zu octets",
                    curr_rptingelem[TAG_LEN_POS], rptingbufsize_remaining);
            return -1;
        }

        tot_elemsize = IEEE80211_IE_HDR_LEN + curr_rptingelem[TAG_LEN_POS];

        /* Determine whether the current element in the reporting STA buffer is
         * not present in the reported STA buffer.
         * Note: As of now, we do not optimize the element search process using
         * assumptions such as fixed order of elements in reported and reporting
         * STA buffers, since such assumptions may not necessarily hold true for
         * some proprietary implementation variations. This can be revisited if
         * required.
         */
        if (curr_rptingelem[ID_POS] != IEEE80211_ELEMID_EXTN) {
            curr_rptedelem = wlan_get_ie_ptr_from_eid(curr_rptingelem[ID_POS],
                    rptedbuf_iestartpos, rptedbuf_iesectionlen);

#ifdef MLO_USER_SPACE_ASSOC_IE_WORKAROUND
            if ((curr_rptingelem[ID_POS] != IEEE80211_ELEMID_RSN) &&
                (curr_rptedelem == NULL)) {
#else
            if (curr_rptedelem == NULL) {
#endif
                /* Element not found in reported STA buffer. Add current element
                 * to the element ID list in the Non-Inheritance IE.
                 */
                if ((ninherit_elemlist_len + 1) >
                        QDF_ARRAY_SIZE(ninherit_elemlist)) {
                    mloie_err("Entry would exceed max number of entries possible in Non-Inheritance IE element ID list. Investigate.");
                    return -1;
                }

                mloie_debug("Adding elem ID %u with len %zu in STA profile Non-Inheritance elem ID list",
                            curr_rptingelem[ID_POS], tot_elemsize);

                ninherit_elemlist[ninherit_elemlist_len] =
                    curr_rptingelem[ID_POS];
                ninherit_elemlist_len++;
            }
        } else {
            if (curr_rptingelem[TAG_LEN_POS] < 1) {
                mloie_err("Incorrect element length");
                return -1;
            }

            curr_rptedelem = wlan_get_ext_ie_ptr_from_ext_id(
                    &curr_rptingelem[IDEXT_POS], 1, rptedbuf_iestartpos,
                    rptedbuf_iesectionlen);

#ifdef MLO_USER_SPACE_ASSOC_IE_WORKAROUND
            if ((curr_rptingelem[IDEXT_POS] !=
                 IEEE80211_ELEMID_EXT_OWE_DH_PARAM) &&
                (curr_rptedelem == NULL)) {
#else
            if (curr_rptedelem == NULL) {
#endif
                /* Element not found in reported STA buffer. Add current element
                 * to the element ID extension list in the Non-Inheritance IE.
                 */
                if ((ninherit_elemextlist_len + 1) >
                        QDF_ARRAY_SIZE(ninherit_elemextlist)) {
                    mloie_err("Entry would exceed max number of entries possible in Non-Inheritance IE element ID extension list. Investigate.");
                    return -1;
                }

                mloie_debug("Adding elem ID %u, elem ID extn %u with len %zu in STA profile Non-Inheritance elem ID extn list",
                            curr_rptingelem[ID_POS], curr_rptingelem[IDEXT_POS],
                            tot_elemsize);

                ninherit_elemextlist[ninherit_elemextlist_len] =
                    curr_rptingelem[IDEXT_POS];
                ninherit_elemextlist_len++;
            }
        }

        curr_rptingbufpos += tot_elemsize;
        rptingbufsize_remaining -= tot_elemsize;
    }

    if (ninherit_elemlist_len || ninherit_elemextlist_len) {
        /* Length of the Non-Inheritance IE is =
         * IEEE80211_IE_HDR_LEN + Element ID extension (1 octet) +
         * Element ID list length (1 octet) +
         *      Element ID list (ninherit_elemlist_len octets) +
         * Element ID extension list length (1 octet) +
         *      Element ID extension list (ninherit_elemextlist_len octets)
         */
        ninherit_ielen = IEEE80211_IE_HDR_LEN + 1 +
            (1 + ninherit_elemlist_len) + (1 + ninherit_elemextlist_len);

        /* The Non-Inheritance IE is not fragmentable */
        if (ninherit_ielen > (IEEE80211_IE_HDR_LEN + IEEE80211_MAX_IE_LEN)) {
            mloie_err("Non-Inheritance IE length %zu would exceed max possible IE size %u. Element ID list length=%zu Element ID extension list length=%zu. Investigate.",
                    ninherit_ielen, IEEE80211_IE_HDR_LEN + IEEE80211_MAX_IE_LEN,
                    ninherit_elemlist_len, ninherit_elemextlist_len);
            return -1;
        }

        if (spbufspace_remaining < ninherit_ielen) {
           mloie_err("Insufficient space remaining in STA profile buffer for Non-Inheritance IE, required: %zu octets, remaining: %zu octets",
                   ninherit_ielen, spbufspace_remaining);
           return -1;
        }

        ninherit_ie = curr_spbufpos;
        ninherit_ie[ID_POS] = IEEE80211_ELEMID_EXTN;
        ninherit_ie[TAG_LEN_POS] = ninherit_ielen - IEEE80211_IE_HDR_LEN;
        ninherit_ie[IDEXT_POS] = IEEE80211_ELEMID_EXT_NON_INHERITANCE;

        curr_ninherit_ie_pos = ninherit_ie + IDEXT_POS + 1;

        /* Copy element ID list */
        *curr_ninherit_ie_pos = ninherit_elemlist_len;
        curr_ninherit_ie_pos++;

        if (ninherit_elemlist_len) {
            memcpy(curr_ninherit_ie_pos, ninherit_elemlist,
                    ninherit_elemlist_len);
            curr_ninherit_ie_pos += ninherit_elemlist_len;
        }

        /* Copy element ID extension list */
        *curr_ninherit_ie_pos = ninherit_elemextlist_len;
        curr_ninherit_ie_pos++;

        if (ninherit_elemextlist_len) {
            memcpy(curr_ninherit_ie_pos, ninherit_elemextlist,
                    ninherit_elemextlist_len);
            curr_ninherit_ie_pos += ninherit_elemextlist_len;
        }

        curr_spbufpos += ninherit_ielen;
        spbufspace_remaining -= ninherit_ielen;
        staprofbuf_iesectionlen += ninherit_ielen;
    }


    /* Update the Quiet count that is populated in
     * ieee80211_add_mlo_quiet_ie_in_assoc_resp().
     */
    quiet_ie = (struct ieee80211_ath_quiet_ie *)wlan_get_ie_ptr_from_eid(
            WLAN_ELEMID_QUIET, staprofbuf_iestartpos, staprofbuf_iesectionlen);
    if (quiet_ie) {
        cur_time = qdf_mc_timer_get_system_time();
        if (cur_time < (iv_quiet->quiet_completed_time + iv_quiet->duration))
            quiet_ie->tbttcount =
                (int)qdf_do_div((uint64_t)(cur_time - iv_quiet->quiet_completed_time),
                        (uint32_t)bcn_intval) + WLAN_QUIET_PERIOD_DEF;
    }

    /* Update the remaining channel switch time that is populated in
     * ieee80211_setup_assocresp().
     */
    eid = WLAN_EXTN_ELEMID_MAX_CHAN_SWITCH_TIME;
    max_cst_ie = (struct ieee80211_max_chan_switch_time_ie *)
        wlan_get_ext_ie_ptr_from_ext_id(&eid, 1, staprofbuf_iestartpos,
                staprofbuf_iesectionlen);
    if (max_cst_ie) {
        max_switch_time_in_ms =
            wlan_utils_get_vdev_remaining_channel_switch_time(vap->vdev_obj);
        if(vap->iv_ic->ic_mcst_of_rootap > max_switch_time_in_ms)
        {
            max_switch_time_in_ms = vap->iv_ic->ic_mcst_of_rootap;
        }
        max_switch_time_in_tu = IEEE80211_MS_TO_TU(max_switch_time_in_ms);

        /* Pack the max_time in 3 octets/bytes. Little endian format */
        for(i = 0; i < SIZE_OF_MAX_TIME_INT; i++) {
            max_cst_ie->switch_time[i] = (max_switch_time_in_tu & ONE_BYTE_MASK);
            max_switch_time_in_tu = (max_switch_time_in_tu >> BITS_IN_A_BYTE);
        }
    }

    return (staprofbufsize - spbufspace_remaining);
}

/**
 * ieee80211_is_tsf_sync_enabled() - Function to get whether support for TSF sync
 * is enabled or not.
 *
 * @vdev: Pointer to VDEV object
 *
 * @return True or False
 */
static bool ieee80211_is_tsf_sync_enabled(struct wlan_objmgr_vdev *vdev)
{
    struct wlan_objmgr_psoc *psoc = NULL;
    struct mlo_mgr_context *mlo_ctx = wlan_objmgr_get_mlo_ctx();
    struct mlo_setup_info *mlo_setup;
    uint8_t ml_grp_id;

    psoc = wlan_vdev_get_psoc(vdev);
    if (!psoc) {
        mloie_err("Psoc is NULL");
        return false;
    }

    ml_grp_id = wlan_mlo_get_psoc_group_id(psoc);
    if (ml_grp_id < 0 || ml_grp_id >= WLAN_MAX_MLO_GROUPS) {
        mloie_err("Invalid ML Grp ID");
        return false;
    }
    mlo_setup = &mlo_ctx->setup_info[ml_grp_id];

    return mlo_setup->tsf_sync_enabled;
}

/**
 * ieee80211_add_basic_multilink_ie_assocreqresp_proberesp_linkinfo() - Helper
 * function to add Link Info field in Basic variant Multi Link element specific
 * to (Re) association request/response and Probe response.
 * This helper does not take care of ML element fragmentation, the caller/caller
 * chain is expected to take care of this.
 *
 * Since this is a static helper function and we would like to avoid redundant
 * checks, it is the caller's/caller chain's responsibility to validate the
 * following:
 * - All pointer arguments are non-NULL
 * - vap in ni is non-NULL
 * - vdev object in vap is non-NULL
 * - This helper is called for the right subtype
 *
 * @param linfo: Starting position in buffer at which Link Info should be added
 * @param linfosize: Space in linfo available for filling Link Info (prior to
 * ML element fragmentation)
 * @param containerfrm: Frame which will contain the Multi-Link element, and
 * which already has other IEs required for forming per-STA profile entries in
 * Link Info of Multi-Link element.
 * @param containerfrmsize: Size of containerfrm
 * @param ni: Pointer to associated node structure
 * @param ic: Pointer to iee80211com
 * @param subtype: Frame subtype
 * @param extractx: Extra framing context. This can be NULL.
 * @param bo: Pointer to beacon offsets
 * @param rs: Pointer to receive status information associated with packet
 *
 * @return Positive integer value giving number of octets added to linfo in the
 * case of success, -1 in the case of failure
 */
static int ieee80211_add_basic_multilink_ie_assocreqresp_proberesp_linkinfo(
        u_int8_t *linfo, size_t linfosize, u_int8_t *containerfrm,
        size_t containerfrmsize, struct ieee80211_node *ni,
        struct ieee80211com *ic, u_int8_t subtype,
        struct ieee80211_framing_extractx *extractx,
        struct ieee80211_beacon_offsets *bo,
        struct ieee80211_rx_status *rs)
{
    struct ieee80211vap *vap = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_vdev *co_mld_vdev = NULL;
    struct ieee80211vap *co_mld_vap = NULL;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    struct tsf_snapshot concurrent_tsf_snapshot_list[WLAN_UMAC_MLO_MAX_VDEVS] = {0};
    uint64_t tsf_snapshot_reporting_ap = 0;
    uint16_t vdev_count = 0;
    uint16_t perstaprof_count = 0;
    uint16_t i = 0;
    uint16_t release_ref_start_idx;
    size_t linfospace_remaining = 0;
    /* The full assoc request or response buffer for the co MLD VDEV, prior to
     * application of inheritance.
     */
    qdf_nbuf_t assocnbuf = NULL;
    u_int8_t *assocbuf = NULL;
    size_t assocbuflen = 0;

    QDF_STATUS ret;

    /* See function documentation about caller's validation responsibilities. */

    if (extractx && extractx->non_tx_vap) {
        vap = extractx->non_tx_vap;
    } else {
        vap = ni->ni_vap;
    }

    vdev = vap->vdev_obj;

    linfospace_remaining = linfosize;

    mlo_get_ml_vdev_list(vdev, &vdev_count, wlan_vdev_list);

    if (!vdev_count) {
         mloie_err("Number of VDEVs under MLD is reported as 0");
         return -1;
    }

    /* In case of errors during initial checks, we need to release all the VDEV
     * references in the VDEV list starting from index 0.
     */
    release_ref_start_idx = 0;

    /* Note: Once API changes to mlo_ap_get_vdev_list() are available to handle
     * array size requirements, remove this condition.
     */
    if (vdev_count > QDF_ARRAY_SIZE(wlan_vdev_list)) {
         mloie_err("Count of VDEVs %u under MLD exceeds expected limit %zu, investigate",
                 vdev_count, QDF_ARRAY_SIZE(wlan_vdev_list));
         goto err_release_refs;
    }

    if (vdev_count < WLAN_UMAC_REPORTED_MLO_IE_PROC_MIN_VDEVS) {
         mloie_debug("Count of VDEVs %u under MLD lesser than expected minimum %u for adding Link Info",
                 vdev_count, WLAN_UMAC_REPORTED_MLO_IE_PROC_MIN_VDEVS);

        if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                     QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count,
                     release_ref_start_idx)) != QDF_STATUS_SUCCESS) {
             /* This should not occur. If this occurs, would indicate larger
              * issues.
              */
             mloie_err("Error while trying to release VDEV references - investigate.");
             return -1;
         }

         return 0;
    }

    /* Carry out pre-checks on the co MLD VDEVs */

    for(i = 0; i < vdev_count; i++) {
        co_mld_vdev = wlan_vdev_list[i];
        if (co_mld_vdev == NULL) {
            mloie_err("VDEV in MLD VDEV list is NULL");
            goto err_release_refs;
        }

        if (co_mld_vdev == vdev)
            continue;

        co_mld_vap = wlan_vdev_get_vap(co_mld_vdev);
        if (co_mld_vap == NULL) {
            mloie_err("Co-MLD VAP is NULL");
            goto err_release_refs;
        }

        /* Add any other pre-checks here if required in the future */
    }

    if (wlan_vdev_mlme_is_mlo_ap(vdev) && !ieee80211_is_tsf_sync_enabled(co_mld_vdev)) {
        ret = ieee80211_get_concurrent_tsf_snapshot_for_all_vdevs_mld(wlan_vdev_list, vdev_count,
                                                                      concurrent_tsf_snapshot_list,
                                                                      WLAN_UMAC_MLO_MAX_VDEVS);
        if (QDF_IS_STATUS_ERROR(ret)) {
            mloie_err("Failed to get concurrent TSF snapshot for all vdevs");
            goto err_release_refs;
        }

        for(i = 0; i < vdev_count; i++) {
            if (wlan_vdev_list[i] == vdev) {
                if (!concurrent_tsf_snapshot_list[i].valid) {
                    mloie_err("Reporting AP is expected to have valid tsf");
                    goto err_release_refs;
                }

                tsf_snapshot_reporting_ap = concurrent_tsf_snapshot_list[i].tsf;
                break;
            }
        }
    }

    for(i = 0; i < vdev_count; i++) {
        u_int8_t *perstaprof;
        size_t perstaprof_maxsize = 0;
        /* Fixed portion of per-STA profile */
        struct wlan_ml_bv_linfo_perstaprof *perstaprof_fixed = NULL;
        size_t perstaproflen = 0;
        bool is_frag_required;
        qdf_size_t required_fragbuff_size;
        size_t perstaproflen_fragseq = 0;
        u_int16_t stacontrol = 0;
        bool complete_profile = true;
        bool link_allow = false;
        u_int8_t linkid = 0;
        u_int8_t bssparamchangecnt;
        u_int16_t bcninterval = 0;
        int64_t tsfoffset = 0;
        u_int8_t *stainfo = NULL;
        u_int8_t j = 0;
        qdf_size_t stainfolen = 0;
        struct wlan_ml_bv_linfo_perstaprof_stainfo_dtiminfo *dtiminfo = NULL;
        /* STA profile buffer for the co MLD VDEV, after application of
         * inheritance.
         */
        u_int8_t *staprof = NULL;
        size_t staproflen = 0;
        bool is_nontx_vap = false;
        struct wlan_objmgr_psoc *psoc = NULL;

        assocnbuf = NULL;
        assocbuf = NULL;
        assocbuflen = 0;

        co_mld_vdev = wlan_vdev_list[i];

        if (co_mld_vdev == vdev) {
            mlo_release_vdev_ref(wlan_vdev_list[i]);
            continue;
        }

        co_mld_vap = wlan_vdev_get_vap(co_mld_vdev);

        /* In case of errors during further processing of co MLD VDEV with index
         * i, we need to release all the VDEV references in the VDEV list
         * starting from index i (since references for VDEVs prior to i would
         * already have been released).
         */
        release_ref_start_idx = i;

        if (co_mld_vap == NULL) {
            mloie_err("Co-MLD VAP is NULL");
            goto err_release_refs;
        }

        psoc = wlan_vdev_get_psoc(co_mld_vdev);
        if (!psoc) {
            mloie_err("Psoc is NULL");
            goto err_release_refs;
        }

        /* Don't add Per-STA profile if cac is running on vdev, based on ini */
        if ((cfg_get(psoc, CFG_OL_ENABLE_MLOADVERT_DEGRADE_ON_CAC) ==
             MLOADVERT_DEGRADE_ON_CAC_ENABLE) &&
             QDF_IS_STATUS_SUCCESS(wlan_vdev_is_dfs_cac_wait(co_mld_vdev))) {
            mloie_debug("Per-Sta profile for vap with vdev_id: %d link address %s not included during CAC",
                         co_mld_vap->iv_unit, ether_sprintf(co_mld_vap->iv_myaddr));
            mlo_release_vdev_ref(wlan_vdev_list[i]);
            continue;
        }


        /* Make sure to check if TX vap is up before adding NON-TX profile
         * This is temporary till we have wrapper API mlo_ap_get_active_vdevs()
         * implemented with similar check.
         */
        if ((co_mld_vap->iv_ic->is_mbssid_enabled) && (co_mld_vap->iv_ic->ic_mbss_ctx)) {
            is_nontx_vap = IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(co_mld_vap);

            if (is_nontx_vap) {
                struct mbssid_context *mbss_ctx;
                struct mbssid_info *ic_mbss;

                mbss_ctx = co_mld_vap->iv_ic->ic_mbss_ctx;
                ic_mbss  = &mbss_ctx->ic_mbss[co_mld_vap->iv_mbss.group_id];

                if ((!ic_mbss->transmit_vap) ||
                     (wlan_vdev_chan_config_valid(ic_mbss->transmit_vap->vdev_obj) != QDF_STATUS_SUCCESS)) {
                    mlo_release_vdev_ref(wlan_vdev_list[i]);
                    continue;
                }
            }
        }

        linkid = wlan_vdev_get_link_id(co_mld_vdev);

        /* Reply to requested LINK_ID and skip the other links for Per-STA profile */
        if(extractx && extractx->ml_preq_info.num_links && extractx->fectx_is_ml_proberesp) {
            for(j = 0; j < extractx->ml_preq_info.num_links; j++) {
                /* link_id will be store at indicies based in the order at which the per-STA profile
                 * in ML request has it. So traverse and see if a matching link is found for the
                 * current Co-MLD vap in which case, add the link in per-STA profile ML probe response
                 */
                if(linkid == extractx->ml_preq_info.link_id[j]) {
                    link_allow = true;
                    break;
                }
            }
            /* If no matching link found skip the current Per-STA profile */
            if(link_allow == false) {
                mlo_release_vdev_ref(wlan_vdev_list[i]);
                continue;
            }

        }

        /* First, determine if an association nbuf is present for this co MLD
         * VDEV. If it is not present, then the co MLD VDEV is not involved in
         * the current association and we are not interested in it.
         */
        if ((subtype == IEEE80211_FC0_SUBTYPE_ASSOC_REQ) ||
            (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ))
            assocnbuf = mlo_peer_get_link_peer_assoc_req_buf(
                    ni->peer_obj->mlo_peer_ctx, linkid);
        else if ((subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
            (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP))
            assocnbuf = mlo_peer_get_link_peer_assoc_resp_buf(
                    ni->peer_obj->mlo_peer_ctx, linkid);
        else if (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP) {
            struct ieee80211_framing_extractx extractx = {0};

            /* only setup don't send the frame */
            extractx.only_setup = true;
            if (EOK == ieee80211_send_proberesp(co_mld_vap->iv_bss,
                                     IEEE80211_GET_BCAST_ADDR(vap->iv_ic),
                                     NULL, 0, &extractx, rs))
                assocnbuf = extractx.ret_nbuf;
        }

        if (assocnbuf == NULL) {
            mlo_release_vdev_ref(wlan_vdev_list[i]);

            mloie_debug("Not attempting to populate per-STA profile for VAP with link address %s since it is not included in the association",
                ether_sprintf(vap->iv_myaddr));
            continue;
        }

        if (linfospace_remaining < sizeof(struct wlan_ml_bv_linfo_perstaprof)) {
            mloie_err("Insufficient space in Link Info buffer for fixed fields of Per-STA Profile, required: %zu octets, remaining: %zu octets",
                sizeof(struct wlan_ml_bv_linfo_perstaprof),
                linfospace_remaining);
            goto err_release_refs;
        }

        perstaprof = linfo;
        perstaprof_maxsize = linfospace_remaining;
        perstaprof_fixed = (struct wlan_ml_bv_linfo_perstaprof *)perstaprof;
        linfospace_remaining -= sizeof(*perstaprof_fixed);

        /* The Per-STA profile sub element length will be filled in later after
         * determining other contents. Till then, we accumulate the total into a
         * separate variable.
         */

        perstaproflen = sizeof(*perstaprof_fixed);

        perstaprof_fixed->subelem_id =
            WLAN_ML_LINFO_SUBELEMID_PERSTAPROFILE;

        stainfo = (u_int8_t *)perstaprof_fixed + sizeof(*perstaprof_fixed);

        /* Increment stainfolen to account for the size of the STA Info Length
         * subfield after validating that there is sufficient remaining space
         * for the subfield. But the subfield will be populated later.
         */
        if (linfospace_remaining <
                WLAN_ML_BV_LINFO_PERSTAPROF_STAINFO_LENGTH_SIZE) {
            mloie_err("Insufficient space in Link Info buffer for STA info length in Per-STA Profile, required: %u octets, remaining: %zu octets",
                WLAN_ML_BV_LINFO_PERSTAPROF_STAINFO_LENGTH_SIZE,
                linfospace_remaining);
            goto err_release_refs;
        }

        stainfolen += WLAN_ML_BV_LINFO_PERSTAPROF_STAINFO_LENGTH_SIZE;
        linfospace_remaining -= WLAN_ML_BV_LINFO_PERSTAPROF_STAINFO_LENGTH_SIZE;

        IEEE80211_SET_BITS(stacontrol,
                WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_LINKID, linkid);

        if (complete_profile) {
            /* (Re)assoc request/response shall carry complete profile */
            IEEE80211_SET_BITS(stacontrol,
                    WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_CMPLTPROF, 1);
        }

        /* The MAC address shall be present if this is a complete profile */
        if (complete_profile) {
            if (linfospace_remaining < QDF_MAC_ADDR_SIZE) {
                mloie_err("Insufficient space in Link Info buffer for MAC address in STA info in Per-STA Profile, required: %u octets, remaining: %zu octets",
                    QDF_MAC_ADDR_SIZE, linfospace_remaining);
                goto err_release_refs;
            }

            IEEE80211_SET_BITS(stacontrol,
                    WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_MACADDRP, 1);

            IEEE80211_ADDR_COPY(stainfo + stainfolen, co_mld_vap->iv_myaddr);
            stainfolen += QDF_MAC_ADDR_SIZE;
            linfospace_remaining -= QDF_MAC_ADDR_SIZE;
        }

        /* An AP shall shall include Beacon Interval, TSF Offset and DTIM info
         * if this is a complete profile.
         */
        if (wlan_vdev_mlme_is_mlo_ap(vdev)) {
            /*
             * If a new field is added which doesn't depends on the complete profile,
             * it has to be added outside the conditional check for complete profile.
             */
            if (complete_profile) {
                if (linfospace_remaining < sizeof(bcninterval)) {
                    mloie_err("Insufficient space in Link Info buffer for Beacon Interval in STA info in Per-STA Profile, required: %zu octets, remaining: %zu octets",
                        sizeof(bcninterval), linfospace_remaining);
                    goto err_release_refs;
                }

                IEEE80211_SET_BITS(stacontrol,
                        WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_BCNINTP, 1);
                bcninterval =
                    ieee80211_node_get_beacon_interval(co_mld_vap->iv_bss);
                bcninterval = htole16(bcninterval);
                memcpy(stainfo + stainfolen, &bcninterval, sizeof(bcninterval));
                stainfolen += sizeof(bcninterval);
                linfospace_remaining -= sizeof(bcninterval);

                if (linfospace_remaining < sizeof(tsfoffset)) {
                    mloie_err("Insufficient space in Link Info buffer for TSF Offset in STA info in Per-STA Profile, required: %zu octets, remaining: %zu octets",
                        sizeof(tsfoffset), linfospace_remaining);
                    goto err_release_refs;
                }

                IEEE80211_SET_BITS(stacontrol,
                        WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_TSFOFFSETP, 1);
                tsfoffset = 0;
                if (!ieee80211_is_tsf_sync_enabled(co_mld_vdev) && concurrent_tsf_snapshot_list[i].valid)
                    tsfoffset = (int64_t)(concurrent_tsf_snapshot_list[i].tsf - tsf_snapshot_reporting_ap) >> 1;
                tsfoffset = htole64(tsfoffset);
                memcpy(stainfo + stainfolen, &tsfoffset, sizeof(tsfoffset));
                stainfolen += sizeof(tsfoffset);
                linfospace_remaining -= sizeof(tsfoffset);

                if (linfospace_remaining <
                    sizeof(struct wlan_ml_bv_linfo_perstaprof_stainfo_dtiminfo)) {
                    mloie_err("Insufficient space in Link Info buffer for DTIM Info in STA info in Per-STA Profile, required: %zu octets, remaining: %zu octets",
                        sizeof(struct wlan_ml_bv_linfo_perstaprof_stainfo_dtiminfo),
                        linfospace_remaining);
                    goto err_release_refs;
                }

                IEEE80211_SET_BITS(stacontrol,
                        WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_DTIMINFOP, 1);
                dtiminfo = (struct wlan_ml_bv_linfo_perstaprof_stainfo_dtiminfo *)
                    (stainfo + stainfolen);
                /* Note: Support for updating DTIM count will be added in a later
                 * change.
                 */
                dtiminfo->dtimcount = 0;
                dtiminfo->dtimperiod =
                    co_mld_vap->vdev_mlme->proto.generic.dtim_period;
                stainfolen += sizeof(*dtiminfo);
                linfospace_remaining -= sizeof(*dtiminfo);
            }
        }

        /* BPCC in probe, assoc and reassoc response frame is populated
         * from the latest copy of BPCC values received from FW.
         */
        if (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP ||
                subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP) {
            if (complete_profile) {
                if (linfospace_remaining < sizeof(bssparamchangecnt)) {
                    mloie_err("Insufficient space in Link Info buffer for BPCC in STA info in Per-STA Profile, required: %zu octets, remaining: %zu octets",
                            sizeof(bssparamchangecnt), linfospace_remaining);

                    goto err_release_refs;
                }
                bssparamchangecnt = iee80211_cu_get_bssparamchangecnt(co_mld_vap, rs);
                IEEE80211_SET_BITS(stacontrol,
                        WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_BSSPARAMCHNGCNTP, 1);

                qdf_mem_copy(stainfo + stainfolen, &bssparamchangecnt, sizeof(bssparamchangecnt));
                stainfolen += sizeof(bssparamchangecnt);
                linfospace_remaining -= sizeof(bssparamchangecnt);
            }
        }

        /* Note:
         *
         * NSTR Link Pair Present bit to be populated based on FW advertisement
         * once available. As of now, it is left set to 0.
         *
         * Similarly, NSTR Bitmap Size to be populated based on FW
         * advertisement once available. As of now, it is left reserved.
         */

        if (stainfolen > WLAN_ML_BV_LINFO_PERSTAPROF_STAINFO_LENGTH_MAX) {
            mloie_err("STA Info len %zu octets exceeds max value %zu octets",
                stainfolen, WLAN_ML_BV_LINFO_PERSTAPROF_STAINFO_LENGTH_MAX);
            goto err_release_refs;
        }

        *stainfo = stainfolen;

        /* End of STA control and STA info population */

        perstaprof_fixed->stacontrol = htole16(stacontrol);
        perstaproflen += stainfolen;

        assocbuf = qdf_nbuf_data(assocnbuf);
        if (assocbuf == NULL) {
            mloie_err("Could not get address of the start of data within assoc nbuf");
            goto err_release_refs;
        }

        assocbuflen = qdf_nbuf_len(assocnbuf);
        if (assocbuflen == 0) {
            mloie_err("Size of data in assoc nbuf is 0");
            goto err_release_refs;
        }

        staprof = stainfo + stainfolen;

        staproflen = ieee80211_gen_basic_multilink_ie_completestaprof(
                containerfrm, containerfrmsize, assocbuf, assocbuflen, staprof,
                linfospace_remaining, subtype, extractx, co_mld_vap);

        if (staproflen == -1) {
            mloie_err("Error when trying to generate complete STA profile");
            goto err_release_refs;
        }

        perstaproflen += staproflen;
        linfospace_remaining -= staproflen;

        ret = wlan_get_subelem_fragseq_requirements(
                    WLAN_ML_LINFO_SUBELEMID_PERSTAPROFILE,
                    perstaproflen - IEEE80211_SUBELEM_HDR_LEN,
                    &is_frag_required,
                    &required_fragbuff_size);
        if (QDF_IS_STATUS_ERROR(ret)) {
            goto err_release_refs;
        }

        if (is_frag_required) {
            if (required_fragbuff_size > perstaprof_maxsize) {
                mloie_err("Insufficient space for creating per-STA profile subelement fragment sequence. Required: %zu octets, available: %zu octets",
                          required_fragbuff_size, perstaprof_maxsize);
                goto err_release_refs;
            }

            /* The utility will take care of populating subelement ID and
             * subelement length, in the leading fragment and other fragments.
             */
            ret = wlan_create_subelem_fragseq(true,
                        WLAN_ML_LINFO_SUBELEMID_PERSTAPROFILE,
                        WLAN_ML_LINFO_SUBELEMID_FRAGMENT,
                        perstaprof,
                        perstaprof_maxsize,
                        perstaproflen - IEEE80211_SUBELEM_HDR_LEN,
                        NULL,
                        0,
                        &perstaproflen_fragseq);

            if (QDF_IS_STATUS_ERROR(ret)) {
                goto err_release_refs;
            }

            /* Reduce linfospace_remaining by the additional octets introduced
             * due to the fragmentation.
             */
            linfospace_remaining -= (perstaproflen_fragseq - perstaproflen);

            perstaproflen = perstaproflen_fragseq;
        } else {
            perstaprof_fixed->subelem_len = perstaproflen -
                IEEE80211_SUBELEM_HDR_LEN;
        }

        linfo += perstaproflen;

        perstaprof_count++;

        mlo_release_vdev_ref(wlan_vdev_list[i]);

        /* See comment for the first call to qdf_nbuf_free(assocnbuf) in this
         * function.
         */
        qdf_nbuf_free(assocnbuf);
    }

    return (linfosize - linfospace_remaining);

err_release_refs:
    if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                    QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count,
                    release_ref_start_idx)) != QDF_STATUS_SUCCESS) {
        /* This should not occur. If this occurs, would indicate larger issues.
         */
        mloie_err("Error while trying to release VDEV references - investigate.");
    }

    /* Though we free assocnbuf here since we no longer need this buffer as of
     * this call, the peer management framework may choose to maintain
     * references/clones to this wbuf for some subtypes, in case it needs to
     * repeat the ML IE generation process for the same assocnbuf. The actual
     * free will happen later in this case and will be handled by the peer
     * management framework.
     */
    if (assocnbuf) {
        qdf_nbuf_free(assocnbuf);
    }

    return -1;
}

/**
 * ieee80211_beacon_add_mlo_csa_ies() - Function to add MLO CSA, eCSA and
 * Max Channel Switch Time IE.
 *
 * @param buf: Starting position in buffer at which CSA, eCSA and Max Channel
 *             Switch Time IE needs be added.
 * @param buflen: Space available for adding CSA, eCSA and Max Channel Switch
 *                Time IE.
 * @param co_mld_vdev: Pointer to partner vdev structure for which CSA, eCSA and
 *                     Max Channel Switch Time IE needs be added in the per-STA
 *                     profile.
 * @param subtype: Frame subtype
 *
 * @return Zero or positive integer value giving number of octets added to
 * buffer in the case of success, -1 in the case of failure. Note that
 * a return value of 0 indicates that CSA, eCSA, Max Channel Switch Time IE are
 * not currently added.
 */
static int ieee80211_beacon_add_mlo_csa_ies(uint8_t *buf, size_t buflen,
        struct wlan_objmgr_vdev *co_mld_vdev,
        uint8_t subtype)
{
    struct ieee80211_ath_channelswitch_ie *csa = NULL;
    struct ieee80211_extendedchannelswitch_ie *ecsa = NULL;
    struct ieee80211_max_chan_switch_time_ie *max_cst_ie = NULL;
    struct ieee80211vap *co_mld_vap;
    struct ieee80211com *co_mld_ic;
    size_t available_space = 0;
    uint16_t total_len = 0;
    int8_t csmode;
    int8_t len = 0;
    uint8_t csa_tbtt = 0;

    co_mld_vap = wlan_vdev_get_vap(co_mld_vdev);
    if (!co_mld_vap) {
        mloie_err("co_mld_vap is NULL");
        return -1;
    }

    co_mld_ic = co_mld_vap->iv_ic;
    if (!co_mld_ic) {
        mloie_err("co_mld_ic is NULL");
        return -1;
    }

    available_space = buflen;

    if (available_space < sizeof(struct ieee80211_ath_channelswitch_ie)) {
        mloie_err("Insufficient space to add CSA IE, required: %zu octets, remaining: %zu octets",
                sizeof(struct ieee80211_ath_channelswitch_ie),
                available_space);
        return -1;
    }

    if (ieee80211_ic_doth_is_set(co_mld_ic) &&
        (co_mld_ic->ic_flags & IEEE80211_F_CHANSWITCH) &&
        IEEE80211_IS_CHAN_VALID(co_mld_ic->ic_chanchange_channel)) {

        /* Get the channel switch mode */
        csmode = ieee80211_beacon_get_csmode(co_mld_vap);
        if (csmode < 0) {
            mloie_err("Failed to get the csmode");
            return -1;
        }

        csa_tbtt = co_mld_ic->ic_chanchange_tbtt -
            co_mld_vap->iv_chanchange_count;

        /* Add Channel Switch Announcement IE */
        csa= (struct ieee80211_ath_channelswitch_ie *)buf;
        len = ieee80211_beacon_add_csa_ie(buf, co_mld_vap, csmode, csa_tbtt);
        if (len < 0) {
            mloie_err("failed to add CSA IE");
            return -1;
        }

        available_space -= len;
        total_len += len;

        /* Add Extended Channel Switch Announcement IE */
        if (available_space < sizeof(struct ieee80211_extendedchannelswitch_ie)) {
            mloie_err("Insufficient space to add eCSA IE, required: %zu octets, remaining: %zu octets",
                    sizeof(struct ieee80211_ath_channelswitch_ie),
                    available_space);
            return -1;
        }

        ecsa = (struct ieee80211_extendedchannelswitch_ie *)(buf + total_len);
        if (co_mld_vap->iv_enable_ecsaie) {
            len = ieee80211_beacon_add_ecsa_ie(buf + total_len, co_mld_vap,
                                               csmode, csa_tbtt);
            if (len < 0) {
                mloie_err("failed to add eCSA IE");
                return -1;
            }
            available_space -= len;
            total_len += len;
        }
    }

    /* Add Max Channel Switch Time IE in per-STA profile, if
     * 1) Impacted link CSA is inprogress, or
     * 2) Impacted link CSA announcement is completed and it is yet to send
     *    the beacon in the new channel.
     */
    if (co_mld_vap->iv_enable_max_ch_sw_time_ie &&
        ((ieee80211_ic_doth_is_set(co_mld_ic) &&
          (co_mld_ic->ic_flags & IEEE80211_F_CHANSWITCH) &&
          IEEE80211_IS_CHAN_VALID(co_mld_ic->ic_chanchange_channel)) ||
         wlan_utils_get_vdev_remaining_channel_switch_time(co_mld_vdev))) {
        if(available_space < sizeof(struct ieee80211_max_chan_switch_time_ie)) {
            mloie_err("Insufficient space to add Max Channel Switch Time IE, required: %zu octets, remaining: %zu octets",
                    sizeof(struct ieee80211_max_chan_switch_time_ie),
                    available_space);
            return -1;
        }

        max_cst_ie = (struct ieee80211_max_chan_switch_time_ie *)(buf + total_len);
        ieee80211_add_max_chan_switch_time(co_mld_vap, (uint8_t *)max_cst_ie,
                                           subtype);
        available_space -= sizeof(*max_cst_ie);
        total_len += sizeof(*max_cst_ie);
    }

    if (!total_len) {
        /* This flag indicates that CSA/eCSA/MCST IEs are present in the per-STA
         * profile. FW removes the per-STA profile on Quiet count 0 if
         * is_other_ie_present flag is set to 0.
         */
    }

    return total_len;
}

/**
 * ieee80211_beacon_add_mlo_quiet_ie() - Function to add MLO Quiet IE.
 * @param buf: Starting position in buffer at which MLO Quiet IE needs to be
 *             added.
 * @param buflen: Space available for adding MLO Quiet IE IE.
 * @param co_mld_vap: Pointer to partner vap structure for which MLO Quiet IE
 *                    needs be added in the per-STA profile.
 */
static int ieee80211_beacon_add_mlo_quiet_ie(uint8_t *buf, size_t buflen,
        struct ieee80211vap *co_mld_vap)
{
    size_t quiet_ie_len = sizeof(struct ieee80211_ath_quiet_ie);

    if (buflen < quiet_ie_len) {
        mloie_err("Insufficient space to add MLO Quiet IE, required: %zu octets, remaining: %zu octets",
                quiet_ie_len, buflen);
        return -1;
    }

    buf = ieee80211_add_quiet(co_mld_vap, co_mld_vap->iv_ic, buf);

    return (int)quiet_ie_len;
}

/**
 * ieee80211_add_basic_multilink_ie_bcnnonmlprbresp_per_STA_profile() - Helper
 * function to add per-STA profile in Basic variant Multi Link element specific
 * to beacons and non-ML probe response. The function may determine that the
 * per-STA profile field is not currently required and thus not fill anything
 * into the frame location passed.
 *
 * @param persta_prof: Starting position in buffer at which per-STA profile
 *                     should be added
 * @param perstaprof_size: Space available for filling per-STA profile
 * @param co_mld_vdev: Pointer to partner vdev structure for which per-STA
 *                     profile needs to be added
 * @param subtype: Frame subtype
 * @param is_mlo_csa: flag to indicate if CSA is triggered on the partner vdev
 * @param is_mlo_quiet: flag to indicate if Quiet IE is triggered on the partner
 *                      vdev.
 * @return Zero or positive integer value giving number of octets added to
 * per-STA profile in the case of success, -1 in the case of failure. Note that
 * a return value of 0 indicates that per-STA profile field is not currently
 * required.
 */
static int ieee80211_add_basic_multilink_ie_bcnnonmlprbresp_per_STA_profile(
        u_int8_t *persta_prof, size_t perstaprof_size,
        struct wlan_objmgr_vdev *co_mld_vdev,
        uint8_t subtype, bool is_mlo_csa, bool is_mlo_quiet)
{
    struct wlan_ml_bv_linfo_perstaprof *perstaprof_fixed = NULL;
    struct wlan_objmgr_pdev *co_mld_pdev = wlan_vdev_get_pdev(co_mld_vdev);
    struct ieee80211vap *co_mld_vap = NULL;
    size_t perstaprof_space_remaining = 0;
    uint8_t linkid = 0;
    uint16_t stacontrol = 0;
    int csa_len = 0;
    int quiet_ie_len = 0;
    uint8_t *staprof = NULL;
    uint8_t *sta_info = NULL;

    if (!co_mld_pdev) {
        mloie_err("co_mld_pdev is NULL");
        return -1;
    }

    co_mld_vap = wlan_vdev_get_vap(co_mld_vdev);
    if (!co_mld_vap) {
        mloie_err("Co-MLD VAP is NULL");
        return -1;
    }

    perstaprof_space_remaining = perstaprof_size;

    if (perstaprof_space_remaining < sizeof(struct wlan_ml_bv_linfo_perstaprof)) {
        mloie_err("Insufficient space in per-STA profile buffer for fixed fields of Per-STA Profile, required: %zu octets, remaining: %zu octets",
                sizeof(struct wlan_ml_bv_linfo_perstaprof),
                perstaprof_space_remaining);
        return -1;
    }

    perstaprof_fixed = (struct wlan_ml_bv_linfo_perstaprof *)persta_prof;
    perstaprof_space_remaining -= sizeof(*perstaprof_fixed);

    qdf_mem_zero(perstaprof_fixed, sizeof(*perstaprof_fixed));

    perstaprof_fixed->subelem_id = WLAN_ML_LINFO_SUBELEMID_PERSTAPROFILE;

    linkid = wlan_vdev_get_link_id(co_mld_vdev);
    IEEE80211_SET_BITS(stacontrol, WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_LINKID,
                       linkid);

    /* Beacon/non-ML probe response shall carry partial profile */
    IEEE80211_SET_BITS(stacontrol,
                       WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_CMPLTPROF, 0);

    /* The MAC address shall not be present in the partial profile */
    IEEE80211_SET_BITS(stacontrol,
                       WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_MACADDRP, 0);

    /* An AP shall not include Beacon Interval and DTIM info in the partial
     * profile.
     */
    IEEE80211_SET_BITS(stacontrol,
                       WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_BCNINTP, 0);
    IEEE80211_SET_BITS(stacontrol,
                       WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_DTIMINFOP, 0);

    /* As of now set NSTR Link Pair Present and NSTR Bitmap size as 0 in the
     * partial profile.
     */
    IEEE80211_SET_BITS(stacontrol,
                       WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_NSTRLINKPRP, 0);
    IEEE80211_SET_BITS(stacontrol,
                       WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_NSTRBMSZ, 0);

    /* End of STA control population */
    perstaprof_fixed->stacontrol = htole16(stacontrol);

    sta_info = (uint8_t *)perstaprof_fixed + sizeof(*perstaprof_fixed);
    /* Set STA info length as 0 */
    *sta_info = 0;
    perstaprof_space_remaining -= sizeof(uint8_t);

    staprof = sta_info + sizeof(uint8_t);

    /* If MLO CSA is triggered, then add CSA, eCSA, Max Channel Switch Time. */
    if (is_mlo_csa ||
            wlan_utils_get_vdev_remaining_channel_switch_time(co_mld_vdev) != 0) {
        csa_len = ieee80211_beacon_add_mlo_csa_ies(staprof, perstaprof_space_remaining,
                co_mld_vdev, subtype);
        if (csa_len < 0) {
            mloie_err("Failed to add CSA IE in Basic variant Multi Link element for Beacon/non-ML probe response");
            return -1;
        }
        perstaprof_space_remaining -= csa_len;
    }

    if (is_mlo_quiet) {
        quiet_ie_len = ieee80211_beacon_add_mlo_quiet_ie(staprof + csa_len,
                perstaprof_space_remaining, co_mld_vap);
        if (quiet_ie_len < 0) {
            mloie_err("Failed to add MLO Quiet IE in Basic variant Multi Link element for Beacon/non-ML probe response");
            return -1;
        }
        perstaprof_space_remaining -= quiet_ie_len;
    }

    perstaprof_fixed->subelem_len = ((perstaprof_size - perstaprof_space_remaining) -
                                     IEEE80211_IE_HDR_LEN);

    return (perstaprof_size - perstaprof_space_remaining);
}

/**
 * ieee80211_add_basic_multilink_ie_bcnnonmlprbresp_linkinfo() - Helper function
 * to add Link Info field in Basic variant Multi Link element specific to
 * beacons and non-ML probe response. The function may determine that the Link
 * Info field is not currently required and thus not fill anything into the
 * frame location passed.
 *
 * Since this is a static helper function and we would like to avoid redundant
 * checks, it is the caller's/caller chain's responsibility to validate the
 * following:
 * - All pointer arguments are non-NULL
 * - vap in ni is non-NULL
 * - vdev object in vap is non-NULL
 * - This helper is called for the right subtype
 *
 * @param linfo: Starting position in buffer at which Link Info should be added
 * @param linfosize: Space in linfo available for filling Link Info
 * @param ni: Pointer to associated node structure
 * @param ic: Pointer to iee80211com
 * @param subtype: Frame subtype
 * @param extractx: Extra framing context. This can be NULL.
 * @param bo: Pointer to beacon offsets
 *
 * @return Zero or positive integer value giving number of octets added to linfo
 * in the case of success, -1 in the case of failure. Note that a return value
 * of 0 indicates that Link Info field is not currently required.
 */
static int ieee80211_add_basic_multilink_ie_bcnnonmlprbresp_linkinfo(
        u_int8_t *linfo, size_t linfosize, struct ieee80211_node *ni,
        struct ieee80211com *ic, u_int8_t subtype,
        struct ieee80211_framing_extractx *extractx,
        struct ieee80211_beacon_offsets *bo)
{

    struct wlan_objmgr_vdev *vdev = NULL;
    struct ieee80211vap *vap = NULL;
    uint16_t perstaprof_len = 0;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS];
    uint16_t vdev_count = 0;
    size_t available_space = linfosize;
    struct wlan_objmgr_vdev *co_mld_vdev = NULL;
    struct ieee80211vap *co_mld_vap = NULL;
    struct ieee80211com *co_mld_ic = NULL;
    uint16_t i = 0;
    uint16_t release_ref_start_idx;

    /* See function documentation about caller's validation responsibilities. */

    vap = ni->ni_vap;
    vdev = vap->vdev_obj;

    /* Determine whether one of the partner APs in the MLD has channel info
     * related IEs, i.e. CSA/ECSA/Max Chan Switch Time element/Quiet element
     * corresponding to quiet intervals other than quiet intervals scheduled
     * to protect restricted TWT service periods/Quiet Channel element.
     */

    ieee80211_mlo_ap_get_beaconing_vdev_list(vdev, &vdev_count, wlan_vdev_list);
    if (!vdev_count) {
        mloie_err("Number of VDEVs under AP-MLD is reported as 0");
        return 0;
    }

    /* In case of errors during initial checks, we need to release all the VDEV
     * references in the VDEV list starting from index 0.
     */
    release_ref_start_idx = 0;

    /* Note: Once API changes to mlo_ap_get_vdev_list() are available to handle
     * array size requirements, remove this condition.
     */
    if (vdev_count > QDF_ARRAY_SIZE(wlan_vdev_list)) {
        mloie_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu, investigate",
                vdev_count, QDF_ARRAY_SIZE(wlan_vdev_list));
        goto retnolinfo_release_refs;
    }

    if (vdev_count < WLAN_UMAC_REPORTED_MLO_IE_PROC_MIN_VDEVS) {
        mloie_debug("Count of VDEVs %u under AP-MLD lesser than expected minimum %u for including reported info",
                vdev_count, WLAN_UMAC_REPORTED_MLO_IE_PROC_MIN_VDEVS);
        goto retnolinfo_release_refs;
    }

    for(i = 0; i < vdev_count; i++) {
        bool is_mlo_csa = false;
        bool is_mlo_quiet = false;

        /* In case of errors during further processing of co MLD VDEV with index
         * i, we need to release all the VDEV references in the VDEV list
         * starting from index i (since references for VDEVs prior to i would
         * already have been released).
         */
        release_ref_start_idx = i;

        co_mld_vdev = wlan_vdev_list[i];
        if (co_mld_vdev == NULL) {
            mloie_err("VDEV in MLD VDEV list is NULL");
            goto retnolinfo_release_refs;
        }

        if (co_mld_vdev == vdev) {
            mlo_release_vdev_ref(wlan_vdev_list[i]);
            continue;
        }

        co_mld_vap = wlan_vdev_get_vap(co_mld_vdev);
        if (co_mld_vap == NULL) {
            mloie_err("Co-MLD VAP is NULL");
            goto retnolinfo_release_refs;
        }

        co_mld_ic = co_mld_vap->iv_ic;
        if (co_mld_ic == NULL) {
            mloie_err("Co-MLD IC is NULL");
            goto retnolinfo_release_refs;
        }

        is_mlo_csa = (co_mld_ic->ic_flags & IEEE80211_F_CHANSWITCH) ? 1 : 0;
        is_mlo_quiet = ieee80211_is_quiet_enabled(co_mld_vap, subtype);

        if ((!is_mlo_csa && !is_mlo_quiet) &&
                (wlan_utils_get_vdev_remaining_channel_switch_time(co_mld_vdev) == 0)) {
            mlo_release_vdev_ref(wlan_vdev_list[i]);
            continue;
        }

        perstaprof_len = ieee80211_add_basic_multilink_ie_bcnnonmlprbresp_per_STA_profile(
                linfo, available_space, co_mld_vdev, subtype, is_mlo_csa,
                is_mlo_quiet);

        if (perstaprof_len < 0) {
            mloie_err("Failed to add Per STA profile IE in Basic variant Multi Link element for Beacon/non-ML probe response");
            goto retnolinfo_release_refs;
        }

        available_space -= perstaprof_len;
        linfo += perstaprof_len;

        mlo_release_vdev_ref(wlan_vdev_list[i]);
    }

    return (linfosize - available_space);

retnolinfo_release_refs:
    if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                    QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count,
                    release_ref_start_idx)) != QDF_STATUS_SUCCESS) {
        /* This should not occur. If this occurs, would indicate larger
         * issues.
         */
        mloie_err("Error while trying to release VDEV references - investigate.");
        return -1;
    }

    return 0;
}

/**
 * ieee80211_add_probereq_multilink_ie_fields() - Helper function to populate
 * Probe Request variant of Multi-Link Information Element, and corresponding
 * Fragment elements if applicable
 *
 * Since this is a static helper function and we would like to avoid redundant
 * checks, it is the caller's/caller chain's responsibility to validate the
 * following:
 * - All pointer arguments are non-NULL (with the potential exception of
 *   containerfrm)
 * - vap in ni is non-NULL
 * - vdev object in vap is non-NULL
 * - This helper is called for the right subtype
 * - For subtye IEEE80211_FC0_SUBTYPE_AUTH, necessary conditions as stipulated
 *   in the standard are satisfied.
 *
 * @param mlbuf: Buffer into which which the element(s) should be added
 * @param mlbufsize: Size of buffer mlbuf
 * @param ni: Pointer to associated node structure
 * @param ic: Pointer to iee80211com
 * @param mld_id: MLD ID of the target MLD device in the AP
 * @param link_id: Link ID of the target VAPs of the MLD device in the AP
 * @param extractx: Extra framing context. This can be NULL.
 *
 * @return Positive integer value giving number of octets added to mlbuf in the
 * case of success, -1 in the case of failure
 */
static int ieee80211_add_probereq_multilink_ie_fields(u_int8_t *mlbuf,
        size_t mlbufsize,
        struct ieee80211_node *ni,
        struct ieee80211com *ic, u_int8_t mld_id, u_int8_t *link_id,
        struct ieee80211_framing_extractx *extractx)
{
    struct wlan_ie_multilink *mlie_fixed = NULL;
    size_t mlbufspace_remaining = 0;
    struct ieee80211vap *vap = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    u_int16_t mlcontrol = 0;
    u_int16_t presencebitmap = 0;
    u_int8_t *commoninfo = NULL;
    qdf_size_t commoninfo_len = 0;
    size_t populatedlen = 0;
    u_int8_t *linkinfopos = NULL;
    u_int8_t *perstaprof;
    size_t perstaprof_maxsize = 0;
    /* Fixed portion of per-STA profile */
    struct wlan_ml_prv_linfo_perstaprof *perstaprof_fixed = NULL;
    size_t perstaproflen = 0;
    u_int16_t stacontrol = 0;
    u_int8_t *stainfo = NULL;
    u_int8_t stainfolen = 0;
    size_t linfospace_remaining = 0;
    int i = 0;

    /* See function documentation about caller's validation responsibilities. */

    vap = ni->ni_vap;
    vdev = vap->vdev_obj;

    mlbufspace_remaining = mlbufsize;

    if (mlbufspace_remaining < sizeof(struct wlan_ie_multilink)) {
        mloie_err("Insufficient space in ML IE buffer for IE headers and control, required: %zu octets, remaining: %zu octets",
                sizeof(struct wlan_ie_multilink), mlbufspace_remaining);
        return -1;
    }

    /*
     * Populate Variant, Presence Bitmap sub-field in Multi-Link Control field
     * and corresponding Common Info sub-fields, where applicable.
     */
    mlie_fixed = (struct wlan_ie_multilink *)mlbuf;
    mlbufspace_remaining -= sizeof(*mlie_fixed);

    qdf_mem_zero(mlie_fixed, sizeof(*mlie_fixed));

    mlie_fixed->elem_id = WLAN_ELEMID_EXTN_ELEM;
    mlie_fixed->elem_id_ext = WLAN_EXTN_ELEMID_MULTI_LINK;

    IEEE80211_SET_BITS(mlcontrol, WLAN_ML_CTRL_TYPE, WLAN_ML_VARIANT_PROBEREQ);

    commoninfo = mlbuf + sizeof(*mlie_fixed);

    /*
     * Common Info Length in Common Info
     *
     * Note: At this stage, we only verify whether we have sufficient space in
     * the buffer for this subfield, and start calculating the length. The
     * calculated value will be populated into the subfield later.
     */
    if (mlbufspace_remaining < WLAN_ML_PRV_CINFO_LENGTH_SIZE) {
        mloie_err("Insufficient space in ML IE buffer for Common Info Len, required: %u octets, remaining: %zu octets",
            WLAN_ML_PRV_CINFO_LENGTH_SIZE, mlbufspace_remaining);
        return -1;
    }

    commoninfo_len += WLAN_ML_PRV_CINFO_LENGTH_SIZE;
    mlbufspace_remaining -= WLAN_ML_PRV_CINFO_LENGTH_SIZE;

    if ( mld_id != ML_PROBE_MLDID_INVALID) {
        presencebitmap |= WLAN_ML_PRV_CTRL_PBM_MLDID_P;

        memcpy(commoninfo + commoninfo_len, &mld_id, sizeof(mld_id));
        commoninfo_len += sizeof(mld_id);
        mlbufspace_remaining -= sizeof(mld_id);
    }
    /* Update Common Info Length */
    if (commoninfo_len > WLAN_ML_PRV_CINFO_LENGTH_MAX) {
            mloie_err("Calculated Common Info len %zu octets higher than max of %u octets",
                commoninfo_len, WLAN_ML_PRV_CINFO_LENGTH_MAX);
            return -1;
    }

    *commoninfo = commoninfo_len;

    IEEE80211_SET_BITS(mlcontrol, WLAN_ML_CTRL_PBM, presencebitmap);

    mlie_fixed->mlcontrol = htole16(mlcontrol);

    linfospace_remaining = mlbufspace_remaining;

    /* Link Info */
    if (link_id[0] != ML_PROBE_LINKID_INVALID) {
        linkinfopos = mlbuf + (mlbufsize - mlbufspace_remaining);
        perstaprof = linkinfopos;

        while(i < MLD_MAX_LINKS && link_id[i] != ML_PROBE_LINKID_INVALID) {
            perstaprof_maxsize = linfospace_remaining;
            perstaprof_fixed = (struct wlan_ml_prv_linfo_perstaprof *)perstaprof;
            linfospace_remaining -= sizeof(*perstaprof_fixed);

            perstaproflen = sizeof(*perstaprof_fixed);

            perstaprof_fixed->subelem_id =
                WLAN_ML_LINFO_SUBELEMID_PERSTAPROFILE;

            stainfo = (u_int8_t *)perstaprof_fixed + sizeof(*perstaprof_fixed);

            IEEE80211_SET_BITS(stacontrol,
                           WLAN_ML_PRV_LINFO_PERSTAPROF_STACTRL_LINKID, link_id[i]);

            /* Support sending only complete profile for now */
            IEEE80211_SET_BITS(stacontrol,
                           WLAN_ML_PRV_LINFO_PERSTAPROF_STACTRL_CMPLTPROF, 1);

            /* As its a complete profile, STA profile in probe request is not needed */

            perstaprof_fixed->stacontrol = htole16(stacontrol);
            perstaproflen += stainfolen;

            perstaprof_fixed->subelem_len = perstaproflen -
                    IEEE80211_SUBELEM_HDR_LEN;
            perstaprof += perstaproflen;
	    i++;
        }
    }
    populatedlen = mlbufsize - linfospace_remaining;
    mlie_fixed->elem_len = populatedlen - IEEE80211_IE_HDR_LEN;

    return populatedlen;
}

/**
 * ieee80211_add_basic_multilink_ie_fields() - Helper function to populate Basic
 * variant Multi-Link Information Element, and corresponding Fragment elements
 * if applicable
 *
 * Since this is a static helper function and we would like to avoid redundant
 * checks, it is the caller's/caller chain's responsibility to validate the
 * following:
 * - All pointer arguments are non-NULL (with the potential exception of
 *   containerfrm)
 * - vap in ni is non-NULL
 * - vdev object in vap is non-NULL
 * - This helper is called for the right subtype
 * - For subtye IEEE80211_FC0_SUBTYPE_AUTH, necessary conditions as stipulated
 *   in the standard are satisfied.
 *
 * @param mlbuf: Buffer into which which the element(s) should be added
 * @param mlbufsize: Size of buffer mlbuf
 * @param containerfrm: Frame which will contain the Multi-Link element, and
 * which already has other IEs required for forming per-STA profile entries in
 * the Multi-Link element using inheritance rules (if applicable). Can be NULL
 * if the caller/caller chain knows this is not applicable and chooses to
 * optimize accordingly (but the function will return an error if the
 * caller/caller chain makes this determination incorrectly).
 * @param containerfrmsize: Size of containerfrm. Will be ignored if
 * containerfrm is not required or NULL.
 * @param ni: Pointer to associated node structure
 * @param ic: Pointer to iee80211com
 * @param subtype: Frame subtype
 * @param extractx: Extra framing context. This can be NULL.
 * @param bo: Pointer to beacon offsets
 * @param rs: Pointer to receive status information associated with packet
 *
 * @return Positive integer value giving number of octets added to mlbuf in the
 * case of success, -1 in the case of failure
 */
static int ieee80211_add_basic_multilink_ie_fields(u_int8_t *mlbuf,
        size_t mlbufsize, u_int8_t *containerfrm, size_t containerfrmsize,
        struct ieee80211_node *ni,
        struct ieee80211com *ic, u_int8_t subtype,
        struct ieee80211_framing_extractx *extractx,
        struct ieee80211_beacon_offsets *bo, struct ieee80211_rx_status *rs)
{
    struct wlan_ie_multilink *mlie_fixed = NULL;
    size_t mlbufspace_remaining = 0;
    struct ieee80211vap *vap = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    u_int16_t mlcontrol = 0;
    u_int16_t presencebitmap = 0;
    u_int8_t *commoninfo = NULL;
    qdf_size_t commoninfo_len = 0;
    struct qdf_mac_addr *mld_addr = NULL;
    u_int8_t linkid = 0;
    u_int8_t linkidinfo = 0;
    u_int8_t bssparamchangecnt = 0;
    u_int16_t emlcapinfo;
    bool is_eml_enabled = false;
    bool exclude_eml_cap = false;
    bool msd_cap_override = false;
    bool is_msd_present = false;
    bool exclude_msd_cap = false;
    u_int8_t *linkinfopos = NULL;
    int linkinfo_len = 0;
    size_t populatedlen = 0;
    bool is_frag_required;
    qdf_size_t required_fragbuff_size;
    size_t populatedlen_fragseq = 0;
    QDF_STATUS ret;
    uint8_t t2lm_supported = 0;
    uint16_t mldcapinfo;
    uint16_t msdcapinfo;
    uint16_t extmldcapinfo;
    uint16_t mld_vdev_cnt = 0;
    bool htc_he_supported = false;
    bool aar_supported = false;
    bool srs_supported = false;
    uint8_t str_freq_sep = 0;
    uint8_t mld_id = 0xff;
    struct wlan_mlo_eml_cap phy_eml_cap;
    struct wlan_mlo_mld_cap phy_mld_cap;
    struct wlan_mlo_msd_cap phy_msd_cap;
    struct wlan_objmgr_vdev *mlo_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS];
    struct mlo_mgr_context *mlo_mgr_ctx = wlan_objmgr_get_mlo_ctx();
    struct wlan_mlo_dev_context *ml_dev;

    /* See function documentation about caller's validation responsibilities. */
    if (extractx && extractx->non_tx_vap) {
        vap = extractx->non_tx_vap;
    } else {
        vap = ni->ni_vap;
    }
    vdev = vap->vdev_obj;

    mlbufspace_remaining = mlbufsize;

    if (mlbufspace_remaining < sizeof(struct wlan_ie_multilink)) {
        mloie_err("Insufficient space in ML IE buffer for IE headers and control, required: %zu octets, remaining: %zu octets",
                sizeof(struct wlan_ie_multilink), mlbufspace_remaining);
        return -1;
    }

    ml_dev = vdev->mlo_dev_ctx;

    if (!ml_dev) {
        mloie_err("ML DEV CTX is NULL");
        return -1;
    }

    /*
     * Populate Variant, Presence Bitmap sub-field in Multi-Link Control field
     * and corresponding Common Info sub-fields, where applicable.
     */
    mlie_fixed = (struct wlan_ie_multilink *)mlbuf;
    mlbufspace_remaining -= sizeof(*mlie_fixed);

    qdf_mem_zero(mlie_fixed, sizeof(*mlie_fixed));

    mlie_fixed->elem_id = WLAN_ELEMID_EXTN_ELEM;
    mlie_fixed->elem_id_ext = WLAN_EXTN_ELEMID_MULTI_LINK;

    IEEE80211_SET_BITS(mlcontrol, WLAN_ML_CTRL_TYPE, WLAN_ML_VARIANT_BASIC);

    commoninfo = mlbuf + sizeof(*mlie_fixed);

    /*
     * Common Info Length in Common Info
     *
     * Note: At this stage, we only verify whether we have sufficient space in
     * the buffer for this subfield, and start calculating the length. The
     * calculated value will be populated into the subfield later.
     */
    if (mlbufspace_remaining < WLAN_ML_BV_CINFO_LENGTH_SIZE) {
        mloie_err("Insufficient space in ML IE buffer for Common Info Len, required: %u octets, remaining: %zu octets",
            WLAN_ML_BV_CINFO_LENGTH_SIZE, mlbufspace_remaining);
        return -1;
    }

    commoninfo_len += WLAN_ML_BV_CINFO_LENGTH_SIZE;
    mlbufspace_remaining -= WLAN_ML_BV_CINFO_LENGTH_SIZE;

    /* MLD Address in Common Info */
    if (mlbufspace_remaining < QDF_MAC_ADDR_SIZE) {
        mloie_err("Insufficient space in ML IE buffer for MAC addr in Common Info, required: %u octets, remaining: %zu octets",
            QDF_MAC_ADDR_SIZE, mlbufspace_remaining);
        return -1;
    }

    wlan_vdev_obj_lock(vdev);
    mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev);
    IEEE80211_ADDR_COPY(commoninfo + commoninfo_len, mld_addr->bytes);
    wlan_vdev_obj_unlock(vdev);

    commoninfo_len += QDF_MAC_ADDR_SIZE;
    mlbufspace_remaining -= QDF_MAC_ADDR_SIZE;

    /* Link ID and BSS Parameters Change Count in Common Info */
    if ((subtype == IEEE80211_FC0_SUBTYPE_BEACON) ||
            (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP) ||
            (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
            (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP)) {
        if (mlbufspace_remaining < sizeof(linkidinfo)) {
            mloie_err("Insufficient space in ML IE buffer for Link ID info in Common Info, required: %zu octets, remaining: %zu octets",
                sizeof(linkidinfo), mlbufspace_remaining);
            return -1;
        }

        linkid = wlan_vdev_get_link_id(vdev);
        presencebitmap |= WLAN_ML_BV_CTRL_PBM_LINKIDINFO_P;
        IEEE80211_SET_BITS(linkidinfo,WLAN_ML_BV_CINFO_LINKIDINFO_LINKID,
                linkid);
        memcpy(commoninfo + commoninfo_len, &linkidinfo, sizeof(linkidinfo));
        commoninfo_len += sizeof(linkidinfo);
        mlbufspace_remaining -= sizeof(linkidinfo);

        /*
         * BPCC will be updated by FW as part of Critical Update framework for beacon.
         * For probe, assoc and reassoc response frame update the latest copy of BPCC
         * values received from FW.
         */
        if (mlbufspace_remaining < sizeof(bssparamchangecnt)) {
            mloie_err("Insufficient space in ML IE buffer for BSS param change count in Common Info, required: %zu octets, remaining: %zu octets",
                sizeof(bssparamchangecnt), mlbufspace_remaining);
            return -1;
        }

        if ((rs && (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP)) ||
            (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
            (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP)) {
            bssparamchangecnt = iee80211_cu_get_bssparamchangecnt(vap, rs);
        }
        presencebitmap |= WLAN_ML_BV_CTRL_PBM_BSSPARAMCHANGECNT_P;
        qdf_mem_copy(commoninfo + commoninfo_len, &bssparamchangecnt,
                sizeof(bssparamchangecnt));
        commoninfo_len += sizeof(bssparamchangecnt);
        mlbufspace_remaining -= sizeof(bssparamchangecnt);
    }

    /* D3.0: Add MSD Capabilities */
    if (wlan_vdev_mlme_is_mlo_ap(vdev)) {
        if ((subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
            (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP) ||
            ((subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP) &&
                (extractx != NULL) && (extractx->fectx_is_ml_proberesp))) {

            if (mlbufspace_remaining < sizeof(msdcapinfo)) {
                mloie_err("Insufficient space in ML IE buffer for MSD capabilities, required: %zu octets, remaining: %zu octets",
                           sizeof(msdcapinfo), mlbufspace_remaining);
                return -1;
            }
            if (ic->ic_get_msd_info_override_enable &&
                (QDF_STATUS_SUCCESS == ic->ic_get_msd_info_override_enable(ic, &msd_cap_override)) &&
                 msd_cap_override) {
                msd_cap_override = true;
            }

            msdcapinfo = 0;
            if (msd_cap_override) {
                if (!ic->ic_get_msd_info_present_override ||
                    (QDF_STATUS_SUCCESS != ic->ic_get_msd_info_present_override(ic, &is_msd_present) ||
                     !is_msd_present)) {
                    exclude_msd_cap = true;
                }
                else {
                    if (ic->ic_get_msd_info_value_override &&
                        (QDF_STATUS_SUCCESS == ic->ic_get_msd_info_value_override(ic, &msdcapinfo))) {
                        presencebitmap |= WLAN_ML_BV_CTRL_PBM_MEDIUMSYNCDELAYINFO_P;
                    }
                    else {
                        exclude_msd_cap = true;
                        mloie_err("Unable to get MSD configurations from INI");
                    }
                }
            }
            else {
                 if (ic->ic_get_msd_phy_cap &&
                        ic->ic_get_msd_phy_cap(ic, &phy_msd_cap) == QDF_STATUS_SUCCESS) {
                     presencebitmap |= WLAN_ML_BV_CTRL_PBM_MEDIUMSYNCDELAYINFO_P;
                     IEEE80211_SET_BITS(msdcapinfo,WLAN_ML_BV_CINFO_MEDMSYNCDELAYINFO_DURATION,
                                        phy_msd_cap.medium_sync_duration);
                     IEEE80211_SET_BITS(msdcapinfo,WLAN_ML_BV_CINFO_MEDMSYNCDELAYINFO_OFDMEDTHRESH,
                                        phy_msd_cap.medium_sync_ofdm_ed_thresh);
                     IEEE80211_SET_BITS(msdcapinfo,WLAN_ML_BV_CINFO_MEDMSYNCDELAYINFO_MAXTXOPS,
                                        phy_msd_cap.medium_sync_max_txop_num);
                 }
                 else {
                      exclude_msd_cap = true;
                      mloie_err("Unable to get MSD configurations from FW");
                 }
            }
            if (!exclude_msd_cap) {
                msdcapinfo = qdf_cpu_to_le16(msdcapinfo);
                memcpy(commoninfo + commoninfo_len, &msdcapinfo, sizeof(msdcapinfo));
                commoninfo_len += sizeof(msdcapinfo);
                mlbufspace_remaining -= sizeof(msdcapinfo);
            }
        }
    }

    /* EML Capabilities in Common Info  */
    if (wlan_vdev_mlme_is_mlo_ap(vdev) &&
        (!ic->ic_get_enable_eml_cap ||
         (QDF_STATUS_SUCCESS != ic->ic_get_enable_eml_cap(ic, &is_eml_enabled)))) {
        mloie_err_rl("Failed to get EML configuration\n");
    }

    /* Handle disable EML requested by CLI commad */
    mlo_mgr_ctx = wlan_objmgr_get_mlo_ctx();
    if (mlo_mgr_ctx && mlo_mgr_ctx->disable_eml && is_eml_enabled)
        is_eml_enabled = false;

    /* WFA requirement: In SLO exclude EML advertisement in ML ie */
    if (is_eml_enabled && ic->ic_disable_eml_in_slo && wlan_vdev_mlme_is_mlo_ap(vdev)) {
        if (wlan_mlo_ap_get_active_links(vdev) == 1) {
            exclude_eml_cap = true;
            mloie_info("Exclude EMLSR in ML IE for Single Link Operation\n");
        }
    }

    if (!(subtype == IEEE80211_FC0_SUBTYPE_AUTH) && is_eml_enabled && !exclude_eml_cap &&
        (ic->ic_get_eml_phy_cap(ic, &phy_eml_cap) == QDF_STATUS_SUCCESS) &&
        (phy_eml_cap.emlsr_supp || phy_eml_cap.emlmr_supp)) {

        if (mlbufspace_remaining < sizeof(emlcapinfo)) {
            mloie_err_rl("no space for EML CAP");
            return -1;
        }

        presencebitmap |= WLAN_ML_BV_CTRL_PBM_EMLCAP_P;
        emlcapinfo = 0;

        IEEE80211_SET_BITS(emlcapinfo, WLAN_ML_BV_CINFO_EMLCAP_EMLSRSUPPORT,
                           phy_eml_cap.emlsr_supp);
        IEEE80211_SET_BITS(emlcapinfo, WLAN_ML_BV_CINFO_EMLCAP_EMLMRSUPPORT,
                           phy_eml_cap.emlmr_supp);
        /* AP-MLD should advertise emlsr_padding_delay as 0 */
        IEEE80211_SET_BITS(emlcapinfo, WLAN_ML_BV_CINFO_EMLCAP_EMLSR_PADDINGDELAY,
                           WLAN_ML_BV_CINFO_EMLCAP_EMLSRDELAY_0US);
        /* AP-MLD should advertise emlsr_transition_delay as 0 */
        IEEE80211_SET_BITS(emlcapinfo, WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY,
                           WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_0US);
        /* AP-MLD should advertise emlmr_padding_delay as 0 */
        IEEE80211_SET_BITS(emlcapinfo, WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY,
                           WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY_0US);
        IEEE80211_SET_BITS(emlcapinfo, WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT,
                           phy_eml_cap.trans_timeout);

        emlcapinfo = qdf_cpu_to_le16(emlcapinfo);
        memcpy(commoninfo + commoninfo_len, &emlcapinfo,
               sizeof(emlcapinfo));
        commoninfo_len += sizeof(emlcapinfo);
        mlbufspace_remaining -= sizeof(emlcapinfo);
    }

    if ((subtype == IEEE80211_FC0_SUBTYPE_BEACON) ||
        (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP) ||
        (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_REQ) ||
        (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ) ||
        (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
        (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP)) {
        if (mlbufspace_remaining < sizeof(mldcapinfo)) {
            mloie_err("Insufficient space in ML IE buffer for T2LM negotiation supported, required: %zu octets, remaining: %zu octets",
                    sizeof(mldcapinfo), mlbufspace_remaining);
            return -1;
        }

        presencebitmap |= WLAN_ML_BV_CTRL_PBM_MLDCAPANDOP_P;
        mldcapinfo = 0;

        /* Below MLD Cap fields are common for both AP and non-AP MLDs
         * WLAN_ML_BV_CINFO_MLDCAPANDOP_MAXSIMULLINKS
         * WLAN_ML_BV_CINFO_MLDCAPANDOP_SRSSUPPORT
         * WLAN_ML_BV_CINFO_MLDCAPANDOP_TIDTOLINKMAPNEGSUPPORT
         */
        if ((subtype == IEEE80211_FC0_SUBTYPE_ASSOC_REQ) ||
            (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ)) {
            /* Get vdev count for non-AP MLDs */
            mlo_get_ml_vdev_list(vdev, &mld_vdev_cnt, mlo_vdev_list);
        } else {
            /* Get vdev count for AP MLDs */
            ieee80211_mlo_ap_get_beaconing_vdev_list(vdev, &mld_vdev_cnt, mlo_vdev_list);
        }

        if (!mld_vdev_cnt)
            mloie_err("Number of VDEVs under MLD is reported as 0");
        else {
            IEEE80211_SET_BITS(mldcapinfo,
                               WLAN_ML_BV_CINFO_MLDCAPANDOP_MAXSIMULLINKS,
                               (mld_vdev_cnt -1));
            if ((ieee80211_mlo_release_vdev_list_refs(mlo_vdev_list,
                                                      QDF_ARRAY_SIZE(mlo_vdev_list),
                                                      mld_vdev_cnt, 0)) != QDF_STATUS_SUCCESS)
                mloie_err("Error while trying to release VDEV references - investigate. This could lead to system issues.");
        }

        if (ic->ic_get_mld_phy_cap(ic, &phy_mld_cap) == QDF_STATUS_SUCCESS) {
            srs_supported = phy_mld_cap.srs_support;
            aar_supported = phy_mld_cap.aar_support;
            str_freq_sep = phy_mld_cap.str_freq_sep;
        }

        if (!srs_supported)
            IEEE80211_SET_BITS(mldcapinfo,
                               WLAN_ML_BV_CINFO_MLDCAPANDOP_SRSSUPPORT,
                               0);
        else
            IEEE80211_SET_BITS(mldcapinfo,
                               WLAN_ML_BV_CINFO_MLDCAPANDOP_SRSSUPPORT,
                               1U);

        ret = ieee80211_get_t2lm_enable(ic, &t2lm_supported);
        if (QDF_IS_STATUS_SUCCESS(ret))
            IEEE80211_SET_BITS(mldcapinfo,
                               WLAN_ML_BV_CINFO_MLDCAPANDOP_TIDTOLINKMAPNEGSUPPORT,
                               t2lm_supported);

        if (wlan_vdev_mlme_is_mlo_ap(vdev)) {
            /* AP-MLD Type: AP-MLD is not NSTR */
            IEEE80211_SET_BITS(mldcapinfo,
                               WLAN_ML_BV_CINFO_MLDCAPANDOP_STRFREQSEPARATION,
                               0);
            htc_he_supported = !!wlan_get_param(vap,
                                                IEEE80211_CONFIG_HE_HT_CTRL);
            if (!aar_supported || !htc_he_supported)
                IEEE80211_SET_BITS(mldcapinfo,
                                   WLAN_ML_BV_CINFO_MLDCAPANDOP_AARSUPPORT,
                                   0);
            else
                IEEE80211_SET_BITS(mldcapinfo,
                                   WLAN_ML_BV_CINFO_MLDCAPANDOP_AARSUPPORT,
                                   1U);
        } else {
            /* Frequency separation for STR for non-AP MLD */
            IEEE80211_SET_BITS(mldcapinfo,
                               WLAN_ML_BV_CINFO_MLDCAPANDOP_STRFREQSEPARATION,
                               str_freq_sep);
            /* AAR Support is reserved for non-AP MLD */
            IEEE80211_SET_BITS(mldcapinfo,
                               WLAN_ML_BV_CINFO_MLDCAPANDOP_AARSUPPORT,
                               0);
        }
        mldcapinfo = qdf_cpu_to_le16(mldcapinfo);
        memcpy(commoninfo + commoninfo_len, &mldcapinfo, sizeof(mldcapinfo));
        commoninfo_len += sizeof(mldcapinfo);
        mlbufspace_remaining -= sizeof(mldcapinfo);
    }

    /* For the ML probe request to NON-TX Vap of MBSSID group set the MLDID
       same as the BSSID Index of NON-TX vap */
    if ((subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP)) {
        if (extractx && extractx->fectx_is_ml_proberesp && extractx->non_tx_vap) {
            presencebitmap |= WLAN_ML_BV_CTRL_PBM_MLDID_P;
            /* vap is alredy pointing to non-tx vap get the mbssid index of the non-tx vap */
            mld_id = vap->vdev_mlme->mgmt.mbss_11ax.profile_idx;
            if( mld_id != ML_PROBE_MLDID_INVALID) {
                if (mlbufspace_remaining < sizeof(mld_id)) {
                    mloie_err("Insufficient space in ML IE buffer for MLD ID, required: %zu octets, remaining: %zu octets",
                            sizeof(mld_id), mlbufspace_remaining);
                    return -1;
                }
                qdf_mem_copy(commoninfo + commoninfo_len, &mld_id, sizeof(mld_id));
                commoninfo_len += sizeof(mld_id);
                mlbufspace_remaining -= sizeof(mld_id);
            }
        }
    }

    /* Update Common Info Length */
    if (commoninfo_len > WLAN_ML_BV_CINFO_LENGTH_MAX) {
            mloie_err("Calculated Common Info len %zu octets higher than max of %u octets",
                commoninfo_len, WLAN_ML_BV_CINFO_LENGTH_MAX);
            return -1;
    }

    /* Fill Extended MLD Capabilities and Operations Subfield of ML IE */

    if (ml_dev->mlo_extmld_cap_advertisement == true &&
            ((subtype == IEEE80211_FC0_SUBTYPE_BEACON) ||
            (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP) ||
            (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
            (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP))) {
        if (mlbufspace_remaining < sizeof(extmldcapinfo)) {
            mloie_err("Insufficient space in MLIE buffer for extmldcapinfo, required: %zu octets, remaining %zu octets",
                    sizeof(extmldcapinfo), mlbufspace_remaining);
            return -1;
        }

        extmldcapinfo = 0;

        presencebitmap |= WLAN_ML_BV_CTRL_PBM_EXT_MLDCAPANDOP_P;

        /* Operation Update param only supported for non-AP MLD.
         * For AP-MLD spec mandates to advertise 0
         */
        if (wlan_vdev_mlme_is_mlo_ap(vdev)) {
            IEEE80211_SET_BITS(extmldcapinfo,
                    WLAN_ML_BV_CINFO_EXTMLDCAPINFO_OP_PARAM_SUPP, 0);
        }

        if (wlan_vdev_mlme_is_mlo_ap(vdev)) {
            /* Set Maximum Recommended Simultaneous Links value in Extended
             * MLD CAP and OP subfields only for BEACON and BCAST PROBE RSP
             * frame types as per D4.0 spec.
             */
            if((subtype == IEEE80211_FC0_SUBTYPE_BEACON) ||
                    (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP &&
                    (vap->iv_bpr_enable ||
                    (extractx != NULL && extractx->is_bcast_probe_rsp)))) {
                if (!mld_vdev_cnt)
                    mloie_err("Number of VDEVs under MLD is reported as 0");
                else {
                    IEEE80211_SET_BITS(extmldcapinfo,
                            WLAN_ML_BV_CINFO_EXTMLDCAPINFO_RECOM_MAX_SIMULT_LINKS,
                            MIN(vdev->mlo_dev_ctx->mlo_max_recom_simult_links,
                                mld_vdev_cnt));
                    mloie_debug("EXT_MLD_CAPINFO_OP param is set to value: 0x%x for subtype: %d",
                            extmldcapinfo, subtype);
                    mloie_debug("Processed MIN value for max_active_links: %d",
                            (MIN(vdev->mlo_dev_ctx->mlo_max_recom_simult_links,
                                 mld_vdev_cnt)));
                }
            } else {
                /* Set the field to reserved for non-AP MLD and other frames */
                IEEE80211_SET_BITS(extmldcapinfo,
                        WLAN_ML_BV_CINFO_EXTMLDCAPINFO_RECOM_MAX_SIMULT_LINKS,
                        1);
                mloie_debug("EXT_MLD_CAPINFO_OP param for Non-Beacon/Probe frames is set to value: %d for subtype: %d",
                        extmldcapinfo, subtype);
            }
        }

        extmldcapinfo = qdf_cpu_to_le16(extmldcapinfo);
        memcpy(commoninfo + commoninfo_len, &extmldcapinfo, sizeof(extmldcapinfo));
        commoninfo_len += sizeof(extmldcapinfo);
        mlbufspace_remaining -= sizeof(extmldcapinfo);
    } else {
        mlo_debug("EXTMLDCAPOP EXcluded for frame subtype:0x%x", subtype);
    }

    *commoninfo = commoninfo_len;

    IEEE80211_SET_BITS(mlcontrol, WLAN_ML_CTRL_PBM, presencebitmap);

    mlie_fixed->mlcontrol = htole16(mlcontrol);

    /* Link Info */

    linkinfopos = mlbuf + (mlbufsize - mlbufspace_remaining);

    /* Note: Support for adding Link Info for ML probe response will be added
     * here later once support for this (or an alternative) stabilizes in the
     * standard.
     */

    if (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP) {
        /* When CSA/Quiet is enabled, FW adds the link info in the beacon
         * frames. Hence, call the below function to add link info only for
         * Probe response frame.
         */
        if ((extractx == NULL) || !(extractx->fectx_is_ml_proberesp) ||
            (extractx->ml_preq_info.is_mld_id_valid &&
            (extractx->ml_preq_info.mlid == ML_PROBE_MLDID_INVALID))) {
                linkinfo_len =
                    ieee80211_add_basic_multilink_ie_bcnnonmlprbresp_linkinfo(
                        linkinfopos, mlbufspace_remaining, ni, ic, subtype, extractx,
                bo);
        } else {
                linkinfo_len = ieee80211_add_basic_multilink_ie_assocreqresp_proberesp_linkinfo(
                    linkinfopos, mlbufspace_remaining, containerfrm, containerfrmsize, ni,
                        ic, subtype, extractx, bo, rs);
        }
        if (linkinfo_len < 0) {
            mloie_err("Failed to add Link Info field in Basic variant Multi Link element for Beacon/non-ML probe response");
            return -1;
        } else {
            /* The helper function has the responsibility of validating that
             * there is enough space remaining in the ML IE buffer for Link
             * Info.
             */
            mlbufspace_remaining -= linkinfo_len;
        }
    } else if ((subtype == IEEE80211_FC0_SUBTYPE_ASSOC_REQ) ||
        (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ) ||
        (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
        (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP)) {
        if (containerfrm == NULL) {
            mloie_err("containerfrm is NULL but is required for adding Link Info field in Basic variant Multi Link element for (Re) Assoc req/resp");
            return -1;
        }

        if (containerfrmsize == 0) {
            mloie_err("containerfrmsize is 0 but containerfrm is required for adding Link Info field in Basic variant Multi Link element for (Re) Assoc req/resp");
            return -1;
        }

        linkinfo_len = ieee80211_add_basic_multilink_ie_assocreqresp_proberesp_linkinfo(
                linkinfopos, mlbufspace_remaining, containerfrm, containerfrmsize, ni,
                ic, subtype, extractx, bo, rs);

        if (linkinfo_len < 0) {
            mloie_err("Failed to add Link Info field in Basic variant Multi Link element for (Re) Assoc req/resp");
            return -1;
        } else {
            /* The helper function has the responsibility of validating that
             * there is enough space remaining in the ML IE buffer for Link
             * Info.
             */
            mlbufspace_remaining -= linkinfo_len;
        }
    }

    populatedlen = mlbufsize - mlbufspace_remaining;

    ret = wlan_get_elem_fragseq_requirements(WLAN_ELEMID_EXTN_ELEM,
                                             populatedlen -
                                                IEEE80211_IE_HDR_LEN - 1,
                                             &is_frag_required,
                                             &required_fragbuff_size);
    if (QDF_IS_STATUS_ERROR(ret))
        return -1;

    if (is_frag_required) {
        if (required_fragbuff_size > mlbufsize) {
            mloie_err("Insufficient space for creating Multi Link element fragment sequence. Required: %zu octets, available: %zu octets",
                      required_fragbuff_size, mlbufsize);
            return -1;
        }

        /* The utility will take care of populating element ID, element length,
         * element ID extension (where applicable) in the leading fragment and
         * other fragments.
         */
        ret = wlan_create_elem_fragseq(true,
                                       WLAN_ELEMID_EXTN_ELEM,
                                       WLAN_EXTN_ELEMID_MULTI_LINK,
                                       mlbuf,
                                       mlbufsize,
                                       populatedlen -  IEEE80211_IE_HDR_LEN - 1,
                                       NULL,
                                       0,
                                       &populatedlen_fragseq);

        if (QDF_IS_STATUS_ERROR(ret))
            return -1;
        else
            return populatedlen_fragseq;
    } else {
        mlie_fixed->elem_len = populatedlen - IEEE80211_IE_HDR_LEN;
        return populatedlen;
    }
}

/**
 * ieee80211_add_reconfig_multilink_ie_linkinfo() - Helper function to add Link
 * Info field in Reconfiguration variant Multi Link element. This helper does
 * not take care of ML element fragmentation, the caller/caller chain is
 * expected to take care of this.
 *
 * Since this is a static helper function and we would like to avoid redundant
 * checks, it is the caller's/caller chain's responsibility to validate the
 * following:
 * - All pointer arguments are non-NULL
 * - vap in ni is non-NULL
 * - vdev object in vap is non-NULL
 * - This helper is called for the right subtype
 * - The AP removal timer TBTT count values satisfy protocol and architectural
 * requirements (however a check for this count being non-zero is carried out by
 * this helper)
 *
 * @param linfo: Starting position in buffer at which Link Info should be added
 * @param linfosize: Space in linfo available for filling Link Info (prior to
 * ML element fragmentation)
 * @param ni: Pointer to node structure
 * @param ic: Pointer to iee80211com
 * @param subtype: Frame subtype
 * @param extractx: Extra framing context
 *
 * @return Positive integer value giving number of octets added to linfo in the
 * case of success, -1 in the case of failure
 */
static int ieee80211_add_reconfig_multilink_ie_linkinfo(
        u_int8_t *linfo, size_t linfosize, struct ieee80211_node *ni,
        struct ieee80211com *ic, u_int8_t subtype,
        struct ieee80211_framing_extractx *extractx)
{
    struct ieee80211vap *vap = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_vdev *co_mld_vdev = NULL;
    struct ieee80211vap *co_mld_vap = NULL;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    bool co_mld_vdev_processed[QDF_ARRAY_SIZE(wlan_vdev_list)] = {false};
    uint16_t vdev_count = 0;
    uint16_t i = 0, j = 0;
    qdf_size_t linfospace_remaining = 0;
    int ret = -1;
    QDF_STATUS status;

    /* See function documentation about caller's validation responsibilities. */

    switch (extractx->fectx_mlreconfreq_purpose) {
        case IEEE80211_MLRECONFREQ_PURPOSE_APREMOVE:
            mloie_debug("Received Reconfiguration ML IE Link Info population request for AP Remove");
            break;
        case IEEE80211_MLRECONFREQ_PURPOSE_OPUPDATE:
            mloie_err("Received Reconfiguration ML IE Link Info population request for ML Operation Update - not yet supported");
            return -1;
        default:
            mloie_err("Received invalid Reconfiguration ML IE Link Info population request purpose %d",
                    extractx->fectx_mlreconfreq_purpose);
            return -1;
    }

    if (!extractx->fectx_mlreconfreq_removetimer.num_links) {
         mloie_err("Reconfig ML IE population req: Number of links is 0");
         return -1;
    }

    if (extractx->fectx_mlreconfreq_removetimer.num_links >
            QDF_ARRAY_SIZE(extractx->fectx_mlreconfreq_removetimer.link_info)) {
         mloie_err("Reconfig ML IE population req: Number of links %u exceeds max entries in link info %zu",
                 extractx->fectx_mlreconfreq_removetimer.num_links,
                 QDF_ARRAY_SIZE(extractx->fectx_mlreconfreq_removetimer.link_info));
         return -1;
    }

    if (extractx->fectx_mlreconfreq_removetimer.num_links >
            QDF_ARRAY_SIZE(wlan_vdev_list)) {
         mloie_err("Reconfig ML IE population req: Number of links %u exceeds max entries in VDEV list %zu",
                 extractx->fectx_mlreconfreq_removetimer.num_links,
                 QDF_ARRAY_SIZE(wlan_vdev_list));
         return -1;
    }

    vap = ni->ni_vap;
    vdev = vap->vdev_obj;

    linfospace_remaining = linfosize;

    /* Note: There will be a subsequent change to transition to an API that
     * returns only those AP MLD VDEVs which are active.
     */
    mlo_ap_get_vdev_list(vdev, &vdev_count, wlan_vdev_list);

    if (!vdev_count) {
         mloie_err("Number of VDEVs under MLD is reported as 0");
         return -1;
    }

    /* Note: Once API changes to mlo_ap_get_vdev_list() are available to handle
     * array size requirements, remove this condition.
     */
    if (vdev_count > QDF_ARRAY_SIZE(wlan_vdev_list)) {
         mloie_err("Count of VDEVs %u under MLD exceeds max entries in VDEV list %zu, investigate",
                 vdev_count, QDF_ARRAY_SIZE(wlan_vdev_list));
         goto err_release_refs;
    }

    if (extractx->fectx_mlreconfreq_removetimer.num_links > vdev_count) {
         mloie_err("Number of links %u in Reconfig ML IE population req is greater than count of VDEVs %u under MLD",
                 extractx->fectx_mlreconfreq_removetimer.num_links, vdev_count);
         goto err_release_refs;
    }

    /* Carry out pre-checks on the co MLD VDEVs */
    for (j = 0; j < vdev_count; j++) {
        co_mld_vdev = wlan_vdev_list[i];
        if (co_mld_vdev == NULL) {
            mloie_err("VDEV in MLD VDEV list is NULL");
            goto err_release_refs;
        }

        /* Add any other pre-checks here if required in the future */
    }

    for (i = 0; i < extractx->fectx_mlreconfreq_removetimer.num_links; i++) {
        uint16_t co_mld_vdev_idx = 0;
        uint8_t *perstaprof = NULL;
        qdf_size_t perstaprof_maxsize = 0;
        /* Fixed portion of per-STA profile */
        struct wlan_ml_rv_linfo_perstaprof *perstaprof_fixed = NULL;
        qdf_size_t perstaproflen = 0;
        bool is_frag_required = false;
        qdf_size_t required_fragbuff_size = 0;
        qdf_size_t perstaproflen_fragseq = 0;
        uint16_t stacontrol = 0;
        /* Flag to indicate whether to include complete profile in per-STA
         * profile
         */
        bool include_complete_prof = false;
        /* Flag to indicate whether to include STA MAC address in STA Info */
        bool include_sta_addr = false;
        /* Flag to indicate whether to include AP removal timer in STA Info */
        bool include_remove_timer = false;
        uint16_t removetimer_tbtt = 0;
        uint8_t linkid = 0;
        uint8_t *stainfo = NULL;
        qdf_size_t stainfolen = 0;

        mloie_debug("Processing Reconfig ML IE population req link_info index %u Link ID %u AP removal timer TBTT count %u",
                i, extractx->fectx_mlreconfreq_removetimer.link_info[i].linkid,
                extractx->fectx_mlreconfreq_removetimer.link_info[i].removetimer_tbtt);

        switch (extractx->fectx_mlreconfreq_purpose) {
            case IEEE80211_MLRECONFREQ_PURPOSE_APREMOVE:
                include_complete_prof = false;
                include_sta_addr = false;
                include_remove_timer = true;
                break;
            default:
                break;
        }

        if (!extractx->fectx_mlreconfreq_removetimer.link_info[i].removetimer_tbtt) {
            mloie_err("AP removal timer TBTT count is 0 at Reconfig ML IE population req link_info index %u",
                    i);
            goto err_release_refs;
        }

        /* Find an unprocessed co MLD VDEV corresponding to the 'i'th link ID in
         * the Reconfig ML IE population request.
         */
        for (j = 0; j < vdev_count; j++) {
            if (co_mld_vdev_processed[j])
                continue;

            co_mld_vdev = wlan_vdev_list[j];
            linkid = wlan_vdev_get_link_id(co_mld_vdev);

            if (linkid ==
                    extractx->fectx_mlreconfreq_removetimer.link_info[i].linkid) {
                /* Use a separate index to mark the array position of the vdev
                 * to aid some static analyzers in determining array access
                 * safety.
                 */
                co_mld_vdev_idx = j;
                break;
            }
        }

        if (j == vdev_count) {
            mloie_err("Unable to find unprocessed co MLD VDEV with link ID matching link ID %u at Reconfig ML IE population req link_info index %u (or duplicate link IDs present in request)",
                    extractx->fectx_mlreconfreq_removetimer.link_info[i].linkid,
                    i);
            goto err_release_refs;
        }

        co_mld_vap = wlan_vdev_get_vap(co_mld_vdev);

        if (co_mld_vap == NULL) {
            mloie_err("Co-MLD VAP is NULL");
            goto err_release_refs;
        }

        if (linfospace_remaining < sizeof(struct wlan_ml_rv_linfo_perstaprof)) {
            mloie_err("Insufficient space in Link Info buffer for fixed fields of Per-STA Profile, required: %zu octets, remaining: %zu octets",
                sizeof(struct wlan_ml_rv_linfo_perstaprof),
                linfospace_remaining);
            goto err_release_refs;
        }

        perstaprof = linfo;
        perstaprof_maxsize = linfospace_remaining;
        perstaprof_fixed = (struct wlan_ml_rv_linfo_perstaprof *)perstaprof;
        linfospace_remaining -= sizeof(*perstaprof_fixed);

        /* The Per-STA profile sub element length will be filled in later after
         * determining other contents. Till then, we accumulate the total into a
         * separate variable.
         *
         * Similarly the Per-STA profile sub element STA control field contents
         * will be filled in later. Till then the contents will be accumulated
         * in a separate variable.
         */

        perstaproflen = sizeof(*perstaprof_fixed);

        qdf_mem_zero(perstaprof_fixed, sizeof(*perstaprof_fixed));

        perstaprof_fixed->subelem_id =
            WLAN_ML_LINFO_SUBELEMID_PERSTAPROFILE;

        IEEE80211_SET_BITS(stacontrol,
                WLAN_ML_RV_LINFO_PERSTAPROF_STACTRL_LINKID, linkid);

        stainfo = (u_int8_t *)perstaprof_fixed + sizeof(*perstaprof_fixed);

        /* Increment stainfolen to account for the size of the STA Info Length
         * subfield after validating that there is sufficient remaining space
         * for the subfield. But the subfield will be populated later.
         */
        if (linfospace_remaining <
                WLAN_ML_RV_LINFO_PERSTAPROF_STAINFO_LENGTH_SIZE) {
            mloie_err("Insufficient space in Link Info buffer for STA info length in STA Info in Per-STA Profile, required: %u octets, remaining: %zu octets",
                WLAN_ML_RV_LINFO_PERSTAPROF_STAINFO_LENGTH_SIZE,
                linfospace_remaining);
            goto err_release_refs;
        }

        stainfolen += WLAN_ML_RV_LINFO_PERSTAPROF_STAINFO_LENGTH_SIZE;
        linfospace_remaining -= WLAN_ML_RV_LINFO_PERSTAPROF_STAINFO_LENGTH_SIZE;

        if (include_complete_prof) {
            IEEE80211_SET_BITS(stacontrol,
                    WLAN_ML_RV_LINFO_PERSTAPROF_STACTRL_CMPLTPROF, 1);

            /* Any other information required for the complete profile should be
             * added here or in other places appropriately.
             */
        }

        if (include_sta_addr) {
            if (linfospace_remaining < QDF_MAC_ADDR_SIZE) {
                mloie_err("Insufficient space in Link Info buffer for STA MAC address in STA info in Per-STA Profile, required: %u octets, remaining: %zu octets",
                    QDF_MAC_ADDR_SIZE, linfospace_remaining);
                goto err_release_refs;
            }

            IEEE80211_SET_BITS(stacontrol,
                    WLAN_ML_RV_LINFO_PERSTAPROF_STACTRL_STAMACADDRP, 1);

            IEEE80211_ADDR_COPY(stainfo + stainfolen, co_mld_vap->iv_myaddr);
            stainfolen += QDF_MAC_ADDR_SIZE;
            linfospace_remaining -= QDF_MAC_ADDR_SIZE;
        }

        if (include_remove_timer) {
            if (linfospace_remaining <
                    WLAN_ML_RV_LINFO_PERSTAPROF_STAINFO_APREMOVALTIMER_SIZE) {
                mloie_err("Insufficient space in Link Info buffer for AP removal timer in STA info in Per-STA Profile, required: %u octets, remaining: %zu octets",
                    WLAN_ML_RV_LINFO_PERSTAPROF_STAINFO_APREMOVALTIMER_SIZE,
                    linfospace_remaining);
                goto err_release_refs;
            }

            IEEE80211_SET_BITS(stacontrol,
                    WLAN_ML_RV_LINFO_PERSTAPROF_STACTRL_APREMOVALTIMERP, 1);

            /* The below guard assert should never trigger unless there is a
             * coding error.
             */
            qdf_assert_always(sizeof(removetimer_tbtt) ==
                    WLAN_ML_RV_LINFO_PERSTAPROF_STAINFO_APREMOVALTIMER_SIZE);
            removetimer_tbtt =
                qdf_cpu_to_le16(extractx->fectx_mlreconfreq_removetimer.link_info[i].removetimer_tbtt);
            qdf_mem_copy(stainfo + stainfolen, &removetimer_tbtt,
                    WLAN_ML_RV_LINFO_PERSTAPROF_STAINFO_APREMOVALTIMER_SIZE);
            stainfolen +=
                WLAN_ML_RV_LINFO_PERSTAPROF_STAINFO_APREMOVALTIMER_SIZE;
            linfospace_remaining -=
                WLAN_ML_RV_LINFO_PERSTAPROF_STAINFO_APREMOVALTIMER_SIZE;
        }

        /* Population of Operation Parameters is not yet handled. Add handling
         * here if and when this is implemented.
         */

        /* Update STA Info length subfield after validating its value. */

        if (stainfolen > U8_MAX) {
            mloie_err("STA Info len %zu octets exceeds max value that can be represented in an 8-bit length field %u. Investigate.",
                stainfolen, U8_MAX);
            goto err_release_refs;
        }

        if (stainfolen > WLAN_ML_RV_LINFO_PERSTAPROF_STAINFO_LENGTH_MAX) {
            mloie_err("STA Info len %zu octets exceeds max expected value %zu octets",
                stainfolen, WLAN_ML_RV_LINFO_PERSTAPROF_STAINFO_LENGTH_MAX);
            goto err_release_refs;
        }

        *stainfo = stainfolen;

        /* End of STA control and STA info population */

        perstaprof_fixed->stacontrol = qdf_cpu_to_le16(stacontrol);
        perstaproflen += stainfolen;

        /* The probability of fragmentation being required for per-STA profile
         * subelement in Reconfiguration Multi-Link element is quite low.
         * However, we put in place the required flow in order to be future
         * ready.
         */
        status = wlan_get_subelem_fragseq_requirements(
                    WLAN_ML_LINFO_SUBELEMID_PERSTAPROFILE,
                    perstaproflen - IEEE80211_SUBELEM_HDR_LEN,
                    &is_frag_required,
                    &required_fragbuff_size);
        if (QDF_IS_STATUS_ERROR(status)) {
            goto err_release_refs;
        }

        /* Note: Remove qdf_likely in the future based on standards changes if
         * applicable.
         */
        if (qdf_likely(!is_frag_required)) {
            perstaprof_fixed->subelem_len = perstaproflen -
                IEEE80211_SUBELEM_HDR_LEN;
        } else {
            if (required_fragbuff_size > perstaprof_maxsize) {
                mloie_err("Insufficient space for creating per-STA profile subelement fragment sequence. Required: %zu octets, available: %zu octets",
                          required_fragbuff_size, perstaprof_maxsize);
                goto err_release_refs;
            }

            /* The utility will take care of populating subelement ID and
             * subelement length, in the leading fragment and other fragments.
             */
            status = wlan_create_subelem_fragseq(true,
                        WLAN_ML_LINFO_SUBELEMID_PERSTAPROFILE,
                        WLAN_ML_LINFO_SUBELEMID_FRAGMENT,
                        perstaprof,
                        perstaprof_maxsize,
                        perstaproflen - IEEE80211_SUBELEM_HDR_LEN,
                        NULL,
                        0,
                        &perstaproflen_fragseq);

            if (QDF_IS_STATUS_ERROR(status)) {
                goto err_release_refs;
            }

            /* Reduce linfospace_remaining by the additional octets introduced
             * due to the fragmentation.
             */
            linfospace_remaining -= (perstaproflen_fragseq - perstaproflen);

            perstaproflen = perstaproflen_fragseq;
        }

        linfo += perstaproflen;
        co_mld_vdev_processed[co_mld_vdev_idx] = true;
    }

    ret = (linfosize - linfospace_remaining);

err_release_refs:
    if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                    QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count,
                    0)) != QDF_STATUS_SUCCESS) {
        /* This should not occur. If this occurs, would indicate larger issues.
         */
        mloie_err("Error while trying to release VDEV references - investigate.");
    }

    return ret;
}

/**
 * ieee80211_add_priority_access_multilink_ie_per_STA_profile() - Helper function
 * to add per STA profile in priority access variant Multi Link element specific
 * to action frames.
 *
 * @param persta_prof: Starting position in buffer at which STA profile
 *                     should be added
 * @param perstaprof_size: Space available for filling STA profile
 * @param co_mld_vdev: Pointer to partner vdev structure for which STA
 *                     profile needs to be added
 *
 * @return Zero or positive integer value giving number of octets added to
 * STA profile in the case of success, -1 in the case of failure. Note that
 * a return value of 0 indicates that STA profile field is not currently
 * required.
 */
static int ieee80211_add_priority_access_multilink_ie_per_STA_profile(
        u_int8_t *persta_prof, size_t perstaprof_size,
        struct wlan_objmgr_vdev *co_mld_vdev)
{
    struct wlan_ml_pav_linfo_perstaprof *perstaprof_fixed = NULL;
    struct wlan_objmgr_pdev *co_mld_pdev = wlan_vdev_get_pdev(co_mld_vdev);
    struct ieee80211vap *co_mld_vap = NULL;
    size_t perstaprof_space_remaining = 0;
    uint8_t linkid = 0;
    uint16_t stacontrol = 0;
    uint8_t *staprof = NULL;
    uint8_t *edca_ie_ptr = NULL;
    uint8_t *muedca_ie_ptr = NULL;
    uint8_t *wme_ie_ptr = NULL;
    uint8_t edca_ie_len;
    uint8_t muedca_ie_len;
    uint8_t wme_ie_len;

    if (!co_mld_pdev) {
        mloie_err("co_mld_pdev is NULL");
        return -1;
    }

    co_mld_vap = wlan_vdev_get_vap(co_mld_vdev);
    if (!co_mld_vap) {
        mloie_err("Co-MLD VAP is NULL");
        return -1;
    }

    perstaprof_space_remaining = perstaprof_size;

    if (perstaprof_space_remaining < sizeof(struct wlan_ml_pav_linfo_perstaprof)) {
        mloie_err("Insufficient space in per-STA profile buffer for fixed fields of Per-STA Profile, required: %zu octets, remaining: %zu octets",
                sizeof(struct wlan_ml_pav_linfo_perstaprof),
                perstaprof_space_remaining);
        return -1;
    }

    perstaprof_fixed = (struct wlan_ml_pav_linfo_perstaprof *)persta_prof;
    perstaprof_space_remaining -= sizeof(*perstaprof_fixed);

    qdf_mem_zero(perstaprof_fixed, sizeof(*perstaprof_fixed));

    perstaprof_fixed->subelem_id = WLAN_ML_LINFO_SUBELEMID_PERSTAPROFILE;

    linkid = wlan_vdev_get_link_id(co_mld_vdev);
    IEEE80211_SET_BITS(stacontrol, WLAN_ML_PAV_LINFO_PERSTAPROF_STACTRL_LINKID,
                       linkid);


    /* End of STA control population */
    perstaprof_fixed->stacontrol = htole16(stacontrol);

    staprof = (uint8_t *)perstaprof_fixed + sizeof(*perstaprof_fixed);

    edca_ie_ptr = ieee80211_add_edca_param(staprof,  &co_mld_vap->iv_epcs_edcastate);
    edca_ie_len = edca_ie_ptr - staprof;
    if (edca_ie_len < 0) {
        mloie_err("Failed to add Edca IE in PA Multi Link element for EPCS action frame");
        return -1;
    }

    muedca_ie_ptr = ieee80211_add_muedca_param(edca_ie_ptr, &co_mld_vap->iv_muedcastate);
    muedca_ie_len = muedca_ie_ptr - edca_ie_ptr;
    if (muedca_ie_len < 0) {
        mloie_err("Failed to add MU Edca IE in PA Multi Link element for EPCS action frame");
        return -1;
    }

    wme_ie_ptr = ieee80211_add_wme_param(muedca_ie_ptr, &co_mld_vap->iv_epcs_edcastate.wme_ChanParams, 0);
    wme_ie_len = wme_ie_ptr - muedca_ie_ptr;
    if (wme_ie_len < 0) {
        mloie_err("Failed to add WME IE in PA Multi Link element for EPCS action frame");
        return -1;
    }

    perstaprof_space_remaining -= edca_ie_len + muedca_ie_len + wme_ie_len;

    perstaprof_fixed->subelem_len = ((perstaprof_size - perstaprof_space_remaining) -
                                     IEEE80211_IE_HDR_LEN);

    return (perstaprof_size - perstaprof_space_remaining);
}

/**
 * ieee80211_add_priority_access_multilink_ie_linkinfo() - Helper function to
 * add Link Info field in Priority access variant Multi Link element.
 * This helper does not take care of ML element fragmentation, the caller/caller
 *  chain is expected to take care of this.
 *
 * Since this is a static helper function and we would like to avoid redundant
 * checks, it is the caller's/caller chain's responsibility to validate the
 * following:
 * - All pointer arguments are non-NULL
 * - vap in ni is non-NULL
 * - vdev object in vap is non-NULL
 * - This helper is called for the right subtype
 *
 * @param linfo: Starting position in buffer at which Link Info should be added
 * @param linfosize: Space in linfo available for filling Link Info (prior to
 * ML element fragmentation)
 * @param ni: Pointer to node structure
 * @param ic: Pointer to iee80211com
 * @param subtype: Frame subtype
 * @param extractx: Extra framing context
 *
 * @return Positive integer value giving number of octets added to linfo in the
 * case of success, -1 in the case of failure
 */
static int ieee80211_add_priority_access_multilink_ie_linkinfo(
        u_int8_t *linfo, size_t linfosize, struct ieee80211_node *ni,
        struct ieee80211com *ic, u_int8_t subtype,
        struct ieee80211_framing_extractx *extractx)
{
    struct ieee80211vap *vap = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_vdev *co_mld_vdev = NULL;
    struct ieee80211vap *co_mld_vap = NULL;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    uint16_t vdev_count = 0;
    uint16_t i = 0;
    qdf_size_t linfospace_remaining = 0;
    int ret = -1;
    uint16_t release_ref_start_idx;
    struct ieee80211com *co_mld_ic = NULL;
    uint16_t perstaprof_len = 0;
    size_t available_space = linfosize;

    /* See function documentation about caller's validation responsibilities. */

    vap = ni->ni_vap;
    vdev = vap->vdev_obj;

    linfospace_remaining = linfosize;

    /* Note: There will be a subsequent change to transition to an API that
     * returns only those AP MLD VDEVs which are active.
     */
    mlo_peer_get_vdev_list(ni->peer_obj, &vdev_count, wlan_vdev_list);

    if (!vdev_count) {
         mloie_err("Number of VDEVs under MLD is reported as 0");
         return -1;
    }

    /* Note: Once API changes to mlo_ap_get_vdev_list() are available to handle
     * array size requirements, remove this condition.
     */
    if (vdev_count > QDF_ARRAY_SIZE(wlan_vdev_list)) {
         mloie_err("Count of VDEVs %u under MLD exceeds max entries in VDEV list %zu, investigate",
                 vdev_count, QDF_ARRAY_SIZE(wlan_vdev_list));
         goto err_release_refs;
    }

    for(i = 0; i < vdev_count; i++) {

        /* In case of errors during further processing of co MLD VDEV with index
         * i, we need to release all the VDEV references in the VDEV list
         * starting from index i (since references for VDEVs prior to i would
         * already have been released).
         */
        release_ref_start_idx = i;

        co_mld_vdev = wlan_vdev_list[i];
        if (co_mld_vdev == NULL) {
            mloie_err("VDEV in MLD VDEV list is NULL");
            goto err_release_refs;
        }

        co_mld_vap = wlan_vdev_get_vap(co_mld_vdev);
        if (co_mld_vap == NULL) {
            mloie_err("Co-MLD VAP is NULL");
            goto err_release_refs;
        }

        co_mld_ic = co_mld_vap->iv_ic;
        if (co_mld_ic == NULL) {
            mloie_err("Co-MLD IC is NULL");
            goto err_release_refs;
        }

        perstaprof_len = ieee80211_add_priority_access_multilink_ie_per_STA_profile(
                linfo, available_space, co_mld_vdev);

        if (perstaprof_len < 0) {
            mloie_err("Failed to add Per STA profile IE in Basic variant Multi Link element for Beacon/non-ML probe response");
            goto err_release_refs;
        }

        available_space -= perstaprof_len;
        linfo += perstaprof_len;

        mlo_release_vdev_ref(wlan_vdev_list[i]);
    }

    return (linfosize - available_space);

err_release_refs:
    if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                    QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count,
                    release_ref_start_idx)) != QDF_STATUS_SUCCESS) {
        /* This should not occur. If this occurs, would indicate larger
         * issues.
         */
        mloie_err("Error while trying to release VDEV references - investigate.");
        return -1;
    }
    return ret;
}

/**
 * ieee80211_add_reconfig_multilink_ie_fields() - Helper function to populate
 * Reconfiguration variant Multi-Link Information Element, and corresponding
 * Fragment elements if applicable
 *
 * Since this is a static helper function and we would like to avoid redundant
 * checks, it is the caller's/caller chain's responsibility to validate the
 * following:
 * - All pointer arguments are non-NULL
 * - vap in ni is non-NULL
 * - vdev object in vap is non-NULL
 * - This helper is called for the right subtype
 *
 * @param mlbuf: Buffer into which which the element(s) should be added
 * @param mlbufsize: Size of buffer mlbuf
 * @param ni: Pointer to associated node structure
 * @param ic: Pointer to iee80211com
 * @param subtype: Frame subtype
 * @param extractx: Extra framing context
 *
 * @return Positive integer value giving number of octets added to mlbuf in the
 * case of success, -1 in the case of failure
 */
static int ieee80211_add_reconfig_multilink_ie_fields(u_int8_t *mlbuf,
        qdf_size_t mlbufsize, struct ieee80211_node *ni, struct ieee80211com *ic,
        u_int8_t subtype, struct ieee80211_framing_extractx *extractx)
{
    struct wlan_ie_multilink *mlie_fixed = NULL;
    qdf_size_t mlbufspace_remaining = 0;
    struct ieee80211vap *vap = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    u_int16_t mlcontrol = 0;
    u_int16_t presencebitmap = 0;
    u_int8_t *commoninfo = NULL;
    qdf_size_t commoninfo_len = 0;
    struct qdf_mac_addr *mld_addr = NULL;
    /* Flag to indicate whether to include MLD MAC address in Common Info */
    bool include_mld_addr = false;
    u_int8_t *linkinfopos = NULL;
    int linkinfo_len = 0;
    qdf_size_t populatedlen = 0;
    bool is_frag_required = false;
    qdf_size_t required_fragbuff_size = 0;
    qdf_size_t populatedlen_fragseq = 0;
    QDF_STATUS ret;

    /* See function documentation about caller's validation responsibilities. */

    vap = ni->ni_vap;
    vdev = vap->vdev_obj;

    if (!wlan_vdev_mlme_is_mlo_ap(vdev)) {
        mloie_err("Reconfiguration ML IE is currently applicable only for AP MLD");
        return -1;
    }

    switch (extractx->fectx_mlreconfreq_purpose) {
        case IEEE80211_MLRECONFREQ_PURPOSE_APREMOVE:
            mloie_debug("Received Reconfiguration ML IE population request for AP remove");
            include_mld_addr = false;
            break;
        case IEEE80211_MLRECONFREQ_PURPOSE_OPUPDATE:
            mloie_err("Received Reconfiguration ML IE population request for ML Operation Update - not yet supported");
            return -1;
        default:
            mloie_err("Received invalid Reconfiguration ML IE population request purpose %d",
                    extractx->fectx_mlreconfreq_purpose);
            return -1;
    }

    mlbufspace_remaining = mlbufsize;

    if (mlbufspace_remaining < sizeof(struct wlan_ie_multilink)) {
        mloie_err("Insufficient space in ML IE buffer for IE headers and control, required: %zu octets, remaining: %zu octets",
                sizeof(struct wlan_ie_multilink), mlbufspace_remaining);
        return -1;
    }

    /*
     * Populate Variant, Presence Bitmap sub-field in Multi-Link Control field
     * and corresponding Common Info sub-fields, where applicable.
     */
    mlie_fixed = (struct wlan_ie_multilink *)mlbuf;
    mlbufspace_remaining -= sizeof(*mlie_fixed);

    qdf_mem_zero(mlie_fixed, sizeof(*mlie_fixed));

    mlie_fixed->elem_id = WLAN_ELEMID_EXTN_ELEM;
    mlie_fixed->elem_id_ext = WLAN_EXTN_ELEMID_MULTI_LINK;

    IEEE80211_SET_BITS(mlcontrol, WLAN_ML_CTRL_TYPE, WLAN_ML_VARIANT_RECONFIG);

    commoninfo = mlbuf + sizeof(*mlie_fixed);

    /*
     * Common Info Length in Common Info
     *
     * Note: At this stage, we only verify whether we have sufficient space in
     * the buffer for this subfield, and start calculating the length. The
     * calculated value will be populated into the subfield later.
     */
    if (mlbufspace_remaining < WLAN_ML_RV_CINFO_LENGTH_SIZE) {
        mloie_err("Insufficient space in ML IE buffer for Common Info Len, required: %u octets, remaining: %zu octets",
            WLAN_ML_RV_CINFO_LENGTH_SIZE, mlbufspace_remaining);
        return -1;
    }

    commoninfo_len += WLAN_ML_RV_CINFO_LENGTH_SIZE;
    mlbufspace_remaining -= WLAN_ML_RV_CINFO_LENGTH_SIZE;

    if (include_mld_addr) {
        /* Add MLD Address in Common Info */
        if (mlbufspace_remaining < QDF_MAC_ADDR_SIZE) {
            mloie_err("Insufficient space in ML IE buffer for MLD MAC addr in Common Info, required: %u octets, remaining: %zu octets",
                QDF_MAC_ADDR_SIZE, mlbufspace_remaining);
            return -1;
        }

        presencebitmap |= WLAN_ML_RV_CTRL_PBM_MLDMACADDR_P;
        wlan_vdev_obj_lock(vdev);
        mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev);
        IEEE80211_ADDR_COPY(commoninfo + commoninfo_len, mld_addr->bytes);
        wlan_vdev_obj_unlock(vdev);

        commoninfo_len += QDF_MAC_ADDR_SIZE;
        mlbufspace_remaining -= QDF_MAC_ADDR_SIZE;
    }

    /* Update Common Info Length subfield after validating its value */

    if (commoninfo_len > U8_MAX) {
            mloie_err("Calculated Common Info len %zu octets higher than max value that can be represented in an 8-bit length field %u. Investigate.",
                commoninfo_len, U8_MAX);
            return -1;
    }

    if (commoninfo_len > WLAN_ML_RV_CINFO_LENGTH_MAX) {
            mloie_err("Calculated Common Info len %zu octets higher than max expected value of %u octets",
                commoninfo_len, WLAN_ML_RV_CINFO_LENGTH_MAX);
            return -1;
    }

    *commoninfo = commoninfo_len;

    /* Set the presence bitmap only at the end so that if new subfields are
     * conditionally added to Common Info in the future, the conditional
     * addition of each subfield can be processed sequentially before reflecting
     * the presence bitmap into the ML Control field. This would also provide
     * uniformity with processing for other ML IE variants.
     */
    IEEE80211_SET_BITS(mlcontrol, WLAN_ML_CTRL_PBM, presencebitmap);
    mlie_fixed->mlcontrol = htole16(mlcontrol);

    /* Link Info */

    linkinfopos = mlbuf + (mlbufsize - mlbufspace_remaining);

    linkinfo_len = ieee80211_add_reconfig_multilink_ie_linkinfo(
            linkinfopos, mlbufspace_remaining, ni, ic, subtype, extractx);

    if (linkinfo_len < 0) {
        mloie_err("Failed to add Link Info field in Reconfig variant Multi Link element");
        return -1;
    } else {
        /* The helper function has the responsibility of validating that
         * there is enough space remaining in the ML IE buffer for Link
         * Info.
         */
        mlbufspace_remaining -= linkinfo_len;
    }

    populatedlen = mlbufsize - mlbufspace_remaining;

    /* The probability of fragmentation being required for Reconfiguration
     * Multi-Link element is quite low. However, we put in place the required
     * flow in order to be future ready.
     */
    ret = wlan_get_elem_fragseq_requirements(WLAN_ELEMID_EXTN_ELEM,
                                             populatedlen -
                                                IEEE80211_IE_HDR_LEN - 1,
                                             &is_frag_required,
                                             &required_fragbuff_size);
    if (QDF_IS_STATUS_ERROR(ret))
        return -1;

    /* Note: Remove qdf_likely in the future based on standards changes if
     * applicable.
     */
    if (qdf_likely(!is_frag_required)) {
        mlie_fixed->elem_len = populatedlen - IEEE80211_IE_HDR_LEN;
        return populatedlen;
    } else {
        if (required_fragbuff_size > mlbufsize) {
            mloie_err("Insufficient space for creating Multi Link element fragment sequence. Required: %zu octets, available: %zu octets",
                      required_fragbuff_size, mlbufsize);
            return -1;
        }

        /* The utility will take care of populating element ID, element length,
         * element ID extension (where applicable) in the leading fragment and
         * other fragments.
         */
        ret = wlan_create_elem_fragseq(true,
                                       WLAN_ELEMID_EXTN_ELEM,
                                       WLAN_EXTN_ELEMID_MULTI_LINK,
                                       mlbuf,
                                       mlbufsize,
                                       populatedlen -  IEEE80211_IE_HDR_LEN - 1,
                                       NULL,
                                       0,
                                       &populatedlen_fragseq);

        if (QDF_IS_STATUS_ERROR(ret))
            return -1;
        else
            return populatedlen_fragseq;
    }
}

/**
 * ieee80211_add_priority_access_multilink_ie_fields() - Helper function to
 * populate priority access variant of Multi-Link Information Element, and
 * corresponding fragment elements if applicable
 *
 * Since this is a static helper function and we would like to avoid redundant
 * checks, it is the caller's/caller chain's responsibility to validate the
 * following:
 * - All pointer arguments are non-NULL
 * - vap in ni is non-NULL
 * - vdev object in vap is non-NULL
 * - This helper is called for the right subtype
 * - For subtye IEEE80211_FC0_SUBTYPE_ACTION, necessary conditions as stipulated
 *   in the standard are satisfied.
 *
 * @param mlbuf: Buffer into which which the element(s) should be added
 * @param mlbufsize: Size of buffer mlbuf
 * @param ni: Pointer to associated node structure
 * @param ic: Pointer to iee80211com
 * @param subtype: Frame subtype
 * @param extractx: Extra framing context. This can be NULL.
 *
 * @return Positive integer value giving number of octets added to mlbuf in the
 * case of success, -1 in the case of failure
 */
static int ieee80211_add_priority_access_multilink_ie_fields(uint8_t *mlbuf,
        qdf_size_t mlbufsize, struct ieee80211_node *ni, struct ieee80211com *ic,
        u_int8_t subtype, struct ieee80211_framing_extractx *extractx)
{
    struct wlan_ie_multilink *mlie_fixed = NULL;
    qdf_size_t mlbufspace_remaining = 0;
    struct ieee80211vap *vap = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    u_int16_t mlcontrol = 0;
    u_int8_t *commoninfo = NULL;
    qdf_size_t commoninfo_len = 0;
    struct qdf_mac_addr *mld_addr = NULL;
    u_int8_t *linkinfopos = NULL;
    int linkinfo_len = 0;
    qdf_size_t populatedlen = 0;
    bool is_frag_required = false;
    qdf_size_t required_fragbuff_size = 0;
    qdf_size_t populatedlen_fragseq = 0;
    QDF_STATUS ret;

    /* See function documentation about caller's validation responsibilities. */

    vap = ni->ni_vap;
    vdev = vap->vdev_obj;

    if (!wlan_vdev_mlme_is_mlo_ap(vdev)) {
        mloie_err("Priority Access varient ML IE is currently applicable only for AP MLD");
        return -1;
    }

    mlbufspace_remaining = mlbufsize;

    if (mlbufspace_remaining < sizeof(struct wlan_ie_multilink)) {
        mloie_err("Insufficient space in ML IE buffer for IE headers and control, required: %zu octets, remaining: %zu octets",
                sizeof(struct wlan_ie_multilink), mlbufspace_remaining);
        return -1;
    }

    /*
     * Populate Variant, Presence Bitmap sub-field in Multi-Link Control field
     * and corresponding Common Info sub-fields, where applicable.
     */
    mlie_fixed = (struct wlan_ie_multilink *)mlbuf;
    mlbufspace_remaining -= sizeof(*mlie_fixed);

    qdf_mem_zero(mlie_fixed, sizeof(*mlie_fixed));

    mlie_fixed->elem_id = WLAN_ELEMID_EXTN_ELEM;
    mlie_fixed->elem_id_ext = WLAN_EXTN_ELEMID_MULTI_LINK;

    IEEE80211_SET_BITS(mlcontrol, WLAN_ML_CTRL_TYPE, WLAN_ML_VARIANT_PRIORITYACCESS);

    commoninfo = mlbuf + sizeof(*mlie_fixed);

    if (mlbufspace_remaining < WLAN_ML_PAV_CINFO_LENGTH_SIZE) {
        mloie_err("Insufficient space in ML IE buffer for Common Info Len, required: %u octets, remaining: %zu octets", WLAN_ML_PAV_CINFO_LENGTH_SIZE, mlbufspace_remaining);
    }

    commoninfo_len += WLAN_ML_PAV_CINFO_LENGTH_SIZE;
    mlbufspace_remaining -= WLAN_ML_PAV_CINFO_LENGTH_SIZE;

    /* Add MLD Address in Common Info */
    if (mlbufspace_remaining < QDF_MAC_ADDR_SIZE) {
        mloie_err("Insufficient space in ML IE buffer for MAC addr in Common Info, required: %u octets, remaining: %zu octets",
            QDF_MAC_ADDR_SIZE, mlbufspace_remaining);
        return -1;
    }

    wlan_vdev_obj_lock(vdev);
    mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev);
    IEEE80211_ADDR_COPY(commoninfo + commoninfo_len, mld_addr->bytes);
    wlan_vdev_obj_unlock(vdev);

    commoninfo_len += QDF_MAC_ADDR_SIZE;
    mlbufspace_remaining -= QDF_MAC_ADDR_SIZE;

    if (commoninfo_len > WLAN_ML_PAV_CINFO_LENGTH_MAX) {
        mloie_err("Calculated Common Info len %zu octets higher than max expected value of %u octets", commoninfo_len, WLAN_ML_PAV_CINFO_LENGTH_MAX);
        return -1;
    }

    *commoninfo = commoninfo_len;

    mlie_fixed->mlcontrol = htole16(mlcontrol);

    /* Link Info */
    linkinfopos = mlbuf + (mlbufsize - mlbufspace_remaining);

    linkinfo_len = ieee80211_add_priority_access_multilink_ie_linkinfo(
            linkinfopos, mlbufspace_remaining, ni, ic, subtype, extractx);

    if (linkinfo_len < 0) {
        mloie_err("Failed to add Link Info field in Reconfig variant Multi Link element");
        return -1;
    } else {
        /* The helper function has the responsibility of validating that
         * there is enough space remaining in the ML IE buffer for Link
         * Info.
         */
        mlbufspace_remaining -= linkinfo_len;
    }

    populatedlen = mlbufsize - mlbufspace_remaining;

    /* The probability of fragmentation being required for Reconfiguration
     * Multi-Link element is quite low. However, we put in place the required
     * flow in order to be future ready.
     */
    ret = wlan_get_elem_fragseq_requirements(WLAN_ELEMID_EXTN_ELEM,
                                             populatedlen -
                                                IEEE80211_IE_HDR_LEN - 1,
                                             &is_frag_required,
                                             &required_fragbuff_size);
    if (QDF_IS_STATUS_ERROR(ret))
        return -1;

    /* Note: Remove qdf_likely in the future based on standards changes if
     * applicable.
     */
    if (qdf_likely(!is_frag_required)) {
        mlie_fixed->elem_len = populatedlen - IEEE80211_IE_HDR_LEN;
        return populatedlen;
    } else {
        if (required_fragbuff_size > mlbufsize) {
            mloie_err("Insufficient space for creating Multi Link element fragment sequence. Required: %zu octets, available: %zu octets",
                      required_fragbuff_size, mlbufsize);
            return -1;
        }

        /* The utility will take care of populating element ID, element length,
         * element ID extension (where applicable) in the leading fragment and
         * other fragments.
         */
        ret = wlan_create_elem_fragseq(true,
                                       WLAN_ELEMID_EXTN_ELEM,
                                       WLAN_EXTN_ELEMID_MULTI_LINK,
                                       mlbuf,
                                       mlbufsize,
                                       populatedlen -  IEEE80211_IE_HDR_LEN - 1,
                                       NULL,
                                       0,
                                       &populatedlen_fragseq);

        if (QDF_IS_STATUS_ERROR(ret))
            return -1;
        else
            return populatedlen_fragseq;
    }
}

u_int8_t* ieee80211_add_multilink_ie(u_int8_t *mlbuf, size_t mlbufsize,
        u_int8_t *containerfrm, size_t containerfrmsize,
        struct ieee80211_node *ni, struct ieee80211com *ic, u_int8_t subtype,
        bool is_variant_specified, enum wlan_ml_variant variant,
        struct ieee80211_framing_extractx *extractx,
        struct ieee80211_beacon_offsets *bo,
        struct ieee80211_rx_status *rs)
{
    struct ieee80211vap *vap = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    enum wlan_ml_variant applied_variant = WLAN_ML_VARIANT_INVALIDSTART;
    int ret = 0;

    if (mlbuf == NULL) {
        mloie_err("mlbuf is NULL");
        return NULL;
    }

    if (mlbufsize == 0) {
        mloie_err("mlbufsize is 0");
        return NULL;
    }

    if (ni == NULL) {
        mloie_err("ni is NULL");
        return NULL;
    }

    if (ic == NULL) {
        mloie_err("ic is NULL");
        return NULL;
    }

    vap = ni->ni_vap;
    if (vap == NULL) {
        mloie_err("vap is NULL");
        return NULL;
    }

    vdev = vap->vdev_obj;
    if (vdev == NULL) {
        mloie_err("vdev is NULL");
        return NULL;
    }

    if (!wlan_vdev_mlme_is_mlo_vdev(vdev)) {
        mloie_err("VDEV is not part of an MLD");
        return NULL;
    }

    /* Note: Update the below based on standards updates */
    if (!((subtype == IEEE80211_FC0_SUBTYPE_BEACON) ||
            (subtype == IEEE80211_FC0_SUBTYPE_PROBE_REQ) ||
            (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP) ||
            (subtype == IEEE80211_FC0_SUBTYPE_AUTH) ||
            (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_REQ) ||
            (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ) ||
            (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
            (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP) ||
            (subtype == IEEE80211_FC0_SUBTYPE_ACTION))) {
        mloie_err("Multi Link element is not applicable for subtype %hhu\n",
                subtype);
        return NULL;
    }

    if (is_variant_specified) {
        /* Validate the requested variant */
        switch (variant) {
            case WLAN_ML_VARIANT_BASIC:
                if (!((subtype == IEEE80211_FC0_SUBTYPE_BEACON) ||
                        (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP) ||
                        (subtype == IEEE80211_FC0_SUBTYPE_AUTH) ||
                        (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_REQ) ||
                        (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ) ||
                        (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
                        (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP))) {
                    mloie_err("Basic variant Multi Link element is not applicable for subtype %hhu\n",
                            subtype);
                    return NULL;
                }
                break;
            case WLAN_ML_VARIANT_PROBEREQ:
                if (subtype != IEEE80211_FC0_SUBTYPE_PROBE_REQ) {
                    mloie_err("Probe request variant Multi Link element is not applicable for subtype %hhu\n",
                            subtype);
                    return NULL;
                }
                break;
           case WLAN_ML_VARIANT_RECONFIG:
                if (!((subtype == IEEE80211_FC0_SUBTYPE_BEACON) ||
                        (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP))) {
                    mloie_err("Reconfiguration variant Multi Link element is not applicable for subtype %hhu\n",
                            subtype);
                    return NULL;
                }

                break;
           case WLAN_ML_VARIANT_PRIORITYACCESS:
                if (!(subtype == IEEE80211_FC0_SUBTYPE_ACTION)) {
                    mloie_err("Priority access Multi Link element is not applicable for subtype %hhu\n",
                            subtype);
                    return NULL;
                }
                break;
           default:
                mloie_err("Multi Link element variant %d is not yet handled or is invalid\n", variant);
                return NULL;
        }

        applied_variant = variant;
    } else {
        /* No variant requested. Assign a default variant as per subtype. */

        if (subtype == IEEE80211_FC0_SUBTYPE_PROBE_REQ)
            applied_variant = WLAN_ML_VARIANT_PROBEREQ;
        else if (subtype == IEEE80211_FC0_SUBTYPE_ACTION)
            applied_variant = WLAN_ML_VARIANT_PRIORITYACCESS;
        else
            applied_variant = WLAN_ML_VARIANT_BASIC;
    }

    switch (applied_variant) {
        case WLAN_ML_VARIANT_PROBEREQ:
            {
#define ML_PREQ_ENABLE_MASK 0x1
#define ML_PREQ_MLDID_MASK 0x2
#define ML_PREQ_LINKID_MASK 0x4
#define ML_PREQ_LINKID_INVALID 0xF
                u_int8_t mld_id = ML_PROBE_MLDID_INVALID;
                u_int8_t link_id[MLD_MAX_LINKS];

                qdf_mem_set(link_id, MLD_MAX_LINKS, 0xFF);

                mloie_debug("Adding Probe Req Multi Link element in Probe Req frame for VAP with link address %s",
                             ether_sprintf(vap->iv_myaddr));

                if (!vap->ml_preq_enable & ML_PREQ_ENABLE_MASK)
                    return mlbuf;

                if (vap->ml_preq_enable & ML_PREQ_MLDID_MASK)
                        mld_id = vap->ml_preq_mldid;

                if (vap->ml_preq_enable & ML_PREQ_LINKID_MASK) {
                    uint8_t _link_id;
                    uint8_t i = 0, j = 0;

                    /* Empty positions in link id map should be 0xF */
                    for (; i < MLD_MAX_LINKS; i++) {
                        _link_id = (vap->ml_preq_linkids & (0xF << (i * 4))) >> (i * 4);
                        if (_link_id < MLD_MAX_LINKS) {
                            link_id[j] = _link_id;
                            if (vap->iv_ml_prbreq_info.scan_freq)
                                vap->iv_ml_prbreq_info.link_ids[j] = link_id[j];
                            j++;
                        }
                    }

                    if (vap->iv_ml_prbreq_info.scan_freq)
                        vap->iv_ml_prbreq_info.n_links = j;
                }

                /* Add support to Send an ML probe req packet without link-info */
                if ((vap->ml_preq_enable) &
                    ((vap->ml_preq_linkids & ML_PREQ_LINKID_INVALID) == ML_PREQ_LINKID_INVALID)) {
                    qdf_mem_set(link_id, MLD_MAX_LINKS, 0xFF);
                }

                ret = ieee80211_add_probereq_multilink_ie_fields(mlbuf,
                            mlbufsize, ni, ic, mld_id, link_id, NULL);

                return mlbuf + ret;
            }
            break;
        case WLAN_ML_VARIANT_BASIC:
            {
                mloie_debug("Adding Basic Multi Link element in frame subtype 0x%hhx for VAP with link address %s",
                        subtype, ether_sprintf(vap->iv_myaddr));

                ret = ieee80211_add_basic_multilink_ie_fields(mlbuf, mlbufsize,
                            containerfrm, containerfrmsize, ni, ic, subtype,
                            extractx, bo, rs);

                if (ret < 0) {
                    mloie_err("Error in adding Basic Multi-Link element");
                    return NULL;
                }

                if ((subtype == IEEE80211_FC0_SUBTYPE_ASSOC_REQ) ||
                        (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ) ||
                        (subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
                        (subtype == IEEE80211_FC0_SUBTYPE_AUTH) ||
                        (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP) ||
                        (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP)) {
                    mloie_debug("Hexdump of Basic Multi-Link element:");
                    qdf_trace_hex_dump(QDF_MODULE_ID_MLOIE,
                            QDF_TRACE_LEVEL_DEBUG, mlbuf, ret);
                }

                return mlbuf + ret;
            }
            break;
        case WLAN_ML_VARIANT_RECONFIG:
            {
                if (extractx == NULL) {
                    mloie_err("Extra context is NULL though Reconfig variant requested");
                    return NULL;
                }

                mloie_debug("Adding Reconfig Multi Link element in frame subtype 0x%hhx for VAP with link address %s",
                        subtype, ether_sprintf(vap->iv_myaddr));

                ret = ieee80211_add_reconfig_multilink_ie_fields(mlbuf,
                            mlbufsize, ni, ic, subtype, extractx);

                if (ret < 0) {
                    mloie_err("Error in adding Reconfig Multi-Link element");
                    return NULL;
                }

                mloie_debug("Hexdump of Reconfig Multi-Link element:");
                qdf_trace_hex_dump(QDF_MODULE_ID_MLOIE,
                        QDF_TRACE_LEVEL_DEBUG, mlbuf, ret);

                return mlbuf + ret;
            }
            break;
        case WLAN_ML_VARIANT_PRIORITYACCESS:
            {
                mloie_debug("Adding Priority access Multi Link element in frame subtype 0x%hhx for VAP with link address %s",
                        subtype, ether_sprintf(vap->iv_myaddr));

                ret = ieee80211_add_priority_access_multilink_ie_fields(mlbuf,
                            mlbufsize, ni, ic, subtype, extractx);

                if (ret < 0) {
                    mloie_err("Error in adding priority access Multi-Link element");
                    return NULL;
                }

                mloie_debug("Hexdump of priority access Multi-Link element:");
                qdf_trace_hex_dump(QDF_MODULE_ID_MLOIE,
                        QDF_TRACE_LEVEL_DEBUG, mlbuf, ret);

                return mlbuf + ret;
            }
            break;

        default:
            /* We should never reach this point */
            mloie_err("Unhandled Multi-Link element applied variant %d. Investigate.",
                    applied_variant);
            break;
    }

    return NULL;
}

void ieee80211_parse_peer_mld_cap(struct ieee80211_node *ni,
                                  uint16_t mldcap)
{
    struct wlan_mlo_mld_cap *peer_mld_cap = &ni->mlpeer_mld_cap;

    peer_mld_cap->max_simult_link = IEEE80211_GET_BITS(mldcap,
                                                       WLAN_ML_BV_CINFO_MLDCAPANDOP_MAXSIMULLINKS);
    peer_mld_cap->srs_support = IEEE80211_GET_BITS(mldcap,
                                                   WLAN_ML_BV_CINFO_MLDCAPANDOP_SRSSUPPORT);
    peer_mld_cap->str_freq_sep = IEEE80211_GET_BITS(mldcap,
                                                    WLAN_ML_BV_CINFO_MLDCAPANDOP_STRFREQSEPARATION);
}

#ifdef WLAN_FEATURE_11BE
void ieee80211_parse_t2lm_enable_val(struct ieee80211_node *ni, uint16_t mldcap)
{
    ni->ni_t2lm_enable_val = QDF_GET_BITS(mldcap,
            WLAN_ML_BV_CINFO_MLDCAPANDOP_TIDTOLINKMAPNEGSUPPORT_IDX,
            WLAN_ML_BV_CINFO_MLDCAPANDOP_TIDTOLINKMAPNEGSUPPORT_BITS);
}
#endif /* WLAN_FEATURE_11BE */

void ieee80211_parse_peer_eml_cap(struct ieee80211_node *ni,
                                  uint16_t eml_cap)
{
    struct wlan_mlo_eml_cap *ml_eml_cap = &ni->mlpeer_eml_cap;

    mloie_debug_rl("EML CAP=%hu for %s MLD", eml_cap,
                         ether_sprintf(ni->peer_obj->mldaddr));

    ml_eml_cap->emlsr_supp = IEEE80211_GET_BITS(eml_cap,
                                                WLAN_ML_BV_CINFO_EMLCAP_EMLSRSUPPORT);
    ml_eml_cap->emlsr_pad_delay = IEEE80211_GET_BITS(eml_cap,
                                                     WLAN_ML_BV_CINFO_EMLCAP_EMLSR_PADDINGDELAY);
    ml_eml_cap->emlsr_trans_delay = IEEE80211_GET_BITS(eml_cap,
                                                       WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY);
    ml_eml_cap->emlmr_supp = IEEE80211_GET_BITS(eml_cap,
                                                WLAN_ML_BV_CINFO_EMLCAP_EMLMRSUPPORT);
    ml_eml_cap->emlmr_delay = IEEE80211_GET_BITS(eml_cap,
                                                 WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY);
    ml_eml_cap->trans_timeout = IEEE80211_GET_BITS(eml_cap,
                                                   WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT);
}

QDF_STATUS ieee80211_parse_multilink_ie(struct ieee80211_node *ni,
        u_int8_t *mlie, qdf_size_t rembufflen, u_int8_t subtype)
{

    struct ieee80211com *ic = NULL;
    struct ieee80211vap *vap = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    uint8_t *currpos = NULL;
    uint8_t *ieptr = NULL;
    qdf_size_t ieseqlen = 0;
    qdf_size_t mlieseqlen = 0;
    QDF_STATUS ret;
    enum wlan_ml_variant variant;
    struct qdf_mac_addr mldmacaddr;
    uint16_t  eml_cap;
    bool eml_cap_found;
    int rptinglinkidx;
    bool mldcapfound = 0;
    uint16_t mldcap = 0;
    bool mldidfound = 0;
    uint8_t mldid = 0;

    if (!ni) {
        mloie_err("ni is NULL");
        return QDF_STATUS_E_INVAL;
    }

    if (!mlie) {
        mloie_err("Pointer to Multi-Link element is NULL");
        return QDF_STATUS_E_INVAL;
    }

    if (!rembufflen) {
        mloie_err("Remaining buffer length is zero");
        return QDF_STATUS_E_INVAL;
    }

    if ((subtype != IEEE80211_FC0_SUBTYPE_ASSOC_REQ) &&
        (subtype != IEEE80211_FC0_SUBTYPE_REASSOC_REQ) &&
        (subtype != IEEE80211_FC0_SUBTYPE_ASSOC_RESP) &&
        (subtype != IEEE80211_FC0_SUBTYPE_PROBE_REQ) &&
        (subtype != IEEE80211_FC0_SUBTYPE_PROBE_RESP) &&
        (subtype != IEEE80211_FC0_SUBTYPE_REASSOC_RESP)) {
        mloie_err("Subtype %u not supported or not valid", subtype);
        return QDF_STATUS_E_NOSUPPORT;
    }

    vap = ni->ni_vap;
    if (vap == NULL) {
        mloie_err("vap is NULL");
        return QDF_STATUS_E_INVAL;
    }

    ic = vap->iv_ic;
    if (ic == NULL) {
        mloie_err("ic is NULL");
        return QDF_STATUS_E_INVAL;
    }

    vdev = vap->vdev_obj;
    if (vdev == NULL) {
        mloie_err("vdev is NULL");
        return QDF_STATUS_E_INVAL;
    }

    if (!wlan_vdev_mlme_is_mlo_vdev(vdev)) {
        mloie_err("VDEV is not part of an MLD");
        return QDF_STATUS_E_INVAL;
    }

    /* Use a shared utility function to parse through all fragments if
     * applicable and determine the Multi-Link sequence length, and also to
     * validate that the element we have been passed is indeed a Multi-Link
     * element. Since we already have the starting position of the Multi-Link
     * element, the execution of the utility function is shortened.
     */
    currpos = mlie;

    ret = util_find_mlie(currpos, rembufflen, &ieptr, &ieseqlen);
    if (ret != QDF_STATUS_SUCCESS) {
        mloie_err_rl("Error when trying to determine Multi-Link element presence and sequence length for ni with MAC address %s",
                     ether_sprintf(ni->ni_macaddr));
        return ret;
    }

    /* In case a Multi-Link element is not found, ieptr will be NULL and will
     * not match currpos. We return an error.
     *
     * Alternatively if a Multi-Link element is found but at a different
     * location than claimed by the caller, then this indicates an issue at
     * caller. This condition too will result in ieptr not matching currpos, and
     * we return an error.
     */
    if (currpos != ieptr) {
        mloie_err_rl("No Multi-Link element found at pointer to Multi-Link element for ni with MAC address %s",
                     ether_sprintf(ni->ni_macaddr));
        return QDF_STATUS_E_INVAL;
    }

    /* Sanity test the IE sequence length returned, as an additional guard */
    if (!ieseqlen) {
        mloie_err("Zero element length reported when trying to validate Multi-Link element pointer for ni with MAC address %s. Investigate!",
                  ether_sprintf(ni->ni_macaddr));
        return QDF_STATUS_E_FAILURE;
    }

    mlieseqlen = ieseqlen;

    ret = util_get_mlie_variant(mlie, mlieseqlen, (int *)&variant);
    if (ret != QDF_STATUS_SUCCESS) {
        mloie_err_rl("Error when trying to determine Multi-Link element variant for ni with MAC address %s",
                     ether_sprintf(ni->ni_macaddr));
        return ret;
    }

    if (!((variant == WLAN_ML_VARIANT_BASIC) || (variant == WLAN_ML_VARIANT_PROBEREQ))) {
        mloie_err_rl("Unexpected variant %u of Multi-Link element in subtype %u for ni with MAC address %s",
                     variant, subtype, ether_sprintf(ni->ni_macaddr));
        return QDF_STATUS_E_PROTO;
    }

    if ((subtype == IEEE80211_FC0_SUBTYPE_ASSOC_REQ) ||
        (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ)) {
        ret = util_get_bvmlie_mldmacaddr(mlie, mlieseqlen, &mldmacaddr);
        if (ret != QDF_STATUS_SUCCESS) {
            mloie_err_rl("Error when trying to get MLD MAC address in Multi-Link element for ni with MAC address %s",
                         ether_sprintf(ni->ni_macaddr));
            return ret;
        }

        qdf_mem_copy(ni->peer_obj->mldaddr, mldmacaddr.bytes,
                     sizeof(ni->peer_obj->mldaddr));

        ret = util_get_bvmlie_eml_cap(mlie, mlieseqlen, &eml_cap_found,
                                      &eml_cap);
        if (ret != QDF_STATUS_SUCCESS) {
            mloie_err_rl("Error when trying to get EML Cap in Multi-Link element for ni with MAC address %s",
                         ether_sprintf(ni->ni_macaddr));
            return ret;
        }

        if (eml_cap_found)
            ieee80211_parse_peer_eml_cap(ni, eml_cap);
        else
            mloie_debug_rl("EML Cap not found in Multi-Link element in subtype %u for ni with MAC address %s",
                           subtype, ether_sprintf(ni->ni_macaddr));

        ret = util_get_bvmlie_mldcap(mlie, mlieseqlen, &mldcapfound, &mldcap);
        if (ret != QDF_STATUS_SUCCESS) {
            mloie_err_rl("Error when trying to get MLD capabilities in Multi-Link element for ni with MAC address %s",
                    ether_sprintf(ni->ni_macaddr));
            return ret;
        }

        if (mldcapfound) {
            ieee80211_parse_peer_mld_cap(ni, mldcap);
            ieee80211_parse_t2lm_enable_val(ni, mldcap);
        }

        ret = util_get_bvmlie_persta_partner_info(mlie, mlieseqlen,
                                                  &ni->ml_info);
        if (ret != QDF_STATUS_SUCCESS) {
            mloie_err_rl("Error when trying to determine Multi-Link partner info for ni with MAC address %s",
                         ether_sprintf(ni->ni_macaddr));
            return ret;
        }

        /* Add link information for the reported STA. This is currently required
         * only at the AP side, outside of scan.
         */

        ni->ml_info.num_partner_links++;
        rptinglinkidx = ni->ml_info.num_partner_links - 1;

        ni->ml_info.partner_link_info[rptinglinkidx].link_id =
            wlan_vdev_get_link_id(vdev);
        memcpy(ni->ml_info.partner_link_info[rptinglinkidx].link_addr.bytes,
                ni->ni_macaddr,
                sizeof(ni->ml_info.partner_link_info[rptinglinkidx].link_addr.bytes));
    }

    if (subtype == IEEE80211_FC0_SUBTYPE_PROBE_REQ) {
        ret = util_get_prvmlie_mldid(mlie, mlieseqlen, &mldidfound, &mldid);

        /* MLDID is not expected for the below request */
        if (mldidfound && (!ni->ml_preq_info.is_mld_id_valid)) {
            mloie_err_rl("Error MLDID should not be present if ML probe req for non-tx vap, drop ni is %s",
                    ether_sprintf(ni->ni_macaddr));
            return QDF_STATUS_E_FAILURE;

        }

        /* MLDID is expected for the below request */
        if (!mldidfound && (ni->ml_preq_info.is_mld_id_valid)) {
            mloie_err_rl("Error MLDID should be present if ML probe req is directed to tx vap, drop ni is %s",
                    ether_sprintf(ni->ni_macaddr));
            return QDF_STATUS_E_FAILURE;

        }

        /* For MBSSID disable case treat only 0 as valid MLDID */
        if (!(ic->is_mbssid_enabled) && (mldid != 0)) {
            mloie_err_rl("Error MLDID %d should be 0 MBSSID disabled vaps, drop ni is %s", mldid ,
                    ether_sprintf(ni->ni_macaddr));
            return QDF_STATUS_E_FAILURE;

        }

        if (ret != QDF_STATUS_SUCCESS) {
            mloie_err_rl("Error when trying to get MLD ID in Multi-Link element for ni with MAC address %s",
                    ether_sprintf(ni->ni_macaddr));
            return ret;
        }

        if (mldidfound)
            ni->ml_preq_info.mlid = mldid;

        ret = util_get_prvmlie_persta_link_id(mlie, mlieseqlen,
                                                  &ni->ml_preq_info);
        if (ret != QDF_STATUS_SUCCESS) {
            mloie_err_rl("Error when trying to determine Multi-Link partner info for ni with MAC address %s",
                         ether_sprintf(ni->ni_macaddr));
            return ret;
        }
    }

    /* Note: Other IEEE802.11be D1.0 MLD information will be filled in an
     * upcoming change. Thus, this function does not currently do much of value
     * for assoc response.
     */
    if (subtype != IEEE80211_FC0_SUBTYPE_PROBE_REQ)
        ni->ni_ml_rptingrole = IEEE80211_MLO_RPTINGROLE_REPORTINGSTA;

    return QDF_STATUS_SUCCESS;
}

static
u_int8_t ieee80211_is_space_avl_for_one_mlo_entry(uint16_t len_cur_rnr_ie)
{
    return ((RNR_NBR_AP_INFO_HDR_SIZE + MLO_TBTT_INFO_FIELD_SIZE) <=
                (IEEE80211_MAX_IE_LEN - len_cur_rnr_ie));
}

/**
 * ieee80211_get_6g_20mhz_psd() - Get 20 MHz psd power for 6 GHz channel
 * @vap: Pointer to struct ieee80211vap
 *
 * Return: PSD power
 */
static int8_t
ieee80211_get_6g_20mhz_psd(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    bool is_psd_pwr;
    uint16_t max_reg_eirp_pwr = 0;
    uint16_t max_reg_eirp_psd_pwr = 0;
    int8_t psd_20mhz;

    wlan_reg_get_client_power_for_6ghz_ap(ic->ic_pdev_obj,
                                          REG_DEFAULT_CLIENT,
                                          vap->iv_bsschan->ic_freq,
                                          &is_psd_pwr, &max_reg_eirp_pwr,
                                          &max_reg_eirp_psd_pwr);
    if (is_psd_pwr) {
        psd_20mhz = (int8_t)max_reg_eirp_psd_pwr * 2;
    } else {
        psd_20mhz = REG_PSD_MAX_TXPOWER_FOR_DEFAULT_CLIENT * 2;
    }
    return psd_20mhz;
}

/**
 * ieee80211_get_non6g_20mhz_psd() - Get 20 MHz psd power for 5/2.4 GHz channel
 * @vap: Pointer to struct ieee80211vap
 *
 * Return: PSD power
 */
static int8_t
ieee80211_get_non6g_20mhz_psd(struct ieee80211vap *vap)
{
    int16_t eirp = vap->iv_bsschan->ic_maxregpower;
    uint16_t ch_bw;
    enum ieee80211_cwm_width cwm_width;
    enum phy_ch_width phy_chan_width;
    int16_t psd_20mhz_value;
    enum ieee80211_phymode phymode = ieee80211_chan2mode(vap->iv_bsschan);
    struct ieee80211com *ic = vap->iv_ic;
    int8_t psd_20mhz;

    cwm_width = get_chwidth_phymode(phymode);
    if (cwm_width == IEEE80211_CWM_WIDTHINVALID)
        cwm_width = ic->ic_cwm_get_width(ic);

    phy_chan_width =  ieee80211_convert_cwm_wd_to_phych_width(cwm_width);
    ch_bw = wlan_reg_get_bw_value(phy_chan_width);
    wlan_reg_eirp_2_psd(ic->ic_pdev_obj, ch_bw, eirp, &psd_20mhz_value);
    psd_20mhz = (int8_t)psd_20mhz_value * 2;

    return psd_20mhz;
}

/**
 * ieee80211_get_20mhz_psd() - For the given vap, find the PSD power
 * of the bss operating channel.
 * @vap: Pointer to struct ieee80211vap
 * Return - PSD power value
 */
static int8_t ieee80211_get_20mhz_psd(struct ieee80211vap *vap)
{
    if (IEEE80211_IS_CHAN_6GHZ(vap->iv_bsschan)) {
        return ieee80211_get_6g_20mhz_psd(vap);
    } else {
        return ieee80211_get_non6g_20mhz_psd(vap);
    }
}

/* 1. If vap is 6Ghz then it can use this function as it is.
 * 2. If vap is lowerband then it needs to skip the 6Ghz partner as that will
 * get added as part of OOB requirement with right mld-id.
 * 3. This fn. should add the remaining partners with right neighbor ap-info
 * at the end of the current RNR IE (either RNR IE1 or RNR IE2)
 */
u_int8_t* ieee80211_add_basic_mlo_rnr_ie(u_int8_t *frm,
                                         struct ieee80211_beacon_offsets *bo,
                                         struct ieee80211vap *vap,
                                         u_int8_t **temp_bo, u_int8_t subtype,
                                         struct ieee80211_rx_status *rs,
                                         bool is_20tu_prb)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_vdev *co_mld_vdev = NULL;
    struct ieee80211vap *co_mld_vap = NULL;
    struct ieee80211com *co_mld_ic = NULL;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS];
    ieee80211_mlo_rnr_nbr_ap_info_ie_t *nbr_ap_info = NULL;
    struct ieee80211_rnr_mld_tbtt_info *mld_tbtt_info = NULL;
    uint16_t chwidth, behav_lim;
    uint8_t op_class, prim_chan;
    uint16_t vdev_count = 0;
    int16_t non_6ghz_vdev_count = 0, six_ghz_vdev_count = 0;
    uint16_t i = 0;
    uint16_t populatedlen = 0;
    uint32_t mld_params = 0;
    uint16_t nbr_ap_info_hdr_params = 0;
    uint8_t *frm_rnr_start, *frm_rnr_end;
    uint16_t len_cur_rnr_ie = 0;
    struct rnr_ie_ctx *rnr_ctx;
    struct ieee80211vap *rnr_vap = NULL;
    uint8_t curr_rnr_ie = 0;
    uint8_t  bpcc = 0;
    struct ieee80211_ath_channel chan;
    bool is_link_disabled = false;
    bool continue_current_cac = false;
    struct wlan_objmgr_psoc *psoc = NULL;
    uint16_t release_ref_start_idx;
    struct wlan_mlo_dev_context *mlo_dev_ctx;
    struct ieee80211com *ic;

    /*
     * Note: This function adds RNR IE for MLO for the basic case where only
     * other APs in the same MLD as the given VAP are reported. Other cases
     * involving MBO, MBSSID/Co-hosted including mix of MLD and non-MLD APs will
     * be handled separately.
     */

    if (frm == NULL) {
        mloie_err("frm is NULL");
        goto err;
    }

    if (vap == NULL) {
        mloie_err("vap is NULL");
        goto err;
    }

    ic = vap->iv_ic;
    if (!ic) {
        mloie_err("ic is NULL");
        goto err;
    }

    mloie_debug("vdev_id: %d %s", vap->iv_unit, ether_sprintf(vap->iv_myaddr));

    if ((subtype != IEEE80211_FC0_SUBTYPE_PROBE_RESP) &&
            (subtype != IEEE80211_FC0_SUBTYPE_BEACON)) {
        mloie_err("Inapplicable subtype %u", subtype);
        return frm;
    }

    vdev = vap->vdev_obj;
    if (vdev == NULL) {
        mloie_err("vdev is NULL");
        goto err;
    }

    if (!wlan_vdev_mlme_is_mlo_ap(vdev)) {
        mloie_err("VDEV is not part of an AP-MLD");
        return frm;
    }

    if (vap->rnr_enable) {
        mloie_err("This function is not applicable to cases where OCE RNR functionality is enabled");
        return frm;
    }

    ieee80211_mlo_ap_get_beaconing_vdev_list(vdev, &vdev_count, wlan_vdev_list);
    if (!vdev_count) {
        mloie_err("Number of VDEVs under AP-MLD is reported as 0");
        return frm;
    }

    /* Note: Once API changes to mlo_ap_get_vdev_list() are available to handle
     * array size requirements, remove this condition. As of now, the size of
     * wlan_vdev_list, WLAN_UMAC_MLO_MAX_VDEVS is the same as that of the source
     * array used in mlo_ap_get_vdev_list(), and the API does check to ensure it
     * doesn't exceed the size of the source array. Hence OOB access is not
     * expected to occur.
     * The below is added as an additional guard for completeness.
     */
    if (vdev_count > QDF_ARRAY_SIZE(wlan_vdev_list)) {
        mloie_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu, investigate",
                 vdev_count, QDF_ARRAY_SIZE(wlan_vdev_list));

        goto err_all_vdev_refs_held;
    }

    if (vdev_count < WLAN_UMAC_REPORTED_MLO_IE_PROC_MIN_VDEVS) {
        mloie_debug_rl("Count of VDEVs %u under AP-MLD lesser than expected minimum %u for including reported info",
                 vdev_count, WLAN_UMAC_REPORTED_MLO_IE_PROC_MIN_VDEVS);

        if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                        QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count, 0)) !=
                    QDF_STATUS_SUCCESS) {
            /* This should not occur. If this occurs, would indicate larger issues.
             */
            mloie_err("Error while trying to release VDEV references - investigate. This could lead to system issues.");
        }

        return frm;
    }

    /* Carry out pre-checks on the co-MLD VDEVs. */
    for(i = 0; i < vdev_count; i++) {
        co_mld_vdev = wlan_vdev_list[i];
        if (co_mld_vdev == NULL) {
            mloie_err("VDEV in MLD VDEV list is NULL");

            goto err_all_vdev_refs_held;
        }

        if (co_mld_vdev == vdev)
            continue;

        co_mld_vap = wlan_vdev_get_vap(co_mld_vdev);
        if (co_mld_vap == NULL) {
            mloie_err("Co-MLD VAP is NULL");

            goto err_all_vdev_refs_held;
        }

        mloie_debug("co_ml_vdev_id: %d %s", co_mld_vap->iv_unit, ether_sprintf(co_mld_vap->iv_myaddr));

        co_mld_ic = co_mld_vap->iv_ic;
        if (co_mld_ic == NULL) {
            mloie_err("Co-MLD IC is NULL");

            goto err_all_vdev_refs_held;
        }

        if (IEEE80211_IS_CHAN_6GHZ(co_mld_ic->ic_curchan)) {
            /* Skip 6Ghz partner APs for a non-6hz reporting vap as they are already taken care
             * as part of OOB requirement.
             */
            if (IEEE80211_IS_CHAN_6GHZ(vap->iv_ic->ic_curchan))
                six_ghz_vdev_count++;
        } else {
            non_6ghz_vdev_count++;
        }
    }

    mloie_debug("non_6ghz_vdev_count: %d", non_6ghz_vdev_count);

    /* When the reporting vdev is non-6Ghz its 6Ghz co-mld vdevs have been already
     * added as part of ieee80211_add_oob_rnr_ie(). So, skip this function if there
     * is 6Ghz partner for a non-6Ghz reporting vdev. On the other hand, if the
     * reporting vdev is 6Ghz then this function needs to be exectued to add any
     * possible 6Ghz and non-6Ghz partners from other 6Ghz and non-6Ghz links in the system.
     */
    if (!non_6ghz_vdev_count) {
        if (!IEEE80211_IS_CHAN_6GHZ(vap->iv_ic->ic_curchan) || !six_ghz_vdev_count) {
            if(!IEEE80211_IS_CHAN_6GHZ(vap->iv_ic->ic_curchan))
                mloie_debug_rl("No non-6Ghz partner for a non-6Ghz reporting vap");
            else
                mloie_debug_rl("No non-6Ghz or 6Ghz partner for a 6Ghz reporting vap");

            if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                                QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count, 0)) !=
                            QDF_STATUS_SUCCESS) {
                /* This should not occur. If this occurs, would indicate larger issues.
                 */
                mloie_err("Error while trying to release VDEV references - investigate."
                           " This could lead to system issues.");
            }

            /* Non-fatal issue - return the frm pointer back */
            return frm;
        }
    }

    if (vap->iv_ic->is_mbssid_enabled) {

        if (!vap->iv_ic->ic_mbss_ctx) {
            mloie_err("mbss_ctx is null");
            if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                            QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count, 0)) !=
                    QDF_STATUS_SUCCESS) {
                mloie_err("Error while trying to release VDEV references - investigate."
                        " This could lead to system issues.");
            }
            return frm;
        }

        if (!IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
            rnr_vap = vap;
        } else {
            struct mbssid_info *ic_mbss = &vap->iv_ic->ic_mbss_ctx->ic_mbss[vap->iv_mbss.group_id];

            if (ic_mbss->transmit_vap) {
                rnr_vap = ic_mbss->transmit_vap;
            } else {
                mloie_err("transmit_vap is null for vdev:%d (%s)",vap->iv_unit, vap->iv_netdev_name);
                if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                                QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count, 0)) !=
                        QDF_STATUS_SUCCESS) {
                    mloie_err("Error while trying to release VDEV references - investigate."
                            " This could lead to system issues.");
                }
                return frm;
            }
        }

    } else {
        rnr_vap = vap;
    }
    rnr_ctx =  ieee80211_get_rnr_ctx(rnr_vap, subtype, is_20tu_prb);

    curr_rnr_ie = rnr_ctx->curr_rnr_ie;
    if (bo->bo_rnr && bo->bo_rnr != frm) {
        /* First RNR IE exists */
        qdf_assert(*temp_bo != NULL);
        if (!*temp_bo) {
            mloie_err("temp_bo can't be null at this point if First RNR IE exists");
            goto err_all_vdev_refs_held;
        }
        frm_rnr_start = rnr_ctx->curr_rnr_start_frm;

        if (rnr_ctx->curr_rnr_start_frm == NULL) {
            mloie_err("!!!!!!FATAL ERROR curr_rnr_start_frm is NULL !!!!!!!!");
            mloie_err("vap:%d (%s) rnr_vap:%d (%s) curr_rnr_ie:%d size:%d,"
                      "bo->bo_rnr:%pK, frm:%pK, length:%d, subtype:%d",
                      vap->iv_unit, vap->iv_netdev_name,
                      rnr_vap->iv_unit, rnr_vap->iv_netdev_name,
                      curr_rnr_ie, rnr_ctx->curr_rnr_available_space,
                      bo->bo_rnr, frm, (uint16_t)(bo->bo_rnr - frm), subtype);
            goto err_all_vdev_refs_held;
        }

        /* Retrieve length of current RNR IE */
        len_cur_rnr_ie = *(frm_rnr_start + IEEE80211_IE_LEN_OFFSET);
        mloie_debug_rl("length of current RNR IE: %u", len_cur_rnr_ie);

        if (!ieee80211_is_space_avl_for_one_mlo_entry(len_cur_rnr_ie)) {
            /* Current RNR element is not sufficient to accommodate
             * the first of the partner APs of the 'vap'. Introduce
             * new RNR element.
             */
            if ((rnr_ctx->curr_rnr_ie + 1) < RNR_MAX_SUPPORTED_ELMN_NO) {

                /* Start new RNR IE */
                *frm = IEEE80211_ELEMID_REDUCED_NBR_RPT;

                /* Start of new RNR IE */
                frm_rnr_start = frm;
                /* End of new RNR IE */
                frm_rnr_end = frm_rnr_start + IE_LEN_ID_LEN;

                /* Re-initialize 'len_cur_rnr_ie' */
                len_cur_rnr_ie = 0;

                curr_rnr_ie += 1;
                /* Update the rnr ie context */

                ieee80211_update_rnr_ie_ctx(rnr_vap, curr_rnr_ie,
                                            IEEE80211_MAX_IE_LEN, frm_rnr_start,
                                            subtype, is_20tu_prb);
            } else {
                mloie_err("Reached Max Supported RNR IE");
                mloie_err("No space to report co-mld vdev across the"
                          " allowed number of RNR elements, available space:%d",
                          rnr_ctx->curr_rnr_available_space);
                mloie_err("%d lower band co-mld vdevs could not be"
                          " reported", non_6ghz_vdev_count);
                mloie_err("%d 6Ghz co-mld vdev could not be reported",
                          six_ghz_vdev_count);
                goto err_all_vdev_refs_held;
            }
        } else {
            /* Start of current RNR IE */
            frm_rnr_start = rnr_ctx->curr_rnr_start_frm;

            /* End of current RNR IE */
            frm_rnr_end = frm_rnr_start + IE_LEN_ID_LEN + len_cur_rnr_ie;

            if (frm != frm_rnr_end) {
                mloie_err("!!!!!!!!!!!!!!!!!!!!FATAL ERROR!!!!!!!!!!!!!!!!!!!!");
                mloie_err("Invariance check frm != frm_rnr_end failed for current RNR IE :%d,"
                          "vap:%d (%s), rnr_vap:%d (%s)",
                          rnr_ctx->curr_rnr_ie, vap->iv_unit,
                          vap->iv_netdev_name, rnr_vap->iv_unit,
                          rnr_vap->iv_netdev_name);
                mloie_err("frm:%pK, frm_rnr_end:%pK, frm_rnr_start:%pK, length: %d, subtype: %d",
                          frm, frm_rnr_end, frm_rnr_start,
                          (uint16_t)(frm_rnr_end - frm),
                          subtype);
                goto err_all_vdev_refs_held;
            }
        }
    } else {
        /* First RNR IE does not exist */

        qdf_assert(*temp_bo == NULL);

        /* Initialize temp_bo */
        *temp_bo = frm;
        /* Initialize bo->bo_rnr */
        bo->bo_rnr = frm;

        /* Start new RNR IE */
        *frm = IEEE80211_ELEMID_REDUCED_NBR_RPT;

        /* Start of first RNR IE */
        frm_rnr_start = frm;
        /* End of first RNR IE */
        frm_rnr_end = frm_rnr_start + IE_LEN_ID_LEN;
        /* Update current rnr ie context */

        ieee80211_update_rnr_ie_ctx(rnr_vap, curr_rnr_ie, IEEE80211_MAX_IE_LEN,
                                    frm_rnr_start, subtype, is_20tu_prb);
    }

    for(i = 0; i < vdev_count; i++) {
        release_ref_start_idx = i;
        co_mld_vdev = wlan_vdev_list[i];

        if (co_mld_vdev == vdev) {
            mlo_release_vdev_ref(wlan_vdev_list[i]);
            continue;
        }

        co_mld_vap = wlan_vdev_get_vap(co_mld_vdev);
        co_mld_ic  = co_mld_vap->iv_ic;

        /* Skip the 6Ghz co-mld-vap if reporting vap is non-6Ghz
         * as it is added as part of OOB RNR reporting.
         */
        if (!IEEE80211_IS_CHAN_6GHZ(vap->iv_ic->ic_curchan)
                && IEEE80211_IS_CHAN_6GHZ(co_mld_vap->iv_ic->ic_curchan)) {
            mlo_release_vdev_ref(wlan_vdev_list[i]);
            continue;
        }

        psoc = wlan_vdev_get_psoc(co_mld_vdev);
        if(psoc == NULL) {
            mloie_err("psoc is NULL");
            goto err_release_refs;
        }

        if (cfg_get(psoc, CFG_OL_ENABLE_MLOADVERT_DEGRADE_ON_CAC) ==
                MLOADVERT_DEGRADE_ON_CAC_ENABLE &&
                IEEE80211_IS_CHAN_5GHZ(co_mld_ic->ic_curchan) &&
                (wlan_vdev_is_up(co_mld_vdev) != QDF_STATUS_SUCCESS) &&
                mlme_dfs_is_cac_required_on_dfs_curchan(co_mld_ic->ic_pdev_obj,
                    &continue_current_cac, co_mld_vap->restart_txn)) {
            mlo_release_vdev_ref(wlan_vdev_list[i]);
            non_6ghz_vdev_count--;
            continue;
        }

        nbr_ap_info = (ieee80211_mlo_rnr_nbr_ap_info_ie_t *)(frm_rnr_end);

        IEEE80211_SET_BITS(nbr_ap_info_hdr_params,
                    WLAN_RNR_NBR_AP_INFO_HDR_INFO_RESERVED, 0);

        IEEE80211_SET_BITS(nbr_ap_info_hdr_params,
                    WLAN_RNR_NBR_AP_INFO_HDR_INFO_FILTERED_NBR_AP, 0);

        IEEE80211_SET_BITS(nbr_ap_info_hdr_params,
                    WLAN_RNR_NBR_AP_INFO_HDR_INFO_FIELD_TYPE, 0);

        /* In the basic RNR case handled here, we will have one TBTT Information
         * field included in the TBTT Information Set field of the Neighbor AP
         * Information field. So the value we set is 1-1=0.
         */
        IEEE80211_SET_BITS(nbr_ap_info_hdr_params,
                    WLAN_RNR_NBR_AP_INFO_HDR_INFO_CNT, 0);

        if (ic->wfa_test_rnr)
            IEEE80211_SET_BITS(nbr_ap_info_hdr_params,
                               WLAN_RNR_NBR_AP_INFO_HDR_INFO_LEN,
                               sizeof(struct ieee80211_rnr_mld_tbtt_info) +
                               sizeof(uint32_t));
        else
            IEEE80211_SET_BITS(nbr_ap_info_hdr_params,
                               WLAN_RNR_NBR_AP_INFO_HDR_INFO_LEN,
                               sizeof(struct ieee80211_rnr_mld_tbtt_info));

        nbr_ap_info_hdr_params = htole16(nbr_ap_info_hdr_params);
        mloie_debug("nbr_ap_info_hdr_params: 0x%x", nbr_ap_info_hdr_params);

        OS_MEMCPY(&nbr_ap_info->hdr_info, &nbr_ap_info_hdr_params,
                  sizeof(nbr_ap_info->hdr_info));

        chan = ieee80211_find_std_channel(co_mld_ic, co_mld_ic->ic_curchan);
        if (!IEEE80211_IS_CHAN_VALID(&chan)) {
            qdf_err("Could not find std valid channel");
            chan = *(co_mld_ic->ic_curchan);
            goto err_release_refs;
        }

        wlan_get_bw_and_behav_limit(&chan, &chwidth,
                &behav_lim);
        if (IEEE80211_IS_CHAN_6GHZ(co_mld_vap->iv_ic->ic_curchan)) {
            /* 11BE Draft4.0 requirement:
             * Reduce 320MHz channel width to downgraded opclass in 6GHz RNR tbtt info.
             */
            ieee80211_downgrade_320mhz_chwidth_rnr_opclass(co_mld_ic, &chwidth);
        }
        wlan_reg_freq_width_to_chan_op_class(co_mld_ic->ic_pdev_obj,
                chan.ic_freq, chwidth, true, behav_lim,
                            &op_class, &prim_chan);

        nbr_ap_info->op_class = op_class;
        nbr_ap_info->channel  = prim_chan;

        populatedlen += RNR_NBR_AP_INFO_HDR_SIZE;

        /* Note: The definition of ieee80211_rnr_nbr_ap_info_t may be modified
         * to reflect the possibility of tbtt_info pointing to
         * ieee80211_rnr_mld_tbtt_info in future changes for MBSSID. Currently,
         * ieee80211_rnr_nbr_ap_info_t is kept undisturbed while we await
         * stabilization in the standard.
         */
        mld_tbtt_info = (struct ieee80211_rnr_mld_tbtt_info *) (frm_rnr_end + populatedlen);

        /* Set TBTT offset value as unknown (255) in both beacon and
         * probe response frames in MLO case. This needs to be revisited
         * in future.
         */
        mld_tbtt_info->tbtt_offset = RNR_TBTT_OFFSET_UNKNOWN;

        IEEE80211_ADDR_COPY(mld_tbtt_info->bssid, co_mld_vap->iv_myaddr);
        mloie_debug("copying bssid: %d %s", co_mld_vap->iv_unit, ether_sprintf(co_mld_vap->iv_myaddr));

        mld_tbtt_info->short_ssid =
            htole32(ieee80211_construct_shortssid(co_mld_vap->iv_bss->ni_essid,
                    co_mld_vap->iv_bss->ni_esslen));

        /*
         * Note: OCT configuration below to be revisited when MLO OCT changes
         * made if required.
         */
        mld_tbtt_info->bss_params.oct_recommended = 0;

        /* All VAPs in the same MLD have the same SSID */
        mld_tbtt_info->bss_params.same_ssid = 1;

        if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
            struct ieee80211vap *tx_vap;
            struct mbssid_info  *ic_mbss;
            uint32_t short_ssid_rptg_vap;

            if(!ic->ic_mbss_ctx) {
                mloie_err("ic_mbss_ctx is null!!");
                goto err_release_refs;
            }
            ic_mbss = &ic->ic_mbss_ctx->ic_mbss[vap->iv_mbss.group_id];

            if (!ic_mbss->transmit_vap) {
                mloie_err("transmit_vap is null!!");
                goto err_release_refs;
            }
            tx_vap = ic_mbss->transmit_vap;

            /* co_mld_vap is the partner vap of the non-Tx vap.
             * But Tx-vap is the reporting vap. So, ssid-match
             * will be checked against the Tx-vap
             */
            short_ssid_rptg_vap =
                        htole32(ieee80211_construct_shortssid(
                        tx_vap->iv_bss->ni_essid,
                        tx_vap->iv_bss->ni_esslen));

            if(mld_tbtt_info->short_ssid != short_ssid_rptg_vap) {
                mld_tbtt_info->bss_params.same_ssid = 0;
            }
        }

        mld_tbtt_info->bss_params.mbssid_set = co_mld_vap->iv_ic->is_mbssid_enabled;
        mld_tbtt_info->bss_params.tx_bssid = mld_tbtt_info->bss_params.mbssid_set ?
                                !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap) : 0;

        /* Note: The below will be implemented in a later change */
        mld_tbtt_info->bss_params.colocated_lower_band_ess = 0;

        /* Note: The below will be implemented in a later change */
        mld_tbtt_info->bss_params.probe_resp_20tu_active = 0;

        mld_tbtt_info->bss_params.co_located_ap = 1;

        mld_tbtt_info->bss_params.reserved = 0;

        /* Fill the PSD value of the neighbour AP's operating channel */
        mld_tbtt_info->psd_20mhz =  ieee80211_get_20mhz_psd(co_mld_vap);

        if (!IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
            IEEE80211_SET_BITS(mld_params,
                    WLAN_RNR_NBRAPINFO_TBTTINFO_MLDPARAMS_MLDID, 0);
        } else {
            /* MLD-id of the reported partner will be the bssid-idx
             * of the non-Tx vap. Note that reporting vap is still the
             * Tx-vap
             */
            IEEE80211_SET_BITS(mld_params,
                    WLAN_RNR_NBRAPINFO_TBTTINFO_MLDPARAMS_MLDID,
                    vap->vdev_mlme->mgmt.mbss_11ax.profile_idx);
        }

        IEEE80211_SET_BITS(mld_params,
                WLAN_RNR_NBRAPINFO_TBTTINFO_MLDPARAMS_LINKID,
                wlan_vdev_get_link_id(co_mld_vdev));

        /* In Beacon and Probe Response frames, the BSS Parameters Change Count (BPCC)
         * for each of the other AP affiliated with the AP MLD shall be carried in
         * the MLD Params in the TBTT Information field of the RNR element
         * corresponding to that AP where each of the other AP is identified by
         * the Link ID subfield of the MLD Params.
         *
         * BPCC update for beacon takes care by FW part of Critical update handling.
         *
         * For probe response, update the respective VDEVs BPCC values which
         * are received from FW in probe request.
         */
        if (rs && subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP) {
            bpcc = iee80211_cu_get_bssparamchangecnt(co_mld_vap, rs);
        } else {
            bpcc = 0;
        }

        IEEE80211_SET_BITS(mld_params,
                WLAN_RNR_NBRAPINFO_TBTTINFO_MLDPARAMS_BSSPARAMCHANGECNT,
                bpcc);

        mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(co_mld_vdev);
        if (mlo_dev_ctx) {
            is_link_disabled =
                (mlo_dev_ctx->t2lm_ctx.established_t2lm.disabled_link_bitmap &
                 BIT(co_mld_vdev->vdev_mlme.mlo_link_id)) ? true : false;
        }

        IEEE80211_SET_BITS(mld_params,
                WLAN_RNR_NBRAPINFO_TBTTINFO_MLDPARAMS_DISABLEDLINKIND,
                is_link_disabled);

        mld_params = htole32(mld_params);
        mloie_debug("mld_params: 0x%x", mld_params);

        OS_MEMCPY(&mld_tbtt_info->mld_params, &mld_params,
                sizeof(mld_tbtt_info->mld_params));

        populatedlen += sizeof(*mld_tbtt_info);

        mlo_release_vdev_ref(wlan_vdev_list[i]);

        /* Update length of current RNR element */
        len_cur_rnr_ie += populatedlen;
        *(frm_rnr_start + IEEE80211_IE_LEN_OFFSET) = len_cur_rnr_ie;

        /* Retrieve new 'frm_rnr_end' */
        frm_rnr_end = frm_rnr_start + IE_LEN_ID_LEN + len_cur_rnr_ie;

        if (ic->wfa_test_rnr) {
            uint32_t rsvd_data = 0xABCDABCD;

            qdf_mem_copy(frm_rnr_end, &rsvd_data, sizeof(uint32_t));
            *(frm_rnr_start + IEEE80211_IE_LEN_OFFSET) += sizeof(uint32_t);
            frm_rnr_end += sizeof(uint32_t);
            len_cur_rnr_ie += sizeof(uint32_t);
        }

        /* Refresh frame pointer */
        frm = frm_rnr_end;

        if (!IEEE80211_IS_CHAN_6GHZ(co_mld_vap->iv_ic->ic_curchan)) {
            if (non_6ghz_vdev_count) {
                /* Number of non_6ghz_vdev to be added goes down by 1 */
                non_6ghz_vdev_count--;
            }
        } else {
            if (six_ghz_vdev_count) {
                /* Number of 6ghz_vdev to be added goes down by 1 */
                six_ghz_vdev_count--;
            }
        }

        /* Update current rnr ie context */
        ieee80211_update_rnr_ie_ctx(rnr_vap, curr_rnr_ie,
                                    (IEEE80211_MAX_IE_LEN - len_cur_rnr_ie),
                                    frm_rnr_start, subtype, is_20tu_prb);

        if ((non_6ghz_vdev_count || six_ghz_vdev_count) &&
                !ieee80211_is_space_avl_for_one_mlo_entry(len_cur_rnr_ie)) {
            /* Current RNR element is not sufficient to accommodate
             * the first of the partner APs of the 'vap'. Introduce
             * new RNR element.
             */
            curr_rnr_ie += 1;

            /* Start a new RNR element */
            *frm = IEEE80211_ELEMID_REDUCED_NBR_RPT;

            /* Start of new RNR IE */
            frm_rnr_start = frm;
            /* End of new RNR IE */
            frm_rnr_end = frm_rnr_start + IE_LEN_ID_LEN;

            /* Re-initialize 'len_cur_rnr_ie' */
            len_cur_rnr_ie = 0;

            /* Update current rnr ie context */
            ieee80211_update_rnr_ie_ctx(rnr_vap, curr_rnr_ie, IEEE80211_MAX_IE_LEN,
                                        frm_rnr_start, subtype, is_20tu_prb);
        }

        /* Re-initialize 'populatedlen' */
        populatedlen = 0;
    }

    return frm;

err_all_vdev_refs_held:
    if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                        QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count, 0)) !=
                    QDF_STATUS_SUCCESS) {
        /* This should not occur. If this occurs, would indicate larger issues.
         */
        mloie_err("Error while trying to release VDEV references - investigate. This could lead to system issues.");
    }
    return NULL;

err_release_refs:
    if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                    QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count,
                    release_ref_start_idx)) != QDF_STATUS_SUCCESS) {
        /* This should not occur. If this occurs, would indicate larger issues.
         */
        mloie_err("Error while trying to release VDEV references - investigate.");
    }

err:
    return NULL;
}

u_int8_t* ieee80211_add_basic_mlo_rnr_ie_wrapper(u_int8_t *frm,
        struct ieee80211_beacon_offsets *bo,
        struct ieee80211vap *vap, u_int8_t **temp_bo, u_int8_t subtype,
        struct ieee80211_rx_status *rs, bool is_20tu_prb)
{
    struct mlo_rnr_param param = {0};

    if (IEEE80211_IS_CHAN_6GHZ(vap->iv_ic->ic_curchan) || vap->iv_ic->is_mbssid_enabled) {
        mloie_debug(">>");
        param.frm = frm;
        param.bo  = bo;
        param.vap = vap;
        param.temp_bo = (uint8_t *) temp_bo;
        param.subtype = subtype;
        param.rs = rs;
        param.is_20tu_prb = (uint8_t) is_20tu_prb;

        /* Same beacon corresponding to the MBSSID group
         * of this vap must carry the partner-link info
         * of all the vaps in that group including Tx &
         * non-Tx.
         *
         * Call ieee80211_add_basic_mlo_rnr_ie() for each
         * vap in that particular MBSSID group
         */
        wlan_do_iter_fun_mbss_group(vap->iv_ic,
             vap->iv_mbss.group_id,
             MBSS_ITER_FUNC_PURPOSE_ADD_MLO_RNR, &param);
    } else {
        if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj)) {
            return ieee80211_add_basic_mlo_rnr_ie(frm, bo, vap, temp_bo, subtype,
                                                  rs, is_20tu_prb);
        } else
            return frm;
    }

    mloie_debug("param.frm: %px", param.frm);
    mloie_debug("bo->bo_rnr: %px", bo->bo_rnr);
    mloie_debug("temp_bo: %px", *temp_bo);
    mloie_debug("<<");
    return param.frm;
}

QDF_STATUS ieee80211_extract_rxs_mlreconf_linkremoval_info(
        struct ieee80211_node *ni, struct ieee80211com *ic,
        struct ieee80211_rx_status *rs,
        struct ieee80211_mlreconfreq_removetimer *mlreconfreq_removetimer)
{
    struct ieee80211vap *vap = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_vdev *co_mld_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    bool co_mld_vdev_processed[QDF_ARRAY_SIZE(co_mld_vdev_list)] = {false};
    uint16_t co_mld_hw_link_id[QDF_ARRAY_SIZE(co_mld_vdev_list)] =
                                                    {INVALID_HW_LINK_ID};
    struct wlan_objmgr_vdev *co_mld_vdev = NULL;
    struct wlan_objmgr_pdev *co_mld_pdev = NULL;
    uint16_t vdev_count = 0;
    int i = 0;
    uint16_t j = 0;
    QDF_STATUS ret;

    if (ni == NULL) {
        mloie_err("ni is NULL");
        return QDF_STATUS_E_INVAL;
    }

    if (ic == NULL) {
        mloie_err("ic is NULL");
        return QDF_STATUS_E_INVAL;
    }

    vap = ni->ni_vap;
    if (vap == NULL) {
        mloie_err("vap is NULL");
        return QDF_STATUS_E_INVAL;
    }

    vdev = vap->vdev_obj;
    if (vdev == NULL) {
        mloie_err("vdev is NULL");
        return QDF_STATUS_E_INVAL;
    }

    if (rs == NULL) {
        mloie_err("Rx status is NULL");
        return QDF_STATUS_E_INVAL;
    }

    if (mlreconfreq_removetimer == NULL) {
        mloie_err("Pointer to Reconf ML IE population request is NULL");
        return QDF_STATUS_E_INVAL;
    }

    if (!wlan_vdev_mlme_is_mlo_ap(vdev)) {
        mloie_err("VDEV is not part of an AP MLD");
        return QDF_STATUS_E_INVAL;
    }

    qdf_mem_zero(mlreconfreq_removetimer, sizeof(*mlreconfreq_removetimer));

    if (!rs->num_link_removal_info) {
        return QDF_STATUS_SUCCESS;
    }

    if (rs->link_removal_info == NULL) {
        mloie_err("Pointer to link removal info is NULL though number of link removal entries indicated is %d",
                rs->num_link_removal_info);
        return QDF_STATUS_E_INVAL;
    }

    /* Note: There will be a subsequent change to transition to an API that
     * returns only those AP MLD VDEVs which are active.
     */
    mlo_ap_get_vdev_list(vdev, &vdev_count, co_mld_vdev_list);

    if (!vdev_count) {
         mloie_err("Number of VDEVs under AP MLD is reported as 0");
         return QDF_STATUS_E_FAILURE;
    }

    /* Note: Once API changes to mlo_ap_get_vdev_list() are available to handle
     * array size requirements, remove this condition.
     */
    if (vdev_count > QDF_ARRAY_SIZE(co_mld_vdev_list)) {
         mloie_err("Count of VDEVs %u under MLD exceeds max entries in VDEV list %zu, investigate",
                 vdev_count, QDF_ARRAY_SIZE(co_mld_vdev_list));
         ret = QDF_STATUS_E_FAILURE;
         goto err_release_refs;
    }

    /* Carry out pre-checks on the co MLD VDEVs and also validate and store HW
     * link IDs for each co MLD VDEV. */
    for (j = 0; j < vdev_count; j++) {
        co_mld_vdev = co_mld_vdev_list[j];
        if (co_mld_vdev == NULL) {
            mloie_err("Co MLD VDEV at list idx %u is NULL", j);
            ret = QDF_STATUS_E_NULL_VALUE;
            goto err_release_refs;
        }

        co_mld_pdev = wlan_vdev_get_pdev(co_mld_vdev);
        if (co_mld_pdev == NULL) {
            mloie_err("PDEV of co MLD VDEV at list idx %u is NULL", j);
            ret = QDF_STATUS_E_NULL_VALUE;
            goto err_release_refs;
        }

        co_mld_hw_link_id[j] = wlan_mlo_get_pdev_hw_link_id(co_mld_pdev);
        if (co_mld_hw_link_id[j] == INVALID_HW_LINK_ID) {
            mloie_err("PDEV of co MLD VDEV at list idx %u has invalid HW link ID",
                    j);
            ret = QDF_STATUS_E_FAILURE;
            goto err_release_refs;
        }

        /* Add any other pre-checks here if required in the future */
    }

    for (i = 0; i < rs->num_link_removal_info; i++)
    {
        mloie_debug("Processing link removal info entry idx %d with HW link id %u VDEV id %u TBTT count %u",
                i, rs->link_removal_info[i].hw_link_id,
                rs->link_removal_info[i].vdev_id,
                rs->link_removal_info[i].tbtt_count);

        for (j = 0; j < vdev_count; j++) {
            mloie_debug("Comparing with co MLD VDEV at list idx %u with HW link id %u VDEV id %u",
                    j, co_mld_hw_link_id[j],
                    wlan_vdev_get_id(co_mld_vdev_list[j]));

            if ((co_mld_hw_link_id[j] == rs->link_removal_info[i].hw_link_id) &&
                (wlan_vdev_get_id(co_mld_vdev_list[j]) ==
                                rs->link_removal_info[i].vdev_id)) {
                    if (co_mld_vdev_processed[j]) {
                        mloie_err("Duplicate entry found in link removal array idx %d for co MLD VDEV at list idx %u",
                                i, j);
                        ret = QDF_STATUS_E_INVAL;
                        goto err_release_refs;
                    }

                    break;
            }
        }

        if (j < vdev_count) {
            /* We have found a unique co-MLD VDEV matching the HW link ID and
             * VDEV ID of the current link removal info entry. Cross check
             * whether the TBTT count in the link removal info entry is non-zero
             * - a Reconfiguration Multi-Link element population request entry
             * is to be added only if the TBTT count is non-zero.
             */
            if (rs->link_removal_info[i].tbtt_count) {
                if (mlreconfreq_removetimer->num_links >=
                        QDF_ARRAY_SIZE(mlreconfreq_removetimer->link_info))
                {
                    mloie_err("Reconfig ML IE population req link_info array full: %u entries, co MLD VDEV list idx %u. Investigate.",
                                    mlreconfreq_removetimer->num_links, j);
                            ret = QDF_STATUS_E_NOMEM;
                            goto err_release_refs;

                }

                mlreconfreq_removetimer->link_info[mlreconfreq_removetimer->num_links].linkid =
                    wlan_vdev_get_link_id(co_mld_vdev_list[j]);
                mlreconfreq_removetimer->link_info[mlreconfreq_removetimer->num_links].removetimer_tbtt =
                    rs->link_removal_info[i].tbtt_count;
                mlreconfreq_removetimer->num_links++;
            }

            /* Note: If required later, we can add a change here to record
             * information about those links for which the TBTT count is 0. Such
             * links would have completed their removal countdown but would not
             * yet have been brought down.
             */

            co_mld_vdev_processed[j] = true;
        }
    }

    ret = QDF_STATUS_SUCCESS;

err_release_refs:
    if ((ieee80211_mlo_release_vdev_list_refs(co_mld_vdev_list,
                    QDF_ARRAY_SIZE(co_mld_vdev_list), vdev_count,
                    0)) != QDF_STATUS_SUCCESS) {
        /* This should not occur. If this occurs, would indicate larger issues.
         */
        mloie_err("Error while trying to release VDEV references - investigate.");
    }

    return ret;
}
#endif /* WLAN_FEATURE_11BE_MLO */
