/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdLMRxInitHandler.s
#include "tlv2Inc.h"
#include "cmdLMRxInitHandler.h"

void* initLMRXINITOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    int i, j; 	//for initializing array parameter
    CMD_LMRXINIT_PARMS  *pLMRXINITParms = (CMD_LMRXINIT_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);

    i = j = 0;	//assign a number to avoid warning in case i and j are not used

    // Populate the parm structure with initial values
    pLMRXINITParms->rxMode = pParmDict[PARM_RXMODE].v.valU8;
    pLMRXINITParms->bandwidth = pParmDict[PARM_BANDWIDTH].v.valU8;
    pLMRXINITParms->antenna = pParmDict[PARM_ANTENNA].v.valU8;
    pLMRXINITParms->enANI = pParmDict[PARM_ENANI].v.valU8;
    pLMRXINITParms->ack = pParmDict[PARM_ACK].v.valU8;
    pLMRXINITParms->broadcast = pParmDict[PARM_BROADCAST].v.valU8;
    pLMRXINITParms->lpl = pParmDict[PARM_LPL].v.valU8;
    pLMRXINITParms->antswitch1 = pParmDict[PARM_ANTSWITCH1].v.valU8;
    pLMRXINITParms->antswitch2 = pParmDict[PARM_ANTSWITCH2].v.valU8;
    pLMRXINITParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    memset(pLMRXINITParms->pad2, 0, sizeof(pLMRXINITParms->pad2));
    pLMRXINITParms->spectralScan = pParmDict[PARM_SPECTRALSCAN].v.valU16;
    pLMRXINITParms->noiseFloor = pParmDict[PARM_NOISEFLOOR].v.valS16;
    for (i = 0; i < 2 ; i++)
    {
        pLMRXINITParms->regDmn[i] = pParmDict[PARM_REGDMN].v.ptU16[i];
    }
    pLMRXINITParms->expectedPkts = pParmDict[PARM_EXPECTEDPKTS].v.valU32;
    pLMRXINITParms->otpWriteFlag = pParmDict[PARM_OTPWRITEFLAG].v.valU32;
    pLMRXINITParms->flags = pParmDict[PARM_FLAGS].v.valU32;
    for (i = 0; i < 6 ; i++)
    {
        pLMRXINITParms->bssid[i] = pParmDict[PARM_BSSID].v.ptU8[i];
    }
    for (i = 0; i < 6 ; i++)
    {
        pLMRXINITParms->staAddr[i] = pParmDict[PARM_STAADDR].v.ptU8[i];
    }
    for (i = 0; i < 6 ; i++)
    {
        pLMRXINITParms->btAddr[i] = pParmDict[PARM_BTADDR].v.ptU8[i];
    }

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_RXMODE, (size_t)(((size_t)&(pLMRXINITParms->rxMode)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BANDWIDTH, (size_t)(((size_t)&(pLMRXINITParms->bandwidth)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ANTENNA, (size_t)(((size_t)&(pLMRXINITParms->antenna)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENANI, (size_t)(((size_t)&(pLMRXINITParms->enANI)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ACK, (size_t)(((size_t)&(pLMRXINITParms->ack)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BROADCAST, (size_t)(((size_t)&(pLMRXINITParms->broadcast)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LPL, (size_t)(((size_t)&(pLMRXINITParms->lpl)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ANTSWITCH1, (size_t)(((size_t)&(pLMRXINITParms->antswitch1)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ANTSWITCH2, (size_t)(((size_t)&(pLMRXINITParms->antswitch2)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pLMRXINITParms->phyId)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD2, (size_t)(((size_t)&(pLMRXINITParms->pad2)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SPECTRALSCAN, (size_t)(((size_t)&(pLMRXINITParms->spectralScan)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NOISEFLOOR, (size_t)(((size_t)&(pLMRXINITParms->noiseFloor)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_REGDMN, (size_t)(((size_t)&(pLMRXINITParms->regDmn)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_EXPECTEDPKTS, (size_t)(((size_t)&(pLMRXINITParms->expectedPkts)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_OTPWRITEFLAG, (size_t)(((size_t)&(pLMRXINITParms->otpWriteFlag)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FLAGS, (size_t)(((size_t)&(pLMRXINITParms->flags)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BSSID, (size_t)(((size_t)&(pLMRXINITParms->bssid)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STAADDR, (size_t)(((size_t)&(pLMRXINITParms->staAddr)) - (size_t)pLMRXINITParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BTADDR, (size_t)(((size_t)&(pLMRXINITParms->btAddr)) - (size_t)pLMRXINITParms), pParmsOffset);
    return((void*) pLMRXINITParms);
}

static LMRXINIT_OP_FUNC LMRXINITOpFunc = NULL;

TLV2_API void registerLMRXINITHandler(LMRXINIT_OP_FUNC fp)
{
    LMRXINITOpFunc = fp;
}

A_BOOL LMRXINITOp(void *pParms)
{
    CMD_LMRXINIT_PARMS *pLMRXINITParms = (CMD_LMRXINIT_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("LMRXINITOp: rxMode %u\n", pLMRXINITParms->rxMode);
    A_PRINTF("LMRXINITOp: bandwidth %u\n", pLMRXINITParms->bandwidth);
    A_PRINTF("LMRXINITOp: antenna %u\n", pLMRXINITParms->antenna);
    A_PRINTF("LMRXINITOp: enANI %u\n", pLMRXINITParms->enANI);
    A_PRINTF("LMRXINITOp: ack %u\n", pLMRXINITParms->ack);
    A_PRINTF("LMRXINITOp: broadcast %u\n", pLMRXINITParms->broadcast);
    A_PRINTF("LMRXINITOp: lpl %u\n", pLMRXINITParms->lpl);
    A_PRINTF("LMRXINITOp: antswitch1 %u\n", pLMRXINITParms->antswitch1);
    A_PRINTF("LMRXINITOp: antswitch2 %u\n", pLMRXINITParms->antswitch2);
    A_PRINTF("LMRXINITOp: phyId %u\n", pLMRXINITParms->phyId);
    for (i = 0; i < 2 ; i++)
    {
        A_PRINTF("LMRXINITOp: pad2 %u\n", pLMRXINITParms->pad2[i]);
    }
    A_PRINTF("LMRXINITOp: spectralScan %u\n", pLMRXINITParms->spectralScan);
    A_PRINTF("LMRXINITOp: noiseFloor %d\n", pLMRXINITParms->noiseFloor);
    for (i = 0; i < 2 ; i++)
    {
        A_PRINTF("LMRXINITOp: regDmn 0x%x\n", pLMRXINITParms->regDmn[i]);
    }
    A_PRINTF("LMRXINITOp: expectedPkts %u\n", pLMRXINITParms->expectedPkts);
    A_PRINTF("LMRXINITOp: otpWriteFlag 0x%x\n", pLMRXINITParms->otpWriteFlag);
    A_PRINTF("LMRXINITOp: flags 0x%x\n", pLMRXINITParms->flags);
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("LMRXINITOp: bssid 0x%x\n", pLMRXINITParms->bssid[i]);
    }
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("LMRXINITOp: staAddr 0x%x\n", pLMRXINITParms->staAddr[i]);
    }
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("LMRXINITOp: btAddr 0x%x\n", pLMRXINITParms->btAddr[i]);
    }
#endif //_DEBUG

    if (NULL != LMRXINITOpFunc) {
        (*LMRXINITOpFunc)(pLMRXINITParms);
    }
    return(TRUE);
}

void* initLMRXINITRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_LMRXINITRSP_PARMS  *pLMRXINITRSPParms = (CMD_LMRXINITRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pLMRXINITRSPParms->cmdId = pParmDict[PARM_CMDID].v.valU16;
    pLMRXINITRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pLMRXINITRSPParms->pad1 = pParmDict[PARM_PAD1].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_CMDID, (size_t)(((size_t)&(pLMRXINITRSPParms->cmdId)) - (size_t)pLMRXINITRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pLMRXINITRSPParms->status)) - (size_t)pLMRXINITRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD1, (size_t)(((size_t)&(pLMRXINITRSPParms->pad1)) - (size_t)pLMRXINITRSPParms), pParmsOffset);
    return((void*) pLMRXINITRSPParms);
}

static LMRXINITRSP_OP_FUNC LMRXINITRSPOpFunc = NULL;

TLV2_API void registerLMRXINITRSPHandler(LMRXINITRSP_OP_FUNC fp)
{
    LMRXINITRSPOpFunc = fp;
}

A_BOOL LMRXINITRSPOp(void *pParms)
{
    CMD_LMRXINITRSP_PARMS *pLMRXINITRSPParms = (CMD_LMRXINITRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("LMRXINITRSPOp: cmdId %u\n", pLMRXINITRSPParms->cmdId);
    A_PRINTF("LMRXINITRSPOp: status %u\n", pLMRXINITRSPParms->status);
    A_PRINTF("LMRXINITRSPOp: pad1 %u\n", pLMRXINITRSPParms->pad1);
#endif //_DEBUG

    if (NULL != LMRXINITRSPOpFunc) {
        (*LMRXINITRSPOpFunc)(pLMRXINITRSPParms);
    }
    return(TRUE);
}
