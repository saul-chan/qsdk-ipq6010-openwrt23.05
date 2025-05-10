/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdTPCGetPDADC.s
#ifndef _CMDTPCGETPDADC_H_
#define _CMDTPCGETPDADC_H_

#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif //WIN32 || WIN64

typedef struct gettpc_pdadc_parms {
    A_UINT8	phyId;
    A_UINT8	chainIdx;
    A_UINT8	pad[2];
} __ATTRIB_PACK CMD_GETTPC_PDADC_PARMS;

typedef struct gettpc_pdadcrsp_parms {
    A_UINT8	phyId;
    A_UINT8	status;
    A_UINT8	tpcpdadc;
    A_UINT8	pad[1];
} __ATTRIB_PACK CMD_GETTPC_PDADCRSP_PARMS;

typedef void (*GETTPC_PDADC_OP_FUNC)(void *pParms);
typedef void (*GETTPC_PDADCRSP_OP_FUNC)(void *pParms);

// Exposed functions

void* initGETTPC_PDADCOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL GETTPC_PDADCOp(void *pParms);

void* initGETTPC_PDADCRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL GETTPC_PDADCRSPOp(void *pParms);

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif //WIN32 || WIN64


#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif //_CMDTPCGETPDADC_H_
