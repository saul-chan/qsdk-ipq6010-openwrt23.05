/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdGetEDPDCalInfo.s
#ifndef _CMDGETEDPDCALINFO_H_
#define _CMDGETEDPDCALINFO_H_

#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif //WIN32 || WIN64

#if !defined(ALL_PAPRD_GLUT_CHAIN_VAL)
#define ALL_PAPRD_GLUT_CHAIN_VAL           32
#endif //!defined(ALL_PAPRD_GLUT4_CHAIN_VAL)

typedef struct getedpdcalinfo_parms {
    A_UINT8	phyId;
    A_UINT16	chainMask;
    A_UINT8	pad[1];
} __ATTRIB_PACK CMD_GETEDPDCALINFO_PARMS;

typedef struct getedpdcalinforsp_parms {
    A_UINT8	status;
    A_UINT8	phyId;
    A_UINT16	chainMask;
    A_INT8	nmse[ALL_PAPRD_GLUT_CHAIN_VAL];
    A_UINT16	signalQuality[ALL_PAPRD_GLUT_CHAIN_VAL];
    A_UINT8	maxBin[ALL_PAPRD_GLUT_CHAIN_VAL];
    A_UINT16	diffn0[ALL_PAPRD_GLUT_CHAIN_VAL];
    A_UINT16	diffn1[ALL_PAPRD_GLUT_CHAIN_VAL];
    A_UINT16	diffn2[ALL_PAPRD_GLUT_CHAIN_VAL];
    A_UINT16	diffn3[ALL_PAPRD_GLUT_CHAIN_VAL];
    A_UINT8	sqIdx[ALL_PAPRD_GLUT_CHAIN_VAL];
    A_UINT8	rxGain[ALL_PAPRD_GLUT_CHAIN_VAL];
    A_INT16	dcI[ALL_PAPRD_GLUT_CHAIN_VAL];
    A_INT16	dcQ[ALL_PAPRD_GLUT_CHAIN_VAL];
} __ATTRIB_PACK CMD_GETEDPDCALINFORSP_PARMS;

typedef void (*GETEDPDCALINFO_OP_FUNC)(void *pParms);
typedef void (*GETEDPDCALINFORSP_OP_FUNC)(void *pParms);

// Exposed functions

void* initGETEDPDCALINFOOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL GETEDPDCALINFOOp(void *pParms);

void* initGETEDPDCALINFORSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL GETEDPDCALINFORSPOp(void *pParms);

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif //WIN32 || WIN64


#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif //_CMDGETEDPDCALINFO_H_
