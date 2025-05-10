/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdSetPCIEConfigParam.s
#include "tlv2Inc.h"
#include "cmdSetPCIEConfigParam.h"

void* initSETPCIECONFIGPARAMSOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_SETPCIECONFIGPARAMS_PARMS  *pSETPCIECONFIGPARAMSParms = (CMD_SETPCIECONFIGPARAMS_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pSETPCIECONFIGPARAMSParms->pcieAddr = pParmDict[PARM_PCIEADDR].v.valU32;
    pSETPCIECONFIGPARAMSParms->pcieValue = pParmDict[PARM_PCIEVALUE].v.valU32;
    pSETPCIECONFIGPARAMSParms->operation = pParmDict[PARM_OPERATION].v.valU8;
    pSETPCIECONFIGPARAMSParms->size = pParmDict[PARM_SIZE].v.valU32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PCIEADDR, (size_t)(((size_t)&(pSETPCIECONFIGPARAMSParms->pcieAddr)) - (size_t)pSETPCIECONFIGPARAMSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PCIEVALUE, (size_t)(((size_t)&(pSETPCIECONFIGPARAMSParms->pcieValue)) - (size_t)pSETPCIECONFIGPARAMSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_OPERATION, (size_t)(((size_t)&(pSETPCIECONFIGPARAMSParms->operation)) - (size_t)pSETPCIECONFIGPARAMSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SIZE, (size_t)(((size_t)&(pSETPCIECONFIGPARAMSParms->size)) - (size_t)pSETPCIECONFIGPARAMSParms), pParmsOffset);
    return((void*) pSETPCIECONFIGPARAMSParms);
}

static SETPCIECONFIGPARAMS_OP_FUNC SETPCIECONFIGPARAMSOpFunc = NULL;

TLV2_API void registerSETPCIECONFIGPARAMSHandler(SETPCIECONFIGPARAMS_OP_FUNC fp)
{
    SETPCIECONFIGPARAMSOpFunc = fp;
}

A_BOOL SETPCIECONFIGPARAMSOp(void *pParms)
{
    CMD_SETPCIECONFIGPARAMS_PARMS *pSETPCIECONFIGPARAMSParms = (CMD_SETPCIECONFIGPARAMS_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("SETPCIECONFIGPARAMSOp: pcieAddr %u\n", pSETPCIECONFIGPARAMSParms->pcieAddr);
    A_PRINTF("SETPCIECONFIGPARAMSOp: pcieValue %u\n", pSETPCIECONFIGPARAMSParms->pcieValue);
    A_PRINTF("SETPCIECONFIGPARAMSOp: operation %u\n", pSETPCIECONFIGPARAMSParms->operation);
    A_PRINTF("SETPCIECONFIGPARAMSOp: size %u\n", pSETPCIECONFIGPARAMSParms->size);
#endif //_DEBUG

    if (NULL != SETPCIECONFIGPARAMSOpFunc) {
        (*SETPCIECONFIGPARAMSOpFunc)(pSETPCIECONFIGPARAMSParms);
    }
    return(TRUE);
}

void* initSETPCIECONFIGPARAMSRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    return(NULL);
}

static SETPCIECONFIGPARAMSRSP_OP_FUNC SETPCIECONFIGPARAMSRSPOpFunc = NULL;

TLV2_API void registerSETPCIECONFIGPARAMSRSPHandler(SETPCIECONFIGPARAMSRSP_OP_FUNC fp)
{
    SETPCIECONFIGPARAMSRSPOpFunc = fp;
}

A_BOOL SETPCIECONFIGPARAMSRSPOp(void *pParms)
{
    if (NULL != SETPCIECONFIGPARAMSRSPOpFunc) {
        (*SETPCIECONFIGPARAMSRSPOpFunc)(NULL);
    }
    return(TRUE);
}
