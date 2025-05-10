/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

// This is an auto-generated file from input/cmdTPCCal_PostProcess.s
#ifndef _CMDTPCCAL_POSTPROCESS_H_
#define _CMDTPCCAL_POSTPROCESS_H_

#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif //WIN32 || WIN64

#if !defined(MCHAIN_UTF)
#define MCHAIN_UTF           16
#endif //!defined(MCHAIN_UTF)
#define NUM_MEASURED_PWR        512

typedef struct tpccal_postprocess_parms {
    A_UINT8	phyId;
    A_UINT8	chanIdx;
    A_UINT8	chainIdx;
    A_UINT8	valuetype;
} __ATTRIB_PACK CMD_TPCCAL_POSTPROCESS_PARMS;

typedef struct tpccal_postprocessrsp_parms {
    A_UINT8	phyId;
    A_UINT8	status;
    A_INT16	measuredPwr[NUM_MEASURED_PWR];
    A_UINT8	pdadc[MCHAIN_UTF];
    A_UINT8	pad[2];
} __ATTRIB_PACK CMD_TPCCAL_POSTPROCESSRSP_PARMS;

typedef void (*TPCCAL_POSTPROCESS_OP_FUNC)(void *pParms);
typedef void (*TPCCAL_POSTPROCESSRSP_OP_FUNC)(void *pParms);

// Exposed functions

void* initTPCCAL_POSTPROCESSOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL TPCCAL_POSTPROCESSOp(void *pParms);

void* initTPCCAL_POSTPROCESSRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL TPCCAL_POSTPROCESSRSPOp(void *pParms);

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif //WIN32 || WIN64


#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif //_CMDTPCCAL_POSTPROCESS_H_
