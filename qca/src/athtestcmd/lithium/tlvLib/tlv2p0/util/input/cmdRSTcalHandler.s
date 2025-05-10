#===============================================================================
# Copyright (c) 2021 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#===============================================================================
# cmd 
CMD = RST

# cmd parm
PARM_START:
UINT32:calTxGain:1:u:0
UINT32:forcedRXIdx:1:u:0
INT32:dacGain:1:d:0
UINT8:rstDir:1:d:0
UINT8:phyId:1:u:0
UINT16:freq:1:u:2412
PARM_END:

# rsp
RSP= RSTRsp

# rsp parm
PARM_START:
INT32:rssi:1:d:0
PARM_END:
