/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

// This is an auto-generated file from input/cmdTPCCal_PostProcess.s
#include "tlv2Inc.h"
#include "cmdTPCCal_PostProcess.h"

void* initTPCCAL_POSTPROCESSOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_TPCCAL_POSTPROCESS_PARMS  *pTPCCAL_POSTPROCESSParms = (CMD_TPCCAL_POSTPROCESS_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pTPCCAL_POSTPROCESSParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pTPCCAL_POSTPROCESSParms->chanIdx = pParmDict[PARM_CHANIDX].v.valU8;
    pTPCCAL_POSTPROCESSParms->chainIdx = pParmDict[PARM_CHAINIDX].v.valU8;
    pTPCCAL_POSTPROCESSParms->valuetype = pParmDict[PARM_VALUETYPE].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pTPCCAL_POSTPROCESSParms->phyId)) - (size_t)pTPCCAL_POSTPROCESSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHANIDX, (size_t)(((size_t)&(pTPCCAL_POSTPROCESSParms->chanIdx)) - (size_t)pTPCCAL_POSTPROCESSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINIDX, (size_t)(((size_t)&(pTPCCAL_POSTPROCESSParms->chainIdx)) - (size_t)pTPCCAL_POSTPROCESSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_VALUETYPE, (size_t)(((size_t)&(pTPCCAL_POSTPROCESSParms->valuetype)) - (size_t)pTPCCAL_POSTPROCESSParms), pParmsOffset);
    return((void*) pTPCCAL_POSTPROCESSParms);
}

static TPCCAL_POSTPROCESS_OP_FUNC TPCCAL_POSTPROCESSOpFunc = NULL;

TLV2_API void registerTPCCAL_POSTPROCESSHandler(TPCCAL_POSTPROCESS_OP_FUNC fp)
{
    TPCCAL_POSTPROCESSOpFunc = fp;
}

A_BOOL TPCCAL_POSTPROCESSOp(void *pParms)
{
    CMD_TPCCAL_POSTPROCESS_PARMS *pTPCCAL_POSTPROCESSParms = (CMD_TPCCAL_POSTPROCESS_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TPCCAL_POSTPROCESSOp: phyId %u\n", pTPCCAL_POSTPROCESSParms->phyId);
    A_PRINTF("TPCCAL_POSTPROCESSOp: chanIdx %u\n", pTPCCAL_POSTPROCESSParms->chanIdx);
    A_PRINTF("TPCCAL_POSTPROCESSOp: chainIdx %u\n", pTPCCAL_POSTPROCESSParms->chainIdx);
    A_PRINTF("TPCCAL_POSTPROCESSOp: valuetype %u\n", pTPCCAL_POSTPROCESSParms->valuetype);
#endif //_DEBUG

    if (NULL != TPCCAL_POSTPROCESSOpFunc) {
        (*TPCCAL_POSTPROCESSOpFunc)(pTPCCAL_POSTPROCESSParms);
    }
    return(TRUE);
}

void* initTPCCAL_POSTPROCESSRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    int i, j; 	//for initializing array parameter
    CMD_TPCCAL_POSTPROCESSRSP_PARMS  *pTPCCAL_POSTPROCESSRSPParms = (CMD_TPCCAL_POSTPROCESSRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);

    i = j = 0;	//assign a number to avoid warning in case i and j are not used

    // Populate the parm structure with initial values
    pTPCCAL_POSTPROCESSRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pTPCCAL_POSTPROCESSRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    memset(pTPCCAL_POSTPROCESSRSPParms->measuredPwr, 0, sizeof(pTPCCAL_POSTPROCESSRSPParms->measuredPwr));
    for (i = 0; i < 2 ; i++)
    {
        pTPCCAL_POSTPROCESSRSPParms->measuredPwr[i] = pParmDict[PARM_MEASUREDPWR].v.ptS16[i];
    }
    memset(pTPCCAL_POSTPROCESSRSPParms->pdadc, 0, sizeof(pTPCCAL_POSTPROCESSRSPParms->pdadc));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pTPCCAL_POSTPROCESSRSPParms->phyId)) - (size_t)pTPCCAL_POSTPROCESSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pTPCCAL_POSTPROCESSRSPParms->status)) - (size_t)pTPCCAL_POSTPROCESSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MEASUREDPWR, (size_t)(((size_t)&(pTPCCAL_POSTPROCESSRSPParms->measuredPwr)) - (size_t)pTPCCAL_POSTPROCESSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PDADC, (size_t)(((size_t)&(pTPCCAL_POSTPROCESSRSPParms->pdadc)) - (size_t)pTPCCAL_POSTPROCESSRSPParms), pParmsOffset);
    return((void*) pTPCCAL_POSTPROCESSRSPParms);
}

static TPCCAL_POSTPROCESSRSP_OP_FUNC TPCCAL_POSTPROCESSRSPOpFunc = NULL;

TLV2_API void registerTPCCAL_POSTPROCESSRSPHandler(TPCCAL_POSTPROCESSRSP_OP_FUNC fp)
{
    TPCCAL_POSTPROCESSRSPOpFunc = fp;
}

A_BOOL TPCCAL_POSTPROCESSRSPOp(void *pParms)
{
    CMD_TPCCAL_POSTPROCESSRSP_PARMS *pTPCCAL_POSTPROCESSRSPParms = (CMD_TPCCAL_POSTPROCESSRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TPCCAL_POSTPROCESSRSPOp: phyId %u\n", pTPCCAL_POSTPROCESSRSPParms->phyId);
    A_PRINTF("TPCCAL_POSTPROCESSRSPOp: status %u\n", pTPCCAL_POSTPROCESSRSPParms->status);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 512 entries
    {
        A_PRINTF("TPCCAL_POSTPROCESSRSPOp: measuredPwr %d\n", pTPCCAL_POSTPROCESSRSPParms->measuredPwr[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("TPCCAL_POSTPROCESSRSPOp: pdadc %u\n", pTPCCAL_POSTPROCESSRSPParms->pdadc[i]);
    }
#endif //_DEBUG

    if (NULL != TPCCAL_POSTPROCESSRSPOpFunc) {
        (*TPCCAL_POSTPROCESSRSPOpFunc)(pTPCCAL_POSTPROCESSRSPParms);
    }
    return(TRUE);
}
