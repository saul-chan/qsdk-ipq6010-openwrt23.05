/* Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdGetTgtPwr.s
#include "tlv2Inc.h"
#include "cmdGetTgtPwr.h"

void* initGETTGTPWROpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETTGTPWR_PARMS  *pGETTGTPWRParms = (CMD_GETTGTPWR_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGETTGTPWRParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pGETTGTPWRParms->userMode = pParmDict[PARM_USERMODE].v.valU8;
    pGETTGTPWRParms->freq = pParmDict[PARM_FREQ].v.valU16;
    pGETTGTPWRParms->rateBit = pParmDict[PARM_RATEBIT].v.valU32;
    pGETTGTPWRParms->nss = pParmDict[PARM_NSS].v.valU32;
    pGETTGTPWRParms->rate = pParmDict[PARM_RATE].v.valU8;
    pGETTGTPWRParms->ofdmaPpduType = pParmDict[PARM_OFDMAPPDUTYPE].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGETTGTPWRParms->phyId)) - (size_t)pGETTGTPWRParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_USERMODE, (size_t)(((size_t)&(pGETTGTPWRParms->userMode)) - (size_t)pGETTGTPWRParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ, (size_t)(((size_t)&(pGETTGTPWRParms->freq)) - (size_t)pGETTGTPWRParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEBIT, (size_t)(((size_t)&(pGETTGTPWRParms->rateBit)) - (size_t)pGETTGTPWRParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NSS, (size_t)(((size_t)&(pGETTGTPWRParms->nss)) - (size_t)pGETTGTPWRParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATE, (size_t)(((size_t)&(pGETTGTPWRParms->rate)) - (size_t)pGETTGTPWRParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_OFDMAPPDUTYPE, (size_t)(((size_t)&(pGETTGTPWRParms->ofdmaPpduType)) - (size_t)pGETTGTPWRParms), pParmsOffset);
    return((void*) pGETTGTPWRParms);
}

static GETTGTPWR_OP_FUNC GETTGTPWROpFunc = NULL;

TLV2_API void registerGETTGTPWRHandler(GETTGTPWR_OP_FUNC fp)
{
    GETTGTPWROpFunc = fp;
}

A_BOOL GETTGTPWROp(void *pParms)
{
    CMD_GETTGTPWR_PARMS *pGETTGTPWRParms = (CMD_GETTGTPWR_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GETTGTPWROp: phyId %u\n", pGETTGTPWRParms->phyId);
    A_PRINTF("GETTGTPWROp: userMode %u\n", pGETTGTPWRParms->userMode);
    A_PRINTF("GETTGTPWROp: freq %u\n", pGETTGTPWRParms->freq);
    A_PRINTF("GETTGTPWROp: rateBit %u\n", pGETTGTPWRParms->rateBit);
    A_PRINTF("GETTGTPWROp: nss %u\n", pGETTGTPWRParms->nss);
    A_PRINTF("GETTGTPWROp: rate %u\n", pGETTGTPWRParms->rate);
    A_PRINTF("GETTGTPWROp: ofdmaPpduType %u\n", pGETTGTPWRParms->ofdmaPpduType);
#endif //_DEBUG

    if (NULL != GETTGTPWROpFunc) {
        (*GETTGTPWROpFunc)(pGETTGTPWRParms);
    }
    return(TRUE);
}

void* initGETTGTPWRRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETTGTPWRRSP_PARMS  *pGETTGTPWRRSPParms = (CMD_GETTGTPWRRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGETTGTPWRRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pGETTGTPWRRSPParms->tgtPwr = pParmDict[PARM_TGTPWR].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGETTGTPWRRSPParms->phyId)) - (size_t)pGETTGTPWRRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TGTPWR, (size_t)(((size_t)&(pGETTGTPWRRSPParms->tgtPwr)) - (size_t)pGETTGTPWRRSPParms), pParmsOffset);
    return((void*) pGETTGTPWRRSPParms);
}

static GETTGTPWRRSP_OP_FUNC GETTGTPWRRSPOpFunc = NULL;

TLV2_API void registerGETTGTPWRRSPHandler(GETTGTPWRRSP_OP_FUNC fp)
{
    GETTGTPWRRSPOpFunc = fp;
}

A_BOOL GETTGTPWRRSPOp(void *pParms)
{
    CMD_GETTGTPWRRSP_PARMS *pGETTGTPWRRSPParms = (CMD_GETTGTPWRRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GETTGTPWRRSPOp: phyId %u\n", pGETTGTPWRRSPParms->phyId);
    A_PRINTF("GETTGTPWRRSPOp: tgtPwr %u\n", pGETTGTPWRRSPParms->tgtPwr);
#endif //_DEBUG

    if (NULL != GETTGTPWRRSPOpFunc) {
        (*GETTGTPWRRSPOpFunc)(pGETTGTPWRRSPParms);
    }
    return(TRUE);
}
