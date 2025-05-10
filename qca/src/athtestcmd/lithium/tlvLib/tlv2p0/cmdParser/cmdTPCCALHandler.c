/* Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input\cmdTPCCALHandler.s
#include "tlv2Inc.h"
#include "cmdTPCCALHandler.h"

void* initTPCCALOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_TPCCAL_PARMS  *pTPCCALParms = (CMD_TPCCAL_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);

    // Populate the parm structure with initial values
    pTPCCALParms->radioId = pParmDict[PARM_RADIOID].v.valU8;
    pTPCCALParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pTPCCALParms->TpcCalScheme = pParmDict[PARM_TPCCALSCHEME].v.valU8;
    pTPCCALParms->AllChainsON = pParmDict[PARM_ALLCHAINSON].v.valU8;
    pTPCCALParms->NumTotalChains = pParmDict[PARM_NUMTOTALCHAINS].v.valU16;
    pTPCCALParms->MultiChainMeasureMask = pParmDict[PARM_MULTICHAINMEASUREMASK].v.valU16;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_RADIOID, (size_t)(((size_t)&(pTPCCALParms->radioId)) - (size_t)pTPCCALParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pTPCCALParms->phyId)) - (size_t)pTPCCALParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TPCCALSCHEME, (size_t)(((size_t)&(pTPCCALParms->TpcCalScheme)) - (size_t)pTPCCALParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ALLCHAINSON, (size_t)(((size_t)&(pTPCCALParms->AllChainsON)) - (size_t)pTPCCALParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMTOTALCHAINS, (size_t)(((size_t)&(pTPCCALParms->NumTotalChains)) - (size_t)pTPCCALParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MULTICHAINMEASUREMASK, (size_t)(((size_t)&(pTPCCALParms->MultiChainMeasureMask)) - (size_t)pTPCCALParms), pParmsOffset);
    return((void*) pTPCCALParms);
}

static TPCCAL_OP_FUNC TPCCALOpFunc = NULL;

TLV2_API void registerTPCCALHandler(TPCCAL_OP_FUNC fp)
{
    TPCCALOpFunc = fp;
}

A_BOOL TPCCALOp(void *pParms)
{
    CMD_TPCCAL_PARMS *pTPCCALParms = (CMD_TPCCAL_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TPCCALOp: radioId %u\n", pTPCCALParms->radioId);
    A_PRINTF("TPCCALOp: phyId %u\n", pTPCCALParms->phyId);
    A_PRINTF("TPCCALOp: TpcCalScheme %u\n", pTPCCALParms->TpcCalScheme);
    A_PRINTF("TPCCALOp: AllChainsON %u\n", pTPCCALParms->AllChainsON);
    A_PRINTF("TPCCALOp: NumTotalChains 0x%x\n", pTPCCALParms->NumTotalChains);
    A_PRINTF("TPCCALOp: MultiChainMeasureMask 0x%x\n", pTPCCALParms->MultiChainMeasureMask);
#endif //_DEBUG

    if (NULL != TPCCALOpFunc) {
        (*TPCCALOpFunc)(pTPCCALParms);
    }
    return(TRUE);
}

void* initTPCCALRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    int i, j; 	//for initializing array parameter
    CMD_TPCCALRSP_PARMS  *pTPCCALRSPParms = (CMD_TPCCALRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);

    i = j = 0;	//assign a number to avoid warning in case i and j are not used

    // Populate the parm structure with initial values
    pTPCCALRSPParms->radioId = pParmDict[PARM_RADIOID].v.valU8;
    pTPCCALRSPParms->chanFreq = pParmDict[PARM_CHANFREQ].v.valU8;
    pTPCCALRSPParms->numFreq2G = pParmDict[PARM_NUMFREQ2G].v.valU8;
    memset(pTPCCALRSPParms->freq2G, 0, sizeof(pTPCCALRSPParms->freq2G));
    for (i = 0; i < 3 ; i++)
    {
        pTPCCALRSPParms->freq2G[i] = pParmDict[PARM_FREQ2G].v.ptU8[i];
    }
    pTPCCALRSPParms->numFreq5G = pParmDict[PARM_NUMFREQ5G].v.valU8;
    memset(pTPCCALRSPParms->freq5G, 0, sizeof(pTPCCALRSPParms->freq5G));
    for (i = 0; i < 8 ; i++)
    {
        pTPCCALRSPParms->freq5G[i] = pParmDict[PARM_FREQ5G].v.ptU8[i];
    }
    memset(pTPCCALRSPParms->chainMasks, 0, sizeof(pTPCCALRSPParms->chainMasks));
    for (i = 0; i < 2 ; i++)
    {
        pTPCCALRSPParms->chainMasks[i] = pParmDict[PARM_CHAINMASKS].v.ptU16[i];
    }
    pTPCCALRSPParms->numChain = pParmDict[PARM_NUMCHAIN].v.valU8;
    pTPCCALRSPParms->numCALPt2G = pParmDict[PARM_NUMCALPT2G].v.valU8;
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALRSPParms->tgtPwr2G[i] = pParmDict[PARM_TGTPWR2G].v.ptS16[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALRSPParms->CALPt2G[i] = pParmDict[PARM_CALPT2G].v.ptU8[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALRSPParms->txGains2G[i] = pParmDict[PARM_TXGAINS2G].v.ptU8[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALRSPParms->dacGains2G[i] = pParmDict[PARM_DACGAINS2G].v.ptU8[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALRSPParms->paCfg2G[i] = pParmDict[PARM_PACFG2G].v.ptU8[i];
    }
    pTPCCALRSPParms->numCALPt5G = pParmDict[PARM_NUMCALPT5G].v.valU8;
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALRSPParms->CALPt5G[i] = pParmDict[PARM_CALPT5G].v.ptU8[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALRSPParms->tgtPwr5G[i] = pParmDict[PARM_TGTPWR5G].v.ptS16[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALRSPParms->txGains5G[i] = pParmDict[PARM_TXGAINS5G].v.ptU8[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALRSPParms->dacGains5G[i] = pParmDict[PARM_DACGAINS5G].v.ptU8[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALRSPParms->paCfg5G[i] = pParmDict[PARM_PACFG5G].v.ptU8[i];
    }
    pTPCCALRSPParms->miscFlags = pParmDict[PARM_MISCFLAGS].v.valU8;
    pTPCCALRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pTPCCALRSPParms->pad1 = pParmDict[PARM_PAD1].v.valU8;
    memset(pTPCCALRSPParms->mhz2G, 0, sizeof(pTPCCALRSPParms->mhz2G));
    memset(pTPCCALRSPParms->mhz5G, 0, sizeof(pTPCCALRSPParms->mhz5G));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_RADIOID, (size_t)(((size_t)&(pTPCCALRSPParms->radioId)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHANFREQ, (size_t)(((size_t)&(pTPCCALRSPParms->chanFreq)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMFREQ2G, (size_t)(((size_t)&(pTPCCALRSPParms->numFreq2G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ2G, (size_t)(((size_t)&(pTPCCALRSPParms->freq2G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMFREQ5G, (size_t)(((size_t)&(pTPCCALRSPParms->numFreq5G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ5G, (size_t)(((size_t)&(pTPCCALRSPParms->freq5G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINMASKS, (size_t)(((size_t)&(pTPCCALRSPParms->chainMasks)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMCHAIN, (size_t)(((size_t)&(pTPCCALRSPParms->numChain)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMCALPT2G, (size_t)(((size_t)&(pTPCCALRSPParms->numCALPt2G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TGTPWR2G, (size_t)(((size_t)&(pTPCCALRSPParms->tgtPwr2G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALPT2G, (size_t)(((size_t)&(pTPCCALRSPParms->CALPt2G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXGAINS2G, (size_t)(((size_t)&(pTPCCALRSPParms->txGains2G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DACGAINS2G, (size_t)(((size_t)&(pTPCCALRSPParms->dacGains2G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PACFG2G, (size_t)(((size_t)&(pTPCCALRSPParms->paCfg2G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMCALPT5G, (size_t)(((size_t)&(pTPCCALRSPParms->numCALPt5G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALPT5G, (size_t)(((size_t)&(pTPCCALRSPParms->CALPt5G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TGTPWR5G, (size_t)(((size_t)&(pTPCCALRSPParms->tgtPwr5G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXGAINS5G, (size_t)(((size_t)&(pTPCCALRSPParms->txGains5G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DACGAINS5G, (size_t)(((size_t)&(pTPCCALRSPParms->dacGains5G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PACFG5G, (size_t)(((size_t)&(pTPCCALRSPParms->paCfg5G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MISCFLAGS, (size_t)(((size_t)&(pTPCCALRSPParms->miscFlags)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pTPCCALRSPParms->phyId)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD1, (size_t)(((size_t)&(pTPCCALRSPParms->pad1)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MHZ2GARRAY, (size_t)(((size_t)&(pTPCCALRSPParms->mhz2G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MHZ5GARRAY, (size_t)(((size_t)&(pTPCCALRSPParms->mhz5G)) - (size_t)pTPCCALRSPParms), pParmsOffset);
    return((void*) pTPCCALRSPParms);
}

static TPCCALRSP_OP_FUNC TPCCALRSPOpFunc = NULL;

TLV2_API void registerTPCCALRSPHandler(TPCCALRSP_OP_FUNC fp)
{
    TPCCALRSPOpFunc = fp;
}

A_BOOL TPCCALRSPOp(void *pParms)
{
    CMD_TPCCALRSP_PARMS *pTPCCALRSPParms = (CMD_TPCCALRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TPCCALRSPOp: radioId %u\n", pTPCCALRSPParms->radioId);
    A_PRINTF("TPCCALRSPOp: chanFreq %u\n", pTPCCALRSPParms->chanFreq);
    A_PRINTF("TPCCALRSPOp: numFreq2G %u\n", pTPCCALRSPParms->numFreq2G);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 14 entries
    {
        A_PRINTF("TPCCALRSPOp: freq2G %u\n", pTPCCALRSPParms->freq2G[i]);
    }
    A_PRINTF("TPCCALRSPOp: numFreq5G %u\n", pTPCCALRSPParms->numFreq5G);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("TPCCALRSPOp: freq5G %u\n", pTPCCALRSPParms->freq5G[i]);
    }
    for (i = 0; i < 4 ; i++)
    {
        A_PRINTF("TPCCALRSPOp: chainMasks 0x%x\n", pTPCCALRSPParms->chainMasks[i]);
    }
    A_PRINTF("TPCCALRSPOp: numChain %u\n", pTPCCALRSPParms->numChain);
    A_PRINTF("TPCCALRSPOp: numCALPt2G %u\n", pTPCCALRSPParms->numCALPt2G);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALRSPOp: tgtPwr2G %d\n", pTPCCALRSPParms->tgtPwr2G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALRSPOp: CALPt2G %u\n", pTPCCALRSPParms->CALPt2G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALRSPOp: txGains2G %u\n", pTPCCALRSPParms->txGains2G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALRSPOp: dacGains2G %u\n", pTPCCALRSPParms->dacGains2G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALRSPOp: paCfg2G %u\n", pTPCCALRSPParms->paCfg2G[i]);
    }
    A_PRINTF("TPCCALRSPOp: numCALPt5G %u\n", pTPCCALRSPParms->numCALPt5G);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALRSPOp: CALPt5G %u\n", pTPCCALRSPParms->CALPt5G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALRSPOp: tgtPwr5G %d\n", pTPCCALRSPParms->tgtPwr5G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALRSPOp: txGains5G %u\n", pTPCCALRSPParms->txGains5G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALRSPOp: dacGains5G %u\n", pTPCCALRSPParms->dacGains5G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALRSPOp: paCfg5G %u\n", pTPCCALRSPParms->paCfg5G[i]);
    }
    A_PRINTF("TPCCALRSPOp: miscFlags 0x%x\n", pTPCCALRSPParms->miscFlags);
    A_PRINTF("TPCCALRSPOp: phyId %u\n", pTPCCALRSPParms->phyId);
    A_PRINTF("TPCCALRSPOp: pad1 %u\n", pTPCCALRSPParms->pad1);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 14 entries
    {
        A_PRINTF("TPCCALRSPOp: mhz2G %u\n", pTPCCALRSPParms->mhz2G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 64 entries
    {
        A_PRINTF("TPCCALRSPOp: mhz5G %u\n", pTPCCALRSPParms->mhz5G[i]);
    }
#endif //_DEBUG

    if (NULL != TPCCALRSPOpFunc) {
        (*TPCCALRSPOpFunc)(pTPCCALRSPParms);
    }
    return(TRUE);
}
