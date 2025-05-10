/* Copyright (c) 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdAdcCaptureConfigHandler.s
#ifndef _CMDADCCAPTURECONFIGHANDLER_H_
#define _CMDADCCAPTURECONFIGHANDLER_H_

#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif //WIN32 || WIN64

typedef struct adccaptureconfig_parms {
    A_UINT8	phyId;
    A_UINT32	csr_capture_bank;
    A_UINT32	csr_tracer_capture;
    A_UINT32	csr_capture_trig_cond;
    A_UINT32	csr_capture_trig_mask_hi;
    A_UINT32	csr_capture_trig_pattern_hi;
    A_UINT32	csr_capture_trig_mask_lo;
    A_UINT32	csr_capture_trig_pattern_lo;
    A_UINT32	csr_capture_stopontrig;
    A_UINT32	csr_adc_iq_selective;
    A_UINT32	csr_adc_sideband;
    A_UINT32	csr_adc_chain_mask;
    A_UINT32	csr_capture_pretrig;
    A_UINT32	csr_phydbg_spares;
    A_UINT32	module_wsi_ctrl[10];
    A_UINT32	module_rfcntl_ctrl[10];
    A_UINT32	module_tpc_ctrl[10];
    A_UINT32	module_cal_ctrl[10];
    A_UINT32	module_impcorr_ctrl[10];
    A_UINT32	module_mpi_ctrl[10];
    A_UINT32	module_fft_ctrl[10];
    A_UINT32	module_txtd_ctrl[10];
    A_UINT32	module_pmi_ctrl[10];
    A_UINT32	module_rxtd_ctrl[10];
    A_UINT32	module_demfront_ctrl[10];
    A_UINT32	module_pcss_ctrl[10];
    A_UINT32	module_txfd_ctrl[10];
    A_UINT32	module_robe_ctrl[10];
    A_UINT32	module_phynoc_ctrl[10];
    A_UINT8	pad[3];
} __ATTRIB_PACK CMD_ADCCAPTURECONFIG_PARMS;

typedef void (*ADCCAPTURECONFIG_OP_FUNC)(void *pParms);

// Exposed functions

void* initADCCAPTURECONFIGOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict);
A_BOOL ADCCAPTURECONFIGOp(void *pParms);

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif //WIN32 || WIN64


#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif

#endif //_CMDADCCAPTURECONFIGHANDLER_H_
