/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input\cmdMemWriteHandler.s
#include "tlv2Inc.h"
#include "cmdMemWriteHandler.h"

void* initMEMWRITEOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_MEMWRITE_PARMS  *pMEMWRITEParms = (CMD_MEMWRITE_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pMEMWRITEParms->memaddress = pParmDict[PARM_MEMADDRESS].v.valU32;
    pMEMWRITEParms->numbytes = pParmDict[PARM_NUMBYTES].v.valU16;
    pMEMWRITEParms->valuetype = pParmDict[PARM_VALUETYPE].v.valU8;
    memset(pMEMWRITEParms->memvalue, 0, sizeof(pMEMWRITEParms->memvalue));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_MEMADDRESS, (size_t)(((size_t)&(pMEMWRITEParms->memaddress)) - (size_t)pMEMWRITEParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMBYTES, (size_t)(((size_t)&(pMEMWRITEParms->numbytes)) - (size_t)pMEMWRITEParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_VALUETYPE, (size_t)(((size_t)&(pMEMWRITEParms->valuetype)) - (size_t)pMEMWRITEParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MEMVALUE, (size_t)(((size_t)&(pMEMWRITEParms->memvalue)) - (size_t)pMEMWRITEParms), pParmsOffset);
    return((void*) pMEMWRITEParms);
}

static MEMWRITE_OP_FUNC MEMWRITEOpFunc = NULL;

TLV2_API void registerMEMWRITEHandler(MEMWRITE_OP_FUNC fp)
{
    MEMWRITEOpFunc = fp;
}

A_BOOL MEMWRITEOp(void *pParms)
{
    CMD_MEMWRITE_PARMS *pMEMWRITEParms = (CMD_MEMWRITE_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("MEMWRITEOp: memaddress 0x%x\n", pMEMWRITEParms->memaddress);
    A_PRINTF("MEMWRITEOp: numbytes %u\n", pMEMWRITEParms->numbytes);
    A_PRINTF("MEMWRITEOp: valuetype %u\n", pMEMWRITEParms->valuetype);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 256 entries
    {
        A_PRINTF("MEMWRITEOp: memvalue 0x%x\n", pMEMWRITEParms->memvalue[i]);
    }
#endif //_DEBUG

    if (NULL != MEMWRITEOpFunc) {
        (*MEMWRITEOpFunc)(pMEMWRITEParms);
    }
    return(TRUE);
}

void* initMEMWRITERSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_MEMWRITERSP_PARMS  *pMEMWRITERSPParms = (CMD_MEMWRITERSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pMEMWRITERSPParms->memaddress = pParmDict[PARM_MEMADDRESS].v.valU32;
    pMEMWRITERSPParms->numbytes = pParmDict[PARM_NUMBYTES].v.valU16;
    pMEMWRITERSPParms->valuetype = pParmDict[PARM_VALUETYPE].v.valU8;
    pMEMWRITERSPParms->status = pParmDict[PARM_STATUS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_MEMADDRESS, (size_t)(((size_t)&(pMEMWRITERSPParms->memaddress)) - (size_t)pMEMWRITERSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMBYTES, (size_t)(((size_t)&(pMEMWRITERSPParms->numbytes)) - (size_t)pMEMWRITERSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_VALUETYPE, (size_t)(((size_t)&(pMEMWRITERSPParms->valuetype)) - (size_t)pMEMWRITERSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pMEMWRITERSPParms->status)) - (size_t)pMEMWRITERSPParms), pParmsOffset);
    return((void*) pMEMWRITERSPParms);
}

static MEMWRITERSP_OP_FUNC MEMWRITERSPOpFunc = NULL;

TLV2_API void registerMEMWRITERSPHandler(MEMWRITERSP_OP_FUNC fp)
{
    MEMWRITERSPOpFunc = fp;
}

A_BOOL MEMWRITERSPOp(void *pParms)
{
    CMD_MEMWRITERSP_PARMS *pMEMWRITERSPParms = (CMD_MEMWRITERSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("MEMWRITERSPOp: memaddress 0x%x\n", pMEMWRITERSPParms->memaddress);
    A_PRINTF("MEMWRITERSPOp: numbytes %u\n", pMEMWRITERSPParms->numbytes);
    A_PRINTF("MEMWRITERSPOp: valuetype %u\n", pMEMWRITERSPParms->valuetype);
    A_PRINTF("MEMWRITERSPOp: status %u\n", pMEMWRITERSPParms->status);
#endif //_DEBUG

    if (NULL != MEMWRITERSPOpFunc) {
        (*MEMWRITERSPOpFunc)(pMEMWRITERSPParms);
    }
    return(TRUE);
}
