/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input\cmdLMChannelListAndRspHandler.s
#include "tlv2Inc.h"
#include "cmdLMChannelListAndRspHandler.h"

void* initLMCHANNELLISTOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_LMCHANNELLIST_PARMS  *pLMCHANNELLISTParms = (CMD_LMCHANNELLIST_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pLMCHANNELLISTParms->lm_flag = pParmDict[PARM_LM_FLAG].v.valU8;
    pLMCHANNELLISTParms->lm_itemNum = pParmDict[PARM_LM_ITEMNUM].v.valU8;
    memset(pLMCHANNELLISTParms->pad2, 0, sizeof(pLMCHANNELLISTParms->pad2));
    memset(pLMCHANNELLISTParms->lm_cbState, 0, sizeof(pLMCHANNELLISTParms->lm_cbState));
    pLMCHANNELLISTParms->lm_chainMask[0] = 0x1;
    memset(&pLMCHANNELLISTParms->lm_chainMask[1], 0x0, 99);
    pLMCHANNELLISTParms->lm_txPower[0] = 22;
    memset(&pLMCHANNELLISTParms->lm_txPower[1], 0, 99);
    memset(pLMCHANNELLISTParms->lm_phyId, 0, sizeof(pLMCHANNELLISTParms->lm_phyId));
    pLMCHANNELLISTParms->lm_freq[0] = 2412;
    memset(&pLMCHANNELLISTParms->lm_freq[1], 0, 99);
    pLMCHANNELLISTParms->lm_freq2[0] = 2412;
    memset(&pLMCHANNELLISTParms->lm_freq2[1], 0, 99);
    pLMCHANNELLISTParms->lm_rateIndex[0] = 11;
    memset(&pLMCHANNELLISTParms->lm_rateIndex[1], 0, 99);
    pLMCHANNELLISTParms->lm_nss[0] = 0;
    memset(&pLMCHANNELLISTParms->lm_nss[1], 0, 99);
    pLMCHANNELLISTParms->lm_ratebw[0] = 0;
    memset(&pLMCHANNELLISTParms->lm_ratebw[1], 0, 99);
    pLMCHANNELLISTParms->lm_gix[0] = 0;
    memset(&pLMCHANNELLISTParms->lm_gix[1], 0, 99);

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_LM_FLAG, (size_t)(((size_t)&(pLMCHANNELLISTParms->lm_flag)) - (size_t)pLMCHANNELLISTParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_ITEMNUM, (size_t)(((size_t)&(pLMCHANNELLISTParms->lm_itemNum)) - (size_t)pLMCHANNELLISTParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD2, (size_t)(((size_t)&(pLMCHANNELLISTParms->pad2)) - (size_t)pLMCHANNELLISTParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_CBSTATE, (size_t)(((size_t)&(pLMCHANNELLISTParms->lm_cbState)) - (size_t)pLMCHANNELLISTParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_CHAINMASK, (size_t)(((size_t)&(pLMCHANNELLISTParms->lm_chainMask)) - (size_t)pLMCHANNELLISTParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_TXPOWER, (size_t)(((size_t)&(pLMCHANNELLISTParms->lm_txPower)) - (size_t)pLMCHANNELLISTParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_PHYID, (size_t)(((size_t)&(pLMCHANNELLISTParms->lm_phyId)) - (size_t)pLMCHANNELLISTParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_FREQ, (size_t)(((size_t)&(pLMCHANNELLISTParms->lm_freq)) - (size_t)pLMCHANNELLISTParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_FREQ2, (size_t)(((size_t)&(pLMCHANNELLISTParms->lm_freq2)) - (size_t)pLMCHANNELLISTParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_RATEINDEX, (size_t)(((size_t)&(pLMCHANNELLISTParms->lm_rateIndex)) - (size_t)pLMCHANNELLISTParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_NSS, (size_t)(((size_t)&(pLMCHANNELLISTParms->lm_nss)) - (size_t)pLMCHANNELLISTParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_RATEBW, (size_t)(((size_t)&(pLMCHANNELLISTParms->lm_ratebw)) - (size_t)pLMCHANNELLISTParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_GIX, (size_t)(((size_t)&(pLMCHANNELLISTParms->lm_gix)) - (size_t)pLMCHANNELLISTParms), pParmsOffset);
    return((void*) pLMCHANNELLISTParms);
}

static LMCHANNELLIST_OP_FUNC LMCHANNELLISTOpFunc = NULL;

TLV2_API void registerLMCHANNELLISTHandler(LMCHANNELLIST_OP_FUNC fp)
{
    LMCHANNELLISTOpFunc = fp;
}

A_BOOL LMCHANNELLISTOp(void *pParms)
{
    CMD_LMCHANNELLIST_PARMS *pLMCHANNELLISTParms = (CMD_LMCHANNELLIST_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("LMCHANNELLISTOp: lm_flag 0x%x\n", pLMCHANNELLISTParms->lm_flag);
    A_PRINTF("LMCHANNELLISTOp: lm_itemNum %u\n", pLMCHANNELLISTParms->lm_itemNum);
    for (i = 0; i < 2 ; i++)
    {
        A_PRINTF("LMCHANNELLISTOp: pad2 %u\n", pLMCHANNELLISTParms->pad2[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMCHANNELLISTOp: lm_cbState %u\n", pLMCHANNELLISTParms->lm_cbState[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMCHANNELLISTOp: lm_chainMask 0x%x\n", pLMCHANNELLISTParms->lm_chainMask[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMCHANNELLISTOp: lm_txPower %u\n", pLMCHANNELLISTParms->lm_txPower[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMCHANNELLISTOp: lm_phyId %u\n", pLMCHANNELLISTParms->lm_phyId[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMCHANNELLISTOp: lm_freq %u\n", pLMCHANNELLISTParms->lm_freq[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMCHANNELLISTOp: lm_freq2 %u\n", pLMCHANNELLISTParms->lm_freq2[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMCHANNELLISTOp: lm_rateIndex %u\n", pLMCHANNELLISTParms->lm_rateIndex[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMCHANNELLISTOp: lm_nss %u\n", pLMCHANNELLISTParms->lm_nss[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMCHANNELLISTOp: lm_ratebw %u\n", pLMCHANNELLISTParms->lm_ratebw[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMCHANNELLISTOp: lm_gix %u\n", pLMCHANNELLISTParms->lm_gix[i]);
    }
#endif //_DEBUG

    if (NULL != LMCHANNELLISTOpFunc) {
        (*LMCHANNELLISTOpFunc)(pLMCHANNELLISTParms);
    }
    return(TRUE);
}

void* initLMCHANNELLISTRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_LMCHANNELLISTRSP_PARMS  *pLMCHANNELLISTRSPParms = (CMD_LMCHANNELLISTRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pLMCHANNELLISTRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pLMCHANNELLISTRSPParms->totalHWCalNum = pParmDict[PARM_TOTALHWCALNUM].v.valU8;
    pLMCHANNELLISTRSPParms->totalItemNum = pParmDict[PARM_TOTALITEMNUM].v.valU16;
    pLMCHANNELLISTRSPParms->hwCalTime = pParmDict[PARM_HWCALTIME].v.valU16;
    memset(pLMCHANNELLISTRSPParms->pad2, 0, sizeof(pLMCHANNELLISTRSPParms->pad2));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pLMCHANNELLISTRSPParms->status)) - (size_t)pLMCHANNELLISTRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TOTALHWCALNUM, (size_t)(((size_t)&(pLMCHANNELLISTRSPParms->totalHWCalNum)) - (size_t)pLMCHANNELLISTRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TOTALITEMNUM, (size_t)(((size_t)&(pLMCHANNELLISTRSPParms->totalItemNum)) - (size_t)pLMCHANNELLISTRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_HWCALTIME, (size_t)(((size_t)&(pLMCHANNELLISTRSPParms->hwCalTime)) - (size_t)pLMCHANNELLISTRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD2, (size_t)(((size_t)&(pLMCHANNELLISTRSPParms->pad2)) - (size_t)pLMCHANNELLISTRSPParms), pParmsOffset);
    return((void*) pLMCHANNELLISTRSPParms);
}

static LMCHANNELLISTRSP_OP_FUNC LMCHANNELLISTRSPOpFunc = NULL;

TLV2_API void registerLMCHANNELLISTRSPHandler(LMCHANNELLISTRSP_OP_FUNC fp)
{
    LMCHANNELLISTRSPOpFunc = fp;
}

A_BOOL LMCHANNELLISTRSPOp(void *pParms)
{
    CMD_LMCHANNELLISTRSP_PARMS *pLMCHANNELLISTRSPParms = (CMD_LMCHANNELLISTRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("LMCHANNELLISTRSPOp: status %u\n", pLMCHANNELLISTRSPParms->status);
    A_PRINTF("LMCHANNELLISTRSPOp: totalHWCalNum %u\n", pLMCHANNELLISTRSPParms->totalHWCalNum);
    A_PRINTF("LMCHANNELLISTRSPOp: totalItemNum %u\n", pLMCHANNELLISTRSPParms->totalItemNum);
    A_PRINTF("LMCHANNELLISTRSPOp: hwCalTime %u\n", pLMCHANNELLISTRSPParms->hwCalTime);
    for (i = 0; i < 2 ; i++)
    {
        A_PRINTF("LMCHANNELLISTRSPOp: pad2 %u\n", pLMCHANNELLISTRSPParms->pad2[i]);
    }
#endif //_DEBUG

    if (NULL != LMCHANNELLISTRSPOpFunc) {
        (*LMCHANNELLISTRSPOpFunc)(pLMCHANNELLISTRSPParms);
    }
    return(TRUE);
}
