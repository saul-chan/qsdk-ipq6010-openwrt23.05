/* Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// This is an auto-generated file from input/cmdOfdmaUlTxConfig.s
#include "tlv2Inc.h"
#include "cmdOfdmaUlTxConfig.h"

void* initOFDMAULTXCONFIGOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    return(NULL);
}

static OFDMAULTXCONFIG_OP_FUNC OFDMAULTXCONFIGOpFunc = NULL;

TLV2_API void registerOFDMAULTXCONFIGHandler(OFDMAULTXCONFIG_OP_FUNC fp)
{
    OFDMAULTXCONFIGOpFunc = fp;
}

A_BOOL OFDMAULTXCONFIGOp(void *pParms)
{
    if (NULL != OFDMAULTXCONFIGOpFunc) {
        (*OFDMAULTXCONFIGOpFunc)(NULL);
    }
    return(TRUE);
}

void* initOFDMAULTXCONFIGRSPOpParms(A_UINT8 *pParmsCommon, PARM_OFFSET_TBL *pParmsOffset, PARM_DICT *pParmDict)
{
    CMD_OFDMAULTXCONFIGRSP_PARMS  *pOFDMAULTXCONFIGRSPParms = (CMD_OFDMAULTXCONFIGRSP_PARMS *)pParmsCommon;

    if (pParmsCommon == NULL) return (NULL);


    // Populate the parm structure with initial values
    pOFDMAULTXCONFIGRSPParms->preFECPad = pParmDict[PARM_PREFECPAD].v.valU8;
    pOFDMAULTXCONFIGRSPParms->ldpcExtraSymbol = pParmDict[PARM_LDPCEXTRASYMBOL].v.valU8;
    pOFDMAULTXCONFIGRSPParms->peDisambiguity = pParmDict[PARM_PEDISAMBIGUITY].v.valU8;
    pOFDMAULTXCONFIGRSPParms->packetExt = pParmDict[PARM_PACKETEXT].v.valU8;
    pOFDMAULTXCONFIGRSPParms->numOfHeLtf = pParmDict[PARM_NUMOFHELTF].v.valU32;

    // Make up ParmOffsetTbl
    resetParmOffsetFields();
    fillParmOffsetTbl((A_UINT32)PARM_PREFECPAD, (size_t)(((size_t)&(pOFDMAULTXCONFIGRSPParms->preFECPad)) - (size_t)pOFDMAULTXCONFIGRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_LDPCEXTRASYMBOL, (size_t)(((size_t)&(pOFDMAULTXCONFIGRSPParms->ldpcExtraSymbol)) - (size_t)pOFDMAULTXCONFIGRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PEDISAMBIGUITY, (size_t)(((size_t)&(pOFDMAULTXCONFIGRSPParms->peDisambiguity)) - (size_t)pOFDMAULTXCONFIGRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_PACKETEXT, (size_t)(((size_t)&(pOFDMAULTXCONFIGRSPParms->packetExt)) - (size_t)pOFDMAULTXCONFIGRSPParms), pParmsOffset);
    fillParmOffsetTbl((A_UINT32)PARM_NUMOFHELTF, (size_t)(((size_t)&(pOFDMAULTXCONFIGRSPParms->numOfHeLtf)) - (size_t)pOFDMAULTXCONFIGRSPParms), pParmsOffset);
    return((void*) pOFDMAULTXCONFIGRSPParms);
}

static OFDMAULTXCONFIGRSP_OP_FUNC OFDMAULTXCONFIGRSPOpFunc = NULL;

TLV2_API void registerOFDMAULTXCONFIGRSPHandler(OFDMAULTXCONFIGRSP_OP_FUNC fp)
{
    OFDMAULTXCONFIGRSPOpFunc = fp;
}

A_BOOL OFDMAULTXCONFIGRSPOp(void *pParms)
{
    CMD_OFDMAULTXCONFIGRSP_PARMS *pOFDMAULTXCONFIGRSPParms = (CMD_OFDMAULTXCONFIGRSP_PARMS *)pParms;

#if 0 //for debugging, comment out this line, and uncomment the line below
//#ifdef _DEBUG

    A_PRINTF("OFDMAULTXCONFIGRSPOp: preFECPad %u\n", pOFDMAULTXCONFIGRSPParms->preFECPad);
    A_PRINTF("OFDMAULTXCONFIGRSPOp: ldpcExtraSymbol %u\n", pOFDMAULTXCONFIGRSPParms->ldpcExtraSymbol);
    A_PRINTF("OFDMAULTXCONFIGRSPOp: peDisambiguity %u\n", pOFDMAULTXCONFIGRSPParms->peDisambiguity);
    A_PRINTF("OFDMAULTXCONFIGRSPOp: packetExt %u\n", pOFDMAULTXCONFIGRSPParms->packetExt);
    A_PRINTF("OFDMAULTXCONFIGRSPOp: numOfHeLtf %u\n", pOFDMAULTXCONFIGRSPParms->numOfHeLtf);
#endif //_DEBUG

    if (NULL != OFDMAULTXCONFIGRSPOpFunc) {
        (*OFDMAULTXCONFIGRSPOpFunc)(pOFDMAULTXCONFIGRSPParms);
    }
    return(TRUE);
}
