/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

// This is an auto-generated file from input/cmdTPCCal_BDFUpdate.s
#ifndef _CMDTPCCAL_BDFUPDATE_H_
#define _CMDTPCCAL_BDFUPDATE_H_

#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif //WIN32 || WIN64

typedef struct tpccal_bdfupdate_parms {
    A_UINT8	phyId;
    A_UINT8	pad[3];
} __ATTRIB_PACK CMD_TPCCAL_BDFUPDATE_PARMS;

typedef void (*TPCCAL_BDFUPDATE_OP_FUNC)(void *pParms);

// Exposed functions

void* initTPCCAL_BDFUPDATEOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL TPCCAL_BDFUPDATEOp(void *pParms);

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif //WIN32 || WIN64


#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif //_CMDTPCCAL_BDFUPDATE_H_
