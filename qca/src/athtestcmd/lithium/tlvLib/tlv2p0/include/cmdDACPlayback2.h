/* Copyright (c) 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdDACPlayback2.s
#ifndef _CMDDACPLAYBACK2_H_
#define _CMDDACPLAYBACK2_H_

#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif //WIN32 || WIN64

typedef struct dacplayback2_parms {
    A_UINT8	phyId;
    A_UINT32	playback_enable;
    A_UINT32	csr_playback_bank;
    A_UINT32	csr_playback_on_both_split_phys;
    A_UINT32	csr_playback_fifo_th;
    A_UINT32	csr_playback_loop_count;
    A_UINT32	csr_playback_mem_depth;
    A_UINT32	csr_playback_count_a;
    A_UINT32	isSysSimPlayback;
    A_UINT32	isPhydbgBankLoaded;
    A_UINT32	playbackChMask;
    A_UINT32	txGainIndex;
    A_UINT32	numberOfPlaybackPackets;
    A_UINT32	playbackPacketDuration[10];
    A_UINT32	playbackPacketStartDelay[10];
    A_UINT8	data4k[4096];
    A_UINT8	pad[3];
} __ATTRIB_PACK CMD_DACPLAYBACK2_PARMS;

typedef void (*DACPLAYBACK2_OP_FUNC)(void *pParms);

// Exposed functions

void* initDACPLAYBACK2OpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL DACPLAYBACK2Op(void *pParms);

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif //WIN32 || WIN64


#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif //_CMDDACPLAYBACK2_H_
