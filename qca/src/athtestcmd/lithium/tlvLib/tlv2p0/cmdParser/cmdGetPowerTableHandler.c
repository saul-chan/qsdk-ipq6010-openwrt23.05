/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdGetPowerTableHandler.s
#include "tlv2Inc.h"
#include "cmdGetPowerTableHandler.h"

void* initGETPOWERTABLEOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETPOWERTABLE_PARMS  *pGETPOWERTABLEParms = (CMD_GETPOWERTABLE_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGETPOWERTABLEParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pGETPOWERTABLEParms->chainMask = pParmDict[PARM_CHAINMASK].v.valU16;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGETPOWERTABLEParms->phyId)) - (size_t)pGETPOWERTABLEParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINMASK, (size_t)(((size_t)&(pGETPOWERTABLEParms->chainMask)) - (size_t)pGETPOWERTABLEParms), pParmsOffset);
    return((void*) pGETPOWERTABLEParms);
}

static GETPOWERTABLE_OP_FUNC GETPOWERTABLEOpFunc = NULL;

TLV2_API void registerGETPOWERTABLEHandler(GETPOWERTABLE_OP_FUNC fp)
{
    GETPOWERTABLEOpFunc = fp;
}

A_BOOL GETPOWERTABLEOp(void *pParms)
{
    CMD_GETPOWERTABLE_PARMS *pGETPOWERTABLEParms = (CMD_GETPOWERTABLE_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GETPOWERTABLEOp: phyId %u\n", pGETPOWERTABLEParms->phyId);
    A_PRINTF("GETPOWERTABLEOp: chainMask 0x%x\n", pGETPOWERTABLEParms->chainMask);
#endif //_DEBUG

    if (NULL != GETPOWERTABLEOpFunc) {
        (*GETPOWERTABLEOpFunc)(pGETPOWERTABLEParms);
    }
    return(TRUE);
}

void* initGETPOWERTABLERSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETPOWERTABLERSP_PARMS  *pGETPOWERTABLERSPParms = (CMD_GETPOWERTABLERSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    memset(pGETPOWERTABLERSPParms->data4k, 0, sizeof(pGETPOWERTABLERSPParms->data4k));
    pGETPOWERTABLERSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pGETPOWERTABLERSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pGETPOWERTABLERSPParms->rspDataLen = pParmDict[PARM_RSPDATALEN].v.valU16;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_DATA4K, (size_t)(((size_t)&(pGETPOWERTABLERSPParms->data4k)) - (size_t)pGETPOWERTABLERSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pGETPOWERTABLERSPParms->status)) - (size_t)pGETPOWERTABLERSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGETPOWERTABLERSPParms->phyId)) - (size_t)pGETPOWERTABLERSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RSPDATALEN, (size_t)(((size_t)&(pGETPOWERTABLERSPParms->rspDataLen)) - (size_t)pGETPOWERTABLERSPParms), pParmsOffset);
    return((void*) pGETPOWERTABLERSPParms);
}

static GETPOWERTABLERSP_OP_FUNC GETPOWERTABLERSPOpFunc = NULL;

TLV2_API void registerGETPOWERTABLERSPHandler(GETPOWERTABLERSP_OP_FUNC fp)
{
    GETPOWERTABLERSPOpFunc = fp;
}

A_BOOL GETPOWERTABLERSPOp(void *pParms)
{
    CMD_GETPOWERTABLERSP_PARMS *pGETPOWERTABLERSPParms = (CMD_GETPOWERTABLERSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    for (i = 0; i < 8 ; i++) // can be modified to print up to 4096 entries
    {
        A_PRINTF("GETPOWERTABLERSPOp: data4k 0x%x\n", pGETPOWERTABLERSPParms->data4k[i]);
    }
    A_PRINTF("GETPOWERTABLERSPOp: status %u\n", pGETPOWERTABLERSPParms->status);
    A_PRINTF("GETPOWERTABLERSPOp: phyId %u\n", pGETPOWERTABLERSPParms->phyId);
    A_PRINTF("GETPOWERTABLERSPOp: rspDataLen %u\n", pGETPOWERTABLERSPParms->rspDataLen);
#endif //_DEBUG

    if (NULL != GETPOWERTABLERSPOpFunc) {
        (*GETPOWERTABLERSPOpFunc)(pGETPOWERTABLERSPParms);
    }
    return(TRUE);
}
