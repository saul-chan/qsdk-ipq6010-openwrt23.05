/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

// This is an auto-generated file from input/cmdSetRxChainOnOff.s
#ifndef _CMDSETRXCHAINONOFF_H_
#define _CMDSETRXCHAINONOFF_H_

#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif //WIN32 || WIN64

typedef struct setrxchainonoff_parms {
    A_UINT8	phyId;
    A_UINT8	chainIdx;
    A_UINT8	enable_disable;
    A_UINT8	pad[1];
} __ATTRIB_PACK CMD_SETRXCHAINONOFF_PARMS;

typedef void (*SETRXCHAINONOFF_OP_FUNC)(void *pParms);

// Exposed functions

void* initSETRXCHAINONOFFOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL SETRXCHAINONOFFOp(void *pParms);

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif //WIN32 || WIN64


#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif //_CMDSETRXCHAINONOFF_H_
