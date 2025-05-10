/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdBacktoBackTxHandler.s
#include "tlv2Inc.h"
#include "cmdBacktoBackTxHandler.h"

void* initBACKTOBACKTXOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    int i, j; 	//for initializing array parameter
    CMD_BACKTOBACKTX_PARMS  *pBACKTOBACKTXParms = (CMD_BACKTOBACKTX_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);

    i = j = 0;	//assign a number to avoid warning in case i and j are not used

    // Populate the parm structure with initial values
    pBACKTOBACKTXParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pBACKTOBACKTXParms->txMode = pParmDict[PARM_TXMODE].v.valU8;
    memset(pBACKTOBACKTXParms->lmfreq, 0, sizeof(pBACKTOBACKTXParms->lmfreq));
    pBACKTOBACKTXParms->freq2 = pParmDict[PARM_FREQ2].v.valU16;
    memset(pBACKTOBACKTXParms->lmchainMask, 0, sizeof(pBACKTOBACKTXParms->lmchainMask));
    pBACKTOBACKTXParms->wlanMode = pParmDict[PARM_WLANMODE].v.valU8;
    pBACKTOBACKTXParms->bandwidth = pParmDict[PARM_BANDWIDTH].v.valU8;
    pBACKTOBACKTXParms->gI = pParmDict[PARM_GI].v.valU8;
    pBACKTOBACKTXParms->antenna = pParmDict[PARM_ANTENNA].v.valU8;
    pBACKTOBACKTXParms->enANI = pParmDict[PARM_ENANI].v.valU8;
    pBACKTOBACKTXParms->scramblerOff = pParmDict[PARM_SCRAMBLEROFF].v.valU8;
    pBACKTOBACKTXParms->aifsn = pParmDict[PARM_AIFSN].v.valU8;
    pBACKTOBACKTXParms->broadcast = pParmDict[PARM_BROADCAST].v.valU8;
    pBACKTOBACKTXParms->agg = pParmDict[PARM_AGG].v.valU8;
    pBACKTOBACKTXParms->shortGuard = pParmDict[PARM_SHORTGUARD].v.valU8;
    pBACKTOBACKTXParms->dutyCycle = pParmDict[PARM_DUTYCYCLE].v.valU8;
    pBACKTOBACKTXParms->padding = pParmDict[PARM_PADDING].v.valU8;
    pBACKTOBACKTXParms->flags = pParmDict[PARM_FLAGS].v.valU32;
    pBACKTOBACKTXParms->ir = pParmDict[PARM_IR].v.valU32;
    memset(pBACKTOBACKTXParms->lmpktSz, 0, sizeof(pBACKTOBACKTXParms->lmpktSz));
    memset(pBACKTOBACKTXParms->dataPattern, 0, sizeof(pBACKTOBACKTXParms->dataPattern));
    memset(pBACKTOBACKTXParms->lmtxNumPackets, 0, sizeof(pBACKTOBACKTXParms->lmtxNumPackets));
    pBACKTOBACKTXParms->txPattern = pParmDict[PARM_TXPATTERN].v.valU16;
    pBACKTOBACKTXParms->nPattern = pParmDict[PARM_NPATTERN].v.valU8;
    pBACKTOBACKTXParms->tpcm = pParmDict[PARM_TPCM].v.valU8;
    pBACKTOBACKTXParms->gainIdx = pParmDict[PARM_GAINIDX].v.valU8;
    pBACKTOBACKTXParms->dacGain = pParmDict[PARM_DACGAIN].v.valS8;
    pBACKTOBACKTXParms->dacGainEnd = pParmDict[PARM_DACGAINEND].v.valS8;
    pBACKTOBACKTXParms->dacGainStep = pParmDict[PARM_DACGAINSTEP].v.valS8;
    pBACKTOBACKTXParms->paConfig = pParmDict[PARM_PACONFIG].v.valU8;
    pBACKTOBACKTXParms->paConfigEnd = pParmDict[PARM_PACONFIGEND].v.valU8;
    pBACKTOBACKTXParms->paConfigStep = pParmDict[PARM_PACONFIGSTEP].v.valU8;
    pBACKTOBACKTXParms->pad1 = pParmDict[PARM_PAD1].v.valU8;
    pBACKTOBACKTXParms->rateBitIndex = pParmDict[PARM_RATEBITINDEX].v.valU16;
    memset(pBACKTOBACKTXParms->lmrateIndex, 0, sizeof(pBACKTOBACKTXParms->lmrateIndex));
    for (i = 0; i < 3 ; i++)
    {
        pBACKTOBACKTXParms->rateMask[i] = pParmDict[PARM_RATEMASK].v.ptU32[i];
    }
    memset(pBACKTOBACKTXParms->rateMask11AC, 0, sizeof(pBACKTOBACKTXParms->rateMask11AC));
    pBACKTOBACKTXParms->rateMaskAC160 = pParmDict[PARM_RATEMASKAC160].v.valU32;
    for (i = 0; i < 24 ; i++)
    {
        pBACKTOBACKTXParms->pwrGainStart[i] = pParmDict[PARM_PWRGAINSTART].v.ptS32[i];
    }
    for (i = 0; i < 24 ; i++)
    {
        pBACKTOBACKTXParms->pwrGainEnd[i] = pParmDict[PARM_PWRGAINEND].v.ptS32[i];
    }
    for (i = 0; i < 24 ; i++)
    {
        pBACKTOBACKTXParms->pwrGainStep[i] = pParmDict[PARM_PWRGAINSTEP].v.ptS32[i];
    }
    for (i = 0; i < 40 ; i++)
    {
        pBACKTOBACKTXParms->pwrGainStart11AC[i] = pParmDict[PARM_PWRGAINSTART11AC].v.ptS32[i];
    }
    for (i = 0; i < 40 ; i++)
    {
        pBACKTOBACKTXParms->pwrGainEnd11AC[i] = pParmDict[PARM_PWRGAINEND11AC].v.ptS32[i];
    }
    for (i = 0; i < 40 ; i++)
    {
        pBACKTOBACKTXParms->pwrGainStep11AC[i] = pParmDict[PARM_PWRGAINSTEP11AC].v.ptS32[i];
    }
    for (i = 0; i < 8 ; i++)
    {
        pBACKTOBACKTXParms->pwrGainStartAC160[i] = pParmDict[PARM_PWRGAINSTARTAC160].v.ptS32[i];
    }
    for (i = 0; i < 8 ; i++)
    {
        pBACKTOBACKTXParms->pwrGainEndAC160[i] = pParmDict[PARM_PWRGAINENDAC160].v.ptS32[i];
    }
    for (i = 0; i < 8 ; i++)
    {
        pBACKTOBACKTXParms->pwrGainStepAC160[i] = pParmDict[PARM_PWRGAINSTEPAC160].v.ptS32[i];
    }
    memset(pBACKTOBACKTXParms->lmtxPower, 0, sizeof(pBACKTOBACKTXParms->lmtxPower));
    for (i = 0; i < 6 ; i++)
    {
        pBACKTOBACKTXParms->bssid[i] = pParmDict[PARM_BSSID].v.ptU8[i];
    }
    for (i = 0; i < 6 ; i++)
    {
        pBACKTOBACKTXParms->txStation[i] = pParmDict[PARM_TXSTATION].v.ptU8[i];
    }
    for (i = 0; i < 6 ; i++)
    {
        pBACKTOBACKTXParms->rxStation[i] = pParmDict[PARM_RXSTATION].v.ptU8[i];
    }
    pBACKTOBACKTXParms->wifiStandard = pParmDict[PARM_WIFISTANDARD].v.valU32;
    pBACKTOBACKTXParms->NSS = pParmDict[PARM_NSS].v.valU32;
    pBACKTOBACKTXParms->rate = pParmDict[PARM_RATE].v.valU8;
    pBACKTOBACKTXParms->tableLength = pParmDict[PARM_TABLELENGTH].v.valU16;
    pBACKTOBACKTXParms->cycleTime = pParmDict[PARM_CYCLETIME].v.valU16;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pBACKTOBACKTXParms->phyId)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXMODE, (size_t)(((size_t)&(pBACKTOBACKTXParms->txMode)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LMFREQ, (size_t)(((size_t)&(pBACKTOBACKTXParms->lmfreq)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ2, (size_t)(((size_t)&(pBACKTOBACKTXParms->freq2)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LMCHAINMASK, (size_t)(((size_t)&(pBACKTOBACKTXParms->lmchainMask)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_WLANMODE, (size_t)(((size_t)&(pBACKTOBACKTXParms->wlanMode)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BANDWIDTH, (size_t)(((size_t)&(pBACKTOBACKTXParms->bandwidth)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GI, (size_t)(((size_t)&(pBACKTOBACKTXParms->gI)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ANTENNA, (size_t)(((size_t)&(pBACKTOBACKTXParms->antenna)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ENANI, (size_t)(((size_t)&(pBACKTOBACKTXParms->enANI)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCRAMBLEROFF, (size_t)(((size_t)&(pBACKTOBACKTXParms->scramblerOff)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_AIFSN, (size_t)(((size_t)&(pBACKTOBACKTXParms->aifsn)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BROADCAST, (size_t)(((size_t)&(pBACKTOBACKTXParms->broadcast)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_AGG, (size_t)(((size_t)&(pBACKTOBACKTXParms->agg)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SHORTGUARD, (size_t)(((size_t)&(pBACKTOBACKTXParms->shortGuard)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DUTYCYCLE, (size_t)(((size_t)&(pBACKTOBACKTXParms->dutyCycle)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PADDING, (size_t)(((size_t)&(pBACKTOBACKTXParms->padding)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FLAGS, (size_t)(((size_t)&(pBACKTOBACKTXParms->flags)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_IR, (size_t)(((size_t)&(pBACKTOBACKTXParms->ir)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LMPKTSZ, (size_t)(((size_t)&(pBACKTOBACKTXParms->lmpktSz)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DATAPATTERN, (size_t)(((size_t)&(pBACKTOBACKTXParms->dataPattern)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LMTXNUMPACKETS, (size_t)(((size_t)&(pBACKTOBACKTXParms->lmtxNumPackets)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXPATTERN, (size_t)(((size_t)&(pBACKTOBACKTXParms->txPattern)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NPATTERN, (size_t)(((size_t)&(pBACKTOBACKTXParms->nPattern)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TPCM, (size_t)(((size_t)&(pBACKTOBACKTXParms->tpcm)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GAINIDX, (size_t)(((size_t)&(pBACKTOBACKTXParms->gainIdx)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DACGAIN, (size_t)(((size_t)&(pBACKTOBACKTXParms->dacGain)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DACGAINEND, (size_t)(((size_t)&(pBACKTOBACKTXParms->dacGainEnd)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DACGAINSTEP, (size_t)(((size_t)&(pBACKTOBACKTXParms->dacGainStep)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PACONFIG, (size_t)(((size_t)&(pBACKTOBACKTXParms->paConfig)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PACONFIGEND, (size_t)(((size_t)&(pBACKTOBACKTXParms->paConfigEnd)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PACONFIGSTEP, (size_t)(((size_t)&(pBACKTOBACKTXParms->paConfigStep)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD1, (size_t)(((size_t)&(pBACKTOBACKTXParms->pad1)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEBITINDEX, (size_t)(((size_t)&(pBACKTOBACKTXParms->rateBitIndex)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LMRATEINDEX, (size_t)(((size_t)&(pBACKTOBACKTXParms->lmrateIndex)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASK, (size_t)(((size_t)&(pBACKTOBACKTXParms->rateMask)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASK11AC, (size_t)(((size_t)&(pBACKTOBACKTXParms->rateMask11AC)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATEMASKAC160, (size_t)(((size_t)&(pBACKTOBACKTXParms->rateMaskAC160)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINSTART, (size_t)(((size_t)&(pBACKTOBACKTXParms->pwrGainStart)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINEND, (size_t)(((size_t)&(pBACKTOBACKTXParms->pwrGainEnd)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINSTEP, (size_t)(((size_t)&(pBACKTOBACKTXParms->pwrGainStep)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINSTART11AC, (size_t)(((size_t)&(pBACKTOBACKTXParms->pwrGainStart11AC)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINEND11AC, (size_t)(((size_t)&(pBACKTOBACKTXParms->pwrGainEnd11AC)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINSTEP11AC, (size_t)(((size_t)&(pBACKTOBACKTXParms->pwrGainStep11AC)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINSTARTAC160, (size_t)(((size_t)&(pBACKTOBACKTXParms->pwrGainStartAC160)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINENDAC160, (size_t)(((size_t)&(pBACKTOBACKTXParms->pwrGainEndAC160)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PWRGAINSTEPAC160, (size_t)(((size_t)&(pBACKTOBACKTXParms->pwrGainStepAC160)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LMTXPOWER, (size_t)(((size_t)&(pBACKTOBACKTXParms->lmtxPower)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BSSID, (size_t)(((size_t)&(pBACKTOBACKTXParms->bssid)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXSTATION, (size_t)(((size_t)&(pBACKTOBACKTXParms->txStation)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXSTATION, (size_t)(((size_t)&(pBACKTOBACKTXParms->rxStation)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_WIFISTANDARD, (size_t)(((size_t)&(pBACKTOBACKTXParms->wifiStandard)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NSS, (size_t)(((size_t)&(pBACKTOBACKTXParms->NSS)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RATE, (size_t)(((size_t)&(pBACKTOBACKTXParms->rate)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TABLELENGTH, (size_t)(((size_t)&(pBACKTOBACKTXParms->tableLength)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CYCLETIME, (size_t)(((size_t)&(pBACKTOBACKTXParms->cycleTime)) - (size_t)pBACKTOBACKTXParms), pParmsOffset);
    return((void*) pBACKTOBACKTXParms);
}

static BACKTOBACKTX_OP_FUNC BACKTOBACKTXOpFunc = NULL;

TLV2_API void registerBACKTOBACKTXHandler(BACKTOBACKTX_OP_FUNC fp)
{
    BACKTOBACKTXOpFunc = fp;
}

A_BOOL BACKTOBACKTXOp(void *pParms)
{
    CMD_BACKTOBACKTX_PARMS *pBACKTOBACKTXParms = (CMD_BACKTOBACKTX_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG
    int i; 	//for initializing array parameter
    i = 0;	//assign a number to avoid warning in case i is not used

    A_PRINTF("BACKTOBACKTXOp: phyId %u\n", pBACKTOBACKTXParms->phyId);
    A_PRINTF("BACKTOBACKTXOp: txMode %u\n", pBACKTOBACKTXParms->txMode);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 25 entries
    {
        A_PRINTF("BACKTOBACKTXOp: lmfreq %u\n", pBACKTOBACKTXParms->lmfreq[i]);
    }
    A_PRINTF("BACKTOBACKTXOp: freq2 %u\n", pBACKTOBACKTXParms->freq2);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 25 entries
    {
        A_PRINTF("BACKTOBACKTXOp: lmchainMask %u\n", pBACKTOBACKTXParms->lmchainMask[i]);
    }
    A_PRINTF("BACKTOBACKTXOp: wlanMode %u\n", pBACKTOBACKTXParms->wlanMode);
    A_PRINTF("BACKTOBACKTXOp: bandwidth %u\n", pBACKTOBACKTXParms->bandwidth);
    A_PRINTF("BACKTOBACKTXOp: gI %u\n", pBACKTOBACKTXParms->gI);
    A_PRINTF("BACKTOBACKTXOp: antenna %u\n", pBACKTOBACKTXParms->antenna);
    A_PRINTF("BACKTOBACKTXOp: enANI %u\n", pBACKTOBACKTXParms->enANI);
    A_PRINTF("BACKTOBACKTXOp: scramblerOff %u\n", pBACKTOBACKTXParms->scramblerOff);
    A_PRINTF("BACKTOBACKTXOp: aifsn %u\n", pBACKTOBACKTXParms->aifsn);
    A_PRINTF("BACKTOBACKTXOp: broadcast %u\n", pBACKTOBACKTXParms->broadcast);
    A_PRINTF("BACKTOBACKTXOp: agg %u\n", pBACKTOBACKTXParms->agg);
    A_PRINTF("BACKTOBACKTXOp: shortGuard %u\n", pBACKTOBACKTXParms->shortGuard);
    A_PRINTF("BACKTOBACKTXOp: dutyCycle %u\n", pBACKTOBACKTXParms->dutyCycle);
    A_PRINTF("BACKTOBACKTXOp: padding %u\n", pBACKTOBACKTXParms->padding);
    A_PRINTF("BACKTOBACKTXOp: flags 0x%x\n", pBACKTOBACKTXParms->flags);
    A_PRINTF("BACKTOBACKTXOp: ir %u\n", pBACKTOBACKTXParms->ir);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 25 entries
    {
        A_PRINTF("BACKTOBACKTXOp: lmpktSz %u\n", pBACKTOBACKTXParms->lmpktSz[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 40 entries
    {
        A_PRINTF("BACKTOBACKTXOp: dataPattern %u\n", pBACKTOBACKTXParms->dataPattern[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 25 entries
    {
        A_PRINTF("BACKTOBACKTXOp: lmtxNumPackets %u\n", pBACKTOBACKTXParms->lmtxNumPackets[i]);
    }
    A_PRINTF("BACKTOBACKTXOp: txPattern %u\n", pBACKTOBACKTXParms->txPattern);
    A_PRINTF("BACKTOBACKTXOp: nPattern %u\n", pBACKTOBACKTXParms->nPattern);
    A_PRINTF("BACKTOBACKTXOp: tpcm %u\n", pBACKTOBACKTXParms->tpcm);
    A_PRINTF("BACKTOBACKTXOp: gainIdx %u\n", pBACKTOBACKTXParms->gainIdx);
    A_PRINTF("BACKTOBACKTXOp: dacGain %d\n", pBACKTOBACKTXParms->dacGain);
    A_PRINTF("BACKTOBACKTXOp: dacGainEnd %d\n", pBACKTOBACKTXParms->dacGainEnd);
    A_PRINTF("BACKTOBACKTXOp: dacGainStep %d\n", pBACKTOBACKTXParms->dacGainStep);
    A_PRINTF("BACKTOBACKTXOp: paConfig %u\n", pBACKTOBACKTXParms->paConfig);
    A_PRINTF("BACKTOBACKTXOp: paConfigEnd %u\n", pBACKTOBACKTXParms->paConfigEnd);
    A_PRINTF("BACKTOBACKTXOp: paConfigStep %u\n", pBACKTOBACKTXParms->paConfigStep);
    A_PRINTF("BACKTOBACKTXOp: pad1 %u\n", pBACKTOBACKTXParms->pad1);
    A_PRINTF("BACKTOBACKTXOp: rateBitIndex %u\n", pBACKTOBACKTXParms->rateBitIndex);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 25 entries
    {
        A_PRINTF("BACKTOBACKTXOp: lmrateIndex %u\n", pBACKTOBACKTXParms->lmrateIndex[i]);
    }
    for (i = 0; i < 3 ; i++)
    {
        A_PRINTF("BACKTOBACKTXOp: rateMask 0x%x\n", pBACKTOBACKTXParms->rateMask[i]);
    }
    for (i = 0; i < 5 ; i++)
    {
        A_PRINTF("BACKTOBACKTXOp: rateMask11AC 0x%x\n", pBACKTOBACKTXParms->rateMask11AC[i]);
    }
    A_PRINTF("BACKTOBACKTXOp: rateMaskAC160 0x%x\n", pBACKTOBACKTXParms->rateMaskAC160);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 24 entries
    {
        A_PRINTF("BACKTOBACKTXOp: pwrGainStart 0x%x\n", pBACKTOBACKTXParms->pwrGainStart[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 24 entries
    {
        A_PRINTF("BACKTOBACKTXOp: pwrGainEnd 0x%x\n", pBACKTOBACKTXParms->pwrGainEnd[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 24 entries
    {
        A_PRINTF("BACKTOBACKTXOp: pwrGainStep 0x%x\n", pBACKTOBACKTXParms->pwrGainStep[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 40 entries
    {
        A_PRINTF("BACKTOBACKTXOp: pwrGainStart11AC 0x%x\n", pBACKTOBACKTXParms->pwrGainStart11AC[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 40 entries
    {
        A_PRINTF("BACKTOBACKTXOp: pwrGainEnd11AC 0x%x\n", pBACKTOBACKTXParms->pwrGainEnd11AC[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 40 entries
    {
        A_PRINTF("BACKTOBACKTXOp: pwrGainStep11AC 0x%x\n", pBACKTOBACKTXParms->pwrGainStep11AC[i]);
    }
    for (i = 0; i < 8 ; i++)
    {
        A_PRINTF("BACKTOBACKTXOp: pwrGainStartAC160 0x%x\n", pBACKTOBACKTXParms->pwrGainStartAC160[i]);
    }
    for (i = 0; i < 8 ; i++)
    {
        A_PRINTF("BACKTOBACKTXOp: pwrGainEndAC160 0x%x\n", pBACKTOBACKTXParms->pwrGainEndAC160[i]);
    }
    for (i = 0; i < 8 ; i++)
    {
        A_PRINTF("BACKTOBACKTXOp: pwrGainStepAC160 0x%x\n", pBACKTOBACKTXParms->pwrGainStepAC160[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 25 entries
    {
        A_PRINTF("BACKTOBACKTXOp: lmtxPower %u\n", pBACKTOBACKTXParms->lmtxPower[i]);
    }
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("BACKTOBACKTXOp: bssid 0x%x\n", pBACKTOBACKTXParms->bssid[i]);
    }
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("BACKTOBACKTXOp: txStation 0x%x\n", pBACKTOBACKTXParms->txStation[i]);
    }
    for (i = 0; i < 6 ; i++)
    {
        A_PRINTF("BACKTOBACKTXOp: rxStation 0x%x\n", pBACKTOBACKTXParms->rxStation[i]);
    }
    A_PRINTF("BACKTOBACKTXOp: wifiStandard %u\n", pBACKTOBACKTXParms->wifiStandard);
    A_PRINTF("BACKTOBACKTXOp: NSS %u\n", pBACKTOBACKTXParms->NSS);
    A_PRINTF("BACKTOBACKTXOp: rate %u\n", pBACKTOBACKTXParms->rate);
    A_PRINTF("BACKTOBACKTXOp: tableLength %u\n", pBACKTOBACKTXParms->tableLength);
    A_PRINTF("BACKTOBACKTXOp: cycleTime %u\n", pBACKTOBACKTXParms->cycleTime);
#endif //_DEBUG

    if (NULL != BACKTOBACKTXOpFunc) {
        (*BACKTOBACKTXOpFunc)(pBACKTOBACKTXParms);
    }
    return(TRUE);
}
