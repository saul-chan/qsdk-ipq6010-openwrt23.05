/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdRadarDetectHandler.s
#include "tlv2Inc.h"
#include "cmdRadarDetectHandler.h"

void *initRADARDETECTOpParms(
        A_UINT8 *pParmsCommon,
        PARM_OFFSET_TBL *pParmsOffset,
        PARM_DICT *pParmDict)
{
    int i, j; // for initializing array parameter
    CMD_RADARDETECT_PARMS *pRADARDETECTParms = (CMD_RADARDETECT_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) {
        return NULL;
    }
    i = j = 0; // assign a number to avoid warning in case i and j are not used

    // Populate the parm structure with initial values
    pRADARDETECTParms->freq  = pParmDict[PARM_FREQ].v.valU16;
    pRADARDETECTParms->freq2 = pParmDict[PARM_FREQ2].v.valU16;

    for (i = 0; i < 2; i++) {
        pRADARDETECTParms->regDmn[i] = pParmDict[PARM_REGDMN].v.ptU16[i];
    }

    pRADARDETECTParms->duration  = pParmDict[PARM_DURATION].v.valU16;
    pRADARDETECTParms->phyId     = pParmDict[PARM_PHYID].v.valU8;
    pRADARDETECTParms->bandwidth = pParmDict[PARM_BANDWIDTH].v.valU8;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_FREQ,
            (size_t)(((size_t)&(pRADARDETECTParms->freq)) - (size_t)pRADARDETECTParms),      pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_FREQ2,
            (size_t)(((size_t)&(pRADARDETECTParms->freq2)) - (size_t)pRADARDETECTParms),     pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_REGDMN,
            (size_t)(((size_t)&(pRADARDETECTParms->regDmn)) - (size_t)pRADARDETECTParms),    pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_DURATION,
            (size_t)(((size_t)&(pRADARDETECTParms->duration)) - (size_t)pRADARDETECTParms),  pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PHYID,
            (size_t)(((size_t)&(pRADARDETECTParms->phyId)) - (size_t)pRADARDETECTParms),     pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_BANDWIDTH,
            (size_t)(((size_t)&(pRADARDETECTParms->bandwidth)) - (size_t)pRADARDETECTParms), pParmsOffset);
    return (void *)pRADARDETECTParms;
}

static RADARDETECT_OP_FUNC RADARDETECTOpFunc = NULL;

TLV2_API void registerRADARDETECTHandler(RADARDETECT_OP_FUNC fp)
{
    RADARDETECTOpFunc = fp;
}

A_BOOL RADARDETECTOp(void *pParms)
{
    CMD_RADARDETECT_PARMS *pRADARDETECTParms = (CMD_RADARDETECT_PARMS *)pParms;

#if 0 // for debugging, comment out this line, and uncomment the line below
// #ifdef _DEBUG

    A_PRINTF("RADARDETECTOp: freq %u\n",  pRADARDETECTParms->freq);
    A_PRINTF("RADARDETECTOp: freq2 %u\n", pRADARDETECTParms->freq2);

    for (i = 0; i < 2; i++) {
        A_PRINTF("RADARDETECTOp: regDmn 0x%x\n", pRADARDETECTParms->regDmn[i]);
    }

    A_PRINTF("RADARDETECTOp: duration %u\n",  pRADARDETECTParms->duration);
    A_PRINTF("RADARDETECTOp: phyId %u\n",     pRADARDETECTParms->phyId);
    A_PRINTF("RADARDETECTOp: bandwidth %u\n", pRADARDETECTParms->bandwidth);
#endif // _DEBUG

    if (NULL != RADARDETECTOpFunc) {
        (*RADARDETECTOpFunc)(pRADARDETECTParms);
    }
    return TRUE;
}

void *initRADARDETECTRSPOpParms(
        A_UINT8 *pParmsCommon,
        PARM_OFFSET_TBL *pParmsOffset,
        PARM_DICT *pParmDict)
{
    CMD_RADARDETECTRSP_PARMS *pRADARDETECTRSPParms = (CMD_RADARDETECTRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) {
        return NULL;
    }

    // Populate the parm structure with initial values
    pRADARDETECTRSPParms->numRadarEvents = pParmDict[PARM_NUMRADAREVENTS].v.valU32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_NUMRADAREVENTS,
            (size_t)(((size_t)&(pRADARDETECTRSPParms->numRadarEvents)) - (size_t)pRADARDETECTRSPParms),
            pParmsOffset);
    return (void *)pRADARDETECTRSPParms;
}

static RADARDETECTRSP_OP_FUNC RADARDETECTRSPOpFunc = NULL;

TLV2_API void registerRADARDETECTRSPHandler(RADARDETECTRSP_OP_FUNC fp)
{
    RADARDETECTRSPOpFunc = fp;
}

A_BOOL RADARDETECTRSPOp(void *pParms)
{
    CMD_RADARDETECTRSP_PARMS *pRADARDETECTRSPParms = (CMD_RADARDETECTRSP_PARMS *)pParms;

#if 0 // for debugging, comment out this line, and uncomment the line below
// #ifdef _DEBUG

    A_PRINTF("RADARDETECTRSPOp: numRadarEvents %u\n", pRADARDETECTRSPParms->numRadarEvents);
#endif // _DEBUG

    if (NULL != RADARDETECTRSPOpFunc) {
        (*RADARDETECTRSPOpFunc)(pRADARDETECTRSPParms);
    }
    return TRUE;
}
