/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdModifyBDFOTPflags.s
#include "tlv2Inc.h"
#include "cmdModifyBdfOtpFlags.h"

void* initMODIFYBDFOTPFLAGSOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_MODIFYBDFOTPFLAGS_PARMS  *pMODIFYBDFOTPFLAGSParms = (CMD_MODIFYBDFOTPFLAGS_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pMODIFYBDFOTPFLAGSParms->modifyBdfOtpFlags = pParmDict[PARM_MODIFYBDFOTPFLAGS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_MODIFYBDFOTPFLAGS, (size_t)(((size_t)&(pMODIFYBDFOTPFLAGSParms->modifyBdfOtpFlags)) - (size_t)pMODIFYBDFOTPFLAGSParms), pParmsOffset);
    return((void*) pMODIFYBDFOTPFLAGSParms);
}

static MODIFYBDFOTPFLAGS_OP_FUNC MODIFYBDFOTPFLAGSOpFunc = NULL;

TLV2_API void registerMODIFYBDFOTPFLAGSHandler(MODIFYBDFOTPFLAGS_OP_FUNC fp)
{
    MODIFYBDFOTPFLAGSOpFunc = fp;
}

A_BOOL MODIFYBDFOTPFLAGSOp(void *pParms)
{
    CMD_MODIFYBDFOTPFLAGS_PARMS *pMODIFYBDFOTPFLAGSParms = (CMD_MODIFYBDFOTPFLAGS_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("MODIFYBDFOTPFLAGSOp: modifyBdfOtpFlags %u\n", pMODIFYBDFOTPFLAGSParms->modifyBdfOtpFlags);
#endif //_DEBUG

    if (NULL != MODIFYBDFOTPFLAGSOpFunc) {
        (*MODIFYBDFOTPFLAGSOpFunc)(pMODIFYBDFOTPFLAGSParms);
    }
    return(TRUE);
}
