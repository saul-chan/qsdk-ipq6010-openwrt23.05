/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input\cmdInvalidateCalDataOTPHandler.s
#include "tlv2Inc.h"
#include "cmdInvalidateCalDataOTPHandler.h"

void* initINVALIDATECALDATAOTPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_INVALIDATECALDATAOTP_PARMS  *pINVALIDATECALDATAOTPParms = (CMD_INVALIDATECALDATAOTP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pINVALIDATECALDATAOTPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pINVALIDATECALDATAOTPParms->sectionId = pParmDict[PARM_SECTIONID].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pINVALIDATECALDATAOTPParms->phyId)) - (size_t)pINVALIDATECALDATAOTPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SECTIONID, (size_t)(((size_t)&(pINVALIDATECALDATAOTPParms->sectionId)) - (size_t)pINVALIDATECALDATAOTPParms), pParmsOffset);
    return((void*) pINVALIDATECALDATAOTPParms);
}

static INVALIDATECALDATAOTP_OP_FUNC INVALIDATECALDATAOTPOpFunc = NULL;

TLV2_API void registerINVALIDATECALDATAOTPHandler(INVALIDATECALDATAOTP_OP_FUNC fp)
{
    INVALIDATECALDATAOTPOpFunc = fp;
}

A_BOOL INVALIDATECALDATAOTPOp(void *pParms)
{
    CMD_INVALIDATECALDATAOTP_PARMS *pINVALIDATECALDATAOTPParms = (CMD_INVALIDATECALDATAOTP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("INVALIDATECALDATAOTPOp: phyId %u\n", pINVALIDATECALDATAOTPParms->phyId);
    A_PRINTF("INVALIDATECALDATAOTPOp: sectionId %u\n", pINVALIDATECALDATAOTPParms->sectionId);
#endif //_DEBUG

    if (NULL != INVALIDATECALDATAOTPOpFunc) {
        (*INVALIDATECALDATAOTPOpFunc)(pINVALIDATECALDATAOTPParms);
    }
    return(TRUE);
}

void* initINVALIDATECALDATAOTPRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_INVALIDATECALDATAOTPRSP_PARMS  *pINVALIDATECALDATAOTPRSPParms = (CMD_INVALIDATECALDATAOTPRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pINVALIDATECALDATAOTPRSPParms->status = pParmDict[PARM_STATUS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pINVALIDATECALDATAOTPRSPParms->status)) - (size_t)pINVALIDATECALDATAOTPRSPParms), pParmsOffset);
    return((void*) pINVALIDATECALDATAOTPRSPParms);
}

static INVALIDATECALDATAOTPRSP_OP_FUNC INVALIDATECALDATAOTPRSPOpFunc = NULL;

TLV2_API void registerINVALIDATECALDATAOTPRSPHandler(INVALIDATECALDATAOTPRSP_OP_FUNC fp)
{
    INVALIDATECALDATAOTPRSPOpFunc = fp;
}

A_BOOL INVALIDATECALDATAOTPRSPOp(void *pParms)
{
    CMD_INVALIDATECALDATAOTPRSP_PARMS *pINVALIDATECALDATAOTPRSPParms = (CMD_INVALIDATECALDATAOTPRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("INVALIDATECALDATAOTPRSPOp: status %u\n", pINVALIDATECALDATAOTPRSPParms->status);
#endif //_DEBUG

    if (NULL != INVALIDATECALDATAOTPRSPOpFunc) {
        (*INVALIDATECALDATAOTPRSPOpFunc)(pINVALIDATECALDATAOTPRSPParms);
    }
    return(TRUE);
}
