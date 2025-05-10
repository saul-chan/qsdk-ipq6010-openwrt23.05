/*
 * Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 */

/**
 * DOC: This file contains declarations of routines for 802.11be Multi-Link
 * Operation (MLO) protocol processing.
 */
#ifndef _IEEE80211_PROTO_MLO_H_
#define _IEEE80211_PROTO_MLO_H_

#include <ieee80211_var.h>
#include <ieee80211.h>
#include <ieee80211_mlo.h>

#ifdef WLAN_FEATURE_11BE_MLO

/**
 * ieee80211_add_multilink_ie() - Add Multi-Link element, and corresponding
 * Fragment elements if applicable. Note that support for authentication frames,
 * and ML probe request/response frames is not yet included (but support for
 * regular probe response frames with Multi-Link element is included).
 *
 * @param mlbuf: Buffer/location within buffer into which which the element(s)
 * should be added. For uniformity with other similar APIs, this function
 * returns location in buffer just after the added element(s), hence the buffer
 * size allocated should factor this in and ensure the size is one more than the
 * expected total size of element(s).
 * @param mlbufsize: Size of buffer mlbuf OR number of bytes within buffer
 * available for adding element(s) + one extra byte, all starting with initial
 * location within buffer
 * @param containerfrm: Frame which will contain the Multi-Link element, and
 * which already has other IEs required for forming per-STA profile entries in
 * the Multi-Link element using inheritance rules (if applicable). Can be NULL
 * if the caller knows this is not applicable and chooses to optimize
 * accordingly (but the function will return an error if the caller makes this
 * determination incorrectly).
 * @param containerfrmsize: Size of containerfrm. Will be ignored if
 * containerfrm is not required or is NULL.
 * @param ni: Pointer to node structure corresponding to this element
 * @param ic: Pointer to iee80211com
 * @param subtype: Frame subtype
 * @param is_variant_specified: Whether a Multi Link element variant is
 * specified. If this is set to true, then the variant parameter should be
 * provided by the caller. If this is set to false, then the variant parameter
 * will be ignored and the API will apply a default applicable variant for the
 * given subtype (the defaults are currently Probe Request variant for probe
 * request subtype and Basic variant for all other valid subtypes).
 * @param variant: Multi-Link element variant specified. This must be a valid
 * variant for the given subtype in accordance with the 802.11be standard.
 * Currently only Basic variant, Probe Request variant, and Reconfiguration
 * variant are handled. This parameter is ignored if is_variant_specified is
 * false.
 * @param extractx: Extra framing context specific to individual subtypes. This
 * can be NULL except if is_variant_specified is true and the variant requested
 * is Reconfiguration variant. The applicable fields in the context are
 * fectx_is_ml_proberesp and ml_preq_info for probe response subtype, and
 * fectx_mlreconfreq_removetimer for beacon and probe response subtypes. See the
 * documentation of these fields for more information.
 * @param bo: Pointer to beacon offsets
 * @param rs: Pointer to receive status information associated with packet
 *
 * @return Location in buffer after the added element(s) on success, NULL on
 * error.
 */
u_int8_t* ieee80211_add_multilink_ie(u_int8_t *mlbuf, size_t mlbufsize,
        u_int8_t *containerfrm, size_t containerfrmsize,
        struct ieee80211_node *ni, struct ieee80211com *ic, u_int8_t subtype,
        bool is_variant_specified, enum wlan_ml_variant variant,
        struct ieee80211_framing_extractx *extractx,
        struct ieee80211_beacon_offsets *bo, struct ieee80211_rx_status *rs);

/*
 * ieee80211_parse_multilink_ie() - Parse Multi-Link information element
 *
 * @param ni: Pointer to node structure corresponding to this element
 * @param mlie: Pointer to the Multi-Link element
 * @param rembufflen: Remaining buffer length from the start of the Multi-Link
 * element till the end of the buffer containing the Multi-Link element, used to
 * ensure that the buffer size is not exceeded when processing Multi-Link
 * element fragments if any
 * @param subtype: Frame subtype
 *
 * @return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_INVAL on failure
 */
QDF_STATUS ieee80211_parse_multilink_ie(struct ieee80211_node *ni,
                                        u_int8_t *mlie, qdf_size_t rembufflen,
                                        u_int8_t subtype);

/**
 * ieee80211_add_basic_mlo_rnr_ie() - Add RNR IE for MLO for the basic case
 * where only other APs in the same MLD as the given VAP are reported.
 *
 * @param frm: Starting position in frame in which this element should be added
 * @param vap: Pointer to vap structure
 * @param subtype: Frame subtype
 *
 * @return Pointer to location in frm after the RNR IE in case of success, NULL
 * in the case of failure.
 */
u_int8_t* ieee80211_add_basic_mlo_rnr_ie(
        uint8_t *frm, struct ieee80211_beacon_offsets *bo,
        struct ieee80211vap *vap, u_int8_t **temp_bo, u_int8_t subtype,
        struct ieee80211_rx_status *rs, bool is_20tu_prb);

u_int8_t* ieee80211_add_basic_mlo_rnr_ie_wrapper(
        uint8_t *frm, struct ieee80211_beacon_offsets *bo,
        struct ieee80211vap *vap, u_int8_t **temp_bo, u_int8_t subtype,
        struct ieee80211_rx_status *rs, bool is_20tu_prb);

/**
 * ieee80211_parse_peer_eml_cap() - Helper function to
 * set EML capability in peer object after parsing EML capability in
 * Basic ML IE from Association request
 *
 * @ni: Pointer to node
 * @eml_cap: EML capability
 *
 * @return: Nothing
 */
void ieee80211_parse_peer_eml_cap(struct ieee80211_node *ni,
                                  uint16_t eml_cap);
/**
 * ieee80211_parse_peer_mld_cap() - Helper function to set MLD Cap
 * parsed from Assoc request sent by non-AP MLD.
 *
 * @ni: Pointer to node
 * @mldcap: MLD capability subfield parsed from ML IE in Assoc request
 *
 * @return: Nothing
 */
void ieee80211_parse_peer_mld_cap(struct ieee80211_node *ni,
                                  uint16_t mldcap);

/**
 * ieee80211_extract_rxs_mlreconf_linkremoval_info() - Extract Multi-Link
 * Reconfiguration link removal information from Rx status
 *
 * @param ni: Pointer to node structure
 * @param ic: Pointer to iee80211com
 * @param rs: Pointer to receive status information associated with packet
 * @param mlreconfreq_removetimer: Pointer to Reconfiguration Multi-Link element
 * population request which will be populated by the API if valid link removal
 * TBTT countdown information is found in the Rx status for one or more of the
 * VDEVs in the AP MLD which ni corresponds to. The population request will be
 * initially zeroed out by the API. If no such valid link removal TBTT countdown
 * information is found then the number of links indicated in the population
 * request will remain at 0 even if QDF_STATUS_SUCCESS is returned and in this
 * case the caller can ignore the population request.
 *
 * @return QDF_STATUS_SUCCESS on success, other QDF_STATUS value in case of
 * error.
 */
QDF_STATUS ieee80211_extract_rxs_mlreconf_linkremoval_info(
        struct ieee80211_node *ni, struct ieee80211com *ic,
        struct ieee80211_rx_status *rs,
        struct ieee80211_mlreconfreq_removetimer *mlreconfreq_removetimer);

/**
 * ieee80211_is_ml_probe_resp_expected() - Check if it is an ML probe response
 * and we are waiting for it.
 *
 * @vap: VAP handle
 * @wbuf: Rx mgmt frame buffer
 *
 * @return: true if we expect this ML probe response; false, otherwise
 */
bool ieee80211_is_ml_probe_resp_expected(struct ieee80211vap *vap, wbuf_t wbuf);
#endif /* WLAN_FEATURE_11BE_MLO */

#ifdef WLAN_FEATURE_11BE
/**
 * ieee80211_parse_t2lm_enable_val() - API to parse the T2LM value from MLD
 * capabilities and store it in ni structure.
 * @ni: Pointer to ni structure
 * @mldcap: peer MLD capabilities
 *
 * @return: none
 */
void ieee80211_parse_t2lm_enable_val(struct ieee80211_node *ni,
                                     uint16_t mldcap);
#else
static inline void ieee80211_parse_t2lm_enable_val(struct ieee80211_node *ni,
                                                   uint16_t mldcap)
{
    return;
}
#endif

#endif /* _IEEE80211_PROTO_MLO_H_ */
