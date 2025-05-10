#===============================================================================
# Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#===============================================================================
#if !defined(ALL_PAPRD_GLUT_CHAIN_VAL)
#define ALL_PAPRD_GLUT_CHAIN_VAL           32
#endif //!defined(ALL_PAPRD_GLUT4_CHAIN_VAL)

# cmd
CMD= GetEDPDCalInfo
 
# cmd parm
PARM_START:
UINT8:phyId:1:u:0
UINT16:chainMask:1:x:0x3
PARM_END:
 
# rsp
RSP= GetEDPDCalInfoRsp
 
# rsp parm
PARM_START:
UINT8:status:1:u:0
UINT8:phyId:1:u:0
UINT16:chainMask:1:x:0x3
INT8:nmse:ALL_PAPRD_GLUT_CHAIN_VAL:32:u:0
UINT16:signalQuality:ALL_PAPRD_GLUT_CHAIN_VAL:32:u:0
UINT8:maxBin:ALL_PAPRD_GLUT_CHAIN_VAL:32:u:0
UINT16:diffn0:ALL_PAPRD_GLUT_CHAIN_VAL:32:u:0
UINT16:diffn1:ALL_PAPRD_GLUT_CHAIN_VAL:32:u:0
UINT16:diffn2:ALL_PAPRD_GLUT_CHAIN_VAL:32:u:0
UINT16:diffn3:ALL_PAPRD_GLUT_CHAIN_VAL:32:u:0
UINT8:sqIdx:ALL_PAPRD_GLUT_CHAIN_VAL:32:u:0
UINT8:rxGain:ALL_PAPRD_GLUT_CHAIN_VAL:32:u:0
INT16:dcI:ALL_PAPRD_GLUT_CHAIN_VAL:32:u:0
INT16:dcQ:ALL_PAPRD_GLUT_CHAIN_VAL:32:u:0
PARM_END:

