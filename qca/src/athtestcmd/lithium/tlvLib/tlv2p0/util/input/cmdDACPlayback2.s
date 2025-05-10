#===============================================================================
# Copyright (c) 2019,2021 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#===============================================================================
# cmd
CMD= DACPlayback2

# cmd parm
PARM_START:
UINT8:phyId:1:u:0
UINT32:playback_enable:1:u
UINT32:csr_playback_bank:1:u
UINT32:csr_playback_on_both_split_phys:1:u
UINT32:csr_playback_fifo_th:1:u
UINT32:csr_playback_loop_count:1:u
UINT32:csr_playback_mem_depth:1:u
UINT32:csr_playback_count_a:1:u
UINT32:isSysSimPlayback:1:u
UINT32:isPhydbgBankLoaded:1:u
UINT32:playbackChMask:1:x:0x1
UINT32:txGainIndex:1:u:15
UINT32:numberOfPlaybackPackets:1:u
UINT32:playbackPacketDuration:10:u
UINT32:playbackPacketStartDelay:10:u
DATA4096:data4k:4096:x
PARM_END:

