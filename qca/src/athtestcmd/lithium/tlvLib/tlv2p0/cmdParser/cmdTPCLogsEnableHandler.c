/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input\cmdTPCLogsEnableHandler.s
#include "tlv2Inc.h"
#include "cmdTPCLogsEnableHandler.h"

void* initTPCLOGSENABLEOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_TPCLOGSENABLE_PARMS  *pTPCLOGSENABLEParms = (CMD_TPCLOGSENABLE_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pTPCLOGSENABLEParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pTPCLOGSENABLEParms->enable = pParmDict[PARM_ENABLE].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pTPCLOGSENABLEParms->phyId)) - (size_t)pTPCLOGSENABLEParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENABLE, (size_t)(((size_t)&(pTPCLOGSENABLEParms->enable)) - (size_t)pTPCLOGSENABLEParms), pParmsOffset);
    return((void*) pTPCLOGSENABLEParms);
}

static TPCLOGSENABLE_OP_FUNC TPCLOGSENABLEOpFunc = NULL;

TLV2_API void registerTPCLOGSENABLEHandler(TPCLOGSENABLE_OP_FUNC fp)
{
    TPCLOGSENABLEOpFunc = fp;
}

A_BOOL TPCLOGSENABLEOp(void *pParms)
{
    CMD_TPCLOGSENABLE_PARMS *pTPCLOGSENABLEParms = (CMD_TPCLOGSENABLE_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TPCLOGSENABLEOp: phyId %u\n", pTPCLOGSENABLEParms->phyId);
    A_PRINTF("TPCLOGSENABLEOp: enable %u\n", pTPCLOGSENABLEParms->enable);
#endif //_DEBUG

    if (NULL != TPCLOGSENABLEOpFunc) {
        (*TPCLOGSENABLEOpFunc)(pTPCLOGSENABLEParms);
    }
    return(TRUE);
}

void* initTPCLOGSENABLERSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_TPCLOGSENABLERSP_PARMS  *pTPCLOGSENABLERSPParms = (CMD_TPCLOGSENABLERSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pTPCLOGSENABLERSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pTPCLOGSENABLERSPParms->status = pParmDict[PARM_STATUS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pTPCLOGSENABLERSPParms->phyId)) - (size_t)pTPCLOGSENABLERSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pTPCLOGSENABLERSPParms->status)) - (size_t)pTPCLOGSENABLERSPParms), pParmsOffset);
    return((void*) pTPCLOGSENABLERSPParms);
}

static TPCLOGSENABLERSP_OP_FUNC TPCLOGSENABLERSPOpFunc = NULL;

TLV2_API void registerTPCLOGSENABLERSPHandler(TPCLOGSENABLERSP_OP_FUNC fp)
{
    TPCLOGSENABLERSPOpFunc = fp;
}

A_BOOL TPCLOGSENABLERSPOp(void *pParms)
{
    CMD_TPCLOGSENABLERSP_PARMS *pTPCLOGSENABLERSPParms = (CMD_TPCLOGSENABLERSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TPCLOGSENABLERSPOp: phyId %u\n", pTPCLOGSENABLERSPParms->phyId);
    A_PRINTF("TPCLOGSENABLERSPOp: status %u\n", pTPCLOGSENABLERSPParms->status);
#endif //_DEBUG

    if (NULL != TPCLOGSENABLERSPOpFunc) {
        (*TPCLOGSENABLERSPOpFunc)(pTPCLOGSENABLERSPParms);
    }
    return(TRUE);
}
