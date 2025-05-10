/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdCaptureFFTReports.s
#include "tlv2Inc.h"
#include "cmdCaptureFFTReports.h"

void* initCAPTUREFFTREPORTSOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_CAPTUREFFTREPORTS_PARMS  *pCAPTUREFFTREPORTSParms = (CMD_CAPTUREFFTREPORTS_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pCAPTUREFFTREPORTSParms->fftreportsize = pParmDict[PARM_FFTREPORTSIZE].v.valU32;
    pCAPTUREFFTREPORTSParms->offset = pParmDict[PARM_OFFSET].v.valU32;
    pCAPTUREFFTREPORTSParms->size = pParmDict[PARM_SIZE].v.valU32;
    pCAPTUREFFTREPORTSParms->phyId = pParmDict[PARM_PHYID].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_FFTREPORTSIZE, (size_t)(((size_t)&(pCAPTUREFFTREPORTSParms->fftreportsize)) - (size_t)pCAPTUREFFTREPORTSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_OFFSET, (size_t)(((size_t)&(pCAPTUREFFTREPORTSParms->offset)) - (size_t)pCAPTUREFFTREPORTSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SIZE, (size_t)(((size_t)&(pCAPTUREFFTREPORTSParms->size)) - (size_t)pCAPTUREFFTREPORTSParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pCAPTUREFFTREPORTSParms->phyId)) - (size_t)pCAPTUREFFTREPORTSParms), pParmsOffset);
    return((void*) pCAPTUREFFTREPORTSParms);
}

static CAPTUREFFTREPORTS_OP_FUNC CAPTUREFFTREPORTSOpFunc = NULL;

TLV2_API void registerCAPTUREFFTREPORTSHandler(CAPTUREFFTREPORTS_OP_FUNC fp)
{
    CAPTUREFFTREPORTSOpFunc = fp;
}

A_BOOL CAPTUREFFTREPORTSOp(void *pParms)
{
    CMD_CAPTUREFFTREPORTS_PARMS *pCAPTUREFFTREPORTSParms = (CMD_CAPTUREFFTREPORTS_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("CAPTUREFFTREPORTSOp: fftreportsize %u\n", pCAPTUREFFTREPORTSParms->fftreportsize);
    A_PRINTF("CAPTUREFFTREPORTSOp: offset %u\n", pCAPTUREFFTREPORTSParms->offset);
    A_PRINTF("CAPTUREFFTREPORTSOp: size %u\n", pCAPTUREFFTREPORTSParms->size);
    A_PRINTF("CAPTUREFFTREPORTSOp: phyId %u\n", pCAPTUREFFTREPORTSParms->phyId);
#endif //_DEBUG

    if (NULL != CAPTUREFFTREPORTSOpFunc) {
        (*CAPTUREFFTREPORTSOpFunc)(pCAPTUREFFTREPORTSParms);
    }
    return(TRUE);
}

void* initCAPTUREFFTREPORTSRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_CAPTUREFFTREPORTSRSP_PARMS  *pCAPTUREFFTREPORTSRSPParms = (CMD_CAPTUREFFTREPORTSRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pCAPTUREFFTREPORTSRSPParms->fftStatus = pParmDict[PARM_FFTSTATUS].v.valU32;
    pCAPTUREFFTREPORTSRSPParms->offset = pParmDict[PARM_OFFSET].v.valU32;
    pCAPTUREFFTREPORTSRSPParms->size = pParmDict[PARM_SIZE].v.valU32;
    memset(pCAPTUREFFTREPORTSRSPParms->data4k, 0, sizeof(pCAPTUREFFTREPORTSRSPParms->data4k));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_FFTSTATUS, (size_t)(((size_t)&(pCAPTUREFFTREPORTSRSPParms->fftStatus)) - (size_t)pCAPTUREFFTREPORTSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_OFFSET, (size_t)(((size_t)&(pCAPTUREFFTREPORTSRSPParms->offset)) - (size_t)pCAPTUREFFTREPORTSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SIZE, (size_t)(((size_t)&(pCAPTUREFFTREPORTSRSPParms->size)) - (size_t)pCAPTUREFFTREPORTSRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DATA4K, (size_t)(((size_t)&(pCAPTUREFFTREPORTSRSPParms->data4k)) - (size_t)pCAPTUREFFTREPORTSRSPParms), pParmsOffset);
    return((void*) pCAPTUREFFTREPORTSRSPParms);
}

static CAPTUREFFTREPORTSRSP_OP_FUNC CAPTUREFFTREPORTSRSPOpFunc = NULL;

TLV2_API void registerCAPTUREFFTREPORTSRSPHandler(CAPTUREFFTREPORTSRSP_OP_FUNC fp)
{
    CAPTUREFFTREPORTSRSPOpFunc = fp;
}

A_BOOL CAPTUREFFTREPORTSRSPOp(void *pParms)
{
    CMD_CAPTUREFFTREPORTSRSP_PARMS *pCAPTUREFFTREPORTSRSPParms = (CMD_CAPTUREFFTREPORTSRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("CAPTUREFFTREPORTSRSPOp: fftStatus %u\n", pCAPTUREFFTREPORTSRSPParms->fftStatus);
    A_PRINTF("CAPTUREFFTREPORTSRSPOp: offset %u\n", pCAPTUREFFTREPORTSRSPParms->offset);
    A_PRINTF("CAPTUREFFTREPORTSRSPOp: size %u\n", pCAPTUREFFTREPORTSRSPParms->size);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 4096 entries
    {
        A_PRINTF("CAPTUREFFTREPORTSRSPOp: data4k 0x%x\n", pCAPTUREFFTREPORTSRSPParms->data4k[i]);
    }
#endif //_DEBUG

    if (NULL != CAPTUREFFTREPORTSRSPOpFunc) {
        (*CAPTUREFFTREPORTSRSPOpFunc)(pCAPTUREFFTREPORTSRSPParms);
    }
    return(TRUE);
}
