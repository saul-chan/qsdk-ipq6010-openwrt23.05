/* Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input\cmdTPCCALPWRHandler.s
#include "tlv2Inc.h"
#include "cmdTPCCALPWRHandler.h"

void* initTPCCALPWROpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    int i, j; 	//for initializing array parameter
    CMD_TPCCALPWR_PARMS  *pTPCCALPWRParms = (CMD_TPCCALPWR_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);

    i = j = 0;	//assign a number to avoid warning in case i and j are not used

    // Populate the parm structure with initial values
    memset(pTPCCALPWRParms->measuredPwr, 0, sizeof(pTPCCALPWRParms->measuredPwr));
    for (i = 0; i < 2 ; i++)
    {
        pTPCCALPWRParms->measuredPwr[i] = pParmDict[PARM_MEASUREDPWR].v.ptS16[i];
    }
    pTPCCALPWRParms->radioId = pParmDict[PARM_RADIOID].v.valU8;
    pTPCCALPWRParms->numMeasuredPwr = pParmDict[PARM_NUMMEASUREDPWR].v.valU8;
    pTPCCALPWRParms->phyId = pParmDict[PARM_PHYID].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_MEASUREDPWR, (size_t)(((size_t)&(pTPCCALPWRParms->measuredPwr)) - (size_t)pTPCCALPWRParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RADIOID, (size_t)(((size_t)&(pTPCCALPWRParms->radioId)) - (size_t)pTPCCALPWRParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMMEASUREDPWR, (size_t)(((size_t)&(pTPCCALPWRParms->numMeasuredPwr)) - (size_t)pTPCCALPWRParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pTPCCALPWRParms->phyId)) - (size_t)pTPCCALPWRParms), pParmsOffset);
    return((void*) pTPCCALPWRParms);
}

static TPCCALPWR_OP_FUNC TPCCALPWROpFunc = NULL;

TLV2_API void registerTPCCALPWRHandler(TPCCALPWR_OP_FUNC fp)
{
    TPCCALPWROpFunc = fp;
}

A_BOOL TPCCALPWROp(void *pParms)
{
    CMD_TPCCALPWR_PARMS *pTPCCALPWRParms = (CMD_TPCCALPWR_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG
    int i; 	//for initializing array parameter
    i = 0;	//assign a number to avoid warning in case i is not used

    for (i = 0; i < 8 ; i++) // can be modified to print up to 512 entries
    {
        A_PRINTF("TPCCALPWROp: measuredPwr %d\n", pTPCCALPWRParms->measuredPwr[i]);
    }
    A_PRINTF("TPCCALPWROp: radioId %u\n", pTPCCALPWRParms->radioId);
    A_PRINTF("TPCCALPWROp: numMeasuredPwr %u\n", pTPCCALPWRParms->numMeasuredPwr);
    A_PRINTF("TPCCALPWROp: phyId %u\n", pTPCCALPWRParms->phyId);
#endif //_DEBUG

    if (NULL != TPCCALPWROpFunc) {
        (*TPCCALPWROpFunc)(pTPCCALPWRParms);
    }
    return(TRUE);
}

void* initTPCCALDATAOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    int i, j; 	//for initializing array parameter
    CMD_TPCCALDATA_PARMS  *pTPCCALDATAParms = (CMD_TPCCALDATA_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);

    i = j = 0;	//assign a number to avoid warning in case i and j are not used

    // Populate the parm structure with initial values
    pTPCCALDATAParms->miscFlags = pParmDict[PARM_MISCFLAGS].v.valU8;
    pTPCCALDATAParms->radioId = pParmDict[PARM_RADIOID].v.valU8;
    memset(pTPCCALDATAParms->calData2G, 0, sizeof(pTPCCALDATAParms->calData2G));
    pTPCCALDATAParms->calData2GOffset = pParmDict[PARM_CALDATA2GOFFSET].v.valU16;
    pTPCCALDATAParms->calData2GLength = pParmDict[PARM_CALDATA2GLENGTH].v.valU16;
    memset(pTPCCALDATAParms->calData5G, 0, sizeof(pTPCCALDATAParms->calData5G));
    pTPCCALDATAParms->calData5GOffset = pParmDict[PARM_CALDATA5GOFFSET].v.valU16;
    pTPCCALDATAParms->calData5GLength = pParmDict[PARM_CALDATA5GLENGTH].v.valU16;
    memset(pTPCCALDATAParms->calData2G_clpc, 0, sizeof(pTPCCALDATAParms->calData2G_clpc));
    pTPCCALDATAParms->calData2GOffset_clpc = pParmDict[PARM_CALDATA2GOFFSET_CLPC].v.valU16;
    pTPCCALDATAParms->calData2GLength_clpc = pParmDict[PARM_CALDATA2GLENGTH_CLPC].v.valU16;
    memset(pTPCCALDATAParms->calData5G_clpc, 0, sizeof(pTPCCALDATAParms->calData5G_clpc));
    pTPCCALDATAParms->calData5GOffset_clpc = pParmDict[PARM_CALDATA5GOFFSET_CLPC].v.valU16;
    pTPCCALDATAParms->calData5GLength_clpc = pParmDict[PARM_CALDATA5GLENGTH_CLPC].v.valU16;
    pTPCCALDATAParms->numFreq2G = pParmDict[PARM_NUMFREQ2G].v.valU8;
    memset(pTPCCALDATAParms->freq2G, 0, sizeof(pTPCCALDATAParms->freq2G));
    for (i = 0; i < 3 ; i++)
    {
        pTPCCALDATAParms->freq2G[i] = pParmDict[PARM_FREQ2G].v.ptU8[i];
    }
    pTPCCALDATAParms->numFreq5G = pParmDict[PARM_NUMFREQ5G].v.valU8;
    memset(pTPCCALDATAParms->freq5G, 0, sizeof(pTPCCALDATAParms->freq5G));
    for (i = 0; i < 8 ; i++)
    {
        pTPCCALDATAParms->freq5G[i] = pParmDict[PARM_FREQ5G].v.ptU8[i];
    }
    memset(pTPCCALDATAParms->chainMasks, 0, sizeof(pTPCCALDATAParms->chainMasks));
    for (i = 0; i < 2 ; i++)
    {
        pTPCCALDATAParms->chainMasks[i] = pParmDict[PARM_CHAINMASKS].v.ptU16[i];
    }
    pTPCCALDATAParms->numChain = pParmDict[PARM_NUMCHAIN].v.valU8;
    pTPCCALDATAParms->numCALPt2G = pParmDict[PARM_NUMCALPT2G].v.valU8;
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALDATAParms->tgtPwr2G[i] = pParmDict[PARM_TGTPWR2G].v.ptS16[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALDATAParms->CALPt2G[i] = pParmDict[PARM_CALPT2G].v.ptU8[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALDATAParms->txGains2G[i] = pParmDict[PARM_TXGAINS2G].v.ptU8[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALDATAParms->dacGains2G[i] = pParmDict[PARM_DACGAINS2G].v.ptU8[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALDATAParms->paCfg2G[i] = pParmDict[PARM_PACFG2G].v.ptU8[i];
    }
    pTPCCALDATAParms->pad1 = pParmDict[PARM_PAD1].v.valU8;
    pTPCCALDATAParms->numCALPt5G = pParmDict[PARM_NUMCALPT5G].v.valU8;
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALDATAParms->tgtPwr5G[i] = pParmDict[PARM_TGTPWR5G].v.ptS16[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALDATAParms->CALPt5G[i] = pParmDict[PARM_CALPT5G].v.ptU8[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALDATAParms->txGains5G[i] = pParmDict[PARM_TXGAINS5G].v.ptU8[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALDATAParms->dacGains5G[i] = pParmDict[PARM_DACGAINS5G].v.ptU8[i];
    }
    for (i = 0; i < 13 ; i++)
    {
        pTPCCALDATAParms->paCfg5G[i] = pParmDict[PARM_PACFG5G].v.ptU8[i];
    }
    pTPCCALDATAParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pTPCCALDATAParms->padding = pParmDict[PARM_PADDING].v.valU8;
    memset(pTPCCALDATAParms->mhz2G, 0, sizeof(pTPCCALDATAParms->mhz2G));
    memset(pTPCCALDATAParms->mhz5G, 0, sizeof(pTPCCALDATAParms->mhz5G));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_MISCFLAGS, (size_t)(((size_t)&(pTPCCALDATAParms->miscFlags)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RADIOID, (size_t)(((size_t)&(pTPCCALDATAParms->radioId)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALDATA2G, (size_t)(((size_t)&(pTPCCALDATAParms->calData2G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALDATA2GOFFSET, (size_t)(((size_t)&(pTPCCALDATAParms->calData2GOffset)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALDATA2GLENGTH, (size_t)(((size_t)&(pTPCCALDATAParms->calData2GLength)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALDATA5G, (size_t)(((size_t)&(pTPCCALDATAParms->calData5G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALDATA5GOFFSET, (size_t)(((size_t)&(pTPCCALDATAParms->calData5GOffset)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALDATA5GLENGTH, (size_t)(((size_t)&(pTPCCALDATAParms->calData5GLength)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALDATA2G_CLPC, (size_t)(((size_t)&(pTPCCALDATAParms->calData2G_clpc)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALDATA2GOFFSET_CLPC, (size_t)(((size_t)&(pTPCCALDATAParms->calData2GOffset_clpc)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALDATA2GLENGTH_CLPC, (size_t)(((size_t)&(pTPCCALDATAParms->calData2GLength_clpc)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALDATA5G_CLPC, (size_t)(((size_t)&(pTPCCALDATAParms->calData5G_clpc)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALDATA5GOFFSET_CLPC, (size_t)(((size_t)&(pTPCCALDATAParms->calData5GOffset_clpc)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALDATA5GLENGTH_CLPC, (size_t)(((size_t)&(pTPCCALDATAParms->calData5GLength_clpc)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMFREQ2G, (size_t)(((size_t)&(pTPCCALDATAParms->numFreq2G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ2G, (size_t)(((size_t)&(pTPCCALDATAParms->freq2G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMFREQ5G, (size_t)(((size_t)&(pTPCCALDATAParms->numFreq5G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ5G, (size_t)(((size_t)&(pTPCCALDATAParms->freq5G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINMASKS, (size_t)(((size_t)&(pTPCCALDATAParms->chainMasks)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMCHAIN, (size_t)(((size_t)&(pTPCCALDATAParms->numChain)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMCALPT2G, (size_t)(((size_t)&(pTPCCALDATAParms->numCALPt2G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TGTPWR2G, (size_t)(((size_t)&(pTPCCALDATAParms->tgtPwr2G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALPT2G, (size_t)(((size_t)&(pTPCCALDATAParms->CALPt2G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXGAINS2G, (size_t)(((size_t)&(pTPCCALDATAParms->txGains2G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DACGAINS2G, (size_t)(((size_t)&(pTPCCALDATAParms->dacGains2G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PACFG2G, (size_t)(((size_t)&(pTPCCALDATAParms->paCfg2G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD1, (size_t)(((size_t)&(pTPCCALDATAParms->pad1)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMCALPT5G, (size_t)(((size_t)&(pTPCCALDATAParms->numCALPt5G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TGTPWR5G, (size_t)(((size_t)&(pTPCCALDATAParms->tgtPwr5G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALPT5G, (size_t)(((size_t)&(pTPCCALDATAParms->CALPt5G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXGAINS5G, (size_t)(((size_t)&(pTPCCALDATAParms->txGains5G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DACGAINS5G, (size_t)(((size_t)&(pTPCCALDATAParms->dacGains5G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PACFG5G, (size_t)(((size_t)&(pTPCCALDATAParms->paCfg5G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pTPCCALDATAParms->phyId)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PADDING, (size_t)(((size_t)&(pTPCCALDATAParms->padding)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MHZ2GARRAY, (size_t)(((size_t)&(pTPCCALDATAParms->mhz2G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MHZ5GARRAY, (size_t)(((size_t)&(pTPCCALDATAParms->mhz5G)) - (size_t)pTPCCALDATAParms), pParmsOffset);
    return((void*) pTPCCALDATAParms);
}

static TPCCALDATA_OP_FUNC TPCCALDATAOpFunc = NULL;

TLV2_API void registerTPCCALDATAHandler(TPCCALDATA_OP_FUNC fp)
{
    TPCCALDATAOpFunc = fp;
}

A_BOOL TPCCALDATAOp(void *pParms)
{
    CMD_TPCCALDATA_PARMS *pTPCCALDATAParms = (CMD_TPCCALDATA_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG
    int i; 	//for initializing array parameter
    i = 0;	//assign a number to avoid warning in case i is not used

    A_PRINTF("TPCCALDATAOp: miscFlags %u\n", pTPCCALDATAParms->miscFlags);
    A_PRINTF("TPCCALDATAOp: radioId %u\n", pTPCCALDATAParms->radioId);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 256 entries
    {
        A_PRINTF("TPCCALDATAOp: calData2G %u\n", pTPCCALDATAParms->calData2G[i]);
    }
    A_PRINTF("TPCCALDATAOp: calData2GOffset %u\n", pTPCCALDATAParms->calData2GOffset);
    A_PRINTF("TPCCALDATAOp: calData2GLength %u\n", pTPCCALDATAParms->calData2GLength);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 512 entries
    {
        A_PRINTF("TPCCALDATAOp: calData5G %u\n", pTPCCALDATAParms->calData5G[i]);
    }
    A_PRINTF("TPCCALDATAOp: calData5GOffset %u\n", pTPCCALDATAParms->calData5GOffset);
    A_PRINTF("TPCCALDATAOp: calData5GLength %u\n", pTPCCALDATAParms->calData5GLength);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 256 entries
    {
        A_PRINTF("TPCCALDATAOp: calData2G_clpc %u\n", pTPCCALDATAParms->calData2G_clpc[i]);
    }
    A_PRINTF("TPCCALDATAOp: calData2GOffset_clpc %u\n", pTPCCALDATAParms->calData2GOffset_clpc);
    A_PRINTF("TPCCALDATAOp: calData2GLength_clpc %u\n", pTPCCALDATAParms->calData2GLength_clpc);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 512 entries
    {
        A_PRINTF("TPCCALDATAOp: calData5G_clpc %u\n", pTPCCALDATAParms->calData5G_clpc[i]);
    }
    A_PRINTF("TPCCALDATAOp: calData5GOffset_clpc %u\n", pTPCCALDATAParms->calData5GOffset_clpc);
    A_PRINTF("TPCCALDATAOp: calData5GLength_clpc %u\n", pTPCCALDATAParms->calData5GLength_clpc);
    A_PRINTF("TPCCALDATAOp: numFreq2G %u\n", pTPCCALDATAParms->numFreq2G);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 14 entries
    {
        A_PRINTF("TPCCALDATAOp: freq2G %u\n", pTPCCALDATAParms->freq2G[i]);
    }
    A_PRINTF("TPCCALDATAOp: numFreq5G %u\n", pTPCCALDATAParms->numFreq5G);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("TPCCALDATAOp: freq5G %u\n", pTPCCALDATAParms->freq5G[i]);
    }
    for (i = 0; i < 4 ; i++)
    {
        A_PRINTF("TPCCALDATAOp: chainMasks 0x%x\n", pTPCCALDATAParms->chainMasks[i]);
    }
    A_PRINTF("TPCCALDATAOp: numChain %u\n", pTPCCALDATAParms->numChain);
    A_PRINTF("TPCCALDATAOp: numCALPt2G %u\n", pTPCCALDATAParms->numCALPt2G);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALDATAOp: tgtPwr2G %d\n", pTPCCALDATAParms->tgtPwr2G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALDATAOp: CALPt2G %u\n", pTPCCALDATAParms->CALPt2G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALDATAOp: txGains2G %u\n", pTPCCALDATAParms->txGains2G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALDATAOp: dacGains2G %u\n", pTPCCALDATAParms->dacGains2G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALDATAOp: paCfg2G %u\n", pTPCCALDATAParms->paCfg2G[i]);
    }
    A_PRINTF("TPCCALDATAOp: pad1 %u\n", pTPCCALDATAParms->pad1);
    A_PRINTF("TPCCALDATAOp: numCALPt5G %u\n", pTPCCALDATAParms->numCALPt5G);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALDATAOp: tgtPwr5G %d\n", pTPCCALDATAParms->tgtPwr5G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALDATAOp: CALPt5G %u\n", pTPCCALDATAParms->CALPt5G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALDATAOp: txGains5G %u\n", pTPCCALDATAParms->txGains5G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALDATAOp: dacGains5G %u\n", pTPCCALDATAParms->dacGains5G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 13 entries
    {
        A_PRINTF("TPCCALDATAOp: paCfg5G %u\n", pTPCCALDATAParms->paCfg5G[i]);
    }
    A_PRINTF("TPCCALDATAOp: phyId %u\n", pTPCCALDATAParms->phyId);
    A_PRINTF("TPCCALDATAOp: padding %u\n", pTPCCALDATAParms->padding);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 14 entries
    {
        A_PRINTF("TPCCALDATAOp: mhz2G %u\n", pTPCCALDATAParms->mhz2G[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 64 entries
    {
        A_PRINTF("TPCCALDATAOp: mhz5G %u\n", pTPCCALDATAParms->mhz5G[i]);
    }
#endif //_DEBUG

    if (NULL != TPCCALDATAOpFunc) {
        (*TPCCALDATAOpFunc)(pTPCCALDATAParms);
    }
    return(TRUE);
}
