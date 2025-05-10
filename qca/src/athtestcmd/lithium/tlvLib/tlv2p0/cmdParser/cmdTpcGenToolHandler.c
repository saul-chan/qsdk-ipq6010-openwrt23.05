/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input\cmdTpcGenToolHandler.s
#include "tlv2Inc.h"
#include "cmdTpcGenToolHandler.h"

void* initTPCGENTOOLOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_TPCGENTOOL_PARMS  *pTPCGENTOOLParms = (CMD_TPCGENTOOL_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pTPCGENTOOLParms->countryCode = pParmDict[PARM_COUNTRYCODE].v.valU32;
    pTPCGENTOOLParms->regDomain = pParmDict[PARM_REGDOMAIN].v.valU32;
    pTPCGENTOOLParms->ctl = pParmDict[PARM_CTL].v.valS32;
    pTPCGENTOOLParms->sarIndex = pParmDict[PARM_SARINDEX].v.valS8;
    pTPCGENTOOLParms->phyMode = pParmDict[PARM_PHYMODE].v.valU8;
    pTPCGENTOOLParms->beamforming = pParmDict[PARM_BEAMFORMING].v.valU8;
    pTPCGENTOOLParms->ctlrd = pParmDict[PARM_CTLRD].v.valU8;
    pTPCGENTOOLParms->freq = pParmDict[PARM_FREQ].v.valU16;
    pTPCGENTOOLParms->chainIdx = pParmDict[PARM_CHAINIDX].v.valU8;
    pTPCGENTOOLParms->chainMask = pParmDict[PARM_CHAINMASK].v.valU8;
    pTPCGENTOOLParms->rate = pParmDict[PARM_RATE].v.valU16;
    pTPCGENTOOLParms->nss = pParmDict[PARM_NSS].v.valU32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_COUNTRYCODE, (size_t)(((size_t)&(pTPCGENTOOLParms->countryCode)) - (size_t)pTPCGENTOOLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_REGDOMAIN, (size_t)(((size_t)&(pTPCGENTOOLParms->regDomain)) - (size_t)pTPCGENTOOLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CTL, (size_t)(((size_t)&(pTPCGENTOOLParms->ctl)) - (size_t)pTPCGENTOOLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SARINDEX, (size_t)(((size_t)&(pTPCGENTOOLParms->sarIndex)) - (size_t)pTPCGENTOOLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYMODE, (size_t)(((size_t)&(pTPCGENTOOLParms->phyMode)) - (size_t)pTPCGENTOOLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BEAMFORMING, (size_t)(((size_t)&(pTPCGENTOOLParms->beamforming)) - (size_t)pTPCGENTOOLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CTLRD, (size_t)(((size_t)&(pTPCGENTOOLParms->ctlrd)) - (size_t)pTPCGENTOOLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ, (size_t)(((size_t)&(pTPCGENTOOLParms->freq)) - (size_t)pTPCGENTOOLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINIDX, (size_t)(((size_t)&(pTPCGENTOOLParms->chainIdx)) - (size_t)pTPCGENTOOLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINMASK, (size_t)(((size_t)&(pTPCGENTOOLParms->chainMask)) - (size_t)pTPCGENTOOLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATE, (size_t)(((size_t)&(pTPCGENTOOLParms->rate)) - (size_t)pTPCGENTOOLParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NSS, (size_t)(((size_t)&(pTPCGENTOOLParms->nss)) - (size_t)pTPCGENTOOLParms), pParmsOffset);
    return((void*) pTPCGENTOOLParms);
}

static TPCGENTOOL_OP_FUNC TPCGENTOOLOpFunc = NULL;

TLV2_API void registerTPCGENTOOLHandler(TPCGENTOOL_OP_FUNC fp)
{
    TPCGENTOOLOpFunc = fp;
}

A_BOOL TPCGENTOOLOp(void *pParms)
{
    CMD_TPCGENTOOL_PARMS *pTPCGENTOOLParms = (CMD_TPCGENTOOL_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TPCGENTOOLOp: countryCode %u\n", pTPCGENTOOLParms->countryCode);
    A_PRINTF("TPCGENTOOLOp: regDomain %u\n", pTPCGENTOOLParms->regDomain);
    A_PRINTF("TPCGENTOOLOp: ctl %u\n", pTPCGENTOOLParms->ctl);
    A_PRINTF("TPCGENTOOLOp: sarIndex %u\n", pTPCGENTOOLParms->sarIndex);
    A_PRINTF("TPCGENTOOLOp: phyMode %u\n", pTPCGENTOOLParms->phyMode);
    A_PRINTF("TPCGENTOOLOp: beamforming %u\n", pTPCGENTOOLParms->beamforming);
    A_PRINTF("TPCGENTOOLOp: ctlrd %u\n", pTPCGENTOOLParms->ctlrd);
    A_PRINTF("TPCGENTOOLOp: freq %u\n", pTPCGENTOOLParms->freq);
    A_PRINTF("TPCGENTOOLOp: chainIdx %u\n", pTPCGENTOOLParms->chainIdx);
    A_PRINTF("TPCGENTOOLOp: chainMask %u\n", pTPCGENTOOLParms->chainMask);
    A_PRINTF("TPCGENTOOLOp: rate %u\n", pTPCGENTOOLParms->rate);
    A_PRINTF("TPCGENTOOLOp: nss %u\n", pTPCGENTOOLParms->nss);
#endif //_DEBUG

    if (NULL != TPCGENTOOLOpFunc) {
        (*TPCGENTOOLOpFunc)(pTPCGENTOOLParms);
    }
    return(TRUE);
}

void* initTPCGENTOOLRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_TPCGENTOOLRSP_PARMS  *pTPCGENTOOLRSPParms = (CMD_TPCGENTOOLRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pTPCGENTOOLRSPParms->tpcPower = pParmDict[PARM_TPCPOWER].v.valU8;
    pTPCGENTOOLRSPParms->RegDmnFW = pParmDict[PARM_REGDMNFW].v.valU8;
    pTPCGENTOOLRSPParms->status = pParmDict[PARM_STATUS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_TPCPOWER, (size_t)(((size_t)&(pTPCGENTOOLRSPParms->tpcPower)) - (size_t)pTPCGENTOOLRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_REGDMNFW, (size_t)(((size_t)&(pTPCGENTOOLRSPParms->RegDmnFW)) - (size_t)pTPCGENTOOLRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pTPCGENTOOLRSPParms->status)) - (size_t)pTPCGENTOOLRSPParms), pParmsOffset);
    return((void*) pTPCGENTOOLRSPParms);
}

static TPCGENTOOLRSP_OP_FUNC TPCGENTOOLRSPOpFunc = NULL;

TLV2_API void registerTPCGENTOOLRSPHandler(TPCGENTOOLRSP_OP_FUNC fp)
{
    TPCGENTOOLRSPOpFunc = fp;
}

A_BOOL TPCGENTOOLRSPOp(void *pParms)
{
    CMD_TPCGENTOOLRSP_PARMS *pTPCGENTOOLRSPParms = (CMD_TPCGENTOOLRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TPCGENTOOLRSPOp: tpcPower %u\n", pTPCGENTOOLRSPParms->tpcPower);
    A_PRINTF("TPCGENTOOLRSPOp: RegDmnFW %u\n", pTPCGENTOOLRSPParms->RegDmnFW);
    A_PRINTF("TPCGENTOOLRSPOp: status %u\n", pTPCGENTOOLRSPParms->status);
#endif //_DEBUG

    if (NULL != TPCGENTOOLRSPOpFunc) {
        (*TPCGENTOOLRSPOpFunc)(pTPCGENTOOLRSPParms);
    }
    return(TRUE);
}
