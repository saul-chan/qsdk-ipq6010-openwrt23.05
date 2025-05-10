/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input\cmdCombCalGroupHandler.s
#include "tlv2Inc.h"
#include "cmdCombCalGroupHandler.h"

void* initCOMBCALGROUPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_COMBCALGROUP_PARMS  *pCOMBCALGROUPParms = (CMD_COMBCALGROUP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pCOMBCALGROUPParms->cmdIdCombCalGroup = pParmDict[PARM_CMDIDCOMBCALGROUP].v.valU8;
    pCOMBCALGROUPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pCOMBCALGROUPParms->bandCode = pParmDict[PARM_BANDCODE].v.valU8;
    pCOMBCALGROUPParms->bwCode = pParmDict[PARM_BWCODE].v.valU8;
    pCOMBCALGROUPParms->freq = pParmDict[PARM_FREQ].v.valU16;
    pCOMBCALGROUPParms->dyn_bw = pParmDict[PARM_DYN_BW].v.valU8;
    pCOMBCALGROUPParms->chain = pParmDict[PARM_CHAIN].v.valU8;
    pCOMBCALGROUPParms->num_plybck_tone = pParmDict[PARM_NUM_PLYBCK_TONE].v.valU8;
    pCOMBCALGROUPParms->lenPlybckTones = pParmDict[PARM_LENPLYBCKTONES].v.valU8;
    pCOMBCALGROUPParms->plybckToneIdx = pParmDict[PARM_PLYBCKTONEIDX].v.valU8;
    pCOMBCALGROUPParms->playBackLen = pParmDict[PARM_PLAYBACKLEN].v.valU8;
    memset(pCOMBCALGROUPParms->plybckTones, 0, sizeof(pCOMBCALGROUPParms->plybckTones));
    memset(pCOMBCALGROUPParms->calToneList, 0, sizeof(pCOMBCALGROUPParms->calToneList));
    pCOMBCALGROUPParms->initCal = pParmDict[PARM_INITCAL].v.valU8;
    pCOMBCALGROUPParms->iteration = pParmDict[PARM_ITERATION].v.valU8;
    pCOMBCALGROUPParms->clLoc = pParmDict[PARM_CLLOC].v.valU8;
    pCOMBCALGROUPParms->isRunCalTx = pParmDict[PARM_ISRUNCALTX].v.valU8;
    pCOMBCALGROUPParms->combine = pParmDict[PARM_COMBINE].v.valU8;
    pCOMBCALGROUPParms->homeCh = pParmDict[PARM_HOMECH].v.valU8;
    pCOMBCALGROUPParms->synth = pParmDict[PARM_SYNTH].v.valU8;
    pCOMBCALGROUPParms->verbose = pParmDict[PARM_VERBOSE].v.valU8;
    pCOMBCALGROUPParms->max_offset = pParmDict[PARM_MAX_OFFSET].v.valU8;
    pCOMBCALGROUPParms->DBS = pParmDict[PARM_DBS].v.valU8;
    pCOMBCALGROUPParms->iqIter = pParmDict[PARM_IQITER].v.valU8;
    pCOMBCALGROUPParms->clIter = pParmDict[PARM_CLITER].v.valU8;
    pCOMBCALGROUPParms->numTxGains = pParmDict[PARM_NUMTXGAINS].v.valU8;
    pCOMBCALGROUPParms->dbStep = pParmDict[PARM_DBSTEP].v.valU8;
    pCOMBCALGROUPParms->calMode = pParmDict[PARM_CALMODE].v.valU8;
    pCOMBCALGROUPParms->havePrevResults = pParmDict[PARM_HAVEPREVRESULTS].v.valU8;
    memset(pCOMBCALGROUPParms->prevResults_magTx, 0, sizeof(pCOMBCALGROUPParms->prevResults_magTx));
    memset(pCOMBCALGROUPParms->prevResults_magRx, 0, sizeof(pCOMBCALGROUPParms->prevResults_magRx));
    memset(pCOMBCALGROUPParms->prevResults_phaseTx, 0, sizeof(pCOMBCALGROUPParms->prevResults_phaseTx));
    memset(pCOMBCALGROUPParms->prevResults_phaseRx, 0, sizeof(pCOMBCALGROUPParms->prevResults_phaseRx));
    memset(pCOMBCALGROUPParms->clOffset, 0, sizeof(pCOMBCALGROUPParms->clOffset));
    memset(pCOMBCALGROUPParms->clCorr_real, 0, sizeof(pCOMBCALGROUPParms->clCorr_real));
    memset(pCOMBCALGROUPParms->clCorr_imag, 0, sizeof(pCOMBCALGROUPParms->clCorr_imag));
    memset(pCOMBCALGROUPParms->txCorr_real, 0, sizeof(pCOMBCALGROUPParms->txCorr_real));
    memset(pCOMBCALGROUPParms->txCorr_imag, 0, sizeof(pCOMBCALGROUPParms->txCorr_imag));
    memset(pCOMBCALGROUPParms->rxCorr_real, 0, sizeof(pCOMBCALGROUPParms->rxCorr_real));
    memset(pCOMBCALGROUPParms->rxCorr_imag, 0, sizeof(pCOMBCALGROUPParms->rxCorr_imag));

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_CMDIDCOMBCALGROUP, (size_t)(((size_t)&(pCOMBCALGROUPParms->cmdIdCombCalGroup)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pCOMBCALGROUPParms->phyId)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BANDCODE, (size_t)(((size_t)&(pCOMBCALGROUPParms->bandCode)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BWCODE, (size_t)(((size_t)&(pCOMBCALGROUPParms->bwCode)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ, (size_t)(((size_t)&(pCOMBCALGROUPParms->freq)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DYN_BW, (size_t)(((size_t)&(pCOMBCALGROUPParms->dyn_bw)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAIN, (size_t)(((size_t)&(pCOMBCALGROUPParms->chain)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUM_PLYBCK_TONE, (size_t)(((size_t)&(pCOMBCALGROUPParms->num_plybck_tone)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LENPLYBCKTONES, (size_t)(((size_t)&(pCOMBCALGROUPParms->lenPlybckTones)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PLYBCKTONEIDX, (size_t)(((size_t)&(pCOMBCALGROUPParms->plybckToneIdx)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PLAYBACKLEN, (size_t)(((size_t)&(pCOMBCALGROUPParms->playBackLen)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PLYBCKTONES, (size_t)(((size_t)&(pCOMBCALGROUPParms->plybckTones)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALTONELIST, (size_t)(((size_t)&(pCOMBCALGROUPParms->calToneList)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_INITCAL, (size_t)(((size_t)&(pCOMBCALGROUPParms->initCal)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ITERATION, (size_t)(((size_t)&(pCOMBCALGROUPParms->iteration)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CLLOC, (size_t)(((size_t)&(pCOMBCALGROUPParms->clLoc)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_ISRUNCALTX, (size_t)(((size_t)&(pCOMBCALGROUPParms->isRunCalTx)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_COMBINE, (size_t)(((size_t)&(pCOMBCALGROUPParms->combine)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_HOMECH, (size_t)(((size_t)&(pCOMBCALGROUPParms->homeCh)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_SYNTH, (size_t)(((size_t)&(pCOMBCALGROUPParms->synth)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_VERBOSE, (size_t)(((size_t)&(pCOMBCALGROUPParms->verbose)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_MAX_OFFSET, (size_t)(((size_t)&(pCOMBCALGROUPParms->max_offset)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DBS, (size_t)(((size_t)&(pCOMBCALGROUPParms->DBS)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_IQITER, (size_t)(((size_t)&(pCOMBCALGROUPParms->iqIter)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CLITER, (size_t)(((size_t)&(pCOMBCALGROUPParms->clIter)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMTXGAINS, (size_t)(((size_t)&(pCOMBCALGROUPParms->numTxGains)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DBSTEP, (size_t)(((size_t)&(pCOMBCALGROUPParms->dbStep)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CALMODE, (size_t)(((size_t)&(pCOMBCALGROUPParms->calMode)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_HAVEPREVRESULTS, (size_t)(((size_t)&(pCOMBCALGROUPParms->havePrevResults)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PREVRESULTS_MAGTX, (size_t)(((size_t)&(pCOMBCALGROUPParms->prevResults_magTx)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PREVRESULTS_MAGRX, (size_t)(((size_t)&(pCOMBCALGROUPParms->prevResults_magRx)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PREVRESULTS_PHASETX, (size_t)(((size_t)&(pCOMBCALGROUPParms->prevResults_phaseTx)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PREVRESULTS_PHASERX, (size_t)(((size_t)&(pCOMBCALGROUPParms->prevResults_phaseRx)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CLOFFSET, (size_t)(((size_t)&(pCOMBCALGROUPParms->clOffset)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CLCORR_REAL, (size_t)(((size_t)&(pCOMBCALGROUPParms->clCorr_real)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CLCORR_IMAG, (size_t)(((size_t)&(pCOMBCALGROUPParms->clCorr_imag)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXCORR_REAL, (size_t)(((size_t)&(pCOMBCALGROUPParms->txCorr_real)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXCORR_IMAG, (size_t)(((size_t)&(pCOMBCALGROUPParms->txCorr_imag)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXCORR_REAL, (size_t)(((size_t)&(pCOMBCALGROUPParms->rxCorr_real)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXCORR_IMAG, (size_t)(((size_t)&(pCOMBCALGROUPParms->rxCorr_imag)) - (size_t)pCOMBCALGROUPParms), pParmsOffset);
    return((void*) pCOMBCALGROUPParms);
}

static COMBCALGROUP_OP_FUNC COMBCALGROUPOpFunc = NULL;

TLV2_API void registerCOMBCALGROUPHandler(COMBCALGROUP_OP_FUNC fp)
{
    COMBCALGROUPOpFunc = fp;
}

A_BOOL COMBCALGROUPOp(void *pParms)
{
    CMD_COMBCALGROUP_PARMS *pCOMBCALGROUPParms = (CMD_COMBCALGROUP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("COMBCALGROUPOp: cmdIdCombCalGroup %u\n", pCOMBCALGROUPParms->cmdIdCombCalGroup);
    A_PRINTF("COMBCALGROUPOp: phyId %u\n", pCOMBCALGROUPParms->phyId);
    A_PRINTF("COMBCALGROUPOp: bandCode %u\n", pCOMBCALGROUPParms->bandCode);
    A_PRINTF("COMBCALGROUPOp: bwCode %u\n", pCOMBCALGROUPParms->bwCode);
    A_PRINTF("COMBCALGROUPOp: freq %u\n", pCOMBCALGROUPParms->freq);
    A_PRINTF("COMBCALGROUPOp: dyn_bw %u\n", pCOMBCALGROUPParms->dyn_bw);
    A_PRINTF("COMBCALGROUPOp: chain %u\n", pCOMBCALGROUPParms->chain);
    A_PRINTF("COMBCALGROUPOp: num_plybck_tone %u\n", pCOMBCALGROUPParms->num_plybck_tone);
    A_PRINTF("COMBCALGROUPOp: lenPlybckTones %u\n", pCOMBCALGROUPParms->lenPlybckTones);
    A_PRINTF("COMBCALGROUPOp: plybckToneIdx %u\n", pCOMBCALGROUPParms->plybckToneIdx);
    A_PRINTF("COMBCALGROUPOp: playBackLen %u\n", pCOMBCALGROUPParms->playBackLen);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPOp: plybckTones %u\n", pCOMBCALGROUPParms->plybckTones[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPOp: calToneList 0x%x\n", pCOMBCALGROUPParms->calToneList[i]);
    }
    A_PRINTF("COMBCALGROUPOp: initCal %u\n", pCOMBCALGROUPParms->initCal);
    A_PRINTF("COMBCALGROUPOp: iteration %u\n", pCOMBCALGROUPParms->iteration);
    A_PRINTF("COMBCALGROUPOp: clLoc %u\n", pCOMBCALGROUPParms->clLoc);
    A_PRINTF("COMBCALGROUPOp: isRunCalTx %u\n", pCOMBCALGROUPParms->isRunCalTx);
    A_PRINTF("COMBCALGROUPOp: combine %u\n", pCOMBCALGROUPParms->combine);
    A_PRINTF("COMBCALGROUPOp: homeCh %u\n", pCOMBCALGROUPParms->homeCh);
    A_PRINTF("COMBCALGROUPOp: synth %u\n", pCOMBCALGROUPParms->synth);
    A_PRINTF("COMBCALGROUPOp: verbose %u\n", pCOMBCALGROUPParms->verbose);
    A_PRINTF("COMBCALGROUPOp: max_offset %u\n", pCOMBCALGROUPParms->max_offset);
    A_PRINTF("COMBCALGROUPOp: DBS %u\n", pCOMBCALGROUPParms->DBS);
    A_PRINTF("COMBCALGROUPOp: iqIter %u\n", pCOMBCALGROUPParms->iqIter);
    A_PRINTF("COMBCALGROUPOp: clIter %u\n", pCOMBCALGROUPParms->clIter);
    A_PRINTF("COMBCALGROUPOp: numTxGains %u\n", pCOMBCALGROUPParms->numTxGains);
    A_PRINTF("COMBCALGROUPOp: dbStep %u\n", pCOMBCALGROUPParms->dbStep);
    A_PRINTF("COMBCALGROUPOp: calMode %u\n", pCOMBCALGROUPParms->calMode);
    A_PRINTF("COMBCALGROUPOp: havePrevResults %u\n", pCOMBCALGROUPParms->havePrevResults);
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPOp: prevResults_magTx 0x%x\n", pCOMBCALGROUPParms->prevResults_magTx[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPOp: prevResults_magRx 0x%x\n", pCOMBCALGROUPParms->prevResults_magRx[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPOp: prevResults_phaseTx 0x%x\n", pCOMBCALGROUPParms->prevResults_phaseTx[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPOp: prevResults_phaseRx 0x%x\n", pCOMBCALGROUPParms->prevResults_phaseRx[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPOp: clOffset %u\n", pCOMBCALGROUPParms->clOffset[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("COMBCALGROUPOp: clCorr_real 0x%x\n", pCOMBCALGROUPParms->clCorr_real[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("COMBCALGROUPOp: clCorr_imag 0x%x\n", pCOMBCALGROUPParms->clCorr_imag[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 112 entries
    {
        A_PRINTF("COMBCALGROUPOp: txCorr_real 0x%x\n", pCOMBCALGROUPParms->txCorr_real[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 112 entries
    {
        A_PRINTF("COMBCALGROUPOp: txCorr_imag 0x%x\n", pCOMBCALGROUPParms->txCorr_imag[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 112 entries
    {
        A_PRINTF("COMBCALGROUPOp: rxCorr_real 0x%x\n", pCOMBCALGROUPParms->rxCorr_real[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 112 entries
    {
        A_PRINTF("COMBCALGROUPOp: rxCorr_imag 0x%x\n", pCOMBCALGROUPParms->rxCorr_imag[i]);
    }
#endif //_DEBUG

    if (NULL != COMBCALGROUPOpFunc) {
        (*COMBCALGROUPOpFunc)(pCOMBCALGROUPParms);
    }
    return(TRUE);
}

void* initCOMBCALGROUPRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_COMBCALGROUPRSP_PARMS  *pCOMBCALGROUPRSPParms = (CMD_COMBCALGROUPRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pCOMBCALGROUPRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pCOMBCALGROUPRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    memset(pCOMBCALGROUPRSPParms->pad2, 0, sizeof(pCOMBCALGROUPRSPParms->pad2));
    memset(pCOMBCALGROUPRSPParms->clCorr_real, 0, sizeof(pCOMBCALGROUPRSPParms->clCorr_real));
    memset(pCOMBCALGROUPRSPParms->clCorr_imag, 0, sizeof(pCOMBCALGROUPRSPParms->clCorr_imag));
    memset(pCOMBCALGROUPRSPParms->txCorr_real, 0, sizeof(pCOMBCALGROUPRSPParms->txCorr_real));
    memset(pCOMBCALGROUPRSPParms->txCorr_imag, 0, sizeof(pCOMBCALGROUPRSPParms->txCorr_imag));
    memset(pCOMBCALGROUPRSPParms->rxCorr_real, 0, sizeof(pCOMBCALGROUPRSPParms->rxCorr_real));
    memset(pCOMBCALGROUPRSPParms->rxCorr_imag, 0, sizeof(pCOMBCALGROUPRSPParms->rxCorr_imag));
    memset(pCOMBCALGROUPRSPParms->trxGainPair_txGainIdx, 0, sizeof(pCOMBCALGROUPRSPParms->trxGainPair_txGainIdx));
    memset(pCOMBCALGROUPRSPParms->trxGainPair_txDacGain, 0, sizeof(pCOMBCALGROUPRSPParms->trxGainPair_txDacGain));
    memset(pCOMBCALGROUPRSPParms->trxGainPair_rxGainIdx, 0, sizeof(pCOMBCALGROUPRSPParms->trxGainPair_rxGainIdx));
    memset(pCOMBCALGROUPRSPParms->prevResults_magTx, 0, sizeof(pCOMBCALGROUPRSPParms->prevResults_magTx));
    memset(pCOMBCALGROUPRSPParms->prevResults_magRx, 0, sizeof(pCOMBCALGROUPRSPParms->prevResults_magRx));
    memset(pCOMBCALGROUPRSPParms->prevResults_phaseTx, 0, sizeof(pCOMBCALGROUPRSPParms->prevResults_phaseTx));
    memset(pCOMBCALGROUPRSPParms->prevResults_phaseRx, 0, sizeof(pCOMBCALGROUPRSPParms->prevResults_phaseRx));
    pCOMBCALGROUPRSPParms->executionTime = pParmDict[PARM_EXECUTIONTIME].v.valU32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->phyId)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->status)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PAD2, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->pad2)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CLCORR_REAL, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->clCorr_real)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CLCORR_IMAG, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->clCorr_imag)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXCORR_REAL, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->txCorr_real)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TXCORR_IMAG, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->txCorr_imag)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXCORR_REAL, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->rxCorr_real)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_RXCORR_IMAG, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->rxCorr_imag)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TRXGAINPAIR_TXGAINIDX, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->trxGainPair_txGainIdx)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TRXGAINPAIR_TXDACGAIN, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->trxGainPair_txDacGain)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TRXGAINPAIR_RXGAINIDX, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->trxGainPair_rxGainIdx)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PREVRESULTS_MAGTX, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->prevResults_magTx)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PREVRESULTS_MAGRX, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->prevResults_magRx)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PREVRESULTS_PHASETX, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->prevResults_phaseTx)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PREVRESULTS_PHASERX, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->prevResults_phaseRx)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_EXECUTIONTIME, (size_t)(((size_t)&(pCOMBCALGROUPRSPParms->executionTime)) - (size_t)pCOMBCALGROUPRSPParms), pParmsOffset);
    return((void*) pCOMBCALGROUPRSPParms);
}

static COMBCALGROUPRSP_OP_FUNC COMBCALGROUPRSPOpFunc = NULL;

TLV2_API void registerCOMBCALGROUPRSPHandler(COMBCALGROUPRSP_OP_FUNC fp)
{
    COMBCALGROUPRSPOpFunc = fp;
}

A_BOOL COMBCALGROUPRSPOp(void *pParms)
{
    CMD_COMBCALGROUPRSP_PARMS *pCOMBCALGROUPRSPParms = (CMD_COMBCALGROUPRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("COMBCALGROUPRSPOp: phyId %u\n", pCOMBCALGROUPRSPParms->phyId);
    A_PRINTF("COMBCALGROUPRSPOp: status %u\n", pCOMBCALGROUPRSPParms->status);
    for (i = 0; i < 2 ; i++)
    {
        A_PRINTF("COMBCALGROUPRSPOp: pad2 %u\n", pCOMBCALGROUPRSPParms->pad2[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("COMBCALGROUPRSPOp: clCorr_real 0x%x\n", pCOMBCALGROUPRSPParms->clCorr_real[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 32 entries
    {
        A_PRINTF("COMBCALGROUPRSPOp: clCorr_imag 0x%x\n", pCOMBCALGROUPRSPParms->clCorr_imag[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 112 entries
    {
        A_PRINTF("COMBCALGROUPRSPOp: txCorr_real 0x%x\n", pCOMBCALGROUPRSPParms->txCorr_real[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 112 entries
    {
        A_PRINTF("COMBCALGROUPRSPOp: txCorr_imag 0x%x\n", pCOMBCALGROUPRSPParms->txCorr_imag[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 112 entries
    {
        A_PRINTF("COMBCALGROUPRSPOp: rxCorr_real 0x%x\n", pCOMBCALGROUPRSPParms->rxCorr_real[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 112 entries
    {
        A_PRINTF("COMBCALGROUPRSPOp: rxCorr_imag 0x%x\n", pCOMBCALGROUPRSPParms->rxCorr_imag[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPRSPOp: trxGainPair_txGainIdx 0x%x\n", pCOMBCALGROUPRSPParms->trxGainPair_txGainIdx[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPRSPOp: trxGainPair_txDacGain 0x%x\n", pCOMBCALGROUPRSPParms->trxGainPair_txDacGain[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPRSPOp: trxGainPair_rxGainIdx 0x%x\n", pCOMBCALGROUPRSPParms->trxGainPair_rxGainIdx[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPRSPOp: prevResults_magTx 0x%x\n", pCOMBCALGROUPRSPParms->prevResults_magTx[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPRSPOp: prevResults_magRx 0x%x\n", pCOMBCALGROUPRSPParms->prevResults_magRx[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPRSPOp: prevResults_phaseTx 0x%x\n", pCOMBCALGROUPRSPParms->prevResults_phaseTx[i]);
    }
    for (i = 0; i < 8 ; i++) // can be modified to print up to 16 entries
    {
        A_PRINTF("COMBCALGROUPRSPOp: prevResults_phaseRx 0x%x\n", pCOMBCALGROUPRSPParms->prevResults_phaseRx[i]);
    }
    A_PRINTF("COMBCALGROUPRSPOp: executionTime %u\n", pCOMBCALGROUPRSPParms->executionTime);
#endif //_DEBUG

    if (NULL != COMBCALGROUPRSPOpFunc) {
        (*COMBCALGROUPRSPOpFunc)(pCOMBCALGROUPRSPParms);
    }
    return(TRUE);
}
