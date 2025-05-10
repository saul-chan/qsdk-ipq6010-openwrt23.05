/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input\cmdRxDcoGroupHandler.s
#include "tlv2Inc.h"
#include "cmdRxDcoGroupHandler.h"

void* initRXDCOGROUPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
#if 0  // Helium code which is not applicable to Lithum codebase
    CMD_RXDCOGROUP_PARMS  *pRXDCOGROUPParms = (CMD_RXDCOGROUP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pRXDCOGROUPParms->cmdIdRxDcoGroup = pParmDict[PARM_CMDIDRXDCOGROUP].v.valU8;
    pRXDCOGROUPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pRXDCOGROUPParms->bandCode = pParmDict[PARM_BANDCODE].v.valU8;
    pRXDCOGROUPParms->bwCode = pParmDict[PARM_BWCODE].v.valU8;
    pRXDCOGROUPParms->chainMask = pParmDict[PARM_CHAINMASK].v.valU16;
    pRXDCOGROUPParms->rxdco_cal_mode = pParmDict[PARM_RXDCO_CAL_MODE].v.valU16;
    pRXDCOGROUPParms->homeCh = pParmDict[PARM_HOMECH].v.valU8;
    pRXDCOGROUPParms->initLUT = pParmDict[PARM_INITLUT].v.valU8;
    pRXDCOGROUPParms->verbose = pParmDict[PARM_VERBOSE].v.valU8;
    pRXDCOGROUPParms->pad1 = pParmDict[PARM_PAD1].v.valU8;
    pRXDCOGROUPParms->bbOvrdI = pParmDict[PARM_BBOVRDI].v.valU16;
    pRXDCOGROUPParms->bbOvrdQ = pParmDict[PARM_BBOVRDQ].v.valU16;
    pRXDCOGROUPParms->rfOvrdI = pParmDict[PARM_RFOVRDI].v.valU16;
    pRXDCOGROUPParms->rfOvrdQ = pParmDict[PARM_RFOVRDQ].v.valU16;
    pRXDCOGROUPParms->userParm1 = pParmDict[PARM_USERPARM1].v.valU32;
    pRXDCOGROUPParms->userParm2 = pParmDict[PARM_USERPARM2].v.valU32;
    pRXDCOGROUPParms->userParm3 = pParmDict[PARM_USERPARM3].v.valU32;
    pRXDCOGROUPParms->userParm4 = pParmDict[PARM_USERPARM4].v.valU32;
    pRXDCOGROUPParms->bbDcocRange0I = pParmDict[PARM_BBDCOCRANGE0I].v.valU8;
    pRXDCOGROUPParms->bbDcocRange0Q = pParmDict[PARM_BBDCOCRANGE0Q].v.valU8;
    pRXDCOGROUPParms->rfDcocRange0I = pParmDict[PARM_RFDCOCRANGE0I].v.valU8;
    pRXDCOGROUPParms->rfDcocRange0Q = pParmDict[PARM_RFDCOCRANGE0Q].v.valU8;
    memset(pRXDCOGROUPParms->corrRF_DCO_LUT_i, 0, sizeof(pRXDCOGROUPParms->corrRF_DCO_LUT_i));
    memset(pRXDCOGROUPParms->corrRF_DCO_LUT_q, 0, sizeof(pRXDCOGROUPParms->corrRF_DCO_LUT_q));
    memset(pRXDCOGROUPParms->corrBB_DCO_LUT_i, 0, sizeof(pRXDCOGROUPParms->corrBB_DCO_LUT_i));
    memset(pRXDCOGROUPParms->corrBB_DCO_LUT_q, 0, sizeof(pRXDCOGROUPParms->corrBB_DCO_LUT_q));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_CMDIDRXDCOGROUP, (size_t)(((size_t)&(pRXDCOGROUPParms->cmdIdRxDcoGroup)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pRXDCOGROUPParms->phyId)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BANDCODE, (size_t)(((size_t)&(pRXDCOGROUPParms->bandCode)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BWCODE, (size_t)(((size_t)&(pRXDCOGROUPParms->bwCode)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINMASK, (size_t)(((size_t)&(pRXDCOGROUPParms->chainMask)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXDCO_CAL_MODE, (size_t)(((size_t)&(pRXDCOGROUPParms->rxdco_cal_mode)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_HOMECH, (size_t)(((size_t)&(pRXDCOGROUPParms->homeCh)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_INITLUT, (size_t)(((size_t)&(pRXDCOGROUPParms->initLUT)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_VERBOSE, (size_t)(((size_t)&(pRXDCOGROUPParms->verbose)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD1, (size_t)(((size_t)&(pRXDCOGROUPParms->pad1)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BBOVRDI, (size_t)(((size_t)&(pRXDCOGROUPParms->bbOvrdI)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BBOVRDQ, (size_t)(((size_t)&(pRXDCOGROUPParms->bbOvrdQ)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RFOVRDI, (size_t)(((size_t)&(pRXDCOGROUPParms->rfOvrdI)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RFOVRDQ, (size_t)(((size_t)&(pRXDCOGROUPParms->rfOvrdQ)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_USERPARM1, (size_t)(((size_t)&(pRXDCOGROUPParms->userParm1)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_USERPARM2, (size_t)(((size_t)&(pRXDCOGROUPParms->userParm2)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_USERPARM3, (size_t)(((size_t)&(pRXDCOGROUPParms->userParm3)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_USERPARM4, (size_t)(((size_t)&(pRXDCOGROUPParms->userParm4)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BBDCOCRANGE0I, (size_t)(((size_t)&(pRXDCOGROUPParms->bbDcocRange0I)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BBDCOCRANGE0Q, (size_t)(((size_t)&(pRXDCOGROUPParms->bbDcocRange0Q)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RFDCOCRANGE0I, (size_t)(((size_t)&(pRXDCOGROUPParms->rfDcocRange0I)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RFDCOCRANGE0Q, (size_t)(((size_t)&(pRXDCOGROUPParms->rfDcocRange0Q)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CORRRF_DCO_LUT_I, (size_t)(((size_t)&(pRXDCOGROUPParms->corrRF_DCO_LUT_i)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CORRRF_DCO_LUT_Q, (size_t)(((size_t)&(pRXDCOGROUPParms->corrRF_DCO_LUT_q)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CORRBB_DCO_LUT_I, (size_t)(((size_t)&(pRXDCOGROUPParms->corrBB_DCO_LUT_i)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CORRBB_DCO_LUT_Q, (size_t)(((size_t)&(pRXDCOGROUPParms->corrBB_DCO_LUT_q)) - (size_t)pRXDCOGROUPParms), pParmsOffset);
    return((void*) pRXDCOGROUPParms);
#else
	 return((void*) NULL);
#endif
}

static RXDCOGROUP_OP_FUNC RXDCOGROUPOpFunc = NULL;

TLV2_API void registerRXDCOGROUPHandler(RXDCOGROUP_OP_FUNC fp)
{
    RXDCOGROUPOpFunc = fp;
}

A_BOOL RXDCOGROUPOp(void *pParms)
{
#if 0 // Helium code which is not compatible to Lithium code component.
    CMD_RXDCOGROUP_PARMS *pRXDCOGROUPParms = (CMD_RXDCOGROUP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("RXDCOGROUPOp: cmdIdRxDcoGroup %u\n", pRXDCOGROUPParms->cmdIdRxDcoGroup);
    A_PRINTF("RXDCOGROUPOp: phyId %u\n", pRXDCOGROUPParms->phyId);
    A_PRINTF("RXDCOGROUPOp: bandCode %u\n", pRXDCOGROUPParms->bandCode);
    A_PRINTF("RXDCOGROUPOp: bwCode %u\n", pRXDCOGROUPParms->bwCode);
    A_PRINTF("RXDCOGROUPOp: chainMask 0x%x\n", pRXDCOGROUPParms->chainMask);
    A_PRINTF("RXDCOGROUPOp: rxdco_cal_mode 0x%x\n", pRXDCOGROUPParms->rxdco_cal_mode);
    A_PRINTF("RXDCOGROUPOp: homeCh %u\n", pRXDCOGROUPParms->homeCh);
    A_PRINTF("RXDCOGROUPOp: initLUT %u\n", pRXDCOGROUPParms->initLUT);
    A_PRINTF("RXDCOGROUPOp: verbose %u\n", pRXDCOGROUPParms->verbose);
    A_PRINTF("RXDCOGROUPOp: pad1 %u\n", pRXDCOGROUPParms->pad1);
    A_PRINTF("RXDCOGROUPOp: bbOvrdI %u\n", pRXDCOGROUPParms->bbOvrdI);
    A_PRINTF("RXDCOGROUPOp: bbOvrdQ %u\n", pRXDCOGROUPParms->bbOvrdQ);
    A_PRINTF("RXDCOGROUPOp: rfOvrdI %u\n", pRXDCOGROUPParms->rfOvrdI);
    A_PRINTF("RXDCOGROUPOp: rfOvrdQ %u\n", pRXDCOGROUPParms->rfOvrdQ);
    A_PRINTF("RXDCOGROUPOp: userParm1 %u\n", pRXDCOGROUPParms->userParm1);
    A_PRINTF("RXDCOGROUPOp: userParm2 %u\n", pRXDCOGROUPParms->userParm2);
    A_PRINTF("RXDCOGROUPOp: userParm3 %u\n", pRXDCOGROUPParms->userParm3);
    A_PRINTF("RXDCOGROUPOp: userParm4 %u\n", pRXDCOGROUPParms->userParm4);
    A_PRINTF("RXDCOGROUPOp: bbDcocRange0I %u\n", pRXDCOGROUPParms->bbDcocRange0I);
    A_PRINTF("RXDCOGROUPOp: bbDcocRange0Q %u\n", pRXDCOGROUPParms->bbDcocRange0Q);
    A_PRINTF("RXDCOGROUPOp: rfDcocRange0I %u\n", pRXDCOGROUPParms->rfDcocRange0I);
    A_PRINTF("RXDCOGROUPOp: rfDcocRange0Q %u\n", pRXDCOGROUPParms->rfDcocRange0Q);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 256 entries
    {
        A_PRINTF("RXDCOGROUPOp: corrRF_DCO_LUT_i 0x%x\n", pRXDCOGROUPParms->corrRF_DCO_LUT_i[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 256 entries
    {
        A_PRINTF("RXDCOGROUPOp: corrRF_DCO_LUT_q 0x%x\n", pRXDCOGROUPParms->corrRF_DCO_LUT_q[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 256 entries
    {
        A_PRINTF("RXDCOGROUPOp: corrBB_DCO_LUT_i 0x%x\n", pRXDCOGROUPParms->corrBB_DCO_LUT_i[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 256 entries
    {
        A_PRINTF("RXDCOGROUPOp: corrBB_DCO_LUT_q 0x%x\n", pRXDCOGROUPParms->corrBB_DCO_LUT_q[i]);
    }
#endif //_DEBUG

    if (NULL != RXDCOGROUPOpFunc) {
        (*RXDCOGROUPOpFunc)(pRXDCOGROUPParms);
    }
#endif
    return(TRUE);
}

void* initRXDCOGROUPRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_RXDCOGROUPRSP_PARMS  *pRXDCOGROUPRSPParms = (CMD_RXDCOGROUPRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pRXDCOGROUPRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    memset(pRXDCOGROUPRSPParms->pad3, 0, sizeof(pRXDCOGROUPRSPParms->pad3));
    memset(pRXDCOGROUPRSPParms->calStatus, 0, sizeof(pRXDCOGROUPRSPParms->calStatus));
    pRXDCOGROUPRSPParms->bbDcocRange0I = pParmDict[PARM_BBDCOCRANGE0I].v.valU8;
    pRXDCOGROUPRSPParms->bbDcocRange0Q = pParmDict[PARM_BBDCOCRANGE0Q].v.valU8;
    pRXDCOGROUPRSPParms->rfDcocRange0I = pParmDict[PARM_RFDCOCRANGE0I].v.valU8;
    pRXDCOGROUPRSPParms->rfDcocRange0Q = pParmDict[PARM_RFDCOCRANGE0Q].v.valU8;
    memset(pRXDCOGROUPRSPParms->corrRF_DCO_LUT_i, 0, sizeof(pRXDCOGROUPRSPParms->corrRF_DCO_LUT_i));
    memset(pRXDCOGROUPRSPParms->corrRF_DCO_LUT_q, 0, sizeof(pRXDCOGROUPRSPParms->corrRF_DCO_LUT_q));
    memset(pRXDCOGROUPRSPParms->corrBB_DCO_LUT_i, 0, sizeof(pRXDCOGROUPRSPParms->corrBB_DCO_LUT_i));
    memset(pRXDCOGROUPRSPParms->corrBB_DCO_LUT_q, 0, sizeof(pRXDCOGROUPRSPParms->corrBB_DCO_LUT_q));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pRXDCOGROUPRSPParms->phyId)) - (size_t)pRXDCOGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD3, (size_t)(((size_t)&(pRXDCOGROUPRSPParms->pad3)) - (size_t)pRXDCOGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALSTATUS, (size_t)(((size_t)&(pRXDCOGROUPRSPParms->calStatus)) - (size_t)pRXDCOGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BBDCOCRANGE0I, (size_t)(((size_t)&(pRXDCOGROUPRSPParms->bbDcocRange0I)) - (size_t)pRXDCOGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BBDCOCRANGE0Q, (size_t)(((size_t)&(pRXDCOGROUPRSPParms->bbDcocRange0Q)) - (size_t)pRXDCOGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RFDCOCRANGE0I, (size_t)(((size_t)&(pRXDCOGROUPRSPParms->rfDcocRange0I)) - (size_t)pRXDCOGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RFDCOCRANGE0Q, (size_t)(((size_t)&(pRXDCOGROUPRSPParms->rfDcocRange0Q)) - (size_t)pRXDCOGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CORRRF_DCO_LUT_I, (size_t)(((size_t)&(pRXDCOGROUPRSPParms->corrRF_DCO_LUT_i)) - (size_t)pRXDCOGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CORRRF_DCO_LUT_Q, (size_t)(((size_t)&(pRXDCOGROUPRSPParms->corrRF_DCO_LUT_q)) - (size_t)pRXDCOGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CORRBB_DCO_LUT_I, (size_t)(((size_t)&(pRXDCOGROUPRSPParms->corrBB_DCO_LUT_i)) - (size_t)pRXDCOGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CORRBB_DCO_LUT_Q, (size_t)(((size_t)&(pRXDCOGROUPRSPParms->corrBB_DCO_LUT_q)) - (size_t)pRXDCOGROUPRSPParms), pParmsOffset);
    return((void*) pRXDCOGROUPRSPParms);
}

static RXDCOGROUPRSP_OP_FUNC RXDCOGROUPRSPOpFunc = NULL;

TLV2_API void registerRXDCOGROUPRSPHandler(RXDCOGROUPRSP_OP_FUNC fp)
{
    RXDCOGROUPRSPOpFunc = fp;
}

A_BOOL RXDCOGROUPRSPOp(void *pParms)
{
    CMD_RXDCOGROUPRSP_PARMS *pRXDCOGROUPRSPParms = (CMD_RXDCOGROUPRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("RXDCOGROUPRSPOp: phyId %u\n", pRXDCOGROUPRSPParms->phyId);
    for (i = 0; i < 3 ; i++)
    {
        A_PRINTF("RXDCOGROUPRSPOp: pad3 %u\n", pRXDCOGROUPRSPParms->pad3[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("RXDCOGROUPRSPOp: calStatus %u\n", pRXDCOGROUPRSPParms->calStatus[i]);
    }
    A_PRINTF("RXDCOGROUPRSPOp: bbDcocRange0I %u\n", pRXDCOGROUPRSPParms->bbDcocRange0I);
    A_PRINTF("RXDCOGROUPRSPOp: bbDcocRange0Q %u\n", pRXDCOGROUPRSPParms->bbDcocRange0Q);
    A_PRINTF("RXDCOGROUPRSPOp: rfDcocRange0I %u\n", pRXDCOGROUPRSPParms->rfDcocRange0I);
    A_PRINTF("RXDCOGROUPRSPOp: rfDcocRange0Q %u\n", pRXDCOGROUPRSPParms->rfDcocRange0Q);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 256 entries
    {
        A_PRINTF("RXDCOGROUPRSPOp: corrRF_DCO_LUT_i 0x%x\n", pRXDCOGROUPRSPParms->corrRF_DCO_LUT_i[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 256 entries
    {
        A_PRINTF("RXDCOGROUPRSPOp: corrRF_DCO_LUT_q 0x%x\n", pRXDCOGROUPRSPParms->corrRF_DCO_LUT_q[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 256 entries
    {
        A_PRINTF("RXDCOGROUPRSPOp: corrBB_DCO_LUT_i 0x%x\n", pRXDCOGROUPRSPParms->corrBB_DCO_LUT_i[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 256 entries
    {
        A_PRINTF("RXDCOGROUPRSPOp: corrBB_DCO_LUT_q 0x%x\n", pRXDCOGROUPRSPParms->corrBB_DCO_LUT_q[i]);
    }
#endif //_DEBUG

    if (NULL != RXDCOGROUPRSPOpFunc) {
        (*RXDCOGROUPRSPOpFunc)(pRXDCOGROUPRSPParms);
    }
    return(TRUE);
}
