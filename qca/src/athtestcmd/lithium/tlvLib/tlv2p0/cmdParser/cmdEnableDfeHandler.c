/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input\cmdEnableDfeHandler.s
#include "tlv2Inc.h"
#include "cmdEnableDfeHandler.h"

void* initENABLEDFEOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_ENABLEDFE_PARMS  *pENABLEDFEParms = (CMD_ENABLEDFE_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pENABLEDFEParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pENABLEDFEParms->enable = pParmDict[PARM_ENABLE].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pENABLEDFEParms->phyId)) - (size_t)pENABLEDFEParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENABLE, (size_t)(((size_t)&(pENABLEDFEParms->enable)) - (size_t)pENABLEDFEParms), pParmsOffset);
    return((void*) pENABLEDFEParms);
}

static ENABLEDFE_OP_FUNC ENABLEDFEOpFunc = NULL;

TLV2_API void registerENABLEDFEHandler(ENABLEDFE_OP_FUNC fp)
{
    ENABLEDFEOpFunc = fp;
}

A_BOOL ENABLEDFEOp(void *pParms)
{
    CMD_ENABLEDFE_PARMS *pENABLEDFEParms = (CMD_ENABLEDFE_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("ENABLEDFEOp: phyId %u\n", pENABLEDFEParms->phyId);
    A_PRINTF("ENABLEDFEOp: enable %u\n", pENABLEDFEParms->enable);
#endif //_DEBUG

    if (NULL != ENABLEDFEOpFunc) {
        (*ENABLEDFEOpFunc)(pENABLEDFEParms);
    }
    return(TRUE);
}

void* initENABLEDFERSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_ENABLEDFERSP_PARMS  *pENABLEDFERSPParms = (CMD_ENABLEDFERSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pENABLEDFERSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pENABLEDFERSPParms->status = pParmDict[PARM_STATUS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pENABLEDFERSPParms->phyId)) - (size_t)pENABLEDFERSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pENABLEDFERSPParms->status)) - (size_t)pENABLEDFERSPParms), pParmsOffset);
    return((void*) pENABLEDFERSPParms);
}

static ENABLEDFERSP_OP_FUNC ENABLEDFERSPOpFunc = NULL;

TLV2_API void registerENABLEDFERSPHandler(ENABLEDFERSP_OP_FUNC fp)
{
    ENABLEDFERSPOpFunc = fp;
}

A_BOOL ENABLEDFERSPOp(void *pParms)
{
    CMD_ENABLEDFERSP_PARMS *pENABLEDFERSPParms = (CMD_ENABLEDFERSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("ENABLEDFERSPOp: phyId %u\n", pENABLEDFERSPParms->phyId);
    A_PRINTF("ENABLEDFERSPOp: status %u\n", pENABLEDFERSPParms->status);
#endif //_DEBUG

    if (NULL != ENABLEDFERSPOpFunc) {
        (*ENABLEDFERSPOpFunc)(pENABLEDFERSPParms);
    }
    return(TRUE);
}
