/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input\cmdStickyHandler.s
#include "tlv2Inc.h"
#include "cmdStickyHandler.h"

void* initSTICKYOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_STICKY_PARMS  *pSTICKYParms = (CMD_STICKY_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pSTICKYParms->regAddress = pParmDict[PARM_REGADDRESS].v.valU32;
    pSTICKYParms->regMask = pParmDict[PARM_REGMASK].v.valU32;
    pSTICKYParms->stickyFlags = pParmDict[PARM_STICKYFLAGS].v.valU32;
    pSTICKYParms->regValue = pParmDict[PARM_REGVALUE].v.valU32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_REGADDRESS, (size_t)(((size_t)&(pSTICKYParms->regAddress)) - (size_t)pSTICKYParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_REGMASK, (size_t)(((size_t)&(pSTICKYParms->regMask)) - (size_t)pSTICKYParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STICKYFLAGS, (size_t)(((size_t)&(pSTICKYParms->stickyFlags)) - (size_t)pSTICKYParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_REGVALUE, (size_t)(((size_t)&(pSTICKYParms->regValue)) - (size_t)pSTICKYParms), pParmsOffset);
    return((void*) pSTICKYParms);
}

static STICKY_OP_FUNC STICKYOpFunc = NULL;

TLV2_API void registerSTICKYHandler(STICKY_OP_FUNC fp)
{
    STICKYOpFunc = fp;
}

A_BOOL STICKYOp(void *pParms)
{
    CMD_STICKY_PARMS *pSTICKYParms = (CMD_STICKY_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("STICKYOp: regAddress 0x%x\n", pSTICKYParms->regAddress);
    A_PRINTF("STICKYOp: regMask 0x%x\n", pSTICKYParms->regMask);
    A_PRINTF("STICKYOp: stickyFlags 0x%x\n", pSTICKYParms->stickyFlags);
    A_PRINTF("STICKYOp: regValue 0x%x\n", pSTICKYParms->regValue);
#endif //_DEBUG

    if (NULL != STICKYOpFunc) {
        (*STICKYOpFunc)(pSTICKYParms);
    }
    return(TRUE);
}
