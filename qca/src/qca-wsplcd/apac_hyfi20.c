/* @File: apac_hyfi20.c
 * @Notes:
 *
 * Copyright (c) 2011-2012, 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * 2011-2012 Qualcomm Atheros, Inc.
 * Qualcomm Atheros Confidential and Proprietary.
 * All rights reserved.
 *
 */

#include "wsplcd.h"
#include "eloop.h"
#include "apac_priv.h"
#include "apac_hyfi20_wps.h"
#include "apac_hyfi20_mib.h"
#include "ieee1905_vendor.h"
#if MAP_ENABLED
#include "apac_map.h"
#include "wsplcd.h"
#endif
#include "storage.h"
#include "wlanif_cmn.h"

#ifdef QCA_EXT_EM_CONTROLLER
#include <emex.h>
#include <emex_tlvs.h>
#endif

/* Number of seconds when we want to send APAC search in a shorter interval
 * Currently this is used in two cases:
 *   1. Look for registrar on all bands after Wi-Fi WPS success;
 *   2. Look for registrar on a different band after APAC completes on one band. */
const u16 APAC_SEARCH_SHORT_INTERVAL = 2;
u16 apac_cfg_apply_interval=10;
u16 apac_cfg_restart_short_interval=5;
u16 apac_cfg_restart_long_interval=20;
#if MAP_ENABLED
apac_alid_list alid_list;
#endif

#define APAC_NUM_CFG_APPLY_FRAMES 5

apacHyfi20GlobalState_t apacS;
apacHyfi20NodeTbl_t apacNodeTbl[APAC_MAXNUM_NTWK_NODES];

extern struct wlanif_config *wlanIfWd;

#define CFGMSG_VERSION_MAJOR_SHIFT 4
#define CFGMSG_VERSION_MINOR_SHIFT 0
#define CFGMSG_VERSION_COMPONENT_MASK 0xFF

/* Calculated by subtrating the max eth frame length supported by sum
 * of tlv sent with search message
 *
 * ETH_FRAME_LEN - 1514
 * Current FrameLen used in Search msg ~ 80 (include both SON AND MAP TLV)
 * MAX_PAYLOAD = 1514 - 80
 *
 * The MAX_PAYLOAD should be adjusted by developers who add new TLV to
 * search message. Decrease the current payload value with the size of
 * new tlv added.
 */
#define SEARCH_MSG_MAX_PAYLOAD 1434

/* CFG Major versions supported */
typedef enum cfgmsgMajorVersion_e {
    cfgmsgMajorVersion1 = 1
} cfgmsgMajorVersion_e;

/* ATF Minor versions supported */
typedef enum cfgmsgMinorVersion_e {
    cfgmsgMinorVersion0 = 0,
} cfgmsgMinorVersion_e;

/* NBTLV response rcvd values */
typedef enum nbtlvResponseRcvd_e {
    RESPONSE_NOT_RCVD = 0,
    RESPONSE_RCVD = 1
} nbtlvResponseRcvd_e;

/* NBTLV device roles */
typedef enum nbtlvDeviceRoles_e {
    NB_AGENT = 0,
    NB_CONTROLLER = 1
} nbtlvDeviceRoles_e;

// Pack the major/minor version numbers into a single value.
#define cfgmsgPackVersionNum(major, minor) \
    (((major & CFGMSG_VERSION_COMPONENT_MASK) \
        << CFGMSG_VERSION_MAJOR_SHIFT) | \
     ((minor & CFGMSG_VERSION_COMPONENT_MASK) \
        << CFGMSG_VERSION_MINOR_SHIFT))

// Extract the major and minor version numbers from the packed value.
#define cfgmsgExtractMajorVersionNum(version) \
    ((version >> CFGMSG_VERSION_MAJOR_SHIFT) \
        & CFGMSG_VERSION_COMPONENT_MASK)
#define cfgmsgExtractMinorVersionNum(version) \
    (version & CFGMSG_VERSION_COMPONENT_MASK)

u8 *apacHyfi20GetXmitBuf() {
    memset(apacS.xmitBuf, 0, ETH_FRAME_LEN);
    return apacS.xmitBuf;
}

/* Setup frame header (ether header and IEEE1905 message header   */
int apacHyfi20SetPktHeader(u8 *frame, ieee1905MessageType_e type,
    u16 mid, u8 fid, u8 flags, u8 *src, u8 *dest)
{
    ieee1905Message_t *msg = (ieee1905Message_t *)frame;

    /* sanity check */
    if (!frame) {
        dprintf(MSG_ERROR, "%s - null frame\n", __func__);
        return -1;
    }
    if( type >= IEEE1905_MSG_TYPE_RESERVED )
    {
        dprintf(MSG_ERROR, "%s - Invalid message type: %d", __func__, type );
        return -1;
    }

    /* set up Ethernet frame header and IEEE1905 message header */
    memset(msg, 0, IEEE1905_ETH_HEAD_LEN);

    msg->etherHeader.ether_type = htons(IEEE1905_ETHER_TYPE);
    memcpy(msg->etherHeader.ether_shost, src, ETH_ALEN);
    memcpy(msg->etherHeader.ether_dhost, dest, ETH_ALEN);

    msg->ieee1905Header.version = IEEE1905_PROTOCOL_VERSION;
    msg->ieee1905Header.type = (u16)htons(type);
    msg->ieee1905Header.mid = htons(mid);
    msg->ieee1905Header.fid = fid;
    msg->ieee1905Header.flags = flags;
    msg->ieee1905Header.reserved = 0;

    return 0;
}

apacHyfi20NodeTbl_t *apacHyfi20GetNodeEntryByMac(u8 *alid) {
    int i;

    for (i = 0; i < APAC_MAXNUM_NTWK_NODES; i++) {
        if (apacNodeTbl[i].valid == APAC_FALSE) {
            continue;
        }

        if (os_memcmp(alid, apacNodeTbl[i].src_alid, ETH_ALEN) == 0) {
            return &apacNodeTbl[i];
        }
    }

    return NULL;
}

void apacHyfi20DumpPacketInfo(u8 *frame, u32 frameLen)
{
    ieee1905Message_t *fPtr = (ieee1905Message_t *)frame;
    ieee1905MessageType_e msgType;
    ieee1905Header_t hdrIEEE1905;
    u16 mid;
    u8 level = MSG_DEBUG;

    if (frameLen < IEEE1905_FRAME_MIN_LEN)
    {
        dprintf(MSG_ERROR, "%s -- invalid IEEE1905 packet with length %d! discard !\n", __func__, frameLen);
        return;
    }

    hdrIEEE1905 = fPtr->ieee1905Header;
    msgType = ntohs(hdrIEEE1905.type);
    mid = ntohs(hdrIEEE1905.mid);

    if (msgType >= IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_SEARCH &&
            msgType <= IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_RENEW) {
        level = MSG_INFO;
    }

    dprintf(level, "DumpPkt: src=%02x:%02x:%02x:%02x:%02x:%02x dest=%02x:%02x:%02x:%02x:%02x:%02x type=%u, mid=%u, fid=%u,"
            "flags=%x framelen %d \n", fPtr->etherHeader.ether_shost[0],
            fPtr->etherHeader.ether_shost[1], fPtr->etherHeader.ether_shost[2],
            fPtr->etherHeader.ether_shost[3], fPtr->etherHeader.ether_shost[4],
            fPtr->etherHeader.ether_shost[5], fPtr->etherHeader.ether_dhost[0],
            fPtr->etherHeader.ether_dhost[1], fPtr->etherHeader.ether_dhost[2],
            fPtr->etherHeader.ether_dhost[3], fPtr->etherHeader.ether_dhost[4],
            fPtr->etherHeader.ether_dhost[5], msgType, mid, hdrIEEE1905.fid,
            hdrIEEE1905.flags,frameLen);
}

int apacHyfi20SendL2Packet(apacHyfi20IF_t *pIF, u8 *frame, u32 frameLen)
{
    int  ret = -1;
    int  ioerr = 0;

    apacHyfi20DumpPacketInfo(frame, frameLen);
    if (pIF->sock > 0)
    {
        ret =  send(pIF->sock, frame, frameLen, 0);
        if (ret  < 0 && errno == ENXIO )
            ioerr = 1;
        if (ret < 0)
            perror("apacHyfi20SendL2Packet");
    }

    if (pIF->sock <= 0 || ioerr == 1)
    {
        apacHyfi20ResetIeee1905TXSock(pIF);
        ret =  send(pIF->sock, frame, frameLen, 0);
        if (ret < 0)
            perror("apacHyfi20SendL2Packet 2");
    }

    return ret;
}

/* Send Config Ack message to Registrar */
int apacHyfi20SendConfigAckE(struct apac_wps_session *sess) {
    apacHyfi20Data_t *pData = sess->pData;
    apacHyfi20IF_t *pIF = pData->hyif;
    u8 *frame = apacHyfi20GetXmitBuf();
    ieee1905TLV_t *tlv = (ieee1905TLV_t *)((ieee1905Message_t *)frame)->content;
    u16 mid = apacHyfi20GetMid();
    u8 dest[ETH_ALEN], j = 0;
    size_t frameLen = IEEE1905_FRAME_MIN_LEN;
    u_int32_t bufferLen = 0;
    ieee1905QCAMessage_t *qcaMessage =
        (ieee1905QCAMessage_t *)ieee1905TLVValGet(tlv);
    ieee1905QCAVendorSpecificType_e type = IEEE1905_QCA_TYPE_CFG_ACK;

    os_memcpy(dest, sess->dest_addr, ETH_ALEN);

    apacHyfi20SetPktHeader(frame, IEEE1905_MSG_TYPE_VENDOR_SPECIFIC,
        mid, 0, IEEE1905_HEADER_FLAG_LAST_FRAGMENT,
        pData->alid, dest);

    ieee1905TLVTypeSet(tlv, IEEE1905_TLV_TYPE_VENDOR_SPECIFIC);
    ieee1905QCAOUIAndTypeSet(qcaMessage, type, bufferLen);
    *qcaMessage->content = cfgmsgPackVersionNum(cfgmsgMajorVersion1,
                                                  cfgmsgMinorVersion0);
    bufferLen++;
    ieee1905TLVLenSet(tlv, bufferLen, frameLen);

    tlv = ieee1905TLVGetNext(tlv);
    /* Add EndOfTlv */
    ieee1905EndOfTLVSet(tlv);

    // Send L2
    if (pData->config.sendOnAllIFs == APAC_FALSE) {
        if (send(pData->bridge.sock, frame, frameLen, 0) < 0) {
           perror("apacHyfi20SendConfigAckE");
           return -1;
        }
    } else {
        /* Send packet on bridge if Traffic Separation is Enabled */
#if MAP_ENABLED
        if (apacHyfiMapIsTrafficSeparationEnabled(HYFI20ToMAP(pData))) {
            if (send(pData->bridge.sock, frame, frameLen, 0) < 0) {
                perror("apacHyfi20SendConfigAckE-onAllIFs");
                return -1;
            }
        }
#endif

        for (j = 0; j < APAC_MAXNUM_HYIF; j++) {
            if (pIF[j].ifIndex != -1) {
                if (apacHyfi20SendL2Packet(&pIF[j], frame, frameLen) < 0) {
                    perror("apacHyfi20SendConfigAckE-onAllIFs");
                    return -1;
                }
                dprintf(MSG_INFO, "%s sent ConfigAck msg mid: %d on %s\n", __func__, mid,
                        pIF[j].ifName);
            }
        }
    }

    return 0;
}

/* Send Config Apply multicast message to all Enrollees */
int apacHyfi20SendConfigApplyR(void *ptrData) {
    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)ptrData;
    apacHyfi20IF_t *pIF = pData->hyif;
    u8 *frame = apacHyfi20GetXmitBuf();
    ieee1905TLV_t *tlv = (ieee1905TLV_t *)((ieee1905Message_t *)frame)->content;
    u16 mid = apacHyfi20GetMid();
    u8 dest[ETH_ALEN], i=0, j = 0;
    size_t frameLen = IEEE1905_FRAME_MIN_LEN;
    u_int32_t bufferLen = 0;
    ieee1905QCAMessage_t *qcaMessage =
        (ieee1905QCAMessage_t *)ieee1905TLVValGet(tlv);
    ieee1905QCAVendorSpecificType_e type = IEEE1905_QCA_TYPE_CFG_APPLY;

    os_memcpy(dest, APAC_MULTICAST_ADDR, ETH_ALEN);

    apacHyfi20SetPktHeader(frame, IEEE1905_MSG_TYPE_VENDOR_SPECIFIC,
        mid, 0, IEEE1905_HEADER_FLAG_LAST_FRAGMENT | IEEE1905_HEADER_FLAG_RELAY,
        pData->alid, dest);

    ieee1905TLVTypeSet(tlv, IEEE1905_TLV_TYPE_VENDOR_SPECIFIC);
    ieee1905QCAOUIAndTypeSet(qcaMessage, type, bufferLen);
    *qcaMessage->content = cfgmsgPackVersionNum(cfgmsgMajorVersion1,
                                                  cfgmsgMinorVersion0);
    bufferLen++;
    ieee1905TLVLenSet(tlv, bufferLen, frameLen);

    tlv = ieee1905TLVGetNext(tlv);
    /* Add EndOfTlv */
    ieee1905EndOfTLVSet(tlv);

#if MAP_ENABLED
    /* Send packet on bridge if Traffic Separation is Enabled */
    if (apacHyfiMapIsTrafficSeparationEnabled(HYFI20ToMAP(pData))) {
        if (send(pData->bridge.sock, frame, frameLen, 0) < 0) {
            perror("apacHyfi20SendConfigApplyE");
            return -1;
        }
    }
#endif

    // Send L2 multicast for 5 times
    for (j = 0; j < APAC_MAXNUM_HYIF; j++) {
        if (pIF[j].ifIndex != -1) {
            for (i = 0; i < APAC_NUM_CFG_APPLY_FRAMES; i++) {
                if (apacHyfi20SendL2Packet(&pIF[j], frame, frameLen) < 0) {
                    perror("apacHyfi20SendConfigApplyE");
                    return -1;
                }
                dprintf(MSG_INFO, "%s sent ConfigApply msg mid: %d on %s\n", __func__, mid,
                        pIF[j].ifName);
            }
        }
    }

    return 0;
}

/* Apply config after waiting for config apply message to come from registrar */
void apacHyfi20ConfigApplyTimeoutHandler(void *eloop_ctx, void *timeout_ctx) {
    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)eloop_ctx;
    apacHyfi20Config_t *pConfig = &(pData->config);

#if MAP_ENABLED
    apacMapData_t *mapData = HYFI20ToMAP(pData);
#endif
    /* mask signals here so that config apply will have time to complete and
       will not get interrupted by signals */
#if MAP_ENABLED
    if (!apacHyfiMapPfComplianceEnabled(HYFI20ToMAP(pData))) {
#endif
        wsplcdMaskSignals();
#if MAP_ENABLED
    }
#endif
    dprintf(MSG_DEBUG, "%s: Config Apply Message Timeout. Applying config. \n", __func__);
    pData->wifiConfigHandle = apac_mib_apply_wifi_configuration(pData->wifiConfigHandle, APAC_TRUE);
    if (!pData->wifiConfigHandle) {
        dprintf(MSG_ERROR, "%s: Config Apply failed, cleaning up and restarting wsplcd.\n", __func__);
        /* delete the first session, on enrollee side there will always be only
           one active session present at a time. */
        if (pData->sess_list) {
            apac_wps_del_session(pData->sess_list);
        }
        shutdown_fatal();
    }
    dprintf(MSG_INFO, "%s: Config Apply success.\n", __func__);
    /* since config apply succeeded, remove cleanup required flag and delete this session */
    if (pData->sess_list) {
        pData->sess_list->cleanupRequired = APAC_FALSE;
        apac_wps_del_session(pData->sess_list);
    }

    /* now it is safe to unmask signals */
#if MAP_ENABLED
    if (!apacHyfiMapPfComplianceEnabled(HYFI20ToMAP(pData))) {
#endif
        wsplcdUnmaskSignals();
#if MAP_ENABLED
    }
#endif

    pData->wifiConfigWaitSecs = 0;
    pConfig->state = APAC_E_IDLE;
#if MAP_ENABLED
    pConfig->configApplyTimerStarted = APAC_FALSE;
    /* if fast onboarding is enabled and config apply succeeds, ie restartWifi uci flag is set
       then, we need to drop any new renews till next restart to avoid starting a new cloning
       session before the current config is applied */
    /* TODO: new libstorage API can be defined to read this flag directly instead of using
       wsplcdUciGetInt(), which has popen overhead */
    s32 restartWifi = wsplcdUciGetInt("repacd.MAPConfig.restartWifi");
    if (restartWifi == APAC_TRUE && mapData->fastOnboardEnabled) {
        pConfig->waitForWifiRestart = APAC_TRUE;
        if (mapData->external_controller) {
            system("/etc/init.d/repacd config_agent_only_mode --create_vlan &");
            dprintf(MSG_INFO, "%s: Apply configs for external controller\n", __func__);
        }
        dprintf(MSG_INFO, "%s: restartWifi uci flag set, renews will be dropped till next restart\n", __func__);
    }
#endif
}

/* add new node and mid to node table, return 0 for success, -1 for error */
int apacHyfi20AddEntryToNodeTbl(u8 *alid, u16 mid, int *index /*out*/) {
    int i;

    for (i = 0; i < APAC_MAXNUM_NTWK_NODES; i++) {
        if (apacNodeTbl[i].valid == APAC_FALSE) {
            apacNodeTbl[i].src_mid = mid;
            os_memcpy(apacNodeTbl[i].src_alid, alid, ETH_ALEN);
            apacNodeTbl[i].valid = APAC_TRUE;
            *index = i;

            return 0;
        }
    }

    dprintf(MSG_ERROR, "%s Node Table is full!\n", __func__);
    return -1;
}

/* calling centrailized IEEE1905 packets Mid generator. */
u16 apacHyfi20GetMid() {
    apacS.mid = messageId_getNext();
    return apacS.mid;
}

/*
 * Check MID: return 0 for new mid; -1 for old mid or error
 * Lei Note (3/19/2012):
 *  MID is stored in apacNodeTbl, which has two fields: ALID and MID
 *  However, Not all 1905 packets include ALID TLV, but we have to check MID
 *  for each of them
 *  The solution is to fill the ALID field with SA when ALID is not available.
 *  Therefore, there may be multiple entries for one device. We assume the side
 *  effect can be safely ignored.
 */
int apacHyfi20CheckMid(u8 *alid, u16 mid, u8 isLastfrag) {
    apacHyfi20NodeTbl_t *ptrEntry = apacHyfi20GetNodeEntryByMac(alid);
    u_int16_t delta;

    apacHyfi20TRACE();

    if (!ptrEntry) {
        int index;
        return (apacHyfi20AddEntryToNodeTbl(alid, mid, &index));
    }

    if (!ptrEntry->src_mid)
        ptrEntry->src_mid = mid;

    delta = mid - ptrEntry->src_mid;

    if (!isLastfrag) {
        dprintf(MSG_INFO, "Same Mid(%d) and fragmented So Accepted \n", mid);
        return 0;
    }

    if( !delta ||
       ( ( mid + APAC_MID_DELTA > ptrEntry->src_mid ) &&
         ( delta > ( 1 << 15 ) ) ) ) {
        dprintf(MSG_DEBUG, "%s: mid Order Mismatch!! mid %05d, saved %05d", __func__, mid,
                ptrEntry->src_mid);
        ptrEntry->src_mid = mid;
        return -1;
    } else {
        ptrEntry->src_mid = mid;
        return 0;
    }
}

/* Record Search MID sent */
void apacHyfi20SetSearchMid(u16 mid, u32 freq) {
    int i, j;

    for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
        if (apacS.searchMidSent[i].freq == freq) {
            j = apacS.searchMidSent[i].nextHistoryIndex;
            apacS.searchMidSent[i].valid[j] = APAC_TRUE;
            apacS.searchMidSent[i].mid[j] = mid;
            apacS.searchMidSent[i].nextHistoryIndex = (j + 1) % APAC_SEARCH_HIST_SZ;
            return;
        }
    }

    /* unlikely to be here */
    dprintf(MSG_ERROR, "%s, freq %u is not valid!\n", __func__, freq);
}

int apacHyfi20ResetSearchMidHist(u32 freq) {
    int j;

    for (j = 0; j < APAC_SEARCH_HIST_SZ; j++)
        apacS.searchMidSent[freq].valid[j] = APAC_FALSE;

    apacS.searchMidSent[freq].nextHistoryIndex = 0;

    return 0;
}

int apacHyfi20CheckMidOnResponseMsg(u16 mid, u32 freq) {
    int i, j, mid_found, freq_found;

    freq_found = 0;
    for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
        if(apacS.searchMidSent[i].freq == freq)
            freq_found = 1;
        else
            continue;

        mid_found = 0;
        for (j = 0; j < APAC_SEARCH_HIST_SZ; j++) {
            if (apacS.searchMidSent[i].valid[j]) {
                if (apacS.searchMidSent[i].mid[j] == mid) {
                    mid_found = 1;
                    break;
                }
                else {
                    dprintf(MSG_DEBUG, "%s, received Mid(%u) is not Mid (%u) last sent, "
                                    "id(%d)\n", __func__, mid, apacS.searchMidSent[i].mid[j], j);
                }
            }
        }
        if (freq_found && mid_found) {
            dprintf(MSG_MSGDUMP, "%s, Line(%d) received Mid(%u) Returning Success %d \n", __func__
                    , __LINE__, mid, freq);
            apacHyfi20ResetSearchMidHist(i);
            return 0;
        } else if (freq_found) {
            dprintf(MSG_DEBUG, "%s, invalid: mid(%u) for freq(%u) \n", __func__, mid,
                    freq);
            return -1;
        }
    }

    dprintf(MSG_DEBUG, "%s, invalid: mid(%u) and freq(%u) not recognized\n", __func__, mid, freq);
    return -1;
}

void printMsg(u8 *frame, size_t frameLen, s32 debugLevel) {
    size_t i;

    dprintf(debugLevel, "Packet len: %d\n",(s32)frameLen);
    for (i = 0; i < frameLen; i++) {
        dprintf(debugLevel, "%02X ", frame[i]);
        if (((i+1) % 16) == 0) {
            dprintf(debugLevel, "\n");
        }
    }
    dprintf(debugLevel, "\n");
}

/* add TLV to list, return 0 for success, else for error */
int apacHyfi20AddTlvToList(p1905TlvCheck_t *tlvList, const int num, ieee1905TlvType_e tlvType, apacBool_e multi)
{
    int i;

    for (i = 0; i < num; i++) {
        if (tlvList[i].tlvType == IEEE1905_TLV_TYPE_END_OF_MESSAGE /*0*/) {
            tlvList[i].tlvType = tlvType;
            tlvList[i].multiTlvAllowed = multi;
            tlvList[i].valid = APAC_FALSE;
            return 0;
        }
    }

    dprintf(MSG_ERROR, "%s - tlvList is full! can't add type %d\n", __func__, tlvType);
    return -1;
}

/* Check if received TLV is required */
apacBool_e apacHyfi20FindTlvInList(p1905TlvCheck_t *tlvList, ieee1905TlvType_e tlvType, const int num)
{
    int i;

    /* does not record vendor specific TLV */
    if (tlvType == IEEE1905_TLV_TYPE_VENDOR_SPECIFIC)
        return APAC_TRUE;

    for (i = 0; i < num; i++) {
        if (tlvList[i].tlvType == tlvType) {
            if (tlvList[i].valid == APAC_FALSE) {
                tlvList[i].valid = APAC_TRUE;
                return APAC_TRUE;
            }
            else if (tlvList[i].multiTlvAllowed == APAC_TRUE) {
                return APAC_TRUE;
            }
            else {
                dprintf(MSG_ERROR, "Multiple TLVs for type %d are not allowed\n", tlvType);
                return APAC_FALSE;
            }
        }
    }

    dprintf(MSG_ERROR, "TLV type %d is not required!\n", tlvType);
    return APAC_FALSE;
}

apacBool_e apacHyfi20ValidateTlvList(p1905TlvCheck_t *list, const int num) {
    int i;

    for (i = 0; i < num; i++) {
        if (list[i].valid == APAC_FALSE && (list[i].tlvType != IEEE1905_TLV_TYPE_END_OF_MESSAGE)) {
             dprintf(MSG_ERROR, "Validate TLV Error: TLV%d not included in the message\n", list[i].tlvType);
        }
    }
    return APAC_TRUE;
}

/* Apply configure and restart wifi on timer expiry */
void apacHyfi20ConfigPostRestartTimeoutHandler(void *eloop_ctx, void *timeout_ctx) {
#if MAP_ENABLED
    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)eloop_ctx;
    apacMapData_t *mapData = HYFI20ToMAP(pData);
#endif

    /* apply and restart wifi right now */
    dprintf(MSG_DEBUG, "%s: Invoking apply and/or wifi restart. \n", __func__);
    apacHyfi20ConfigApplyTimeoutHandler(eloop_ctx, timeout_ctx);

    /* since apply may restart wifi, so the below code may not be called */
    dprintf(MSG_DEBUG, "%s: Invoking wifi restart. \n", __func__);
#if MAP_ENABLED
    if (mapData->fastOnboardEnabled) {
        apac_mib_restart_wireless_repacd();
    } else {
        apac_mib_restart_wireless();
    }
#else
    apac_mib_restart_wireless();
#endif
}


/* Restart wifi on Restart timer expiry, for enrollee only */
void apacHyfi20ConfigRestartTimeoutHandler(void *eloop_ctx, void *timeout_ctx) {
    (void)timeout_ctx;
    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)eloop_ctx;
    apacHyfi20Config_t *pConfig = &pData->config;

    if (pConfig->role == APAC_REGISTRAR) {
        dprintf(MSG_DEBUG, "%s: Registrar is Sending Config Apply \n", __func__);
        // Ask Enrollees to restart their service
        apacHyfi20SendConfigApplyR(pData);

        dprintf(MSG_DEBUG, "%s: set wifi restart timer. \n",__func__);
        eloop_cancel_timeout(apacHyfi20ConfigPostRestartTimeoutHandler, pData, NULL);
        eloop_cancel_timeout(apacHyfi20ConfigApplyTimeoutHandler, pData, NULL);
        eloop_register_timeout(apac_cfg_restart_short_interval, 0,
            apacHyfi20ConfigPostRestartTimeoutHandler, pData, NULL);

        /* if registrar, do not restart wifi now, it will be restarted by timer */
        return;
    }

    /* if not registrar, apply and restart wifi right now */
     apacHyfi20ConfigPostRestartTimeoutHandler(eloop_ctx, timeout_ctx);
}

/* receive IEEE1905 packet; check packet type and dispatch */
void apacHyfi20GetIEEE1905PktCB(s32 sock, void *eloop_ctx, void *sock_ctx) {
    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)eloop_ctx;
    apacHyfi20Config_t *pConfig = &pData->config;
#if MAP_ENABLED
    apacHyfi20IF_t *pIF = (apacHyfi20IF_t *)sock_ctx;
#endif
    u8 frame[ETH_FRAME_LEN];
    struct sockaddr_ll ll;
    socklen_t socklen;
    ssize_t frameLen;

    ieee1905Message_t *msg = (ieee1905Message_t *)frame;
    ieee1905MessageType_e msgType;
    ieee1905Header_t hdrIEEE1905;
    u16 mid;
    u8 isfrag = 0;
    struct timeval t_now = {0};

    apacHyfi20TRACE();

    memset(&ll, 0, sizeof(ll));
    socklen = sizeof(ll);

    frameLen = recvfrom(sock, frame, sizeof(frame), 0, (struct sockaddr *)&ll, &socklen);
    {
        struct tpacket_stats kstats;

        memset(&kstats, 0, sizeof(kstats));
        socklen_t len = sizeof (struct tpacket_stats);
        memset(&kstats, 0, len);

        if (getsockopt(sock, SOL_PACKET, PACKET_STATISTICS, &kstats, &len) > -1)
            dprintf(MSG_DEBUG, "%u %u\n", kstats.tp_packets, kstats.tp_drops);
    }

    if (frameLen < 0) {
        perror("apacHyfi20GetIEEE905PktCB - recvfrom");
        apacHyfi20ResetIeee1905RXSock(pData);
#if MAP_ENABLED
        if (apacHyfiMapIsUseLoInterface(HYFI20ToMAP(pData)) && pIF)
            apacHyfi20ResetIeee1905RXSockIface(pData, pIF);
#endif
        return;
    }

    if (frameLen < IEEE1905_FRAME_MIN_LEN)
    {
        dprintf(MSG_ERROR, "%s -- invalid IEEE1905 packet with length %d! discard !\n", __func__, (s32)frameLen);
        return;
    }

    hdrIEEE1905 = msg->ieee1905Header;
    msgType = ntohs(hdrIEEE1905.type);
    mid = ntohs(hdrIEEE1905.mid);

    /* Sanity check */
    if ( msg->etherHeader.ether_type != ntohs(IEEE1905_ETHER_TYPE) )
    {
        dprintf(MSG_ERROR, "%s -- not IEEE1905 packet! discard\n", __func__);
        return;
    }

    //printMsg(frame, frameLen, MSG_MSGDUMP);
    if ( msg->ieee1905Header.version != 0 ) {
        dprintf(MSG_ERROR, "%s - message version != 0: %d\n",  __func__, ntohs(msg->ieee1905Header.version));
        return;
    }

    /* discard packet from my own */
    ieee1905Message_t *fPtr = (ieee1905Message_t *)frame;
    if ( os_memcmp(fPtr->etherHeader.ether_shost, pData->alid, ETH_ALEN) == 0)
    {
        dprintf(MSG_DEBUG, "%s -- receive packet from my own, discard \n", __func__);
        return;
    }

    apacHyfi20DumpPacketInfo(frame, frameLen);

    /* Discard fragmented packets (exclude WPS packets) */
    if ( !((hdrIEEE1905.flags & IEEE1905_HEADER_FLAG_LAST_FRAGMENT) && (hdrIEEE1905.fid == 0))
      && (msgType != IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_WPS) )
    {
        dprintf(MSG_MSGDUMP, "%s -- receiving non-WPS fragmented packet; discard\n", __func__);
        return;
    }

    isfrag = hdrIEEE1905.flags & IEEE1905_HEADER_FLAG_LAST_FRAGMENT;

    if (  !(msgType == IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_SEARCH ||
                msgType == IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_RESPONSE  ||
                msgType == IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_WPS  ||
                msgType == IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_RENEW  ||
                msgType == IEEE1905_MSG_TYPE_PB_EVENT_NOTIFICATION ||
                msgType == IEEE1905_MSG_TYPE_PB_JOIN_NOTIFICATION ||
                msgType == IEEE1905_MSG_TYPE_VENDOR_SPECIFIC ||
                msgType == IEEE1905_MSG_TYPE_EARLY_AP_CAP_REPORT ) )
    {
        return;
    }
    /* check relayed bit: Only WPS packet may set relay bit */
    else {
        if (msgType == IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_WPS && hdrIEEE1905.flags & IEEE1905_HEADER_FLAG_RELAY) {
            dprintf(MSG_ERROR, "%s - msgtype(%d) shouldn't have relay bit set!\n", __func__, msgType);
            return;
        }
    }

    /* Check message type and dispatch it */
    switch (msgType)
    {
    case IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_SEARCH:
        dprintf(MSG_INFO, "%s, Receive Search Msg. My State: %u\n", __func__, pConfig->state);
        /* If NBTLV support is enabled read northbound tlv and
         * update shared file with the payload received */
        if (pData->config.enable_NB_tlv == 1)
            NBTlvReadReceiveSearch(pData, frame, frameLen);
        apacHyfi20ReceiveSearchR(pData, frame, frameLen);
        break;

    case IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_RESPONSE:
        dprintf(MSG_INFO, "%s, Receive Response Msg. My State: %u\n", __func__, pConfig->state);
        /* Check State */
        if (!(pConfig->state == APAC_E_PB_WAIT_RESP || pConfig->state == APAC_E_WAIT_RESP)) {
            dprintf(MSG_ERROR, "%s - State(%d) does not match!\n", __func__, pConfig->state);
            return;
        }
        apacHyfi20ReceiveResponseE(pData, frame, frameLen);
        break;

    case IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_WPS:
        dprintf(MSG_INFO, "%s, Receive WPS Msg. My State: %u, role: %u\n", __func__, pConfig->state, pConfig->role);
        /* Check MID using SA as ALID and discard the ones that have seen */
        if (apacHyfi20CheckMid(msg->etherHeader.ether_shost, mid, isfrag) < 0) {
            return;
        }
        apacHyfi20ReceiveWps(pData, frame, frameLen);
        break;

    case IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_RENEW:
        dprintf(MSG_INFO, "%s, Receive Renew Msg. My State: %u, role: %u\n", __func__, pConfig->state, pConfig->role);
#if MAP_ENABLED
        /* drop renews if we are waiting for wifi restart */
        if (pConfig->waitForWifiRestart) {
            dprintf(MSG_INFO, "%s, Dropping renew since 'wait for wifi restart' flag is set\n", __func__);
            return;
        }
#endif
        apacHyfi20ReceiveRenewalE(pData, frame, frameLen);
        break;

    case IEEE1905_MSG_TYPE_PB_EVENT_NOTIFICATION:
        dprintf(MSG_INFO, "%s, Receive PB Notification Msg. My State: %u, role: %u\n", __func__, pConfig->state, pConfig->role);
        pbcReceiveEventNotificationMsg(pData, frame, frameLen);
        break;

    case IEEE1905_MSG_TYPE_PB_JOIN_NOTIFICATION:
        dprintf(MSG_INFO, "%s, Receive PB Join Msg. My State: %u, role: %u\n", __func__, pConfig->state, pConfig->role);
        /* do not process */
        break;
    case IEEE1905_MSG_TYPE_VENDOR_SPECIFIC:
        /* Check MID using SA as ALID and discard the ones that have seen */

        if (apacHyfi20CheckMid(msg->etherHeader.ether_shost, mid, isfrag) < 0) {
            return;
        }

        dprintf(MSG_DEBUG, "%s, Received 1905 Vendor Specific Packet!!\n", __func__);
        apacHyfi20VendorSpecificHandle(pData, frame, frameLen);
        break;
    case IEEE1905_MSG_TYPE_EARLY_AP_CAP_REPORT:
        dprintf(MSG_DEBUG, "%s, Receive Early AP Capability Report\n", __func__);
        apacHyfi20ReceiveEarlyAPCapabilityReport(pData, frame, frameLen);
        break;
    /* discard Non-security packets */
    default:
        dprintf(MSG_DEBUG, "Msg type(%d) is not for wsplcd, discard\n", msgType);
        return;
    }

    gettimeofday(&t_now, NULL);
    dprintf(MSG_DEBUG, "%s, time: %ld Received msg type: %u, mid: %u, My state: %u\n",
            __func__, t_now.tv_sec, msgType, mid, pConfig->state);

}

const ieee1905QCAMessage_t *extractQCAMessage(const ieee1905TLV_t *tlv) {
      const ieee1905QCAMessage_t *qcaMessage =
          (const ieee1905QCAMessage_t *)ieee1905TLVValGet(tlv);

      // The TLVs we care about have a type field followed by a version
      // field, so ignore any that are of insufficient length.
      if (ieee1905TLVLenGet(tlv) >= IEEE1905_OUI_LENGTH + 2 &&
          ieee1905QCAIsQCAOUI(qcaMessage->oui) &&
          qcaMessage->type >= IEEE1905_QCA_TYPE_SYSTEM_INFO_REQ) {
                dprintf(MSG_MSGDUMP, "QCA TLV %s\n", __func__);
                return qcaMessage;
      }

      // Not a TLV we will handle.
      return NULL;
}

void apacHyfi20VendorSpecificHandle(apacHyfi20Data_t *pData, u8 *message, size_t frameLen) {
    ieee1905Message_t *frame = (ieee1905Message_t *)message;
    ieee1905TLV_t *tlv =  (ieee1905TLV_t *)frame->content;
    ieee1905TlvType_e tlvType = ieee1905TLVTypeGet(tlv);
    apacHyfi20Config_t *pConfig = &pData->config;

    apacHyfi20TRACE();

    // Process QCA vendor sub-type messages
    if( tlvType == IEEE1905_TLV_TYPE_VENDOR_SPECIFIC ) {
        const ieee1905QCAMessage_t *qcaMessage = extractQCAMessage(tlv);

        if (qcaMessage) {
            switch (ieee1905QCATypeGet(qcaMessage)) {
                case IEEE1905_QCA_TYPE_CFG_ACK:
                   // Start Restart Timer if Registrar and config has changed.
                   if ((pConfig->cfg_changed) && (pConfig->role == APAC_REGISTRAR)) {
                       dprintf(MSG_DEBUG, "%s: Received QCA TLV CFG_ACK from src: ", __func__);
                       printMac(MSG_DEBUG, frame->etherHeader.ether_shost);
                       dprintf(MSG_DEBUG, "%s: Starting wifi restart long timer \n", __func__);
                       eloop_cancel_timeout(apacHyfi20ConfigRestartTimeoutHandler, pData, NULL);
                       eloop_register_timeout(apac_cfg_restart_long_interval, 0,
                               apacHyfi20ConfigRestartTimeoutHandler, pData, NULL);
                   } else {
                       dprintf(MSG_DEBUG, "%s: Ignoring received QCA TLV CFG_ACK as either config has not changed or we are not registrar.\n", __func__);
                   }
                   break;
               case IEEE1905_QCA_TYPE_CFG_APPLY:
                   // Process this only if you are Enrollee
                   if (pConfig->role == APAC_ENROLLEE) {
                       dprintf(MSG_DEBUG, "%s: Received QCA TLV CFG_APPLY. Starting wifi restart short timer\n", __func__);
                       eloop_cancel_timeout(apacHyfi20ConfigApplyTimeoutHandler, pData, NULL);
                       eloop_cancel_timeout(apacHyfi20ConfigRestartTimeoutHandler, pData, NULL);
                       eloop_register_timeout(apac_cfg_restart_short_interval, 0,
                               apacHyfi20ConfigRestartTimeoutHandler, pData, NULL);
                   } else {
                       dprintf(MSG_DEBUG, "%s: Ignoring received QCA TLV CFG_APPLY as we are not enrollee.\n", __func__);
                   }
                   break;
               default:
                   break;
            }
        }

        if(pConfig->atf_config_enabled == APAC_TRUE) {
            if(qcaMessage) {
                apacHyfi20ReceiveAtfConfig(message, frameLen);
            }
        }
    }
}


/* push button was actived */
void apacHyfi20EventPushButtonActivatedCB(void *eloop_ctx) {
    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)eloop_ctx;
    apacHyfi20Config_t *pConfig = &pData->config;

    if (pConfig->role == APAC_REGISTRAR)  {

        pConfig->state = APAC_R_PB_WAIT_SEARCH;
    }
    else {
        apacHyfi20SendSearchE(pData);
        pConfig->state = APAC_E_PB_WAIT_RESP;

        eloop_register_timeout(pConfig->pb_search_to, 0,
                        apacHyfi20PbSearchTimeoutHandler, pData, NULL);
    }
    eloop_register_timeout(pConfig->pushbutton_to, 0,
                    apacHyfi20PushButtonTimeoutHandler, pData, NULL);

    return;
}

#if SON_ENABLED
extern struct wlanif_config *wlanIfWd;
#endif

u8 apacHyfi20IsFullBand( u8 bandInfo ){
#if SON_ENABLED
   return ( (bandInfo >> 4) == bandInfo_Full );
#else
   return 0;
#endif
}

u8 apacHyfi20GetLocalBandFromRadioName(char *RadioName, u8 band) {
    u8 bandInfo;

    if ( !wlanIfWd ) {
        dprintf(MSG_ERROR, "%s - wlanif nl init failed\n", __func__);
        return bandInfo_Unknown;
    }

    if (!((band == APAC_WIFI_FREQ_5) || (band == APAC_WIFI_FREQ_5_OTHER) ||
        (band == APAC_WIFI_FREQ_6) || (band == APAC_WIFI_FREQ_6_OTHER))) {
        return bandInfo_Unknown;
    }

    if(!RadioName) {
        return bandInfo_Unknown;
    }

    if (getBandInfo_cfg80211(wlanIfWd->ctx, RadioName , &bandInfo) < 0) {
        dprintf(MSG_ERROR, "%s:%d> Error: ioctl() failed, radio ifName: %s.\n", __func__, __LINE__, RadioName );
        return bandInfo_Unknown;
    }

    return bandInfo;
}

static int apacHyfi20SetBand( apacHyfi20Data_t *pData, u8 band ) {
#if SON_ENABLED
    u8 bandInfo;
    apacHyfi20AP_t *pAP = pData->ap;

    if ( !pAP[band].valid ) {
        return -1;
    }

    if (!pAP[band].radioName) {
        if (band == APAC_WIFI_FREQ_5) {
            if(!pAP[APAC_WIFI_FREQ_5_OTHER].valid || !pAP[APAC_WIFI_FREQ_5_OTHER].radioName)
                return -1;
            else
                band = APAC_WIFI_FREQ_5_OTHER;
        } else if (band == APAC_WIFI_FREQ_5_OTHER) {
            if(!pAP[APAC_WIFI_FREQ_5].valid || !pAP[APAC_WIFI_FREQ_5].radioName)
                return -1;
            else
                band = APAC_WIFI_FREQ_5;
        }
    }

    bandInfo = apacHyfi20GetLocalBandFromRadioName(pAP[band].radioName, band);

    if(bandInfo == bandInfo_Unknown) {
        return -1;
    }

    pData->config.wpsConf->rf_bands = ((bandInfo << 4) | pData->config.wpsConf->rf_bands);
#endif
    return 0;
}

void apacHyfi20RMCollectTimeoutHandler(void *eloop_ctx, void *time_out) {
    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)eloop_ctx;
    apacHyfi20Config_t *pConfig = &pData->config;
    apacHyfi20IF_t *pIF = pData->hyif;
    apacHyfi20AP_t *pAP = pData->ap;
    u32 band;
    u32 oldBand = APAC_WIFI_FREQ_INVALID;
    int i;

    if (!(pConfig->wlan_chip_cap == APAC_DB && pConfig->band_sel_enabled)) {
        dprintf(MSG_ERROR, "%s, Invalid!\n", __func__);
        return;
    }

    apacHyfi20TRACE();

    /* cancel any outstanding RMCollect timeout */
    eloop_cancel_timeout(apacHyfi20RMCollectTimeoutHandler, pData, NULL);

    dprintf(MSG_DEBUG, "%s, band supported by Reg: %u, band_choice: %u\n", __func__,
            apacS.bandSupportedByReg, pConfig->band_choice);

    if ( apacS.bandSupportedByReg & (1 << pConfig->band_choice) ) {
        band = pConfig->band_choice;
    }
    else { /* XXX: currently only 2G and 5G are supported */
        if (apacS.bandSupportedByReg == (1 << APAC_WIFI_FREQ_2)) {
            band = APAC_WIFI_FREQ_2;
        }
        else if (apacS.bandSupportedByReg == (1 << APAC_WIFI_FREQ_5)) {
            band = APAC_WIFI_FREQ_5;
        }
        else if (apacS.bandSupportedByReg == (1 << APAC_WIFI_FREQ_60)) {  /* unlikely */
            band = APAC_WIFI_FREQ_60;
        }
        else if (apacS.bandSupportedByReg == (1 << APAC_WIFI_FREQ_5_OTHER)) {  /* unlikely */
            band = APAC_WIFI_FREQ_5_OTHER;
        }
        else if (apacS.bandSupportedByReg == (1 << APAC_WIFI_FREQ_6)) {  /* unlikely */
            band = APAC_WIFI_FREQ_6;
        }
        else if (apacS.bandSupportedByReg == (1 << APAC_WIFI_FREQ_6_OTHER)) {  /* unlikely */
            band = APAC_WIFI_FREQ_6_OTHER;
        }
        else {
            dprintf(MSG_INFO, "%s, No response or invalid band info (%u) from Registrar\n",
                    __func__, apacS.bandSupportedByReg);
            return;
        }
    }

    /* update information */
    apacS.bandInRegistration = band;
    pData->config.wpsConf->rf_bands = apac_get_wps_rfband(band);

    apacHyfi20SetBand(pData, band);
    /* reset info */
    for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
        if (pAP[i].valid) {
            oldBand = pAP[i].freq;

            if (band == i) {
                break;              /*  no need to reset band */
            }

            pAP[band] = pAP[i];     /* copy the AP information */
            pAP[band].freq = band;  /* reset freq information */
            pAP[i].valid = APAC_FALSE; /* invalidate old AP in the previous freq band */
            break;
        }
    }

    if (oldBand == APAC_WIFI_FREQ_INVALID) {
        dprintf(MSG_ERROR, "%s, No valid band to select!\n", __func__);
        return;
    }

    if (oldBand != band) {
        for (i = 0; i < APAC_MAXNUM_HYIF; i++) {
            if ( pIF[i].mediaType == APAC_MEDIATYPE_WIFI && pIF[i].wifiFreq == oldBand ){
                pIF[i].wifiFreq = band;

                if (apacHyfi20Set80211Channel(pIF[i].ifName, band) < 0) {
                    dprintf(MSG_ERROR, "%s, can't set band to %u!\n", __func__, band);
                    return;
                }
            }
        }
    }

    /* for Debug */
    dprintf(MSG_DEBUG, "%s, Resetting Band...\n", __func__);
    for (i = 0; i < APAC_MAXNUM_HYIF; i++) {
        if (!pIF[i].valid)
            continue;

        if (strncmp(pIF[i].ifName, "ath", 3) == 0) {
            dprintf(MSG_DEBUG, "WlanIF: %s \tdevice mode: %u \tfreq: %u \tvapIndex: %u\n",
                    pIF[i].ifName, pIF[i].wlanDeviceMode, pIF[i].wifiFreq, pIF[i].vapIndex);


        }
    }

    /* Start Registration */
    if (apacHyfi20StartRegistrationE(pData, apacS.reg_src) < 0) {
        dprintf(MSG_ERROR, "%s Can't start registration \n", __func__);
        return;
    }
}

/* pushbutton times out, disabling autoconfig */
void apacHyfi20PushButtonTimeoutHandler(void *eloop_ctx, void *time_out) {
    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)eloop_ctx;
    apacHyfi20Config_t *pConfig = &pData->config;

    if (pConfig->role == APAC_ENROLLEE) {
        eloop_cancel_timeout(apacHyfi20PbSearchTimeoutHandler, pData, NULL);
        pConfig->state = APAC_E_PB_IDLE;
    }
    else {
        pConfig->state = APAC_R_PB_IDLE;
    }
}

/* resend Search Msg in PushButton mode */
void apacHyfi20PbSearchTimeoutHandler(void *eloop_ctx, void *timeout_ctx) {
    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)eloop_ctx;

    apacHyfi20SendSearchE(pData);

    /* register PB search timeout callback */
    eloop_register_timeout(pData->config.pb_search_to, 0,
                    apacHyfi20PbSearchTimeoutHandler, pData, NULL);
}

/* resend Search Msg in auto-config mode */
void apacHyfi20SearchTimeoutHandler(void *eloop_ctx, void *timeout_ctx) {
    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)eloop_ctx;

    apacHyfi20SendSearchE(pData);

    const u16 *interval = (const u16 *)timeout_ctx;
#if SON_ENABLED
    if (interval) {
        /* When an alternate interval is provided, it means we are still waiting for
           APAC complete on all band. */

        size_t i;
        u32 waitThreshold = pData->config.wait_wifi_config_secs_first;
        for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
            if (pData->ap[i].isAutoConfigured) {
                // Wait shorter if there is already band configured
                waitThreshold = pData->config.wait_wifi_config_secs_other;
                break;
            }
        }
        pData->wifiConfigWaitSecs += *interval;
        if (pData->wifiConfigWaitSecs >= waitThreshold) {
            /* wait long enough */
            dprintf(MSG_INFO, "Abort waiting for all bands configured after %u seconds\n",
                    pData->wifiConfigWaitSecs);
            pData->wifiConfigHandle =
                apac_mib_apply_wifi_configuration(pData->wifiConfigHandle, APAC_TRUE);
            if (!pData->wifiConfigHandle) {
                dprintf(MSG_ERROR, "Config Apply failed, restarting wsplcd.\n");
                if (pData->sess_list) {
                    apac_wps_del_session(pData->sess_list);
                }
                shutdown_fatal();
            }
            pData->wifiConfigWaitSecs = 0;
            /* Keep searching at long interval */
            interval = NULL;
        }
    }
#endif
    /* register PB search timeout callback */
    eloop_register_timeout(interval ? *interval : pData->config.search_to, 0,
                           apacHyfi20SearchTimeoutHandler, pData, (void *)interval);
}

/* WPS session timer */
void apacHyfi20WpsSessionTimeoutHandler(void *eloop_ctx, void *timeout_ctx) {
    struct apac_wps_session *sess = (struct apac_wps_session *)eloop_ctx;
    apacHyfi20Data_t *pData;
    apacHyfi20Config_t *pConfig;

    if (!sess)
        return;

    pData = sess->pData;
    pConfig = &(pData->config);

    sess->wps_session_ts ++;
    if (sess->wps_session_ts >= pConfig->wps_session_to)
    {
        if (!sess->wps_sess_success)
            dprintf(MSG_ERROR, "%s session time out!\n", __func__);
        goto fail;
    }

    sess->wps_message_ts ++;
    if (sess->wps_message_ts >= pConfig->wps_per_msg_to)
    {
        if (!sess->wps_sess_success)
            dprintf(MSG_ERROR, "%s WPS packet[%d] response time out, session failed!\n", __func__, sess->state);
        goto fail;
    }

    sess->wps_retrans_ts ++;
    if (sess->wps_retrans_ts >= pConfig->wps_retransmit_to)
    {
        APAC_WPS_DATA *data = sess->pWpsData;

        if (sess->wps_sess_success)
        {
            /* Registrar need not retry when session finished
               but it response when it receives retried M7*/
        }
        else
        {
             dprintf(MSG_INFO, "%s WPS packet[%d] response time out, reqest it again!\n", __func__, sess->state);
            if (!data || !data->sndMsg || !data->sndMsgLen)
            {
                dprintf(MSG_ERROR, "%s failed to get last message!\n", __func__);
                goto fail;
            }
            if(data->fragment) {
                if(data->rcvMsg) {
                    free(data->rcvMsg);
                    data->rcvMsg=NULL;
                }
                dprintf(MSG_ERROR, "%s:%d> >>> Fragment - WPS Resent <<< !\n", __func__, __LINE__);
                data->fragment = 0;
            }

            if (apacHyfi20SendWps(sess, data->sndMsg, data->sndMsgLen) < 0) {
                dprintf(MSG_ERROR, "%s failed to retransmit last message!\n", __func__);
                return ;
            }
        }
        sess->wps_retrans_ts = 0;

    }

    sess->timerScheduled = APAC_TRUE;
    eloop_register_timeout(1, 0, apacHyfi20WpsSessionTimeoutHandler, sess, NULL);
    return;

fail:
    apacHyfi20ResetState(sess, APAC_FALSE, NULL);
    apac_wps_del_session(sess);

    return;
}

#if 0
/* STA scanning timeout, stop STA if it is not associated */
void apacHyfi20ScanningTimeoutHandler(void *eloop_ctx, void *timeout_ctx) {
    apacHyfi20IF_t *pIF = (apacHyfi20IF_t *)eloop_ctx;
    int32_t isRunning;
    int ret;

    ret = pbcGetVapStatus(pIF->ifName, &isRunning);
    if (ret == 0 && isRunning == 0) {
        apac_mib_set_vap_status(pIF->vapIndex, 0);
        dprintf(MSG_ERROR, "%s scanning timeout, stop VAP %s!\n", __func__, pIF->ifName);
    }

}
#endif


/* Channel polling timer */
void apacHyfi20ChannelPollingHandler(void *eloop_ctx, void *timeout_ctx) {
    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)eloop_ctx;
    apacHyfi20AP_t *pAP = pData->ap;
    uint32_t oldChan;
    char oldStd[APAC_STD_MAX_LEN];
    int i, sendRenewal;

    for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
        if (!pAP[i].valid || pAP[i].isStaOnly)
            continue;
        if (pAP[i].freq == APAC_WIFI_FREQ_6 &&
            pData->config.enableAfcChanSelection) {
            dprintf(MSG_INFO, "%s AFC is configured and skip channel "
                                   "monitoring for 6G band", __func__);
            continue;
        }
        sendRenewal = 0;
        oldChan = pAP[i].channel;
        strlcpy(oldStd, pAP[i].standard, APAC_STD_MAX_LEN);
        apacHyfi20GetChannel(&pAP[i]);
        apacHyfi20GetAPMode(&pAP[i]);
        if (oldChan != pAP[i].channel) {
            dprintf(MSG_INFO, "%s channel changed from %d to %d\n",
                    __func__, oldChan, pAP[i].channel);
#if MAP_ENABLED
                if (!apacHyfiMapIsEnabled(HYFI20ToMAP(pData))) {
                    sendRenewal = 1;
                }
#else
                sendRenewal = 1;
#endif
        }
        if (strcmp(oldStd, pAP[i].standard) != 0) {
            dprintf(MSG_INFO, "%s standard changed from %s to %s\n",
                    __func__, oldStd, pAP[i].standard);
            sendRenewal = 1;
       }
        if (sendRenewal) {
            apacHyfi20SendRenewalR(pData);
        }
    }

    eloop_register_timeout(APAC_CHANNEL_POLLING_TIMEOUT, 0,
        apacHyfi20ChannelPollingHandler, eloop_ctx, timeout_ctx);
}

#ifdef QCA_EXT_EM_CONTROLLER
int g_emex_onboard_3rd_parties = -1;

static int i5MessageEMEXConfig3rdPartyOnboarding()
{
    if (cbs.emex_get_config) {
        char ucival[UCI_MAX_STR_LEN] = {'\0'};
        int ret = 0;
        if (g_emex_onboard_3rd_parties == -1) {
            g_emex_onboard_3rd_parties = 0;
            ret = cbs.emex_get_config(EZMESH_UCI_MODULE, EMEX_ONBOARD_3RD, ucival);
            if ( ret != 0 && strcmp(ucival,"0") ) {
                g_emex_onboard_3rd_parties = 1;
            }
        }
    }
    return g_emex_onboard_3rd_parties;
}
#endif

/*
 * Internal APIs
 */

/********************
 **** ENROLEE *******
 ********************/
/* Enrollee receives Renewal msg */
int apacHyfi20ReceiveRenewalE(apacHyfi20Data_t *pData, u8 *msg, u32 msgLen) {
    u32 processedLen;
    u8 *pTlvVal;
    ieee1905Message_t *frame = (ieee1905Message_t *)msg;
    ieee1905TLV_t *pTLV =  (ieee1905TLV_t *)frame->content;
    ieee1905TlvType_e tlvType;
    u8 src[ETH_ALEN];
    //apacBool_e isBusy = APAC_TRUE;
    int i;
    u16 mid = 0;
    u8 isfrag = 0;
    apacBool_e isRoleMatching = APAC_FALSE;
    u32 supportedFreq = APAC_WIFI_FREQ_INVALID;
    const int num_tlv = 3;

    apacHyfi20TRACE();

    /* sanity check */
    /* Ignore Renew msg if in PB mode, or Search msg has been sent */
    if (pData->config.pbmode_enabled) {
        return 0;
    }

    #if 0
    /* check availability: Send Search Message if Enrollee (1) is in IDLE state, or
     * (2) is waiting for Response Msg
     */
    if (pData->config.state == APAC_E_IDLE || pData->config.state == APAC_E_WAIT_RESP) {
        isBusy = APAC_FALSE;
    }
    #endif

    /* check role */
    if (pData->config.role != APAC_ENROLLEE) {
        dprintf(MSG_ERROR, "%s, Device role (%u) is not ENROLLEE. Discard,\n", __func__, pData->config.role);
        return 0;
    }

    /* Get mid */
    mid = ntohs(frame->ieee1905Header.mid);

    isfrag = frame->ieee1905Header.flags & IEEE1905_HEADER_FLAG_LAST_FRAGMENT;
    /* Message verification. */
    p1905TlvCheck_t tlvList[num_tlv];
    os_memset(tlvList, 0, sizeof(p1905TlvCheck_t)*num_tlv);
    apacHyfi20AddTlvToList(tlvList, num_tlv, IEEE1905_TLV_TYPE_AL_ID, APAC_FALSE);
    apacHyfi20AddTlvToList(tlvList, num_tlv, IEEE1905_TLV_TYPE_SUPPORTED_ROLE, APAC_FALSE);
    apacHyfi20AddTlvToList(tlvList, num_tlv, IEEE1905_TLV_TYPE_SUPPORTED_FREQ_BAND, APAC_FALSE);

    /* Parse TLV */
    processedLen = ieee1905TLVLenGet(pTLV) + IEEE1905_ETH_HEAD_LEN + IEEE1905_HEAD_LEN;
    while (processedLen <= msgLen) {
        tlvType = ieee1905TLVTypeGet(pTLV);
        dprintf(MSG_MSGDUMP, "Get TLV type: %d\n", tlvType);

        if (tlvType == IEEE1905_TLV_TYPE_END_OF_MESSAGE) {
            break;
        }

#if MAP_ENABLED
        /* Remove check to not return when we rx r2 TLV on r1 device */
        if (!apacHyfiMapIsEnabled(HYFI20ToMAP(pData))) {
#endif
            if (apacHyfi20FindTlvInList(tlvList, tlvType, num_tlv) == APAC_FALSE) {
                dprintf(MSG_ERROR, "%s - illagel TLV type %d\n", __func__, tlvType);
                return -1;
            }
#if MAP_ENABLED
        }
#endif

        pTlvVal = ieee1905TLVValGet(pTLV);

        if (tlvType ==  IEEE1905_TLV_TYPE_SUPPORTED_ROLE &&
            *pTlvVal == APAC_REGISTRAR)
        {
            isRoleMatching = APAC_TRUE;
        }
        else if (tlvType == IEEE1905_TLV_TYPE_SUPPORTED_FREQ_BAND) {
            supportedFreq = *pTlvVal;
            dprintf(MSG_DEBUG, "Received Renewal For Freq %u\n", supportedFreq);

            #if 0 /* always sends Search Message on receiving a valid Renewal Message */
            /* check if need to process */
            if ((pData->ap[supportedFreq]).valid == APAC_FALSE) {
                dprintf(MSG_MSGDUMP, "%s, No valid AP/STA on Freq%u.\n", __func__, supportedFreq);
                return 0;
            }
            #endif

            if (supportedFreq < APAC_WIFI_FREQ_2 || supportedFreq > APAC_WIFI_FREQ_6_OTHER) {
                dprintf(MSG_ERROR, "%s - received band(%d) is undefined!\n", __func__, supportedFreq);
                return -1;
            }
            dprintf(MSG_MSGDUMP, "Registrar supports freq#%d\n", *pTlvVal);

        }
        else if (tlvType == IEEE1905_TLV_TYPE_AL_ID) {
            memcpy(src, pTlvVal, ETH_ALEN);

            /* Check MID and discard the ones that have seen */
            if (apacHyfi20CheckMid(pTlvVal, mid, isfrag) < 0) {
#if MAP_ENABLED
                if (!apacHyfiMapPfComplianceEnabled(HYFI20ToMAP(pData))) {
                    return -1;
                }
#else
                return -1;
#endif
            }
        }

        processedLen += ieee1905TLVLenGet(pTLV);
        pTLV = ieee1905TLVGetNext(pTLV);
    }

    if (isRoleMatching == APAC_FALSE) {
        dprintf(MSG_ERROR, "%s Renewal is not from Registrar!\n", __func__);
        return -1;
    }

    /* Check if all required TLVs have been received. Ignore check for MAP */
#if MAP_ENABLED
    if (!apacHyfiMapIsEnabled(HYFI20ToMAP(pData))) {
#endif
        if (!apacHyfi20ValidateTlvList(tlvList, num_tlv)) {
            dprintf(MSG_ERROR, "required TLV type missing in the message!\n");
            return -1;
        }
#if MAP_ENABLED
    }
#endif

    /* Clear out all AutoConfigured flags and send search messages */
    for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
        pData->ap[i].isAutoConfigured = APAC_FALSE;
    }
    apacS.bandInRegistration = APAC_WIFI_FREQ_INVALID;
    apacS.bandSupportedByReg = 0;

    /*delete unfinished WPS session if any*/
    {
        struct apac_wps_session* sess;
        sess = apac_wps_find_session(pData, src);

        if (sess)
        {
            dprintf(MSG_DEBUG, "Receive Renew Message, delete WPS session.\n");
            apac_wps_del_session(sess);
        }
    }

    /* send Search Messages and reset state */
    apacHyfi20SendSearchE(pData);
    eloop_register_timeout(pData->config.search_to, 0, apacHyfi20SearchTimeoutHandler, pData, NULL);

    #if 0
    /* send Search Message if Enrollee is not busy */
    if (!isBusy) {
        dprintf(MSG_DEBUG, "%s, Receive Renewal Mesage for Freq %u. Send Search\n", __func__, supportedFreq);
        apacHyfi20SendSearchE(pData);
    }

    /* Enrollee send Search msg */
    apacS.bandInRegistration = supportedFreq;
    pData->config.wpsConf->rf_bands = apac_get_wps_rfband(supportedFreq);
    dprintf(MSG_DEBUG, "%s, start registration for freq %d\n", __func__, supportedFreq);

    pData->ap[supportedFreq].isAutoConfigured = APAC_FALSE;
    if (apacHyfi20StartRegistrationE(pData, src) < 0) {
        dprintf(MSG_ERROR, "%s Can't start registration \n", __func__);
        return -1;
    }
    #endif

    return 0;
}

/* Print north bound parameters to the shared file */
void print_nb_file(FILE *fptr, char *mac,
        int *role, char *payload, long int *pos, int read) {

    fseek(fptr, *pos, SEEK_SET);

    if(payload)
        fprintf(fptr, "%s:%d:%s\n", mac, *role, payload);
    else
    {
#define NB_SHARE_FILE_STR_MIN 14
        if(read > NB_SHARE_FILE_STR_MIN) {
            int i;
            char *tmpstr = NULL;
            /* This condition is to handle the corner case, where part
             * of the line already present in the file is moved to
             * new line, caused when the buffer to be written to the
             * file has lesser string length than the buffer already present.
             * The excess character already present are replaced with
             * 'blankspace' avoiding the already existing character being
             * moved to new line.
             */
            tmpstr = (char *)malloc(read-NB_SHARE_FILE_STR_MIN);
            if(!tmpstr) {
                perror("nb file malloc allocation failed");
                exit(EXIT_FAILURE);
            }
            for(i=0;i<(read-NB_SHARE_FILE_STR_MIN-1);i++) {
                tmpstr[i] = ' ';
            }
            tmpstr[i] = '\0';
            fprintf(fptr, "%s:%d%s\n", mac, *role, tmpstr);
            free(tmpstr);
#undef NB_SHARE_FILE_STR_MIN
        } else {
            fprintf(fptr, "%s:%d\n", mac, *role);
        }
    }
}

/* Read the shared file and print the alid, role and payload.
 * Each alid should be printed on only one line, any msg from
 * same alid should be reprinted in the same line, so there
 * aren't any duplicate entry
 */
int apac_write_nb_file(u8 *alid, int role, char *payload) {
#define MAC_SIZE_IN_CHAR 12
    FILE *fp = NULL;
    char *line = NULL;
    size_t length = 0;
    long int pos = 0;
    ssize_t read = 0;
    int printed =0;
    char mac[MAC_SIZE_IN_CHAR+1];

    /* Create shared file if it didn't exist */
    if (access("/tmp/nb_share_file", F_OK) == -1) {
        FILE *fptr = NULL;
        fptr = fopen("/tmp/nb_share_file", "w+");
        if (fptr == NULL) {
            return -1;
        }
        fclose(fptr);
    }

    fp = fopen("/tmp/nb_share_file", "r+");
    if (fp == NULL) {
        return -1;
    }
    snprintf(mac, MAC_SIZE_IN_CHAR+1, "%02x%02x%02x%02x%02x%02x",alid[0],alid[1], alid[2],
            alid[3], alid[4], alid[5]);

    /* Read line by line to check whether the alid already present*/
    while ((read = getline(&line, &length, fp)) != -1) {
        if(!strncmp(mac, line, strlen(mac))) {
            print_nb_file(fp, mac, &role, payload, &pos, (int)read);
            printed=1;
            break;
        }
        pos += read;
    }
    if (!printed) {
        print_nb_file(fp, mac, &role, payload, &pos, (int)read);
    }
    if (line)
        free(line);
    fclose(fp);
    return 0;
#undef MAC_SIZE_IN_CHAR
}

/* uci set responsercvd parameter in wsplcd config */
void apac_set_responserecvd(int value){
    void *mibHandle = NULL;
    char path[256];
    char *root = CONFIG_WSPLC;
    int fail=0;

    mibHandle = storage_getHandle();
    if(NULL == mibHandle)
    {
        return;
    }

    snprintf(path, sizeof(path), "%s%s", root, "ResponseRcvd");
    if(value) {
        storage_setParam(mibHandle,path,"1");
    } else {
        storage_setParam(mibHandle,path,"0");
    }
    fail = storage_apply(mibHandle);
    if (fail)
        dprintf(MSG_ERROR, "%s: responsercvd config set failed! \n",__func__);
}

/* Read NBTLVbuff config from wsplcd log and deconstruct the buffer.
 * The buffer contains <msgTlv><vendorTlv><lengthofpayload><payload>
 * example: <0007><65><0D><ABBAABBAABBA0>
 *
 * +-------+...+-------+-------+-------+-------+-------+-------+...+-------+
 * |char 1 |...|char 4 |char 5 |char 6 |char 7 |char 8 |char 9 |...|char n |
 * +-------+...+-------+-------+-------+-------+-------+-------+...+-------+
 * |<---Message tlv--->| <-vendor tlv->| payload length|<------payload---->|
 * +-----------------------------------------------------------------------+
 */
int apac_get_NBbuff(char *value, int *mtlv, int *vtlv, int *length, char *payload) {
    char msgTLV[5];
    char vendortlv[3];
    char payloadlen[3];

#define NBTLVBUFF_MIN_PAYLOAD 9
#define NBTLVBUFF_MSGTLV_LEN 4
#define NBTLVBUFF_VENTLV_LEN 2
#define NBTLVBUFF_PAYLOAD_LEN 2
#define NBTLVBUFF_PAYLOAD 8

    /* payload of atleast 1 character is required */
    if (strlen(value) > NBTLVBUFF_MIN_PAYLOAD) {
        /* First 4 character denotes message TLV.
         * The TLV in which the vendor TLV is sent
         * search message in this case
         */
        strlcpy(msgTLV, value, NBTLVBUFF_MSGTLV_LEN+1);
        /* Next 2 charcters denotes the vendor tlv subtype */
        strlcpy(vendortlv, value+NBTLVBUFF_MSGTLV_LEN, NBTLVBUFF_VENTLV_LEN+1);
        /* Next 2 characters denotes the payload length */
        strlcpy(payloadlen, value+NBTLVBUFF_MSGTLV_LEN+NBTLVBUFF_VENTLV_LEN,
                NBTLVBUFF_PAYLOAD_LEN+1);
        *mtlv = (int)strtol(msgTLV, NULL, 16);
        *vtlv = (int)strtol(vendortlv, NULL, 16);
        *length = (int)strtol(payloadlen, NULL, 16);
        /* Next characters are copied based on the length
         * provide in the length value
         */
        strlcpy(payload, value+NBTLVBUFF_PAYLOAD, *length+1);

        if (strlen(payload) > SEARCH_MSG_MAX_PAYLOAD) {
            perror("NBTlv buffer payload exceeds supported search msg payload");
            exit(EXIT_FAILURE);
        }
        return 0;
    } else {
        perror("Invalid NBTLVbuff entry");
        exit(EXIT_FAILURE);
    }
    return -1;
#undef NBTLVBUFF_MIN_PAYLOAD
#undef NBTLVBUFF_MSGTLV_LEN
#undef NBTLVBUFF_VENTLV_LEN
#undef NBTLVBUFF_PAYLOAD_LEN
#undef NBTLVBUFF_PAYLOAD
}

int NBTlvReadReceiveSearch(apacHyfi20Data_t *ptrData, u8 *msg, u32 msgLen) {
    ieee1905Message_t *frame = (ieee1905Message_t *)msg;
    ieee1905TLV_t *TLV = (ieee1905TLV_t *)frame->content;
    ieee1905TlvType_e tlvType;
    u32 processedLen;
    u8 *tlvVal;
    u8 srcmac[ETH_ALEN];

    apacHyfi20TRACE();
    memcpy(srcmac, frame->etherHeader.ether_shost, ETH_ALEN);

    if (ntohs(frame->ieee1905Header.type) != IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_SEARCH) {
        dprintf(MSG_ERROR, "Msg Type %u is not SEARCH(7)\n", ntohs(frame->ieee1905Header.type));
        return -1;

    }

    /* Parse TLV */
    processedLen = ieee1905TLVLenGet(TLV) + IEEE1905_ETH_HEAD_LEN + IEEE1905_HEAD_LEN;
    while (processedLen <= msgLen) {
        tlvType = ieee1905TLVTypeGet(TLV);
        dprintf(MSG_MSGDUMP, "Get TLV type: %d\n", tlvType);

        if (tlvType == IEEE1905_TLV_TYPE_END_OF_MESSAGE) {
            break;
        }

        tlvVal = ieee1905TLVValGet(TLV);

        /* To get the Second Byte */
        if(tlvType == IEEE1905_TLV_TYPE_SEARCHED_SERVICE)
            tlvVal++;

        if (tlvType == IEEE1905_TLV_TYPE_VENDOR_SPECIFIC) {
            ieee1905QCAMessage_t *qcaMessage =
                (ieee1905QCAMessage_t *) tlvVal;

            if(ieee1905QCAIsQCAOUI(qcaMessage->oui)) {
                /* Read vendor tlv received from search msg and write it to shared file */
                if(qcaMessage->type == IEEE1905_QCA_TYPE_NORTHBOUND_TLV) {
                    char temp[256];
                    ieee1905QCANorthBoundTLV_t *NorthBoundTLV;
                    NorthBoundTLV = (ieee1905QCANorthBoundTLV_t *) qcaMessage->content;
                    strlcpy((char *)&temp, (const char *) &NorthBoundTLV->content, NorthBoundTLV->length+1);
                    apac_write_nb_file(srcmac, NB_AGENT, temp);
                    break;
                }
            }
        }

        processedLen += ieee1905TLVLenGet(TLV);
        TLV = ieee1905TLVGetNext(TLV);
    }
    return 0;
}

/* Enrollee receives Response msg */
int apacHyfi20ReceiveResponseE(apacHyfi20Data_t *pData, u8 *msg, u32 msgLen) {
    apacHyfi20Config_t *pConfig = &pData->config;
    u32 processedLen;
    u8 *pTlvVal;
    ieee1905Message_t *frame = (ieee1905Message_t *)msg;
    ieee1905TLV_t *pTLV =  (ieee1905TLV_t *)frame->content;
    ieee1905TlvType_e tlvType;
    u8 *src = frame->etherHeader.ether_shost;
    u32 supportedFreq = APAC_WIFI_FREQ_INVALID;
    u16 receivedMid;
#if MAP_ENABLED
    apacMapData_t *mapData = HYFI20ToMAP(pData);
    apacBool_e isEarlyAPCapabilitySupported = APAC_FALSE;
#endif
    apacBool_e isRoleMatching = APAC_FALSE;
    int num_tlv = 3;
    p1905TlvCheck_t tlvList[num_tlv];
#ifdef QCA_EXT_EM_CONTROLLER
    u8 supportedService = 0;
#endif

    apacHyfi20TRACE();

    u8 mac[ETH_ALEN];
    memcpy(mac, src, ETH_ALEN);

    if ( !(pConfig->state == APAC_E_PB_WAIT_RESP ||
           pConfig->state == APAC_E_WAIT_RESP) )
    {
        dprintf(MSG_ERROR, "%s state(%d) is not correct, discard\n", __func__, pConfig->state);
        return -1;
    }

    receivedMid = ntohs(frame->ieee1905Header.mid);

    /* Message verification. */
    os_memset(tlvList, 0, sizeof(p1905TlvCheck_t)*num_tlv);

    apacHyfi20AddTlvToList(tlvList, num_tlv, IEEE1905_TLV_TYPE_SUPPORTED_ROLE, APAC_FALSE);
    apacHyfi20AddTlvToList(tlvList, num_tlv, IEEE1905_TLV_TYPE_SUPPORTED_FREQ_BAND, APAC_FALSE);

#if MAP_ENABLED
    if (apacHyfiMapIsEnabled(HYFI20ToMAP(pData)))
        apacHyfi20AddTlvToList(tlvList, num_tlv, IEEE1905_TLV_TYPE_SUPPORTED_SERVICE, APAC_FALSE);
#endif

    /* Parse TLV */
    processedLen = ieee1905TLVLenGet(pTLV) + IEEE1905_ETH_HEAD_LEN + IEEE1905_HEAD_LEN;
    while (processedLen <= msgLen) {
        tlvType = ieee1905TLVTypeGet(pTLV);
        dprintf(MSG_MSGDUMP, "Get TLV type: %d\n", tlvType);

        if (tlvType == IEEE1905_TLV_TYPE_END_OF_MESSAGE) {
            break;
        }

        /* Remove check to not return when we rx r2 TLV on r1 device */
#if MAP_ENABLED
        if (!apacHyfiMapIsEnabled(HYFI20ToMAP(pData))) {
#endif
            if (apacHyfi20FindTlvInList(tlvList, tlvType, num_tlv) == APAC_FALSE) {
                dprintf(MSG_ERROR, "%s - unknown TLV type %d, discard\n", __func__, tlvType);
                return -1;
            }
#if MAP_ENABLED
        }
#endif

        pTlvVal = ieee1905TLVValGet(pTLV);

        if (tlvType ==  IEEE1905_TLV_TYPE_SUPPORTED_ROLE &&
            *pTlvVal == APAC_REGISTRAR)
        {
            isRoleMatching = APAC_TRUE;
        }
        else if (tlvType == IEEE1905_TLV_TYPE_SUPPORTED_FREQ_BAND) {
            supportedFreq = *pTlvVal;
            dprintf(MSG_DEBUG, "Registrar supports freq#%d\n", *pTlvVal);

            if (supportedFreq < APAC_WIFI_FREQ_2 || supportedFreq > APAC_WIFI_FREQ_6_OTHER) {
                dprintf(MSG_ERROR, "%s - received band(%d) is undefined!\n", __func__, supportedFreq);
                return -1;
            }

            if (apacHyfi20CheckMidOnResponseMsg(receivedMid, supportedFreq) < 0) {
#if MAP_ENABLED
                if (!apacHyfiMapPfComplianceEnabled(mapData)) {
                    return -1;
                }
#else
                return -1;
#endif
            }
        } else if (tlvType == IEEE1905_TLV_TYPE_MAP_VERSION) {
            dprintf(MSG_ERROR, "%s - Received version(%d) search response", __func__, *pTlvVal);
        } else if (tlvType == IEEE1905_TLV_TYPE_SECURITY_CAP) {
            dprintf(MSG_ERROR, "%s - Got Security CAP\n", __func__);
        }
#ifdef QCA_EXT_EM_CONTROLLER
        else if (tlvType == IEEE1905_TLV_TYPE_SUPPORTED_SERVICE) {
            TlvSupportedServiceTypeExtract(pTlvVal, &supportedService, ieee1905TLVLenGet(pTLV));
            if (IS_MULTIAP_CONTROLLER(supportedService) && !IS_EMEX_CONTROLLER(supportedService)) {
                dprintf(MSG_ERROR,"Non-EM+ controller detected!\n");
                if (i5MessageEMEXConfig3rdPartyOnboarding() <= 0) {
                    dprintf(MSG_ERROR,"3rd party onboarding is disabled, not onboarding!\n");
                    return -1;
                } else {
                    dprintf(MSG_ERROR,"Going to be onboarded to a 3rd party controller without EMEX!\n");
                    if (cbs.emex_deinit) {
                        cbs.emex_deinit();
                    }
                }
            } else if (IS_MULTIAP_CONTROLLER(supportedService) && IS_EMEX_CONTROLLER(supportedService)) {
                dprintf(MSG_ERROR,"EMEX Controller found, check EMEX services for EM+ onboarding!\n");
                if (cbs.emex_init) {
                    cbs.emex_init();
                }
            }
        }
#endif

#if MAP_ENABLED
        else if (tlvType == IEEE1905_TLV_TYPE_CONTROLLER_CAPABILITY) {
            dprintf(MSG_ERROR, "%s - Received Controller capability: KiBMiB Counter = %d, "
                               "Accept Early AP Capability Report = %d  n", __func__,
                               ((*pTlvVal & 0x80) >> 7), ((*pTlvVal & 0x40) >> 6));
            isEarlyAPCapabilitySupported = ((*pTlvVal & 0x40) >> 6) ? APAC_TRUE : APAC_FALSE;
        }
#endif

        processedLen += ieee1905TLVLenGet(pTLV);
        pTLV = ieee1905TLVGetNext(pTLV);
    }

    if (isRoleMatching == APAC_FALSE) {
        dprintf(MSG_ERROR, "%s Reponser is not Registrar!\n", __func__);
        return -1;
    }

    /* Check if all required TLVs have been received. Ignore check for MAP */
#if MAP_ENABLED
    if (!apacHyfiMapIsEnabled(HYFI20ToMAP(pData))) {
#endif
        if (!apacHyfi20ValidateTlvList(tlvList, num_tlv)) {
            dprintf(MSG_ERROR, "required TLV type missing in the message!\n");
            return -1;
        }
#if MAP_ENABLED
    }
#endif

    /* cancel Search timeout */
    if (pConfig->pbmode_enabled) {
        eloop_cancel_timeout(apacHyfi20PbSearchTimeoutHandler, pData, NULL);
        eloop_cancel_timeout(apacHyfi20PushButtonTimeoutHandler, pData, NULL);
    }

    /* For DB band adaptation, record band info and wait */
    if (pData->config.wlan_chip_cap == APAC_DB && pData->config.band_sel_enabled) {
        apacS.bandSupportedByReg |= (1 << supportedFreq);
        os_memcpy(apacS.reg_src, src, ETH_ALEN);
        return 0;
    }
    /* check if I have AP on the supported freq */
    if (supportedFreq == APAC_WIFI_FREQ_INVALID)
    {
        dprintf(MSG_ERROR, "Invalid Frequency!\n");
        return -1;
    }

    apacS.bandInRegistration = supportedFreq;
#if MAP_ENABLED
    if (!apacHyfiMapIsEnabled(HYFI20ToMAP(pData))) {
#endif
        if (!(pData->ap[supportedFreq].valid) || pData->ap[supportedFreq].isAutoConfigured) {
            dprintf(MSG_ERROR, "Enrollee has no AP on band%d or is alread configured!\n",
                    supportedFreq);
            return -1;
        }
#if MAP_ENABLED
    } else {
#endif
        if (pData->ap[supportedFreq].isAutoConfigured && supportedFreq == APAC_WIFI_FREQ_5)
            apacS.bandInRegistration = APAC_WIFI_FREQ_5_OTHER;

        if (pData->ap[supportedFreq].isAutoConfigured && supportedFreq == APAC_WIFI_FREQ_6)
            apacS.bandInRegistration = APAC_WIFI_FREQ_6_OTHER;

#if MAP_ENABLED
    }
#endif

#if MAP_ENABLED
    if (apacHyfiMapIsEnabled(HYFI20ToMAP(pData)) >= APAC_MAP_VERSION_6 &&
        isEarlyAPCapabilitySupported == APAC_TRUE) {
        apacHyfiMapCtrlSetSpecMloOnbSupported(HYFI20ToMAP(pData), 1);
        apacHyfi20SendEarlyAPCapabilityReport(pData, src);
    }
#endif

    /* Start Registration */
    pData->config.wpsConf->rf_bands = apac_get_wps_rfband(supportedFreq);
    apacHyfi20SetBand(pData, supportedFreq);
    dprintf(MSG_INFO, "%s, received Response include rf band %d\n", __func__, supportedFreq);
    dprintf(MSG_INFO, "%s, Band in registration %d\n", __func__, apacS.bandInRegistration);

    if (apacHyfi20StartRegistrationE(pData, src) < 0) {
        dprintf(MSG_ERROR, "%s Can't start registration \n", __func__);
        return -1;
    }

    /* update responsercvd and shared file for NB application*/
    if (pData->config.enable_NB_tlv == 1) {
        apac_set_responserecvd(RESPONSE_RCVD);
        apac_write_nb_file(mac, NB_CONTROLLER, NULL);
    }

#if HYFI10_COMPATIBLE
    /* Disable 1.0 cloning client */
    if (pData->config.hyfi10_compatible)
        wsplc_disable_cloning(HYFI20ToHYFI10(pData));
#endif

    return 0;
}

ieee1905TLV_t *ieee1905AddFeatureFlagTLV(ieee1905TLV_t *TLV, u_int32_t *Len, apacHyfi20Data_t *pData)
{
    u_int16_t tlvLen = 0;
    ieee1905QCAMessage_t *qcaMessage = NULL;

    apacHyfi20TRACE();

    ieee1905QCASupportedFeatureList_t *featureList;
    TLV = ieee1905TLVGetNext(TLV);
    qcaMessage =
        (ieee1905QCAMessage_t *)ieee1905TLVValGet(TLV);

    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_VENDOR_SPECIFIC);

    ieee1905QCAOUIAndTypeSet(qcaMessage, IEEE1905_QCA_TYPE_FEATURE_SUPPORT_LIST, tlvLen);
    featureList = (ieee1905QCASupportedFeatureList_t *)qcaMessage->content;
    os_memset(featureList, 0, sizeof(ieee1905QCASupportedFeatureList_t));

#if SON_ENABLED
#define MULTI_M2_SUPPORT 1
        featureList->sonFeatureList.MultipleM2Support = MULTI_M2_SUPPORT;
#undef MULTI_M2_SUPPORT
#elif MAP_ENABLED
#if DOT1X_SUPPORT
    {
        apacMapData_t *mapData = HYFI20ToMAP(pData);
        if (apacHyfiMapIsDot1xEnabled(mapData))
            featureList->mapFeatureList.dot1xSupport = IEEE1905_QCA_DOT1X_FEATURE_SUPPORT;
        dprintf(MSG_DEBUG, "%s:%d> dot1xSupport=%d IsDot1xEnabled=%d\n",
                __func__, __LINE__, featureList->mapFeatureList.dot1xSupport,
                mapData->dot1xSupport);
    }
#endif
    featureList->mapFeatureList.reservedData[0] = 0;
#endif
    tlvLen += sizeof(ieee1905QCASupportedFeatureList_t);
    ieee1905TLVLenSet(TLV, tlvLen, *Len);

    printMsg((u8 *)TLV, tlvLen, MSG_DEBUG);
    return TLV;
}
/* Enrollee sends Early AP Capability Report msg */
#if MAP_ENABLED
int apacHyfi20SendEarlyAPCapabilityReport(apacHyfi20Data_t *pData, u8 *dest_mac) {
    u8 *frame = NULL;
    ieee1905MessageType_e msgType = IEEE1905_MSG_TYPE_EARLY_AP_CAP_REPORT;
    u16 mid = apacHyfi20GetMid();
    apacHyfi20AP_t *pAP = pData->ap;
    u8 flags = 0;
    u8 i = 0;
    u8 fragNum = 0;
    u32 frameLen = 0;
    u8 wifi7CapMsg[IEEE1905_CONTENT_MAXLEN] = {0};
    u32 wifi7CapMsgLen = 0;

    ieee1905TLV_t *TLV;


    /* Overhead: WPS TLV + EndTLV */
    size_t maxWifi7CapMsgLen = IEEE1905_CONTENT_MAXLEN - 2 * IEEE1905_TLV_MIN_LEN;

    apacHyfi20TRACE();

    serializeWifi7AgentCapabilitiesTLV(wifi7CapMsg, &wifi7CapMsgLen, pData);
    u8 *curWifi7CapMsg = &wifi7CapMsg[0];
    size_t curWifi7CapMsgLen = wifi7CapMsgLen;

    /* construct WPS packet */
    flags = 0;
    fragNum = 0;
    while (curWifi7CapMsgLen > 0) {
        frame = apacHyfi20GetXmitBuf();
        frameLen = IEEE1905_FRAME_MIN_LEN;
        TLV = (ieee1905TLV_t *)((ieee1905Message_t *)frame)->content;

        /* last frame */
        if (curWifi7CapMsgLen <= maxWifi7CapMsgLen) {

            flags |= IEEE1905_HEADER_FLAG_LAST_FRAGMENT;
            /* Add WPS TLV */
            ieee1905TLVSet(TLV, IEEE1905_TLV_TYPE_WIFI7_AGENT_CAPABILITY, curWifi7CapMsgLen, curWifi7CapMsg, frameLen);
            curWifi7CapMsg += curWifi7CapMsgLen;
            curWifi7CapMsgLen = 0;
        } else {
            ieee1905TLVSet(TLV, IEEE1905_TLV_TYPE_WIFI7_AGENT_CAPABILITY, maxWifi7CapMsgLen, curWifi7CapMsg, frameLen);
            curWifi7CapMsgLen -= maxWifi7CapMsgLen;
            curWifi7CapMsg += maxWifi7CapMsgLen;
        }

        if (flags & IEEE1905_HEADER_FLAG_LAST_FRAGMENT) {

            TLV = ieee1905MapAddAPCapabilityTLV(TLV, &frameLen, pData);

            for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
                if (pAP[i].valid) {
                    TLV = ieee1905MapAddBasicRadioTLV(TLV, &frameLen, i, pData);
                }
            }

            for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
                if (pAP[i].valid) {
                    TLV = ieee1905Map2AddApRadioAdvancedCapTLV(TLV, &frameLen, i, pData);
                }
            }

            TLV = ieee1905MapAddAKMSuiteCapabilitiesTLV(TLV, &frameLen, apacS.bandInRegistration,
                                                        pData);

            for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
                if (pAP[i].valid) {
                    TLV = ieee1905MapAddAPHTCapabilitiesTLV(TLV, &frameLen, i, pData);
                }
            }

            for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
                if (pAP[i].valid && (pAP[i].freq != APAC_WIFI_FREQ_2)) {
                    TLV = ieee1905MapAddAPVHTCapabilitiesTLV(TLV, &frameLen, i, pData);
                }
            }

            for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
                if (pAP[i].valid) {
                    TLV = ieee1905MapAddAPHECapabilitiesTLV(TLV, &frameLen, i, pData);
                }
            }

            for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
                if (pAP[i].valid) {
                    TLV = ieee1905MapAddWifi6CapabilitiesTLV(TLV, &frameLen, i, pData);
                }
            }

            TLV = ieee1905Map2AddApCapTLV(TLV, &frameLen, apacS.bandInRegistration,
                                                            pData);

            TLV = ieee1905MapAddMetricCollectionTLV(TLV, &frameLen, apacS.bandInRegistration,
                                                            pData);

            TLV = ieee1905MapAddDeviceInventoryTLV(TLV, &frameLen, apacS.bandInRegistration,
                                                            pData);
        }

        /* Add EndOfTlv */
        TLV = ieee1905TLVGetNext(TLV);
        ieee1905EndOfTLVSet(TLV);

        /* set up packet header */
        apacHyfi20SetPktHeader(frame, msgType, mid, fragNum, flags, pData->alid, dest_mac);

        /* send packet */
        if (pData->config.sendOnAllIFs == APAC_FALSE) {
            if (send(pData->bridge.sock, frame, frameLen, 0) < 0) {
                perror("apacHyfi20SendEarlyAPCapabilityReport");
                return -1;
            }
        } else { /* send unicast packet on all interfaces. Debug Only! */

#if MAP_ENABLED
            /* Send packet on bridge if Traffic Separation is Enabled */
            if (apacHyfiMapIsTrafficSeparationEnabled(HYFI20ToMAP(pData))) {
                if (send(pData->bridge.sock, frame, frameLen, 0) < 0) {
                    perror("apacHyfi20SendEarlyAPCapabilityReport-onAllIFs");
                }
            }
#endif

            for (i = 0; i < APAC_MAXNUM_HYIF; i++) {
                if (pData->hyif[i].ifIndex == -1)
                    continue;

                if (apacHyfi20SendL2Packet(&pData->hyif[i], frame, frameLen) < 0) {
                    perror("apacHyfi20SendEarlyAPCapabilityReport-onAllIFs");
                }
            }
        }

        dprintf(MSG_DEBUG, "%s len of msg sent: %u, mid: %u\n", __func__,(u32)frameLen, mid);
        fragNum++;
    }

    return 0;

}
#endif /* MAP_ENABLED */

/* Enrollee sends Search msg */
int apacHyfi20SendSearchE(apacHyfi20Data_t *pData) {
    apacHyfi20IF_t *pIF = pData->hyif;
    apacHyfi20AP_t *pAP = pData->ap;

    s32 i, j;
    u8 tlvValue;
    u8 dest[ETH_ALEN] = APAC_MULTICAST_ADDR;
    u32 frameLen;
    u8 *frame;
    u8 *content;
    ieee1905TLV_t *TLV;
    u8 freq;
    u16 mid = 0;
    u_int16_t tlvLen = 0;
    u_int8_t *contenttlv;
    u_int32_t bufferLen = 0;
    ieee1905QCAVendorSpecificType_e type;
    ieee1905QCAMessage_t *qcaMessage = NULL;
#if (defined QCA_EXT_EM_CONTROLLER && defined MAP_ENABLED)
    unsigned char *payload=NULL;
    unsigned short payload_len;
#endif

    apacHyfi20TRACE();

    if (pData->config.role != APAC_ENROLLEE) {
        dprintf(MSG_ERROR, "%s, Device role (%u) is not ENROLLEE\n", __func__, pData->config.role);
        return -1;
    }

    /* If a device sends search msg it doesn't have response from controller */
    if (pData->config.enable_NB_tlv == 1)
        apac_set_responserecvd(RESPONSE_NOT_RCVD);

    /* MUST READ: any new TLV added to search message should update the
     * SEARCH_MSG_MAX_PAYLOAD value defined
     */

    /* Set freq TLV and send IEEE 1905 Packet from each configured IF */
    for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
        dprintf(MSG_INFO, "%s - index:%d , valid:%d , Freq Value:%d\n", __func__, i,
                pAP[i].valid, pAP[i].freq);
        if (pAP[i].valid == APAC_FALSE) {
            /* currently, only consider 2G/5G Dual Band */
            if (!(pAP[i].freq == APAC_WIFI_FREQ_2 || pAP[i].freq == APAC_WIFI_FREQ_5 ||
                  pAP[i].freq == APAC_WIFI_FREQ_5_OTHER || pAP[i].freq == APAC_WIFI_FREQ_6 ||
                  pAP[i].freq == APAC_WIFI_FREQ_6_OTHER)) {
                dprintf(MSG_INFO, "%s, don't send Search Msg for freq%u .\n", __func__, i);
                continue;
            }
            if (!(pData->config.wlan_chip_cap == APAC_DB && pData->config.band_sel_enabled)) {
                dprintf(MSG_INFO, "%s, don't send Search Msg for freq%u .\n", __func__, i);
                continue;
            }
        } else if (pAP[i].isAutoConfigured) {
            dprintf(MSG_INFO, "%s - freq%d has APAC done, don't search Search Msg\n", __func__, i);
            continue;
        }

        freq = pAP[i].freq;
        frameLen = IEEE1905_FRAME_MIN_LEN;
        frame = apacHyfi20GetXmitBuf();
        content = ((ieee1905Message_t *)frame)->content;
        TLV = (ieee1905TLV_t *)content;

        /* set up new MID for each band */
        mid = apacHyfi20GetMid();

        apacHyfi20SetPktHeader(frame, IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_SEARCH, mid, 0,
                               IEEE1905_HEADER_FLAG_LAST_FRAGMENT | IEEE1905_HEADER_FLAG_RELAY,
                               pData->alid, dest);

        /* Add ALID TLV */
        ieee1905TLVSet(TLV, IEEE1905_TLV_TYPE_AL_ID, ETH_ALEN, pData->alid, frameLen);
        TLV = ieee1905TLVGetNext(TLV);

        /* Add SearchedRole TLV */
        tlvValue = APAC_REGISTRAR;
        ieee1905TLVSet(TLV, IEEE1905_TLV_TYPE_SEARCHED_ROLE, APAC_TLVLEN_ROLE, &tlvValue, frameLen);

        /* Record Search MID sent */
        apacHyfi20SetSearchMid(mid, freq);

        TLV = ieee1905TLVGetNext(TLV);
        ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_FREQ_BAND);

        contenttlv = ieee1905TLVValGet(TLV);
        tlvLen = 0;

        *contenttlv = freq;
        contenttlv++; tlvLen++;

        ieee1905TLVLenSet(TLV, tlvLen, frameLen);

#if MAP_ENABLED
        if (apacHyfiMapIsEnabled(HYFI20ToMAP(pData))) {
            u_int8_t tlvValue;
            u_int8_t listSupportedServices;
            u_int8_t listSearchedServices;
            tlvValue = APAC_MAP_AGENT;
#ifdef QCA_EXT_EM_CONTROLLER
            listSupportedServices = 2;
#else
            listSupportedServices = 1;
#endif
            /* Add SupportedService TLV */
            TLV = ieee1905TLVGetNext(TLV);
            ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_SUPPORTED_SERVICE);

            /* Get a pointer to the value field. We need to construct it manually */
            content = ieee1905TLVValGet(TLV);
            tlvLen = 0;

            /* Setup List of Supported Services */
            *content = listSupportedServices;
            content++;
            tlvLen++;

            /* Setup List of Supported Services */
            *content = tlvValue;
            content++;
            tlvLen++;

#ifdef QCA_EXT_EM_CONTROLLER
            /* Setup List of Supported Services */
            *content = MultiAPEmexAgent;
            content++;
            tlvLen++;
#endif
            /* Setup SupportedService TLV length */
            ieee1905TLVLenSet(TLV, tlvLen, frameLen);

            /* Add SearchedService TLV */
            tlvValue = APAC_MAP_CONTROLLER;
            listSearchedServices = 1;

            TLV = ieee1905TLVGetNext(TLV);
            ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_SEARCHED_SERVICE);

            /* Get a pointer to the value field. We need to construct it manually */
            contenttlv = ieee1905TLVValGet(TLV);
            tlvLen = 0;

            /* Setup List of Supported Services */
            *contenttlv = listSearchedServices;
            contenttlv++;
            tlvLen++;

            /* Setup List of Supported Services */
            *contenttlv = tlvValue;
            contenttlv++;
            tlvLen++;

            /* Setup Supported Services TLV length */
            ieee1905TLVLenSet(TLV, tlvLen, frameLen);

            if (apacHyfiMapIsEnabled(HYFI20ToMAP(pData)) >= APAC_MAP_VERSION_2) {
                TLV = ieee1905TLVGetNext(TLV);
                ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_MAP_VERSION);

                contenttlv = ieee1905TLVValGet(TLV);
                tlvLen = 0;

                *contenttlv = apacHyfiMapIsEnabled(HYFI20ToMAP(pData));
                contenttlv++;
                tlvLen++;

                ieee1905TLVLenSet(TLV, tlvLen, frameLen);
#ifdef QCA_EXT_EM_CONTROLLER
                if (!ieee1905EMEXTypeInsert(TLV_1905_VENDOR_SPECIFIC, EMEX_TLV_FEATURE_PROFILE, NULL, 0, &payload, &payload_len)){
                    if(payload){
                       dprintf(MSG_INFO, "%s: [EMEX] inserting tlv \n", __func__);
                       TLV = ieee1905TLVGetNext(TLV);
                       ieee1905TLVTypeSet(TLV, TLV_1905_VENDOR_SPECIFIC);
                       ieee1905TLVLenSet(TLV, payload_len, frameLen);
                       ieee1905TLVValSet(TLV,payload,payload_len);
                       free(payload);
                       payload = NULL;
                    }
                }
#endif
            }
        }
#endif

        /* Add Supported feature list Vendor specific TLV for both SON and MAP features */
        TLV = ieee1905AddFeatureFlagTLV(TLV, &frameLen, pData);

        /* Send Vendor tlv defined by Northbound application via
         * NBTLVbuff config from wsplcd
         */
        if (pData->config.enable_NB_tlv == 1) {
            int msgTlv = 0;
            int vendorTlv = 0;
            int payload_length = 0;
            char payload[256];
            int ret = 0;
            ret = apac_get_NBbuff(pData->config.nbtlvbuff, &msgTlv, &vendorTlv, &payload_length, payload);
            if((ret == 0) && (msgTlv == IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_SEARCH))  {
                bufferLen = 0;
                ieee1905QCANorthBoundTLV_t *NorthBoundTLV;
                TLV = ieee1905TLVGetNext(TLV);

                qcaMessage =
                    (ieee1905QCAMessage_t *)ieee1905TLVValGet(TLV);
                type = vendorTlv;

                ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_VENDOR_SPECIFIC);

                ieee1905QCAOUIAndTypeSet(qcaMessage, type, bufferLen);
                NorthBoundTLV = (ieee1905QCANorthBoundTLV_t *)qcaMessage->content;
                NorthBoundTLV->length = payload_length;
                memcpy((char *)&NorthBoundTLV->content, payload, NorthBoundTLV->length);
                bufferLen += sizeof(ieee1905QCANorthBoundTLV_t) + NorthBoundTLV->length;

                ieee1905TLVLenSet(TLV, bufferLen, frameLen);
            } else {
                dprintf(MSG_ERROR,"%s: invalid NBbuffTLV entry, skipping vendortlv \n",__func__);
            }
        }

        /* Set EndOfTLV */
        TLV = ieee1905TLVGetNext(TLV);
        ieee1905EndOfTLVSet(TLV);

#if MAP_ENABLED
        /* Send packet on bridge if Traffic Separation is Enabled */
        if (apacHyfiMapIsTrafficSeparationEnabled(HYFI20ToMAP(pData)) ||
            apacHyfiMapIsSendPacketOnBridge(HYFI20ToMAP(pData))) {
            if (send(pData->bridge.sock, frame, frameLen, 0) < 0) {
                perror("apacHyfi20SendSearchE");
            }
        }
#endif

        dprintf(MSG_INFO, "Sending search msg mid: %d on all Iface for freq: %d\n", mid, freq);
        for (j = 0; j < APAC_MAXNUM_HYIF; j++) {
            if (pIF[j].ifIndex != -1) {
                if (apacHyfi20SendL2Packet(&pIF[j], frame, frameLen) < 0) {
                    perror("apacHyfi20SendSearchE"); /* could be configured AP is not started yet */
                    //return -1;
                }
                dprintf(MSG_MSGDUMP, "%s - Sent msg mid: %d on %s for freq: %d\n", __func__, mid,
                        pIF[j].ifName, freq);
            }
        }
#if MAP_ENABLED
        //send packet to lo interface
        if (apacHyfiMapIsUseLoInterface(HYFI20ToMAP(pData))) {
            for (j = 0; j < APAC_MAXNUM_HYIF; j++) {
                if (pIF[j].ifIndex == -1 || strcmp(pIF[j].ifName, "lo")) {
                        continue;
                }
                if (apacHyfi20SendL2Packet(&pIF[j], frame, frameLen) < 0) {
                    perror("apacHyfi20SendSearchE"); /* could be configured AP is not started yet */
                    //return -1;
                }
                dprintf(MSG_DEBUG, "%s - Sent msg mid: %d on %s \n", __func__, mid, pIF[j].ifName);
            }
        }
#endif
    }

    /* Set state after sending Search Msg */
    pData->config.state = APAC_E_WAIT_RESP;

    /* Wifi DB band selection enabled */
    if (pData->config.wlan_chip_cap == APAC_DB && pData->config.band_sel_enabled) {
        eloop_register_timeout(pData->config.rm_collect_to, 0,
                        apacHyfi20RMCollectTimeoutHandler, pData, NULL);
    }
    return 0;
}

#if MAP_ENABLED
void apacHyfi20RenewTimeoutHandler(void *eloop_ctx, void *timeout_ctx) {
    apacHyfi20Data_t *pData = (apacHyfi20Data_t *)eloop_ctx;
    const u16 *interval = (const u16 *)timeout_ctx;
    apacHyfi20AP_t *pAP = pData->ap;
    apacHyfi20Config_t *pConfig = &pData->config;

    s32 i;
    u8 tlvValue;
    u8 dest[ETH_ALEN] = APAC_MULTICAST_ADDR;
    u32 frameLen = IEEE1905_FRAME_MIN_LEN;
    u8 *frame = apacHyfi20GetXmitBuf();
    ieee1905TLV_t *TLV = (ieee1905TLV_t *)((ieee1905Message_t *)frame)-> content;
    ieee1905TLV_t *TLV2;
    u8 freq = 0;
    u32 frameLen2;
    u16 mid = 0;

    apacHyfi20TRACE();

    apacHyfi20SetPktHeader(frame,IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_RENEW,
            mid, 0, IEEE1905_HEADER_FLAG_LAST_FRAGMENT | IEEE1905_HEADER_FLAG_RELAY,
            pData->alid, dest);

    /* Add ALID TLV */
    ieee1905TLVSet(TLV, IEEE1905_TLV_TYPE_AL_ID, ETH_ALEN, pData->alid, frameLen);
    TLV = ieee1905TLVGetNext(TLV);

    /* Add SupportedRole TLV */
    tlvValue = APAC_REGISTRAR;
    ieee1905TLVSet(TLV, IEEE1905_TLV_TYPE_SUPPORTED_ROLE, APAC_TLVLEN_ROLE, &tlvValue, frameLen);
    dprintf(MSG_DEBUG, "frameLen: %d\n", frameLen);

    for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
        if (pAP[i].valid == APAC_FALSE) {
            continue;
        }

        mid = alid_list.mid;
        ((ieee1905Message_t *)frame)->ieee1905Header.mid = htons(mid);

        TLV2 = TLV;
        freq = pAP[i].freq;
        frameLen2 = frameLen;

        TLV2 = ieee1905TLVGetNext(TLV2);
        ieee1905TLVSet(TLV2, IEEE1905_TLV_TYPE_SUPPORTED_FREQ_BAND, APAC_TLVLEN_FREQ,
                &freq, frameLen2);

        /* Set EndOfTLV */
        TLV2 = ieee1905TLVGetNext(TLV2);
        ieee1905EndOfTLVSet(TLV2);

        u8 j = 0;
        apacMapData_t *mapData = HYFI20ToMAP(pData);
        apacHyfi20IF_t *pIF = pData->hyif;

        if (mapData && mapData->unicastRenewEnabled) {
            //apac_alid alidEntries[APAC_MAP_ALID_DB_MAX_ALID_COUNT] = {0};

            ((ieee1905Message_t *)frame)->ieee1905Header.flags &= ~IEEE1905_HEADER_FLAG_RELAY;

            for (u8 count = 0; count < alid_list.no_of_alid; count++) {
                // change destination address to unicast address (stored alid)
                if (!alid_list.renew_acknowledged[count]) {
                    dprintf(MSG_ERROR, " %d Retransmit Renew to ALID ", alid_list.no_of_alid); printMac(MSG_DEBUG, alid_list.alid[count].addr);
                    memcpy(((ieee1905Message_t *)frame)->etherHeader.ether_dhost, alid_list.alid[count].addr, ETH_ALEN);

                    /* Send packet on bridge if Traffic Separation is Enabled */
                    if (apacHyfiMapIsTrafficSeparationEnabled(HYFI20ToMAP(pData))) {
                        if (send(pData->bridge.sock, frame, frameLen2, 0) < 0) {
                            perror("apacHyfi20RenewTimeoutHandler");
                        }
                    }

                    // send the modified frame
                    for (j = 0; j < APAC_MAXNUM_HYIF; j++) {
                        if (pIF[j].ifIndex != -1) {
                            if (apacHyfi20SendL2Packet(&pIF[j], frame, frameLen2) < 0) {
                                perror("apacHyfi20RenewTimeoutHandler");
                            }
                            dprintf(MSG_DEBUG, "%s - Sent unicast msg mid: %d on IF%d for freq: %d\n", __func__, mid,
                                    j, i);
                        }
                    }
                } else {
                    dprintf(MSG_ERROR, "%s: Failed to get stored alid entries. Not sending unicast renews\n", __func__);
                }
            }
        } else {
            dprintf(MSG_INFO, "%s: Not configured to send unicast renews\n", __func__);
        }
        break;
    }
    /* register PB search timeout callback */
    alid_list.max_renew_retry--;
    if ((alid_list.max_renew_retry != 0) || alid_list.no_of_alid)
        eloop_register_timeout(interval?*interval:pConfig->cfg_renew_retransmit_interval , 0,
                apacHyfi20RenewTimeoutHandler, pData, (void *)interval);

}
#endif

/********************
 **** REGISTRAR *****
 ********************/
/* Registrar receives Search msg */
int apacHyfi20ReceiveSearchR(apacHyfi20Data_t *ptrData, u8 *msg, u32 msgLen) {
    ieee1905Message_t *frame = (ieee1905Message_t *)msg;
    ieee1905TLV_t *TLV = (ieee1905TLV_t *)frame->content;
    ieee1905TlvType_e tlvType;
    u32 processedLen;
    u8 *tlvVal;
    u8 src_alid[ETH_ALEN];
    u16 mid = ntohs(frame->ieee1905Header.mid);
    u8 isfrag = frame->ieee1905Header.flags & IEEE1905_HEADER_FLAG_LAST_FRAGMENT;
    int num_tlv = 5;
#if MAP_ENABLED
    u8 count = 0, retransmit_renew = 0;
#endif
    p1905TlvCheck_t tlvList[num_tlv];
    u8 freq;
#if SON_ENABLED
    u8 multi_M2_support=0;
#elif MAP_ENABLED
    u32 mapFeatureData=0;
#endif
    apacBool_e isFreqRequested[APAC_NUM_WIFI_FREQ] = {APAC_FALSE};
    apacBool_e isRoleMatching = APAC_FALSE;
#if MAP_ENABLED
    u8 mapVersion = 0, searchReqFreq = 0;
#if DOT1X_SUPPORT
    u8 dot1xSupport=0;
#endif
    apacBool_e isMapCapable = APAC_FALSE;
    apacBool_e isSupportedServiceMatch = APAC_FALSE;
    apacBool_e isSearchedServiceMatch = APAC_FALSE;
    apacMapData_t *mapData = HYFI20ToMAP(ptrData);
#endif

#if (defined QCA_EXT_EM_CONTROLLER && defined MAP_ENABLED)
    u8 supportedService = 0;
#endif

    apacHyfi20TRACE();
    memcpy(src_alid, frame->etherHeader.ether_shost, ETH_ALEN);

#if MAP_ENABLED
    dprintf(MSG_DEBUG, "Autoconfig Search received from Agent ALID");
    printMac(MSG_DEBUG, src_alid);
    for (count = 0; alid_list.no_of_alid > count; count++) {

        if (!os_memcmp(alid_list.alid[count].addr, src_alid, ETH_ALEN)) {
            alid_list.renew_acknowledged[count] = APAC_TRUE;
            dprintf(MSG_INFO, "Search received Renew message acknowledged, src addr");
            printMac(MSG_DEBUG, alid_list.alid[count].addr);
        }

        if (!alid_list.renew_acknowledged[count])
            retransmit_renew = APAC_TRUE;

    }
    if (!retransmit_renew) {
        eloop_cancel_timeout(apacHyfi20RenewTimeoutHandler, ptrData, NULL);
        memset(&alid_list, 0, sizeof(alid_list));
        dprintf(MSG_DEBUG, "No pending Search message\n");
    }

#endif

    if (!((ptrData->config.state == APAC_R_NO_PB) || (ptrData->config.state == APAC_R_PB_WAIT_SEARCH))) {
        dprintf(MSG_DEBUG, "State(%d): discard Search Msg\n", ptrData->config.state);
        return -1;
    }

    if (ntohs(frame->ieee1905Header.type) != IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_SEARCH) {
        dprintf(MSG_ERROR, "Msg Type %u is not SEARCH(7)\n", ntohs(frame->ieee1905Header.type));
        return -1;
    }

    /* Message verification. Search Msg TLVs: 1 ALID, 1 SearchRole, 1 FreqBand */
    os_memset(tlvList, 0, sizeof(p1905TlvCheck_t)*num_tlv);


    apacHyfi20AddTlvToList(tlvList, num_tlv, IEEE1905_TLV_TYPE_AL_ID, APAC_FALSE);
    apacHyfi20AddTlvToList(tlvList, num_tlv, IEEE1905_TLV_TYPE_SEARCHED_ROLE, APAC_FALSE);
    apacHyfi20AddTlvToList(tlvList, num_tlv, IEEE1905_TLV_TYPE_FREQ_BAND, APAC_FALSE);

#if MAP_ENABLED
    if (apacHyfiMapIsEnabled(HYFI20ToMAP(ptrData))) {
        apacHyfi20AddTlvToList(tlvList, num_tlv, IEEE1905_TLV_TYPE_SEARCHED_SERVICE, APAC_FALSE);
        apacHyfi20AddTlvToList(tlvList, num_tlv, IEEE1905_TLV_TYPE_SUPPORTED_SERVICE, APAC_FALSE);
    }
#endif

    /* Parse TLV */
    processedLen = ieee1905TLVLenGet(TLV) + IEEE1905_ETH_HEAD_LEN + IEEE1905_HEAD_LEN;
    while (processedLen <= msgLen) {
        tlvType = ieee1905TLVTypeGet(TLV);
        dprintf(MSG_MSGDUMP, "Get TLV type: %d\n", tlvType);

        if (tlvType == IEEE1905_TLV_TYPE_END_OF_MESSAGE) {
            break;
        }

        tlvVal = ieee1905TLVValGet(TLV);

        /* To get the Second Byte */
        if(tlvType == IEEE1905_TLV_TYPE_SEARCHED_SERVICE)
            tlvVal++;

        /* Remove check to not return when we rx r2 TLV on r1 device */
#if MAP_ENABLED
        if (!apacHyfiMapIsEnabled(HYFI20ToMAP(ptrData))) {
#endif
            if (apacHyfi20FindTlvInList(tlvList, tlvType, num_tlv) == APAC_FALSE) {
                dprintf(MSG_ERROR, "%s - Unexpected tlv type %d, discard\n", __func__, tlvType);
                return -1;
            }
#if MAP_ENABLED
        }
#endif

        if (tlvType ==  IEEE1905_TLV_TYPE_SEARCHED_ROLE &&
            *tlvVal == APAC_REGISTRAR)
        {
            isRoleMatching = APAC_TRUE;
        }
        else if (tlvType == IEEE1905_TLV_TYPE_FREQ_BAND) {
            if ((*tlvVal < APAC_WIFI_FREQ_2) || (*tlvVal > APAC_WIFI_FREQ_6_OTHER))
            {
                dprintf(MSG_ERROR, "Invalid freq#%d\n", *tlvVal);
                return -1;
            }
            isFreqRequested[*tlvVal] = APAC_TRUE;
#if MAP_ENABLED
            searchReqFreq = *tlvVal;
#endif
        }
        else if (tlvType == IEEE1905_TLV_TYPE_AL_ID) {
            /* Check MID and discard the ones that have seen */
            if (apacHyfi20CheckMid(tlvVal, mid, isfrag) < 0) {
#if MAP_ENABLED
                if (!apacHyfiMapIsEnabled(HYFI20ToMAP(ptrData))) {
#endif
                    return -1;
#if MAP_ENABLED
                }
#endif
            }

            /* use src ALID as the destination address */
            memcpy(src_alid, tlvVal, ETH_ALEN);
        }
#if MAP_ENABLED
        else if (tlvType == IEEE1905_TLV_TYPE_SEARCHED_SERVICE &&
                *tlvVal == APAC_MAP_CONTROLLER) {
            isSearchedServiceMatch = APAC_TRUE;
        } else if (tlvType == IEEE1905_TLV_TYPE_SUPPORTED_SERVICE && *tlvVal == APAC_MAP_AGENT) {
            isSupportedServiceMatch = APAC_TRUE;
            mapVersion = APAC_MAP_VERSION_1;
        }
#ifdef QCA_EXT_EM_CONTROLLER
        else if (tlvType == IEEE1905_TLV_TYPE_SUPPORTED_SERVICE) {
            TlvSupportedServiceTypeExtract(tlvVal, &supportedService, ieee1905TLVLenGet(TLV));
            if (IS_MULTIAP_CONTROLLER(supportedService) && !IS_EMEX_CONTROLLER(supportedService)) {
                dprintf(MSG_ERROR,"Non-EM+ controller detected!\n");
                if (i5MessageEMEXConfig3rdPartyOnboarding() <= 0) {
                    dprintf(MSG_ERROR,"3rd party onboarding is disabled, not processing the search\n");
                    return -1;
                }
            }
        }
#endif
        else if (tlvType == IEEE1905_TLV_TYPE_MAP_VERSION) {
            if ((apacHyfiMapIsEnabled(HYFI20ToMAP(ptrData)) >= APAC_MAP_VERSION_2) &&
                (*tlvVal >= APAC_MAP_VERSION_2)) {
                dprintf(MSG_DEBUG, " Controller and Enrollee MAP Version %d \n", *tlvVal);
                mapVersion = *tlvVal;
            }
        }
#endif
        else if (tlvType == IEEE1905_TLV_TYPE_VENDOR_SPECIFIC) {
            ieee1905QCASupportedFeatureList_t *featureList;
            ieee1905QCAMessage_t *qcaMessage =
                (ieee1905QCAMessage_t *) tlvVal;

            if(ieee1905QCAIsQCAOUI(qcaMessage->oui) &&
                    qcaMessage->type == IEEE1905_QCA_TYPE_FEATURE_SUPPORT_LIST) {
                featureList = (ieee1905QCASupportedFeatureList_t *) qcaMessage->content;
#if SON_ENABLED
                multi_M2_support = featureList->sonFeatureList.MultipleM2Support;
#elif MAP_ENABLED
                mapFeatureData = featureList->mapFeatureList.reservedData[0];
                dprintf(MSG_DEBUG, "MAP Feature supported flag : %x \n", mapFeatureData);
#endif
#if MAP_ENABLED
#if DOT1X_SUPPORT
                if (apacHyfiMapIsDot1xEnabled(mapData))
                    dot1xSupport = featureList->mapFeatureList.dot1xSupport;
                dprintf(MSG_DEBUG, "%s:%d> dot1xSupport=%d \n", __func__, __LINE__, dot1xSupport);
#endif
#endif
            }
        }

        processedLen += ieee1905TLVLenGet(TLV);
        TLV = ieee1905TLVGetNext(TLV);
    }

    if (isRoleMatching == APAC_FALSE) {
        dprintf(MSG_ERROR, "Search Msg not looking for Registrar!\n");
        return -1;
    }

    /* Check if all required TLVs have been received. Ignore check for MAP */
#if MAP_ENABLED
    if (!apacHyfiMapIsEnabled(HYFI20ToMAP(ptrData))) {
#endif
        if (!apacHyfi20ValidateTlvList(tlvList, num_tlv)) {
            dprintf(MSG_ERROR, "required TLV type missing in the message!\n");
            return -1;
        }
#if MAP_ENABLED
    }
#endif

#if MAP_ENABLED
    if (apacHyfiMapIsEnabled(HYFI20ToMAP(ptrData))) {
        if (!isSupportedServiceMatch) {
            dprintf(MSG_ERROR, "Search Msg not Supporting Map Agent Mode !\n");
            return -1;
        }
        if (!isSearchedServiceMatch) {
            dprintf(MSG_ERROR, "Search Msg not Looking for MAP Controller \n");
            return -1;
        }
        isMapCapable = isSupportedServiceMatch && isSearchedServiceMatch;
    }
#endif

#if MAP_ENABLED
    if (apacHyfiMapPfComplianceEnabled(mapData) && isFreqRequested[searchReqFreq]) {
        dprintf(MSG_MSGDUMP, "Send response for Search freq#%d\n", searchReqFreq);
        apacHyfi20SendResponseR(ptrData, src_alid, searchReqFreq, mid, isMapCapable);
    } else {
#endif
        /* Send response msg */
        for (freq = 0; freq < APAC_NUM_WIFI_FREQ; freq++) {
            if (isFreqRequested[freq]) {
                dprintf(MSG_INFO, "Send response for freq#%d\n", freq);
#if MAP_ENABLED
                apacHyfi20SendResponseR(ptrData, src_alid, freq, mid, isMapCapable);
#else
                apacHyfi20SendResponseR(ptrData, src_alid, freq, mid, APAC_FALSE);
#endif
            }
        }
#if MAP_ENABLED
    }
#endif
    if (ptrData->config.state == APAC_R_PB_WAIT_SEARCH) {
        ptrData->config.state = APAC_R_PB_WAIT_M1;
    }

    /*delete unfinished WPS session if any*/
#if MAP_ENABLED
    if ((apacHyfiMapIsEnabled(HYFI20ToMAP(ptrData)) < APAC_MAP_VERSION_6) &&
                (mapVersion < APAC_MAP_VERSION_6))
#endif
    {
        struct apac_wps_session* sess;
        sess = apac_wps_find_session(ptrData, src_alid);
        if (sess)
        {
            dprintf(MSG_INFO, "Searching received from Enrollee, delete stale session!\n");
            apac_wps_del_session(sess);
        }
    }

    struct apac_wps_session *sess;
    sess = apac_wps_find_session(ptrData, src_alid);
    /* session not found */
    if (!sess) {

        /* initialize the start session*/
        sess = apac_wps_new_session(ptrData);
        if (!sess) {
            dprintf(MSG_ERROR, "can't open new session!\n");
            return -1;
        }

        /* write sender's mac in the session data */
        os_memcpy(sess->dest_addr, src_alid, ETH_ALEN);
    }

#if MAP_ENABLED
    if	(sess->stateR == APAC_WPS_R_INIT) {
        /* Check if both the device is MAP R6 Capable and Controller support EAP CAPsupports */
        if ((apacHyfiMapIsEnabled(HYFI20ToMAP(ptrData)) >= APAC_MAP_VERSION_6) &&
                (mapData->earlyAPCapabilitySupported == 1) &&
                (mapVersion >= APAC_MAP_VERSION_6))  {
            /*Change state to wait for EAP CAP */
            sess->stateR = APAC_WPS_R_WAIT_EAP_CAP;
            dprintf(MSG_INFO, "%s, %d sess->stateR  = %d",__func__, __LINE__,  sess->stateR);
        }
    }
    /* Create new session and save MAP Version */
    if (apacHyfiMapIsEnabled(HYFI20ToMAP(ptrData))) {
        sess->mapVersion = mapVersion;
        if (mapData && mapVersion == APAC_MAP_VERSION_1) {
            mapData->r1AgentInNw = APAC_TRUE;
        }
    }
#if DOT1X_SUPPORT
    if (apacHyfiMapIsDot1xEnabled(mapData))
        sess->dot1xSupport = dot1xSupport;
    dprintf(MSG_DEBUG, "%s:%d> dot1xSupport=%d \n", __func__, __LINE__, dot1xSupport);
#endif
#endif
#if SON_ENABLED
    sess->multiM2Support = multi_M2_support;
#endif
    dprintf(MSG_INFO, "%s after finding session\n", __func__);
    dprintf(MSG_INFO, "src: "); printMac(MSG_INFO, sess->own_addr);
    dprintf(MSG_INFO, "dest: "); printMac(MSG_INFO, sess->dest_addr);
#if MAP_ENABLED
    dprintf(MSG_INFO, "%s Map Version %d \n", __func__, sess->mapVersion);
    dprintf(MSG_INFO, "%s R1 Agent Present in Network %d \n", __func__, mapData->r1AgentInNw);
#endif
    return 0;
}

/* This function parses WiFi7 Agent Capabilities received from the agent
   and decides if this agent supports MLO configurations */
u8 apacHyfi20GetMLOEnabledFromWiFi7AgentCap(struct apac_wps_session *sess) {
    u8 *ptr = NULL;
    u_int8_t MLOsupported = 0;
    u_int8_t numRadios = 0;

    ptr = sess->wifi7AgentCap;
    // Skip max num MLDs
    ptr++;
    // Skip max num links
    ptr++;
    // Skip t2lm capability
    ptr++;
    // Skip reserved
    ptr += 13;
    // Skip numRadios
    numRadios = *ptr;
    if(numRadios) {
        ptr++;
        // Skip RUID
        ptr += ETH_ALEN;
        // Skip STA non MLO
        ptr++;
        // Reserved
        ptr += 23;
        // Skip AP MLO mode support
        ptr++;
        // Skip bSTA MLO mode support
        ptr++;
        // Skip num of ap str records
        MLOsupported = *ptr;
        if(MLOsupported)
            return 1;
        else
            return 0;
    }
    return 0;
}

/* Registrar recieves early AP capability message from Enrollee.
   For now, we do not save all TLVs. We only save AP basic radio
   capability and WiFi7 AP agent capability TLVs */
int apacHyfi20ReceiveEarlyAPCapabilityReport(apacHyfi20Data_t *ptrData, u8 *msg, u32 msgLen) {
    ieee1905Message_t *frame = (ieee1905Message_t *)msg;
    ieee1905TLV_t *TLV = (ieee1905TLV_t *)frame->content;
    ieee1905TlvType_e tlvType;
    u32 processedLen;
    u8 src_alid[ETH_ALEN];
    u8 *ptr = NULL;
    u8 radioCount = 0;
    struct apac_wps_session* sess;

    memcpy(src_alid, frame->etherHeader.ether_shost, ETH_ALEN);

    sess = apac_wps_find_session(ptrData, src_alid);
    if (!sess)
    {
        dprintf(MSG_INFO, "Invalid Session discard Early AP capability Msg!\n");
        return -1;
    }

    apacHyfi20TRACE();

    if (sess->stateR != APAC_WPS_R_WAIT_EAP_CAP) {
        dprintf(MSG_INFO, "State(%d): discard Early AP capability Msg\n", sess->stateR);
        return -1;
    }
    else {
        sess->stateR = APAC_WPS_R_WAIT_M1;
        dprintf(MSG_INFO, "State(%d): Early AP capability Msg \n", sess->stateR);
    }

    sess->rxdEarlyAPCapability = 1;

    /* Parse TLV */
    processedLen = ieee1905TLVLenGet(TLV) + IEEE1905_ETH_HEAD_LEN + IEEE1905_HEAD_LEN;
    while (processedLen <= msgLen) {
        tlvType = ieee1905TLVTypeGet(TLV);
        dprintf(MSG_ERROR, "Get TLV type: %d\n", tlvType);

        if (tlvType == IEEE1905_TLV_TYPE_END_OF_MESSAGE) {
            break;
        }
        else if(tlvType == IEEE1905_TLV_TYPE_AP_CAP) {
            dprintf(MSG_ERROR, "Recieved IEEE1905_TLV_TYPE_AP_CAP "
                                "(%d)\n", tlvType);
        }
        else if (tlvType == IEEE1905_TLV_TYPE_AP_RADIO_BASIC_CAP) {
            dprintf(MSG_ERROR, "Recieved IEEE1905_TLV_TYPE_AP_RADIO_BASIC_CAP "
                                "(%d)\n", tlvType);
            u8 size = ieee1905TLVLenGet(TLV);
            ptr = ieee1905TLVValGet(TLV);
            memcpy(sess->basicRadioCapMLO[radioCount], ptr, size);
            radioCount++;
        }
        else if (tlvType == IEEE1905_TLV_TYPE_R2_APCAP) {
            dprintf(MSG_ERROR, "Recieved IEEE1905_TLV_TYPE_R2_APCAP "
                                "(%d)\n", tlvType);
        }
        else if (tlvType == IEEE1905_TLV_TYPE_METRIC_COLLECTION_INTERVAL) {
            dprintf(MSG_ERROR, "Recieved IEEE1905_TLV_TYPE_METRIC_COLLECTION_INTERVAL "
                                "(%d)\n", tlvType);
        }
        else if (tlvType == IEEE1905_TLV_TYPE_DEVICE_INVENTORY) {
            dprintf(MSG_ERROR, "Recieved IEEE1905_TLV_TYPE_DEVICE_INVENTORY "
                                "(%d)\n", tlvType);
        }
        else if (tlvType == IEEE1905_TLV_TYPE_WIFI7_AGENT_CAPABILITY) {
            dprintf(MSG_ERROR, "Recieved IEEE1905_TLV_TYPE_WIFI7_AGENT_CAPABILITY"
                                "(%d)\n", tlvType);

            ptr = ieee1905TLVValGet(TLV);
            memcpy(sess->wifi7AgentCap, ptr, 1024);
            sess->mloSupport = apacHyfi20GetMLOEnabledFromWiFi7AgentCap(sess);
            dprintf(MSG_ERROR, "mloSupport = %d", sess->mloSupport);
        }

        TLV = ieee1905TLVGetNext(TLV);
        processedLen += ieee1905TLVLenGet(TLV);
    }

    sess->radioCount = radioCount;

    return 0;
}

/* Registrar sends Response msg */
int apacHyfi20SendResponseR(apacHyfi20Data_t *pData, u8 *dest_mac, u8 freq, u16 mid,
                            apacBool_e isMapCapable) {
    u8 tlvValue;
    u32 frameLen = IEEE1905_FRAME_MIN_LEN;
    u8 *frame = apacHyfi20GetXmitBuf();
    u8 *payload = ((ieee1905Message_t *)frame)->content;
    ieee1905TLV_t *TLV = (ieee1905TLV_t *)payload;
    int ret = 0;
#if MAP_ENABLED
    u_int16_t tlvLen = 0;
    u_int8_t *content;
    apacMapData_t *mapData = HYFI20ToMAP(pData);
#endif /* MAP_ENABLED */

    apacHyfi20TRACE();

#if MAP_ENABLED
    if (apacHyfiMapConfigServiceEnabled(mapData)) {
        dprintf(MSG_DEBUG, "%s,  Dont send APAC Response. Config Service Enabled in HYD\n",
                __func__);
        return ret;
    }
#endif

    apacHyfi20SetPktHeader(frame, IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_RESPONSE,
        mid, 0, IEEE1905_HEADER_FLAG_LAST_FRAGMENT, pData->alid, dest_mac);

    /* Add SupportedRole TLV */
    tlvValue = APAC_REGISTRAR;
    ieee1905TLVSet(TLV, IEEE1905_TLV_TYPE_SUPPORTED_ROLE, APAC_TLVLEN_ROLE, &tlvValue, frameLen);

    /* Set SupportedFreq TLV */
    TLV = ieee1905TLVGetNext(TLV);
    ieee1905TLVSet(TLV, IEEE1905_TLV_TYPE_SUPPORTED_FREQ_BAND, APAC_TLVLEN_ROLE, &freq, frameLen);

#if MAP_ENABLED
    if (isMapCapable) {
        // meaning both agent and controller are map Capable irrespective of version
        u_int8_t  tlvValue;
        u_int8_t  listSupportedServices;
        tlvValue = APAC_MAP_CONTROLLER;
        listSupportedServices = 1;

        TLV = ieee1905TLVGetNext(TLV);
        ieee1905TLVTypeSet( TLV, IEEE1905_TLV_TYPE_SUPPORTED_SERVICE );

        /* Get a pointer to the value field. We need to construct it manually */
        content = ieee1905TLVValGet( TLV );
        tlvLen = 0;

        /* Setup List of Supported Services */
        *content = listSupportedServices;
        content++; tlvLen++;

        /* Setup List of Supported Services */
        *content = tlvValue;
        content++; tlvLen++;
        /* Setup device information TLV length */
        ieee1905TLVLenSet( TLV, tlvLen, frameLen );

        if (apacHyfiMapIsEnabled(HYFI20ToMAP(pData)) >= APAC_MAP_VERSION_2) {
            /* One Multi-AP Version TLV  */
            TLV = ieee1905TLVGetNext(TLV);
            ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_MAP_VERSION);

            content = ieee1905TLVValGet(TLV);
            tlvLen = 0;

            *content = apacHyfiMapIsEnabled(HYFI20ToMAP(pData));
            content++; tlvLen++;

            ieee1905TLVLenSet(TLV, tlvLen, frameLen);
        }

        if (apacHyfiMapIsEnabled(HYFI20ToMAP(pData)) >= APAC_MAP_VERSION_6) {
            /* One Controller Capability TLV  */
            TLV = ieee1905TLVGetNext(TLV);
            ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_CONTROLLER_CAPABILITY);

            content = ieee1905TLVValGet(TLV);
            tlvLen = 0;

            /* Bit 7 - KiBMiB Counter */
            /* Bit 6 - Accept Early AP Capability Report */
            *content = 0;
            *content |= (1 << 7);
            *content |= (mapData->earlyAPCapabilitySupported << 6);
            content++; tlvLen++;

            ieee1905TLVLenSet(TLV, tlvLen, frameLen);
        }
    }
#endif /* MAP_ENABLED */

    /* Set EndOfTLV */
    TLV = ieee1905TLVGetNext(TLV);
    ieee1905EndOfTLVSet(TLV);

    /* Send the packt */
    if (pData->config.sendOnAllIFs == APAC_FALSE) {
        if (send(pData->bridge.sock, frame, frameLen, 0) < 0) {
            perror("apacHyfi20SendResponseR");
            ret = -1;
        }
    }
    else {  /* send unicast packet on all interfaces. Debug Only! */
        int i;
#if MAP_ENABLED
        if (apacHyfiMapIsTrafficSeparationEnabled(HYFI20ToMAP(pData))) {
            if (send(pData->bridge.sock, frame, frameLen, 0) < 0) {
                perror("apacHyfi20SendResponseR-onAllIFs");
                return -1;
            }
        }
#endif

        for (i = 0; i < APAC_MAXNUM_HYIF; i++) {
            if (pData->hyif[i].ifIndex == -1)
                continue;

            if (apacHyfi20SendL2Packet(&pData->hyif[i], frame, frameLen) < 0) {
                perror("apacHyfi20SendResponseR-onAllIFs");
                ret = -1;
            }
        }
    }

    dprintf(MSG_INFO, "%s - Sent msg mid: %d\n", __func__, mid);
    return ret;
}

/* Registrar sends Renewal msg */
int apacHyfi20SendRenewalR(apacHyfi20Data_t *pData){
    apacHyfi20IF_t *pIF = pData->hyif;
    apacHyfi20AP_t *pAP = pData->ap;

    s32 i, j;
    u8 tlvValue;
    u8 dest[ETH_ALEN] = APAC_MULTICAST_ADDR;
    u32 frameLen = IEEE1905_FRAME_MIN_LEN;
    u8 *frame = apacHyfi20GetXmitBuf();
    ieee1905TLV_t *TLV = (ieee1905TLV_t *)((ieee1905Message_t *)frame)-> content;
    ieee1905TLV_t *TLV2;
    u8 freq = 0;
    u32 frameLen2;
    u16 mid = 0;

    apacHyfi20TRACE();

    apacHyfi20SetPktHeader(frame,IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_RENEW,
        mid, 0, IEEE1905_HEADER_FLAG_LAST_FRAGMENT | IEEE1905_HEADER_FLAG_RELAY,
        pData->alid, dest);

    /* Add ALID TLV */
    ieee1905TLVSet(TLV, IEEE1905_TLV_TYPE_AL_ID, ETH_ALEN, pData->alid, frameLen);
    TLV = ieee1905TLVGetNext(TLV);

    /* Add SupportedRole TLV */
    tlvValue = APAC_REGISTRAR;
    ieee1905TLVSet(TLV, IEEE1905_TLV_TYPE_SUPPORTED_ROLE, APAC_TLVLEN_ROLE, &tlvValue, frameLen);
    dprintf(MSG_DEBUG, "frameLen: %d\n", frameLen);

    /* Set freq TLV and send IEEE 1905 Packet from each configured IF */
    for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
        if (pAP[i].valid == APAC_FALSE) {
            continue;
        }

        /* set up new MID for each band */
        mid = apacHyfi20GetMid();
        ((ieee1905Message_t *)frame)->ieee1905Header.mid = htons(mid);

        TLV2 = TLV;
        freq = pAP[i].freq;
        frameLen2 = frameLen;

        TLV2 = ieee1905TLVGetNext(TLV2);
        ieee1905TLVSet(TLV2, IEEE1905_TLV_TYPE_SUPPORTED_FREQ_BAND, APAC_TLVLEN_FREQ,
                &freq, frameLen2);

        /* Set EndOfTLV */
        TLV2 = ieee1905TLVGetNext(TLV2);
        ieee1905EndOfTLVSet(TLV2);

#if MAP_ENABLED
        /* Send packet on bridge if Traffic Separation is Enabled */
        if (apacHyfiMapIsTrafficSeparationEnabled(HYFI20ToMAP(pData)) ||
            apacHyfiMapIsSendPacketOnBridge(HYFI20ToMAP(pData))) {
            if (send(pData->bridge.sock, frame, frameLen2, 0) < 0) {
                perror("apacHyfi20SendRenewalR");
            }
        }
#endif

        for (j = 0; j < APAC_MAXNUM_HYIF; j++) {
            if (pIF[j].ifIndex != -1) {
                if (apacHyfi20SendL2Packet(&pIF[j], frame, frameLen2) < 0) {
                    perror("apacHyfi20SendRenewalR");
                    // return -1;
                }
                dprintf(MSG_INFO, "%s - Sent msg mid: %d on IF%d for freq: %d\n", __func__, mid,
                        j, i);
            }
        }
#if MAP_ENABLED
        /**
         * From field testing, it found that multicast renew is not reliable, so
         * some agents may not receive it and fail to clone the latest credentials.
         * To improve reliability we send unicast renew as well to all the agent alids
         * that were stored in alid db file when M2 was sent.
         *
         * For unicast, the same multicast frame will be used, but the destination
         * address will be replaced with unicast address and relay bit in flags will
         * be reset.
         **/
        apacHyfi20Config_t *pConfig = &pData->config;
        apacMapData_t *mapData = HYFI20ToMAP(pData);
        if (mapData && mapData->unicastRenewEnabled) {
            u8 entryCount = APAC_MAP_ALID_DB_MAX_ALID_COUNT;
            if (apac_map_alid_db_get_all_alid(alid_list.alid, &entryCount)){
                // reset relay bit in flag
                ((ieee1905Message_t *)frame)->ieee1905Header.flags &= ~IEEE1905_HEADER_FLAG_RELAY;
                alid_list.no_of_alid = entryCount;
                alid_list.mid = mid;
                for (u8 count = 0; count < entryCount; count++) {
                    // change destination address to unicast address (stored alid)
                    memcpy(((ieee1905Message_t *)frame)->etherHeader.ether_dhost, alid_list.alid[count].addr, ETH_ALEN);

                    /* Send packet on bridge if Traffic Separation is Enabled */
                    if (apacHyfiMapIsTrafficSeparationEnabled(HYFI20ToMAP(pData))) {
                        if (send(pData->bridge.sock, frame, frameLen2, 0) < 0) {
                            perror("apacHyfi20SendRenewalR");
                        }
                    }
                    // send the modified frame
                    for (j = 0; j < APAC_MAXNUM_HYIF; j++) {
                        if (pIF[j].ifIndex != -1) {
                            if (apacHyfi20SendL2Packet(&pIF[j], frame, frameLen2) < 0) {
                                perror("apacHyfi20SendRenewalR");
                            }
                            dprintf(MSG_INFO, "%s - Sent unicast msg mid: %d on IF%d for freq: %d\n", __func__, mid,
                                    j, i);
                        }
                    }
                }
                alid_list.max_renew_retry = pConfig->cfg_max_renew_retry;
                eloop_register_timeout(pConfig->cfg_renew_retransmit_interval, 0,
                         apacHyfi20RenewTimeoutHandler, pData, NULL);
            }
            else {
                dprintf(MSG_ERROR, "%s: Failed to get stored alid entries. Not sending unicast renews\n", __func__);
            }
        }
        else {
            dprintf(MSG_INFO, "%s: Not configured to send unicast renews\n", __func__);
        }
#endif

        /* Send only 1 renew Msg */
#if MAP_ENABLED
        if (apacHyfiMapIsEnabled(HYFI20ToMAP(pData))) {
            if (apacHyfiMapPfComplianceEnabled(HYFI20ToMAP(pData))) {
                FILE *fp = NULL;
                fp = fopen("/tmp/map_auto_renew_mid.tmp", "w+");
                if (fp == NULL) {
                    return -1;
                }
                fprintf(fp, "0x%x", mid);
                fclose(fp);
                fp = fopen("/tmp/map_auto_renew_mid_5g.tmp", "w+");
                if (fp == NULL) {
                    return -1;
                }
                fprintf(fp, "0x%x", mid);
                fclose(fp);
            }
            break;
        }
#endif
    }

    return 0;
}

apacBool_e apacHyfiParse1905TLV(struct apac_wps_session *sess, u8 *content, u32 contentLen,
                                   struct wps_tlv *container, u8 *ieParsed) {
    ieee1905TLV_t *TLV = (ieee1905TLV_t *)content;
    u_int32_t accumulatedLen = 0;
    ieee1905TlvType_e tlvType;
    apacBool_e retv = APAC_TRUE;
    u32 index = 0;
    struct wps_tlv *wps = NULL;
    u8 *ptr = NULL;

    accumulatedLen = ieee1905TLVLenGet(TLV) + IEEE1905_TLV_MIN_LEN;

    while (accumulatedLen <= contentLen) {
        tlvType = ieee1905TLVTypeGet(TLV);
        dprintf(MSG_DEBUG, "%s, TLV Type %d\n", __func__, tlvType);
        wps = &container[index];
        if (tlvType == IEEE1905_TLV_TYPE_END_OF_MESSAGE) {
            dprintf(MSG_ERROR, "%s, Should not get EOM here.\n", __func__);
        } else if (tlvType == IEEE1905_TLV_TYPE_WPS) {
            wps->type = IEEE1905_TLV_TYPE_WPS;
            wps->length = ieee1905TLVLenGet(TLV);
            if (wps->length)
                wps->value.ptr_ = os_malloc(wps->length);
            else {
                retv = APAC_FALSE;
                break;
            }

            if (!wps->value.ptr_) {
                retv = APAC_FALSE;
                break;
            }
            ptr = ieee1905TLVValGet(TLV);
            os_memcpy(wps->value.ptr_, ptr, wps->length);
            index++;
        } else if (tlvType == IEEE1905_TLV_TYPE_RADIO_IDENTIFIER) {
            sess->radioCap = malloc(ieee1905TLVLenGet(TLV));
            if (sess->radioCap) {
                sess->radioCapLen = ieee1905TLVLenGet(TLV);
                ptr = ieee1905TLVValGet(TLV);
                memcpy(sess->radioCap, ptr, sess->radioCapLen);
            }
        } else if (tlvType == IEEE1905_TLV_TYPE_AP_RADIO_BASIC_CAP) {
            sess->basicRadioCap = malloc(ieee1905TLVLenGet(TLV));
            if (sess->basicRadioCap) {
                ptr = ieee1905TLVValGet(TLV);
                sess->basicRadioCapLen = ieee1905TLVLenGet(TLV);
                memcpy(sess->basicRadioCap, ptr, sess->basicRadioCapLen);
            }
        }
#if MAP_ENABLED
        else if (tlvType == IEEE1905_TLV_TYPE_8021Q_RULES) {
            sess->traffic8021QPolicy = malloc(ieee1905TLVLenGet(TLV));
            if (sess->traffic8021QPolicy) {
                sess->traffic8021QPolicyLen = ieee1905TLVLenGet(TLV);
                ptr = ieee1905TLVValGet(TLV);
                memcpy(sess->traffic8021QPolicy, ptr, sess->traffic8021QPolicyLen);
            }
        } else if (tlvType == IEEE1905_TLV_TRAFFIC_SEPARATON_POLICY) {
            sess->trafficSepPolicy = malloc(ieee1905TLVLenGet(TLV));
            if (sess->trafficSepPolicy) {
                sess->trafficSepLen = ieee1905TLVLenGet(TLV);
                ptr = ieee1905TLVValGet(TLV);
                memcpy(sess->trafficSepPolicy, ptr, sess->trafficSepLen);
            }
        } else if (tlvType == IEEE1905_TLV_TYPE_AGENT_AP_MLD_CONFIG) {
            sess->agentApMldConfig = malloc(ieee1905TLVLenGet(TLV));
            if (sess->agentApMldConfig) {
                sess->agentApMldConfigLen = ieee1905TLVLenGet(TLV);
                ptr = ieee1905TLVValGet(TLV);
                memcpy(sess->agentApMldConfig, ptr, sess->agentApMldConfigLen);
            }
        }
        else if (tlvType == IEEE1905_TLV_TYPE_BSTA_MLD_CONFIG) {
            sess->bstaMldConfig = malloc(ieee1905TLVLenGet(TLV));
            if (sess->bstaMldConfig) {
                sess->bstaMldConfigLen = ieee1905TLVLenGet(TLV);
                ptr = ieee1905TLVValGet(TLV);
                memcpy(sess->bstaMldConfig, ptr, sess->bstaMldConfigLen);
            }
        }
#endif

#ifdef QCA_EXT_EM_CONTROLLER
        else if (tlvType == IEEE1905_TLV_TYPE_VENDOR_SPECIFIC) {
            unsigned int conLen = ieee1905TLVLenGet(TLV);
            unsigned char *emex_tlv = ieee1905TLVValGet(TLV);
            if (!(cbs.emex_vendor_message(&emex_tlv, &conLen))) {
                emex_vendor_autoconfig_wsc(emex_tlv, conLen);
            }
        }
#endif
        TLV = ieee1905TLVGetNext(TLV);
        accumulatedLen += ieee1905TLVLenGet(TLV) + IEEE1905_TLV_MIN_LEN;
    }

    *ieParsed = index;

    dprintf(MSG_MSGDUMP, "%s ToTal Frame received  %d \n",__func__, *ieParsed);

    return retv;
}

/* Registrar/Enrollee sends WPS msg */
int apacHyfiSendMultipleM2(struct apac_wps_session *sess)
{
    u8 *frame;
    ieee1905MessageType_e msgType = IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_WPS;
    u16 mid = apacHyfi20GetMid();
    u8 flags = 0;
    u8 *src;
    u8 *dest;
    APAC_WPS_DATA    *pWpsData = sess->pWpsData;
    struct wps_tlv *tlvList = &pWpsData->sndMsgM2[0];
    u8 fragNum = 0;
    u_int32_t frameLen = 0;
    ieee1905TLV_t *TLV;
    size_t sentLen = 0;
    u8  more = 0;
    u8 index = 0;
#if MAP_ENABLED
    apacBool_e radioTlvSent = APAC_FALSE;
    apacHyfi20Data_t *pData = sess->pData;
    apacMapData_t *mapData = HYFI20ToMAP(pData);
#endif
    apacHyfi20TRACE();
    src = sess->own_addr;
    dest = sess->dest_addr;

    if (!src || !dest) {
        dprintf(MSG_ERROR, "src or dest address is null!\n");
        dprintf(MSG_ERROR, " src: "); printMac(MSG_DEBUG, src);
        dprintf(MSG_ERROR, " dest: "); printMac(MSG_DEBUG, dest);

        return -1;
    }

nextIndex:
    frame = apacHyfi20GetXmitBuf();
    frameLen = IEEE1905_FRAME_MIN_LEN;
    TLV = (ieee1905TLV_t *)((ieee1905Message_t *)frame)->content;
#if MAP_ENABLED
    if(radioTlvSent == APAC_FALSE) {
        TLV = ieee1905MapAddRadioIdTLV(TLV, &frameLen, sess);

        if (apacHyfiMapIsEnabled(HYFI20ToMAP(pData)) >= APAC_MAP_VERSION_2) {
            // Is r1 Agent preset in NW dont include Traffic Separation Policy in WSC message
            // It will be added in Policy Config message in HYD
            dprintf(MSG_INFO, "R1 Agent in Network %d \n", mapData->r1AgentInNw);
            if (mapData->r1AgentInNw == APAC_FALSE) {
                if (sess->mapVersion >= APAC_MAP_VERSION_2) {
                    TLV = ieee1905MapAddTrafficSeparationPolicyTLV(TLV, &frameLen, sess);
                }
            }
        }

        if (apacHyfiMapIsMLOEnabled(HYFI20ToMAP(pData)) &&
            apacHyfiMapIsEnabled(HYFI20ToMAP(pData)) >= APAC_MAP_VERSION_6 &&
            apacHyfiMapEarlyAPCapabilitySupported(HYFI20ToMAP(pData)) &&
            sess->mapVersion >= APAC_MAP_VERSION_6 && sess->rxdEarlyAPCapability == 1 &&
            sess->mloSupport == 1) {
            dprintf(MSG_INFO, "Adding MLO Information\n");
            TLV = ieee1905MapAddMLOPolicyTLV(TLV, &frameLen, sess);
        }

        radioTlvSent = APAC_TRUE;
    }
#endif
    if (tlvList[index].length >= IEEE1905_CONTENT_MAXLEN) {
        return -1;
    }

    sentLen = frameLen - IEEE1905_FRAME_MIN_LEN;
    while(index < pWpsData->M2TlvCnt)
    {
        // To make sure TLV boundry fragmentation; min 1 for EOM , one for TLV itself
        if (sentLen + tlvList[index].length < IEEE1905_CONTENT_MAXLEN - IEEE1905_TLV_MIN_LEN) {
            ieee1905TLVSet(TLV, tlvList[index].type, tlvList[index].length,
                           tlvList[index].value.ptr_, frameLen);
            // Adding TLV header length for checking boundary condition
            sentLen += tlvList[index].length + IEEE1905_TLV_MIN_LEN;

            dprintf(MSG_INFO, "%s sent len %d Type %x \n", __func__, (s32)sentLen,
                    tlvList[index].type);
            index++;
        } else {
            more = 1;
            break;
        }
        TLV = ieee1905TLVGetNext(TLV);
    }

    if (!more) {
        flags |= IEEE1905_HEADER_FLAG_LAST_FRAGMENT;
        ieee1905EndOfTLVSet(TLV);
    }
#if MAP_ENABLED
    else {
        /// for R2 onwards we should not end  fragment with EOM
        if (apacHyfiMapIsEnabled(HYFI20ToMAP(pData)) >= APAC_MAP_VERSION_2) {
            frameLen -= IEEE1905_TLV_MIN_LEN;
        }
    }
#endif
    /* set up packet header */
    apacHyfi20SetPktHeader(frame, msgType, mid, fragNum, flags, src, dest);
    /* send packet */

    dprintf(MSG_INFO," %s Sending frame Length %d flags %x Mid %d fid %d Index %d \n",__func__, (s32)frameLen,
            flags, mid,fragNum, index);

    if (sess->pData->config.sendOnAllIFs == APAC_FALSE) {
        if (send(sess->pData->bridge.sock, frame, frameLen, 0) < 0) {
            perror("apacHyfi20SendWps");
            return -1;
        }
    } else {  /* send unicast packet on all interfaces. Debug Only! */
        int i;

        for (i = 0; i < APAC_MAXNUM_HYIF; i++) {
            if (sess->pData->hyif[i].ifIndex == -1)
                continue;
#if MAP_ENABLED
            if (apacHyfiMapIsEnabled(HYFI20ToMAP(sess->pData))) {
                if (send(sess->pData->br_guest_list[0].sock, frame, frameLen, 0) < 0) {
                    perror("apacHyfi20SendWps-onAllIFs");
                }
            }
#endif
            if (apacHyfi20SendL2Packet(&sess->pData->hyif[i], frame, frameLen) < 0) {
                perror("apacHyfi20SendWps-onAllIFs");
            }
        }
    }

    fragNum++;

    if (more) {
        more = 0; //for next iteration
        sentLen = 0;
        frameLen = 0;
        flags = 0;
        goto nextIndex;
    }

#if MAP_ENABLED
    // store the ALID of agent
    apac_alid destAlid = {0};
    memcpy(destAlid.addr, dest, ETH_ALEN);
    apac_map_alid_db_store_alid(&destAlid);
#endif
    return 0;
}

/* Registrar/Enrollee sends WPS msg */
int apacHyfi20SendWps(struct apac_wps_session *sess, u8 *wpsMsg, size_t wpsMsgLen) {
    // APAC_WPS_DATA *pWpsData = sess->pWpsData;

    u8 *frame;
    ieee1905MessageType_e msgType = IEEE1905_MSG_TYPE_AP_AUTOCONFIGURATION_WPS;
    u16 mid = apacHyfi20GetMid();
#if MAP_ENABLED
    apacHyfi20Data_t *pData = sess->pData;
#endif
    u8 flags;
    u8 *src;
    u8 *dest;

    u8 fragNum;
    u32 frameLen;
    u8 *curWpsMsg = wpsMsg;
    size_t curWpsMsgLen = wpsMsgLen;
    ieee1905TLV_t *TLV;
#ifdef QCA_EXT_EM_CONTROLLER
    unsigned char *payload=NULL;
    unsigned short payload_len;
#endif

    /* Overhead: WPS TLV + EndTLV */
    size_t maxWpsMsgLen = IEEE1905_CONTENT_MAXLEN - 2 * IEEE1905_TLV_MIN_LEN;

    apacHyfi20TRACE();
    src = sess->own_addr;
    dest = sess->dest_addr;

    if (!src || !dest) {
        return -1;
    }

    /* construct WPS packet */
    flags = 0;
    fragNum = 0;
    while (curWpsMsgLen > 0) {
        frame = apacHyfi20GetXmitBuf();
        frameLen = IEEE1905_FRAME_MIN_LEN;
        TLV = (ieee1905TLV_t *)((ieee1905Message_t *)frame)->content;

        /* last frame */
        if (curWpsMsgLen <= maxWpsMsgLen) {
            flags |= IEEE1905_HEADER_FLAG_LAST_FRAGMENT;
            /* Add WPS TLV */
            ieee1905TLVSet(TLV, IEEE1905_TLV_TYPE_WPS, curWpsMsgLen, curWpsMsg, frameLen);
            curWpsMsg += curWpsMsgLen;
            curWpsMsgLen = 0;
        } else {
            ieee1905TLVSet(TLV, IEEE1905_TLV_TYPE_WPS, maxWpsMsgLen, curWpsMsg, frameLen);
            curWpsMsgLen -= maxWpsMsgLen;
            curWpsMsg += maxWpsMsgLen;
        }

#if MAP_ENABLED
        if (apacHyfiMapIsEnabled(HYFI20ToMAP(pData))) {
            if (flags & IEEE1905_HEADER_FLAG_LAST_FRAGMENT) {
                if (pData->config.role == APAC_ENROLLEE && sess->state == APAC_WPS_E_M1_SENT) {
                    TLV = ieee1905MapAddBasicRadioTLV(TLV, &frameLen, apacS.bandInRegistration,
                                                      pData);

                    /* MLO Cap TLV*/
                    if (apacHyfiMapIsMLOEnabled(HYFI20ToMAP(pData)) &&
                        apacHyfiMapIsEnabled(HYFI20ToMAP(pData)) < APAC_MAP_VERSION_6) {
                        TLV = ieee1905MapAddMLOVendorTLV(TLV, &frameLen, apacS.bandInRegistration,
                                                          pData);
                    }

                    /* MAPv2 TLVs */
                    if (apacHyfiMapIsEnabled(HYFI20ToMAP(pData)) >= APAC_MAP_VERSION_2) {
                        TLV = ieee1905Map2AddApCapTLV(TLV, &frameLen, apacS.bandInRegistration,
                                                      pData);
                        TLV = ieee1905Map2AddApRadioAdvancedCapTLV(TLV, &frameLen,
                                                                   apacS.bandInRegistration, pData);
#ifdef QCA_EXT_EM_CONTROLLER
                        if (!ieee1905EMEXTypeInsert(TLV_1905_VENDOR_SPECIFIC, EMEX_TLV_FEATURE_PROFILE, NULL, 0, &payload, &payload_len)){
                            if(payload){
                               dprintf(MSG_INFO, "%s: [EMEX] inserting tlv \n", __func__);
                               TLV = ieee1905TLVGetNext(TLV);
                               ieee1905TLVTypeSet(TLV, TLV_1905_VENDOR_SPECIFIC);
                               ieee1905TLVLenSet(TLV, payload_len, frameLen);
                               ieee1905TLVValSet(TLV,payload,payload_len);
                               free(payload);
                               payload = NULL;
                            }
                        }
#endif
                    }

                    TLV = ieee1905AddFeatureFlagTLV(TLV, &frameLen, pData);
                    /* add feature Flag TLV in M1*/
                }
            }
        }
#endif

#ifdef QCA_EXT_EM_CONTROLLER
        if (!ieee1905EMEXTypeInsert(TLV_1905_VENDOR_SPECIFIC, EMEX_TLV_DEVICE_INFO, NULL, 0, &payload, &payload_len)){
            if(payload){
               dprintf(MSG_INFO, "%s: [EMEX] inserting tlv \n", __func__);
               TLV = ieee1905TLVGetNext(TLV);
               ieee1905TLVTypeSet(TLV, TLV_1905_VENDOR_SPECIFIC);
               ieee1905TLVLenSet(TLV, payload_len, frameLen);
               ieee1905TLVValSet(TLV,payload,payload_len);
               free(payload);
               payload = NULL;
            }
        }
#endif
        /* Add EndOfTlv */
        TLV = ieee1905TLVGetNext(TLV);
        ieee1905EndOfTLVSet(TLV);

        /* set up packet header */
        apacHyfi20SetPktHeader(frame, msgType, mid, fragNum, flags, src, dest);

        /* send packet */
        if (sess->pData->config.sendOnAllIFs == APAC_FALSE) {
#if MAP_ENABLED
            //send packet to lo interface if dest mac is controller
            dprintf(MSG_MSGDUMP, "%s: apacHyfiMapIsUseLoInterface:%d compare:%d",__func__, apacHyfiMapIsUseLoInterface(HYFI20ToMAP(pData)), !memcmp(apacHyfiGetControllerAlid(HYFI20ToMAP(pData)), dest, ETHER_ADDR_LEN));
            if (apacHyfiMapIsUseLoInterface(HYFI20ToMAP(pData)) &&
                 !memcmp(apacHyfiGetControllerAlid(HYFI20ToMAP(pData)), dest, ETHER_ADDR_LEN))
            {
                int j;
                for (j = 0; j < APAC_MAXNUM_HYIF; j++) {
                    if (sess->pData->hyif[j].ifIndex == -1 || strcmp(sess->pData->hyif[j].ifName, "lo")) continue;
                    dprintf(MSG_MSGDUMP, "%s: send packet on :%s",__func__,sess->pData->hyif[j].ifName);
                    if (apacHyfi20SendL2Packet(&sess->pData->hyif[j], frame, frameLen) < 0) {
                        perror("apacHyfi20SendWps-onLoIF");
                    }
                }
            } else
#endif
           if (send(sess->pData->bridge.sock, frame, frameLen, 0) < 0) {
                 perror("apacHyfi20SendWps");
                 return -1;
            }
        } else { /* send unicast packet on all interfaces. Debug Only! */

#if MAP_ENABLED
            //send packet to lo interface if dest mac is controller
            if (apacHyfiMapIsUseLoInterface(HYFI20ToMAP(pData)) &&
                 !memcmp(apacHyfiGetControllerAlid(HYFI20ToMAP(pData)), dest, ETHER_ADDR_LEN))
            {
                int j;
                for (j = 0; j < APAC_MAXNUM_HYIF; j++) {
                    if (sess->pData->hyif[j].ifIndex == -1 || strcmp(sess->pData->hyif[j].ifName, "lo")) continue;
                    dprintf(MSG_MSGDUMP, "%s: send packet on :%s",__func__,sess->pData->hyif[j].ifName);
                    if (apacHyfi20SendL2Packet(&sess->pData->hyif[j], frame, frameLen) < 0) {
                        perror("apacHyfi20SendWps-onLoIF");
                    }
                }
            } else
            /* Send packet on bridge if Traffic Separation is Enabled */
            if (apacHyfiMapIsTrafficSeparationEnabled(HYFI20ToMAP(pData))) {
                if (send(pData->bridge.sock, frame, frameLen, 0) < 0) {
                    perror("apacHyfi20SendWps-onAllIFs");
                }
            }
#endif

            int i;
            for (i = 0; i < APAC_MAXNUM_HYIF; i++) {
                if (sess->pData->hyif[i].ifIndex == -1) continue;

                if (apacHyfi20SendL2Packet(&sess->pData->hyif[i], frame, frameLen) < 0) {
                    perror("apacHyfi20SendWps-onAllIFs");
                }
            }
        }

        dprintf(MSG_DEBUG, "%s len of msg sent: %u, mid: %u\n", __func__,(u32)frameLen, mid);
        fragNum++;
    }

    return 0;
}

/* Process WPS request in Registration Phase
 * Registrar receives M1, M3, M5, and M7; sends M2, M4, M6 and M8
 * Enrollee sends M1, M3, M5, and M7; receives M2, M4, M6 and M8
 */
int apacHyfi20ReceiveWps(apacHyfi20Data_t *pData, u8 *buf, u32 len) {
#if 0
    srand((unsigned int) time(NULL));
    if (random() % 3 == 0)
    {
        /*drop packets*/
        dprintf(MSG_ERROR, "1905.1 AP Auto Configuration Packet dropped for testing\n");
        return 0;
    }
#endif

    if (pData->config.role == APAC_ENROLLEE) {
        return apacHyfi20ReceiveWpsE(pData, buf, len);
    }
    else if (pData->config.role == APAC_REGISTRAR) {
        return apacHyfi20ReceiveWpsR(pData, buf, len);
    }
    else {
        dprintf(MSG_ERROR, "%s Device role (%d) not supported!\n", __func__, pData->config.role);
        return -1;
    }
}

int apacHyfi20Startup(apacHyfi20Data_t *ptrData) {
    apacHyfi20Config_t *pConfig = &ptrData->config;

    /* XXX pushbutton enabled: only SM. APAC with PB feature is not supported (3/5/2012) */
    if (pConfig->pbmode_enabled == APAC_TRUE) {
        //XXX register PBActivatedCB when pushbutton signal is received

        /* Wait for push button signal */
        if (pConfig->role == APAC_REGISTRAR) {
            pConfig->state = APAC_R_PB_IDLE;
        }
        else {
            pConfig->state = APAC_E_PB_IDLE;
        }
        return 0;
    }

    /* pushbutton disabled */
    if (pConfig->role == APAC_REGISTRAR) {
        if ((pConfig->cfg_changed) && (pConfig->role == APAC_REGISTRAR)) {
            /* Start restart timer */
            eloop_register_timeout(apac_cfg_restart_long_interval, 0,
                            apacHyfi20ConfigRestartTimeoutHandler, ptrData, NULL);
        }
        apacHyfi20SendRenewalR(ptrData);
        pConfig->state = APAC_R_NO_PB;
        /* register channel polling timer */
        apacHyfi20ChannelPollingHandler(ptrData, NULL);
    }
    else {
        apacHyfi20SendSearchE(ptrData);
        pConfig->state = APAC_E_WAIT_RESP;

        /* register Search timout */
        eloop_register_timeout(pConfig->search_to, 0,
                        apacHyfi20SearchTimeoutHandler, ptrData, NULL);
    }

    if(pConfig->atf_config_enabled == APAC_TRUE) {
        int rep_index = 0;
        for( rep_index = 0; rep_index < pConfig->apac_atf_num_repeaters; rep_index++) {
            dprintf(MSG_INFO, "Send ATF configuration\n\r");
            apacHyfi20SendAtfConfig(ptrData, rep_index);
        }
    }

    return 0;
}

void apacHyfi20ResetState(struct apac_wps_session *sess, apacBool_e success, apacBool_e *bandPendingForConfig) {
    apacHyfi20Config_t *pConfig = &(sess->pData->config);
    apacHyfi20AP_t *pAP = sess->pData->ap;
    int i;

    if (!sess) {
        dprintf(MSG_ERROR, "%s sess is NULL!\n", __func__);
        return;
    }

    if (bandPendingForConfig) {
        *bandPendingForConfig = APAC_FALSE;
    }
#if MAP_ENABLED
    apacHyfi20Data_t *pData = sess->pData;
    apacMapData_t *mapData = HYFI20ToMAP(pData);
#endif

    apacHyfi20TRACE();

    if (sess->timerScheduled) {
        sess->timerScheduled = APAC_FALSE;
        eloop_cancel_timeout(apacHyfi20WpsSessionTimeoutHandler, sess, NULL);
    }

    if (pConfig->role == APAC_ENROLLEE) {
        u32 freq = apacS.bandInRegistration;

        apacS.bandSupportedByReg = 0;

        if (freq == APAC_WIFI_FREQ_INVALID) {
            dprintf(MSG_ERROR, "%s - Invalid Band(%d)!\n", __func__, apacS.bandInRegistration);
            return;
        }

        /* Once APAC is successful, reset apacS band information  */
        if (success) {
            dprintf(MSG_INFO, "band %u is configured\n", freq);
            if (pAP[freq].isAutoConfigured) {
                dprintf(MSG_ERROR, "%s band#%u has been auto-configured before\n", __func__, freq);
            }
            else {
                pAP[freq].isAutoConfigured = APAC_TRUE;
            }
            apacS.bandInRegistration = APAC_WIFI_FREQ_INVALID;
            pConfig->state = APAC_E_IDLE;

            /* Enrollee, check if needs to resume search timeout */
            for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
                if (!(pAP[i].isAutoConfigured) && pAP[i].valid) {
                    /* do not send search right away, give some time */
                    /* register search timeout callback */
                    dprintf(MSG_INFO, "%s, band %u not configured. Send search in %u seconds\n",
                            __func__, i, APAC_SEARCH_SHORT_INTERVAL);

                    /* Reset wait timer to make sure the other band has enough time to */
                    /* complete cloning */
                    sess->pData->wifiConfigWaitSecs = 0;
                    eloop_register_timeout(APAC_SEARCH_SHORT_INTERVAL, 0,
                                           apacHyfi20SearchTimeoutHandler,
                                           sess->pData, (void *)&APAC_SEARCH_SHORT_INTERVAL);
                    /* infrom the caller that, there are bands pending for configuration */
                    if (bandPendingForConfig) {
                        *bandPendingForConfig = APAC_TRUE;
                    }
                    return;
                }
            }
            /* All bands are configured */
            eloop_cancel_timeout(apacHyfi20SearchTimeoutHandler, sess->pData,
                                 (void *)&APAC_SEARCH_SHORT_INTERVAL);
            dprintf(MSG_INFO, "All bands configuration received, apply\n");
            // Send "Config Ack Message" here and start a Apply timer
            apacHyfi20SendConfigAckE(sess);

            dprintf(MSG_INFO, "Starting config apply timer %s\n", __func__);
#if MAP_ENABLED
            pConfig->configApplyTimerStarted = APAC_TRUE;
            if (mapData->fastOnboardEnabled) {
                apac_cfg_apply_interval = 1;
            }
#endif
            eloop_register_timeout(apac_cfg_apply_interval, 0,
                                   apacHyfi20ConfigApplyTimeoutHandler,
                                   sess->pData, NULL);

            return;
        }
        /* APAC is not successful, resend Search */
        apacS.bandInRegistration = APAC_WIFI_FREQ_INVALID;
        apacHyfi20SendSearchE(sess->pData);
        pConfig->state = APAC_E_WAIT_RESP;
#if MAP_ENABLED
        /* Clear out all AutoConfigured flags when we are resetting the states.
           This is required only for MAP where we don't support applying partial
           configuration (ie config not received for all radios/bands), incase
           of SON, partical config apply is still supported, so we don't need to
           send search messages for band this already configured */
        dprintf(MSG_INFO, "%s: Autoconfiguration failed, resetting states for all bands\n", __func__);
        for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {
            pData->ap[i].isAutoConfigured = APAC_FALSE;
        }
#endif
        eloop_register_timeout(pConfig->search_to, 0, apacHyfi20SearchTimeoutHandler, sess->pData, NULL);
        return;
    } else if (pConfig->role == APAC_REGISTRAR) {
        pConfig->state = (pConfig->pbmode_enabled ? APAC_R_PB_IDLE : APAC_R_NO_PB);
        dprintf(MSG_ERROR, "%s, %d pConfig->state = %d",__func__, __LINE__,  pConfig->state);
    } else {
        dprintf(MSG_INFO, "%s, device is neither Registrar nor Enrollee: %u\n", __func__, pConfig->role);
    }

}

#ifdef QCA_EXT_EM_CONTROLLER
int ieee1905EMEXTypeInsert(unsigned char type, unsigned short emex_type, void *args, unsigned short args_len, unsigned char **payload, unsigned short *payload_len)
{
    int rc = -1;
    *payload_len = 0;
    unsigned char *payload_tmp = NULL;

    if (cbs.emex_prepare_tlv) {
        if  (args_len > EMEX_VENDOR_TLV_MAX_LEN)
        {
             dprintf(MSG_ERROR, "[EMEX] args_len is greater than maximum TLV length for TLV %d!\n", emex_type);
             goto out;
        }

        payload_tmp = calloc(1, EMEX_VENDOR_TLV_MAX_LEN);

        if (payload_tmp == NULL) {
            dprintf(MSG_DEBUG, "[EMEX] Error on malloc!\n");
            goto out;
        }
        if (args && args_len > 0) {
            /* Embed parameters to be passed into payload buffer if exist. */
            memcpy(payload_tmp, args, args_len);
        }
        else if (args && args_len == 0) {
            dprintf(MSG_DEBUG, "[EMEX] args with a len zero\n");
            free(payload_tmp);
            payload_tmp = NULL;
            goto out;
        }
        if (cbs.emex_prepare_tlv(type, emex_type, &payload_tmp, payload_len)) {
            dprintf(MSG_DEBUG, "[EMEX] Incoming TLV is NOT valid!\n");
            free(payload_tmp);
            payload_tmp = NULL;
            goto out;
        }
        /* Silently discard payload with 0 length (no TLV needs to be added) */
        if (!(*payload_len)) {
            free(payload_tmp);
            payload_tmp = NULL;
            goto out;
        }
        *payload = payload_tmp;
        rc = 0;
    } else {
        dprintf(MSG_DEBUG, "[EMEX] Callback is not registered.\n");
    }
out:
    return rc;
}

int TlvSupportedServiceTypeExtract(u8 *pvalue, u8 *supportedService, u32 length)
{
    int rc = 0;
    unsigned char  i;
    unsigned int pos = 0;
    unsigned char num_supported_services;

    if (length <= 0) {
       rc = -1;
       goto end;
    }

    num_supported_services = pvalue[pos];
    pos++;

    for (i = 0; i < num_supported_services; i++) {
         if (length >= pos + 1) {
             if (pvalue[pos] == APAC_MAP_CONTROLLER) {
                 (*supportedService) |= CONFIG_FLAG_CONTROLLER;
                 pos++;
             } else if (pvalue[pos] == APAC_MAP_AGENT) {
                 (*supportedService) |= CONFIG_FLAG_AGENT;
                 pos++;
             } else if (pvalue[pos] == MultiAPEmexController) {
                 (*supportedService) |= CONFIG_FLAG_EMEX_CONTROLLER;
                 pos++;
             } else if (pvalue[pos] == MultiAPEmexAgent) {
                 (*supportedService) |= CONFIG_FLAG_EMEX_AGENT;
                 pos++;
             }
         }
    }
end:
  return rc;
}
#endif
