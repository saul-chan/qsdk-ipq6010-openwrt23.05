/*
* Copyright (c) 2018 - 2021 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// This is an auto-generated file from input\cmdOPCHandler.s
#ifndef _CMDOPCHANDLER_H_
#define _CMDOPCHANDLER_H_

#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif //WIN32 || WIN64

typedef struct cust_opccalproc_parms {
    A_UINT8	phyId;
    A_UINT8	band;
    A_UINT16	freq;
    A_UINT8	chainIdx;
    A_UINT8	opcCtrl;
    A_INT8	powerOffset;
    A_INT8	glutOffset;
    A_UINT8	thermalOffset;
    A_UINT8	pad[3];
} __ATTRIB_PACK CMD_CUST_OPCCALPROC_PARMS;

typedef struct cust_opccalprocrsp_parms {
    A_UINT8	phyId;
    A_UINT8	band;
    A_UINT16	freq;
    A_UINT8	chainIdx;
    A_UINT8	status;
    A_INT8	powerOffset;
    A_INT8	glutOffset;
    A_UINT8	thermalOffset;
    A_UINT8	pad[3];
} __ATTRIB_PACK CMD_CUST_OPCCALPROCRSP_PARMS;

typedef void (*CUST_OPCCALPROC_OP_FUNC)(void *pParms);
typedef void (*CUST_OPCCALPROCRSP_OP_FUNC)(void *pParms);

// Exposed functions

void* initCUST_OPCCALPROCOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL CUST_OPCCALPROCOp(void *pParms);

void* initCUST_OPCCALPROCRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL CUST_OPCCALPROCRSPOp(void *pParms);

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif //WIN32 || WIN64


#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif //_CMDOPCHANDLER_H_
