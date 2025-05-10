/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

// This is an auto-generated file from input/cmdGetBDFversion.s
#include "tlv2Inc.h"
#include "cmdGetBDFversion.h"

void* initGETBDFVERSIONOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETBDFVERSION_PARMS  *pGETBDFVERSIONParms = (CMD_GETBDFVERSION_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGETBDFVERSIONParms->bdfVerType = pParmDict[PARM_BDFVERTYPE].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_BDFVERTYPE, (size_t)(((size_t)&(pGETBDFVERSIONParms->bdfVerType)) - (size_t)pGETBDFVERSIONParms), pParmsOffset);
    return((void*) pGETBDFVERSIONParms);
}

static GETBDFVERSION_OP_FUNC GETBDFVERSIONOpFunc = NULL;

TLV2_API void registerGETBDFVERSIONHandler(GETBDFVERSION_OP_FUNC fp)
{
    GETBDFVERSIONOpFunc = fp;
}

A_BOOL GETBDFVERSIONOp(void *pParms)
{
    CMD_GETBDFVERSION_PARMS *pGETBDFVERSIONParms = (CMD_GETBDFVERSION_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GETBDFVERSIONOp: bdfVerType %u\n", pGETBDFVERSIONParms->bdfVerType);
#endif //_DEBUG

    if (NULL != GETBDFVERSIONOpFunc) {
        (*GETBDFVERSIONOpFunc)(pGETBDFVERSIONParms);
    }
    return(TRUE);
}

void* initGETBDFVERSIONRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GETBDFVERSIONRSP_PARMS  *pGETBDFVERSIONRSPParms = (CMD_GETBDFVERSIONRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGETBDFVERSIONRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    memset(pGETBDFVERSIONRSPParms->ver_info, 0, sizeof(pGETBDFVERSIONRSPParms->ver_info));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pGETBDFVERSIONRSPParms->status)) - (size_t)pGETBDFVERSIONRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_VER_INFO, (size_t)(((size_t)&(pGETBDFVERSIONRSPParms->ver_info)) - (size_t)pGETBDFVERSIONRSPParms), pParmsOffset);
    return((void*) pGETBDFVERSIONRSPParms);
}

static GETBDFVERSIONRSP_OP_FUNC GETBDFVERSIONRSPOpFunc = NULL;

TLV2_API void registerGETBDFVERSIONRSPHandler(GETBDFVERSIONRSP_OP_FUNC fp)
{
    GETBDFVERSIONRSPOpFunc = fp;
}

A_BOOL GETBDFVERSIONRSPOp(void *pParms)
{
    CMD_GETBDFVERSIONRSP_PARMS *pGETBDFVERSIONRSPParms = (CMD_GETBDFVERSIONRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GETBDFVERSIONRSPOp: status %u\n", pGETBDFVERSIONRSPParms->status);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 1024 entries
    {
        A_PRINTF("GETBDFVERSIONRSPOp: ver_info %u\n", pGETBDFVERSIONRSPParms->ver_info[i]);
    }
#endif //_DEBUG

    if (NULL != GETBDFVERSIONRSPOpFunc) {
        (*GETBDFVERSIONRSPOpFunc)(pGETBDFVERSIONRSPParms);
    }
    return(TRUE);
}
