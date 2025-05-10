/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

// This is an auto-generated file from input/cmdSetBDFversion.s
#include "tlv2Inc.h"
#include "cmdSetBDFversion.h"

void* initSETBDFVERSIONOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_SETBDFVERSION_PARMS  *pSETBDFVERSIONParms = (CMD_SETBDFVERSION_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pSETBDFVERSIONParms->bdfVerType = pParmDict[PARM_BDFVERTYPE].v.valU8;
    memset(pSETBDFVERSIONParms->ver_info, 0, sizeof(pSETBDFVERSIONParms->ver_info));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_BDFVERTYPE, (size_t)(((size_t)&(pSETBDFVERSIONParms->bdfVerType)) - (size_t)pSETBDFVERSIONParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_VER_INFO, (size_t)(((size_t)&(pSETBDFVERSIONParms->ver_info)) - (size_t)pSETBDFVERSIONParms), pParmsOffset);
    return((void*) pSETBDFVERSIONParms);
}

static SETBDFVERSION_OP_FUNC SETBDFVERSIONOpFunc = NULL;

TLV2_API void registerSETBDFVERSIONHandler(SETBDFVERSION_OP_FUNC fp)
{
    SETBDFVERSIONOpFunc = fp;
}

A_BOOL SETBDFVERSIONOp(void *pParms)
{
    CMD_SETBDFVERSION_PARMS *pSETBDFVERSIONParms = (CMD_SETBDFVERSION_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("SETBDFVERSIONOp: bdfVerType %u\n", pSETBDFVERSIONParms->bdfVerType);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 1024 entries
    {
        A_PRINTF("SETBDFVERSIONOp: ver_info %u\n", pSETBDFVERSIONParms->ver_info[i]);
    }
#endif //_DEBUG

    if (NULL != SETBDFVERSIONOpFunc) {
        (*SETBDFVERSIONOpFunc)(pSETBDFVERSIONParms);
    }
    return(TRUE);
}
