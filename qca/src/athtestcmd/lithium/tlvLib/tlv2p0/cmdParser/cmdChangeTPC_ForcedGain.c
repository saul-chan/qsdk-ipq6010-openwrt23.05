/* Copyright (c) 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

// This is an auto-generated file from input/cmdChangeTPC_ForcedGain.s
#include "tlv2Inc.h"
#include "cmdChangeTPC_ForcedGain.h"

void* initCHANGETPC_FORCEDGAINOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_CHANGETPC_FORCEDGAIN_PARMS  *pCHANGETPC_FORCEDGAINParms = (CMD_CHANGETPC_FORCEDGAIN_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pCHANGETPC_FORCEDGAINParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pCHANGETPC_FORCEDGAINParms->gainIdx = pParmDict[PARM_GAINIDX].v.valU8;
    pCHANGETPC_FORCEDGAINParms->dacGain = pParmDict[PARM_DACGAIN].v.valS8;
    pCHANGETPC_FORCEDGAINParms->alutOffset = pParmDict[PARM_ALUTOFFSET].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pCHANGETPC_FORCEDGAINParms->phyId)) - (size_t)pCHANGETPC_FORCEDGAINParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GAINIDX, (size_t)(((size_t)&(pCHANGETPC_FORCEDGAINParms->gainIdx)) - (size_t)pCHANGETPC_FORCEDGAINParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DACGAIN, (size_t)(((size_t)&(pCHANGETPC_FORCEDGAINParms->dacGain)) - (size_t)pCHANGETPC_FORCEDGAINParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ALUTOFFSET, (size_t)(((size_t)&(pCHANGETPC_FORCEDGAINParms->alutOffset)) - (size_t)pCHANGETPC_FORCEDGAINParms), pParmsOffset);
    return((void*) pCHANGETPC_FORCEDGAINParms);
}

static CHANGETPC_FORCEDGAIN_OP_FUNC CHANGETPC_FORCEDGAINOpFunc = NULL;

TLV2_API void registerCHANGETPC_FORCEDGAINHandler(CHANGETPC_FORCEDGAIN_OP_FUNC fp)
{
    CHANGETPC_FORCEDGAINOpFunc = fp;
}

A_BOOL CHANGETPC_FORCEDGAINOp(void *pParms)
{
    CMD_CHANGETPC_FORCEDGAIN_PARMS *pCHANGETPC_FORCEDGAINParms = (CMD_CHANGETPC_FORCEDGAIN_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("CHANGETPC_FORCEDGAINOp: phyId %u\n", pCHANGETPC_FORCEDGAINParms->phyId);
    A_PRINTF("CHANGETPC_FORCEDGAINOp: gainIdx %u\n", pCHANGETPC_FORCEDGAINParms->gainIdx);
    A_PRINTF("CHANGETPC_FORCEDGAINOp: dacGain %d\n", pCHANGETPC_FORCEDGAINParms->dacGain);
    A_PRINTF("CHANGETPC_FORCEDGAINOp: alutOffset %u\n", pCHANGETPC_FORCEDGAINParms->alutOffset);
#endif //_DEBUG

    if (NULL != CHANGETPC_FORCEDGAINOpFunc) {
        (*CHANGETPC_FORCEDGAINOpFunc)(pCHANGETPC_FORCEDGAINParms);
    }
    return(TRUE);
}
