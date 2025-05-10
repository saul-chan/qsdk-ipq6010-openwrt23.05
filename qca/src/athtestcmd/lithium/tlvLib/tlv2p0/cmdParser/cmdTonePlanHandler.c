/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdTonePlanHandler.s
#include "tlv2Inc.h"
#include "cmdTonePlanHandler.h"

void* initTONEPLANOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_TONEPLAN_PARMS  *pTONEPLANParms = (CMD_TONEPLAN_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pTONEPLANParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pTONEPLANParms->bandwidth = pParmDict[PARM_BANDWIDTH].v.valU8;
    pTONEPLANParms->version = pParmDict[PARM_VERSION].v.valU8;
    pTONEPLANParms->pad1 = pParmDict[PARM_PAD1].v.valU8;
    pTONEPLANParms->userEntries = pParmDict[PARM_USERENTRIES].v.valU32;
    memset(pTONEPLANParms->dataRates, 0, sizeof(pTONEPLANParms->dataRates));
    memset(pTONEPLANParms->powerdBm, 0, sizeof(pTONEPLANParms->powerdBm));
    memset(pTONEPLANParms->fec, 0, sizeof(pTONEPLANParms->fec));
    memset(pTONEPLANParms->dcm, 0, sizeof(pTONEPLANParms->dcm));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pTONEPLANParms->phyId)) - (size_t)pTONEPLANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BANDWIDTH, (size_t)(((size_t)&(pTONEPLANParms->bandwidth)) - (size_t)pTONEPLANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_VERSION, (size_t)(((size_t)&(pTONEPLANParms->version)) - (size_t)pTONEPLANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD1, (size_t)(((size_t)&(pTONEPLANParms->pad1)) - (size_t)pTONEPLANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_USERENTRIES, (size_t)(((size_t)&(pTONEPLANParms->userEntries)) - (size_t)pTONEPLANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DATARATES, (size_t)(((size_t)&(pTONEPLANParms->dataRates)) - (size_t)pTONEPLANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_POWERDBM, (size_t)(((size_t)&(pTONEPLANParms->powerdBm)) - (size_t)pTONEPLANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FEC, (size_t)(((size_t)&(pTONEPLANParms->fec)) - (size_t)pTONEPLANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DCM, (size_t)(((size_t)&(pTONEPLANParms->dcm)) - (size_t)pTONEPLANParms), pParmsOffset);
    return((void*) pTONEPLANParms);
}

static TONEPLAN_OP_FUNC TONEPLANOpFunc = NULL;

TLV2_API void registerTONEPLANHandler(TONEPLAN_OP_FUNC fp)
{
    TONEPLANOpFunc = fp;
}

A_BOOL TONEPLANOp(void *pParms)
{
    CMD_TONEPLAN_PARMS *pTONEPLANParms = (CMD_TONEPLAN_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TONEPLANOp: phyId %u\n", pTONEPLANParms->phyId);
    A_PRINTF("TONEPLANOp: bandwidth %u\n", pTONEPLANParms->bandwidth);
    A_PRINTF("TONEPLANOp: version %u\n", pTONEPLANParms->version);
    A_PRINTF("TONEPLANOp: pad1 %u\n", pTONEPLANParms->pad1);
    A_PRINTF("TONEPLANOp: userEntries %u\n", pTONEPLANParms->userEntries);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 72 entries
    {
        A_PRINTF("TONEPLANOp: dataRates %u\n", pTONEPLANParms->dataRates[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 72 entries
    {
        A_PRINTF("TONEPLANOp: powerdBm %u\n", pTONEPLANParms->powerdBm[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 72 entries
    {
        A_PRINTF("TONEPLANOp: fec %u\n", pTONEPLANParms->fec[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 72 entries
    {
        A_PRINTF("TONEPLANOp: dcm %u\n", pTONEPLANParms->dcm[i]);
    }
#endif //_DEBUG

    if (NULL != TONEPLANOpFunc) {
        (*TONEPLANOpFunc)(pTONEPLANParms);
    }
    return(TRUE);
}
