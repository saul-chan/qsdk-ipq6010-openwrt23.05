/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from ./input/cmdAdcCaptureHandler.s
#include "tlv2Inc.h"
#include "cmdAdcCaptureHandler.h"

void* initADCCAPTUREOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_ADCCAPTURE_PARMS  *pADCCAPTUREParms = (CMD_ADCCAPTURE_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pADCCAPTUREParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pADCCAPTUREParms->adcCompMode = pParmDict[PARM_ADCCOMPMODE].v.valU8;
    pADCCAPTUREParms->chainMask = pParmDict[PARM_CHAINMASK].v.valU16;
    pADCCAPTUREParms->adcTriggerMode = pParmDict[PARM_ADCTRIGGERMODE].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pADCCAPTUREParms->phyId)) - (size_t)pADCCAPTUREParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ADCCOMPMODE, (size_t)(((size_t)&(pADCCAPTUREParms->adcCompMode)) - (size_t)pADCCAPTUREParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINMASK, (size_t)(((size_t)&(pADCCAPTUREParms->chainMask)) - (size_t)pADCCAPTUREParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ADCTRIGGERMODE, (size_t)(((size_t)&(pADCCAPTUREParms->adcTriggerMode)) - (size_t)pADCCAPTUREParms), pParmsOffset);
    return((void*) pADCCAPTUREParms);
}

static ADCCAPTURE_OP_FUNC ADCCAPTUREOpFunc = NULL;

TLV2_API void registerADCCAPTUREHandler(ADCCAPTURE_OP_FUNC fp)
{
    ADCCAPTUREOpFunc = fp;
}

A_BOOL ADCCAPTUREOp(void *pParms)
{
    CMD_ADCCAPTURE_PARMS *pADCCAPTUREParms = (CMD_ADCCAPTURE_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("ADCCAPTUREOp: phyId %u\n", pADCCAPTUREParms->phyId);
    A_PRINTF("ADCCAPTUREOp: adcCompMode %u\n", pADCCAPTUREParms->adcCompMode);
    A_PRINTF("ADCCAPTUREOp: chainMask 0x%x\n", pADCCAPTUREParms->chainMask);
    A_PRINTF("ADCCAPTUREOp: adcTriggerMode %u\n", pADCCAPTUREParms->adcTriggerMode);
#endif //_DEBUG

    if (NULL != ADCCAPTUREOpFunc) {
        (*ADCCAPTUREOpFunc)(pADCCAPTUREParms);
    }
    return(TRUE);
}

void* initADCCAPTURERSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_ADCCAPTURERSP_PARMS  *pADCCAPTURERSPParms = (CMD_ADCCAPTURERSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pADCCAPTURERSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pADCCAPTURERSPParms->status = pParmDict[PARM_STATUS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pADCCAPTURERSPParms->phyId)) - (size_t)pADCCAPTURERSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pADCCAPTURERSPParms->status)) - (size_t)pADCCAPTURERSPParms), pParmsOffset);
    return((void*) pADCCAPTURERSPParms);
}

static ADCCAPTURERSP_OP_FUNC ADCCAPTURERSPOpFunc = NULL;

TLV2_API void registerADCCAPTURERSPHandler(ADCCAPTURERSP_OP_FUNC fp)
{
    ADCCAPTURERSPOpFunc = fp;
}

A_BOOL ADCCAPTURERSPOp(void *pParms)
{
    CMD_ADCCAPTURERSP_PARMS *pADCCAPTURERSPParms = (CMD_ADCCAPTURERSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("ADCCAPTURERSPOp: phyId %u\n", pADCCAPTURERSPParms->phyId);
    A_PRINTF("ADCCAPTURERSPOp: status %u\n", pADCCAPTURERSPParms->status);
#endif //_DEBUG

    if (NULL != ADCCAPTURERSPOpFunc) {
        (*ADCCAPTURERSPOpFunc)(pADCCAPTURERSPParms);
    }
    return(TRUE);
}
