#===============================================================================
# Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#===============================================================================

# cmd
CMD= ChangeTPC_ForcedGain

# cmd parm
PARM_START:
UINT8:phyId:1:u:0
UINT8:gainIdx:1:u:15
INT8:dacGain:1:d:0
UINT8:alutOffset:1:u:0
PARM_END:

