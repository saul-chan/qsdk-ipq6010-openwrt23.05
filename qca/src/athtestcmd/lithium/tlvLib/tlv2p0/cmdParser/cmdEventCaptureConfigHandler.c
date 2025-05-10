/* Copyright (c) 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdEventCaptureConfigHandler.s
#include "tlv2Inc.h"
#include "cmdEventCaptureConfigHandler.h"

void* initEVENTCAPTURECONFIGOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_EVENTCAPTURECONFIG_PARMS  *pEVENTCAPTURECONFIGParms = (CMD_EVENTCAPTURECONFIG_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pEVENTCAPTURECONFIGParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pEVENTCAPTURECONFIGParms->clear_bank = pParmDict[PARM_CLEAR_BANK].v.valU32;
    pEVENTCAPTURECONFIGParms->csr_tracer_capture = pParmDict[PARM_CSR_TRACER_CAPTURE].v.valU32;
    pEVENTCAPTURECONFIGParms->csr_capture_events_2banks = pParmDict[PARM_CSR_CAPTURE_EVENTS_2BANKS].v.valU32;
    pEVENTCAPTURECONFIGParms->csr_capture_bank = pParmDict[PARM_CSR_CAPTURE_BANK].v.valU32;
    pEVENTCAPTURECONFIGParms->csr_capture_trig_cond = pParmDict[PARM_CSR_CAPTURE_TRIG_COND].v.valU32;
    pEVENTCAPTURECONFIGParms->csr_capture_trig_mask_hi = pParmDict[PARM_CSR_CAPTURE_TRIG_MASK_HI].v.valU32;
    pEVENTCAPTURECONFIGParms->csr_capture_trig_pattern_hi = pParmDict[PARM_CSR_CAPTURE_TRIG_PATTERN_HI].v.valU32;
    pEVENTCAPTURECONFIGParms->csr_capture_trig_mask_lo = pParmDict[PARM_CSR_CAPTURE_TRIG_MASK_LO].v.valU32;
    pEVENTCAPTURECONFIGParms->csr_capture_trig_pattern_lo = pParmDict[PARM_CSR_CAPTURE_TRIG_PATTERN_LO].v.valU32;
    pEVENTCAPTURECONFIGParms->csr_capture_event_mask = pParmDict[PARM_CSR_CAPTURE_EVENT_MASK].v.valU32;
    pEVENTCAPTURECONFIGParms->csr_capture_bypass_ts_reordering = pParmDict[PARM_CSR_CAPTURE_BYPASS_TS_REORDERING].v.valU32;
    pEVENTCAPTURECONFIGParms->csr_capture_stopontrig = pParmDict[PARM_CSR_CAPTURE_STOPONTRIG].v.valU32;
    pEVENTCAPTURECONFIGParms->csr_capture_pretrig = pParmDict[PARM_CSR_CAPTURE_PRETRIG].v.valU32;
    memset(pEVENTCAPTURECONFIGParms->module_wsi_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_wsi_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_rfcntl_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_rfcntl_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_tpc_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_tpc_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_cal_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_cal_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_impcorr_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_impcorr_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_mpi_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_mpi_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_fft_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_fft_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_txtd_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_txtd_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_pmi_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_pmi_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_rxtd_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_rxtd_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_demfront_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_demfront_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_pcss_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_pcss_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_txfd_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_txfd_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_robe_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_robe_ctrl));
    memset(pEVENTCAPTURECONFIGParms->module_phynoc_ctrl, 0, sizeof(pEVENTCAPTURECONFIGParms->module_phynoc_ctrl));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->phyId)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CLEAR_BANK, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->clear_bank)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_TRACER_CAPTURE, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->csr_tracer_capture)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_EVENTS_2BANKS, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->csr_capture_events_2banks)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_BANK, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->csr_capture_bank)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_TRIG_COND, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->csr_capture_trig_cond)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_TRIG_MASK_HI, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->csr_capture_trig_mask_hi)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_TRIG_PATTERN_HI, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->csr_capture_trig_pattern_hi)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_TRIG_MASK_LO, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->csr_capture_trig_mask_lo)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_TRIG_PATTERN_LO, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->csr_capture_trig_pattern_lo)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_EVENT_MASK, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->csr_capture_event_mask)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_BYPASS_TS_REORDERING, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->csr_capture_bypass_ts_reordering)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_STOPONTRIG, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->csr_capture_stopontrig)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CSR_CAPTURE_PRETRIG, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->csr_capture_pretrig)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_WSI_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_wsi_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_RFCNTL_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_rfcntl_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_TPC_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_tpc_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_CAL_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_cal_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_IMPCORR_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_impcorr_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_MPI_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_mpi_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_FFT_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_fft_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_TXTD_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_txtd_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_PMI_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_pmi_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_RXTD_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_rxtd_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_DEMFRONT_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_demfront_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_PCSS_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_pcss_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_TXFD_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_txfd_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_ROBE_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_robe_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MODULE_PHYNOC_CTRL, (size_t)(((size_t)&(pEVENTCAPTURECONFIGParms->module_phynoc_ctrl)) - (size_t)pEVENTCAPTURECONFIGParms), pParmsOffset);
    return((void*) pEVENTCAPTURECONFIGParms);
}

static EVENTCAPTURECONFIG_OP_FUNC EVENTCAPTURECONFIGOpFunc = NULL;

TLV2_API void registerEVENTCAPTURECONFIGHandler(EVENTCAPTURECONFIG_OP_FUNC fp)
{
    EVENTCAPTURECONFIGOpFunc = fp;
}

A_BOOL EVENTCAPTURECONFIGOp(void *pParms)
{
    CMD_EVENTCAPTURECONFIG_PARMS *pEVENTCAPTURECONFIGParms = (CMD_EVENTCAPTURECONFIG_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("EVENTCAPTURECONFIGOp: phyId %u\n", pEVENTCAPTURECONFIGParms->phyId);
    A_PRINTF("EVENTCAPTURECONFIGOp: clear_bank %u\n", pEVENTCAPTURECONFIGParms->clear_bank);
    A_PRINTF("EVENTCAPTURECONFIGOp: csr_tracer_capture %u\n", pEVENTCAPTURECONFIGParms->csr_tracer_capture);
    A_PRINTF("EVENTCAPTURECONFIGOp: csr_capture_events_2banks %u\n", pEVENTCAPTURECONFIGParms->csr_capture_events_2banks);
    A_PRINTF("EVENTCAPTURECONFIGOp: csr_capture_bank %u\n", pEVENTCAPTURECONFIGParms->csr_capture_bank);
    A_PRINTF("EVENTCAPTURECONFIGOp: csr_capture_trig_cond %u\n", pEVENTCAPTURECONFIGParms->csr_capture_trig_cond);
    A_PRINTF("EVENTCAPTURECONFIGOp: csr_capture_trig_mask_hi %u\n", pEVENTCAPTURECONFIGParms->csr_capture_trig_mask_hi);
    A_PRINTF("EVENTCAPTURECONFIGOp: csr_capture_trig_pattern_hi %u\n", pEVENTCAPTURECONFIGParms->csr_capture_trig_pattern_hi);
    A_PRINTF("EVENTCAPTURECONFIGOp: csr_capture_trig_mask_lo %u\n", pEVENTCAPTURECONFIGParms->csr_capture_trig_mask_lo);
    A_PRINTF("EVENTCAPTURECONFIGOp: csr_capture_trig_pattern_lo %u\n", pEVENTCAPTURECONFIGParms->csr_capture_trig_pattern_lo);
    A_PRINTF("EVENTCAPTURECONFIGOp: csr_capture_event_mask %u\n", pEVENTCAPTURECONFIGParms->csr_capture_event_mask);
    A_PRINTF("EVENTCAPTURECONFIGOp: csr_capture_bypass_ts_reordering %u\n", pEVENTCAPTURECONFIGParms->csr_capture_bypass_ts_reordering);
    A_PRINTF("EVENTCAPTURECONFIGOp: csr_capture_stopontrig %u\n", pEVENTCAPTURECONFIGParms->csr_capture_stopontrig);
    A_PRINTF("EVENTCAPTURECONFIGOp: csr_capture_pretrig %u\n", pEVENTCAPTURECONFIGParms->csr_capture_pretrig);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_wsi_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_wsi_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_rfcntl_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_rfcntl_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_tpc_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_tpc_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_cal_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_cal_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_impcorr_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_impcorr_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_mpi_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_mpi_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_fft_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_fft_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_txtd_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_txtd_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_pmi_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_pmi_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_rxtd_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_rxtd_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_demfront_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_demfront_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_pcss_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_pcss_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_txfd_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_txfd_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_robe_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_robe_ctrl[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 10 entries
    {
        A_PRINTF("EVENTCAPTURECONFIGOp: module_phynoc_ctrl %u\n", pEVENTCAPTURECONFIGParms->module_phynoc_ctrl[i]);
    }
#endif //_DEBUG

    if (NULL != EVENTCAPTURECONFIGOpFunc) {
        (*EVENTCAPTURECONFIGOpFunc)(pEVENTCAPTURECONFIGParms);
    }
    return(TRUE);
}
