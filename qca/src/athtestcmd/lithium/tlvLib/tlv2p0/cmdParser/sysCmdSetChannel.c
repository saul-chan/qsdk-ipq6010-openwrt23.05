/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

// This is an auto-generated file from input/sysCmdSetChannel.s
#include "tlv2Inc.h"
#include "sysCmdSetChannel.h"

void* initSETCHANNELOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    SYSCMD_SETCHANNEL_PARMS  *pSETCHANNELParms = (SYSCMD_SETCHANNEL_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pSETCHANNELParms->phy_mask = pParmDict[SYSPARM_PHY_MASK-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELParms->radio_mask = pParmDict[SYSPARM_RADIO_MASK-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELParms->crossing = pParmDict[SYSPARM_CROSSING-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELParms->xconnection = pParmDict[SYSPARM_XCONNECTION-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELParms->center_channel_R0 = pParmDict[SYSPARM_CENTER_CHANNEL_R0-SYSPARM_FIRST_IDX].v.valU32;
    pSETCHANNELParms->center_channel_R1 = pParmDict[SYSPARM_CENTER_CHANNEL_R1-SYSPARM_FIRST_IDX].v.valU32;
    pSETCHANNELParms->BW_R0 = pParmDict[SYSPARM_BW_R0-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELParms->BW_R1 = pParmDict[SYSPARM_BW_R1-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELParms->mode = pParmDict[SYSPARM_MODE-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELParms->primary_location_R0 = pParmDict[SYSPARM_PRIMARY_LOCATION_R0-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELParms->primary_location_R1 = pParmDict[SYSPARM_PRIMARY_LOCATION_R1-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELParms->cal_mask = pParmDict[SYSPARM_CAL_MASK-SYSPARM_FIRST_IDX].v.valU32;
    pSETCHANNELParms->force_cal_mask = pParmDict[SYSPARM_FORCE_CAL_MASK-SYSPARM_FIRST_IDX].v.valU32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)SYSPARM_PHY_MASK, (size_t)(((size_t)&(pSETCHANNELParms->phy_mask)) - (size_t)pSETCHANNELParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_RADIO_MASK, (size_t)(((size_t)&(pSETCHANNELParms->radio_mask)) - (size_t)pSETCHANNELParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_CROSSING, (size_t)(((size_t)&(pSETCHANNELParms->crossing)) - (size_t)pSETCHANNELParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_XCONNECTION, (size_t)(((size_t)&(pSETCHANNELParms->xconnection)) - (size_t)pSETCHANNELParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_CENTER_CHANNEL_R0, (size_t)(((size_t)&(pSETCHANNELParms->center_channel_R0)) - (size_t)pSETCHANNELParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_CENTER_CHANNEL_R1, (size_t)(((size_t)&(pSETCHANNELParms->center_channel_R1)) - (size_t)pSETCHANNELParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_BW_R0, (size_t)(((size_t)&(pSETCHANNELParms->BW_R0)) - (size_t)pSETCHANNELParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_BW_R1, (size_t)(((size_t)&(pSETCHANNELParms->BW_R1)) - (size_t)pSETCHANNELParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_MODE, (size_t)(((size_t)&(pSETCHANNELParms->mode)) - (size_t)pSETCHANNELParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_PRIMARY_LOCATION_R0, (size_t)(((size_t)&(pSETCHANNELParms->primary_location_R0)) - (size_t)pSETCHANNELParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_PRIMARY_LOCATION_R1, (size_t)(((size_t)&(pSETCHANNELParms->primary_location_R1)) - (size_t)pSETCHANNELParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_CAL_MASK, (size_t)(((size_t)&(pSETCHANNELParms->cal_mask)) - (size_t)pSETCHANNELParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_FORCE_CAL_MASK, (size_t)(((size_t)&(pSETCHANNELParms->force_cal_mask)) - (size_t)pSETCHANNELParms), pParmsOffset);
    return((void*) pSETCHANNELParms);
}

static SETCHANNEL_OP_FUNC SETCHANNELOpFunc = NULL;

TLV2_API void registerSETCHANNELHandler(SETCHANNEL_OP_FUNC fp)
{
    SETCHANNELOpFunc = fp;
}

A_BOOL SETCHANNELOp(void *pParms)
{
    SYSCMD_SETCHANNEL_PARMS *pSETCHANNELParms = (SYSCMD_SETCHANNEL_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("SETCHANNELOp: phy_mask %u\n", pSETCHANNELParms->phy_mask);
    A_PRINTF("SETCHANNELOp: radio_mask %u\n", pSETCHANNELParms->radio_mask);
    A_PRINTF("SETCHANNELOp: crossing %u\n", pSETCHANNELParms->crossing);
    A_PRINTF("SETCHANNELOp: xconnection %u\n", pSETCHANNELParms->xconnection);
    A_PRINTF("SETCHANNELOp: center_channel_R0 %u\n", pSETCHANNELParms->center_channel_R0);
    A_PRINTF("SETCHANNELOp: center_channel_R1 %u\n", pSETCHANNELParms->center_channel_R1);
    A_PRINTF("SETCHANNELOp: BW_R0 %u\n", pSETCHANNELParms->BW_R0);
    A_PRINTF("SETCHANNELOp: BW_R1 %u\n", pSETCHANNELParms->BW_R1);
    A_PRINTF("SETCHANNELOp: mode %u\n", pSETCHANNELParms->mode);
    A_PRINTF("SETCHANNELOp: primary_location_R0 %u\n", pSETCHANNELParms->primary_location_R0);
    A_PRINTF("SETCHANNELOp: primary_location_R1 %u\n", pSETCHANNELParms->primary_location_R1);
    A_PRINTF("SETCHANNELOp: cal_mask %u\n", pSETCHANNELParms->cal_mask);
    A_PRINTF("SETCHANNELOp: force_cal_mask %u\n", pSETCHANNELParms->force_cal_mask);
#endif //_DEBUG

    if (NULL != SETCHANNELOpFunc) {
        (*SETCHANNELOpFunc)(pSETCHANNELParms);
    }
    return(TRUE);
}

void* initSETCHANNELRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    SYSCMD_SETCHANNELRSP_PARMS  *pSETCHANNELRSPParms = (SYSCMD_SETCHANNELRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pSETCHANNELRSPParms->phy_mask = pParmDict[SYSPARM_PHY_MASK-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELRSPParms->radio_mask = pParmDict[SYSPARM_RADIO_MASK-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELRSPParms->crossing = pParmDict[SYSPARM_CROSSING-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELRSPParms->xconnection = pParmDict[SYSPARM_XCONNECTION-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELRSPParms->center_channel_R0 = pParmDict[SYSPARM_CENTER_CHANNEL_R0-SYSPARM_FIRST_IDX].v.valU32;
    pSETCHANNELRSPParms->center_channel_R1 = pParmDict[SYSPARM_CENTER_CHANNEL_R1-SYSPARM_FIRST_IDX].v.valU32;
    pSETCHANNELRSPParms->BW_R0 = pParmDict[SYSPARM_BW_R0-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELRSPParms->BW_R1 = pParmDict[SYSPARM_BW_R1-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELRSPParms->mode = pParmDict[SYSPARM_MODE-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELRSPParms->primary_location_R0 = pParmDict[SYSPARM_PRIMARY_LOCATION_R0-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELRSPParms->primary_location_R1 = pParmDict[SYSPARM_PRIMARY_LOCATION_R1-SYSPARM_FIRST_IDX].v.valU8;
    pSETCHANNELRSPParms->cal_mask = pParmDict[SYSPARM_CAL_MASK-SYSPARM_FIRST_IDX].v.valU32;
    pSETCHANNELRSPParms->force_cal_mask = pParmDict[SYSPARM_FORCE_CAL_MASK-SYSPARM_FIRST_IDX].v.valU32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)SYSPARM_PHY_MASK, (size_t)(((size_t)&(pSETCHANNELRSPParms->phy_mask)) - (size_t)pSETCHANNELRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_RADIO_MASK, (size_t)(((size_t)&(pSETCHANNELRSPParms->radio_mask)) - (size_t)pSETCHANNELRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_CROSSING, (size_t)(((size_t)&(pSETCHANNELRSPParms->crossing)) - (size_t)pSETCHANNELRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_XCONNECTION, (size_t)(((size_t)&(pSETCHANNELRSPParms->xconnection)) - (size_t)pSETCHANNELRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_CENTER_CHANNEL_R0, (size_t)(((size_t)&(pSETCHANNELRSPParms->center_channel_R0)) - (size_t)pSETCHANNELRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_CENTER_CHANNEL_R1, (size_t)(((size_t)&(pSETCHANNELRSPParms->center_channel_R1)) - (size_t)pSETCHANNELRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_BW_R0, (size_t)(((size_t)&(pSETCHANNELRSPParms->BW_R0)) - (size_t)pSETCHANNELRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_BW_R1, (size_t)(((size_t)&(pSETCHANNELRSPParms->BW_R1)) - (size_t)pSETCHANNELRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_MODE, (size_t)(((size_t)&(pSETCHANNELRSPParms->mode)) - (size_t)pSETCHANNELRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_PRIMARY_LOCATION_R0, (size_t)(((size_t)&(pSETCHANNELRSPParms->primary_location_R0)) - (size_t)pSETCHANNELRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_PRIMARY_LOCATION_R1, (size_t)(((size_t)&(pSETCHANNELRSPParms->primary_location_R1)) - (size_t)pSETCHANNELRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_CAL_MASK, (size_t)(((size_t)&(pSETCHANNELRSPParms->cal_mask)) - (size_t)pSETCHANNELRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)SYSPARM_FORCE_CAL_MASK, (size_t)(((size_t)&(pSETCHANNELRSPParms->force_cal_mask)) - (size_t)pSETCHANNELRSPParms), pParmsOffset);
    return((void*) pSETCHANNELRSPParms);
}

static SETCHANNELRSP_OP_FUNC SETCHANNELRSPOpFunc = NULL;

TLV2_API void registerSETCHANNELRSPHandler(SETCHANNELRSP_OP_FUNC fp)
{
    SETCHANNELRSPOpFunc = fp;
}

A_BOOL SETCHANNELRSPOp(void *pParms)
{
    SYSCMD_SETCHANNELRSP_PARMS *pSETCHANNELRSPParms = (SYSCMD_SETCHANNELRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("SETCHANNELRSPOp: phy_mask %u\n", pSETCHANNELRSPParms->phy_mask);
    A_PRINTF("SETCHANNELRSPOp: radio_mask %u\n", pSETCHANNELRSPParms->radio_mask);
    A_PRINTF("SETCHANNELRSPOp: crossing %u\n", pSETCHANNELRSPParms->crossing);
    A_PRINTF("SETCHANNELRSPOp: xconnection %u\n", pSETCHANNELRSPParms->xconnection);
    A_PRINTF("SETCHANNELRSPOp: center_channel_R0 %u\n", pSETCHANNELRSPParms->center_channel_R0);
    A_PRINTF("SETCHANNELRSPOp: center_channel_R1 %u\n", pSETCHANNELRSPParms->center_channel_R1);
    A_PRINTF("SETCHANNELRSPOp: BW_R0 %u\n", pSETCHANNELRSPParms->BW_R0);
    A_PRINTF("SETCHANNELRSPOp: BW_R1 %u\n", pSETCHANNELRSPParms->BW_R1);
    A_PRINTF("SETCHANNELRSPOp: mode %u\n", pSETCHANNELRSPParms->mode);
    A_PRINTF("SETCHANNELRSPOp: primary_location_R0 %u\n", pSETCHANNELRSPParms->primary_location_R0);
    A_PRINTF("SETCHANNELRSPOp: primary_location_R1 %u\n", pSETCHANNELRSPParms->primary_location_R1);
    A_PRINTF("SETCHANNELRSPOp: cal_mask %u\n", pSETCHANNELRSPParms->cal_mask);
    A_PRINTF("SETCHANNELRSPOp: force_cal_mask %u\n", pSETCHANNELRSPParms->force_cal_mask);
#endif //_DEBUG

    if (NULL != SETCHANNELRSPOpFunc) {
        (*SETCHANNELRSPOpFunc)(pSETCHANNELRSPParms);
    }
    return(TRUE);
}
