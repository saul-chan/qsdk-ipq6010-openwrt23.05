/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdGetRssi.s
#include "tlv2Inc.h"
#include "cmdGetRssi.h"

void* initGETRSSIOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETRSSI_PARMS  *pGETRSSIParms = (CMD_GETRSSI_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGETRSSIParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pGETRSSIParms->chain = pParmDict[PARM_CHAIN].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGETRSSIParms->phyId)) - (size_t)pGETRSSIParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAIN, (size_t)(((size_t)&(pGETRSSIParms->chain)) - (size_t)pGETRSSIParms), pParmsOffset);
    return((void*) pGETRSSIParms);
}

static GETRSSI_OP_FUNC GETRSSIOpFunc = NULL;

TLV2_API void registerGETRSSIHandler(GETRSSI_OP_FUNC fp)
{
    GETRSSIOpFunc = fp;
}

A_BOOL GETRSSIOp(void *pParms)
{
    CMD_GETRSSI_PARMS *pGETRSSIParms = (CMD_GETRSSI_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GETRSSIOp: phyId %u\n", pGETRSSIParms->phyId);
    A_PRINTF("GETRSSIOp: chain %u\n", pGETRSSIParms->chain);
#endif //_DEBUG

    if (NULL != GETRSSIOpFunc) {
        (*GETRSSIOpFunc)(pGETRSSIParms);
    }
    return(TRUE);
}

void* initGETRSSIRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETRSSIRSP_PARMS  *pGETRSSIRSPParms = (CMD_GETRSSIRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGETRSSIRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pGETRSSIRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pGETRSSIRSPParms->rssi = pParmDict[PARM_RSSI].v.valS32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGETRSSIRSPParms->phyId)) - (size_t)pGETRSSIRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pGETRSSIRSPParms->status)) - (size_t)pGETRSSIRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RSSI, (size_t)(((size_t)&(pGETRSSIRSPParms->rssi)) - (size_t)pGETRSSIRSPParms), pParmsOffset);
    return((void*) pGETRSSIRSPParms);
}

static GETRSSIRSP_OP_FUNC GETRSSIRSPOpFunc = NULL;

TLV2_API void registerGETRSSIRSPHandler(GETRSSIRSP_OP_FUNC fp)
{
    GETRSSIRSPOpFunc = fp;
}

A_BOOL GETRSSIRSPOp(void *pParms)
{
    CMD_GETRSSIRSP_PARMS *pGETRSSIRSPParms = (CMD_GETRSSIRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GETRSSIRSPOp: phyId %u\n", pGETRSSIRSPParms->phyId);
    A_PRINTF("GETRSSIRSPOp: status %u\n", pGETRSSIRSPParms->status);
    A_PRINTF("GETRSSIRSPOp: rssi %d\n", pGETRSSIRSPParms->rssi);
#endif //_DEBUG

    if (NULL != GETRSSIRSPOpFunc) {
        (*GETRSSIRSPOpFunc)(pGETRSSIRSPParms);
    }
    return(TRUE);
}
