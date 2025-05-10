/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdTxStatusHandler.s
#include "tlv2Inc.h"
#include "cmdTxStatusHandler.h"

void* initTXSTATUSOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_TXSTATUS_PARMS  *pTXSTATUSParms = (CMD_TXSTATUS_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);

    // Populate the parm structure with initial values
    pTXSTATUSParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pTXSTATUSParms->stopTx = pParmDict[PARM_STOPTX].v.valU8;
    pTXSTATUSParms->needReport = pParmDict[PARM_NEEDREPORT].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pTXSTATUSParms->phyId)) - (size_t)pTXSTATUSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STOPTX, (size_t)(((size_t)&(pTXSTATUSParms->stopTx)) - (size_t)pTXSTATUSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NEEDREPORT, (size_t)(((size_t)&(pTXSTATUSParms->needReport)) - (size_t)pTXSTATUSParms), pParmsOffset);
    return((void*) pTXSTATUSParms);
}

static TXSTATUS_OP_FUNC TXSTATUSOpFunc = NULL;

TLV2_API void registerTXSTATUSHandler(TXSTATUS_OP_FUNC fp)
{
    TXSTATUSOpFunc = fp;
}

A_BOOL TXSTATUSOp(void *pParms)
{
    CMD_TXSTATUS_PARMS *pTXSTATUSParms = (CMD_TXSTATUS_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TXSTATUSOp: phyId %u\n", pTXSTATUSParms->phyId);
    A_PRINTF("TXSTATUSOp: stopTx %u\n", pTXSTATUSParms->stopTx);
    A_PRINTF("TXSTATUSOp: needReport %u\n", pTXSTATUSParms->needReport);
#endif //_DEBUG

    if (NULL != TXSTATUSOpFunc) {
        (*TXSTATUSOpFunc)(pTXSTATUSParms);
    }
    return(TRUE);
}

void* initTXSTATUSRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_TXSTATUSRSP_PARMS  *pTXSTATUSRSPParms = (CMD_TXSTATUSRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);

    // Populate the parm structure with initial values
    pTXSTATUSRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pTXSTATUSRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pTXSTATUSRSPParms->numOfReports = pParmDict[PARM_NUMOFREPORTS].v.valU8;
    pTXSTATUSRSPParms->paCfg = pParmDict[PARM_PACFG].v.valU8;
    pTXSTATUSRSPParms->gainIdx = pParmDict[PARM_GAINIDX].v.valU8;
    pTXSTATUSRSPParms->dacGain = pParmDict[PARM_DACGAIN].v.valS8;
    memset(pTXSTATUSRSPParms->pad2, 0, sizeof(pTXSTATUSRSPParms->pad2));
    memset(pTXSTATUSRSPParms->pdadc, 0, sizeof(pTXSTATUSRSPParms->pdadc));
    memset(pTXSTATUSRSPParms->thermCal, 0, sizeof(pTXSTATUSRSPParms->thermCal));
    pTXSTATUSRSPParms->totalPackets = pParmDict[PARM_TOTALPACKETS].v.valU32;
    pTXSTATUSRSPParms->goodPackets = pParmDict[PARM_GOODPACKETS].v.valU32;
    pTXSTATUSRSPParms->underruns = pParmDict[PARM_UNDERRUNS].v.valU32;
    pTXSTATUSRSPParms->otherError = pParmDict[PARM_OTHERERROR].v.valU32;
    pTXSTATUSRSPParms->excessRetries = pParmDict[PARM_EXCESSRETRIES].v.valU32;
    pTXSTATUSRSPParms->shortRetry = pParmDict[PARM_SHORTRETRY].v.valU32;
    pTXSTATUSRSPParms->longRetry = pParmDict[PARM_LONGRETRY].v.valU32;
    pTXSTATUSRSPParms->startTime = pParmDict[PARM_STARTTIME].v.valU32;
    pTXSTATUSRSPParms->endTime = pParmDict[PARM_ENDTIME].v.valU32;
    pTXSTATUSRSPParms->byteCount = pParmDict[PARM_BYTECOUNT].v.valU32;
    pTXSTATUSRSPParms->dontCount = pParmDict[PARM_DONTCOUNT].v.valU32;
    pTXSTATUSRSPParms->rssi = pParmDict[PARM_RSSI].v.valS32;
    memset(pTXSTATUSRSPParms->rssic, 0, sizeof(pTXSTATUSRSPParms->rssic));
    memset(pTXSTATUSRSPParms->rssie, 0, sizeof(pTXSTATUSRSPParms->rssie));
    pTXSTATUSRSPParms->rateBitIndex = pParmDict[PARM_RATEBITINDEX].v.valU16;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pTXSTATUSRSPParms->phyId)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pTXSTATUSRSPParms->status)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMOFREPORTS, (size_t)(((size_t)&(pTXSTATUSRSPParms->numOfReports)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PACFG, (size_t)(((size_t)&(pTXSTATUSRSPParms->paCfg)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GAINIDX, (size_t)(((size_t)&(pTXSTATUSRSPParms->gainIdx)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DACGAIN, (size_t)(((size_t)&(pTXSTATUSRSPParms->dacGain)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD2, (size_t)(((size_t)&(pTXSTATUSRSPParms->pad2)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PDADC, (size_t)(((size_t)&(pTXSTATUSRSPParms->pdadc)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_THERMCAL, (size_t)(((size_t)&(pTXSTATUSRSPParms->thermCal)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TOTALPACKETS, (size_t)(((size_t)&(pTXSTATUSRSPParms->totalPackets)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GOODPACKETS, (size_t)(((size_t)&(pTXSTATUSRSPParms->goodPackets)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_UNDERRUNS, (size_t)(((size_t)&(pTXSTATUSRSPParms->underruns)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_OTHERERROR, (size_t)(((size_t)&(pTXSTATUSRSPParms->otherError)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_EXCESSRETRIES, (size_t)(((size_t)&(pTXSTATUSRSPParms->excessRetries)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SHORTRETRY, (size_t)(((size_t)&(pTXSTATUSRSPParms->shortRetry)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LONGRETRY, (size_t)(((size_t)&(pTXSTATUSRSPParms->longRetry)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STARTTIME, (size_t)(((size_t)&(pTXSTATUSRSPParms->startTime)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENDTIME, (size_t)(((size_t)&(pTXSTATUSRSPParms->endTime)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BYTECOUNT, (size_t)(((size_t)&(pTXSTATUSRSPParms->byteCount)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DONTCOUNT, (size_t)(((size_t)&(pTXSTATUSRSPParms->dontCount)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RSSI, (size_t)(((size_t)&(pTXSTATUSRSPParms->rssi)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RSSIC, (size_t)(((size_t)&(pTXSTATUSRSPParms->rssic)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RSSIE, (size_t)(((size_t)&(pTXSTATUSRSPParms->rssie)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEBITINDEX, (size_t)(((size_t)&(pTXSTATUSRSPParms->rateBitIndex)) - (size_t)pTXSTATUSRSPParms), pParmsOffset);
    return((void*) pTXSTATUSRSPParms);
}

static TXSTATUSRSP_OP_FUNC TXSTATUSRSPOpFunc = NULL;

TLV2_API void registerTXSTATUSRSPHandler(TXSTATUSRSP_OP_FUNC fp)
{
    TXSTATUSRSPOpFunc = fp;
}

A_BOOL TXSTATUSRSPOp(void *pParms)
{
    CMD_TXSTATUSRSP_PARMS *pTXSTATUSRSPParms = (CMD_TXSTATUSRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TXSTATUSRSPOp: phyId %u\n", pTXSTATUSRSPParms->phyId);
    A_PRINTF("TXSTATUSRSPOp: status %u\n", pTXSTATUSRSPParms->status);
    A_PRINTF("TXSTATUSRSPOp: numOfReports %u\n", pTXSTATUSRSPParms->numOfReports);
    A_PRINTF("TXSTATUSRSPOp: paCfg %u\n", pTXSTATUSRSPParms->paCfg);
    A_PRINTF("TXSTATUSRSPOp: gainIdx %u\n", pTXSTATUSRSPParms->gainIdx);
    A_PRINTF("TXSTATUSRSPOp: dacGain %d\n", pTXSTATUSRSPParms->dacGain);
    for (i = 0; i < 2 ; i++)
    {
        A_PRINTF("TXSTATUSRSPOp: pad2 %u\n", pTXSTATUSRSPParms->pad2[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("TXSTATUSRSPOp: pdadc %u\n", pTXSTATUSRSPParms->pdadc[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("TXSTATUSRSPOp: thermCal %d\n", pTXSTATUSRSPParms->thermCal[i]);
    }
    A_PRINTF("TXSTATUSRSPOp: totalPackets %u\n", pTXSTATUSRSPParms->totalPackets);
    A_PRINTF("TXSTATUSRSPOp: goodPackets %u\n", pTXSTATUSRSPParms->goodPackets);
    A_PRINTF("TXSTATUSRSPOp: underruns %u\n", pTXSTATUSRSPParms->underruns);
    A_PRINTF("TXSTATUSRSPOp: otherError %u\n", pTXSTATUSRSPParms->otherError);
    A_PRINTF("TXSTATUSRSPOp: excessRetries %u\n", pTXSTATUSRSPParms->excessRetries);
    A_PRINTF("TXSTATUSRSPOp: shortRetry %u\n", pTXSTATUSRSPParms->shortRetry);
    A_PRINTF("TXSTATUSRSPOp: longRetry %u\n", pTXSTATUSRSPParms->longRetry);
    A_PRINTF("TXSTATUSRSPOp: startTime %u\n", pTXSTATUSRSPParms->startTime);
    A_PRINTF("TXSTATUSRSPOp: endTime %u\n", pTXSTATUSRSPParms->endTime);
    A_PRINTF("TXSTATUSRSPOp: byteCount %u\n", pTXSTATUSRSPParms->byteCount);
    A_PRINTF("TXSTATUSRSPOp: dontCount %u\n", pTXSTATUSRSPParms->dontCount);
    A_PRINTF("TXSTATUSRSPOp: rssi %d\n", pTXSTATUSRSPParms->rssi);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("TXSTATUSRSPOp: rssic %d\n", pTXSTATUSRSPParms->rssic[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("TXSTATUSRSPOp: rssie %d\n", pTXSTATUSRSPParms->rssie[i]);
    }
    A_PRINTF("TXSTATUSRSPOp: rateBitIndex %u\n", pTXSTATUSRSPParms->rateBitIndex);
#endif //_DEBUG

    if (NULL != TXSTATUSRSPOpFunc) {
        (*TXSTATUSRSPOpFunc)(pTXSTATUSRSPParms);
    }
    return(TRUE);
}
