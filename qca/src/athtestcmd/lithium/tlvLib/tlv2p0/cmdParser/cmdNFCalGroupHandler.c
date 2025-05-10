/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input\cmdNFCalGroupHandler.s
#include "tlv2Inc.h"
#include "cmdNFCalGroupHandler.h"

void* initNFCALGROUPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_NFCALGROUP_PARMS  *pNFCALGROUPParms = (CMD_NFCALGROUP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pNFCALGROUPParms->cmdIdNFCalGroup = pParmDict[PARM_CMDIDNFCALGROUP].v.valU8;
    pNFCALGROUPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pNFCALGROUPParms->bandCode = pParmDict[PARM_BANDCODE].v.valU8;
    pNFCALGROUPParms->bwCode = pParmDict[PARM_BWCODE].v.valU8;
    pNFCALGROUPParms->freq = pParmDict[PARM_FREQ].v.valU16;
    pNFCALGROUPParms->txChMask = pParmDict[PARM_TXCHMASK].v.valU16;
    pNFCALGROUPParms->rxChMask = pParmDict[PARM_RXCHMASK].v.valU16;
    pNFCALGROUPParms->concMode = pParmDict[PARM_CONCMODE].v.valU8;
    pNFCALGROUPParms->pad1 = pParmDict[PARM_PAD1].v.valU8;
    pNFCALGROUPParms->userParm1 = pParmDict[PARM_USERPARM1].v.valU32;
    pNFCALGROUPParms->userParm2 = pParmDict[PARM_USERPARM2].v.valU32;
    memset(pNFCALGROUPParms->nfCalResults, 0, sizeof(pNFCALGROUPParms->nfCalResults));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_CMDIDNFCALGROUP, (size_t)(((size_t)&(pNFCALGROUPParms->cmdIdNFCalGroup)) - (size_t)pNFCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pNFCALGROUPParms->phyId)) - (size_t)pNFCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BANDCODE, (size_t)(((size_t)&(pNFCALGROUPParms->bandCode)) - (size_t)pNFCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BWCODE, (size_t)(((size_t)&(pNFCALGROUPParms->bwCode)) - (size_t)pNFCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ, (size_t)(((size_t)&(pNFCALGROUPParms->freq)) - (size_t)pNFCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXCHMASK, (size_t)(((size_t)&(pNFCALGROUPParms->txChMask)) - (size_t)pNFCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXCHMASK, (size_t)(((size_t)&(pNFCALGROUPParms->rxChMask)) - (size_t)pNFCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CONCMODE, (size_t)(((size_t)&(pNFCALGROUPParms->concMode)) - (size_t)pNFCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD1, (size_t)(((size_t)&(pNFCALGROUPParms->pad1)) - (size_t)pNFCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_USERPARM1, (size_t)(((size_t)&(pNFCALGROUPParms->userParm1)) - (size_t)pNFCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_USERPARM2, (size_t)(((size_t)&(pNFCALGROUPParms->userParm2)) - (size_t)pNFCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NFCALRESULTS, (size_t)(((size_t)&(pNFCALGROUPParms->nfCalResults)) - (size_t)pNFCALGROUPParms), pParmsOffset);
    return((void*) pNFCALGROUPParms);
}

static NFCALGROUP_OP_FUNC NFCALGROUPOpFunc = NULL;

TLV2_API void registerNFCALGROUPHandler(NFCALGROUP_OP_FUNC fp)
{
    NFCALGROUPOpFunc = fp;
}

A_BOOL NFCALGROUPOp(void *pParms)
{
    CMD_NFCALGROUP_PARMS *pNFCALGROUPParms = (CMD_NFCALGROUP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("NFCALGROUPOp: cmdIdNFCalGroup %u\n", pNFCALGROUPParms->cmdIdNFCalGroup);
    A_PRINTF("NFCALGROUPOp: phyId %u\n", pNFCALGROUPParms->phyId);
    A_PRINTF("NFCALGROUPOp: bandCode %u\n", pNFCALGROUPParms->bandCode);
    A_PRINTF("NFCALGROUPOp: bwCode %u\n", pNFCALGROUPParms->bwCode);
    A_PRINTF("NFCALGROUPOp: freq %u\n", pNFCALGROUPParms->freq);
    A_PRINTF("NFCALGROUPOp: txChMask 0x%x\n", pNFCALGROUPParms->txChMask);
    A_PRINTF("NFCALGROUPOp: rxChMask 0x%x\n", pNFCALGROUPParms->rxChMask);
    A_PRINTF("NFCALGROUPOp: concMode %u\n", pNFCALGROUPParms->concMode);
    A_PRINTF("NFCALGROUPOp: pad1 %u\n", pNFCALGROUPParms->pad1);
    A_PRINTF("NFCALGROUPOp: userParm1 %u\n", pNFCALGROUPParms->userParm1);
    A_PRINTF("NFCALGROUPOp: userParm2 %u\n", pNFCALGROUPParms->userParm2);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("NFCALGROUPOp: nfCalResults %u\n", pNFCALGROUPParms->nfCalResults[i]);
    }
#endif //_DEBUG

    if (NULL != NFCALGROUPOpFunc) {
        (*NFCALGROUPOpFunc)(pNFCALGROUPParms);
    }
    return(TRUE);
}

void* initNFCALGROUPRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_NFCALGROUPRSP_PARMS  *pNFCALGROUPRSPParms = (CMD_NFCALGROUPRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pNFCALGROUPRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pNFCALGROUPRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pNFCALGROUPRSPParms->miscFlags = pParmDict[PARM_MISCFLAGS].v.valU8;
    pNFCALGROUPRSPParms->pad1 = pParmDict[PARM_PAD1].v.valU8;
    memset(pNFCALGROUPRSPParms->nfCalResults, 0, sizeof(pNFCALGROUPRSPParms->nfCalResults));
    pNFCALGROUPRSPParms->executionTime = pParmDict[PARM_EXECUTIONTIME].v.valU32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pNFCALGROUPRSPParms->phyId)) - (size_t)pNFCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pNFCALGROUPRSPParms->status)) - (size_t)pNFCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MISCFLAGS, (size_t)(((size_t)&(pNFCALGROUPRSPParms->miscFlags)) - (size_t)pNFCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD1, (size_t)(((size_t)&(pNFCALGROUPRSPParms->pad1)) - (size_t)pNFCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NFCALRESULTS, (size_t)(((size_t)&(pNFCALGROUPRSPParms->nfCalResults)) - (size_t)pNFCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_EXECUTIONTIME, (size_t)(((size_t)&(pNFCALGROUPRSPParms->executionTime)) - (size_t)pNFCALGROUPRSPParms), pParmsOffset);
    return((void*) pNFCALGROUPRSPParms);
}

static NFCALGROUPRSP_OP_FUNC NFCALGROUPRSPOpFunc = NULL;

TLV2_API void registerNFCALGROUPRSPHandler(NFCALGROUPRSP_OP_FUNC fp)
{
    NFCALGROUPRSPOpFunc = fp;
}

A_BOOL NFCALGROUPRSPOp(void *pParms)
{
    CMD_NFCALGROUPRSP_PARMS *pNFCALGROUPRSPParms = (CMD_NFCALGROUPRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("NFCALGROUPRSPOp: phyId %u\n", pNFCALGROUPRSPParms->phyId);
    A_PRINTF("NFCALGROUPRSPOp: status %u\n", pNFCALGROUPRSPParms->status);
    A_PRINTF("NFCALGROUPRSPOp: miscFlags %u\n", pNFCALGROUPRSPParms->miscFlags);
    A_PRINTF("NFCALGROUPRSPOp: pad1 %u\n", pNFCALGROUPRSPParms->pad1);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("NFCALGROUPRSPOp: nfCalResults %u\n", pNFCALGROUPRSPParms->nfCalResults[i]);
    }
    A_PRINTF("NFCALGROUPRSPOp: executionTime %u\n", pNFCALGROUPRSPParms->executionTime);
#endif //_DEBUG

    if (NULL != NFCALGROUPRSPOpFunc) {
        (*NFCALGROUPRSPOpFunc)(pNFCALGROUPRSPParms);
    }
    return(TRUE);
}
