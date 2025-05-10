/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdRxGainCtrl.s
#include "tlv2Inc.h"
#include "cmdRxGainCtrl.h"

void* initRXGAINCTRLOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_RXGAINCTRL_PARMS  *pRXGAINCTRLParms = (CMD_RXGAINCTRL_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pRXGAINCTRLParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pRXGAINCTRLParms->xlnaCtrl = pParmDict[PARM_XLNACTRL].v.valU8;
    pRXGAINCTRLParms->gainCtrlMode = pParmDict[PARM_GAINCTRLMODE].v.valU8;
    pRXGAINCTRLParms->band = pParmDict[PARM_BAND].v.valU8;
    pRXGAINCTRLParms->gainIdx = pParmDict[PARM_GAINIDX].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pRXGAINCTRLParms->phyId)) - (size_t)pRXGAINCTRLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_XLNACTRL, (size_t)(((size_t)&(pRXGAINCTRLParms->xlnaCtrl)) - (size_t)pRXGAINCTRLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GAINCTRLMODE, (size_t)(((size_t)&(pRXGAINCTRLParms->gainCtrlMode)) - (size_t)pRXGAINCTRLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BAND, (size_t)(((size_t)&(pRXGAINCTRLParms->band)) - (size_t)pRXGAINCTRLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GAINIDX, (size_t)(((size_t)&(pRXGAINCTRLParms->gainIdx)) - (size_t)pRXGAINCTRLParms), pParmsOffset);
    return((void*) pRXGAINCTRLParms);
}

static RXGAINCTRL_OP_FUNC RXGAINCTRLOpFunc = NULL;

TLV2_API void registerRXGAINCTRLHandler(RXGAINCTRL_OP_FUNC fp)
{
    RXGAINCTRLOpFunc = fp;
}

A_BOOL RXGAINCTRLOp(void *pParms)
{
    CMD_RXGAINCTRL_PARMS *pRXGAINCTRLParms = (CMD_RXGAINCTRL_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("RXGAINCTRLOp: phyId %u\n", pRXGAINCTRLParms->phyId);
    A_PRINTF("RXGAINCTRLOp: xlnaCtrl %u\n", pRXGAINCTRLParms->xlnaCtrl);
    A_PRINTF("RXGAINCTRLOp: gainCtrlMode %u\n", pRXGAINCTRLParms->gainCtrlMode);
    A_PRINTF("RXGAINCTRLOp: band %u\n", pRXGAINCTRLParms->band);
    A_PRINTF("RXGAINCTRLOp: gainIdx %u\n", pRXGAINCTRLParms->gainIdx);
#endif //_DEBUG

    if (NULL != RXGAINCTRLOpFunc) {
        (*RXGAINCTRLOpFunc)(pRXGAINCTRLParms);
    }
    return(TRUE);
}

void* initRXGAINCTRLRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_RXGAINCTRLRSP_PARMS  *pRXGAINCTRLRSPParms = (CMD_RXGAINCTRLRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pRXGAINCTRLRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pRXGAINCTRLRSPParms->status = pParmDict[PARM_STATUS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pRXGAINCTRLRSPParms->phyId)) - (size_t)pRXGAINCTRLRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pRXGAINCTRLRSPParms->status)) - (size_t)pRXGAINCTRLRSPParms), pParmsOffset);
    return((void*) pRXGAINCTRLRSPParms);
}

static RXGAINCTRLRSP_OP_FUNC RXGAINCTRLRSPOpFunc = NULL;

TLV2_API void registerRXGAINCTRLRSPHandler(RXGAINCTRLRSP_OP_FUNC fp)
{
    RXGAINCTRLRSPOpFunc = fp;
}

A_BOOL RXGAINCTRLRSPOp(void *pParms)
{
    CMD_RXGAINCTRLRSP_PARMS *pRXGAINCTRLRSPParms = (CMD_RXGAINCTRLRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("RXGAINCTRLRSPOp: phyId %u\n", pRXGAINCTRLRSPParms->phyId);
    A_PRINTF("RXGAINCTRLRSPOp: status %u\n", pRXGAINCTRLRSPParms->status);
#endif //_DEBUG

    if (NULL != RXGAINCTRLRSPOpFunc) {
        (*RXGAINCTRLRSPOpFunc)(pRXGAINCTRLRSPParms);
    }
    return(TRUE);
}
