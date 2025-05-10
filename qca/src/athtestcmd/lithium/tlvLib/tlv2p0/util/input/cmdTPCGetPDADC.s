#===============================================================================
# Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#===============================================================================

# cmd
CMD= GetTPC_PDADC

# cmd parm
PARM_START:
UINT8:phyId:1:u:0
UINT8:chainIdx:1:u:0
PARM_END:

# rsp
RSP= GetTPC_PDADCRsp
PARM_START:
UINT8:phyId:1:u:0
UINT8:status:1:u:0
UINT8:tpcpdadc:1:u:0
PARM_END:
