/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

// This is an auto-generated file from input/cmdChangeTPC_ForcedGain.s
#ifndef _CMDCHANGETPC_FORCEDGAIN_H_
#define _CMDCHANGETPC_FORCEDGAIN_H_

#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif //WIN32 || WIN64

typedef struct changetpc_forcedgain_parms {
    A_UINT8	phyId;
    A_UINT8	gainIdx;
    A_INT8	dacGain;
    A_UINT8	alutOffset;
} __ATTRIB_PACK CMD_CHANGETPC_FORCEDGAIN_PARMS;

typedef void (*CHANGETPC_FORCEDGAIN_OP_FUNC)(void *pParms);

// Exposed functions

void* initCHANGETPC_FORCEDGAINOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL CHANGETPC_FORCEDGAINOp(void *pParms);

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif //WIN32 || WIN64


#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif //_CMDCHANGETPC_FORCEDGAIN_H_
