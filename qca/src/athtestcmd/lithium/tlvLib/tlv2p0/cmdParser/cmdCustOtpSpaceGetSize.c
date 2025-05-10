/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdCustOtpSpaceGetSize.s
#include "tlv2Inc.h"
#include "cmdCustOtpSpaceGetSize.h"

void* initGETCUSTOTPSIZEOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    return(NULL);
}

static GETCUSTOTPSIZE_OP_FUNC GETCUSTOTPSIZEOpFunc = NULL;

TLV2_API void registerGETCUSTOTPSIZEHandler(GETCUSTOTPSIZE_OP_FUNC fp)
{
    GETCUSTOTPSIZEOpFunc = fp;
}

A_BOOL GETCUSTOTPSIZEOp(void *pParms)
{
    if (NULL != GETCUSTOTPSIZEOpFunc) {
        (*GETCUSTOTPSIZEOpFunc)(NULL);
    }
    return(TRUE);
}

void* initGETCUSTOTPSIZERSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETCUSTOTPSIZERSP_PARMS  *pGETCUSTOTPSIZERSPParms = (CMD_GETCUSTOTPSIZERSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGETCUSTOTPSIZERSPParms->otpCustSize = pParmDict[PARM_OTPCUSTSIZE].v.valU32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_OTPCUSTSIZE, (size_t)(((size_t)&(pGETCUSTOTPSIZERSPParms->otpCustSize)) - (size_t)pGETCUSTOTPSIZERSPParms), pParmsOffset);
    return((void*) pGETCUSTOTPSIZERSPParms);
}

static GETCUSTOTPSIZERSP_OP_FUNC GETCUSTOTPSIZERSPOpFunc = NULL;

TLV2_API void registerGETCUSTOTPSIZERSPHandler(GETCUSTOTPSIZERSP_OP_FUNC fp)
{
    GETCUSTOTPSIZERSPOpFunc = fp;
}

A_BOOL GETCUSTOTPSIZERSPOp(void *pParms)
{
    CMD_GETCUSTOTPSIZERSP_PARMS *pGETCUSTOTPSIZERSPParms = (CMD_GETCUSTOTPSIZERSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GETCUSTOTPSIZERSPOp: otpCustSize %u\n", pGETCUSTOTPSIZERSPParms->otpCustSize);
#endif //_DEBUG

    if (NULL != GETCUSTOTPSIZERSPOpFunc) {
        (*GETCUSTOTPSIZERSPOpFunc)(pGETCUSTOTPSIZERSPParms);
    }
    return(TRUE);
}
