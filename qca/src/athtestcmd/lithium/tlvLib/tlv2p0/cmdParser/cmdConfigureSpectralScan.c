/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdConfigureSpectralScan.s
#include "tlv2Inc.h"
#include "cmdConfigureSpectralScan.h"

void* initCONFIGURESPECTRALSCANOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_CONFIGURESPECTRALSCAN_PARMS  *pCONFIGURESPECTRALSCANParms = (CMD_CONFIGURESPECTRALSCAN_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pCONFIGURESPECTRALSCANParms->freq = pParmDict[PARM_FREQ].v.valU16;
    pCONFIGURESPECTRALSCANParms->freq2 = pParmDict[PARM_FREQ2].v.valU16;
    pCONFIGURESPECTRALSCANParms->chainMask = pParmDict[PARM_CHAINMASK].v.valU16;
    pCONFIGURESPECTRALSCANParms->bandwidth = pParmDict[PARM_BANDWIDTH].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanCount = pParmDict[PARM_SCANCOUNT].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanPeriod = pParmDict[PARM_SCANPERIOD].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanPriority = pParmDict[PARM_SCANPRIORITY].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanFFTSize = pParmDict[PARM_SCANFFTSIZE].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanGCEna = pParmDict[PARM_SCANGCENA].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanRestartEna = pParmDict[PARM_SCANRESTARTENA].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanInitDelay = pParmDict[PARM_SCANINITDELAY].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanNBToneThr = pParmDict[PARM_SCANNBTONETHR].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanStrBinThr = pParmDict[PARM_SCANSTRBINTHR].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanWBRptMode = pParmDict[PARM_SCANWBRPTMODE].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanRssiRptMode = pParmDict[PARM_SCANRSSIRPTMODE].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanPwrFormat = pParmDict[PARM_SCANPWRFORMAT].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanRptMode = pParmDict[PARM_SCANRPTMODE].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanBinScale = pParmDict[PARM_SCANBINSCALE].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScandBmAdj = pParmDict[PARM_SCANDBMADJ].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanChnMask = pParmDict[PARM_SCANCHNMASK].v.valU8;
    pCONFIGURESPECTRALSCANParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pCONFIGURESPECTRALSCANParms->ScanRssiThr = pParmDict[PARM_SCANRSSITHR].v.valS32;
    pCONFIGURESPECTRALSCANParms->ScanNoiseFloorRef = pParmDict[PARM_SCANNOISEFLOORREF].v.valS32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_FREQ, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->freq)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ2, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->freq2)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINMASK, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->chainMask)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BANDWIDTH, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->bandwidth)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANCOUNT, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanCount)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANPERIOD, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanPeriod)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANPRIORITY, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanPriority)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANFFTSIZE, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanFFTSize)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANGCENA, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanGCEna)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANRESTARTENA, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanRestartEna)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANINITDELAY, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanInitDelay)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANNBTONETHR, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanNBToneThr)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANSTRBINTHR, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanStrBinThr)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANWBRPTMODE, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanWBRptMode)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANRSSIRPTMODE, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanRssiRptMode)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANPWRFORMAT, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanPwrFormat)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANRPTMODE, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanRptMode)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANBINSCALE, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanBinScale)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANDBMADJ, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScandBmAdj)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANCHNMASK, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanChnMask)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->phyId)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANRSSITHR, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanRssiThr)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SCANNOISEFLOORREF, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANParms->ScanNoiseFloorRef)) - (size_t)pCONFIGURESPECTRALSCANParms), pParmsOffset);
    return((void*) pCONFIGURESPECTRALSCANParms);
}

static CONFIGURESPECTRALSCAN_OP_FUNC CONFIGURESPECTRALSCANOpFunc = NULL;

TLV2_API void registerCONFIGURESPECTRALSCANHandler(CONFIGURESPECTRALSCAN_OP_FUNC fp)
{
    CONFIGURESPECTRALSCANOpFunc = fp;
}

A_BOOL CONFIGURESPECTRALSCANOp(void *pParms)
{
    CMD_CONFIGURESPECTRALSCAN_PARMS *pCONFIGURESPECTRALSCANParms = (CMD_CONFIGURESPECTRALSCAN_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("CONFIGURESPECTRALSCANOp: freq %u\n", pCONFIGURESPECTRALSCANParms->freq);
    A_PRINTF("CONFIGURESPECTRALSCANOp: freq2 %u\n", pCONFIGURESPECTRALSCANParms->freq2);
    A_PRINTF("CONFIGURESPECTRALSCANOp: chainMask %u\n", pCONFIGURESPECTRALSCANParms->chainMask);
    A_PRINTF("CONFIGURESPECTRALSCANOp: bandwidth %u\n", pCONFIGURESPECTRALSCANParms->bandwidth);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanCount %u\n", pCONFIGURESPECTRALSCANParms->ScanCount);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanPeriod %u\n", pCONFIGURESPECTRALSCANParms->ScanPeriod);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanPriority %u\n", pCONFIGURESPECTRALSCANParms->ScanPriority);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanFFTSize %u\n", pCONFIGURESPECTRALSCANParms->ScanFFTSize);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanGCEna %u\n", pCONFIGURESPECTRALSCANParms->ScanGCEna);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanRestartEna %u\n", pCONFIGURESPECTRALSCANParms->ScanRestartEna);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanInitDelay %u\n", pCONFIGURESPECTRALSCANParms->ScanInitDelay);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanNBToneThr %u\n", pCONFIGURESPECTRALSCANParms->ScanNBToneThr);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanStrBinThr %u\n", pCONFIGURESPECTRALSCANParms->ScanStrBinThr);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanWBRptMode %u\n", pCONFIGURESPECTRALSCANParms->ScanWBRptMode);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanRssiRptMode %u\n", pCONFIGURESPECTRALSCANParms->ScanRssiRptMode);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanPwrFormat %u\n", pCONFIGURESPECTRALSCANParms->ScanPwrFormat);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanRptMode %u\n", pCONFIGURESPECTRALSCANParms->ScanRptMode);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanBinScale %u\n", pCONFIGURESPECTRALSCANParms->ScanBinScale);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScandBmAdj %u\n", pCONFIGURESPECTRALSCANParms->ScandBmAdj);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanChnMask %u\n", pCONFIGURESPECTRALSCANParms->ScanChnMask);
    A_PRINTF("CONFIGURESPECTRALSCANOp: phyId %u\n", pCONFIGURESPECTRALSCANParms->phyId);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanRssiThr %d\n", pCONFIGURESPECTRALSCANParms->ScanRssiThr);
    A_PRINTF("CONFIGURESPECTRALSCANOp: ScanNoiseFloorRef %d\n", pCONFIGURESPECTRALSCANParms->ScanNoiseFloorRef);
#endif //_DEBUG

    if (NULL != CONFIGURESPECTRALSCANOpFunc) {
        (*CONFIGURESPECTRALSCANOpFunc)(pCONFIGURESPECTRALSCANParms);
    }
    return(TRUE);
}

void* initCONFIGURESPECTRALSCANRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_CONFIGURESPECTRALSCANRSP_PARMS  *pCONFIGURESPECTRALSCANRSPParms = (CMD_CONFIGURESPECTRALSCANRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pCONFIGURESPECTRALSCANRSPParms->fftreportsize = pParmDict[PARM_FFTREPORTSIZE].v.valU32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_FFTREPORTSIZE, (size_t)(((size_t)&(pCONFIGURESPECTRALSCANRSPParms->fftreportsize)) - (size_t)pCONFIGURESPECTRALSCANRSPParms), pParmsOffset);
    return((void*) pCONFIGURESPECTRALSCANRSPParms);
}

static CONFIGURESPECTRALSCANRSP_OP_FUNC CONFIGURESPECTRALSCANRSPOpFunc = NULL;

TLV2_API void registerCONFIGURESPECTRALSCANRSPHandler(CONFIGURESPECTRALSCANRSP_OP_FUNC fp)
{
    CONFIGURESPECTRALSCANRSPOpFunc = fp;
}

A_BOOL CONFIGURESPECTRALSCANRSPOp(void *pParms)
{
    CMD_CONFIGURESPECTRALSCANRSP_PARMS *pCONFIGURESPECTRALSCANRSPParms = (CMD_CONFIGURESPECTRALSCANRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("CONFIGURESPECTRALSCANRSPOp: fftreportsize %u\n", pCONFIGURESPECTRALSCANRSPParms->fftreportsize);
#endif //_DEBUG

    if (NULL != CONFIGURESPECTRALSCANRSPOpFunc) {
        (*CONFIGURESPECTRALSCANRSPOpFunc)(pCONFIGURESPECTRALSCANRSPParms);
    }
    return(TRUE);
}
