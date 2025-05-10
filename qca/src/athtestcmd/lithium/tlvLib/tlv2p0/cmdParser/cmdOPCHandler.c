/*
* Copyright (c) 2021 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// This is an auto-generated file from input\cmdOPCHandler.s
#include "tlv2Inc.h"
#include "cmdOPCHandler.h"

void* initCUST_OPCCALPROCOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_CUST_OPCCALPROC_PARMS  *pCUST_OPCCALPROCParms = (CMD_CUST_OPCCALPROC_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pCUST_OPCCALPROCParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pCUST_OPCCALPROCParms->band = pParmDict[PARM_BAND].v.valU8;
    pCUST_OPCCALPROCParms->freq = pParmDict[PARM_FREQ].v.valU16;
    pCUST_OPCCALPROCParms->chainIdx = pParmDict[PARM_CHAINIDX].v.valU8;
    pCUST_OPCCALPROCParms->opcCtrl = pParmDict[PARM_OPCCTRL].v.valU8;
    pCUST_OPCCALPROCParms->powerOffset = pParmDict[PARM_POWEROFFSET].v.valS8;
    pCUST_OPCCALPROCParms->glutOffset = pParmDict[PARM_GLUTOFFSET].v.valS8;
    pCUST_OPCCALPROCParms->thermalOffset = pParmDict[PARM_THERMALOFFSET].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pCUST_OPCCALPROCParms->phyId)) - (size_t)pCUST_OPCCALPROCParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BAND, (size_t)(((size_t)&(pCUST_OPCCALPROCParms->band)) - (size_t)pCUST_OPCCALPROCParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ, (size_t)(((size_t)&(pCUST_OPCCALPROCParms->freq)) - (size_t)pCUST_OPCCALPROCParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINIDX, (size_t)(((size_t)&(pCUST_OPCCALPROCParms->chainIdx)) - (size_t)pCUST_OPCCALPROCParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_OPCCTRL, (size_t)(((size_t)&(pCUST_OPCCALPROCParms->opcCtrl)) - (size_t)pCUST_OPCCALPROCParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_POWEROFFSET, (size_t)(((size_t)&(pCUST_OPCCALPROCParms->powerOffset)) - (size_t)pCUST_OPCCALPROCParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GLUTOFFSET, (size_t)(((size_t)&(pCUST_OPCCALPROCParms->glutOffset)) - (size_t)pCUST_OPCCALPROCParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_THERMALOFFSET, (size_t)(((size_t)&(pCUST_OPCCALPROCParms->thermalOffset)) - (size_t)pCUST_OPCCALPROCParms), pParmsOffset);
    return((void*) pCUST_OPCCALPROCParms);
}

static CUST_OPCCALPROC_OP_FUNC CUST_OPCCALPROCOpFunc = NULL;

TLV2_API void registerCUST_OPCCALPROCHandler(CUST_OPCCALPROC_OP_FUNC fp)
{
    CUST_OPCCALPROCOpFunc = fp;
}

A_BOOL CUST_OPCCALPROCOp(void *pParms)
{
    CMD_CUST_OPCCALPROC_PARMS *pCUST_OPCCALPROCParms = (CMD_CUST_OPCCALPROC_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("CUST_OPCCALPROCOp: phyId %u\n", pCUST_OPCCALPROCParms->phyId);
    A_PRINTF("CUST_OPCCALPROCOp: band %u\n", pCUST_OPCCALPROCParms->band);
    A_PRINTF("CUST_OPCCALPROCOp: freq %u\n", pCUST_OPCCALPROCParms->freq);
    A_PRINTF("CUST_OPCCALPROCOp: chainIdx %u\n", pCUST_OPCCALPROCParms->chainIdx);
    A_PRINTF("CUST_OPCCALPROCOp: opcCtrl %u\n", pCUST_OPCCALPROCParms->opcCtrl);
    A_PRINTF("CUST_OPCCALPROCOp: powerOffset %u\n", pCUST_OPCCALPROCParms->powerOffset);
    A_PRINTF("CUST_OPCCALPROCOp: glutOffset %u\n", pCUST_OPCCALPROCParms->glutOffset);
    A_PRINTF("CUST_OPCCALPROCOp: thermalOffset %u\n", pCUST_OPCCALPROCParms->thermalOffset);
#endif //_DEBUG

    if (NULL != CUST_OPCCALPROCOpFunc) {
        (*CUST_OPCCALPROCOpFunc)(pCUST_OPCCALPROCParms);
    }
    return(TRUE);
}

void* initCUST_OPCCALPROCRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_CUST_OPCCALPROCRSP_PARMS  *pCUST_OPCCALPROCRSPParms = (CMD_CUST_OPCCALPROCRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pCUST_OPCCALPROCRSPParms->phyId = pParmDict[PARM_PHYID].v.valU8;
    pCUST_OPCCALPROCRSPParms->band = pParmDict[PARM_BAND].v.valU8;
    pCUST_OPCCALPROCRSPParms->freq = pParmDict[PARM_FREQ].v.valU16;
    pCUST_OPCCALPROCRSPParms->chainIdx = pParmDict[PARM_CHAINIDX].v.valU8;
    pCUST_OPCCALPROCRSPParms->status = pParmDict[PARM_STATUS].v.valU8;
    pCUST_OPCCALPROCRSPParms->powerOffset = pParmDict[PARM_POWEROFFSET].v.valS8;
    pCUST_OPCCALPROCRSPParms->glutOffset = pParmDict[PARM_GLUTOFFSET].v.valS8;
    pCUST_OPCCALPROCRSPParms->thermalOffset = pParmDict[PARM_THERMALOFFSET].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PHYID, (size_t)(((size_t)&(pCUST_OPCCALPROCRSPParms->phyId)) - (size_t)pCUST_OPCCALPROCRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BAND, (size_t)(((size_t)&(pCUST_OPCCALPROCRSPParms->band)) - (size_t)pCUST_OPCCALPROCRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ, (size_t)(((size_t)&(pCUST_OPCCALPROCRSPParms->freq)) - (size_t)pCUST_OPCCALPROCRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_CHAINIDX, (size_t)(((size_t)&(pCUST_OPCCALPROCRSPParms->chainIdx)) - (size_t)pCUST_OPCCALPROCRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_STATUS, (size_t)(((size_t)&(pCUST_OPCCALPROCRSPParms->status)) - (size_t)pCUST_OPCCALPROCRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_POWEROFFSET, (size_t)(((size_t)&(pCUST_OPCCALPROCRSPParms->powerOffset)) - (size_t)pCUST_OPCCALPROCRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_GLUTOFFSET, (size_t)(((size_t)&(pCUST_OPCCALPROCRSPParms->glutOffset)) - (size_t)pCUST_OPCCALPROCRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_THERMALOFFSET, (size_t)(((size_t)&(pCUST_OPCCALPROCRSPParms->thermalOffset)) - (size_t)pCUST_OPCCALPROCRSPParms), pParmsOffset);
    return((void*) pCUST_OPCCALPROCRSPParms);
}

static CUST_OPCCALPROCRSP_OP_FUNC CUST_OPCCALPROCRSPOpFunc = NULL;

TLV2_API void registerCUST_OPCCALPROCRSPHandler(CUST_OPCCALPROCRSP_OP_FUNC fp)
{
    CUST_OPCCALPROCRSPOpFunc = fp;
}

A_BOOL CUST_OPCCALPROCRSPOp(void *pParms)
{
    CMD_CUST_OPCCALPROCRSP_PARMS *pCUST_OPCCALPROCRSPParms = (CMD_CUST_OPCCALPROCRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("CUST_OPCCALPROCRSPOp: phyId %u\n", pCUST_OPCCALPROCRSPParms->phyId);
    A_PRINTF("CUST_OPCCALPROCRSPOp: band %u\n", pCUST_OPCCALPROCRSPParms->band);
    A_PRINTF("CUST_OPCCALPROCRSPOp: freq %u\n", pCUST_OPCCALPROCRSPParms->freq);
    A_PRINTF("CUST_OPCCALPROCRSPOp: chainIdx %u\n", pCUST_OPCCALPROCRSPParms->chainIdx);
    A_PRINTF("CUST_OPCCALPROCRSPOp: status %u\n", pCUST_OPCCALPROCRSPParms->status);
    A_PRINTF("CUST_OPCCALPROCRSPOp: powerOffset %u\n", pCUST_OPCCALPROCRSPParms->powerOffset);
    A_PRINTF("CUST_OPCCALPROCRSPOp: glutOffset %u\n", pCUST_OPCCALPROCRSPParms->glutOffset);
    A_PRINTF("CUST_OPCCALPROCRSPOp: thermalOffset %u\n", pCUST_OPCCALPROCRSPParms->thermalOffset);
#endif //_DEBUG

    if (NULL != CUST_OPCCALPROCRSPOpFunc) {
        (*CUST_OPCCALPROCRSPOpFunc)(pCUST_OPCCALPROCRSPParms);
    }
    return(TRUE);
}
