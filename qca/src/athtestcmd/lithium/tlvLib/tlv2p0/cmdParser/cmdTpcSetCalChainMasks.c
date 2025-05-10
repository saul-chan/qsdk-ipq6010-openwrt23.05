/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input\cmdTpcSetCalChainMasks.s
#include "tlv2Inc.h"
#include "cmdTpcSetCalChainMasks.h"

void* initTPCSETCALCHAINMASKSOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_TPCSETCALCHAINMASKS_PARMS  *pTPCSETCALCHAINMASKSParms = (CMD_TPCSETCALCHAINMASKS_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pTPCSETCALCHAINMASKSParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pTPCSETCALCHAINMASKSParms->pad1 = pParmDict[PARM_PAD1].v.valU8;
    pTPCSETCALCHAINMASKSParms->txMask2G = pParmDict[PARM_TXMASK2G].v.valU16;
    pTPCSETCALCHAINMASKSParms->rxMask2G = pParmDict[PARM_RXMASK2G].v.valU16;
    pTPCSETCALCHAINMASKSParms->txMask5G = pParmDict[PARM_TXMASK5G].v.valU16;
    pTPCSETCALCHAINMASKSParms->rxMask5G = pParmDict[PARM_RXMASK5G].v.valU16;
    pTPCSETCALCHAINMASKSParms->concurrencyModeMask = pParmDict[PARM_CONCURRENCYMODEMASK].v.valU32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pTPCSETCALCHAINMASKSParms->phyId)) - (size_t)pTPCSETCALCHAINMASKSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD1, (size_t)(((size_t)&(pTPCSETCALCHAINMASKSParms->pad1)) - (size_t)pTPCSETCALCHAINMASKSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXMASK2G, (size_t)(((size_t)&(pTPCSETCALCHAINMASKSParms->txMask2G)) - (size_t)pTPCSETCALCHAINMASKSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXMASK2G, (size_t)(((size_t)&(pTPCSETCALCHAINMASKSParms->rxMask2G)) - (size_t)pTPCSETCALCHAINMASKSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXMASK5G, (size_t)(((size_t)&(pTPCSETCALCHAINMASKSParms->txMask5G)) - (size_t)pTPCSETCALCHAINMASKSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXMASK5G, (size_t)(((size_t)&(pTPCSETCALCHAINMASKSParms->rxMask5G)) - (size_t)pTPCSETCALCHAINMASKSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CONCURRENCYMODEMASK, (size_t)(((size_t)&(pTPCSETCALCHAINMASKSParms->concurrencyModeMask)) - (size_t)pTPCSETCALCHAINMASKSParms), pParmsOffset);
    return((void*) pTPCSETCALCHAINMASKSParms);
}

static TPCSETCALCHAINMASKS_OP_FUNC TPCSETCALCHAINMASKSOpFunc = NULL;

TLV2_API void registerTPCSETCALCHAINMASKSHandler(TPCSETCALCHAINMASKS_OP_FUNC fp)
{
    TPCSETCALCHAINMASKSOpFunc = fp;
}

A_BOOL TPCSETCALCHAINMASKSOp(void *pParms)
{
    CMD_TPCSETCALCHAINMASKS_PARMS *pTPCSETCALCHAINMASKSParms = (CMD_TPCSETCALCHAINMASKS_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TPCSETCALCHAINMASKSOp: phyId %u\n", pTPCSETCALCHAINMASKSParms->phyId);
    A_PRINTF("TPCSETCALCHAINMASKSOp: pad1 %u\n", pTPCSETCALCHAINMASKSParms->pad1);
    A_PRINTF("TPCSETCALCHAINMASKSOp: txMask2G %u\n", pTPCSETCALCHAINMASKSParms->txMask2G);
    A_PRINTF("TPCSETCALCHAINMASKSOp: rxMask2G %u\n", pTPCSETCALCHAINMASKSParms->rxMask2G);
    A_PRINTF("TPCSETCALCHAINMASKSOp: txMask5G %u\n", pTPCSETCALCHAINMASKSParms->txMask5G);
    A_PRINTF("TPCSETCALCHAINMASKSOp: rxMask5G %u\n", pTPCSETCALCHAINMASKSParms->rxMask5G);
    A_PRINTF("TPCSETCALCHAINMASKSOp: concurrencyModeMask %u\n", pTPCSETCALCHAINMASKSParms->concurrencyModeMask);
#endif //_DEBUG

    if (NULL != TPCSETCALCHAINMASKSOpFunc) {
        (*TPCSETCALCHAINMASKSOpFunc)(pTPCSETCALCHAINMASKSParms);
    }
    return(TRUE);
}

void* initTPCSETCALCHAINMASKSRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_TPCSETCALCHAINMASKSRSP_PARMS  *pTPCSETCALCHAINMASKSRSPParms = (CMD_TPCSETCALCHAINMASKSRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pTPCSETCALCHAINMASKSRSPParms->status = pParmDict[PARM_STATUS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pTPCSETCALCHAINMASKSRSPParms->status)) - (size_t)pTPCSETCALCHAINMASKSRSPParms), pParmsOffset);
    return((void*) pTPCSETCALCHAINMASKSRSPParms);
}

static TPCSETCALCHAINMASKSRSP_OP_FUNC TPCSETCALCHAINMASKSRSPOpFunc = NULL;

TLV2_API void registerTPCSETCALCHAINMASKSRSPHandler(TPCSETCALCHAINMASKSRSP_OP_FUNC fp)
{
    TPCSETCALCHAINMASKSRSPOpFunc = fp;
}

A_BOOL TPCSETCALCHAINMASKSRSPOp(void *pParms)
{
    CMD_TPCSETCALCHAINMASKSRSP_PARMS *pTPCSETCALCHAINMASKSRSPParms = (CMD_TPCSETCALCHAINMASKSRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TPCSETCALCHAINMASKSRSPOp: status %u\n", pTPCSETCALCHAINMASKSRSPParms->status);
#endif //_DEBUG

    if (NULL != TPCSETCALCHAINMASKSRSPOpFunc) {
        (*TPCSETCALCHAINMASKSRSPOpFunc)(pTPCSETCALCHAINMASKSRSPParms);
    }
    return(TRUE);
}
