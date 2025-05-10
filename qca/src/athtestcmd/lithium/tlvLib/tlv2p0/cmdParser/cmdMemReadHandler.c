/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input\cmdMemReadHandler.s
#include "tlv2Inc.h"
#include "cmdMemReadHandler.h"

void* initMEMREADOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_MEMREAD_PARMS  *pMEMREADParms = (CMD_MEMREAD_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pMEMREADParms->memaddress = pParmDict[PARM_MEMADDRESS].v.valU32;
    pMEMREADParms->numbytes = pParmDict[PARM_NUMBYTES].v.valU16;
    pMEMREADParms->valuetype = pParmDict[PARM_VALUETYPE].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_MEMADDRESS, (size_t)(((size_t)&(pMEMREADParms->memaddress)) - (size_t)pMEMREADParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMBYTES, (size_t)(((size_t)&(pMEMREADParms->numbytes)) - (size_t)pMEMREADParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_VALUETYPE, (size_t)(((size_t)&(pMEMREADParms->valuetype)) - (size_t)pMEMREADParms), pParmsOffset);
    return((void*) pMEMREADParms);
}

static MEMREAD_OP_FUNC MEMREADOpFunc = NULL;

TLV2_API void registerMEMREADHandler(MEMREAD_OP_FUNC fp)
{
    MEMREADOpFunc = fp;
}

A_BOOL MEMREADOp(void *pParms)
{
    CMD_MEMREAD_PARMS *pMEMREADParms = (CMD_MEMREAD_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("MEMREADOp: memaddress 0x%x\n", pMEMREADParms->memaddress);
    A_PRINTF("MEMREADOp: numbytes %u\n", pMEMREADParms->numbytes);
    A_PRINTF("MEMREADOp: valuetype %u\n", pMEMREADParms->valuetype);
#endif //_DEBUG

    if (NULL != MEMREADOpFunc) {
        (*MEMREADOpFunc)(pMEMREADParms);
    }
    return(TRUE);
}

void* initMEMREADRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_MEMREADRSP_PARMS  *pMEMREADRSPParms = (CMD_MEMREADRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pMEMREADRSPParms->memaddress = pParmDict[PARM_MEMADDRESS].v.valU32;
    pMEMREADRSPParms->numbytes = pParmDict[PARM_NUMBYTES].v.valU16;
    pMEMREADRSPParms->valuetype = pParmDict[PARM_VALUETYPE].v.valU8;
    memset(pMEMREADRSPParms->memvalue, 0, sizeof(pMEMREADRSPParms->memvalue));
    pMEMREADRSPParms->status = pParmDict[PARM_STATUS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_MEMADDRESS, (size_t)(((size_t)&(pMEMREADRSPParms->memaddress)) - (size_t)pMEMREADRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMBYTES, (size_t)(((size_t)&(pMEMREADRSPParms->numbytes)) - (size_t)pMEMREADRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_VALUETYPE, (size_t)(((size_t)&(pMEMREADRSPParms->valuetype)) - (size_t)pMEMREADRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MEMVALUE, (size_t)(((size_t)&(pMEMREADRSPParms->memvalue)) - (size_t)pMEMREADRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pMEMREADRSPParms->status)) - (size_t)pMEMREADRSPParms), pParmsOffset);
    return((void*) pMEMREADRSPParms);
}

static MEMREADRSP_OP_FUNC MEMREADRSPOpFunc = NULL;

TLV2_API void registerMEMREADRSPHandler(MEMREADRSP_OP_FUNC fp)
{
    MEMREADRSPOpFunc = fp;
}

A_BOOL MEMREADRSPOp(void *pParms)
{
    CMD_MEMREADRSP_PARMS *pMEMREADRSPParms = (CMD_MEMREADRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("MEMREADRSPOp: memaddress 0x%x\n", pMEMREADRSPParms->memaddress);
    A_PRINTF("MEMREADRSPOp: numbytes %u\n", pMEMREADRSPParms->numbytes);
    A_PRINTF("MEMREADRSPOp: valuetype %u\n", pMEMREADRSPParms->valuetype);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 256 entries
    {
        A_PRINTF("MEMREADRSPOp: memvalue 0x%x\n", pMEMREADRSPParms->memvalue[i]);
    }
    A_PRINTF("MEMREADRSPOp: status %u\n", pMEMREADRSPParms->status);
#endif //_DEBUG

    if (NULL != MEMREADRSPOpFunc) {
        (*MEMREADRSPOpFunc)(pMEMREADRSPParms);
    }
    return(TRUE);
}
