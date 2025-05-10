/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdAgcHistoryConfig.s
#include "tlv2Inc.h"
#include "cmdAgcHistoryConfig.h"

void* initAGCHISTORYCONFIGOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_AGCHISTORYCONFIG_PARMS  *pAGCHISTORYCONFIGParms = (CMD_AGCHISTORYCONFIG_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pAGCHISTORYCONFIGParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pAGCHISTORYCONFIGParms->agcControl = pParmDict[PARM_AGCCONTROL].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pAGCHISTORYCONFIGParms->phyId)) - (size_t)pAGCHISTORYCONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_AGCCONTROL, (size_t)(((size_t)&(pAGCHISTORYCONFIGParms->agcControl)) - (size_t)pAGCHISTORYCONFIGParms), pParmsOffset);
    return((void*) pAGCHISTORYCONFIGParms);
}

static AGCHISTORYCONFIG_OP_FUNC AGCHISTORYCONFIGOpFunc = NULL;

TLV2_API void registerAGCHISTORYCONFIGHandler(AGCHISTORYCONFIG_OP_FUNC fp)
{
    AGCHISTORYCONFIGOpFunc = fp;
}

A_BOOL AGCHISTORYCONFIGOp(void *pParms)
{
    CMD_AGCHISTORYCONFIG_PARMS *pAGCHISTORYCONFIGParms = (CMD_AGCHISTORYCONFIG_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("AGCHISTORYCONFIGOp: phyId %u\n", pAGCHISTORYCONFIGParms->phyId);
    A_PRINTF("AGCHISTORYCONFIGOp: agcControl %u\n", pAGCHISTORYCONFIGParms->agcControl);
#endif //_DEBUG

    if (NULL != AGCHISTORYCONFIGOpFunc) {
        (*AGCHISTORYCONFIGOpFunc)(pAGCHISTORYCONFIGParms);
    }
    return(TRUE);
}

void* initAGCHISTORYCONFIGRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_AGCHISTORYCONFIGRSP_PARMS  *pAGCHISTORYCONFIGRSPParms = (CMD_AGCHISTORYCONFIGRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pAGCHISTORYCONFIGRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pAGCHISTORYCONFIGRSPParms->status = pParmDict[PARM_STATUS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pAGCHISTORYCONFIGRSPParms->phyId)) - (size_t)pAGCHISTORYCONFIGRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pAGCHISTORYCONFIGRSPParms->status)) - (size_t)pAGCHISTORYCONFIGRSPParms), pParmsOffset);
    return((void*) pAGCHISTORYCONFIGRSPParms);
}

static AGCHISTORYCONFIGRSP_OP_FUNC AGCHISTORYCONFIGRSPOpFunc = NULL;

TLV2_API void registerAGCHISTORYCONFIGRSPHandler(AGCHISTORYCONFIGRSP_OP_FUNC fp)
{
    AGCHISTORYCONFIGRSPOpFunc = fp;
}

A_BOOL AGCHISTORYCONFIGRSPOp(void *pParms)
{
    CMD_AGCHISTORYCONFIGRSP_PARMS *pAGCHISTORYCONFIGRSPParms = (CMD_AGCHISTORYCONFIGRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("AGCHISTORYCONFIGRSPOp: phyId %u\n", pAGCHISTORYCONFIGRSPParms->phyId);
    A_PRINTF("AGCHISTORYCONFIGRSPOp: status %u\n", pAGCHISTORYCONFIGRSPParms->status);
#endif //_DEBUG

    if (NULL != AGCHISTORYCONFIGRSPOpFunc) {
        (*AGCHISTORYCONFIGRSPOpFunc)(pAGCHISTORYCONFIGRSPParms);
    }
    return(TRUE);
}
