/*
 * Copyright (c) 2017,2023 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 */

#if QCN_IE
#ifndef _UMAC_IEEE80211_QCN__
#define _UMAC_IEEE80211_QCN__

#include <ieee80211_var.h>
#include <ieee80211_node.h>

typedef enum {
    QCN_TRANSITION_REJECTION_UNSPECIFIED,
    QCN_TRANSITION_REJECTION_FRAME_LOSS,
    QCN_TRANSITION_REJECTION_EXCESSIVE_DELAY,
    QCN_TRANSITION_REJECTION_INSUFFICIENT_QOS,
    QCN_TRANSITION_REJECTION_LOW_RSSI,
    QCN_TRANSITION_REJECTION_INTERFERENCE,
    QCN_TRANSITION_REJECTION_GRAY_ZONE,
    QCN_TRANSITION_REJECTION_SERVICE_UNAVAIL,

/*Reserved values form 8-255 */
    QCN_TRANSITION_REJECTION_MAX = 255
}qcn_transition_rej_code;


typedef enum {
    QCN_TRANSITION_REASON_UNSPECIFIED,
    QCN_TRANSITION_REASON_FRAME_LOSS,
    QCN_TRANSITION_REASON_EXCESSIVE_DELAY,
    QCN_TRANSITION_REASON_INSUFFICIENT_BANDWIDTH,
    QCN_TRANSITION_REASON_LOAD_BALANCE,
    QCN_TRANSITION_REASON_LOW_RSSI,
    QCN_TRANSITION_REASON_RETRANSMISSION,
    QCN_TRANSITION_REASON_INTERFERENCE,
    QCN_TRANSITION_REASON_GRAY_ZONE,
    QCN_TRANSITION_REASON_PREMIUM_AP,

/*Reserved values form 10-255 */
    QCN_TRANSITION_REASON_MAX = 255
}qcn_transition_reason_code;

/* QCN type to identify attributes to be added to QCN IE
 */
typedef enum {
    QCN_TRANS_REASON_IE_TYPE        = 1,
    QCN_MAC_PHY_PARAM_IE_TYPE       = 2
}qcn_ie_type;
#define QCN_VER_ATTR_VER              0x01
#define QCN_VER_ATTR_SUBVERSION       0x00

#define VHT_MCS10_11_SUPP_ATTRIB_LEN            1
#define HE_400NS_SGI_SUPP_ATTRIB_LEN            3
#define HE_2XLTF_160_80P80_SUPP_ATTRIB_LEN      1
#define HE_DL_OFDMA_SUPP_ATTRIB_LEN             1
#define HE_DL_MUMIMO_SUPP_ATTRIB_LEN            1
#define HE_MCS12_13_SUPP_ATTRIB_LEN             2
#define REPEATER_INFO_ATTRIB_LEN                1
#define HE_240_MHZ_SUPP_ATTRIB_LEN              9
/*
 * @ieee80211_setup_qcn_ie_bstmreq_target() -  adds QCN  IE in BTM request packet
 * @vap:pointer to vap
 * @ie:location where we need to add ie
 * @bstm_reqinfo:points to structure for transition reason code passing
 * @macaddr:pointer to mac address of the node
 *
 * Return:pointer where next item should be populated
 */
u_int8_t* ieee80211_setup_qcn_ie_bstmreq_target(struct ieee80211vap *vap, u_int8_t *ie,
                                         struct ieee80211_bstm_reqinfo_target *bstm_reqinfo,u_int8_t *macaddr);

/*
 * @ieee80211_parse_240_mhz_qcnie() - parse QCN IE for 240 MHz support
 * @frm: Location where ie is present
 * @ic: ic context
 * @param: Data needs to be parsed into this
 *
 * Return: status
 */
int ieee80211_parse_240_mhz_qcnie(u_int8_t *frm, struct ieee80211com *ic, struct qcn_240_mhz_param *param);

/*
 * @ieee80211_parse_qcnie() -  parse QCN  trans_reason sub-attribute
 * @frm:tracks location where to attach next sub-attribute
 * @wh:frame pointer
 * @ni:node pointer
 * @data: pointer for sending the version info to the caller
 * subtype: frame subtype
 * subtype 0xFF indicates invalid subtype and will result in selection of a default log category
 *
 * Return:0 on success , -1 on failure
 */
int ieee80211_parse_qcnie(u_int8_t *frm, const struct ieee80211_frame *wh,
                    struct ieee80211_node *ni,u_int8_t * data, u_int8_t subtype);


/*
 * @ieee80211_add_qcn_info_ie() - adds QCN IE
 * @ie:ie pointer
 * @vap :pointer to vap struct
 * @ie_len:pointer to length that will be returned
 * @qcn_ie_type: QCN IE param type
 * @qcn_reqinfo:pointer to qcn request info

 * Return:New buffer pointer after the added IE
 */
u_int8_t *ieee80211_add_qcn_info_ie(u_int8_t *frm,
        struct ieee80211vap *vap, u_int16_t *ie_len,
        qcn_ie_type type, void *qcn_reqinfo);

u_int8_t *ieee80211_add_qcn_ie_mac_phy_params(u_int8_t *ie, struct ieee80211vap *vap);
#if QCN_ESP_IE
u_int8_t *ieee80211_add_esp_info_ie(u_int8_t *frm,
        struct ieee80211com *ic, u_int16_t *ie_len);

u_int8_t ieee80211_est_field_attribute(struct ieee80211com *ic, u_int8_t *frm);
#endif

#endif
/* Corresponding copy of this enum is maintained on FW common header file
   Don't change these values, unless FW common header enum defnitions are modified*/
enum qcn_bsscolor_rept_type {
    QCN_REPT_INVALID_TYPE           = 0, /* Invalid type*/
    QCN_REPT_ONLY_STAVAP            = 1, /* AP has only STAVAP and APVAP is not present on any radio*/
    QCN_REPT_APVAP_ON_OTHER_RADIO   = 2, /* AP has STAVAP on one radio and APVAP on other radios*/
    QCN_REPT_FH_APVAP_ON_SAME_RADIO = 3, /* AP has STAVAP and APVAP on same radio. During STAVAP connection,
                                            no repeater client is connected with this repeater APVAP */
    QCN_REPT_BH_APVAP_ON_SAME_RADIO = 4, /* AP has STAVAP and APVAP on same radio. During STAVAP connection,
                   at least one repeater client is connected with this repeater APVAP (daisy chain config)*/
};

#endif //QCN_IE
