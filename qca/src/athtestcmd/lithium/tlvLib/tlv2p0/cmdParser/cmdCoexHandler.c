/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdCoexHandler.s
#include "tlv2Inc.h"
#include "cmdCoexHandler.h"

void* initCOEXREQOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_COEXREQ_PARMS  *pCOEXREQParms = (CMD_COEXREQ_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    memset(pCOEXREQParms->WlanCoexWeight, 0, sizeof(pCOEXREQParms->WlanCoexWeight));
    pCOEXREQParms->shareChainMask = pParmDict[PARM_SHARECHAINMASK].v.valU16;
    pCOEXREQParms->enableDynamicAggr = pParmDict[PARM_ENABLEDYNAMICAGGR].v.valU8;
    pCOEXREQParms->enableConcurrentTxChain0 = pParmDict[PARM_ENABLECONCURRENTTXCHAIN0].v.valU8;
    pCOEXREQParms->btRxAllowWlanTxUnshared = pParmDict[PARM_BTRXALLOWWLANTXUNSHARED].v.valU8;
    pCOEXREQParms->btTxAllowWlanTxShared = pParmDict[PARM_BTTXALLOWWLANTXSHARED].v.valU8;
    pCOEXREQParms->wlanTxAllowBtRxShared = pParmDict[PARM_WLANTXALLOWBTRXSHARED].v.valU8;
    pCOEXREQParms->wlanRxAllowBtTxShared = pParmDict[PARM_WLANRXALLOWBTTXSHARED].v.valU8;
    pCOEXREQParms->concurrentTxEnable = pParmDict[PARM_CONCURRENTTXENABLE].v.valU8;
    pCOEXREQParms->btRxAllowWlanUnsharedTx = pParmDict[PARM_BTRXALLOWWLANUNSHAREDTX].v.valU8;
    pCOEXREQParms->wlan1sRxBtTx = pParmDict[PARM_WLAN1SRXBTTX].v.valU8;
    pCOEXREQParms->enableBtContInfo = pParmDict[PARM_ENABLEBTCONTINFO].v.valU8;
    pCOEXREQParms->enableBtSchedInfo = pParmDict[PARM_ENABLEBTSCHEDINFO].v.valU8;
    pCOEXREQParms->enableBtLnaInfo = pParmDict[PARM_ENABLEBTLNAINFO].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_WLANCOEXWEIGHT, (size_t)(((size_t)&(pCOEXREQParms->WlanCoexWeight)) - (size_t)pCOEXREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SHARECHAINMASK, (size_t)(((size_t)&(pCOEXREQParms->shareChainMask)) - (size_t)pCOEXREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENABLEDYNAMICAGGR, (size_t)(((size_t)&(pCOEXREQParms->enableDynamicAggr)) - (size_t)pCOEXREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENABLECONCURRENTTXCHAIN0, (size_t)(((size_t)&(pCOEXREQParms->enableConcurrentTxChain0)) - (size_t)pCOEXREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BTRXALLOWWLANTXUNSHARED, (size_t)(((size_t)&(pCOEXREQParms->btRxAllowWlanTxUnshared)) - (size_t)pCOEXREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BTTXALLOWWLANTXSHARED, (size_t)(((size_t)&(pCOEXREQParms->btTxAllowWlanTxShared)) - (size_t)pCOEXREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_WLANTXALLOWBTRXSHARED, (size_t)(((size_t)&(pCOEXREQParms->wlanTxAllowBtRxShared)) - (size_t)pCOEXREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_WLANRXALLOWBTTXSHARED, (size_t)(((size_t)&(pCOEXREQParms->wlanRxAllowBtTxShared)) - (size_t)pCOEXREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CONCURRENTTXENABLE, (size_t)(((size_t)&(pCOEXREQParms->concurrentTxEnable)) - (size_t)pCOEXREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BTRXALLOWWLANUNSHAREDTX, (size_t)(((size_t)&(pCOEXREQParms->btRxAllowWlanUnsharedTx)) - (size_t)pCOEXREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_WLAN1SRXBTTX, (size_t)(((size_t)&(pCOEXREQParms->wlan1sRxBtTx)) - (size_t)pCOEXREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENABLEBTCONTINFO, (size_t)(((size_t)&(pCOEXREQParms->enableBtContInfo)) - (size_t)pCOEXREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENABLEBTSCHEDINFO, (size_t)(((size_t)&(pCOEXREQParms->enableBtSchedInfo)) - (size_t)pCOEXREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENABLEBTLNAINFO, (size_t)(((size_t)&(pCOEXREQParms->enableBtLnaInfo)) - (size_t)pCOEXREQParms), pParmsOffset);
    return((void*) pCOEXREQParms);
}

static COEXREQ_OP_FUNC COEXREQOpFunc = NULL;

TLV2_API void registerCOEXREQHandler(COEXREQ_OP_FUNC fp)
{
    COEXREQOpFunc = fp;
}

A_BOOL COEXREQOp(void *pParms)
{
    CMD_COEXREQ_PARMS *pCOEXREQParms = (CMD_COEXREQ_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    for (i = 0; i < 8 ; i++) // can be modified to print up to 11 entries
    {
        A_PRINTF("COEXREQOp: WlanCoexWeight %u\n", pCOEXREQParms->WlanCoexWeight[i]);
    }
    A_PRINTF("COEXREQOp: shareChainMask %u\n", pCOEXREQParms->shareChainMask);
    A_PRINTF("COEXREQOp: enableDynamicAggr %u\n", pCOEXREQParms->enableDynamicAggr);
    A_PRINTF("COEXREQOp: enableConcurrentTxChain0 %u\n", pCOEXREQParms->enableConcurrentTxChain0);
    A_PRINTF("COEXREQOp: btRxAllowWlanTxUnshared %u\n", pCOEXREQParms->btRxAllowWlanTxUnshared);
    A_PRINTF("COEXREQOp: btTxAllowWlanTxShared %u\n", pCOEXREQParms->btTxAllowWlanTxShared);
    A_PRINTF("COEXREQOp: wlanTxAllowBtRxShared %u\n", pCOEXREQParms->wlanTxAllowBtRxShared);
    A_PRINTF("COEXREQOp: wlanRxAllowBtTxShared %u\n", pCOEXREQParms->wlanRxAllowBtTxShared);
    A_PRINTF("COEXREQOp: concurrentTxEnable %u\n", pCOEXREQParms->concurrentTxEnable);
    A_PRINTF("COEXREQOp: btRxAllowWlanUnsharedTx %u\n", pCOEXREQParms->btRxAllowWlanUnsharedTx);
    A_PRINTF("COEXREQOp: wlan1sRxBtTx %u\n", pCOEXREQParms->wlan1sRxBtTx);
    A_PRINTF("COEXREQOp: enableBtContInfo %u\n", pCOEXREQParms->enableBtContInfo);
    A_PRINTF("COEXREQOp: enableBtSchedInfo %u\n", pCOEXREQParms->enableBtSchedInfo);
    A_PRINTF("COEXREQOp: enableBtLnaInfo %u\n", pCOEXREQParms->enableBtLnaInfo);
#endif //_DEBUG

    if (NULL != COEXREQOpFunc) {
        (*COEXREQOpFunc)(pCOEXREQParms);
    }
    return(TRUE);
}

void* initCOEXREQRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_COEXREQRSP_PARMS  *pCOEXREQRSPParms = (CMD_COEXREQRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pCOEXREQRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pCOEXREQRSPParms->reason = pParmDict[PARM_REASON].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pCOEXREQRSPParms->status)) - (size_t)pCOEXREQRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_REASON, (size_t)(((size_t)&(pCOEXREQRSPParms->reason)) - (size_t)pCOEXREQRSPParms), pParmsOffset);
    return((void*) pCOEXREQRSPParms);
}

static COEXREQRSP_OP_FUNC COEXREQRSPOpFunc = NULL;

TLV2_API void registerCOEXREQRSPHandler(COEXREQRSP_OP_FUNC fp)
{
    COEXREQRSPOpFunc = fp;
}

A_BOOL COEXREQRSPOp(void *pParms)
{
    CMD_COEXREQRSP_PARMS *pCOEXREQRSPParms = (CMD_COEXREQRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("COEXREQRSPOp: status %u\n", pCOEXREQRSPParms->status);
    A_PRINTF("COEXREQRSPOp: reason %u\n", pCOEXREQRSPParms->reason);
#endif //_DEBUG

    if (NULL != COEXREQRSPOpFunc) {
        (*COEXREQRSPOpFunc)(pCOEXREQRSPParms);
    }
    return(TRUE);
}
