#===============================================================================
# Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#===============================================================================

# cmd
CMD= SendTPC_TxMeas

# cmd parm
PARM_START:
UINT8:phyId:1:u:0
UINT8:chanIdx:1:u:0
UINT8:chainIdx:1:u:0
UINT8:loopCnt:1:u:1
UINT8:gainIdx:1:u:15
UINT8:tpcpdadc:1:u:0
INT16:tpcMeasuredPwr:1:d:0
PARM_END:

