/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdDPDTrainingQuality.s
#include "tlv2Inc.h"
#include "cmdDPDTrainingQuality.h"

void* initDPDTRAININGQUALITYOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_DPDTRAININGQUALITY_PARMS  *pDPDTRAININGQUALITYParms = (CMD_DPDTRAININGQUALITY_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pDPDTRAININGQUALITYParms->phyId = pParmDict[PARM_PHYID].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pDPDTRAININGQUALITYParms->phyId)) - (size_t)pDPDTRAININGQUALITYParms), pParmsOffset);
    return((void*) pDPDTRAININGQUALITYParms);
}

static DPDTRAININGQUALITY_OP_FUNC DPDTRAININGQUALITYOpFunc = NULL;

TLV2_API void registerDPDTRAININGQUALITYHandler(DPDTRAININGQUALITY_OP_FUNC fp)
{
    DPDTRAININGQUALITYOpFunc = fp;
}

A_BOOL DPDTRAININGQUALITYOp(void *pParms)
{
    CMD_DPDTRAININGQUALITY_PARMS *pDPDTRAININGQUALITYParms = (CMD_DPDTRAININGQUALITY_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("DPDTRAININGQUALITYOp: phyId %u\n", pDPDTRAININGQUALITYParms->phyId);
#endif //_DEBUG

    if (NULL != DPDTRAININGQUALITYOpFunc) {
        (*DPDTRAININGQUALITYOpFunc)(pDPDTRAININGQUALITYParms);
    }
    return(TRUE);
}

void* initDPDTRAININGQUALITYRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_DPDTRAININGQUALITYRSP_PARMS  *pDPDTRAININGQUALITYRSPParms = (CMD_DPDTRAININGQUALITYRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pDPDTRAININGQUALITYRSPParms->DPDTrainQual = pParmDict[PARM_DPDTRAINQUAL].v.valU16;
    pDPDTRAININGQUALITYRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pDPDTRAININGQUALITYRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_DPDTRAINQUAL, (size_t)(((size_t)&(pDPDTRAININGQUALITYRSPParms->DPDTrainQual)) - (size_t)pDPDTRAININGQUALITYRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pDPDTRAININGQUALITYRSPParms->status)) - (size_t)pDPDTRAININGQUALITYRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pDPDTRAININGQUALITYRSPParms->phyId)) - (size_t)pDPDTRAININGQUALITYRSPParms), pParmsOffset);
    return((void*) pDPDTRAININGQUALITYRSPParms);
}

static DPDTRAININGQUALITYRSP_OP_FUNC DPDTRAININGQUALITYRSPOpFunc = NULL;

TLV2_API void registerDPDTRAININGQUALITYRSPHandler(DPDTRAININGQUALITYRSP_OP_FUNC fp)
{
    DPDTRAININGQUALITYRSPOpFunc = fp;
}

A_BOOL DPDTRAININGQUALITYRSPOp(void *pParms)
{
    CMD_DPDTRAININGQUALITYRSP_PARMS *pDPDTRAININGQUALITYRSPParms = (CMD_DPDTRAININGQUALITYRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("DPDTRAININGQUALITYRSPOp: DPDTrainQual %u\n", pDPDTRAININGQUALITYRSPParms->DPDTrainQual);
    A_PRINTF("DPDTRAININGQUALITYRSPOp: status %u\n", pDPDTRAININGQUALITYRSPParms->status);
    A_PRINTF("DPDTRAININGQUALITYRSPOp: phyId %u\n", pDPDTRAININGQUALITYRSPParms->phyId);
#endif //_DEBUG

    if (NULL != DPDTRAININGQUALITYRSPOpFunc) {
        (*DPDTRAININGQUALITYRSPOpFunc)(pDPDTRAININGQUALITYRSPParms);
    }
    return(TRUE);
}
