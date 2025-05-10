/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdGenericUtfCmdHandler.s
#include "tlv2Inc.h"
#include "cmdGenericUtfCmdHandler.h"

void* initGENERICUTFCMDOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GENERICUTFCMD_PARMS  *pGENERICUTFCMDParms = (CMD_GENERICUTFCMD_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGENERICUTFCMDParms->Measurement = pParmDict[PARM_MEASUREMENT].v.valU32;
    memset(pGENERICUTFCMDParms->inParams, 0, sizeof(pGENERICUTFCMDParms->inParams));
    pGENERICUTFCMDParms->GenericUtfCmdID = pParmDict[PARM_GENERICUTFCMDID].v.valU16;
    pGENERICUTFCMDParms->phyId = pParmDict[PARM_PHYID].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_MEASUREMENT, (size_t)(((size_t)&(pGENERICUTFCMDParms->Measurement)) - (size_t)pGENERICUTFCMDParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_INPARAMS, (size_t)(((size_t)&(pGENERICUTFCMDParms->inParams)) - (size_t)pGENERICUTFCMDParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GENERICUTFCMDID, (size_t)(((size_t)&(pGENERICUTFCMDParms->GenericUtfCmdID)) - (size_t)pGENERICUTFCMDParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGENERICUTFCMDParms->phyId)) - (size_t)pGENERICUTFCMDParms), pParmsOffset);
    return((void*) pGENERICUTFCMDParms);
}

static GENERICUTFCMD_OP_FUNC GENERICUTFCMDOpFunc = NULL;

TLV2_API void registerGENERICUTFCMDHandler(GENERICUTFCMD_OP_FUNC fp)
{
    GENERICUTFCMDOpFunc = fp;
}

A_BOOL GENERICUTFCMDOp(void *pParms)
{
    CMD_GENERICUTFCMD_PARMS *pGENERICUTFCMDParms = (CMD_GENERICUTFCMD_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GENERICUTFCMDOp: Measurement %u\n", pGENERICUTFCMDParms->Measurement);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("GENERICUTFCMDOp: inParams %d\n", pGENERICUTFCMDParms->inParams[i]);
    }
    A_PRINTF("GENERICUTFCMDOp: GenericUtfCmdID %u\n", pGENERICUTFCMDParms->GenericUtfCmdID);
    A_PRINTF("GENERICUTFCMDOp: phyId %u\n", pGENERICUTFCMDParms->phyId);
#endif //_DEBUG

    if (NULL != GENERICUTFCMDOpFunc) {
        (*GENERICUTFCMDOpFunc)(pGENERICUTFCMDParms);
    }
    return(TRUE);
}

void* initGENERICUTFCMDRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_GENERICUTFCMDRSP_PARMS  *pGENERICUTFCMDRSPParms = (CMD_GENERICUTFCMDRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pGENERICUTFCMDRSPParms->GenericUtfCmdDone = pParmDict[PARM_GENERICUTFCMDDONE].v.valU8;
    pGENERICUTFCMDRSPParms->NumGenericUtfCmdRetData = pParmDict[PARM_NUMGENERICUTFCMDRETDATA].v.valU8;
    pGENERICUTFCMDRSPParms->GenericUtfCmdStatus = pParmDict[PARM_GENERICUTFCMDSTATUS].v.valU16;
    memset(pGENERICUTFCMDRSPParms->GenericUtfCmdRetData, 0, sizeof(pGENERICUTFCMDRSPParms->GenericUtfCmdRetData));
    pGENERICUTFCMDRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_GENERICUTFCMDDONE, (size_t)(((size_t)&(pGENERICUTFCMDRSPParms->GenericUtfCmdDone)) - (size_t)pGENERICUTFCMDRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMGENERICUTFCMDRETDATA, (size_t)(((size_t)&(pGENERICUTFCMDRSPParms->NumGenericUtfCmdRetData)) - (size_t)pGENERICUTFCMDRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GENERICUTFCMDSTATUS, (size_t)(((size_t)&(pGENERICUTFCMDRSPParms->GenericUtfCmdStatus)) - (size_t)pGENERICUTFCMDRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GENERICUTFCMDRETDATA, (size_t)(((size_t)&(pGENERICUTFCMDRSPParms->GenericUtfCmdRetData)) - (size_t)pGENERICUTFCMDRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pGENERICUTFCMDRSPParms->phyId)) - (size_t)pGENERICUTFCMDRSPParms), pParmsOffset);
    return((void*) pGENERICUTFCMDRSPParms);
}

static GENERICUTFCMDRSP_OP_FUNC GENERICUTFCMDRSPOpFunc = NULL;

TLV2_API void registerGENERICUTFCMDRSPHandler(GENERICUTFCMDRSP_OP_FUNC fp)
{
    GENERICUTFCMDRSPOpFunc = fp;
}

A_BOOL GENERICUTFCMDRSPOp(void *pParms)
{
    CMD_GENERICUTFCMDRSP_PARMS *pGENERICUTFCMDRSPParms = (CMD_GENERICUTFCMDRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("GENERICUTFCMDRSPOp: GenericUtfCmdDone %u\n", pGENERICUTFCMDRSPParms->GenericUtfCmdDone);
    A_PRINTF("GENERICUTFCMDRSPOp: NumGenericUtfCmdRetData %u\n", pGENERICUTFCMDRSPParms->NumGenericUtfCmdRetData);
    A_PRINTF("GENERICUTFCMDRSPOp: GenericUtfCmdStatus %u\n", pGENERICUTFCMDRSPParms->GenericUtfCmdStatus);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("GENERICUTFCMDRSPOp: GenericUtfCmdRetData %d\n", pGENERICUTFCMDRSPParms->GenericUtfCmdRetData[i]);
    }
    A_PRINTF("GENERICUTFCMDRSPOp: phyId %u\n", pGENERICUTFCMDRSPParms->phyId);
#endif //_DEBUG

    if (NULL != GENERICUTFCMDRSPOpFunc) {
        (*GENERICUTFCMDRSPOpFunc)(pGENERICUTFCMDRSPParms);
    }
    return(TRUE);
}
