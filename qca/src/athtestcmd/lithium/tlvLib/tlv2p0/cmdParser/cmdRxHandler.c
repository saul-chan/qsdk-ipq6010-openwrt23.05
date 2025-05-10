/* Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input\cmdRxHandler.s
#include "tlv2Inc.h"
#include "cmdRxHandler.h"

void* initRXOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    int i, j; 	//for initializing array parameter
    CMD_RX_PARMS  *pRXParms = (CMD_RX_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);

    i = j = 0;	//assign a number to avoid warning in case i and j are not used

    // Populate the parm structure with initial values
    pRXParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pRXParms->rxMode = pParmDict[PARM_RXMODE].v.valU8;
    pRXParms->freq = pParmDict[PARM_FREQ].v.valU16;
    pRXParms->freq2 = pParmDict[PARM_FREQ2].v.valU16;
    pRXParms->chainMask = pParmDict[PARM_CHAINMASK].v.valU16;
    pRXParms->wlanMode = pParmDict[PARM_WLANMODE].v.valU8;
    pRXParms->bandwidth = pParmDict[PARM_BANDWIDTH].v.valU8;
    pRXParms->antenna = pParmDict[PARM_ANTENNA].v.valU8;
    pRXParms->enANI = pParmDict[PARM_ENANI].v.valU8;
    pRXParms->ack = pParmDict[PARM_ACK].v.valU8;
    pRXParms->broadcast = pParmDict[PARM_BROADCAST].v.valU8;
    pRXParms->lpl = pParmDict[PARM_LPL].v.valU8;
    pRXParms->antswitch1 = pParmDict[PARM_ANTSWITCH1].v.valU8;
    pRXParms->antswitch2 = pParmDict[PARM_ANTSWITCH2].v.valU8;
    pRXParms->synth = pParmDict[PARM_SYNTH].v.valU8;
    pRXParms->spectralScan = pParmDict[PARM_SPECTRALSCAN].v.valU16;
    pRXParms->noiseFloor = pParmDict[PARM_NOISEFLOOR].v.valS16;
    for (i = 0; i < 2 ; i++)
    {
        pRXParms->regDmn[i] = pParmDict[PARM_REGDMN].v.ptU16[i];
    }
    pRXParms->expectedPkts = pParmDict[PARM_EXPECTEDPKTS].v.valU32;
    pRXParms->otpWriteFlag = pParmDict[PARM_OTPWRITEFLAG].v.valU32;
    pRXParms->flags = pParmDict[PARM_FLAGS].v.valU32;
    for (i = 0; i < 3 ; i++)
    {
        pRXParms->rateMask[i] = pParmDict[PARM_RATEMASK].v.ptU32[i];
    }
    memset(pRXParms->rateMask11AC, 0, sizeof(pRXParms->rateMask11AC));
    pRXParms->rateMaskAC160 = pParmDict[PARM_RATEMASKAC160].v.valU32;
    for (i = 0; i < 6 ; i++)
    {
        pRXParms->bssid[i] = pParmDict[PARM_BSSID].v.ptU8[i];
    }
    for (i = 0; i < 6 ; i++)
    {
        pRXParms->staAddr[i] = pParmDict[PARM_STAADDR].v.ptU8[i];
    }
    for (i = 0; i < 6 ; i++)
    {
        pRXParms->btAddr[i] = pParmDict[PARM_BTADDR].v.ptU8[i];
    }
    pRXParms->wifiStandard = pParmDict[PARM_WIFISTANDARD].v.valU32;
    pRXParms->NSS = pParmDict[PARM_NSS].v.valU32;
    pRXParms->rate = pParmDict[PARM_RATE].v.valU8;
    pRXParms->ctrlFlag = pParmDict[PARM_CTRLFLAG].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pRXParms->phyId)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXMODE, (size_t)(((size_t)&(pRXParms->rxMode)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ, (size_t)(((size_t)&(pRXParms->freq)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ2, (size_t)(((size_t)&(pRXParms->freq2)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINMASK, (size_t)(((size_t)&(pRXParms->chainMask)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_WLANMODE, (size_t)(((size_t)&(pRXParms->wlanMode)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BANDWIDTH, (size_t)(((size_t)&(pRXParms->bandwidth)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ANTENNA, (size_t)(((size_t)&(pRXParms->antenna)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENANI, (size_t)(((size_t)&(pRXParms->enANI)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ACK, (size_t)(((size_t)&(pRXParms->ack)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BROADCAST, (size_t)(((size_t)&(pRXParms->broadcast)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LPL, (size_t)(((size_t)&(pRXParms->lpl)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ANTSWITCH1, (size_t)(((size_t)&(pRXParms->antswitch1)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ANTSWITCH2, (size_t)(((size_t)&(pRXParms->antswitch2)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SYNTH, (size_t)(((size_t)&(pRXParms->synth)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SPECTRALSCAN, (size_t)(((size_t)&(pRXParms->spectralScan)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NOISEFLOOR, (size_t)(((size_t)&(pRXParms->noiseFloor)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_REGDMN, (size_t)(((size_t)&(pRXParms->regDmn)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_EXPECTEDPKTS, (size_t)(((size_t)&(pRXParms->expectedPkts)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_OTPWRITEFLAG, (size_t)(((size_t)&(pRXParms->otpWriteFlag)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FLAGS, (size_t)(((size_t)&(pRXParms->flags)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASK, (size_t)(((size_t)&(pRXParms->rateMask)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASK11AC, (size_t)(((size_t)&(pRXParms->rateMask11AC)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASKAC160, (size_t)(((size_t)&(pRXParms->rateMaskAC160)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BSSID, (size_t)(((size_t)&(pRXParms->bssid)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STAADDR, (size_t)(((size_t)&(pRXParms->staAddr)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BTADDR, (size_t)(((size_t)&(pRXParms->btAddr)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_WIFISTANDARD, (size_t)(((size_t)&(pRXParms->wifiStandard)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NSS, (size_t)(((size_t)&(pRXParms->NSS)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATE, (size_t)(((size_t)&(pRXParms->rate)) - (size_t)pRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CTRLFLAG, (size_t)(((size_t)&(pRXParms->ctrlFlag)) - (size_t)pRXParms), pParmsOffset);
    return((void*) pRXParms);
}

static RX_OP_FUNC RXOpFunc = NULL;

TLV2_API void registerRXHandler(RX_OP_FUNC fp)
{
    RXOpFunc = fp;
}

A_BOOL RXOp(void *pParms)
{
    CMD_RX_PARMS *pRXParms = (CMD_RX_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG
    int i; 	//for initializing array parameter
    i = 0;	//assign a number to avoid warning in case i is not used

    A_PRINTF("RXOp: phyId %u\n", pRXParms->phyId);
    A_PRINTF("RXOp: rxMode %u\n", pRXParms->rxMode);
    A_PRINTF("RXOp: freq %u\n", pRXParms->freq);
    A_PRINTF("RXOp: freq2 %u\n", pRXParms->freq2);
    A_PRINTF("RXOp: chainMask 0x%x\n", pRXParms->chainMask);
    A_PRINTF("RXOp: wlanMode %u\n", pRXParms->wlanMode);
    A_PRINTF("RXOp: bandwidth %u\n", pRXParms->bandwidth);
    A_PRINTF("RXOp: antenna %u\n", pRXParms->antenna);
    A_PRINTF("RXOp: enANI %u\n", pRXParms->enANI);
    A_PRINTF("RXOp: ack %u\n", pRXParms->ack);
    A_PRINTF("RXOp: broadcast %u\n", pRXParms->broadcast);
    A_PRINTF("RXOp: lpl %u\n", pRXParms->lpl);
    A_PRINTF("RXOp: antswitch1 %u\n", pRXParms->antswitch1);
    A_PRINTF("RXOp: antswitch2 %u\n", pRXParms->antswitch2);
    A_PRINTF("RXOp: synth %u\n", pRXParms->synth);
    A_PRINTF("RXOp: spectralScan %u\n", pRXParms->spectralScan);
    A_PRINTF("RXOp: noiseFloor %d\n", pRXParms->noiseFloor);
    for (i = 0; i < 2 ; i++)
    {
        A_PRINTF("RXOp: regDmn 0x%x\n", pRXParms->regDmn[i]);
    }
    A_PRINTF("RXOp: expectedPkts %u\n", pRXParms->expectedPkts);
    A_PRINTF("RXOp: otpWriteFlag 0x%x\n", pRXParms->otpWriteFlag);
    A_PRINTF("RXOp: flags 0x%x\n", pRXParms->flags);
    for (i = 0; i < 3 ; i++)
    {
        A_PRINTF("RXOp: rateMask 0x%x\n", pRXParms->rateMask[i]);
    }
    for (i = 0; i < 5 ; i++)
    {
        A_PRINTF("RXOp: rateMask11AC 0x%x\n", pRXParms->rateMask11AC[i]);
    }
    A_PRINTF("RXOp: rateMaskAC160 0x%x\n", pRXParms->rateMaskAC160);
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("RXOp: bssid 0x%x\n", pRXParms->bssid[i]);
    }
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("RXOp: staAddr 0x%x\n", pRXParms->staAddr[i]);
    }
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("RXOp: btAddr 0x%x\n", pRXParms->btAddr[i]);
    }
    A_PRINTF("RXOp: wifiStandard %u\n", pRXParms->wifiStandard);
    A_PRINTF("RXOp: NSS %u\n", pRXParms->NSS);
    A_PRINTF("RXOp: rate %u\n", pRXParms->rate);
    A_PRINTF("RXOp: ctrlFlag %u\n", pRXParms->ctrlFlag);
#endif //_DEBUG

    if (NULL != RXOpFunc) {
        (*RXOpFunc)(pRXParms);
    }
    return(TRUE);
}

void* initRXRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    int i, j; 	//for initializing array parameter
    CMD_RXRSP_PARMS  *pRXRSPParms = (CMD_RXRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);

    i = j = 0;	//assign a number to avoid warning in case i and j are not used

    // Populate the parm structure with initial values
    pRXRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pRXRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pRXRSPParms->rxMode = pParmDict[PARM_RXMODE].v.valU8;
    pRXRSPParms->pad1 = pParmDict[PARM_PAD1].v.valU8;
    pRXRSPParms->rssiInDBm = pParmDict[PARM_RSSIINDBM].v.valS32;
    pRXRSPParms->totalPackets = pParmDict[PARM_TOTALPACKETS].v.valU32;
    pRXRSPParms->crcPackets = pParmDict[PARM_CRCPACKETS].v.valU32;
    pRXRSPParms->secErrPkt = pParmDict[PARM_SECERRPKT].v.valU32;
    for (i = 0; i < 3 ; i++)
    {
        pRXRSPParms->rateMask[i] = pParmDict[PARM_RATEMASK].v.ptU32[i];
    }
    memset(pRXRSPParms->rateMask11AC, 0, sizeof(pRXRSPParms->rateMask11AC));
    pRXRSPParms->rateMaskAC160 = pParmDict[PARM_RATEMASKAC160].v.valU32;
    memset(pRXRSPParms->rateCnt, 0, sizeof(pRXRSPParms->rateCnt));
    memset(pRXRSPParms->rateCnt11ac, 0, sizeof(pRXRSPParms->rateCnt11ac));
    memset(pRXRSPParms->rateShortG, 0, sizeof(pRXRSPParms->rateShortG));
    memset(pRXRSPParms->rateShortG11ac, 0, sizeof(pRXRSPParms->rateShortG11ac));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pRXRSPParms->phyId)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pRXRSPParms->status)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXMODE, (size_t)(((size_t)&(pRXRSPParms->rxMode)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD1, (size_t)(((size_t)&(pRXRSPParms->pad1)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RSSIINDBM, (size_t)(((size_t)&(pRXRSPParms->rssiInDBm)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TOTALPACKETS, (size_t)(((size_t)&(pRXRSPParms->totalPackets)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CRCPACKETS, (size_t)(((size_t)&(pRXRSPParms->crcPackets)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SECERRPKT, (size_t)(((size_t)&(pRXRSPParms->secErrPkt)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASK, (size_t)(((size_t)&(pRXRSPParms->rateMask)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASK11AC, (size_t)(((size_t)&(pRXRSPParms->rateMask11AC)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASKAC160, (size_t)(((size_t)&(pRXRSPParms->rateMaskAC160)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATECNT, (size_t)(((size_t)&(pRXRSPParms->rateCnt)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATECNT11AC, (size_t)(((size_t)&(pRXRSPParms->rateCnt11ac)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATESHORTG, (size_t)(((size_t)&(pRXRSPParms->rateShortG)) - (size_t)pRXRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATESHORTG11AC, (size_t)(((size_t)&(pRXRSPParms->rateShortG11ac)) - (size_t)pRXRSPParms), pParmsOffset);
    return((void*) pRXRSPParms);
}

static RXRSP_OP_FUNC RXRSPOpFunc = NULL;

TLV2_API void registerRXRSPHandler(RXRSP_OP_FUNC fp)
{
    RXRSPOpFunc = fp;
}

A_BOOL RXRSPOp(void *pParms)
{
    CMD_RXRSP_PARMS *pRXRSPParms = (CMD_RXRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG
    int i; 	//for initializing array parameter
    i = 0;	//assign a number to avoid warning in case i is not used

    A_PRINTF("RXRSPOp: phyId %u\n", pRXRSPParms->phyId);
    A_PRINTF("RXRSPOp: status %u\n", pRXRSPParms->status);
    A_PRINTF("RXRSPOp: rxMode %u\n", pRXRSPParms->rxMode);
    A_PRINTF("RXRSPOp: pad1 0x%x\n", pRXRSPParms->pad1);
    A_PRINTF("RXRSPOp: rssiInDBm %d\n", pRXRSPParms->rssiInDBm);
    A_PRINTF("RXRSPOp: totalPackets %u\n", pRXRSPParms->totalPackets);
    A_PRINTF("RXRSPOp: crcPackets %u\n", pRXRSPParms->crcPackets);
    A_PRINTF("RXRSPOp: secErrPkt %u\n", pRXRSPParms->secErrPkt);
    for (i = 0; i < 3 ; i++)
    {
        A_PRINTF("RXRSPOp: rateMask 0x%x\n", pRXRSPParms->rateMask[i]);
    }
    for (i = 0; i < 5 ; i++)
    {
        A_PRINTF("RXRSPOp: rateMask11AC 0x%x\n", pRXRSPParms->rateMask11AC[i]);
    }
    A_PRINTF("RXRSPOp: rateMaskAC160 0x%x\n", pRXRSPParms->rateMaskAC160);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("RXRSPOp: rateCnt %u\n", pRXRSPParms->rateCnt[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 69 entries
    {
        A_PRINTF("RXRSPOp: rateCnt11ac %u\n", pRXRSPParms->rateCnt11ac[i]);
    }
    for (i = 0; i < 2 ; i++)
    {
        A_PRINTF("RXRSPOp: rateShortG 0x%x\n", pRXRSPParms->rateShortG[i]);
    }
    for (i = 0; i < 5 ; i++)
    {
        A_PRINTF("RXRSPOp: rateShortG11ac 0x%x\n", pRXRSPParms->rateShortG11ac[i]);
    }
#endif //_DEBUG

    if (NULL != RXRSPOpFunc) {
        (*RXRSPOpFunc)(pRXRSPParms);
    }
    return(TRUE);
}
