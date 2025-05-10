/* Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdSendTPC_TxMeas.s
#include "tlv2Inc.h"
#include "cmdSendTPC_TxMeas.h"

void* initSENDTPC_TXMEASOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_SENDTPC_TXMEAS_PARMS  *pSENDTPC_TXMEASParms = (CMD_SENDTPC_TXMEAS_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pSENDTPC_TXMEASParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pSENDTPC_TXMEASParms->chanIdx = pParmDict[PARM_CHANIDX].v.valU8;
    pSENDTPC_TXMEASParms->chainIdx = pParmDict[PARM_CHAINIDX].v.valU8;
    pSENDTPC_TXMEASParms->loopCnt = pParmDict[PARM_LOOPCNT].v.valU8;
    pSENDTPC_TXMEASParms->gainIdx = pParmDict[PARM_GAINIDX].v.valU8;
    pSENDTPC_TXMEASParms->tpcpdadc = pParmDict[PARM_TPCPDADC].v.valU8;
    pSENDTPC_TXMEASParms->tpcMeasuredPwr = pParmDict[PARM_TPCMEASUREDPWR].v.valS16;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pSENDTPC_TXMEASParms->phyId)) - (size_t)pSENDTPC_TXMEASParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHANIDX, (size_t)(((size_t)&(pSENDTPC_TXMEASParms->chanIdx)) - (size_t)pSENDTPC_TXMEASParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINIDX, (size_t)(((size_t)&(pSENDTPC_TXMEASParms->chainIdx)) - (size_t)pSENDTPC_TXMEASParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LOOPCNT, (size_t)(((size_t)&(pSENDTPC_TXMEASParms->loopCnt)) - (size_t)pSENDTPC_TXMEASParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GAINIDX, (size_t)(((size_t)&(pSENDTPC_TXMEASParms->gainIdx)) - (size_t)pSENDTPC_TXMEASParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TPCPDADC, (size_t)(((size_t)&(pSENDTPC_TXMEASParms->tpcpdadc)) - (size_t)pSENDTPC_TXMEASParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_TPCMEASUREDPWR, (size_t)(((size_t)&(pSENDTPC_TXMEASParms->tpcMeasuredPwr)) - (size_t)pSENDTPC_TXMEASParms), pParmsOffset);
    return((void*) pSENDTPC_TXMEASParms);
}

static SENDTPC_TXMEAS_OP_FUNC SENDTPC_TXMEASOpFunc = NULL;

TLV2_API void registerSENDTPC_TXMEASHandler(SENDTPC_TXMEAS_OP_FUNC fp)
{
    SENDTPC_TXMEASOpFunc = fp;
}

A_BOOL SENDTPC_TXMEASOp(void *pParms)
{
    CMD_SENDTPC_TXMEAS_PARMS *pSENDTPC_TXMEASParms = (CMD_SENDTPC_TXMEAS_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("SENDTPC_TXMEASOp: phyId %u\n", pSENDTPC_TXMEASParms->phyId);
    A_PRINTF("SENDTPC_TXMEASOp: chanIdx %u\n", pSENDTPC_TXMEASParms->chanIdx);
    A_PRINTF("SENDTPC_TXMEASOp: chainIdx %u\n", pSENDTPC_TXMEASParms->chainIdx);
    A_PRINTF("SENDTPC_TXMEASOp: loopCnt %u\n", pSENDTPC_TXMEASParms->loopCnt);
    A_PRINTF("SENDTPC_TXMEASOp: gainIdx %u\n", pSENDTPC_TXMEASParms->gainIdx);
    A_PRINTF("SENDTPC_TXMEASOp: tpcpdadc %u\n", pSENDTPC_TXMEASParms->tpcpdadc);
    A_PRINTF("SENDTPC_TXMEASOp: tpcMeasuredPwr %d\n", pSENDTPC_TXMEASParms->tpcMeasuredPwr);
#endif //_DEBUG

    if (NULL != SENDTPC_TXMEASOpFunc) {
        (*SENDTPC_TXMEASOpFunc)(pSENDTPC_TXMEASParms);
    }
    return(TRUE);
}
