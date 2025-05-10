/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

// This is an auto-generated file from input/cmdSetTPC_CalFreq.s
#include "tlv2Inc.h"
#include "cmdSetTPC_CalFreq.h"

void* initSETTPC_CALFREQOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_SETTPC_CALFREQ_PARMS  *pSETTPC_CALFREQParms = (CMD_SETTPC_CALFREQ_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pSETTPC_CALFREQParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pSETTPC_CALFREQParms->chanIdx = pParmDict[PARM_CHANIDX].v.valU8;
    pSETTPC_CALFREQParms->freq = pParmDict[PARM_FREQ].v.valU16;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pSETTPC_CALFREQParms->phyId)) - (size_t)pSETTPC_CALFREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHANIDX, (size_t)(((size_t)&(pSETTPC_CALFREQParms->chanIdx)) - (size_t)pSETTPC_CALFREQParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ, (size_t)(((size_t)&(pSETTPC_CALFREQParms->freq)) - (size_t)pSETTPC_CALFREQParms), pParmsOffset);
    return((void*) pSETTPC_CALFREQParms);
}

static SETTPC_CALFREQ_OP_FUNC SETTPC_CALFREQOpFunc = NULL;

TLV2_API void registerSETTPC_CALFREQHandler(SETTPC_CALFREQ_OP_FUNC fp)
{
    SETTPC_CALFREQOpFunc = fp;
}

A_BOOL SETTPC_CALFREQOp(void *pParms)
{
    CMD_SETTPC_CALFREQ_PARMS *pSETTPC_CALFREQParms = (CMD_SETTPC_CALFREQ_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("SETTPC_CALFREQOp: phyId %u\n", pSETTPC_CALFREQParms->phyId);
    A_PRINTF("SETTPC_CALFREQOp: chanIdx %u\n", pSETTPC_CALFREQParms->chanIdx);
    A_PRINTF("SETTPC_CALFREQOp: freq %u\n", pSETTPC_CALFREQParms->freq);
#endif //_DEBUG

    if (NULL != SETTPC_CALFREQOpFunc) {
        (*SETTPC_CALFREQOpFunc)(pSETTPC_CALFREQParms);
    }
    return(TRUE);
}
