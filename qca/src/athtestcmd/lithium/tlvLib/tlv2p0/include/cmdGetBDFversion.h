/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdGetBDFversion.s
#ifndef _CMDGETBDFVERSION_H_
#define _CMDGETBDFVERSION_H_

#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif //WIN32 || WIN64

typedef struct getbdfversion_parms {
    A_UINT8	bdfVerType;
    A_UINT8	pad[3];
} __ATTRIB_PACK CMD_GETBDFVERSION_PARMS;

typedef struct getbdfversionrsp_parms {
    A_UINT8	status;
    A_UINT8	ver_info[MAX_VER_INFO_SIZE];
    A_UINT8	pad[3];
} __ATTRIB_PACK CMD_GETBDFVERSIONRSP_PARMS;

typedef void (*GETBDFVERSION_OP_FUNC)(void *pParms);
typedef void (*GETBDFVERSIONRSP_OP_FUNC)(void *pParms);

// Exposed functions

void* initGETBDFVERSIONOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL GETBDFVERSIONOp(void *pParms);

void* initGETBDFVERSIONRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL GETBDFVERSIONRSPOp(void *pParms);

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif //WIN32 || WIN64


#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif //_CMDGETBDFVERSION_H_
