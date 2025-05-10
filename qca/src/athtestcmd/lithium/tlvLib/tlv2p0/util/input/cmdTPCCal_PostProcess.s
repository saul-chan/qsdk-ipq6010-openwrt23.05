#===============================================================================
# Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#===============================================================================
#if !defined(MCHAIN_UTF)
#define MCHAIN_UTF           16
#endif //!defined(MCHAIN_UTF)

#define NUM_MEASURED_PWR        512
# cmd
CMD= TPCCal_PostProcess

# cmd parm
PARM_START:
UINT8:phyId:1:u:0
UINT8:chanIdx:1:u:0
UINT8:chainIdx:1:u:0
UINT8:valuetype:1:u:1
PARM_END:

# rsp
RSP= TPCCal_PostProcessRsp

PARM_START:
UINT8:phyId:1:u:0
UINT8:status:1:u:0
INT16:measuredPwr:NUM_MEASURED_PWR:512:d:24:16
UINT8:pdadc:MCHAIN_UTF:16:u
PARM_END:
