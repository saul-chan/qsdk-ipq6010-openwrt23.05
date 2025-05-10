/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdConfigHandler.s
#include "tlv2Inc.h"
#include "cmdConfigHandler.h"

void* initCONFIGOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_CONFIG_PARMS  *pCONFIGParms = (CMD_CONFIG_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pCONFIGParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pCONFIGParms->wlanMode = pParmDict[PARM_WLANMODE].v.valU8;
    pCONFIGParms->freq = pParmDict[PARM_FREQ].v.valU16;
    pCONFIGParms->freq2 = pParmDict[PARM_FREQ2].v.valU16;
    pCONFIGParms->bandwidth = pParmDict[PARM_BANDWIDTH].v.valU8;
    pCONFIGParms->rate = pParmDict[PARM_RATE].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pCONFIGParms->phyId)) - (size_t)pCONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_WLANMODE, (size_t)(((size_t)&(pCONFIGParms->wlanMode)) - (size_t)pCONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ, (size_t)(((size_t)&(pCONFIGParms->freq)) - (size_t)pCONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ2, (size_t)(((size_t)&(pCONFIGParms->freq2)) - (size_t)pCONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BANDWIDTH, (size_t)(((size_t)&(pCONFIGParms->bandwidth)) - (size_t)pCONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATE, (size_t)(((size_t)&(pCONFIGParms->rate)) - (size_t)pCONFIGParms), pParmsOffset);
    return((void*) pCONFIGParms);
}

static CONFIG_OP_FUNC CONFIGOpFunc = NULL;

TLV2_API void registerCONFIGHandler(CONFIG_OP_FUNC fp)
{
    CONFIGOpFunc = fp;
}

A_BOOL CONFIGOp(void *pParms)
{
    CMD_CONFIG_PARMS *pCONFIGParms = (CMD_CONFIG_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("CONFIGOp: phyId %u\n", pCONFIGParms->phyId);
    A_PRINTF("CONFIGOp: wlanMode %u\n", pCONFIGParms->wlanMode);
    A_PRINTF("CONFIGOp: freq %u\n", pCONFIGParms->freq);
    A_PRINTF("CONFIGOp: freq2 %u\n", pCONFIGParms->freq2);
    A_PRINTF("CONFIGOp: bandwidth %u\n", pCONFIGParms->bandwidth);
    A_PRINTF("CONFIGOp: rate %u\n", pCONFIGParms->rate);
#endif //_DEBUG

    if (NULL != CONFIGOpFunc) {
        (*CONFIGOpFunc)(pCONFIGParms);
    }
    return(TRUE);
}
