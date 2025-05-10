/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdGetLUTTablesHandler.s
#include "tlv2Inc.h"
#include "cmdGetLUTTablesHandler.h"

void* initGETLUTTABLESOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETLUTTABLES_PARMS  *pGETLUTTABLESParms = (CMD_GETLUTTABLES_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGETLUTTABLESParms->LUTdumpOption = pParmDict[PARM_LUTDUMPOPTION].v.valU8;
    pGETLUTTABLESParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pGETLUTTABLESParms->chainMask = pParmDict[PARM_CHAINMASK].v.valU16;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_LUTDUMPOPTION, (size_t)(((size_t)&(pGETLUTTABLESParms->LUTdumpOption)) - (size_t)pGETLUTTABLESParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGETLUTTABLESParms->phyId)) - (size_t)pGETLUTTABLESParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINMASK, (size_t)(((size_t)&(pGETLUTTABLESParms->chainMask)) - (size_t)pGETLUTTABLESParms), pParmsOffset);
    return((void*) pGETLUTTABLESParms);
}

static GETLUTTABLES_OP_FUNC GETLUTTABLESOpFunc = NULL;

TLV2_API void registerGETLUTTABLESHandler(GETLUTTABLES_OP_FUNC fp)
{
    GETLUTTABLESOpFunc = fp;
}

A_BOOL GETLUTTABLESOp(void *pParms)
{
    CMD_GETLUTTABLES_PARMS *pGETLUTTABLESParms = (CMD_GETLUTTABLES_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GETLUTTABLESOp: LUTdumpOption %u\n", pGETLUTTABLESParms->LUTdumpOption);
    A_PRINTF("GETLUTTABLESOp: phyId %u\n", pGETLUTTABLESParms->phyId);
    A_PRINTF("GETLUTTABLESOp: chainMask 0x%x\n", pGETLUTTABLESParms->chainMask);
#endif //_DEBUG

    if (NULL != GETLUTTABLESOpFunc) {
        (*GETLUTTABLESOpFunc)(pGETLUTTABLESParms);
    }
    return(TRUE);
}

void* initGETLUTTABLESRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETLUTTABLESRSP_PARMS  *pGETLUTTABLESRSPParms = (CMD_GETLUTTABLESRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    memset(pGETLUTTABLESRSPParms->data4k, 0, sizeof(pGETLUTTABLESRSPParms->data4k));
    pGETLUTTABLESRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pGETLUTTABLESRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pGETLUTTABLESRSPParms->rspDataLen = pParmDict[PARM_RSPDATALEN].v.valU16;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_DATA4K, (size_t)(((size_t)&(pGETLUTTABLESRSPParms->data4k)) - (size_t)pGETLUTTABLESRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pGETLUTTABLESRSPParms->status)) - (size_t)pGETLUTTABLESRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGETLUTTABLESRSPParms->phyId)) - (size_t)pGETLUTTABLESRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RSPDATALEN, (size_t)(((size_t)&(pGETLUTTABLESRSPParms->rspDataLen)) - (size_t)pGETLUTTABLESRSPParms), pParmsOffset);
    return((void*) pGETLUTTABLESRSPParms);
}

static GETLUTTABLESRSP_OP_FUNC GETLUTTABLESRSPOpFunc = NULL;

TLV2_API void registerGETLUTTABLESRSPHandler(GETLUTTABLESRSP_OP_FUNC fp)
{
    GETLUTTABLESRSPOpFunc = fp;
}

A_BOOL GETLUTTABLESRSPOp(void *pParms)
{
    CMD_GETLUTTABLESRSP_PARMS *pGETLUTTABLESRSPParms = (CMD_GETLUTTABLESRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    for (i = 0; i < 8 ; i++) // can be modified to print up to 4096 entries
    {
        A_PRINTF("GETLUTTABLESRSPOp: data4k 0x%x\n", pGETLUTTABLESRSPParms->data4k[i]);
    }
    A_PRINTF("GETLUTTABLESRSPOp: status %u\n", pGETLUTTABLESRSPParms->status);
    A_PRINTF("GETLUTTABLESRSPOp: phyId %u\n", pGETLUTTABLESRSPParms->phyId);
    A_PRINTF("GETLUTTABLESRSPOp: rspDataLen %u\n", pGETLUTTABLESRSPParms->rspDataLen);
#endif //_DEBUG

    if (NULL != GETLUTTABLESRSPOpFunc) {
        (*GETLUTTABLESRSPOpFunc)(pGETLUTTABLESRSPParms);
    }
    return(TRUE);
}
