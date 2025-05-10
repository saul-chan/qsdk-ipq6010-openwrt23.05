/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdSendTPC_TxMeas.s
#ifndef _CMDSENDTPC_TXMEAS_H_
#define _CMDSENDTPC_TXMEAS_H_

#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif //WIN32 || WIN64

typedef struct sendtpc_txmeas_parms {
    A_UINT8	phyId;
    A_UINT8	chanIdx;
    A_UINT8	chainIdx;
    A_UINT8	loopCnt;
    A_UINT8	gainIdx;
    A_UINT8	tpcpdadc;
    A_INT16	tpcMeasuredPwr;
} __ATTRIB_PACK CMD_SENDTPC_TXMEAS_PARMS;

typedef void (*SENDTPC_TXMEAS_OP_FUNC)(void *pParms);

// Exposed functions

void* initSENDTPC_TXMEASOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL SENDTPC_TXMEASOp(void *pParms);

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif //WIN32 || WIN64


#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif //_CMDSENDTPC_TXMEAS_H_
