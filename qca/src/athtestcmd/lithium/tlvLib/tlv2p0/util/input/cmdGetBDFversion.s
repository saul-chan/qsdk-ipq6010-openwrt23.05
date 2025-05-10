#===============================================================================
# Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#===============================================================================

# cmd
CMD= GetBDFVersion

# cmd parm
PARM_START:
UINT8:bdfVerType:1:u:0
PARM_END:

# rsp
RSP= GetBDFVersionRsp

# rsp parm
PARM_START:
UINT8:status:1:u:0
DATA1024:ver_info:MAX_VER_INFO_SIZE:1024:u
PARM_END:
