/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input\cmdRegWriteHandler.s
#include "tlv2Inc.h"
#include "cmdRegWriteHandler.h"

void* initREGWRITEOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_REGWRITE_PARMS  *pREGWRITEParms = (CMD_REGWRITE_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pREGWRITEParms->regaddress = pParmDict[PARM_REGADDRESS].v.valU32;
    pREGWRITEParms->regvalue = pParmDict[PARM_REGVALUE].v.valU32;
    pREGWRITEParms->radioId = pParmDict[PARM_RADIOID].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_REGADDRESS, (size_t)(((size_t)&(pREGWRITEParms->regaddress)) - (size_t)pREGWRITEParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_REGVALUE, (size_t)(((size_t)&(pREGWRITEParms->regvalue)) - (size_t)pREGWRITEParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RADIOID, (size_t)(((size_t)&(pREGWRITEParms->radioId)) - (size_t)pREGWRITEParms), pParmsOffset);
    return((void*) pREGWRITEParms);
}

static REGWRITE_OP_FUNC REGWRITEOpFunc = NULL;

TLV2_API void registerREGWRITEHandler(REGWRITE_OP_FUNC fp)
{
    REGWRITEOpFunc = fp;
}

A_BOOL REGWRITEOp(void *pParms)
{
    CMD_REGWRITE_PARMS *pREGWRITEParms = (CMD_REGWRITE_PARMS *)pParms;
#if 0
//#ifdef _DEBUG

    A_PRINTF("REGWRITEOp: regaddress 0x%x\n", pREGWRITEParms->regaddress);
    A_PRINTF("REGWRITEOp: regvalue 0x%x\n", pREGWRITEParms->regvalue);
    A_PRINTF("REGWRITEOp: radioId %u\n", pREGWRITEParms->radioId);
#endif //_DEBUG

    if (NULL != REGWRITEOpFunc) {
        (*REGWRITEOpFunc)(pREGWRITEParms);
    }
    return(TRUE);
}

void* initREGWRITERSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_REGWRITERSP_PARMS  *pREGWRITERSPParms = (CMD_REGWRITERSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pREGWRITERSPParms->status = pParmDict[PARM_STATUS].v.valU32;
    pREGWRITERSPParms->radioId = pParmDict[PARM_RADIOID].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pREGWRITERSPParms->status)) - (size_t)pREGWRITERSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RADIOID, (size_t)(((size_t)&(pREGWRITERSPParms->radioId)) - (size_t)pREGWRITERSPParms), pParmsOffset);
    return((void*) pREGWRITERSPParms);
}

static REGWRITERSP_OP_FUNC REGWRITERSPOpFunc = NULL;

TLV2_API void registerREGWRITERSPHandler(REGWRITERSP_OP_FUNC fp)
{
    REGWRITERSPOpFunc = fp;
}

A_BOOL REGWRITERSPOp(void *pParms)
{
    CMD_REGWRITERSP_PARMS *pREGWRITERSPParms = (CMD_REGWRITERSP_PARMS *)pParms;
#if 0
//#ifdef _DEBUG

    A_PRINTF("REGWRITERSPOp: status %u\n", pREGWRITERSPParms->status);
    A_PRINTF("REGWRITERSPOp: radioId %u\n", pREGWRITERSPParms->radioId);
#endif //_DEBUG

    if (NULL != REGWRITERSPOpFunc) {
        (*REGWRITERSPOpFunc)(pREGWRITERSPParms);
    }
    return(TRUE);
}
