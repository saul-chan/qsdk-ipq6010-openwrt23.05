/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdLMRxHandler.s
#include "tlv2Inc.h"
#include "cmdLMRxHandler.h"

void* initLMRXOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    int i; 	//for initializing array parameter
    CMD_LMRX_PARMS  *pLMRXParms = (CMD_LMRX_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);

    i = 0;	//assign a number to avoid warning in case i is not used

    // Populate the parm structure with initial values
    pLMRXParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pLMRXParms->rxMode = pParmDict[PARM_RXMODE].v.valU8;
    memset(pLMRXParms->lm_resvd1, 0, sizeof(pLMRXParms->lm_resvd1));
    memset(pLMRXParms->lm_freq, 0, sizeof(pLMRXParms->lm_freq));
    for (i = 0; i < 2 ; i++)
    {
        pLMRXParms->lm_freq[i] = pParmDict[PARM_LM_FREQ].v.ptU16[i];
    }
    memset(pLMRXParms->lm_freq2, 0, sizeof(pLMRXParms->lm_freq2));
    for (i = 0; i < 2 ; i++)
    {
        pLMRXParms->lm_freq2[i] = pParmDict[PARM_LM_FREQ2].v.ptU16[i];
    }
    memset(pLMRXParms->lm_chainMask, 0, sizeof(pLMRXParms->lm_chainMask));
    for (i = 0; i < 1 ; i++)
    {
        pLMRXParms->lm_chainMask[i] = pParmDict[PARM_LM_CHAINMASK].v.ptU8[i];
    }
    memset(pLMRXParms->wlanModeMask, 0, sizeof(pLMRXParms->wlanModeMask));
    for (i = 0; i < 1 ; i++)
    {
        pLMRXParms->wlanModeMask[i] = pParmDict[PARM_WLANMODEMASK].v.ptU32[i];
    }
    memset(pLMRXParms->wlanModeMaskExt, 0, sizeof(pLMRXParms->wlanModeMaskExt));
    memset(pLMRXParms->lm_bandwidth, 0, sizeof(pLMRXParms->lm_bandwidth));
    pLMRXParms->antenna = pParmDict[PARM_ANTENNA].v.valU8;
    pLMRXParms->enANI = pParmDict[PARM_ENANI].v.valU8;
    pLMRXParms->ack = pParmDict[PARM_ACK].v.valU8;
    pLMRXParms->broadcast = pParmDict[PARM_BROADCAST].v.valU8;
    pLMRXParms->lpl = pParmDict[PARM_LPL].v.valU8;
    pLMRXParms->antswitch1 = pParmDict[PARM_ANTSWITCH1].v.valU8;
    pLMRXParms->antswitch2 = pParmDict[PARM_ANTSWITCH2].v.valU8;
    pLMRXParms->lm_resvd0 = pParmDict[PARM_LM_RESVD0].v.valU8;
    pLMRXParms->spectralScan = pParmDict[PARM_SPECTRALSCAN].v.valU16;
    pLMRXParms->noiseFloor = pParmDict[PARM_NOISEFLOOR].v.valS16;
    memset(pLMRXParms->regDmn, 0, sizeof(pLMRXParms->regDmn));
    pLMRXParms->expectedPkts = pParmDict[PARM_EXPECTEDPKTS].v.valU32;
    pLMRXParms->otpWriteFlag = pParmDict[PARM_OTPWRITEFLAG].v.valU32;
    pLMRXParms->flags = pParmDict[PARM_FLAGS].v.valU32;
    for (i = 0; i < 3 ; i++)
    {
        pLMRXParms->rateMask[i] = pParmDict[PARM_RATEMASK].v.ptU32[i];
    }
    memset(pLMRXParms->rateMask11AC, 0, sizeof(pLMRXParms->rateMask11AC));
    pLMRXParms->rateMaskAC160 = pParmDict[PARM_RATEMASKAC160].v.valU32;
    for (i = 0; i < 6 ; i++)
    {
        pLMRXParms->bssid[i] = pParmDict[PARM_BSSID].v.ptU8[i];
    }
    for (i = 0; i < 6 ; i++)
    {
        pLMRXParms->staAddr[i] = pParmDict[PARM_STAADDR].v.ptU8[i];
    }
    memset(pLMRXParms->btAddr, 0, sizeof(pLMRXParms->btAddr));
    memset(pLMRXParms->pad2, 0, sizeof(pLMRXParms->pad2));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pLMRXParms->phyId)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXMODE, (size_t)(((size_t)&(pLMRXParms->rxMode)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_RESVD1, (size_t)(((size_t)&(pLMRXParms->lm_resvd1)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_FREQ, (size_t)(((size_t)&(pLMRXParms->lm_freq)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_FREQ2, (size_t)(((size_t)&(pLMRXParms->lm_freq2)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_CHAINMASK, (size_t)(((size_t)&(pLMRXParms->lm_chainMask)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_WLANMODEMASK, (size_t)(((size_t)&(pLMRXParms->wlanModeMask)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_WLANMODEMASKEXT, (size_t)(((size_t)&(pLMRXParms->wlanModeMaskExt)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_BANDWIDTH, (size_t)(((size_t)&(pLMRXParms->lm_bandwidth)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ANTENNA, (size_t)(((size_t)&(pLMRXParms->antenna)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENANI, (size_t)(((size_t)&(pLMRXParms->enANI)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ACK, (size_t)(((size_t)&(pLMRXParms->ack)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BROADCAST, (size_t)(((size_t)&(pLMRXParms->broadcast)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LPL, (size_t)(((size_t)&(pLMRXParms->lpl)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ANTSWITCH1, (size_t)(((size_t)&(pLMRXParms->antswitch1)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ANTSWITCH2, (size_t)(((size_t)&(pLMRXParms->antswitch2)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_RESVD0, (size_t)(((size_t)&(pLMRXParms->lm_resvd0)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SPECTRALSCAN, (size_t)(((size_t)&(pLMRXParms->spectralScan)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NOISEFLOOR, (size_t)(((size_t)&(pLMRXParms->noiseFloor)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_REGDMN, (size_t)(((size_t)&(pLMRXParms->regDmn)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_EXPECTEDPKTS, (size_t)(((size_t)&(pLMRXParms->expectedPkts)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_OTPWRITEFLAG, (size_t)(((size_t)&(pLMRXParms->otpWriteFlag)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FLAGS, (size_t)(((size_t)&(pLMRXParms->flags)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASK, (size_t)(((size_t)&(pLMRXParms->rateMask)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASK11AC, (size_t)(((size_t)&(pLMRXParms->rateMask11AC)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASKAC160, (size_t)(((size_t)&(pLMRXParms->rateMaskAC160)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BSSID, (size_t)(((size_t)&(pLMRXParms->bssid)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STAADDR, (size_t)(((size_t)&(pLMRXParms->staAddr)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BTADDR, (size_t)(((size_t)&(pLMRXParms->btAddr)) - (size_t)pLMRXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD2, (size_t)(((size_t)&(pLMRXParms->pad2)) - (size_t)pLMRXParms), pParmsOffset);
    return((void*) pLMRXParms);
}

static LMRX_OP_FUNC LMRXOpFunc = NULL;

TLV2_API void registerLMRXHandler(LMRX_OP_FUNC fp)
{
    LMRXOpFunc = fp;
}

A_BOOL LMRXOp(void *pParms)
{
    CMD_LMRX_PARMS *pLMRXParms = (CMD_LMRX_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG
    int i; 	//for initializing array parameter
    i = 0;	//assign a number to avoid warning in case i is not used

    A_PRINTF("LMRXOp: phyId %u\n", pLMRXParms->phyId);
    A_PRINTF("LMRXOp: rxMode %u\n", pLMRXParms->rxMode);
    for (i = 0; i < 2 ; i++)
    {
        A_PRINTF("LMRXOp: lm_resvd1 %u\n", pLMRXParms->lm_resvd1[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 36 entries
    {
        A_PRINTF("LMRXOp: lm_freq %u\n", pLMRXParms->lm_freq[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 36 entries
    {
        A_PRINTF("LMRXOp: lm_freq2 %u\n", pLMRXParms->lm_freq2[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 36 entries
    {
        A_PRINTF("LMRXOp: lm_chainMask 0x%x\n", pLMRXParms->lm_chainMask[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 36 entries
    {
        A_PRINTF("LMRXOp: wlanModeMask 0x%x\n", pLMRXParms->wlanModeMask[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 36 entries
    {
        A_PRINTF("LMRXOp: wlanModeMaskExt 0x%x\n", pLMRXParms->wlanModeMaskExt[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 36 entries
    {
        A_PRINTF("LMRXOp: lm_bandwidth %u\n", pLMRXParms->lm_bandwidth[i]);
    }
    A_PRINTF("LMRXOp: antenna %u\n", pLMRXParms->antenna);
    A_PRINTF("LMRXOp: enANI %u\n", pLMRXParms->enANI);
    A_PRINTF("LMRXOp: ack %u\n", pLMRXParms->ack);
    A_PRINTF("LMRXOp: broadcast %u\n", pLMRXParms->broadcast);
    A_PRINTF("LMRXOp: lpl %u\n", pLMRXParms->lpl);
    A_PRINTF("LMRXOp: antswitch1 %u\n", pLMRXParms->antswitch1);
    A_PRINTF("LMRXOp: antswitch2 %u\n", pLMRXParms->antswitch2);
    A_PRINTF("LMRXOp: lm_resvd0 %u\n", pLMRXParms->lm_resvd0);
    A_PRINTF("LMRXOp: spectralScan %u\n", pLMRXParms->spectralScan);
    A_PRINTF("LMRXOp: noiseFloor %d\n", pLMRXParms->noiseFloor);
    for (i = 0; i < 2 ; i++)
    {
        A_PRINTF("LMRXOp: regDmn 0x%x\n", pLMRXParms->regDmn[i]);
    }
    A_PRINTF("LMRXOp: expectedPkts %u\n", pLMRXParms->expectedPkts);
    A_PRINTF("LMRXOp: otpWriteFlag 0x%x\n", pLMRXParms->otpWriteFlag);
    A_PRINTF("LMRXOp: flags 0x%x\n", pLMRXParms->flags);
    for (i = 0; i < 3 ; i++)
    {
        A_PRINTF("LMRXOp: rateMask 0x%x\n", pLMRXParms->rateMask[i]);
    }
    for (i = 0; i < 5 ; i++)
    {
        A_PRINTF("LMRXOp: rateMask11AC 0x%x\n", pLMRXParms->rateMask11AC[i]);
    }
    A_PRINTF("LMRXOp: rateMaskAC160 0x%x\n", pLMRXParms->rateMaskAC160);
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("LMRXOp: bssid 0x%x\n", pLMRXParms->bssid[i]);
    }
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("LMRXOp: staAddr 0x%x\n", pLMRXParms->staAddr[i]);
    }
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("LMRXOp: btAddr 0x%x\n", pLMRXParms->btAddr[i]);
    }
    for (i = 0; i < 2 ; i++)
    {
        A_PRINTF("LMRXOp: pad2 %u\n", pLMRXParms->pad2[i]);
    }
#endif //_DEBUG

    if (NULL != LMRXOpFunc) {
        (*LMRXOpFunc)(pLMRXParms);
    }
    return(TRUE);
}
