#===============================================================================
# Copyright (c) 2021 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#===============================================================================
# cmd
CMD= CUST_RXGAINCALPROC

# cmd parm
PARM_START:
UINT8:phyId:1:u:0
UINT8:band:1:u:0
UINT16:freq:1:u:0
UINT8:chainIdx:1:u:0
UINT8:rxGainCalCtrl:1:u:0
INT8:rxNFCalPowerDBr:1:u:0
INT8:rxNFCalPowerDBm:1:u:0
INT8:minCcaThreshold:1:u:0
PARM_END:

# rsp
RSP= CUST_RXGAINCALPROCRSP

# rsp parm
PARM_START:
UINT8:phyId:1:u:0
UINT8:band:1:u:0
UINT16:freq:1:u:0
UINT8:chainIdx:1:u:0
UINT8:status:1:u:0
INT8:rxNFCalPowerDBr:1:u:0
INT8:rxNFCalPowerDBm:1:u:0
INT8:minCcaThreshold:1:u:0
PARM_END:
