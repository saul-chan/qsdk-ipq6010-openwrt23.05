/*
 * Copyright (c) 2021 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from ./input/cmdAdcCaptureHandler.s
#include "tlv2Inc.h"
#include "cmdAdcCaptureConfigHandler.h"

void* initADCCAPTURECONFIGOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_ADCCAPTURECONFIG_PARMS  *pADCCAPTURECONFIGParms = (CMD_ADCCAPTURECONFIG_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pADCCAPTURECONFIGParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pADCCAPTURECONFIGParms->csr_capture_bank = pParmDict[PARM_CSR_CAPTURE_BANK].v.valU32;
    pADCCAPTURECONFIGParms->csr_tracer_capture = pParmDict[PARM_CSR_TRACER_CAPTURE].v.valU32;
    pADCCAPTURECONFIGParms->csr_capture_trig_cond = pParmDict[PARM_CSR_CAPTURE_TRIG_COND].v.valU32;
    pADCCAPTURECONFIGParms->csr_capture_trig_mask_hi = pParmDict[PARM_CSR_CAPTURE_TRIG_MASK_HI].v.valU32;
    pADCCAPTURECONFIGParms->csr_capture_trig_pattern_hi = pParmDict[PARM_CSR_CAPTURE_TRIG_PATTERN_HI].v.valU32;
    pADCCAPTURECONFIGParms->csr_capture_trig_mask_lo = pParmDict[PARM_CSR_CAPTURE_TRIG_MASK_LO].v.valU32;
    pADCCAPTURECONFIGParms->csr_capture_trig_pattern_lo = pParmDict[PARM_CSR_CAPTURE_TRIG_PATTERN_LO].v.valU32;
    pADCCAPTURECONFIGParms->csr_capture_stopontrig = pParmDict[PARM_CSR_CAPTURE_STOPONTRIG].v.valU32;
    pADCCAPTURECONFIGParms->csr_adc_iq_selective = pParmDict[PARM_CSR_ADC_IQ_SELECTIVE].v.valU32;
    pADCCAPTURECONFIGParms->csr_adc_sideband = pParmDict[PARM_CSR_ADC_SIDEBAND].v.valU32;
    pADCCAPTURECONFIGParms->csr_adc_chain_mask = pParmDict[PARM_CSR_ADC_CHAIN_MASK].v.valU32;
    pADCCAPTURECONFIGParms->csr_capture_pretrig = pParmDict[PARM_CSR_CAPTURE_PRETRIG].v.valU32;
    pADCCAPTURECONFIGParms->csr_phydbg_spares = pParmDict[PARM_CSR_PHYDBG_SPARES].v.valU32;
    memset(pADCCAPTURECONFIGParms->module_wsi_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_wsi_ctrl));
    memset(pADCCAPTURECONFIGParms->module_rfcntl_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_rfcntl_ctrl));
    memset(pADCCAPTURECONFIGParms->module_tpc_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_tpc_ctrl));
    memset(pADCCAPTURECONFIGParms->module_cal_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_cal_ctrl));
    memset(pADCCAPTURECONFIGParms->module_impcorr_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_impcorr_ctrl));
    memset(pADCCAPTURECONFIGParms->module_mpi_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_mpi_ctrl));
    memset(pADCCAPTURECONFIGParms->module_fft_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_fft_ctrl));
    memset(pADCCAPTURECONFIGParms->module_txtd_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_txtd_ctrl));
    memset(pADCCAPTURECONFIGParms->module_pmi_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_pmi_ctrl));
    memset(pADCCAPTURECONFIGParms->module_rxtd_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_rxtd_ctrl));
    memset(pADCCAPTURECONFIGParms->module_demfront_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_demfront_ctrl));
    memset(pADCCAPTURECONFIGParms->module_pcss_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_pcss_ctrl));
    memset(pADCCAPTURECONFIGParms->module_txfd_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_txfd_ctrl));
    memset(pADCCAPTURECONFIGParms->module_robe_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_robe_ctrl));
    memset(pADCCAPTURECONFIGParms->module_phynoc_ctrl, 0, sizeof(pADCCAPTURECONFIGParms->module_phynoc_ctrl));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->phyId)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_BANK, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->csr_capture_bank)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_TRACER_CAPTURE, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->csr_tracer_capture)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_TRIG_COND, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->csr_capture_trig_cond)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_TRIG_MASK_HI, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->csr_capture_trig_mask_hi)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_TRIG_PATTERN_HI, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->csr_capture_trig_pattern_hi)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_TRIG_MASK_LO, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->csr_capture_trig_mask_lo)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_TRIG_PATTERN_LO, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->csr_capture_trig_pattern_lo)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_STOPONTRIG, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->csr_capture_stopontrig)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_ADC_IQ_SELECTIVE, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->csr_adc_iq_selective)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_ADC_SIDEBAND, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->csr_adc_sideband)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_ADC_CHAIN_MASK, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->csr_adc_chain_mask)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_PRETRIG, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->csr_capture_pretrig)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_PHYDBG_SPARES, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->csr_phydbg_spares)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_WSI_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_wsi_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_RFCNTL_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_rfcntl_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_TPC_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_tpc_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_CAL_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_cal_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_IMPCORR_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_impcorr_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_MPI_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_mpi_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_FFT_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_fft_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_TXTD_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_txtd_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_PMI_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_pmi_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_RXTD_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_rxtd_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_DEMFRONT_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_demfront_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_PCSS_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_pcss_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_TXFD_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_txfd_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_ROBE_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_robe_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_PHYNOC_CTRL, (size_t)(((size_t)&(pADCCAPTURECONFIGParms->module_phynoc_ctrl)) - (size_t)pADCCAPTURECONFIGParms), pParmsOffset);
    return((void*) pADCCAPTURECONFIGParms);
}

static ADCCAPTURECONFIG_OP_FUNC ADCCAPTURECONFIGOpFunc = NULL;

TLV2_API void registerADCCAPTURECONFIGHandler(ADCCAPTURECONFIG_OP_FUNC fp)
{
    ADCCAPTURECONFIGOpFunc = fp;
}

A_BOOL ADCCAPTURECONFIGOp(void *pParms)
{
    CMD_ADCCAPTURECONFIG_PARMS *pADCCAPTURECONFIGParms = (CMD_ADCCAPTURECONFIG_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("ADCCAPTURECONFIGOp: phyId %u\n", pADCCAPTURECONFIGParms->phyId);
    A_PRINTF("ADCCAPTURECONFIGOp: csr_capture_bank %u\n", pADCCAPTURECONFIGParms->csr_capture_bank);
    A_PRINTF("ADCCAPTURECONFIGOp: csr_tracer_capture %u\n", pADCCAPTURECONFIGParms->csr_tracer_capture);
    A_PRINTF("ADCCAPTURECONFIGOp: csr_capture_trig_cond %u\n", pADCCAPTURECONFIGParms->csr_capture_trig_cond);
    A_PRINTF("ADCCAPTURECONFIGOp: csr_capture_trig_mask_hi %u\n", pADCCAPTURECONFIGParms->csr_capture_trig_mask_hi);
    A_PRINTF("ADCCAPTURECONFIGOp: csr_capture_trig_pattern_hi %u\n", pADCCAPTURECONFIGParms->csr_capture_trig_pattern_hi);
    A_PRINTF("ADCCAPTURECONFIGOp: csr_capture_trig_mask_lo %u\n", pADCCAPTURECONFIGParms->csr_capture_trig_mask_lo);
    A_PRINTF("ADCCAPTURECONFIGOp: csr_capture_trig_pattern_lo %u\n", pADCCAPTURECONFIGParms->csr_capture_trig_pattern_lo);
    A_PRINTF("ADCCAPTURECONFIGOp: csr_capture_stopontrig %u\n", pADCCAPTURECONFIGParms->csr_capture_stopontrig);
    A_PRINTF("ADCCAPTURECONFIGOp: csr_adc_iq_selective %u\n", pADCCAPTURECONFIGParms->csr_adc_iq_selective);
    A_PRINTF("ADCCAPTURECONFIGOp: csr_adc_sideband %u\n", pADCCAPTURECONFIGParms->csr_adc_sideband);
    A_PRINTF("ADCCAPTURECONFIGOp: csr_adc_chain_mask 0x%x\n", pADCCAPTURECONFIGParms->csr_adc_chain_mask);
    A_PRINTF("ADCCAPTURECONFIGOp: csr_capture_pretrig %u\n", pADCCAPTURECONFIGParms->csr_capture_pretrig);
    A_PRINTF("ADCCAPTURECONFIGOp: csr_phydbg_spares %u\n", pADCCAPTURECONFIGParms->csr_phydbg_spares);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_wsi_ctrl %u\n", pADCCAPTURECONFIGParms->module_wsi_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_rfcntl_ctrl %u\n", pADCCAPTURECONFIGParms->module_rfcntl_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_tpc_ctrl %u\n", pADCCAPTURECONFIGParms->module_tpc_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_cal_ctrl %u\n", pADCCAPTURECONFIGParms->module_cal_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_impcorr_ctrl %u\n", pADCCAPTURECONFIGParms->module_impcorr_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_mpi_ctrl %u\n", pADCCAPTURECONFIGParms->module_mpi_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_fft_ctrl %u\n", pADCCAPTURECONFIGParms->module_fft_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_txtd_ctrl %u\n", pADCCAPTURECONFIGParms->module_txtd_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_pmi_ctrl %u\n", pADCCAPTURECONFIGParms->module_pmi_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_rxtd_ctrl %u\n", pADCCAPTURECONFIGParms->module_rxtd_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_demfront_ctrl %u\n", pADCCAPTURECONFIGParms->module_demfront_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_pcss_ctrl %u\n", pADCCAPTURECONFIGParms->module_pcss_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_txfd_ctrl %u\n", pADCCAPTURECONFIGParms->module_txfd_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_robe_ctrl %u\n", pADCCAPTURECONFIGParms->module_robe_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("ADCCAPTURECONFIGOp: module_phynoc_ctrl %u\n", pADCCAPTURECONFIGParms->module_phynoc_ctrl[i]);
    }
#endif //_DEBUG

    if (NULL != ADCCAPTURECONFIGOpFunc) {
        (*ADCCAPTURECONFIGOpFunc)(pADCCAPTURECONFIGParms);
    }
    return(TRUE);
}
