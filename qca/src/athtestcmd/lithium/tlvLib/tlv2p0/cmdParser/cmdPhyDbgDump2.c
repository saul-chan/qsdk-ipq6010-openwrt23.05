/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdPHYDBGdump2.s
#include "tlv2Inc.h"
#include "cmdPhyDbgDump2.h"

void* initPHYDBGDUMP2OpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_PHYDBGDUMP2_PARMS  *pPHYDBGDUMP2Parms = (CMD_PHYDBGDUMP2_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pPHYDBGDUMP2Parms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pPHYDBGDUMP2Parms->capture_bank = pParmDict[PARM_CAPTURE_BANK].v.valU32;
    pPHYDBGDUMP2Parms->dumpOption = pParmDict[PARM_DUMPOPTION].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pPHYDBGDUMP2Parms->phyId)) - (size_t)pPHYDBGDUMP2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CAPTURE_BANK, (size_t)(((size_t)&(pPHYDBGDUMP2Parms->capture_bank)) - (size_t)pPHYDBGDUMP2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DUMPOPTION, (size_t)(((size_t)&(pPHYDBGDUMP2Parms->dumpOption)) - (size_t)pPHYDBGDUMP2Parms), pParmsOffset);
    return((void*) pPHYDBGDUMP2Parms);
}

static PHYDBGDUMP2_OP_FUNC PHYDBGDUMP2OpFunc = NULL;

TLV2_API void registerPHYDBGDUMP2Handler(PHYDBGDUMP2_OP_FUNC fp)
{
    PHYDBGDUMP2OpFunc = fp;
}

A_BOOL PHYDBGDUMP2Op(void *pParms)
{
    CMD_PHYDBGDUMP2_PARMS *pPHYDBGDUMP2Parms = (CMD_PHYDBGDUMP2_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("PHYDBGDUMP2Op: phyId %u\n", pPHYDBGDUMP2Parms->phyId);
    A_PRINTF("PHYDBGDUMP2Op: capture_bank %u\n", pPHYDBGDUMP2Parms->capture_bank);
    A_PRINTF("PHYDBGDUMP2Op: dumpOption %u\n", pPHYDBGDUMP2Parms->dumpOption);
#endif //_DEBUG

    if (NULL != PHYDBGDUMP2OpFunc) {
        (*PHYDBGDUMP2OpFunc)(pPHYDBGDUMP2Parms);
    }
    return(TRUE);
}

void* initPHYDBGDUMP2RSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_PHYDBGDUMP2RSP_PARMS  *pPHYDBGDUMP2RSPParms = (CMD_PHYDBGDUMP2RSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    memset(pPHYDBGDUMP2RSPParms->data4k, 0, sizeof(pPHYDBGDUMP2RSPParms->data4k));
    pPHYDBGDUMP2RSPParms->status = pParmDict[PARM_STATUS].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_DATA4K, (size_t)(((size_t)&(pPHYDBGDUMP2RSPParms->data4k)) - (size_t)pPHYDBGDUMP2RSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pPHYDBGDUMP2RSPParms->status)) - (size_t)pPHYDBGDUMP2RSPParms), pParmsOffset);
    return((void*) pPHYDBGDUMP2RSPParms);
}

static PHYDBGDUMP2RSP_OP_FUNC PHYDBGDUMP2RSPOpFunc = NULL;

TLV2_API void registerPHYDBGDUMP2RSPHandler(PHYDBGDUMP2RSP_OP_FUNC fp)
{
    PHYDBGDUMP2RSPOpFunc = fp;
}

A_BOOL PHYDBGDUMP2RSPOp(void *pParms)
{
    CMD_PHYDBGDUMP2RSP_PARMS *pPHYDBGDUMP2RSPParms = (CMD_PHYDBGDUMP2RSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    for (i = 0; i < 8 ; i++) // can be modified to print up to 4096 entries
    {
        A_PRINTF("PHYDBGDUMP2RSPOp: data4k 0x%x\n", pPHYDBGDUMP2RSPParms->data4k[i]);
    }
    A_PRINTF("PHYDBGDUMP2RSPOp: status %u\n", pPHYDBGDUMP2RSPParms->status);
#endif //_DEBUG

    if (NULL != PHYDBGDUMP2RSPOpFunc) {
        (*PHYDBGDUMP2RSPOpFunc)(pPHYDBGDUMP2RSPParms);
    }
    return(TRUE);
}
