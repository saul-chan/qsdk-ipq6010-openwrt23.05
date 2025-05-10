/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input\cmdLMRxStatusHandler.s
#include "tlv2Inc.h"
#include "cmdLMRxStatusHandler.h"

void* initLMRXSTATUSOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_LMRXSTATUS_PARMS  *pLMRXSTATUSParms = (CMD_LMRXSTATUS_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pLMRXSTATUSParms->lm_flag = pParmDict[PARM_LM_FLAG].v.valU8;
    pLMRXSTATUSParms->lm_itemNum = pParmDict[PARM_LM_ITEMNUM].v.valU8;
    memset(pLMRXSTATUSParms->pad2, 0, sizeof(pLMRXSTATUSParms->pad2));
    pLMRXSTATUSParms->pad2[0] = pParmDict[PARM_PAD2].v.ptU8[0];

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_LM_FLAG, (size_t)(((size_t)&(pLMRXSTATUSParms->lm_flag)) - (size_t)pLMRXSTATUSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_ITEMNUM, (size_t)(((size_t)&(pLMRXSTATUSParms->lm_itemNum)) - (size_t)pLMRXSTATUSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD2, (size_t)(((size_t)&(pLMRXSTATUSParms->pad2)) - (size_t)pLMRXSTATUSParms), pParmsOffset);
    return((void*) pLMRXSTATUSParms);
}

static LMRXSTATUS_OP_FUNC LMRXSTATUSOpFunc = NULL;

TLV2_API void registerLMRXSTATUSHandler(LMRXSTATUS_OP_FUNC fp)
{
    LMRXSTATUSOpFunc = fp;
}

A_BOOL LMRXSTATUSOp(void *pParms)
{
    CMD_LMRXSTATUS_PARMS *pLMRXSTATUSParms = (CMD_LMRXSTATUS_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("LMRXSTATUSOp: lm_flag %u\n", pLMRXSTATUSParms->lm_flag);
    A_PRINTF("LMRXSTATUSOp: lm_itemNum %u\n", pLMRXSTATUSParms->lm_itemNum);
    for (i = 0; i < 2 ; i++)
    {
        A_PRINTF("LMRXSTATUSOp: pad2 %u\n", pLMRXSTATUSParms->pad2[i]);
    }
#endif //_DEBUG

    if (NULL != LMRXSTATUSOpFunc) {
        (*LMRXSTATUSOpFunc)(pLMRXSTATUSParms);
    }
    return(TRUE);
}

void* initLMRXSTATUSRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_LMRXSTATUSRSP_PARMS  *pLMRXSTATUSRSPParms = (CMD_LMRXSTATUSRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    memset(pLMRXSTATUSRSPParms->lm_phyId, 0, sizeof(pLMRXSTATUSRSPParms->lm_phyId));
    pLMRXSTATUSRSPParms->lm_totalGoodPackets[0] = 0;
    memset(&pLMRXSTATUSRSPParms->lm_totalGoodPackets[1], 0, 99);
    pLMRXSTATUSRSPParms->lm_goodPackets[0] = 0;
    memset(&pLMRXSTATUSRSPParms->lm_goodPackets[1], 0, 99);
    pLMRXSTATUSRSPParms->lm_otherErrors[0] = 0;
    memset(&pLMRXSTATUSRSPParms->lm_otherErrors[1], 0, 99);
    pLMRXSTATUSRSPParms->lm_crcPackets[0] = 0;
    memset(&pLMRXSTATUSRSPParms->lm_crcPackets[1], 0, 99);
    pLMRXSTATUSRSPParms->lm_crcPackets[0] = 0;
    memset(&pLMRXSTATUSRSPParms->lm_crcPackets[1], 0, 99);
    pLMRXSTATUSRSPParms->lm_decrypErrors[0] = 0;
    memset(&pLMRXSTATUSRSPParms->lm_decrypErrors[1], 0, 99);
    pLMRXSTATUSRSPParms->lm_rateBit[0] = 0;
    memset(&pLMRXSTATUSRSPParms->lm_rateBit[1], 0, 99);
    pLMRXSTATUSRSPParms->lm_statTime[0] = 0;
    memset(&pLMRXSTATUSRSPParms->lm_statTime[1], 0, 99);
    pLMRXSTATUSRSPParms->lm_endTime[0] = 0;
    memset(&pLMRXSTATUSRSPParms->lm_endTime[1], 0, 99);
    pLMRXSTATUSRSPParms->lm_byteCount[0] = 0;
    memset(&pLMRXSTATUSRSPParms->lm_byteCount[1], 0, 99);
    pLMRXSTATUSRSPParms->lm_dontCount[0] = 0;
    memset(&pLMRXSTATUSRSPParms->lm_dontCount[1], 0, 99);
    pLMRXSTATUSRSPParms->lm_rssi[0] = 0;
    memset(&pLMRXSTATUSRSPParms->lm_rssi[1], 0, 99);

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_LM_PHYID, (size_t)(((size_t)&(pLMRXSTATUSRSPParms->lm_phyId)) - (size_t)pLMRXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_TOTALGOODPACKETS, (size_t)(((size_t)&(pLMRXSTATUSRSPParms->lm_totalGoodPackets)) - (size_t)pLMRXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_GOODPACKETS, (size_t)(((size_t)&(pLMRXSTATUSRSPParms->lm_goodPackets)) - (size_t)pLMRXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_OTHERERRORS, (size_t)(((size_t)&(pLMRXSTATUSRSPParms->lm_otherErrors)) - (size_t)pLMRXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_CRCPACKETS, (size_t)(((size_t)&(pLMRXSTATUSRSPParms->lm_crcPackets)) - (size_t)pLMRXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_CRCPACKETS, (size_t)(((size_t)&(pLMRXSTATUSRSPParms->lm_crcPackets)) - (size_t)pLMRXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_DECRYPERRORS, (size_t)(((size_t)&(pLMRXSTATUSRSPParms->lm_decrypErrors)) - (size_t)pLMRXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_RATEBIT, (size_t)(((size_t)&(pLMRXSTATUSRSPParms->lm_rateBit)) - (size_t)pLMRXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_STATTIME, (size_t)(((size_t)&(pLMRXSTATUSRSPParms->lm_statTime)) - (size_t)pLMRXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_ENDTIME, (size_t)(((size_t)&(pLMRXSTATUSRSPParms->lm_endTime)) - (size_t)pLMRXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_BYTECOUNT, (size_t)(((size_t)&(pLMRXSTATUSRSPParms->lm_byteCount)) - (size_t)pLMRXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_DONTCOUNT, (size_t)(((size_t)&(pLMRXSTATUSRSPParms->lm_dontCount)) - (size_t)pLMRXSTATUSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_RSSI, (size_t)(((size_t)&(pLMRXSTATUSRSPParms->lm_rssi)) - (size_t)pLMRXSTATUSRSPParms), pParmsOffset);
    return((void*) pLMRXSTATUSRSPParms);
}

static LMRXSTATUSRSP_OP_FUNC LMRXSTATUSRSPOpFunc = NULL;

TLV2_API void registerLMRXSTATUSRSPHandler(LMRXSTATUSRSP_OP_FUNC fp)
{
    LMRXSTATUSRSPOpFunc = fp;
}

A_BOOL LMRXSTATUSRSPOp(void *pParms)
{
    CMD_LMRXSTATUSRSP_PARMS *pLMRXSTATUSRSPParms = (CMD_LMRXSTATUSRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMRXSTATUSRSPOp: lm_phyId %u\n", pLMRXSTATUSRSPParms->lm_phyId[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMRXSTATUSRSPOp: lm_totalGoodPackets %u\n", pLMRXSTATUSRSPParms->lm_totalGoodPackets[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMRXSTATUSRSPOp: lm_goodPackets %u\n", pLMRXSTATUSRSPParms->lm_goodPackets[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMRXSTATUSRSPOp: lm_otherErrors %u\n", pLMRXSTATUSRSPParms->lm_otherErrors[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMRXSTATUSRSPOp: lm_crcPackets %u\n", pLMRXSTATUSRSPParms->lm_crcPackets[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMRXSTATUSRSPOp: lm_crcPackets %u\n", pLMRXSTATUSRSPParms->lm_crcPackets[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMRXSTATUSRSPOp: lm_decrypErrors %u\n", pLMRXSTATUSRSPParms->lm_decrypErrors[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMRXSTATUSRSPOp: lm_rateBit %u\n", pLMRXSTATUSRSPParms->lm_rateBit[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMRXSTATUSRSPOp: lm_statTime %u\n", pLMRXSTATUSRSPParms->lm_statTime[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMRXSTATUSRSPOp: lm_endTime %u\n", pLMRXSTATUSRSPParms->lm_endTime[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMRXSTATUSRSPOp: lm_byteCount %u\n", pLMRXSTATUSRSPParms->lm_byteCount[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 100 entries
    {
        A_PRINTF("LMRXSTATUSRSPOp: lm_dontCount %u\n", pLMRXSTATUSRSPParms->lm_dontCount[i]);
    }
    A_PRINTF("LMRXSTATUSRSPOp: lm_rssi %s\n", pLMRXSTATUSRSPParms->lm_rssi);
#endif //_DEBUG

    if (NULL != LMRXSTATUSRSPOpFunc) {
        (*LMRXSTATUSRSPOpFunc)(pLMRXSTATUSRSPParms);
    }
    return(TRUE);
}
