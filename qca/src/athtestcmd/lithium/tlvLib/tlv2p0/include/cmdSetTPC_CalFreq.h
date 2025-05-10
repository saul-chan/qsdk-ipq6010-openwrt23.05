/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

// This is an auto-generated file from input/cmdSetTPC_CalFreq.s
#ifndef _CMDSETTPC_CALFREQ_H_
#define _CMDSETTPC_CALFREQ_H_

#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif //WIN32 || WIN64

typedef struct settpc_calfreq_parms {
    A_UINT8	phyId;
    A_UINT8	chanIdx;
    A_UINT16	freq;
} __ATTRIB_PACK CMD_SETTPC_CALFREQ_PARMS;

typedef void (*SETTPC_CALFREQ_OP_FUNC)(void *pParms);

// Exposed functions

void* initSETTPC_CALFREQOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL SETTPC_CALFREQOp(void *pParms);

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif //WIN32 || WIN64


#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif //_CMDSETTPC_CALFREQ_H_
