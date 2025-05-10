/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from ./input/sysCmdtlvCaptureCtrl.s
#include "tlv2Inc.h"
#include "sysCmdtlvCaptureCtrl.h"

void* initTLVCAPTURECTRLOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    SYSCMD_TLVCAPTURECTRL_PARMS  *pTLVCAPTURECTRLParms = (SYSCMD_TLVCAPTURECTRL_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pTLVCAPTURECTRLParms->tlvCaptureCtrl_cmdID = pParmDict[SYSPARM_TLVCAPTURECTRL_CMDID-SYSPARM_FIRST_IDX].v.valU32;
    pTLVCAPTURECTRLParms->tlvCaptureCtrl_01 = pParmDict[SYSPARM_TLVCAPTURECTRL_01-SYSPARM_FIRST_IDX].v.valU32;
    pTLVCAPTURECTRLParms->tlvCaptureCtrl_02 = pParmDict[SYSPARM_TLVCAPTURECTRL_02-SYSPARM_FIRST_IDX].v.valU32;
    pTLVCAPTURECTRLParms->tlvCaptureCtrl_03 = pParmDict[SYSPARM_TLVCAPTURECTRL_03-SYSPARM_FIRST_IDX].v.valU32;
    pTLVCAPTURECTRLParms->tlvCaptureCtrl_04 = pParmDict[SYSPARM_TLVCAPTURECTRL_04-SYSPARM_FIRST_IDX].v.valU32;
    pTLVCAPTURECTRLParms->phyId = pParmDict[SYSPARM_PHYID-SYSPARM_FIRST_IDX].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)SYSPARM_TLVCAPTURECTRL_CMDID, (size_t)(((size_t)&(pTLVCAPTURECTRLParms->tlvCaptureCtrl_cmdID)) - (size_t)pTLVCAPTURECTRLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_TLVCAPTURECTRL_01, (size_t)(((size_t)&(pTLVCAPTURECTRLParms->tlvCaptureCtrl_01)) - (size_t)pTLVCAPTURECTRLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_TLVCAPTURECTRL_02, (size_t)(((size_t)&(pTLVCAPTURECTRLParms->tlvCaptureCtrl_02)) - (size_t)pTLVCAPTURECTRLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_TLVCAPTURECTRL_03, (size_t)(((size_t)&(pTLVCAPTURECTRLParms->tlvCaptureCtrl_03)) - (size_t)pTLVCAPTURECTRLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_TLVCAPTURECTRL_04, (size_t)(((size_t)&(pTLVCAPTURECTRLParms->tlvCaptureCtrl_04)) - (size_t)pTLVCAPTURECTRLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_PHYID, (size_t)(((size_t)&(pTLVCAPTURECTRLParms->phyId)) - (size_t)pTLVCAPTURECTRLParms), pParmsOffset);
    return((void*) pTLVCAPTURECTRLParms);
}

static TLVCAPTURECTRL_OP_FUNC TLVCAPTURECTRLOpFunc = NULL;

TLV2_API void registerTLVCAPTURECTRLHandler(TLVCAPTURECTRL_OP_FUNC fp)
{
    TLVCAPTURECTRLOpFunc = fp;
}

A_BOOL TLVCAPTURECTRLOp(void *pParms)
{
    SYSCMD_TLVCAPTURECTRL_PARMS *pTLVCAPTURECTRLParms = (SYSCMD_TLVCAPTURECTRL_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TLVCAPTURECTRLOp: tlvCaptureCtrl_cmdID %u\n", pTLVCAPTURECTRLParms->tlvCaptureCtrl_cmdID);
    A_PRINTF("TLVCAPTURECTRLOp: tlvCaptureCtrl_01 %u\n", pTLVCAPTURECTRLParms->tlvCaptureCtrl_01);
    A_PRINTF("TLVCAPTURECTRLOp: tlvCaptureCtrl_02 %u\n", pTLVCAPTURECTRLParms->tlvCaptureCtrl_02);
    A_PRINTF("TLVCAPTURECTRLOp: tlvCaptureCtrl_03 %u\n", pTLVCAPTURECTRLParms->tlvCaptureCtrl_03);
    A_PRINTF("TLVCAPTURECTRLOp: tlvCaptureCtrl_04 %u\n", pTLVCAPTURECTRLParms->tlvCaptureCtrl_04);
    A_PRINTF("TLVCAPTURECTRLOp: phyId %u\n", pTLVCAPTURECTRLParms->phyId);
#endif //_DEBUG

    if (NULL != TLVCAPTURECTRLOpFunc) {
        (*TLVCAPTURECTRLOpFunc)(pTLVCAPTURECTRLParms);
    }
    return(TRUE);
}

void* initTLVCAPTURECTRLRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    SYSCMD_TLVCAPTURECTRLRSP_PARMS  *pTLVCAPTURECTRLRSPParms = (SYSCMD_TLVCAPTURECTRLRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_cmdID = pParmDict[SYSPARM_TLVCAPTURECTRL_CMDID-SYSPARM_FIRST_IDX].v.valU32;
    pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_01 = pParmDict[SYSPARM_TLVCAPTURECTRL_01-SYSPARM_FIRST_IDX].v.valU32;
    pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_02 = pParmDict[SYSPARM_TLVCAPTURECTRL_02-SYSPARM_FIRST_IDX].v.valU32;
    pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_03 = pParmDict[SYSPARM_TLVCAPTURECTRL_03-SYSPARM_FIRST_IDX].v.valU32;
    pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_04 = pParmDict[SYSPARM_TLVCAPTURECTRL_04-SYSPARM_FIRST_IDX].v.valU32;
    pTLVCAPTURECTRLRSPParms->phyId = pParmDict[SYSPARM_PHYID-SYSPARM_FIRST_IDX].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)SYSPARM_TLVCAPTURECTRL_CMDID, (size_t)(((size_t)&(pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_cmdID)) - (size_t)pTLVCAPTURECTRLRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_TLVCAPTURECTRL_01, (size_t)(((size_t)&(pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_01)) - (size_t)pTLVCAPTURECTRLRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_TLVCAPTURECTRL_02, (size_t)(((size_t)&(pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_02)) - (size_t)pTLVCAPTURECTRLRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_TLVCAPTURECTRL_03, (size_t)(((size_t)&(pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_03)) - (size_t)pTLVCAPTURECTRLRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_TLVCAPTURECTRL_04, (size_t)(((size_t)&(pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_04)) - (size_t)pTLVCAPTURECTRLRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_PHYID, (size_t)(((size_t)&(pTLVCAPTURECTRLRSPParms->phyId)) - (size_t)pTLVCAPTURECTRLRSPParms), pParmsOffset);
    return((void*) pTLVCAPTURECTRLRSPParms);
}

static TLVCAPTURECTRLRSP_OP_FUNC TLVCAPTURECTRLRSPOpFunc = NULL;

TLV2_API void registerTLVCAPTURECTRLRSPHandler(TLVCAPTURECTRLRSP_OP_FUNC fp)
{
    TLVCAPTURECTRLRSPOpFunc = fp;
}

A_BOOL TLVCAPTURECTRLRSPOp(void *pParms)
{
    SYSCMD_TLVCAPTURECTRLRSP_PARMS *pTLVCAPTURECTRLRSPParms = (SYSCMD_TLVCAPTURECTRLRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TLVCAPTURECTRLRSPOp: tlvCaptureCtrl_cmdID %u\n", pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_cmdID);
    A_PRINTF("TLVCAPTURECTRLRSPOp: tlvCaptureCtrl_01 %u\n", pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_01);
    A_PRINTF("TLVCAPTURECTRLRSPOp: tlvCaptureCtrl_02 %u\n", pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_02);
    A_PRINTF("TLVCAPTURECTRLRSPOp: tlvCaptureCtrl_03 %u\n", pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_03);
    A_PRINTF("TLVCAPTURECTRLRSPOp: tlvCaptureCtrl_04 %u\n", pTLVCAPTURECTRLRSPParms->tlvCaptureCtrl_04);
    A_PRINTF("TLVCAPTURECTRLRSPOp: phyId %u\n", pTLVCAPTURECTRLRSPParms->phyId);
#endif //_DEBUG

    if (NULL != TLVCAPTURECTRLRSPOpFunc) {
        (*TLVCAPTURECTRLRSPOpFunc)(pTLVCAPTURECTRLRSPParms);
    }
    return(TRUE);
}
