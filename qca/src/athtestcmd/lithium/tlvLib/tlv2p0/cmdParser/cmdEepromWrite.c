/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdEepromWrite.s
#include "tlv2Inc.h"
#include "cmdEepromWrite.h"

void* initEEPROMWRITEOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_EEPROMWRITE_PARMS  *pEEPROMWRITEParms = (CMD_EEPROMWRITE_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pEEPROMWRITEParms->offset = pParmDict[PARM_OFFSET].v.valU32;
    pEEPROMWRITEParms->size = pParmDict[PARM_SIZE].v.valU32;
    memset(pEEPROMWRITEParms->data4k, 0, sizeof(pEEPROMWRITEParms->data4k));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_OFFSET, (size_t)(((size_t)&(pEEPROMWRITEParms->offset)) - (size_t)pEEPROMWRITEParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SIZE, (size_t)(((size_t)&(pEEPROMWRITEParms->size)) - (size_t)pEEPROMWRITEParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DATA4K, (size_t)(((size_t)&(pEEPROMWRITEParms->data4k)) - (size_t)pEEPROMWRITEParms), pParmsOffset);
    return((void*) pEEPROMWRITEParms);
}

static EEPROMWRITE_OP_FUNC EEPROMWRITEOpFunc = NULL;

TLV2_API void registerEEPROMWRITEHandler(EEPROMWRITE_OP_FUNC fp)
{
    EEPROMWRITEOpFunc = fp;
}

A_BOOL EEPROMWRITEOp(void *pParms)
{
    CMD_EEPROMWRITE_PARMS *pEEPROMWRITEParms = (CMD_EEPROMWRITE_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("EEPROMWRITEOp: offset %u\n", pEEPROMWRITEParms->offset);
    A_PRINTF("EEPROMWRITEOp: size %u\n", pEEPROMWRITEParms->size);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 4096 entries
    {
        A_PRINTF("EEPROMWRITEOp: data4k 0x%x\n", pEEPROMWRITEParms->data4k[i]);
    }
#endif //_DEBUG

    if (NULL != EEPROMWRITEOpFunc) {
        (*EEPROMWRITEOpFunc)(pEEPROMWRITEParms);
    }
    return(TRUE);
}
