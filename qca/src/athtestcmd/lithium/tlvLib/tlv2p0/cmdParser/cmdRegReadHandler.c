/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input\cmdRegReadHandler.s
#include "tlv2Inc.h"
#include "cmdRegReadHandler.h"

void* initREGREADOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_REGREAD_PARMS  *pREGREADParms = (CMD_REGREAD_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pREGREADParms->regaddress = pParmDict[PARM_REGADDRESS].v.valU32;
    pREGREADParms->radioId = pParmDict[PARM_RADIOID].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_REGADDRESS, (size_t)(((size_t)&(pREGREADParms->regaddress)) - (size_t)pREGREADParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RADIOID, (size_t)(((size_t)&(pREGREADParms->radioId)) - (size_t)pREGREADParms), pParmsOffset);
    return((void*) pREGREADParms);
}

static REGREAD_OP_FUNC REGREADOpFunc = NULL;

TLV2_API void registerREGREADHandler(REGREAD_OP_FUNC fp)
{
    REGREADOpFunc = fp;
}

A_BOOL REGREADOp(void *pParms)
{
    CMD_REGREAD_PARMS *pREGREADParms = (CMD_REGREAD_PARMS *)pParms;
#if 0
//#ifdef _DEBUG

    A_PRINTF("REGREADOp: regaddress 0x%x\n", pREGREADParms->regaddress);
    A_PRINTF("REGREADOp: radioId %u\n", pREGREADParms->radioId);
#endif //_DEBUG

    if (NULL != REGREADOpFunc) {
        (*REGREADOpFunc)(pREGREADParms);
    }
    return(TRUE);
}

void* initREGREADRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_REGREADRSP_PARMS  *pREGREADRSPParms = (CMD_REGREADRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pREGREADRSPParms->regvalue = pParmDict[PARM_REGVALUE].v.valU32;
    pREGREADRSPParms->radioId = pParmDict[PARM_RADIOID].v.valU8;
    pREGREADRSPParms->status = pParmDict[PARM_STATUS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_REGVALUE, (size_t)(((size_t)&(pREGREADRSPParms->regvalue)) - (size_t)pREGREADRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RADIOID, (size_t)(((size_t)&(pREGREADRSPParms->radioId)) - (size_t)pREGREADRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pREGREADRSPParms->status)) - (size_t)pREGREADRSPParms), pParmsOffset);
    return((void*) pREGREADRSPParms);
}

static REGREADRSP_OP_FUNC REGREADRSPOpFunc = NULL;

TLV2_API void registerREGREADRSPHandler(REGREADRSP_OP_FUNC fp)
{
    REGREADRSPOpFunc = fp;
}

A_BOOL REGREADRSPOp(void *pParms)
{
    CMD_REGREADRSP_PARMS *pREGREADRSPParms = (CMD_REGREADRSP_PARMS *)pParms;
#if 0
//#ifdef _DEBUG

    A_PRINTF("REGREADRSPOp: regvalue 0x%x\n", pREGREADRSPParms->regvalue);
    A_PRINTF("REGREADRSPOp: radioId %u\n", pREGREADRSPParms->radioId);
    A_PRINTF("REGREADRSPOp: status %u\n", pREGREADRSPParms->status);
#endif //_DEBUG

    if (NULL != REGREADRSPOpFunc) {
        (*REGREADRSPOpFunc)(pREGREADRSPParms);
    }
    return(TRUE);
}
