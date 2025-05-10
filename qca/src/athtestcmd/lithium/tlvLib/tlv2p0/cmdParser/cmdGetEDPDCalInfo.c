/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdGetEDPDCalInfo.s
#include "tlv2Inc.h"
#include "cmdGetEDPDCalInfo.h"

void* initGETEDPDCALINFOOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETEDPDCALINFO_PARMS  *pGETEDPDCALINFOParms = (CMD_GETEDPDCALINFO_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGETEDPDCALINFOParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pGETEDPDCALINFOParms->chainMask = pParmDict[PARM_CHAINMASK].v.valU16;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGETEDPDCALINFOParms->phyId)) - (size_t)pGETEDPDCALINFOParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINMASK, (size_t)(((size_t)&(pGETEDPDCALINFOParms->chainMask)) - (size_t)pGETEDPDCALINFOParms), pParmsOffset);
    return((void*) pGETEDPDCALINFOParms);
}

static GETEDPDCALINFO_OP_FUNC GETEDPDCALINFOOpFunc = NULL;

TLV2_API void registerGETEDPDCALINFOHandler(GETEDPDCALINFO_OP_FUNC fp)
{
    GETEDPDCALINFOOpFunc = fp;
}

A_BOOL GETEDPDCALINFOOp(void *pParms)
{
    CMD_GETEDPDCALINFO_PARMS *pGETEDPDCALINFOParms = (CMD_GETEDPDCALINFO_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GETEDPDCALINFOOp: phyId %u\n", pGETEDPDCALINFOParms->phyId);
    A_PRINTF("GETEDPDCALINFOOp: chainMask 0x%x\n", pGETEDPDCALINFOParms->chainMask);
#endif //_DEBUG

    if (NULL != GETEDPDCALINFOOpFunc) {
        (*GETEDPDCALINFOOpFunc)(pGETEDPDCALINFOParms);
    }
    return(TRUE);
}

void* initGETEDPDCALINFORSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETEDPDCALINFORSP_PARMS  *pGETEDPDCALINFORSPParms = (CMD_GETEDPDCALINFORSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGETEDPDCALINFORSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pGETEDPDCALINFORSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pGETEDPDCALINFORSPParms->chainMask = pParmDict[PARM_CHAINMASK].v.valU16;
    memset(pGETEDPDCALINFORSPParms->nmse, 0, sizeof(pGETEDPDCALINFORSPParms->nmse));
    pGETEDPDCALINFORSPParms->nmse[0] = pParmDict[PARM_NMSE].v.ptS8[0];
    memset(pGETEDPDCALINFORSPParms->signalQuality, 0, sizeof(pGETEDPDCALINFORSPParms->signalQuality));
    pGETEDPDCALINFORSPParms->signalQuality[0] = pParmDict[PARM_SIGNALQUALITY].v.ptU16[0];
    memset(pGETEDPDCALINFORSPParms->maxBin, 0, sizeof(pGETEDPDCALINFORSPParms->maxBin));
    pGETEDPDCALINFORSPParms->maxBin[0] = pParmDict[PARM_MAXBIN].v.ptU8[0];
    memset(pGETEDPDCALINFORSPParms->diffn0, 0, sizeof(pGETEDPDCALINFORSPParms->diffn0));
    pGETEDPDCALINFORSPParms->diffn0[0] = pParmDict[PARM_DIFFN0].v.ptU16[0];
    memset(pGETEDPDCALINFORSPParms->diffn1, 0, sizeof(pGETEDPDCALINFORSPParms->diffn1));
    pGETEDPDCALINFORSPParms->diffn1[0] = pParmDict[PARM_DIFFN1].v.ptU16[0];
    memset(pGETEDPDCALINFORSPParms->diffn2, 0, sizeof(pGETEDPDCALINFORSPParms->diffn2));
    pGETEDPDCALINFORSPParms->diffn2[0] = pParmDict[PARM_DIFFN2].v.ptU16[0];
    memset(pGETEDPDCALINFORSPParms->diffn3, 0, sizeof(pGETEDPDCALINFORSPParms->diffn3));
    pGETEDPDCALINFORSPParms->diffn3[0] = pParmDict[PARM_DIFFN3].v.ptU16[0];
    memset(pGETEDPDCALINFORSPParms->sqIdx, 0, sizeof(pGETEDPDCALINFORSPParms->sqIdx));
    pGETEDPDCALINFORSPParms->sqIdx[0] = pParmDict[PARM_SQIDX].v.ptU8[0];
    memset(pGETEDPDCALINFORSPParms->rxGain, 0, sizeof(pGETEDPDCALINFORSPParms->rxGain));
    pGETEDPDCALINFORSPParms->rxGain[0] = pParmDict[PARM_RXGAIN].v.ptU8[0];
    memset(pGETEDPDCALINFORSPParms->dcI, 0, sizeof(pGETEDPDCALINFORSPParms->dcI));
    pGETEDPDCALINFORSPParms->dcI[0] = pParmDict[PARM_DCI].v.ptS16[0];
    memset(pGETEDPDCALINFORSPParms->dcQ, 0, sizeof(pGETEDPDCALINFORSPParms->dcQ));
    pGETEDPDCALINFORSPParms->dcQ[0] = pParmDict[PARM_DCQ].v.ptS16[0];

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->status)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->phyId)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINMASK, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->chainMask)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NMSE, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->nmse)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SIGNALQUALITY, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->signalQuality)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MAXBIN, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->maxBin)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DIFFN0, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->diffn0)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DIFFN1, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->diffn1)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DIFFN2, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->diffn2)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DIFFN3, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->diffn3)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SQIDX, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->sqIdx)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXGAIN, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->rxGain)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DCI, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->dcI)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DCQ, (size_t)(((size_t)&(pGETEDPDCALINFORSPParms->dcQ)) - (size_t)pGETEDPDCALINFORSPParms), pParmsOffset);
    return((void*) pGETEDPDCALINFORSPParms);
}

static GETEDPDCALINFORSP_OP_FUNC GETEDPDCALINFORSPOpFunc = NULL;

TLV2_API void registerGETEDPDCALINFORSPHandler(GETEDPDCALINFORSP_OP_FUNC fp)
{
    GETEDPDCALINFORSPOpFunc = fp;
}

A_BOOL GETEDPDCALINFORSPOp(void *pParms)
{
    CMD_GETEDPDCALINFORSP_PARMS *pGETEDPDCALINFORSPParms = (CMD_GETEDPDCALINFORSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GETEDPDCALINFORSPOp: status %u\n", pGETEDPDCALINFORSPParms->status);
    A_PRINTF("GETEDPDCALINFORSPOp: phyId %u\n", pGETEDPDCALINFORSPParms->phyId);
    A_PRINTF("GETEDPDCALINFORSPOp: chainMask 0x%x\n", pGETEDPDCALINFORSPParms->chainMask);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("GETEDPDCALINFORSPOp: nmse %u\n", pGETEDPDCALINFORSPParms->nmse[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("GETEDPDCALINFORSPOp: signalQuality %u\n", pGETEDPDCALINFORSPParms->signalQuality[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("GETEDPDCALINFORSPOp: maxBin %u\n", pGETEDPDCALINFORSPParms->maxBin[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("GETEDPDCALINFORSPOp: diffn0 %u\n", pGETEDPDCALINFORSPParms->diffn0[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("GETEDPDCALINFORSPOp: diffn1 %u\n", pGETEDPDCALINFORSPParms->diffn1[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("GETEDPDCALINFORSPOp: diffn2 %u\n", pGETEDPDCALINFORSPParms->diffn2[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("GETEDPDCALINFORSPOp: diffn3 %u\n", pGETEDPDCALINFORSPParms->diffn3[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("GETEDPDCALINFORSPOp: sqIdx %u\n", pGETEDPDCALINFORSPParms->sqIdx[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("GETEDPDCALINFORSPOp: rxGain %u\n", pGETEDPDCALINFORSPParms->rxGain[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("GETEDPDCALINFORSPOp: dcI %u\n", pGETEDPDCALINFORSPParms->dcI[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("GETEDPDCALINFORSPOp: dcQ %u\n", pGETEDPDCALINFORSPParms->dcQ[i]);
    }
#endif //_DEBUG

    if (NULL != GETEDPDCALINFORSPOpFunc) {
        (*GETEDPDCALINFORSPOpFunc)(pGETEDPDCALINFORSPParms);
    }
    return(TRUE);
}
