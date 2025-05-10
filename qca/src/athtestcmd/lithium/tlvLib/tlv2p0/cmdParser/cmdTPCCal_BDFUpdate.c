/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

// This is an auto-generated file from input/cmdTPCCal_BDFUpdate.s
#include "tlv2Inc.h"
#include "cmdTPCCal_BDFUpdate.h"

void* initTPCCAL_BDFUPDATEOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_TPCCAL_BDFUPDATE_PARMS  *pTPCCAL_BDFUPDATEParms = (CMD_TPCCAL_BDFUPDATE_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pTPCCAL_BDFUPDATEParms->phyId = pParmDict[PARM_PHYID].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pTPCCAL_BDFUPDATEParms->phyId)) - (size_t)pTPCCAL_BDFUPDATEParms), pParmsOffset);
    return((void*) pTPCCAL_BDFUPDATEParms);
}

static TPCCAL_BDFUPDATE_OP_FUNC TPCCAL_BDFUPDATEOpFunc = NULL;

TLV2_API void registerTPCCAL_BDFUPDATEHandler(TPCCAL_BDFUPDATE_OP_FUNC fp)
{
    TPCCAL_BDFUPDATEOpFunc = fp;
}

A_BOOL TPCCAL_BDFUPDATEOp(void *pParms)
{
    CMD_TPCCAL_BDFUPDATE_PARMS *pTPCCAL_BDFUPDATEParms = (CMD_TPCCAL_BDFUPDATE_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("TPCCAL_BDFUPDATEOp: phyId %u\n", pTPCCAL_BDFUPDATEParms->phyId);
#endif //_DEBUG

    if (NULL != TPCCAL_BDFUPDATEOpFunc) {
        (*TPCCAL_BDFUPDATEOpFunc)(pTPCCAL_BDFUPDATEParms);
    }
    return(TRUE);
}
