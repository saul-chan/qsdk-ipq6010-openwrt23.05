/*
 * Copyright (c) 2021 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/cmdDACPlyback.s
#include "tlv2Inc.h"
#include "cmdDACPlayback2.h"

void* initDACPLAYBACK2OpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_DACPLAYBACK2_PARMS  *pDACPLAYBACK2Parms = (CMD_DACPLAYBACK2_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pDACPLAYBACK2Parms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pDACPLAYBACK2Parms->playback_enable = pParmDict[PARM_PLAYBACK_ENABLE].v.valU32;
    pDACPLAYBACK2Parms->csr_playback_bank = pParmDict[PARM_CSR_PLAYBACK_BANK].v.valU32;
    pDACPLAYBACK2Parms->csr_playback_on_both_split_phys = pParmDict[PARM_CSR_PLAYBACK_ON_BOTH_SPLIT_PHYS].v.valU32;
    pDACPLAYBACK2Parms->csr_playback_fifo_th = pParmDict[PARM_CSR_PLAYBACK_FIFO_TH].v.valU32;
    pDACPLAYBACK2Parms->csr_playback_loop_count = pParmDict[PARM_CSR_PLAYBACK_LOOP_COUNT].v.valU32;
    pDACPLAYBACK2Parms->csr_playback_mem_depth = pParmDict[PARM_CSR_PLAYBACK_MEM_DEPTH].v.valU32;
    pDACPLAYBACK2Parms->csr_playback_count_a = pParmDict[PARM_CSR_PLAYBACK_COUNT_A].v.valU32;
    pDACPLAYBACK2Parms->isSysSimPlayback = pParmDict[PARM_ISSYSSIMPLAYBACK].v.valU32;
    pDACPLAYBACK2Parms->isPhydbgBankLoaded = pParmDict[PARM_ISPHYDBGBANKLOADED].v.valU32;
    pDACPLAYBACK2Parms->playbackChMask = pParmDict[PARM_PLAYBACKCHMASK].v.valU32;
    pDACPLAYBACK2Parms->txGainIndex = pParmDict[PARM_TXGAININDEX].v.valU32;
    pDACPLAYBACK2Parms->numberOfPlaybackPackets = pParmDict[PARM_NUMBEROFPLAYBACKPACKETS].v.valU32;
    memset(pDACPLAYBACK2Parms->playbackPacketDuration, 0, sizeof(pDACPLAYBACK2Parms->playbackPacketDuration));
    memset(pDACPLAYBACK2Parms->playbackPacketStartDelay, 0, sizeof(pDACPLAYBACK2Parms->playbackPacketStartDelay));
    memset(pDACPLAYBACK2Parms->data4k, 0, sizeof(pDACPLAYBACK2Parms->data4k));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pDACPLAYBACK2Parms->phyId)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PLAYBACK_ENABLE, (size_t)(((size_t)&(pDACPLAYBACK2Parms->playback_enable)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_PLAYBACK_BANK, (size_t)(((size_t)&(pDACPLAYBACK2Parms->csr_playback_bank)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_PLAYBACK_ON_BOTH_SPLIT_PHYS, (size_t)(((size_t)&(pDACPLAYBACK2Parms->csr_playback_on_both_split_phys)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_PLAYBACK_FIFO_TH, (size_t)(((size_t)&(pDACPLAYBACK2Parms->csr_playback_fifo_th)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_PLAYBACK_LOOP_COUNT, (size_t)(((size_t)&(pDACPLAYBACK2Parms->csr_playback_loop_count)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_PLAYBACK_MEM_DEPTH, (size_t)(((size_t)&(pDACPLAYBACK2Parms->csr_playback_mem_depth)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_PLAYBACK_COUNT_A, (size_t)(((size_t)&(pDACPLAYBACK2Parms->csr_playback_count_a)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ISSYSSIMPLAYBACK, (size_t)(((size_t)&(pDACPLAYBACK2Parms->isSysSimPlayback)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ISPHYDBGBANKLOADED, (size_t)(((size_t)&(pDACPLAYBACK2Parms->isPhydbgBankLoaded)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PLAYBACKCHMASK, (size_t)(((size_t)&(pDACPLAYBACK2Parms->playbackChMask)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXGAININDEX, (size_t)(((size_t)&(pDACPLAYBACK2Parms->txGainIndex)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMBEROFPLAYBACKPACKETS, (size_t)(((size_t)&(pDACPLAYBACK2Parms->numberOfPlaybackPackets)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PLAYBACKPACKETDURATION, (size_t)(((size_t)&(pDACPLAYBACK2Parms->playbackPacketDuration)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PLAYBACKPACKETSTARTDELAY, (size_t)(((size_t)&(pDACPLAYBACK2Parms->playbackPacketStartDelay)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DATA4K, (size_t)(((size_t)&(pDACPLAYBACK2Parms->data4k)) - (size_t)pDACPLAYBACK2Parms), pParmsOffset);
    return((void*) pDACPLAYBACK2Parms);
}

static DACPLAYBACK2_OP_FUNC DACPLAYBACK2OpFunc = NULL;

TLV2_API void registerDACPLAYBACK2Handler(DACPLAYBACK2_OP_FUNC fp)
{
    DACPLAYBACK2OpFunc = fp;
}

A_BOOL DACPLAYBACK2Op(void *pParms)
{
    CMD_DACPLAYBACK2_PARMS *pDACPLAYBACK2Parms = (CMD_DACPLAYBACK2_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("DACPLAYBACK2Op: phyId %u\n", pDACPLAYBACK2Parms->phyId);
    A_PRINTF("DACPLAYBACK2Op: playback_enable %u\n", pDACPLAYBACK2Parms->playback_enable);
    A_PRINTF("DACPLAYBACK2Op: csr_playback_bank %u\n", pDACPLAYBACK2Parms->csr_playback_bank);
    A_PRINTF("DACPLAYBACK2Op: csr_playback_on_both_split_phys %u\n", pDACPLAYBACK2Parms->csr_playback_on_both_split_phys);
    A_PRINTF("DACPLAYBACK2Op: csr_playback_fifo_th %u\n", pDACPLAYBACK2Parms->csr_playback_fifo_th);
    A_PRINTF("DACPLAYBACK2Op: csr_playback_loop_count %u\n", pDACPLAYBACK2Parms->csr_playback_loop_count);
    A_PRINTF("DACPLAYBACK2Op: csr_playback_mem_depth %u\n", pDACPLAYBACK2Parms->csr_playback_mem_depth);
    A_PRINTF("DACPLAYBACK2Op: csr_playback_count_a %u\n", pDACPLAYBACK2Parms->csr_playback_count_a);
    A_PRINTF("DACPLAYBACK2Op: isSysSimPlayback %u\n", pDACPLAYBACK2Parms->isSysSimPlayback);
    A_PRINTF("DACPLAYBACK2Op: isPhydbgBankLoaded %u\n", pDACPLAYBACK2Parms->isPhydbgBankLoaded);
    A_PRINTF("DACPLAYBACK2Op: playbackChMask 0x%x\n", pDACPLAYBACK2Parms->playbackChMask);
    A_PRINTF("DACPLAYBACK2Op: txGainIndex %u\n", pDACPLAYBACK2Parms->txGainIndex);
    A_PRINTF("DACPLAYBACK2Op: numberOfPlaybackPackets %u\n", pDACPLAYBACK2Parms->numberOfPlaybackPackets);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("DACPLAYBACK2Op: playbackPacketDuration %u\n", pDACPLAYBACK2Parms->playbackPacketDuration[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("DACPLAYBACK2Op: playbackPacketStartDelay %u\n", pDACPLAYBACK2Parms->playbackPacketStartDelay[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 4096 entries
    {
        A_PRINTF("DACPLAYBACK2Op: data4k 0x%x\n", pDACPLAYBACK2Parms->data4k[i]);
    }
#endif //_DEBUG

    if (NULL != DACPLAYBACK2OpFunc) {
        (*DACPLAYBACK2OpFunc)(pDACPLAYBACK2Parms);
    }
    return(TRUE);
}
