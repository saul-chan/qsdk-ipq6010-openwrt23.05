/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdPHYDBGdump2.s
#ifndef _CMDPHYDBGDUMP2_H_
#define _CMDPHYDBGDUMP2_H_

#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif //WIN32 || WIN64

typedef struct phydbgdump2_parms {
    A_UINT8	phyId;
    A_UINT32	capture_bank;
    A_UINT8	dumpOption;
    A_UINT8	pad[2];
} __ATTRIB_PACK CMD_PHYDBGDUMP2_PARMS;

typedef struct phydbgdump2rsp_parms {
    A_UINT8	data4k[4096];
    A_UINT8	status;
    A_UINT8	pad[3];
} __ATTRIB_PACK CMD_PHYDBGDUMP2RSP_PARMS;

typedef void (*PHYDBGDUMP2_OP_FUNC)(void *pParms);
typedef void (*PHYDBGDUMP2RSP_OP_FUNC)(void *pParms);

// Exposed functions

void* initPHYDBGDUMP2OpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL PHYDBGDUMP2Op(void *pParms);

void* initPHYDBGDUMP2RSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL PHYDBGDUMP2RSPOp(void *pParms);

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif //WIN32 || WIN64


#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif //_CMDPHYDBGDUMP2_H_
