/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdTPCGetPDADC.s
#include "tlv2Inc.h"
#include "cmdTPCGetPDADC.h"

void* initGETTPC_PDADCOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETTPC_PDADC_PARMS  *pGETTPC_PDADCParms = (CMD_GETTPC_PDADC_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGETTPC_PDADCParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pGETTPC_PDADCParms->chainIdx = pParmDict[PARM_CHAINIDX].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGETTPC_PDADCParms->phyId)) - (size_t)pGETTPC_PDADCParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINIDX, (size_t)(((size_t)&(pGETTPC_PDADCParms->chainIdx)) - (size_t)pGETTPC_PDADCParms), pParmsOffset);
    return((void*) pGETTPC_PDADCParms);
}

static GETTPC_PDADC_OP_FUNC GETTPC_PDADCOpFunc = NULL;

TLV2_API void registerGETTPC_PDADCHandler(GETTPC_PDADC_OP_FUNC fp)
{
    GETTPC_PDADCOpFunc = fp;
}

A_BOOL GETTPC_PDADCOp(void *pParms)
{
    CMD_GETTPC_PDADC_PARMS *pGETTPC_PDADCParms = (CMD_GETTPC_PDADC_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GETTPC_PDADCOp: phyId %u\n", pGETTPC_PDADCParms->phyId);
    A_PRINTF("GETTPC_PDADCOp: chainIdx %u\n", pGETTPC_PDADCParms->chainIdx);
#endif //_DEBUG

    if (NULL != GETTPC_PDADCOpFunc) {
        (*GETTPC_PDADCOpFunc)(pGETTPC_PDADCParms);
    }
    return(TRUE);
}

void* initGETTPC_PDADCRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETTPC_PDADCRSP_PARMS  *pGETTPC_PDADCRSPParms = (CMD_GETTPC_PDADCRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGETTPC_PDADCRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pGETTPC_PDADCRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pGETTPC_PDADCRSPParms->tpcpdadc = pParmDict[PARM_TPCPDADC].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGETTPC_PDADCRSPParms->phyId)) - (size_t)pGETTPC_PDADCRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pGETTPC_PDADCRSPParms->status)) - (size_t)pGETTPC_PDADCRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TPCPDADC, (size_t)(((size_t)&(pGETTPC_PDADCRSPParms->tpcpdadc)) - (size_t)pGETTPC_PDADCRSPParms), pParmsOffset);
    return((void*) pGETTPC_PDADCRSPParms);
}

static GETTPC_PDADCRSP_OP_FUNC GETTPC_PDADCRSPOpFunc = NULL;

TLV2_API void registerGETTPC_PDADCRSPHandler(GETTPC_PDADCRSP_OP_FUNC fp)
{
    GETTPC_PDADCRSPOpFunc = fp;
}

A_BOOL GETTPC_PDADCRSPOp(void *pParms)
{
    CMD_GETTPC_PDADCRSP_PARMS *pGETTPC_PDADCRSPParms = (CMD_GETTPC_PDADCRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GETTPC_PDADCRSPOp: phyId %u\n", pGETTPC_PDADCRSPParms->phyId);
    A_PRINTF("GETTPC_PDADCRSPOp: status %u\n", pGETTPC_PDADCRSPParms->status);
    A_PRINTF("GETTPC_PDADCRSPOp: tpcpdadc %u\n", pGETTPC_PDADCRSPParms->tpcpdadc);
#endif //_DEBUG

    if (NULL != GETTPC_PDADCRSPOpFunc) {
        (*GETTPC_PDADCRSPOpFunc)(pGETTPC_PDADCRSPParms);
    }
    return(TRUE);
}
