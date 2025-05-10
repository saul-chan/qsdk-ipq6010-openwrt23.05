/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

// This is an auto-generated file from input/cmdSetPhyOnOff.s
#include "tlv2Inc.h"
#include "cmdSetPhyOnOff.h"

void* initSETPHYONOFFOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_SETPHYONOFF_PARMS  *pSETPHYONOFFParms = (CMD_SETPHYONOFF_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pSETPHYONOFFParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pSETPHYONOFFParms->enable_disable = pParmDict[PARM_ENABLE_DISABLE].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pSETPHYONOFFParms->phyId)) - (size_t)pSETPHYONOFFParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENABLE_DISABLE, (size_t)(((size_t)&(pSETPHYONOFFParms->enable_disable)) - (size_t)pSETPHYONOFFParms), pParmsOffset);
    return((void*) pSETPHYONOFFParms);
}

static SETPHYONOFF_OP_FUNC SETPHYONOFFOpFunc = NULL;

TLV2_API void registerSETPHYONOFFHandler(SETPHYONOFF_OP_FUNC fp)
{
    SETPHYONOFFOpFunc = fp;
}

A_BOOL SETPHYONOFFOp(void *pParms)
{
    CMD_SETPHYONOFF_PARMS *pSETPHYONOFFParms = (CMD_SETPHYONOFF_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("SETPHYONOFFOp: phyId %u\n", pSETPHYONOFFParms->phyId);
    A_PRINTF("SETPHYONOFFOp: enable_disable %u\n", pSETPHYONOFFParms->enable_disable);
#endif //_DEBUG

    if (NULL != SETPHYONOFFOpFunc) {
        (*SETPHYONOFFOpFunc)(pSETPHYONOFFParms);
    }
    return(TRUE);
}
