/* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdEepromGetSize.s
#include "tlv2Inc.h"
#include "cmdEepromGetSize.h"

void* initEEPROMGETSIZEOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    return(NULL);
}

static EEPROMGETSIZE_OP_FUNC EEPROMGETSIZEOpFunc = NULL;

TLV2_API void registerEEPROMGETSIZEHandler(EEPROMGETSIZE_OP_FUNC fp)
{
    EEPROMGETSIZEOpFunc = fp;
}

A_BOOL EEPROMGETSIZEOp(void *pParms)
{
    if (NULL != EEPROMGETSIZEOpFunc) {
        (*EEPROMGETSIZEOpFunc)(NULL);
    }
    return(TRUE);
}

void* initEEPROMGETSIZERSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_EEPROMGETSIZERSP_PARMS  *pEEPROMGETSIZERSPParms = (CMD_EEPROMGETSIZERSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pEEPROMGETSIZERSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    memset(pEEPROMGETSIZERSPParms->pad3, 0, sizeof(pEEPROMGETSIZERSPParms->pad3));
    pEEPROMGETSIZERSPParms->eepromCalDataSize = pParmDict[PARM_EEPROMCALDATASIZE].v.valU32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pEEPROMGETSIZERSPParms->status)) - (size_t)pEEPROMGETSIZERSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD3, (size_t)(((size_t)&(pEEPROMGETSIZERSPParms->pad3)) - (size_t)pEEPROMGETSIZERSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_EEPROMCALDATASIZE, (size_t)(((size_t)&(pEEPROMGETSIZERSPParms->eepromCalDataSize)) - (size_t)pEEPROMGETSIZERSPParms), pParmsOffset);
    return((void*) pEEPROMGETSIZERSPParms);
}

static EEPROMGETSIZERSP_OP_FUNC EEPROMGETSIZERSPOpFunc = NULL;

TLV2_API void registerEEPROMGETSIZERSPHandler(EEPROMGETSIZERSP_OP_FUNC fp)
{
    EEPROMGETSIZERSPOpFunc = fp;
}

A_BOOL EEPROMGETSIZERSPOp(void *pParms)
{
    CMD_EEPROMGETSIZERSP_PARMS *pEEPROMGETSIZERSPParms = (CMD_EEPROMGETSIZERSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("EEPROMGETSIZERSPOp: status %u\n", pEEPROMGETSIZERSPParms->status);
    for (i = 0; i < 3 ; i++)
    {
        A_PRINTF("EEPROMGETSIZERSPOp: pad3 %u\n", pEEPROMGETSIZERSPParms->pad3[i]);
    }
    A_PRINTF("EEPROMGETSIZERSPOp: eepromCalDataSize %u\n", pEEPROMGETSIZERSPParms->eepromCalDataSize);
#endif //_DEBUG

    if (NULL != EEPROMGETSIZERSPOpFunc) {
        (*EEPROMGETSIZERSPOpFunc)(pEEPROMGETSIZERSPParms);
    }
    return(TRUE);
}
