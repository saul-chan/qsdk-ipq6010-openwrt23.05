/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdLMTxHandler.s
#include "tlv2Inc.h"
#include "cmdLMTxHandler.h"

void* initLMTXOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    int i; 	//for initializing array parameter
    CMD_LMTX_PARMS  *pLMTXParms = (CMD_LMTX_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);

    i = 0;	//assign a number to avoid warning in case i is not used

    // Populate the parm structure with initial values
    pLMTXParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pLMTXParms->txMode = pParmDict[PARM_TXMODE].v.valU8;
    memset(pLMTXParms->lm_resvd1, 0, sizeof(pLMTXParms->lm_resvd1));
    memset(pLMTXParms->lm_freq, 0, sizeof(pLMTXParms->lm_freq));
    for (i = 0; i < 2 ; i++)
    {
        pLMTXParms->lm_freq[i] = pParmDict[PARM_LM_FREQ].v.ptU16[i];
    }
    memset(pLMTXParms->lm_freq2, 0, sizeof(pLMTXParms->lm_freq2));
    for (i = 0; i < 2 ; i++)
    {
        pLMTXParms->lm_freq2[i] = pParmDict[PARM_LM_FREQ2].v.ptU16[i];
    }
    memset(pLMTXParms->lm_chainMask, 0, sizeof(pLMTXParms->lm_chainMask));
    for (i = 0; i < 1 ; i++)
    {
        pLMTXParms->lm_chainMask[i] = pParmDict[PARM_LM_CHAINMASK].v.ptU8[i];
    }
    memset(pLMTXParms->wlanModeMask, 0, sizeof(pLMTXParms->wlanModeMask));
    for (i = 0; i < 1 ; i++)
    {
        pLMTXParms->wlanModeMask[i] = pParmDict[PARM_WLANMODEMASK].v.ptU32[i];
    }
    memset(pLMTXParms->wlanModeMaskExt, 0, sizeof(pLMTXParms->wlanModeMaskExt));
    memset(pLMTXParms->lm_bandwidth, 0, sizeof(pLMTXParms->lm_bandwidth));
    pLMTXParms->gI = pParmDict[PARM_GI].v.valU8;
    pLMTXParms->antenna = pParmDict[PARM_ANTENNA].v.valU8;
    pLMTXParms->enANI = pParmDict[PARM_ENANI].v.valU8;
    pLMTXParms->scramblerOff = pParmDict[PARM_SCRAMBLEROFF].v.valU8;
    pLMTXParms->aifsn = pParmDict[PARM_AIFSN].v.valU8;
    pLMTXParms->broadcast = pParmDict[PARM_BROADCAST].v.valU8;
    pLMTXParms->agg = pParmDict[PARM_AGG].v.valU8;
    pLMTXParms->shortGuard = pParmDict[PARM_SHORTGUARD].v.valU8;
    pLMTXParms->dutyCycle = pParmDict[PARM_DUTYCYCLE].v.valU8;
    memset(pLMTXParms->lm_resvd0, 0, sizeof(pLMTXParms->lm_resvd0));
    pLMTXParms->flags = pParmDict[PARM_FLAGS].v.valU32;
    pLMTXParms->ir = pParmDict[PARM_IR].v.valU32;
    pLMTXParms->pktSz = pParmDict[PARM_PKTSZ].v.valU32;
    memset(pLMTXParms->dataPattern, 0, sizeof(pLMTXParms->dataPattern));
    pLMTXParms->txNumPackets = pParmDict[PARM_TXNUMPACKETS].v.valU16;
    pLMTXParms->txPattern = pParmDict[PARM_TXPATTERN].v.valU16;
    pLMTXParms->nPattern = pParmDict[PARM_NPATTERN].v.valU8;
    pLMTXParms->tpcm = pParmDict[PARM_TPCM].v.valU8;
    pLMTXParms->gainIdx = pParmDict[PARM_GAINIDX].v.valU8;
    pLMTXParms->dacGain = pParmDict[PARM_DACGAIN].v.valS8;
    pLMTXParms->dacGainEnd = pParmDict[PARM_DACGAINEND].v.valS8;
    pLMTXParms->dacGainStep = pParmDict[PARM_DACGAINSTEP].v.valS8;
    pLMTXParms->paConfig = pParmDict[PARM_PACONFIG].v.valU8;
    pLMTXParms->paConfigEnd = pParmDict[PARM_PACONFIGEND].v.valU8;
    pLMTXParms->paConfigStep = pParmDict[PARM_PACONFIGSTEP].v.valU8;
    memset(pLMTXParms->pad3, 0, sizeof(pLMTXParms->pad3));
    for (i = 0; i < 3 ; i++)
    {
        pLMTXParms->rateMask[i] = pParmDict[PARM_RATEMASK].v.ptU32[i];
    }
    memset(pLMTXParms->rateMask11AC, 0, sizeof(pLMTXParms->rateMask11AC));
    pLMTXParms->rateMaskAC160 = pParmDict[PARM_RATEMASKAC160].v.valU32;
    for (i = 0; i < 24 ; i++)
    {
        pLMTXParms->pwrGainStart[i] = pParmDict[PARM_PWRGAINSTART].v.ptS32[i];
    }
    for (i = 0; i < 24 ; i++)
    {
        pLMTXParms->pwrGainEnd[i] = pParmDict[PARM_PWRGAINEND].v.ptS32[i];
    }
    for (i = 0; i < 24 ; i++)
    {
        pLMTXParms->pwrGainStep[i] = pParmDict[PARM_PWRGAINSTEP].v.ptS32[i];
    }
    for (i = 0; i < 40 ; i++)
    {
        pLMTXParms->pwrGainStart11AC[i] = pParmDict[PARM_PWRGAINSTART11AC].v.ptS32[i];
    }
    for (i = 0; i < 40 ; i++)
    {
        pLMTXParms->pwrGainEnd11AC[i] = pParmDict[PARM_PWRGAINEND11AC].v.ptS32[i];
    }
    for (i = 0; i < 40 ; i++)
    {
        pLMTXParms->pwrGainStep11AC[i] = pParmDict[PARM_PWRGAINSTEP11AC].v.ptS32[i];
    }
    for (i = 0; i < 8 ; i++)
    {
        pLMTXParms->pwrGainStartAC160[i] = pParmDict[PARM_PWRGAINSTARTAC160].v.ptS32[i];
    }
    for (i = 0; i < 8 ; i++)
    {
        pLMTXParms->pwrGainEndAC160[i] = pParmDict[PARM_PWRGAINENDAC160].v.ptS32[i];
    }
    for (i = 0; i < 8 ; i++)
    {
        pLMTXParms->pwrGainStepAC160[i] = pParmDict[PARM_PWRGAINSTEPAC160].v.ptS32[i];
    }
    for (i = 0; i < 6 ; i++)
    {
        pLMTXParms->bssid[i] = pParmDict[PARM_BSSID].v.ptU8[i];
    }
    for (i = 0; i < 6 ; i++)
    {
        pLMTXParms->txStation[i] = pParmDict[PARM_TXSTATION].v.ptU8[i];
    }
    for (i = 0; i < 6 ; i++)
    {
        pLMTXParms->rxStation[i] = pParmDict[PARM_RXSTATION].v.ptU8[i];
    }

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pLMTXParms->phyId)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXMODE, (size_t)(((size_t)&(pLMTXParms->txMode)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_RESVD1, (size_t)(((size_t)&(pLMTXParms->lm_resvd1)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_FREQ, (size_t)(((size_t)&(pLMTXParms->lm_freq)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_FREQ2, (size_t)(((size_t)&(pLMTXParms->lm_freq2)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_CHAINMASK, (size_t)(((size_t)&(pLMTXParms->lm_chainMask)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_WLANMODEMASK, (size_t)(((size_t)&(pLMTXParms->wlanModeMask)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_WLANMODEMASKEXT, (size_t)(((size_t)&(pLMTXParms->wlanModeMaskExt)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_BANDWIDTH, (size_t)(((size_t)&(pLMTXParms->lm_bandwidth)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GI, (size_t)(((size_t)&(pLMTXParms->gI)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ANTENNA, (size_t)(((size_t)&(pLMTXParms->antenna)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENANI, (size_t)(((size_t)&(pLMTXParms->enANI)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCRAMBLEROFF, (size_t)(((size_t)&(pLMTXParms->scramblerOff)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_AIFSN, (size_t)(((size_t)&(pLMTXParms->aifsn)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BROADCAST, (size_t)(((size_t)&(pLMTXParms->broadcast)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_AGG, (size_t)(((size_t)&(pLMTXParms->agg)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SHORTGUARD, (size_t)(((size_t)&(pLMTXParms->shortGuard)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DUTYCYCLE, (size_t)(((size_t)&(pLMTXParms->dutyCycle)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LM_RESVD0, (size_t)(((size_t)&(pLMTXParms->lm_resvd0)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FLAGS, (size_t)(((size_t)&(pLMTXParms->flags)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_IR, (size_t)(((size_t)&(pLMTXParms->ir)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PKTSZ, (size_t)(((size_t)&(pLMTXParms->pktSz)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DATAPATTERN, (size_t)(((size_t)&(pLMTXParms->dataPattern)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXNUMPACKETS, (size_t)(((size_t)&(pLMTXParms->txNumPackets)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXPATTERN, (size_t)(((size_t)&(pLMTXParms->txPattern)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NPATTERN, (size_t)(((size_t)&(pLMTXParms->nPattern)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TPCM, (size_t)(((size_t)&(pLMTXParms->tpcm)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GAINIDX, (size_t)(((size_t)&(pLMTXParms->gainIdx)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DACGAIN, (size_t)(((size_t)&(pLMTXParms->dacGain)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DACGAINEND, (size_t)(((size_t)&(pLMTXParms->dacGainEnd)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DACGAINSTEP, (size_t)(((size_t)&(pLMTXParms->dacGainStep)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PACONFIG, (size_t)(((size_t)&(pLMTXParms->paConfig)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PACONFIGEND, (size_t)(((size_t)&(pLMTXParms->paConfigEnd)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PACONFIGSTEP, (size_t)(((size_t)&(pLMTXParms->paConfigStep)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD3, (size_t)(((size_t)&(pLMTXParms->pad3)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASK, (size_t)(((size_t)&(pLMTXParms->rateMask)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASK11AC, (size_t)(((size_t)&(pLMTXParms->rateMask11AC)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASKAC160, (size_t)(((size_t)&(pLMTXParms->rateMaskAC160)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINSTART, (size_t)(((size_t)&(pLMTXParms->pwrGainStart)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINEND, (size_t)(((size_t)&(pLMTXParms->pwrGainEnd)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINSTEP, (size_t)(((size_t)&(pLMTXParms->pwrGainStep)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINSTART11AC, (size_t)(((size_t)&(pLMTXParms->pwrGainStart11AC)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINEND11AC, (size_t)(((size_t)&(pLMTXParms->pwrGainEnd11AC)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINSTEP11AC, (size_t)(((size_t)&(pLMTXParms->pwrGainStep11AC)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINSTARTAC160, (size_t)(((size_t)&(pLMTXParms->pwrGainStartAC160)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINENDAC160, (size_t)(((size_t)&(pLMTXParms->pwrGainEndAC160)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINSTEPAC160, (size_t)(((size_t)&(pLMTXParms->pwrGainStepAC160)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BSSID, (size_t)(((size_t)&(pLMTXParms->bssid)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXSTATION, (size_t)(((size_t)&(pLMTXParms->txStation)) - (size_t)pLMTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXSTATION, (size_t)(((size_t)&(pLMTXParms->rxStation)) - (size_t)pLMTXParms), pParmsOffset);
    return((void*) pLMTXParms);
}

static LMTX_OP_FUNC LMTXOpFunc = NULL;

TLV2_API void registerLMTXHandler(LMTX_OP_FUNC fp)
{
    LMTXOpFunc = fp;
}

A_BOOL LMTXOp(void *pParms)
{
    CMD_LMTX_PARMS *pLMTXParms = (CMD_LMTX_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG
    int i; 	//for initializing array parameter
    i = 0;	//assign a number to avoid warning in case i is not used

    A_PRINTF("LMTXOp: phyId %u\n", pLMTXParms->phyId);
    A_PRINTF("LMTXOp: txMode %u\n", pLMTXParms->txMode);
    for (i = 0; i < 2 ; i++)
    {
        A_PRINTF("LMTXOp: lm_resvd1 %u\n", pLMTXParms->lm_resvd1[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 36 entries
    {
        A_PRINTF("LMTXOp: lm_freq %u\n", pLMTXParms->lm_freq[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 36 entries
    {
        A_PRINTF("LMTXOp: lm_freq2 %u\n", pLMTXParms->lm_freq2[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 36 entries
    {
        A_PRINTF("LMTXOp: lm_chainMask 0x%x\n", pLMTXParms->lm_chainMask[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 36 entries
    {
        A_PRINTF("LMTXOp: wlanModeMask 0x%x\n", pLMTXParms->wlanModeMask[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 36 entries
    {
        A_PRINTF("LMTXOp: wlanModeMaskExt 0x%x\n", pLMTXParms->wlanModeMaskExt[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 36 entries
    {
        A_PRINTF("LMTXOp: lm_bandwidth %u\n", pLMTXParms->lm_bandwidth[i]);
    }
    A_PRINTF("LMTXOp: gI %u\n", pLMTXParms->gI);
    A_PRINTF("LMTXOp: antenna %u\n", pLMTXParms->antenna);
    A_PRINTF("LMTXOp: enANI %u\n", pLMTXParms->enANI);
    A_PRINTF("LMTXOp: scramblerOff %u\n", pLMTXParms->scramblerOff);
    A_PRINTF("LMTXOp: aifsn %u\n", pLMTXParms->aifsn);
    A_PRINTF("LMTXOp: broadcast %u\n", pLMTXParms->broadcast);
    A_PRINTF("LMTXOp: agg %u\n", pLMTXParms->agg);
    A_PRINTF("LMTXOp: shortGuard %u\n", pLMTXParms->shortGuard);
    A_PRINTF("LMTXOp: dutyCycle %u\n", pLMTXParms->dutyCycle);
    for (i = 0; i < 3 ; i++)
    {
        A_PRINTF("LMTXOp: lm_resvd0 %u\n", pLMTXParms->lm_resvd0[i]);
    }
    A_PRINTF("LMTXOp: flags 0x%x\n", pLMTXParms->flags);
    A_PRINTF("LMTXOp: ir %u\n", pLMTXParms->ir);
    A_PRINTF("LMTXOp: pktSz %u\n", pLMTXParms->pktSz);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 40 entries
    {
        A_PRINTF("LMTXOp: dataPattern %u\n", pLMTXParms->dataPattern[i]);
    }
    A_PRINTF("LMTXOp: txNumPackets %u\n", pLMTXParms->txNumPackets);
    A_PRINTF("LMTXOp: txPattern %u\n", pLMTXParms->txPattern);
    A_PRINTF("LMTXOp: nPattern %u\n", pLMTXParms->nPattern);
    A_PRINTF("LMTXOp: tpcm %u\n", pLMTXParms->tpcm);
    A_PRINTF("LMTXOp: gainIdx %u\n", pLMTXParms->gainIdx);
    A_PRINTF("LMTXOp: dacGain %d\n", pLMTXParms->dacGain);
    A_PRINTF("LMTXOp: dacGainEnd %d\n", pLMTXParms->dacGainEnd);
    A_PRINTF("LMTXOp: dacGainStep %d\n", pLMTXParms->dacGainStep);
    A_PRINTF("LMTXOp: paConfig %u\n", pLMTXParms->paConfig);
    A_PRINTF("LMTXOp: paConfigEnd %u\n", pLMTXParms->paConfigEnd);
    A_PRINTF("LMTXOp: paConfigStep %u\n", pLMTXParms->paConfigStep);
    for (i = 0; i < 3 ; i++)
    {
        A_PRINTF("LMTXOp: pad3 %u\n", pLMTXParms->pad3[i]);
    }
    for (i = 0; i < 3 ; i++)
    {
        A_PRINTF("LMTXOp: rateMask 0x%x\n", pLMTXParms->rateMask[i]);
    }
    for (i = 0; i < 5 ; i++)
    {
        A_PRINTF("LMTXOp: rateMask11AC 0x%x\n", pLMTXParms->rateMask11AC[i]);
    }
    A_PRINTF("LMTXOp: rateMaskAC160 0x%x\n", pLMTXParms->rateMaskAC160);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 24 entries
    {
        A_PRINTF("LMTXOp: pwrGainStart 0x%x\n", pLMTXParms->pwrGainStart[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 24 entries
    {
        A_PRINTF("LMTXOp: pwrGainEnd 0x%x\n", pLMTXParms->pwrGainEnd[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 24 entries
    {
        A_PRINTF("LMTXOp: pwrGainStep 0x%x\n", pLMTXParms->pwrGainStep[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 40 entries
    {
        A_PRINTF("LMTXOp: pwrGainStart11AC 0x%x\n", pLMTXParms->pwrGainStart11AC[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 40 entries
    {
        A_PRINTF("LMTXOp: pwrGainEnd11AC 0x%x\n", pLMTXParms->pwrGainEnd11AC[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 40 entries
    {
        A_PRINTF("LMTXOp: pwrGainStep11AC 0x%x\n", pLMTXParms->pwrGainStep11AC[i]);
    }
    for (i = 0; i < 8 ; i++)
    {
        A_PRINTF("LMTXOp: pwrGainStartAC160 0x%x\n", pLMTXParms->pwrGainStartAC160[i]);
    }
    for (i = 0; i < 8 ; i++)
    {
        A_PRINTF("LMTXOp: pwrGainEndAC160 0x%x\n", pLMTXParms->pwrGainEndAC160[i]);
    }
    for (i = 0; i < 8 ; i++)
    {
        A_PRINTF("LMTXOp: pwrGainStepAC160 0x%x\n", pLMTXParms->pwrGainStepAC160[i]);
    }
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("LMTXOp: bssid 0x%x\n", pLMTXParms->bssid[i]);
    }
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("LMTXOp: txStation 0x%x\n", pLMTXParms->txStation[i]);
    }
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("LMTXOp: rxStation 0x%x\n", pLMTXParms->rxStation[i]);
    }
#endif //_DEBUG

    if (NULL != LMTXOpFunc) {
        (*LMTXOpFunc)(pLMTXParms);
    }
    return(TRUE);
}
