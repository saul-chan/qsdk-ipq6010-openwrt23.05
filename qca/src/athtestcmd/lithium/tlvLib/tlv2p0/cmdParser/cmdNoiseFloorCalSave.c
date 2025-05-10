/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from ./input\cmdNoiseFloorCalSave.s
#include "tlv2Inc.h"
#include "cmdNoiseFloorCalSave.h"

void* initNOISEFLOORCALSAVEOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_NOISEFLOORCALSAVE_PARMS  *pNOISEFLOORCALSAVEParms = (CMD_NOISEFLOORCALSAVE_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    memset(pNOISEFLOORCALSAVEParms->freqs, 0, sizeof(pNOISEFLOORCALSAVEParms->freqs));
    memset(pNOISEFLOORCALSAVEParms->NFCalValues, 0, sizeof(pNOISEFLOORCALSAVEParms->NFCalValues));
    pNOISEFLOORCALSAVEParms->itemNum = pParmDict[PARM_ITEMNUM].v.valU16;
    pNOISEFLOORCALSAVEParms->phyId = pParmDict[PARM_PHYID].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_FREQS, (size_t)(((size_t)&(pNOISEFLOORCALSAVEParms->freqs)) - (size_t)pNOISEFLOORCALSAVEParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NFCALVALUES, (size_t)(((size_t)&(pNOISEFLOORCALSAVEParms->NFCalValues)) - (size_t)pNOISEFLOORCALSAVEParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ITEMNUM, (size_t)(((size_t)&(pNOISEFLOORCALSAVEParms->itemNum)) - (size_t)pNOISEFLOORCALSAVEParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pNOISEFLOORCALSAVEParms->phyId)) - (size_t)pNOISEFLOORCALSAVEParms), pParmsOffset);
    return((void*) pNOISEFLOORCALSAVEParms);
}

static NOISEFLOORCALSAVE_OP_FUNC NOISEFLOORCALSAVEOpFunc = NULL;

TLV2_API void registerNOISEFLOORCALSAVEHandler(NOISEFLOORCALSAVE_OP_FUNC fp)
{
    NOISEFLOORCALSAVEOpFunc = fp;
}

A_BOOL NOISEFLOORCALSAVEOp(void *pParms)
{
    CMD_NOISEFLOORCALSAVE_PARMS *pNOISEFLOORCALSAVEParms = (CMD_NOISEFLOORCALSAVE_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("NOISEFLOORCALSAVEOp: freqs %u\n", pNOISEFLOORCALSAVEParms->freqs[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 256 entries
    {
        A_PRINTF("NOISEFLOORCALSAVEOp: NFCalValues %d\n", pNOISEFLOORCALSAVEParms->NFCalValues[i]);
    }
    A_PRINTF("NOISEFLOORCALSAVEOp: itemNum %u\n", pNOISEFLOORCALSAVEParms->itemNum);
    A_PRINTF("NOISEFLOORCALSAVEOp: phyId %u\n", pNOISEFLOORCALSAVEParms->phyId);
#endif //_DEBUG

    if (NULL != NOISEFLOORCALSAVEOpFunc) {
        (*NOISEFLOORCALSAVEOpFunc)(pNOISEFLOORCALSAVEParms);
    }
    return(TRUE);
}

void* initNOISEFLOORCALSAVERSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_NOISEFLOORCALSAVERSP_PARMS  *pNOISEFLOORCALSAVERSPParms = (CMD_NOISEFLOORCALSAVERSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pNOISEFLOORCALSAVERSPParms->status = pParmDict[PARM_STATUS].v.valU32;
    pNOISEFLOORCALSAVERSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pNOISEFLOORCALSAVERSPParms->status)) - (size_t)pNOISEFLOORCALSAVERSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pNOISEFLOORCALSAVERSPParms->phyId)) - (size_t)pNOISEFLOORCALSAVERSPParms), pParmsOffset);
    return((void*) pNOISEFLOORCALSAVERSPParms);
}

static NOISEFLOORCALSAVERSP_OP_FUNC NOISEFLOORCALSAVERSPOpFunc = NULL;

TLV2_API void registerNOISEFLOORCALSAVERSPHandler(NOISEFLOORCALSAVERSP_OP_FUNC fp)
{
    NOISEFLOORCALSAVERSPOpFunc = fp;
}

A_BOOL NOISEFLOORCALSAVERSPOp(void *pParms)
{
    CMD_NOISEFLOORCALSAVERSP_PARMS *pNOISEFLOORCALSAVERSPParms = (CMD_NOISEFLOORCALSAVERSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("NOISEFLOORCALSAVERSPOp: status %u\n", pNOISEFLOORCALSAVERSPParms->status);
    A_PRINTF("NOISEFLOORCALSAVERSPOp: phyId %u\n", pNOISEFLOORCALSAVERSPParms->phyId);
#endif //_DEBUG

    if (NULL != NOISEFLOORCALSAVERSPOpFunc) {
        (*NOISEFLOORCALSAVERSPOpFunc)(pNOISEFLOORCALSAVERSPParms);
    }
    return(TRUE);
}
