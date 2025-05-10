/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

// This is an auto-generated file from input/cmdSetRxChainOnOff.s
#include "tlv2Inc.h"
#include "cmdSetRxChainOnOff.h"

void* initSETRXCHAINONOFFOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_SETRXCHAINONOFF_PARMS  *pSETRXCHAINONOFFParms = (CMD_SETRXCHAINONOFF_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pSETRXCHAINONOFFParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pSETRXCHAINONOFFParms->chainIdx = pParmDict[PARM_CHAINIDX].v.valU8;
    pSETRXCHAINONOFFParms->enable_disable = pParmDict[PARM_ENABLE_DISABLE].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pSETRXCHAINONOFFParms->phyId)) - (size_t)pSETRXCHAINONOFFParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINIDX, (size_t)(((size_t)&(pSETRXCHAINONOFFParms->chainIdx)) - (size_t)pSETRXCHAINONOFFParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENABLE_DISABLE, (size_t)(((size_t)&(pSETRXCHAINONOFFParms->enable_disable)) - (size_t)pSETRXCHAINONOFFParms), pParmsOffset);
    return((void*) pSETRXCHAINONOFFParms);
}

static SETRXCHAINONOFF_OP_FUNC SETRXCHAINONOFFOpFunc = NULL;

TLV2_API void registerSETRXCHAINONOFFHandler(SETRXCHAINONOFF_OP_FUNC fp)
{
    SETRXCHAINONOFFOpFunc = fp;
}

A_BOOL SETRXCHAINONOFFOp(void *pParms)
{
    CMD_SETRXCHAINONOFF_PARMS *pSETRXCHAINONOFFParms = (CMD_SETRXCHAINONOFF_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("SETRXCHAINONOFFOp: phyId %u\n", pSETRXCHAINONOFFParms->phyId);
    A_PRINTF("SETRXCHAINONOFFOp: chainIdx %u\n", pSETRXCHAINONOFFParms->chainIdx);
    A_PRINTF("SETRXCHAINONOFFOp: enable_disable %u\n", pSETRXCHAINONOFFParms->enable_disable);
#endif //_DEBUG

    if (NULL != SETRXCHAINONOFFOpFunc) {
        (*SETRXCHAINONOFFOpFunc)(pSETRXCHAINONOFFParms);
    }
    return(TRUE);
}
