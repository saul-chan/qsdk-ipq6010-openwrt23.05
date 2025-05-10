#===============================================================================
# Copyright (c) 2019,2021 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#===============================================================================
# cmd
CMD= AdcCaptureConfig

# cmd parm
PARM_START:
UINT8:phyId:1:u:0
UINT32:csr_capture_bank:1:u
UINT32:csr_tracer_capture:1:u
UINT32:csr_capture_trig_cond:1:u
UINT32:csr_capture_trig_mask_hi:1:u
UINT32:csr_capture_trig_pattern_hi:1:u
UINT32:csr_capture_trig_mask_lo:1:u
UINT32:csr_capture_trig_pattern_lo:1:u
UINT32:csr_capture_stopontrig:1:u
UINT32:csr_adc_iq_selective:1:u
UINT32:csr_adc_sideband:1:u
UINT32:csr_adc_chain_mask:1:x:0x1    #Bitmap of chain to capture from
UINT32:csr_capture_pretrig:1:u
UINT32:csr_phydbg_spares:1:u
UINT32:module_wsi_ctrl:10:u
UINT32:module_rfcntl_ctrl:10:u
UINT32:module_tpc_ctrl:10:u
UINT32:module_cal_ctrl:10:u
UINT32:module_impcorr_ctrl:10:u
UINT32:module_mpi_ctrl:10:u
UINT32:module_fft_ctrl:10:u
UINT32:module_txtd_ctrl:10:u
UINT32:module_pmi_ctrl:10:u
UINT32:module_rxtd_ctrl:10:u
UINT32:module_demfront_ctrl:10:u
UINT32:module_pcss_ctrl:10:u
UINT32:module_txfd_ctrl:10:u
UINT32:module_robe_ctrl:10:u
UINT32:module_phynoc_ctrl:10:u
PARM_END:

