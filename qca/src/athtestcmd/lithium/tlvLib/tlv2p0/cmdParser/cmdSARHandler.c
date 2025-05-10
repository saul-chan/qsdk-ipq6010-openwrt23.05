/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdSARHandler.s
#include "tlv2Inc.h"
#include "cmdSARHandler.h"

void* initSAROpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_SAR_PARMS  *pSARParms = (CMD_SAR_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pSARParms->cmdId = pParmDict[PARM_CMDID].v.valU16;
    pSARParms->CCK2gLimit = pParmDict[PARM_CCK2GLIMIT].v.valU8;
    pSARParms->Ofdm2gLimit = pParmDict[PARM_OFDM2GLIMIT].v.valU8;
    pSARParms->Ofdm5gLimit = pParmDict[PARM_OFDM5GLIMIT].v.valU8;
    pSARParms->chain = pParmDict[PARM_CHAIN].v.valU8;
    pSARParms->index8 = pParmDict[PARM_INDEX8].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_CMDID, (size_t)(((size_t)&(pSARParms->cmdId)) - (size_t)pSARParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CCK2GLIMIT, (size_t)(((size_t)&(pSARParms->CCK2gLimit)) - (size_t)pSARParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_OFDM2GLIMIT, (size_t)(((size_t)&(pSARParms->Ofdm2gLimit)) - (size_t)pSARParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_OFDM5GLIMIT, (size_t)(((size_t)&(pSARParms->Ofdm5gLimit)) - (size_t)pSARParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAIN, (size_t)(((size_t)&(pSARParms->chain)) - (size_t)pSARParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_INDEX8, (size_t)(((size_t)&(pSARParms->index8)) - (size_t)pSARParms), pParmsOffset);
    return((void*) pSARParms);
}

static SAR_OP_FUNC SAROpFunc = NULL;

TLV2_API void registerSARHandler(SAR_OP_FUNC fp)
{
    SAROpFunc = fp;
}

A_BOOL SAROp(void *pParms)
{
    CMD_SAR_PARMS *pSARParms = (CMD_SAR_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("SAROp: cmdId %u\n", pSARParms->cmdId);
    A_PRINTF("SAROp: CCK2gLimit %u\n", pSARParms->CCK2gLimit);
    A_PRINTF("SAROp: Ofdm2gLimit %u\n", pSARParms->Ofdm2gLimit);
    A_PRINTF("SAROp: Ofdm5gLimit %u\n", pSARParms->Ofdm5gLimit);
    A_PRINTF("SAROp: chain %u\n", pSARParms->chain);
    A_PRINTF("SAROp: index8 %u\n", pSARParms->index8);
#endif //_DEBUG

    if (NULL != SAROpFunc) {
        (*SAROpFunc)(pSARParms);
    }
    return(TRUE);
}

void* initSARRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_SARRSP_PARMS  *pSARRSPParms = (CMD_SARRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pSARRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pSARRSPParms->CCK2gLimit = pParmDict[PARM_CCK2GLIMIT].v.valU8;
    pSARRSPParms->Ofdm2gLimit = pParmDict[PARM_OFDM2GLIMIT].v.valU8;
    pSARRSPParms->Ofdm5gLimit = pParmDict[PARM_OFDM5GLIMIT].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pSARRSPParms->status)) - (size_t)pSARRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CCK2GLIMIT, (size_t)(((size_t)&(pSARRSPParms->CCK2gLimit)) - (size_t)pSARRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_OFDM2GLIMIT, (size_t)(((size_t)&(pSARRSPParms->Ofdm2gLimit)) - (size_t)pSARRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_OFDM5GLIMIT, (size_t)(((size_t)&(pSARRSPParms->Ofdm5gLimit)) - (size_t)pSARRSPParms), pParmsOffset);
    return((void*) pSARRSPParms);
}

static SARRSP_OP_FUNC SARRSPOpFunc = NULL;

TLV2_API void registerSARRSPHandler(SARRSP_OP_FUNC fp)
{
    SARRSPOpFunc = fp;
}

A_BOOL SARRSPOp(void *pParms)
{
    CMD_SARRSP_PARMS *pSARRSPParms = (CMD_SARRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("SARRSPOp: status %u\n", pSARRSPParms->status);
    A_PRINTF("SARRSPOp: CCK2gLimit %u\n", pSARRSPParms->CCK2gLimit);
    A_PRINTF("SARRSPOp: Ofdm2gLimit %u\n", pSARRSPParms->Ofdm2gLimit);
    A_PRINTF("SARRSPOp: Ofdm5gLimit %u\n", pSARRSPParms->Ofdm5gLimit);
#endif //_DEBUG

    if (NULL != SARRSPOpFunc) {
        (*SARRSPOpFunc)(pSARRSPParms);
    }
    return(TRUE);
}
