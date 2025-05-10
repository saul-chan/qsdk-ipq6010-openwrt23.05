/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input\cmdBasicRspHandler.s
#include "tlv2Inc.h"
#include "cmdBasicRspHandler.h"

void* initBASICRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_BASICRSP_PARMS  *pBASICRSPParms = (CMD_BASICRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pBASICRSPParms->cmdId = pParmDict[PARM_CMDID].v.valU16;
    pBASICRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pBASICRSPParms->status = pParmDict[PARM_STATUS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_CMDID, (size_t)(((size_t)&(pBASICRSPParms->cmdId)) - (size_t)pBASICRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pBASICRSPParms->phyId)) - (size_t)pBASICRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pBASICRSPParms->status)) - (size_t)pBASICRSPParms), pParmsOffset);
    return((void*) pBASICRSPParms);
}

static BASICRSP_OP_FUNC BASICRSPOpFunc = NULL;

TLV2_API void registerBASICRSPHandler(BASICRSP_OP_FUNC fp)
{
    BASICRSPOpFunc = fp;
}

A_BOOL BASICRSPOp(void *pParms)
{
    CMD_BASICRSP_PARMS *pBASICRSPParms = (CMD_BASICRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("BASICRSPOp: cmdId %u\n", pBASICRSPParms->cmdId);
    A_PRINTF("BASICRSPOp: phyId %u\n", pBASICRSPParms->phyId);
    A_PRINTF("BASICRSPOp: status %u\n", pBASICRSPParms->status);
#endif //_DEBUG

    if (NULL != BASICRSPOpFunc) {
        (*BASICRSPOpFunc)(pBASICRSPParms);
    }
    return(TRUE);
}
