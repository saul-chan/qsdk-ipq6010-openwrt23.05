/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

        LOWI ROME Finite State Machine Header file

GENERAL DESCRIPTION
  This file contains the functions and global data definitions used by the
  LOWI Rome Finite State Machine

Copyright (c) 2014-2019, 2021-2024 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/

#include <base_util/log.h>
#include <common/lowi_utils.h>
#include "lowi_ranging_fsm.h"
#include "lowi_ranging_pronto_fsm.h"
#include "lowi_ranging_helium_fsm.h"
#include "lowi_ranging_sparrow_fsm.h"
#include "lowi_wifidriver_utils.h"
#include "lowi_p2p_ranging.h"
#include "rttm.h"
#include "lowi_time.h"
#include "lowi_internal_const.h"
#include <lowi_strings.h>
#include <unistd.h>
#include <wlan_capabilities.h>

#include <base_util/time_routines.h>


#define LOWI_FSM_EXIT_IF_NULL_POINTER(p,s) if (NULL == p) \
  {                                                       \
    log_warning(TAG, "%s: %s", __FUNCTION__, s);          \
    return -1;                                            \
  }


// Enter/Exit debug macros
#undef ALLOW_ENTER_EXIT_DBG_RANGING_FSM

#ifdef ALLOW_ENTER_EXIT_DBG_RANGING_FSM
#define FSM_ENTER() log_verbose(TAG, "ENTER: %s", __FUNCTION__);
#define FSM_EXIT() log_verbose(TAG, "EXIT: %s", __FUNCTION__);
#else
#define FSM_ENTER()
#define FSM_EXIT()
#endif

using namespace qc_loc_fw;

const char * const LOWIRangingFSM::TAG = "LOWIRangingFSM";
bool RangingCapRespRecevied = false;

eLOWIPhyMode validPhyModeTable[LOWIDiscoveryScanRequest::BAND_MAX_BIT][RTT_PREAMBLE_MAX][BW_MAX] =
{
  /* 2G BAND */
  {
    /* RTT_PREAMBLE_LEGACY */
    {
      LOWI_RTT_PHY_MODE_11G    ,        /*  BW_20MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_40MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_80MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN         /*  BW_160MHZ  */
    },
    /* RTT_PREAMBLE_HT     */
    {
      LOWI_RTT_PHY_MODE_11NG_HT20,      /*  BW_20MHZ   */
      LOWI_RTT_PHY_MODE_11NG_HT40,      /*  BW_40MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_80MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN         /*  BW_160MHZ  */
    },
    /* RTT_PREAMBLE_VHT    */
    {
      LOWI_RTT_PHY_MODE_11AC_VHT20_2G,  /*  BW_20MHZ   */
      LOWI_RTT_PHY_MODE_11AC_VHT40_2G,  /*  BW_40MHZ   */
      LOWI_RTT_PHY_MODE_11AC_VHT80_2G,  /*  BW_80MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN         /*  BW_160MHZ  */
     },
    /* RTT_PREAMBLE_HE     */
    {
      LOWI_RTT_PHY_MODE_11AX_HE20_2G,   /*  BW_20MHZ   */
      LOWI_RTT_PHY_MODE_11AX_HE40_2G,   /*  BW_40MHZ   */
      LOWI_RTT_PHY_MODE_11AX_HE80_2G,   /*  BW_80MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN         /*  BW_160MHZ  */
    }
  },

  /* 5G BAND */
  {
    /* RTT_PREAMBLE_LEGACY */
    {
      LOWI_RTT_PHY_MODE_11A    ,        /*  BW_20MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_40MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_80MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN         /*  BW_160MHZ  */
    },
    /* RTT_PREAMBLE_HT     */
    {
      LOWI_RTT_PHY_MODE_11NA_HT20,      /*  BW_20MHZ   */
      LOWI_RTT_PHY_MODE_11NA_HT40,      /*  BW_40MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_80MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN         /*  BW_160MHZ  */
    },
    /* RTT_PREAMBLE_VHT    */
    {
       LOWI_RTT_PHY_MODE_11AC_VHT20,    /*  BW_20MHZ   */
       LOWI_RTT_PHY_MODE_11AC_VHT40,    /*  BW_40MHZ   */
       LOWI_RTT_PHY_MODE_11AC_VHT80,    /*  BW_80MHZ   */
       LOWI_RTT_PHY_MODE_11AC_VHT160    /*  BW_160MHZ  */
     },
    /* RTT_PREAMBLE_HE     */
    {
      LOWI_RTT_PHY_MODE_11AX_HE20,      /*  BW_20MHZ   */
      LOWI_RTT_PHY_MODE_11AX_HE40,      /*  BW_40MHZ   */
      LOWI_RTT_PHY_MODE_11AX_HE80,      /*  BW_80MHZ   */
      LOWI_RTT_PHY_MODE_11AX_HE160,     /*  BW_160MHZ  */
    }
  },

  /* 6G BAND */
  {
    /* RTT_PREAMBLE_LEGACY */
    {
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_20MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_40MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_80MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN         /*  BW_160MHZ  */
    },
    /* RTT_PREAMBLE_HT     */
    {
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_20MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_40MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_80MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN         /*  BW_160MHZ  */
    },
    /* RTT_PREAMBLE_VHT    */
    {
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_20MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_40MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN,        /*  BW_80MHZ   */
      LOWI_RTT_PHY_MODE_UNKNOWN         /*  BW_160MHZ  */
     },
    /* RTT_PREAMBLE_HE     */
    {
      LOWI_RTT_PHY_MODE_11AX_HE20,      /*  BW_20MHZ   */
      LOWI_RTT_PHY_MODE_11AX_HE40,      /*  BW_40MHZ   */
      LOWI_RTT_PHY_MODE_11AX_HE80,      /*  BW_80MHZ   */
      LOWI_RTT_PHY_MODE_11AX_HE160      /*  BW_160MHZ  */
    }
  },

};
RangingFSM_Event RomeCLDToRomeFSMEventMap[ROME_MSG_MAX] =
{
  /** ROME CLD Messages */
  /* ROME_REG_RSP_MSG */             EVENT_REGISTRATION_SUCCESS,
  /* ROME_CHANNEL_INFO_MSG */        EVENT_CHANNEL_INFO,
  /* ROME_P2P_PEER_EVENT_MSG */      EVENT_P2P_STATUS_UPDATE,
  /* ROME_CLD_ERROR_MSG */           EVENT_CLD_ERROR_MESSAGE,
  /* ROME_WIPHY_INFO_MSG */          EVENT_WIPHY_INFO,

  /** ROME FW Messages */
  /* ROME_RANGING_CAP_MSG */         EVENT_RANGING_CAP_INFO,
  /* ROME_RANGING_MEAS_MSG */        EVENT_RANGING_MEAS_RECV,
  /* ROME_RANGING_ERROR_MSG */       EVENT_RANGING_ERROR,
  /* ROME_RTT_CHANNEL_INFO_MSG */    EVENT_RTT_AVAILABLE_CHANNEL_INFO,
  /* ROME_RESPONDER_INFO_MSG */      EVENT_RESPONDER_CHANNEL_INFO,
  /* ROME_CFG_RESPONDER_MEAS_RSP_MSG*/ EVENT_CFG_RESPONDER_MEAS_RSP,
  /* ROME_RESPONDER_MEAS_INFO_MSG*/    EVENT_RESPONDER_MEAS_INFO,
  /* ROME_FTM_SESSION_DONE_MSG */    EVENT_FTM_SESSION_DONE,
  /* ROME_CANCEL_RTT_RSP_MSG */        EVENT_CANCEL_RANGING_RSP,

  /** NL/Kernel Messages */
  /* ROME_NL_ERROR_MSG */            EVENT_INVALID_NL_MESSAGE
};

// initialize static variables
LOWIRangingFSM *LOWIRangingFSM::mWigigInstance = NULL;
LOWIRangingFSM *LOWIRangingFSM::mWifiInstance  = NULL;
std::map<tANI_U32, ChannelInfo> LOWIRangingFSM::mChannelInfoArray;
_RomeRangingRequest::_RomeRangingRequest()
{
  validRangingScan = FALSE;
  curChIndex       = 0;
  totChs           = 0;
  curAp            = 0;
  totAp            = 0;
}

LOWIRangingFSM::LOWIRangingFSM(LOWIScanResultReceiverListener *scanResultListener,
                               LOWICacheManager *cacheManager,
                               LOWIRanging *lowiRanging)
{
  mListener = scanResultListener;
  mCacheManager = cacheManager;
  mRangingFsmContext.curEvent = EVENT_START_THREAD;
  mRangingFsmContext.curState = STATE_IDLE_START;
  mRangingFsmContext.timeEnd = ROME_FSM_TIMEOUT_FOREVER;
  mRangingFsmContext.terminate_thread = FALSE;
  mRangingFsmContext.notTryingToRegister = TRUE;
  mRangingFsmContext.internalFsmEventPending = FALSE;
  mNewReq = NULL;
  mRangingPipeEvents = MAX_PIPE_EVENTS;
  mLowiMeasResult = NULL;
  mLOWIRanging    = lowiRanging;
  mRtsCtsTag = 1;
  //initialize the responder expiry time to invalid,if responder is not enabled
  //make it valid only when enableresponder API request comes
  mRTTResponderExpiryTimeStamp = 0;

  mRTTResponderMeasStarted = false;

  memset(&mRangingReqRspInfo, 0, sizeof(RangingReqRspInfo));
  mRangingReqRspInfo.reportType = RTT_AGGREGATE_REPORT_NON_CFR;
  mChannelInfoArray.clear();
  max2GHzFreq = 0;
  max5GHzFreq = 0;
  /* Ready for new request */
  mRangingReqRspInfo.lastResponseRecv = TRUE;
  mRangingReqRspInfo.nonAsapTargetPresent = FALSE;
  mRomeRttCapabilities.rangingTypeMask = 0;
  mUse160MhzChanBw = false;
  SetupFsm();
};

LOWIRangingFSM::~LOWIRangingFSM()
{
  mWigigInstance = NULL;
  mWifiInstance  = NULL;
  mChannelInfoArray.clear();
}

void LOWIRangingFSM::SetupFsm()
{
  FSM_ENTER()
  /** Initialize all action functions to "DoNothing". */

  for (unsigned int state = STATE_IDLE_START; state < STATE_MAX; state++)
  {
    for (unsigned int fsmEvent = EVENT_START_THREAD; fsmEvent < EVENT_MAX; fsmEvent++)
    {
      stateTable[state][fsmEvent] = DoNothing;
    }
  }

  /***************** State Functions for State: STATE_IDLE_START *************/
  /** -- Events from LOWI Controller -- */
             /** Current State */    /** Trigger Event */                     /** Action Function */
  stateTable[STATE_IDLE_START]   [EVENT_START_THREAD]                         = SendRegRequest;
  stateTable[STATE_IDLE_START]   [EVENT_RANGING_REQ]                          = HandleRangingReqWhenNotReg;
  /** -- Events from NL Socket -- */
  stateTable[STATE_IDLE_START]   [EVENT_REGISTRATION_SUCCESS]                 = HandleRegSuccess;
  /** -- Events from Timer -- */
  stateTable[STATE_IDLE_START]   [EVENT_TIMEOUT]                              = HandleRegFailureOrLost;

  /***************** State Functions for State: STATE_WAITING_FOR_WIPHY_INFO *************/
  /** -- Events from LOWI Controller -- */
             /** Current State */              /** Trigger Event */                /** Action Function */
  stateTable[STATE_WAITING_FOR_WIPHY_INFO]   [EVENT_RANGING_REQ]                   = HandleRangingReqWhenNotReg;
  /** -- Events from NL Socket -- */
  stateTable[STATE_WAITING_FOR_WIPHY_INFO]   [EVENT_CHANNEL_INFO]                  = HandleChannelInfo;
  /** -- Internal FSM Events  --  */
  stateTable[STATE_WAITING_FOR_WIPHY_INFO]   [EVENT_REGISTRATION_FAILURE_OR_LOST]  = HandleRegFailureOrLost;
  /** -- Events from Timer -- */
  stateTable[STATE_WAITING_FOR_WIPHY_INFO]   [EVENT_TIMEOUT]                       = HandleRegFailureOrLost;

  /***************** State Functions for State: STATE_WAITING_FOR_RANGING_CAP *************/
  /** -- Events from LOWI Controller -- */
             /** Current State */             /** Trigger Event */                /** Action Function */
  stateTable[STATE_WAITING_FOR_RANGING_CAP]   [EVENT_RANGING_REQ]                   = HandleRangingReqWhenNotReg;
  /** -- Events from NL Socket -- */
  stateTable[STATE_WAITING_FOR_RANGING_CAP]   [EVENT_RANGING_CAP_INFO]              = HandleRangingCap;
  /** -- Internal FSM Events  --  */
  stateTable[STATE_WAITING_FOR_RANGING_CAP]   [EVENT_REGISTRATION_FAILURE_OR_LOST]  = HandleRegFailureOrLost;
  /** -- Events from Timer -- */
  stateTable[STATE_WAITING_FOR_RANGING_CAP]   [EVENT_TIMEOUT]                       = HandleRegFailureOrLost;

  /***************** State Functions for State: STATE_READY_AND_IDLE *************/
  /** -- Events from LOWI Controller -- */
             /** Current State */    /** Trigger Event */                /** Action Function */
  stateTable[STATE_READY_AND_IDLE]   [EVENT_RANGING_REQ]                   = HandleRangingReq;
  stateTable[STATE_READY_AND_IDLE]   [EVENT_CONFIGURATION_REQ]             = HandleConfigReq;
  stateTable[STATE_READY_AND_IDLE]   [EVENT_INVALID_REQ]                   = IgnoreRangingReq;
  /** -- Events from NL Socket -- */
  stateTable[STATE_READY_AND_IDLE]   [EVENT_P2P_STATUS_UPDATE]             = HandleP2PInfo;
  stateTable[STATE_READY_AND_IDLE]   [EVENT_RANGING_MEAS_RECV]             = HandleRangingMeas;
  /** -- Internal FSM Events  --  */
  stateTable[STATE_READY_AND_IDLE]   [EVENT_REGISTRATION_FAILURE_OR_LOST]  = HandleRegFailureOrLost;

  /***************** State Functions for State: STATE_PROCESSING_RANGING_REQ *************/
  /** -- Events from LOWI Controller -- */
             /** Current State */           /** Trigger Event */                 /** Action Function */
  stateTable[STATE_PROCESSING_RANGING_REQ]   [EVENT_RANGING_REQ]                   = IgnoreRangingReq;
  /** -- Events from NL Socket -- */
  stateTable[STATE_PROCESSING_RANGING_REQ]   [EVENT_RANGING_ERROR]                 = HandleRangingErrorMsg;
  stateTable[STATE_PROCESSING_RANGING_REQ]   [EVENT_RANGING_MEAS_RECV]             = HandleRangingMeas;
  stateTable[STATE_PROCESSING_RANGING_REQ]   [EVENT_P2P_STATUS_UPDATE]             = HandleP2PInfo;
  stateTable[STATE_PROCESSING_RANGING_REQ]   [EVENT_CLD_ERROR_MESSAGE]             = HandleCldErrorMsg;

  stateTable[STATE_PROCESSING_RANGING_REQ] [EVENT_CANCEL_RANGING_REQ]        =
      HandleCancelRangingReq;
  stateTable[STATE_PROCESSING_CANCEL_RANGING_REQ] [EVENT_CANCEL_RANGING_RSP] =
      HandleCancelRangingRsp;
  stateTable[STATE_PROCESSING_CANCEL_RANGING_REQ] [EVENT_RANGING_ERROR]      =
      HandleCancelRangingError;
  stateTable[STATE_PROCESSING_CANCEL_RANGING_REQ] [EVENT_TIMEOUT]            =
      HandleNlTimeout;
  stateTable[STATE_PROCESSING_CANCEL_RANGING_REQ] [EVENT_RANGING_REQ]        =
      IgnoreRangingReq;

  /** -- Internal FSM Events  --  */
  stateTable[STATE_PROCESSING_RANGING_REQ]   [EVENT_REGISTRATION_FAILURE_OR_LOST]  = HandleRegFailureOrLost;
  /** -- Events from Timer -- */
             /** Current State */           /** Trigger Event */                 /** Action Function */
  stateTable[STATE_PROCESSING_RANGING_REQ]   [EVENT_TIMEOUT]                       = HandleNlTimeout;

  /***************** State Functions for State: STATE_PROCESSING_CONFIG_REQ *************/
  /** -- Events from LOWI Controller -- */
             /** Current State */           /** Trigger Event */                 /** Action Function */
  stateTable[STATE_PROCESSING_CONFIG_REQ]   [EVENT_RANGING_REQ]                   = IgnoreRangingReq;
  /** -- Events from NL Socket -- */
  stateTable[STATE_PROCESSING_CONFIG_REQ]   [EVENT_RANGING_ERROR]                 = HandleConfigRspOrErrorMsg;
  stateTable[STATE_PROCESSING_CONFIG_REQ]   [EVENT_P2P_STATUS_UPDATE]             = HandleP2PInfo;
  stateTable[STATE_PROCESSING_CONFIG_REQ]   [EVENT_CLD_ERROR_MESSAGE]             = HandleCldErrorMsg;
  /** -- Internal FSM Events  --  */
  stateTable[STATE_PROCESSING_CONFIG_REQ]   [EVENT_REGISTRATION_FAILURE_OR_LOST]  = HandleRegFailureOrLost;
  /** -- Events from Timer -- */
             /** Current State */           /** Trigger Event */                 /** Action Function */
  stateTable[STATE_PROCESSING_CONFIG_REQ]   [EVENT_TIMEOUT]                       = HandleNlTimeout;
  FSM_EXIT()
}

int LOWIRangingFSM::HandleRegFailureOrLost(LOWIRangingFSM* pFsmObj)
{
  int retVal = 0;

  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")

  if (pFsmObj->ProcessingRangingRequest())
  {
    /* Stop Processing Request and return Result to User */
    pFsmObj->SendCurrentResultToUser(LOWIResponse::SCAN_STATUS_DRIVER_ERROR);
  }
  if (pFsmObj->NewRequestPending())
  {
    /* Respond to User with Failure */
    IgnoreRangingReq(pFsmObj);
  }

  /* Reset to STATE_IDLE_START and wait indefinitely */
  log_warning(TAG, "%s:Failed with Host Driver", __FUNCTION__);
  pFsmObj->mRangingFsmContext.curState = STATE_IDLE_START;
  pFsmObj->mRangingFsmContext.timeEnd = ROME_FSM_TIMEOUT_FOREVER;
  pFsmObj->mRangingFsmContext.notTryingToRegister = TRUE;

  return retVal;
}

int LOWIRangingFSM::HandleCldErrorMsg(LOWIRangingFSM* pFsmObj)
{
  int retVal = 0;
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")

  tAniMsgHdr * pMsgHdr = (tAniMsgHdr *)pFsmObj->mRangingFsmData;
  if ((ANI_MSG_OEM_ERROR != pMsgHdr->type) ||
      (0 == pMsgHdr->length))
  {
    log_warning(TAG, "%s: Invalid ANI Msg Type %u, len = %d", __FUNCTION__,
                pMsgHdr->type, pMsgHdr->length);
    return -1;
  }
  eOemErrorCode errCode = *((eOemErrorCode *)(pFsmObj->mRangingFsmData +
                                              sizeof(tAniMsgHdr)));
  log_debug(TAG, "%s: CLD Error Msg Type %u, len = %d, code %d", __FUNCTION__,
            pMsgHdr->type, pMsgHdr->length, errCode);
  switch (errCode)
  {
    case OEM_ERR_APP_NOT_REGISTERED:   /* OEM App is not registered */
      /* Reset to STATE_IDLE_START */
      pFsmObj->mRangingFsmContext.curState = STATE_IDLE_START;
      pFsmObj->mRangingFsmContext.timeEnd = ROME_FSM_TIMEOUT_FOREVER;
      pFsmObj->mRangingFsmContext.notTryingToRegister = TRUE;

      //Copy the appropriate request to New Req
      if (pFsmObj->mCurReq)
      {
        if(pFsmObj->eRequestType == LOWIRequest::RANGING_SCAN)
        {
          // Clean up the state for Previous Ranging Request.
          pFsmObj->mRangingReqRspInfo.lastResponseRecv = TRUE;
          pFsmObj->mRangingReqRspInfo.expectedRspFromFw = 0;
        }
        pFsmObj->mNewReq = pFsmObj->mCurReq;
        pFsmObj->mCurReq = NULL;
      }
      // Clean up the stored measurement result.
      // Make sure the measurement result vector is empty
      if (pFsmObj->mLowiMeasResult)
      {
        for (vector <LOWIScanMeasurement*>::Iterator it = pFsmObj->mLowiMeasResult->scanMeasurements.begin();
             it != pFsmObj->mLowiMeasResult->scanMeasurements.end(); it++)
        {
          delete (*it);
        }
        pFsmObj->mLowiMeasResult->scanMeasurements.flush();
        delete pFsmObj->mLowiMeasResult;
        pFsmObj->mLowiMeasResult = NULL;
      }
      //start Registration
      SendRegRequest(pFsmObj);
      break;

    case OEM_ERR_NULL_CONTEXT:          /* Error null context */
    case OEM_ERR_INVALID_SIGNATURE:     /* Invalid signature */
    case OEM_ERR_NULL_MESSAGE_HEADER:   /* Invalid message type */
    case OEM_ERR_INVALID_MESSAGE_TYPE:  /* Invalid message type */
    case OEM_ERR_INVALID_MESSAGE_LENGTH:/* Invalid length in message body */
    case OEM_ERR_REQUEST_REJECTED:      /* Request rejected by the lower layers */
    default:
      log_warning(TAG, "%s:Unexpected error code %d", __FUNCTION__, errCode);
      break;
  }

  return retVal;
}

int LOWIRangingFSM::DoNothing(LOWIRangingFSM* pFsmObj)
{
  int retVal = 0;
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")

  log_debug(TAG, "%s: FSM Recieved Event: %s while in State: %s", __FUNCTION__,
              LOWIStrings::to_string(pFsmObj->mRangingFsmContext.curEvent),
              LOWIStrings::to_string(pFsmObj->mRangingFsmContext.curState));
  return retVal;
}

void LOWIRangingFSM::setFsmInternalEvent()
{
  mRangingFsmContext.internalFsmEventPending = true;
}

void LOWIRangingFSM::clearFsmInternalEvent()
{
  mRangingFsmContext.internalFsmEventPending = false;
}

int LOWIRangingFSM::HandleRangingReqWhenNotReg(LOWIRangingFSM* pFsmObj)
{
  log_verbose(TAG, "%s: Received Ranging Request when Not registered", __FUNCTION__);
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")

  if (pFsmObj->mRangingFsmContext.notTryingToRegister)
  {
    if(SendRegRequest(pFsmObj) != 0)
    {
      log_debug(TAG, "%s: Failed to Start Registration Process", __FUNCTION__);
      return -1;
    }
  }
  return 0;
}
int LOWIRangingFSM::IgnoreRangingReq(LOWIRangingFSM* pFsmObj)
{
  int retVal = 0;
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")

  log_debug(TAG, "%s: Ignore newly arrived ranging request", __FUNCTION__);
  LOWIMeasurementResult* result = new (std::nothrow) LOWIMeasurementResult;
  if (result == NULL)
  {
    log_debug(TAG, "%s:Ranging Measurment Results memory allocation faliure", __FUNCTION__);
    return -1;
  }
  if (pFsmObj->mRangingFsmContext.curEvent == EVENT_INVALID_REQ)
  {
    retVal = pFsmObj->InitializeMeasResult(result, LOWIResponse::SCAN_STATUS_INVALID_REQ);
  }
  else
  {
    retVal = pFsmObj->InitializeMeasResult(result);
  }
  if (retVal != 0)
  {
    log_info(TAG, "%s: initialize LOWI Meas Results failed", __FUNCTION__);
  }
  else
  {
    pFsmObj->mRangingFsmContext.curEvent = EVENT_RANGING_RESPONSE_TO_USER;
    /* Send Result to User */
    pFsmObj->SendResultToClient(result);
  }
  return retVal;
}

int LOWIRangingFSM::SendRegRequest(LOWIRangingFSM* pFsmObj)
{
  int retVal = 0;
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj->mLOWIRanging, "Invalid LOWIRanging object")

  retVal = pFsmObj->mLOWIRanging->RomeSendRegReq();
  if (retVal != 0)
  {
    /* Failed to send Registration request */
    log_warning(TAG, "%s: Failed - Send Registration request to Host Driver", __FUNCTION__);
    pFsmObj->mRangingFsmContext.curEvent = EVENT_REGISTRATION_FAILURE_OR_LOST;
    HandleRegFailureOrLost(pFsmObj);
  }
  else
  {
    pFsmObj->mRangingFsmContext.notTryingToRegister = FALSE;
    pFsmObj->mRangingFsmContext.timeEnd = get_time_rtc_ms() + (SELECT_TIMEOUT_NORMAL * 1000);
  }
  return retVal;
}

uint32 LOWIRangingFSM::getNewRtsCtsTag()
{
  //2^15, mRtsCtsTag from 1 to 32767
  mRtsCtsTag = ((0UL + mRtsCtsTag) % (1UL * (32768 - 1))) +1;
  return mRtsCtsTag;
}

int LOWIRangingFSM::HandleRegSuccess(LOWIRangingFSM* pFsmObj)
{
  IwOemDataCap iwOemDataCap = {0};
  int retVal = -1;
  LOWIMacAddress localStaMac;
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj->mLOWIRanging, "Invalid LOWIRanging object")

  do
  {
    if ( pFsmObj->mLOWIRanging->RomeExtractRegRsp(pFsmObj->mRangingFsmData) != 0)
    {
      log_warning(TAG, "%s:Failed - Register with Wi-Fi Host Driver", __FUNCTION__);
      HandleRegFailureOrLost(pFsmObj);
      break;
    }
    if (LOWIWifiDriverUtils::getWiFiIdentityandCapability(&(iwOemDataCap), localStaMac) < 0)
    {
      log_warning(TAG, "%s:Failed - Get Wifi Capabilities", __FUNCTION__);
      HandleRegFailureOrLost(pFsmObj);
      break;
    }
    if(pFsmObj->mLOWIRanging->RomeSendChannelInfoReq(iwOemDataCap) != 0)
    {
      log_warning(TAG, "%s:Failed - get Channel Information", __FUNCTION__);
      HandleRegFailureOrLost(pFsmObj);
      break;
    }
    pFsmObj->mRangingFsmContext.timeEnd = get_time_rtc_ms() + (SELECT_TIMEOUT_NORMAL * 1000);
    pFsmObj->mRangingFsmContext.curState = STATE_WAITING_FOR_WIPHY_INFO;
    retVal = 0;
  } while(0);

  return retVal;
}

int LOWIRangingFSM::HandleChannelInfo(LOWIRangingFSM* pFsmObj)
{
  std::string interface = "wifi0";
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj->mLOWIRanging, "Invalid LOWIRanging object")

  mChannelInfoArray.clear();
  if(pFsmObj->mLOWIRanging->RomeExtractChannelInfo(pFsmObj->mRangingFsmData,
                                                   pFsmObj->mChannelInfoArray,
                                                   &pFsmObj->max2GHzFreq,
                                                   &pFsmObj->max5GHzFreq,
                                                   &pFsmObj->max6GHzFreq) != 0)
  {
    log_warning(TAG, "%s: Failed - get Channel Information", __FUNCTION__);
    HandleRegFailureOrLost(pFsmObj);
    return -1;
  }
  RangingCapRespRecevied = false;
  if(pFsmObj->mLOWIRanging->RomeSendRangingCapReq(interface) != 0)
  {
    log_warning(TAG, "%s:Failed - get Ranging Capabilities from FW", __FUNCTION__);
    HandleRegFailureOrLost(pFsmObj);
    return -1;
  }
  pFsmObj->mRangingFsmContext.timeEnd = get_time_rtc_ms() + (SELECT_TIMEOUT_NORMAL * 1000);
  pFsmObj->mRangingFsmContext.curState = STATE_WAITING_FOR_RANGING_CAP;
  return 0;
}

int LOWIRangingFSM::HandleRangingCap(LOWIRangingFSM* pFsmObj)
{
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj->mLOWIRanging, "Invalid LOWIRanging object")

  if(pFsmObj->mLOWIRanging->RomeExtractRangingCap(pFsmObj->mRangingFsmData, &(pFsmObj->mRomeRttCapabilities)) != 0)
  {
    log_warning(TAG, "%s:Failed - get Ranging Capabilities from FW", __FUNCTION__);
    HandleRegFailureOrLost(pFsmObj);
    RangingCapRespRecevied = true;
    return -1;
  }
  pFsmObj->mRangingFsmContext.curState = STATE_READY_AND_IDLE;
  RangingCapRespRecevied = true;

  if (pFsmObj->NewRequestPending()) /* Check if there is a pending new request, if so process it immediately */
  {
    pFsmObj->setFsmInternalEvent();
    pFsmObj->LowiReqToEvent(pFsmObj->mNewReq);
  }
  else /* Other wise sit and wait for new request */
  {
    pFsmObj->mRangingFsmContext.timeEnd = ROME_FSM_TIMEOUT_FOREVER;
  }
  return 0;
}

void LOWIRangingFSM::AddRttBgNodeInList(LOWIRangingNode *rangingNode)
{
  // Do not worry, Scheduler will ensure one channel request at a time
  // So we can safley copy this node here instead of while doing in
  // Group by Channel Request.
  if (rangingNode->targetNode.isRttBgNode)
  {
    LOWINodeInfo *node = new (std::nothrow)LOWINodeInfo();
    if ( NULL == node )
    {
      log_error(TAG, "%s: memory allocation failure", __FUNCTION__);
      return;
    }
    // LOWIPeriodicNodeInfo do not have any pointer so shd be fine
    // But we need to write the =Operator method for LOWIPeriodicNodeInfo.
    //*node = mRomeRangingReq.arrayVecRangingNodes[foo][bar].targetNode;
    memcpy((char*)node, (char*)&rangingNode->targetNode, sizeof(LOWINodeInfo));
    log_info(TAG, "adding the bg node in mRomeRangingReq mRttBgNodeList");
    node->bssid.print();
    mRomeRangingReq.mRttBgNodeList.add(node);
  }
}

void LOWIRangingFSM::RemoveRttBgNodeFromList(LOWIMacAddress bssid)
{
  // iterate over the node list to see whether RTT BG nodes got finished
  for (List<LOWINodeInfo*>::Iterator it = mRomeRangingReq.mRttBgNodeList.begin();
         it != mRomeRangingReq.mRttBgNodeList.end(); )
  {
    LOWINodeInfo *node = *it;
    if (node->bssid.compareTo(bssid) == 0)
    {
      log_warning(TAG, "Remove rtt bg node from mRttBgNodeList");
      mRomeRangingReq.mRttBgNodeList.erase(it);
      break;
    }
    else
      ++it;
  }
}

bool LOWIRangingFSM::IsRttBgNodeInList(LOWIMacAddress bssid)
{
  // iterate over the node list to see whether RTT BG nodes got finished
  for (List<LOWINodeInfo*>::Iterator it = mRomeRangingReq.mRttBgNodeList.begin();
         it != mRomeRangingReq.mRttBgNodeList.end(); )
  {
    LOWINodeInfo *node = *it;
    if (node->bssid.compareTo(bssid) == 0)
    {
      log_verbose(TAG, "Found rtt bg node in mRttBgNodeList");
      return true;
    }
    else
      ++it;
  }
  return false;
}

bool LOWIRangingFSM::GetRttBgNodeInList(LOWIMacAddress bssid, LOWINodeInfo *node)
{
  // iterate over the node list to see whether RTT BG nodes got finished
  for (List<LOWINodeInfo*>::Iterator it = mRomeRangingReq.mRttBgNodeList.begin();
         it != mRomeRangingReq.mRttBgNodeList.end(); )
  {
    LOWINodeInfo *pNode = *it;
    if (pNode->bssid.compareTo(bssid) == 0)
    {
      memcpy((char*)node, (char*)pNode, sizeof(68));
      log_verbose(TAG, "GetRttBgNodeInList fill node from mRttBgNodeList");
      return true;
    }
    else
      ++it;
  }
  return false;
}

static inline void copy_mLowiMeasResult(LOWIMeasurementResult *in_meas,
              LOWIMeasurementResult *out_meas, bool isLast)
{

  out_meas->driverError = in_meas->driverError;
  out_meas->scanType = in_meas->scanType;
  out_meas->scanStatus = in_meas->scanStatus;
  out_meas->measurementTimestamp = in_meas->measurementTimestamp;
  out_meas->request = in_meas->request;
  out_meas->isWigigResult = in_meas->isWigigResult;

  log_info("", "out_meas scanType %u scanStatus %u request %pK out_meas %pK in_meas %pK isLast %d",
                 out_meas->scanType,
                 out_meas->scanStatus,
                 out_meas->request,
                 out_meas,
                 in_meas, isLast);

  log_info("", "%s: wifi nodes in mLowiMeasResult(%u)", __FUNCTION__,
              in_meas->scanMeasurements.getNumOfElements());
  // iterate over the wifi nodes in this set of results
  for (uint32 ii = 0; ii < in_meas->scanMeasurements.getNumOfElements(); ii++)
  {
     // get the wifi node to process
     LOWIScanMeasurement *node = new (std::nothrow)
          LOWIScanMeasurement(*(in_meas->scanMeasurements[ii]));
     log_info("", "before node %pK mac is in_meas->scanMeasurements[ii] %pK",
              node, in_meas->scanMeasurements[ii]);
     node->bssid.print();
     node->is_last_resp_for_peer = isLast;
     out_meas->scanMeasurements.push_back(node);
  }
  log_info("", "%s: wifi nodes in out_meas(%u)", __FUNCTION__,
              out_meas->scanMeasurements.getNumOfElements());

}

void LOWIRangingFSM::SendRttBgNodeRspToClient(LOWIMacAddress bssid,
                         LOWIScanMeasurement::eTargetStatus status,
                         bool last_rsp_for_peer)
{
  /* Send Result to User */
  mLowiMeasResult->scanStatus = LOWIResponse::SCAN_STATUS_SUCCESS;

  if (last_rsp_for_peer) {
    for (List<LOWINodeInfo*>::Iterator it = mRomeRangingReq.mRttBgNodeList.begin();
         it != mRomeRangingReq.mRttBgNodeList.end();)
    {
      LOWINodeInfo node, *pNode = *it;
      if (pNode->bssid.compareTo(bssid) == 0)
      {
        mRomeRangingReq.mRttBgNodeList.erase(it);
        break;
      }
    }
  }

  if (mRomeRangingReq.mRttBgNodeList.getSize() == 0)
  {
    log_info(TAG, "All mRttBgNodeList have been served ", __FUNCTION__);

    mRangingFsmContext.timeEnd  = ROME_FSM_TIMEOUT_FOREVER;
    mRangingFsmContext.curEvent = EVENT_RANGING_RESPONSE_TO_USER;
    mRangingFsmContext.curState = STATE_READY_AND_IDLE;

    SendResultToClient(mLowiMeasResult, true);
    mRangingReqRspInfo.lastResponseRecv = true;
    // Queue New request if present
    if (mRomeRangingReq.validRangingScan)
       SendRangingReqToFw();
  }
  else
  {
    log_info(TAG, "All mRttBgNodeList is not empty ", __FUNCTION__);
    LOWIMeasurementResult* result = new (std::nothrow) LOWIMeasurementResult;
    if (NULL == result)
    {
      log_error (TAG, "Unable to create the measurement result");
    }
    else
    {
      copy_mLowiMeasResult(mLowiMeasResult, result,
            mLowiMeasResult->scanMeasurements[0]->is_last_resp_for_peer);
      SendResultToClient(result, false);

      /** Reset some critical mLowiMeasResult parameters */
      mLowiMeasResult->scanMeasurements.flush();
      mLowiMeasResult->scanStatus = LOWIResponse::SCAN_STATUS_DRIVER_ERROR;
    }
  }
}

int LOWIRangingFSM::InitializeMeasResult(LOWIMeasurementResult* lowiMeasResult, LOWIResponse::eScanStatus status)
{
  log_verbose(TAG, "%s", __FUNCTION__);
  if (lowiMeasResult == NULL)
  {
    log_warning(TAG, "%s:NULL Meas Results Pointer!!!", __FUNCTION__);
    return -1;
  }

  lowiMeasResult->scanType = LOWIDiscoveryScanResponse::WLAN_SCAN_TYPE_UNKNOWN;
  lowiMeasResult->scanStatus = status;

  /* If Ranging Request comes before the Ranging Registration is done,
  we will not update mCurrReq with the new req, So better send the response to mNewreq directly */

  if ((lowiMeasResult->request == NULL) && (mNewReq != NULL))
  {
    if ((mNewReq->getRequestType() == LOWIRequest::RANGING_SCAN) ||
        (mNewReq->getRequestType() == LOWIRequest::PERIODIC_RANGING_SCAN) ||
        (mNewReq->getRequestType() == LOWIRequest::CANCEL_RANGING_SCAN))
    {
      lowiMeasResult->request = (LOWIRangingScanRequest*)mNewReq;
      SetLOWIRequest(NULL);
    }
    else
    {
      lowiMeasResult->request = (LOWIRangingScanRequest*)mCurReq;
    }
  }
  else
  {
    lowiMeasResult->request = (LOWIRangingScanRequest*)mCurReq;
  }
  return 0;
}

int LOWIRangingFSM::HandleConfigReq(LOWIRangingFSM* pFsmObj)
{
  int retVal = 0;
  log_verbose(TAG, "%s", __FUNCTION__);
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj->mLOWIRanging, "Invalid LOWIRanging object")

  log_verbose(TAG, "%s: Send %s request", __FUNCTION__, LOWIUtils::to_string(pFsmObj->eRequestType));

  pFsmObj->mRtsCtsTag = pFsmObj->getNewRtsCtsTag();

  if (pFsmObj->mCurReq && (pFsmObj->eRequestType == LOWIRequest::SET_LCI_INFORMATION))
  {
    retVal = pFsmObj->mLOWIRanging->RomeSendLCIConfiguration(pFsmObj->mRtsCtsTag,
                                                             (LOWISetLCILocationInformation*)pFsmObj->mCurReq);
  }
  else if (pFsmObj->mCurReq && (pFsmObj->eRequestType == LOWIRequest::SET_LCR_INFORMATION))
  {
    retVal = pFsmObj->mLOWIRanging->RomeSendLCRConfiguration(pFsmObj->mRtsCtsTag,
                                                             (LOWISetLCRLocationInformation*)pFsmObj->mCurReq);
  }
  else if (pFsmObj->mCurReq && (pFsmObj->eRequestType == LOWIRequest::SEND_LCI_REQUEST))
  {
    retVal = pFsmObj->mLOWIRanging->RomeSendLCIRequest(pFsmObj->mRtsCtsTag,
                                                       (LOWISendLCIRequest*)pFsmObj->mCurReq);
  }
  else if (pFsmObj->mCurReq && (pFsmObj->eRequestType == LOWIRequest::FTM_RANGE_REQ))
  {
    retVal = pFsmObj->mLOWIRanging->RomeSendFTMRR(pFsmObj->mRtsCtsTag,
                                                  (LOWIFTMRangingRequest*)pFsmObj->mCurReq);
  }
  else
  {
    retVal = -1;
  }

  if (retVal != -1) // Successfully sent Request to FW
  {
    pFsmObj->mRangingFsmContext.curState = STATE_PROCESSING_CONFIG_REQ;

    log_verbose(TAG, "%s: Setting timeout %u secs in the future",
                __FUNCTION__,
                SELECT_TIMEOUT_NORMAL);
    pFsmObj->mRangingFsmContext.timeEnd = get_time_rtc_ms() + (SELECT_TIMEOUT_NORMAL * 1000);
  }

  return retVal;
}

int LOWIRangingFSM::HandleCancelRangingError(LOWIRangingFSM* pFsmObj)
{
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj->mLOWIRanging, "Invalid LOWIRanging object")
  tANI_U32 status = 0;
  tANI_U32 cancel_reqid = 0;

  if (!pFsmObj->mLowiMeasResult)
  {
    log_warning(TAG, "mLowiMeasResult is Null");
    return -1;
  }

  /**
  if ( pFsmObj->mLOWIRanging->RomeExtractCancelRttRsp(pFsmObj->mRangingFsmData,
                                                   cancel_reqid, status) != 0)
  {
    log_warning(TAG, "%s:Failed - Cancel Meas Rsp Fwr status %u req_id %u",
                __FUNCTION__, status, cancel_reqid);
  }
  else
  {
    log_warning(TAG, "%s:Successfully  - Cancel Meas Rsp status %u req_id %u",
                __FUNCTION__, status, cancel_reqid);
  }
  */

  if (pFsmObj->mLowiMeasResult->request)
  {
    log_warning(TAG, "mLowiMeasResult->request:id %u type %u",
                   pFsmObj->mLowiMeasResult->request->getRequestId(),
                   pFsmObj->mLowiMeasResult->request->getRequestType());
  }

  // Clean up the state for Previous Ranging Request.
  pFsmObj->mRangingReqRspInfo.lastResponseRecv = TRUE;
  pFsmObj->mRangingReqRspInfo.expectedRspFromFw = 0;
  pFsmObj->mRangingReqRspInfo.nonAsapTargetPresent = FALSE;
  pFsmObj->mRangingReqRspInfo.totalRspFromFw = 0;
  pFsmObj->mRomeRangingReq.validRangingScan = FALSE;

  pFsmObj->mLowiMeasResult->scanStatus = LOWIResponse::SCAN_STATUS_SUCCESS;
  pFsmObj->mRangingFsmContext.curState = STATE_READY_AND_IDLE;
  pFsmObj->mRangingFsmContext.timeEnd    = ROME_FSM_TIMEOUT_FOREVER;
  pFsmObj->mRangingFsmContext.curEvent   = EVENT_CANCEL_RANGING_RSP;

  /* Send Result to User */
  pFsmObj->SendResultToClient(pFsmObj->mLowiMeasResult);
  pFsmObj->mLowiMeasResult = NULL;

  return 0;
}

int LOWIRangingFSM::HandleCancelRangingRsp(LOWIRangingFSM* pFsmObj)
{
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj->mLOWIRanging, "Invalid LOWIRanging object")
  tANI_U32 status = 0;
  tANI_U32 cancel_reqid = 0;

  if (!pFsmObj->mLowiMeasResult)
  {
    log_warning(TAG, "mLowiMeasResult is Null something went wrong");
    return -1;
  }

  if ( pFsmObj->mLOWIRanging->RomeExtractCancelRttRsp(pFsmObj->mRangingFsmData,
                                                   cancel_reqid, status) != 0)
  {
    log_warning(TAG, "%s:Failed - Cancel Meas Rsp Fwr status %u req_id %u",
                __FUNCTION__, status, cancel_reqid);
  }
  else
  {
    log_warning(TAG, "%s:Successfully  - Cancel Meas Rsp status %u req_id %u",
                __FUNCTION__, status, cancel_reqid);
  }

  if (pFsmObj->mLowiMeasResult->request)
  {
    log_warning(TAG, "mLowiMeasResult->request:id %u type %u",
                   pFsmObj->mLowiMeasResult->request->getRequestId(),
                   pFsmObj->mLowiMeasResult->request->getRequestType());
  }

  // Clean up the state for Previous Ranging Request.
  pFsmObj->mRangingReqRspInfo.lastResponseRecv = TRUE;
  pFsmObj->mRangingReqRspInfo.nonAsapTargetPresent = FALSE;
  pFsmObj->mRangingReqRspInfo.expectedRspFromFw = 0;
  pFsmObj->mRangingReqRspInfo.totalRspFromFw = 0;
  pFsmObj->mRomeRangingReq.validRangingScan = FALSE;

  pFsmObj->mLowiMeasResult->scanStatus = LOWIResponse::SCAN_STATUS_SUCCESS;
  pFsmObj->mRangingFsmContext.curState = STATE_READY_AND_IDLE;
  pFsmObj->mRangingFsmContext.timeEnd    = ROME_FSM_TIMEOUT_FOREVER;
  pFsmObj->mRangingFsmContext.curEvent   = EVENT_CANCEL_RANGING_RSP;

  /* Send Result to User */
  pFsmObj->SendResultToClient(pFsmObj->mLowiMeasResult);
  pFsmObj->mLowiMeasResult = NULL;

  return 0;
}

int LOWIRangingFSM::HandleCancelRangingReq(LOWIRangingFSM* pFsmObj)
{
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj->mLOWIRanging, "Invalid Ranging Obj")
  log_debug(TAG, "%s", __FUNCTION__);

  // Free current lowi results as we must be in
  // STATE_PROCESSING_RANGING_REQ if we reached here
  // Clean up the stored measurement result.
  // Make sure the measurement result vector is empty
  if (pFsmObj->mLowiMeasResult)
  {
    for (vector <LOWIScanMeasurement*>::Iterator
           it = pFsmObj->mLowiMeasResult->scanMeasurements.begin();
           it != pFsmObj->mLowiMeasResult->scanMeasurements.end(); it++)
    {
       delete (*it);
    }
    pFsmObj->mLowiMeasResult->scanMeasurements.flush();
    delete pFsmObj->mLowiMeasResult;
    pFsmObj->mLowiMeasResult = NULL;
  }

  pFsmObj->mLowiMeasResult = new (std::nothrow) LOWIMeasurementResult;
  if (pFsmObj->mLowiMeasResult == NULL)
  {
    log_debug(TAG, "%s Warning: failed to allocate memory ", __FUNCTION__);
    return -1;
  }

  pFsmObj->mLowiMeasResult->scanType =
      LOWIDiscoveryScanResponse::WLAN_SCAN_TYPE_UNKNOWN;
  pFsmObj->mLowiMeasResult->scanStatus =
      LOWIDiscoveryScanResponse::SCAN_STATUS_DRIVER_ERROR;
  if (pFsmObj->mCurReq)
  {
    pFsmObj->mLowiMeasResult->request = pFsmObj->mCurReq;
    pFsmObj->SetLOWIRequest(NULL);
    pFsmObj->eRequestType = LOWIRequest::CANCEL_RANGING_SCAN;
    // Clean up the state for Previous Ranging Request.
    pFsmObj->mRangingReqRspInfo.lastResponseRecv = TRUE;
    pFsmObj->mRangingReqRspInfo.expectedRspFromFw = 0;
    pFsmObj->mRangingReqRspInfo.nonAsapTargetPresent = FALSE;
    pFsmObj->mRangingReqRspInfo.totalRspFromFw = 0;
    log_debug(TAG, "FsmObj->eRequestType %u", pFsmObj->eRequestType);
  }
  else
  {
    log_debug(TAG, "Null pFsmObj->mCurReq ");
  }

  if (pFsmObj->mLOWIRanging->RomeSendCancelRttReq(pFsmObj->mRtsCtsTag) < 0)
  {
    log_info(TAG, "%s: Failed -To  SendCancelRangingReq", __FUNCTION__);
    pFsmObj->mLowiMeasResult->scanStatus   =
        LOWIResponse::SCAN_STATUS_INTERNAL_ERROR;
    pFsmObj->mRangingFsmContext.timeEnd    = ROME_FSM_TIMEOUT_FOREVER;
    pFsmObj->mRangingFsmContext.curEvent   = EVENT_CANCEL_RANGING_RSP;
    pFsmObj->mRangingFsmContext.curState   = STATE_READY_AND_IDLE;
    pFsmObj->SendResultToClient(pFsmObj->mLowiMeasResult);
    pFsmObj->mLowiMeasResult = NULL;
    return -1;
  }
  else
  {
    // Send Rsp on either Time out or Cancel Meas Rsp from Fwr
    pFsmObj->mRangingFsmContext.curState = STATE_PROCESSING_CANCEL_RANGING_REQ;
    log_verbose(TAG, "%s: Setting timeout %u secs in the future",
                __FUNCTION__,
                SELECT_TIMEOUT_NORMAL);
    pFsmObj->mRangingFsmContext.timeEnd = get_time_rtc_ms() + (SELECT_TIMEOUT_NORMAL * 1000);
    pFsmObj->mRangingReqRspInfo.lastResponseRecv = FALSE;
    pFsmObj->mRangingReqRspInfo.expectedRspFromFw = RESP_REPORT_TYPE2;
    return 0;
  }
}

int LOWIRangingFSM::HandleRangingReq(LOWIRangingFSM* pFsmObj)
{
  FSM_ENTER()
  int retVal = -1;
  do
  {
    LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")
    uint64 rangingRequestTimestamp = lowi_get_time_from_boot();
    // if RTT responder is still enabled ignore the incoming ranging request
    if((pFsmObj->mRTTResponderExpiryTimeStamp > 0) &&
       (rangingRequestTimestamp <= pFsmObj->mRTTResponderExpiryTimeStamp))
    {
      log_debug(TAG, "%s: Responder mode is enabled", __FUNCTION__);
      IgnoreRangingReq(pFsmObj);
      break;
    }

    if(pFsmObj->PrepareRangingRequest() != 0)
    {
      log_debug(TAG, "%s: Failed - Prepare Ranging Request", __FUNCTION__);
      IgnoreRangingReq(pFsmObj);
      break;
    }
    if (pFsmObj->ValidateRangingRequest() != 0)
    {
      log_debug(TAG, "%s: Ranging Request Not Valid, Aborting request", __FUNCTION__);
      pFsmObj->RejectNewRangingReq();
      break;
    }
    retVal = pFsmObj->SendRangingReq();
    if(retVal < 0)
    {
      log_debug(TAG, "%s: Failed - Send Ranging Request to FW", __FUNCTION__);
      IgnoreRangingReq(pFsmObj);
      break;
    }
    else if (retVal > 0)
    {
      retVal = pFsmObj->SendReqToFwOrRespToUser();
      break;
    }
    pFsmObj->mRangingFsmContext.curState = STATE_PROCESSING_RANGING_REQ;

    uint64 timeOut = (pFsmObj->mRangingReqRspInfo.nonAsapTargetPresent ?
                      SELECT_TIMEOUT_NON_ASAP_TARGET :
                      SELECT_TIMEOUT_NORMAL);
    log_verbose(TAG, "%s: Setting timeout %u secs in the future", __FUNCTION__, timeOut);
    pFsmObj->mRangingFsmContext.timeEnd = get_time_rtc_ms() + (timeOut * 1000);
  }
  while (0);
  FSM_EXIT()
  return 0;
}

int LOWIRangingFSM::HandleRangingMeas(LOWIRangingFSM* pFsmObj)
{
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")
  bool valid_state = 0;
  int reqId;
  bool rxAllChainSupport = false;
  if(pFsmObj->mRangingFsmContext.curState == STATE_READY_AND_IDLE ||
     pFsmObj->mRangingFsmContext.curState == STATE_PROCESSING_RANGING_REQ)
  {
    valid_state = 1;
  }

  if (valid_state)
  {
    reqId = pFsmObj->ExtractReqId();
  }
  else
  {
    log_info(TAG, "%s:Failed due to incorrect current state", __FUNCTION__);
    return -1;
  }

  rxAllChainSupport = pFsmObj->mRomeRttCapabilities.rx_all_chain_reporting_support;

  if((reqId == 0 && pFsmObj->mRangingFsmContext.curState == STATE_READY_AND_IDLE) ||
     pFsmObj->mRangingFsmContext.curState == STATE_PROCESSING_RANGING_REQ)
  {
    if(pFsmObj->ProcessRangingMeas(rxAllChainSupport) != 0)
    {
      log_info(TAG, "%s: Failed - Process Ranging Measurements", __FUNCTION__);
      pFsmObj->ProcessRttRejected();
    }
    log_info(TAG, "%s: Current state is %s", __FUNCTION__, LOWIStrings::to_string(pFsmObj->mRangingFsmContext.curState));
    if(pFsmObj->mRangingFsmContext.curState == STATE_PROCESSING_RANGING_REQ)
    {
      pFsmObj->SendReqToFwOrRespToUser();
    }
  }
  else
  {
    log_info(TAG, "%s:Failed due to incorrect events", __FUNCTION__);
    return -1;
  }
  return 0;
}

int LOWIRangingFSM::RejectNewRangingReq()
{
  int retVal = 0;
  log_debug(TAG, "%s: Reject newly arrived ranging request", __FUNCTION__);
  LOWIMeasurementResult* result = new (std::nothrow) LOWIMeasurementResult;
  if (result == NULL)
  {
    log_debug(TAG, "%s - memory allocation faliure - Ranging Measurment Results", __FUNCTION__);
    return -1;
  }
  retVal = InitializeMeasResult(result, LOWIResponse::SCAN_STATUS_INVALID_REQ);
  if (retVal != 0)
  {
    log_info(TAG, "%s: initialize LOWI Meas Results failed", __FUNCTION__);
  }
  else
  {
    mRangingFsmContext.curEvent = EVENT_RANGING_RESPONSE_TO_USER;
    /* Set New request to NULL */
    SetLOWIRequest(NULL);
    /* Send Result to User */
    SendResultToClient(result);
  }
  return retVal;
}

int LOWIRangingFSM::SendCurrentResultToUser(LOWIResponse::eScanStatus status)
{
  /* Done with the Ranging Request - Respond to User */
  mRangingFsmContext.timeEnd = ROME_FSM_TIMEOUT_FOREVER;
  mRangingFsmContext.curEvent = EVENT_RANGING_RESPONSE_TO_USER;
  mRangingFsmContext.curState = STATE_READY_AND_IDLE;
  /* Send Result to User */
  if (mLowiMeasResult)
  {
    mLowiMeasResult->scanStatus = status;
    SendResultToClient(mLowiMeasResult);
  }
  else
  {
    IgnoreRangingReq(this);
  }

  return 0;
}

int LOWIRangingFSM::SendReqToFwOrRespToUser()
{
  /* check to see if all expected Responses from FW have arrived */
  mRangingReqRspInfo.lastResponseRecv = (mRangingReqRspInfo.totalRspFromFw == mRangingReqRspInfo.expectedRspFromFw)? TRUE : FALSE;

  /* All Expected responses have arrived from FW */
  if (mRangingReqRspInfo.lastResponseRecv)
  {
    /* Send another Ranging Request to FW if the Ranging request is still valid */
    if (mRomeRangingReq.validRangingScan)
    {
      if(SendRangingReq() != 0)
      {
        log_warning(TAG, "%s:Send Ranging Request - failed, Aborting current LOWI request", __FUNCTION__);
        /* Done with the Ranging Request - Respond to User */
        (mLowiMeasResult)->scanStatus = LOWIResponse::SCAN_STATUS_SUCCESS;
        mRangingFsmContext.timeEnd = ROME_FSM_TIMEOUT_FOREVER;
        mRangingFsmContext.curEvent = EVENT_RANGING_RESPONSE_TO_USER;
        mRangingFsmContext.curState = STATE_READY_AND_IDLE;
        /* Send Result to User */
        SendResultToClient(mLowiMeasResult);
        return -1;
      }
      uint64 timeOut = (mRangingReqRspInfo.nonAsapTargetPresent ?
                        SELECT_TIMEOUT_NON_ASAP_TARGET :
                        SELECT_TIMEOUT_NORMAL);
      log_verbose(TAG, "%s: Setting timeout %u secs in the future",
                  __FUNCTION__,
                  timeOut);
      mRangingFsmContext.timeEnd = get_time_rtc_ms() + (timeOut * 1000);
    }
    else /* Other wise send the result to User */
    {
      /* Done with the Ranging Request - Respond to User */
      mRangingFsmContext.timeEnd = ROME_FSM_TIMEOUT_FOREVER;
      mRangingFsmContext.curEvent = EVENT_RANGING_RESPONSE_TO_USER;
      mRangingFsmContext.curState = STATE_READY_AND_IDLE;
      /* Send Result to User */
      if (NULL != mLowiMeasResult)
      {
        log_info(TAG, "%s: All ranging meas completed.Send response to client", __FUNCTION__);
        (mLowiMeasResult)->scanStatus = LOWIResponse::SCAN_STATUS_SUCCESS;
        SendResultToClient(mLowiMeasResult);
      }
    }
  }
  else /* Still expecting responses from FW */
  {
    // Do Nothing
  }

  return 0;
}

int LOWIRangingFSM::HandleConfigRspOrErrorMsg(LOWIRangingFSM* pFsmObj)
{
  log_verbose(TAG, "%s", __FUNCTION__);
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")

  int retVal = HandleRangingErrorMsg(pFsmObj);

  if (pFsmObj->mRangingReqRspInfo.rangingConfigStatus == CONFIG_SUCCESS ||
      pFsmObj->mRangingReqRspInfo.rangingConfigStatus == CONFIG_FAIL)
  {
    log_verbose(TAG, "%s Response for Configuration request", __FUNCTION__);
    pFsmObj->mLowiMeasResult = new (std::nothrow) LOWIMeasurementResult;
    if (pFsmObj->mLowiMeasResult == NULL)
    {
      log_debug(TAG, "%s:Memory allocation faliure - Ranging Measurment Results", __FUNCTION__);
      return -1;
    }
    pFsmObj->mLowiMeasResult->scanType = LOWIDiscoveryScanResponse::WLAN_SCAN_TYPE_UNKNOWN;
    if (pFsmObj->mRangingReqRspInfo.rangingConfigStatus == CONFIG_SUCCESS)
    {
      log_verbose(TAG, "%s: Response for Config request: SUCCESS", __FUNCTION__);
      pFsmObj->mLowiMeasResult->scanStatus = LOWIResponse::SCAN_STATUS_SUCCESS;
    }
    else
    {
      log_verbose(TAG, "%s: Response for Config request: FAILURE", __FUNCTION__);
      pFsmObj->mLowiMeasResult->scanStatus = LOWIResponse::SCAN_STATUS_DRIVER_ERROR;
    }
    pFsmObj->mLowiMeasResult->request = pFsmObj->mCurReq;

    pFsmObj->SendResultToClient(pFsmObj->mLowiMeasResult);

    pFsmObj->mCurReq = NULL;

    pFsmObj->mRangingFsmContext.timeEnd  = ROME_FSM_TIMEOUT_FOREVER;
    pFsmObj->mRangingFsmContext.curEvent = EVENT_CONFIG_RESPONSE_TO_USER;
    pFsmObj->mRangingFsmContext.curState = STATE_READY_AND_IDLE;
  }
  return retVal;
}

int LOWIRangingFSM::HandleRangingErrorMsg(LOWIRangingFSM* pFsmObj)
{
  int retVal = 0;
  tANI_U32 errorCode;
  tANI_U8 bssid[BSSID_LEN + 2];
  bool isLastMeas = false;

  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj->mLOWIRanging, "Invalid LOWIRanging object")

  if (pFsmObj->mLOWIRanging->RomeExtractRangingError(
      pFsmObj->mRangingFsmData, &errorCode, bssid, &isLastMeas) != 0)
  {
    log_info(TAG, "%s: Extract Err Code faliure from Ranging Err Msg", __FUNCTION__);
    return -1;
  }

  if (errorCode > WMI_RTT_REJECT_MAX)
  {
    log_debug(TAG, "%s:Received invalid errCode from FW: %s",__FUNCTION__,
              LOWIStrings::to_string((WMI_RTT_STATUS_INDICATOR)errorCode));
    return -1;
  }
  log_debug(TAG, "%s:Received Error(%s) isLastMeas %d", __FUNCTION__,
            LOWIStrings::to_string((WMI_RTT_STATUS_INDICATOR)errorCode),
            isLastMeas);
  switch (errorCode)
  {
    case RTT_COMMAND_HEADER_ERROR:       //rtt cmd header parsing error --terminate
    case RTT_MODULE_BUSY:                //rtt no resource -- terminate
    case RTT_NO_RESOURCE:                //Any Resource allocate failure
    case RTT_CHAN_SWITCH_ERROR:          //channel swicth failed
    case RTT_REPORT_TYPE2_MIX:           //do not allow report type2 mix with type 0, 1
    case RTT_COMMAND_ERROR:              //rtt body parsing error -- skip current STA REQ
    case RTT_VOIP_IN_PROGRESS:           // voip call in progress, reject the request
    case RTT_RESPONDER_SESSION_TERMINATED: // Active responder session is terminated
    case RTT_RANGING_PEER_ALLOC_FAILURE: // peer alloc failure for 11az ranging
    case RTT_PASN_KEY_INSTALL_FAILURE:   // PASN key install failure
    case RTT_TOO_MANY_CHAN_CONFIG:
    case RTT_11AZ_MAX_TIMER_EXPIRE:
    case RTT_PASN_SECURITY_NOT_SUPPORTED: // Peer doesn't support security ranging
    {
      // Entire request Rejected
      unsigned int totalRspFromFw = pFsmObj->mRangingReqRspInfo.totalRspFromFw;
      unsigned int expectedRspFromFw= pFsmObj->mRangingReqRspInfo.expectedRspFromFw;
      log_debug(TAG, "totalRspFromFw %d expectedRspFromFw %d",
                totalRspFromFw, expectedRspFromFw);
      pFsmObj->ProcessRttRejected();
      if (!isLastMeas)
      {
        pFsmObj->mRangingReqRspInfo.totalRspFromFw = totalRspFromFw;
        pFsmObj->mRangingReqRspInfo.expectedRspFromFw = expectedRspFromFw;
      }
      break;
    }
    case RTT_TOO_MANY_STA:               //STA exceed the support limit -- only serve the first n STA
    {
      // Do Nothing
      if (pFsmObj->mRangingReqRspInfo.reportType == RTT_REPORT_PER_FRAME_WITH_CFR ||
          pFsmObj->mRangingReqRspInfo.reportType == RTT_REPORT_PER_FRAME_NO_CFR)
      {
        /* For Report Type 0/1 Reduce the expected number of measurements
           to MAX supported */
        pFsmObj->mRangingReqRspInfo.expectedRspFromFw = (MAX_BSSIDS_TO_SCAN * pFsmObj->mRangingReqRspInfo.measPerTarget);
      }
      else if (pFsmObj->mRangingReqRspInfo.reportType == RTT_AGGREGATE_REPORT_NON_CFR)
      {
        LOWINodeInfo node;
        if (pFsmObj->GetRttBgNodeInList(bssid, &node))
        {
          pFsmObj->AddTargetToResult(node, LOWIScanMeasurement::LOWI_TARGET_STATUS_SUCCESS, true);
          pFsmObj->SendReqToFwOrRespToUser();
          log_info(TAG,"%s Sent error(%s) for BG node", __FUNCTION__,
                   LOWIStrings::to_string((WMI_RTT_STATUS_INDICATOR)errorCode));
          return 0;
        }
      }
      break;
    }
    case RTT_VDEV_ERROR:                 //can not find vdev with vdev ID -- skip current STA REQ
    case RTT_FRAME_TYPE_NOSUPPORT:       //We do not support RTT measurement with this type of frame
    case RTT_TMR_TRANS_ERROR:            //TMR trans error, this dest peer will be skipped
    case RTT_TM_TIMER_EXPIRE:            //wait for first TM timer expire -- terminate current STA measurement
    case WMI_RTT_REJECT_MAX:             // Temporarily this will be used to indicate FTMR rejected by peer
    case RTT_DFS_CHANNEL_QUIET:          // Ranging request send with DFS channel for a BSSID. BSSID will be skipped.
    case RTT_NAN_REQUEST_FAILURE:        // NAN ranging request failure
    case RTT_NAN_NEGOTIATION_FAILURE:    // NAN Ranging negotiation failure
    case RTT_NAN_DATA_PATH_ACTIVE:       // concurrency not supported (NDP+RTT)
    case RTT_UNSUPPORTED_CONCURRENCY:    // When multiple peers of diff. vdev type received in single OEM request
    {
      // For Report Type 0/1 - No measurement reports for the specifed BSSID
      // For report Type 2   - No measurements for specified BSSID in Aggregated Report
      log_debug(TAG, "%s: Due to error(%s), skip bssid(" LOWI_MACADDR_FMT ") ",__FUNCTION__,
               LOWIStrings::to_string((WMI_RTT_STATUS_INDICATOR)errorCode), LOWI_MACADDR(bssid));

      if (pFsmObj->mRangingReqRspInfo.reportType == RTT_REPORT_PER_FRAME_WITH_CFR ||
          pFsmObj->mRangingReqRspInfo.reportType == RTT_REPORT_PER_FRAME_NO_CFR)
      {
        /* For Report Type 0/1 Reduce the expected number of measurements
           by subtracting number of measurements/Target */
        pFsmObj->mRangingReqRspInfo.expectedRspFromFw -= pFsmObj->mRangingReqRspInfo.measPerTarget;
      }
      else if (pFsmObj->mRangingReqRspInfo.reportType == RTT_AGGREGATE_REPORT_NON_CFR)
      {
        LOWINodeInfo node;
        if (pFsmObj->GetRttBgNodeInList(bssid, &node))
        {
          pFsmObj->AddTargetToResult(node, LOWIScanMeasurement::LOWI_TARGET_STATUS_SUCCESS, true);
          pFsmObj->SendReqToFwOrRespToUser();
          log_info(TAG,"%s Sent error(%s) for BG node", __FUNCTION__,
                   LOWIStrings::to_string((WMI_RTT_STATUS_INDICATOR)errorCode));
          return 0;
        }
      }
      break;
    }
    case RTT_TRANSIMISSION_ERROR:        //Tx failure -- continiue and measure number-- Applicable only for RTT V2
    case RTT_NO_REPORT_BAD_CFR_TOKEN:    //V3 only. If both CFR and Token mismatch, do not report
    case RTT_NO_REPORT_FIRST_TM_BAD_CFR: //For First TM, if CFR is bad, then do not report
    {
      // For Report Type 0/1 - No measurement report for current Frame for specifed BSSID
      // For report Type 2   - No measurement for 1 frame in aggregated report for specifed BSSID
      if (pFsmObj->mRangingReqRspInfo.reportType == RTT_REPORT_PER_FRAME_WITH_CFR ||
          pFsmObj->mRangingReqRspInfo.reportType == RTT_REPORT_PER_FRAME_NO_CFR)
      {
        /* For Report Type 0/1 Reduce the expected number of measurements by 1 */
        pFsmObj->mRangingReqRspInfo.expectedRspFromFw--;
      }
      else if (pFsmObj->mRangingReqRspInfo.reportType == RTT_AGGREGATE_REPORT_NON_CFR)
      {
        LOWINodeInfo node;
        if (pFsmObj->GetRttBgNodeInList(bssid, &node))
        {
          pFsmObj->AddTargetToResult(node, LOWIScanMeasurement::LOWI_TARGET_STATUS_SUCCESS, true);
          pFsmObj->SendReqToFwOrRespToUser();
          log_info(TAG,"%s Sent error(%s) for BG node", __FUNCTION__,
                   LOWIStrings::to_string((WMI_RTT_STATUS_INDICATOR)errorCode));
          return 0;
        }
      }
      break;
    }
    case RTT_TIMER_EXPIRE:               //Whole RTT measurement timer expire -- terminate current STA measurement
    {
      // For Report Type 0/1 - No MORE measurement reports for the specifed BSSID
      // For report Type 2   - Less than requested measurements for specified BSSID in Aggregated Report
      if (pFsmObj->mRangingReqRspInfo.reportType == RTT_REPORT_PER_FRAME_WITH_CFR ||
          pFsmObj->mRangingReqRspInfo.reportType == RTT_REPORT_PER_FRAME_NO_CFR)
      {
        /* For Report Type 0/1 Reduce the expected number of measurements
           by subtracting number of measurements remaining for the current BSSID */
        pFsmObj->mRangingReqRspInfo.expectedRspFromFw -= (pFsmObj->mRangingReqRspInfo.measPerTarget - (pFsmObj->mRangingReqRspInfo.totalRspFromFw % pFsmObj->mRangingReqRspInfo.measPerTarget));
      }
      else if (pFsmObj->mRangingReqRspInfo.reportType == RTT_AGGREGATE_REPORT_NON_CFR)
      {
        LOWINodeInfo node;
        if (pFsmObj->GetRttBgNodeInList(bssid, &node))
        {
          pFsmObj->AddTargetToResult(node, LOWIScanMeasurement::LOWI_TARGET_STATUS_SUCCESS, true);
          pFsmObj->SendReqToFwOrRespToUser();
          log_info(TAG,"%s Sent RTT TIMER expirement for BG node", __FUNCTION__);
          return 0;
        }
        else
          log_info(TAG, "Cd not find in RttBgNodeInList");
      }
      break;
    }
    case RTT_LCI_CFG_OK:                 //LCI Configuration OK
    case RTT_LCR_CFG_OK:                 //LCR Configuration OK
    case RTT_CFG_ERROR:                  //Bad LCI or LCR Configuration Request
    case RTT_LCI_REQ_OK:                 //Where are you request OK
    case RTT_FTMRR_OK:                   //FTMRR OK
    {
      if (pFsmObj->mRangingFsmContext.curState == STATE_PROCESSING_CONFIG_REQ)
      {
        pFsmObj->mRangingReqRspInfo.rangingConfigStatus =
          (errorCode == RTT_CFG_ERROR) ? CONFIG_FAIL : CONFIG_SUCCESS;
      }
      else
      {
        // Do Nothing
        log_info(TAG,"%s Received unexpected error code(%s)", __FUNCTION__,
                 LOWIStrings::to_string((WMI_RTT_STATUS_INDICATOR)errorCode));
      }
      break;
    }
    case RTT_AP_INITITAOR_MODE_RUNNING:  // When initiator session is active, reject the responder session request from peer initiator
    {
        //Do Nothing
    }
    default:
    {
      log_debug(TAG, "%s: invalid error(%d) -- %s arrived!, not expected",
                __FUNCTION__, errorCode, LOWIStrings::to_string((WMI_RTT_STATUS_INDICATOR)errorCode));
      break;
    }
  }

  if (errorCode != RTT_LCI_CFG_OK &&
      errorCode != RTT_LCR_CFG_OK &&
      errorCode != RTT_CFG_ERROR  &&
      errorCode != RTT_LCI_REQ_OK &&
      errorCode != RTT_FTMRR_OK )
  {
    /* Add Skipped Target to List */
    pFsmObj->mLOWIRanging->RomeAddSkippedTargetToList(errorCode, bssid);
    retVal = pFsmObj->SendReqToFwOrRespToUser();
  }

  return retVal;
}

int LOWIRangingFSM::HandleP2PInfo(LOWIRangingFSM* pFsmObj)
{
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj->mLOWIRanging, "Invalid LOWIRanging object")

  if(pFsmObj->mLOWIRanging->RomeExtractP2PInfo(pFsmObj->mRangingFsmData) != 0)
  {
    log_debug(TAG, "%s:Extract P2P info faliure", __FUNCTION__);
    return -1;
  }
  return 0;
}

int LOWIRangingFSM::HandleNlTimeout(LOWIRangingFSM* pFsmObj)
{
  LOWI_FSM_EXIT_IF_NULL_POINTER(pFsmObj, "Invalid FSM object")

  log_verbose(TAG, "%s", __FUNCTION__);
  if(pFsmObj->mCurReq)
  {
    if((pFsmObj->eRequestType == LOWIRequest::RANGING_SCAN) ||
       (pFsmObj->eRequestType == LOWIRequest::PERIODIC_RANGING_SCAN))
     {
  pFsmObj->ProcessRttRejected();
  if(pFsmObj->SendReqToFwOrRespToUser() != 0)
  {
    log_debug(TAG, "%s: Failed to Respond to User!!", __FUNCTION__);
    return -1;
       }
     }
     else if (pFsmObj->eRequestType == LOWIRequest::CANCEL_RANGING_SCAN)
     {
       pFsmObj->mLowiMeasResult->request = pFsmObj->mCurReq;
       pFsmObj->mLowiMeasResult->scanStatus   =
           LOWIResponse::SCAN_STATUS_DRIVER_TIMEOUT;
       pFsmObj->mRangingFsmContext.timeEnd    = ROME_FSM_TIMEOUT_FOREVER;
       pFsmObj->mRangingFsmContext.curEvent   = EVENT_CANCEL_RANGING_RSP;
       pFsmObj->mRangingFsmContext.curState   = STATE_READY_AND_IDLE;
       pFsmObj->SendResultToClient(pFsmObj->mLowiMeasResult);
       pFsmObj->mLowiMeasResult = NULL;
     }
     else
     {
       if( pFsmObj->mLowiMeasResult == NULL)
       {
         pFsmObj->mLowiMeasResult = new (std::nothrow) LOWIMeasurementResult;
         if (pFsmObj->mLowiMeasResult == NULL)
         {
           log_debug(TAG, "%s Warning: failed to allocate memory", __FUNCTION__);
           return -1;
         }
       }
       pFsmObj->mRTTResponderExpiryTimeStamp = 0;
       pFsmObj->mLowiMeasResult->request = pFsmObj->mCurReq;
       pFsmObj->mLowiMeasResult->scanStatus   = LOWIResponse::SCAN_STATUS_DRIVER_TIMEOUT;
       pFsmObj->mRangingFsmContext.timeEnd    = ROME_FSM_TIMEOUT_FOREVER;
       pFsmObj->mRangingFsmContext.curEvent   = EVENT_CONFIG_RESPONSE_TO_USER;
       pFsmObj->mRangingFsmContext.curState   = STATE_READY_AND_IDLE;
       pFsmObj->SendResultToClient(pFsmObj->mLowiMeasResult);
     }
  }
  return 0;
}

void LOWIRangingFSM::ProcessRttRejected(LOWIScanMeasurement::eTargetStatus
                                   status)
{
  /* We will get no more Responses from FW */
  mRangingReqRspInfo.totalRspFromFw = 0;
  mRangingReqRspInfo.expectedRspFromFw = 0;
  AddDummyMeas(status);
}

int LOWIRangingFSM::SetLOWIRequest(LOWIRequest* pReq)
{
  mNewReq = pReq;
  return 0;
}

bool LOWIRangingFSM::GetWmiChannelInfo(tANI_U32 freq, ChannelInfo& chInfo)
{
  if (!LOWIUtils::isValidCenFreq(freq))
  {
    log_warning(TAG, "%s: freq is invalid (%u)", __FUNCTION__, freq);
    return false;
  }

  auto it = mChannelInfoArray.find(freq);
  if (it == mChannelInfoArray.end())
  {
    log_warning(TAG, "Not able find the freq:%u in map", freq);
    return false;
  }
  else
  {
    chInfo = it->second;
    return true;
  }
}

bool LOWIRangingFSM::NewRequestPending()
{
  if (mNewReq != NULL)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool LOWIRangingFSM::ProcessingRangingRequest()
{
  return mRomeRangingReq.validRangingScan;
}

int LOWIRangingFSM::SendResultToClient(LOWIMeasurementResult* result,
                                                  bool del_curr_req)
{
  int retVal = 0;
  /* Send Result through listener */
  if (mListener != NULL)
  {
    log_verbose(TAG, "%s: Total Number of APs in result: %u", __FUNCTION__,
                result->scanMeasurements.getNumOfElements());

    mListener->scanResultsReceived(result);
  }
  else
  {
    log_debug(TAG, "%s: Invalid Listener, send back Result failed", __FUNCTION__);
    delete result;
    retVal = -1;
  }

  /* Reset the request pointer */
  if (!del_curr_req)
    log_debug(TAG, "%s: keeping FSM  current request", __FUNCTION__);
  else
    mCurReq = NULL;

  return retVal;
}

bool LOWIRangingFSM::IsReady()
{
  if (mRangingFsmContext.curState == STATE_READY_AND_IDLE)
    return TRUE;
  else
    return FALSE;
}

#define CAPABILITY_RESP_TIMEOUT_COUNT 3000
bool LOWIRangingFSM::SendRangingCap(std::string interface)
{
    int count = 0;

    RangingCapRespRecevied = false;
    mRangingFsmContext.timeEnd = get_time_rtc_ms() + (SELECT_TIMEOUT_NORMAL * 1000);
    mRangingFsmContext.curState = STATE_WAITING_FOR_RANGING_CAP;

    if(mLOWIRanging->RomeSendRangingCapReq(interface) != 0)
    {
        log_warning(TAG, "%s:Failed - to send Ranging Capabilities request to FW", __FUNCTION__);
        return false;
    }

    do {
        usleep (10000);
        count++;
    } while (!RangingCapRespRecevied && (count < CAPABILITY_RESP_TIMEOUT_COUNT));

    if (!RangingCapRespRecevied) {
        log_warning(TAG, "%s:Failed - get Ranging Capabilities from FW", __FUNCTION__);
        return false;
    }

    return true;
}

LOWI_RangingCapabilities LOWIRangingFSM::GetRangingCap()
{
  FSM_ENTER()
  LOWI_RangingCapabilities lowiRangingCap;
  memset(&lowiRangingCap, 0, sizeof(LOWI_RangingCapabilities));

  lowiRangingCap.oneSidedSupported = true;
  lowiRangingCap.dualSidedSupported11mc = true;
  lowiRangingCap.bwSupport = RTT_CAP_BW_80;
  lowiRangingCap.preambleSupport = (CAP_PREAMBLE_LEGACY |
                                    CAP_PREAMBLE_HT     |
                                    CAP_PREAMBLE_VHT    |
                                    CAP_PREAMBLE_HE);

  if (mRangingFsmContext.curState == STATE_READY_AND_IDLE)
  {
    lowiRangingCap.oneSidedSupported = (CAP_SINGLE_SIDED_RTT_SUPPORTED(mRomeRttCapabilities.rangingTypeMask)) ? TRUE : FALSE;
    lowiRangingCap.dualSidedSupported11mc = (CAP_11MC_DOUBLE_SIDED_RTT_SUPPORTED(mRomeRttCapabilities.rangingTypeMask)) ? TRUE : FALSE;
    lowiRangingCap.bwSupport = mRomeRttCapabilities.maxBwAllowed;
    lowiRangingCap.rxAllChainSupport = mRomeRttCapabilities.rx_all_chain_reporting_support;
    lowiRangingCap.preambleSupport = mRomeRttCapabilities.preambleSupportedMask;
    if (mRomeRttCapabilities.fwMultiBurstSupport)
    {
      lowiRangingCap.supportedCaps |= LOWI_FW_MULTI_BURST_SUPPORTED;
    }
    else
    {
      lowiRangingCap.supportedCaps &= ~LOWI_FW_MULTI_BURST_SUPPORTED;
    }
  }

  uint8 tmrCaps = mRomeRttCapabilities.capTmrFeatSupportMask;
  lowiRangingCap.isNtbSupp = (CAP_IS_TMR_NTB_SUPPORTED(tmrCaps)) ? TRUE : FALSE;
  lowiRangingCap.isTbSupp  = (CAP_IS_TMR_TB_SUPPORTED(tmrCaps)) ? TRUE : FALSE;
  lowiRangingCap.isMacSecSupp  = (CAP_IS_TMR_MAC_SEC_SUPPORTED(tmrCaps)) ? TRUE : FALSE;
  lowiRangingCap.isPhySecSupp  = (CAP_IS_TMR_PHY_SEC_SUPPORTED(tmrCaps)) ? TRUE : FALSE;

  FSM_EXIT()
  return lowiRangingCap;
}

unsigned int LOWIRangingFSM::ExpectedMeasSizeForTarget(unsigned int numMeas, uint32 ftmRangingParams)
{
  unsigned int maxLen = 0;
  bool lci = false, lcr = false;

  lci = FTM_GET_LCI_REQ(ftmRangingParams);
  lcr = FTM_GET_LOC_CIVIC_REQ(ftmRangingParams);

  // The size of Subtype Field +
  // RTT Message Header +
  // Per Target Header
  maxLen += sizeof(OemMsgSubType) +
            sizeof(RomeRTTReportHeaderIE) +
            sizeof(RomeRttPerPeerReportHdr);
  // Measurement Size
  maxLen += numMeas * (sizeof(RomeRttPerFrame_IE_RTTV3));
  // Size for LCI information
  maxLen += lci ? LCI_FTM_LCI_TOT_MAX_LEN : 0;
  // Size for LCR information
  maxLen += lcr ? LOC_CIVIC_ELE_MAX_LEN : 0;

  return maxLen;
}

void LOWIRangingFSM::AddDummyMeas(LOWIScanMeasurement::eTargetStatus status)
{
  // Find the index of the channel and the first BSSID in the previous request
  unsigned int chIdx = mRomeRangingReq.curChIndex;
  unsigned int apIdx = mRomeRangingReq.curAp;

  if (chIdx && (!mRomeRangingReq.validRangingScan || (apIdx == 0)))
  {
    // If on 1st AP that means previous channel was used.
    // Go to last AP on this channel.
    chIdx--;
    apIdx = mRomeRangingReq.arrayVecRangingNodes[chIdx].getNumOfElements()-1;
  }
  else if (apIdx)
  {
    apIdx--;
  }

  if (mRomeRangingReq.mRttBgNodeList.getSize() == 0)
  {
    vector <LOWIRangingNode>& vNodes = mRomeRangingReq.arrayVecRangingNodes[chIdx];
    for (unsigned int bssIdx = 0; (bssIdx < mRangingReqRspInfo.bssidCount) &&
                                  (bssIdx <= apIdx); bssIdx++)
    {
      AddTargetToResult(vNodes[apIdx-bssIdx].targetNode, status, true);
    }
  }
  else // Handle BG node
  {
    // iterate over the node list to prepare Dummy node
    vector <LOWIRangingNode>& vNodes = mRomeRangingReq.arrayVecRangingNodes[chIdx];
    bool found_bg_node = false;
    for (unsigned int bssIdx = 0; (bssIdx < mRangingReqRspInfo.bssidCount) &&
                                  (bssIdx <= apIdx); bssIdx++)
    {
      if (IsRttBgNodeInList(vNodes[apIdx-bssIdx].targetNode.bssid))
      {
        AddTargetToResult(vNodes[apIdx-bssIdx].targetNode, status, true);
        found_bg_node = true;
      }
    }
    if (!found_bg_node)
      log_error(TAG, "%s: Cannot find BG node in list ", __FUNCTION__);
  }
}

bool LOWIRangingFSM::validChannelBWCombo(eRangingBandwidth bw,
        uint32 primary_freq, uint32 center_freq1, uint32 center_freq2)
{
  LOWIRangingFSM FSMObj;
  bool retVal = false;
  uint32 max2GHzFreq = FSMObj.Getmax2GHzFreq();
  uint32 max5GHzFreq = FSMObj.Getmax5GHzFreq();
  uint32 max6GHzFreq = FSMObj.Getmax6GHzFreq();
  uint32 maxFreq = 0;

  if (LOWIUtils::isValid6GFreq(primary_freq)) {
      maxFreq = max6GHzFreq;
  } else if (LOWIUtils::isValid5GFreq(primary_freq)) {
      maxFreq = max5GHzFreq;
  }

  /* Ensure that the Channel spacing requested is allowed for requested BW */
  switch (bw)
  {
    case BW_20MHZ:
    {
      /* For 20MHZ BW, only allow Center Frequency of 0 or same as Primary */
      retVal = IS_VALID_20MHZ_CHAN_SPACING(primary_freq, center_freq1);
      break;
    }
    case BW_40MHZ:
    {
      /* For 40MHZ BW, only allow Center Frequency of Primary +/- 10 MHz */
      retVal = IS_VALID_40MHZ_CHAN_SPACING(primary_freq, center_freq1, max2GHzFreq, maxFreq);
      break;
    }
    case BW_80MHZ:
    {
      /* For 80MHZ BW, allow Center Frequency of Primary +/- 10 MHz OR Primary +/- 30 MHz */
      retVal = IS_VALID_80MHZ_CHAN_SPACING(primary_freq, center_freq1, maxFreq);
      break;
    }
    case BW_160MHZ:
      // 160MHz BW can operation can be in either one single 160MHz Band or 80 + 80
      if (IS_VALID_160MHZ_CHAN_SPACING(primary_freq, center_freq1, center_freq2, maxFreq) ||
          IS_VALID_80P80MHZ_CHAN_SPACING(primary_freq, center_freq1, center_freq2, maxFreq))
      {
        /* Valid channel spacing */
        retVal = true;
      }
      break;
    default:
    {
      /* Invalid Bandwidth - Do Nothing */
      log_debug (TAG, "%s, Invalid bandwidth requested", __FUNCTION__);
      break;
    }
  }
  return retVal;
}

bool LOWIRangingFSM::validChannelBWPreambleCombo(LOWIRangingNode &targetNode)
{

  bool retVal = false;
  wmi_channel chanInfo = targetNode.chanInfo.wmiChannelInfo;
  LOWINodeInfo node = targetNode.targetNode;

  do
  {
    /* Check to ensure the primary and center frequency provided by user are valid */
    if (LOWIUtils::isValidCenFreq(chanInfo.mhz) == 0)
    {
      log_debug (TAG, "%s, Invalid primary channel", __FUNCTION__);
      break;
    }

    /* Ensure Preamble requested by user is within the supported range */
    if (node.preamble < RTT_PREAMBLE_LEGACY ||
        node.preamble >= RTT_PREAMBLE_MAX)
    {
      log_debug (TAG, "%s, Invalid Preamble requested", __FUNCTION__);
      break;
    }

    if (!validChannelBWCombo(node.bandwidth, chanInfo.mhz, chanInfo.band_center_freq1,
                             chanInfo.band_center_freq2))
    {
      break;
    }
    /* Ensure that the requested Bandwidth, Channel and preamble are an allowed combination */
    /* Using the valid Phy mode table to check the combination */
    LOWIDiscoveryScanRequest::eBandBit bandBit = LOWIUtils::freqToBandBit(chanInfo.mhz);
    if (bandBit == LOWIDiscoveryScanRequest::BAND_INVALID_BIT)
    {
      log_warning (TAG, "%s, Invalid band", __FUNCTION__);
      break;
    }
    if (validPhyModeTable[bandBit][node.preamble][node.bandwidth] == LOWI_RTT_PHY_MODE_UNKNOWN)
    {
      log_warning (TAG, "%s, validPhyModeTable check failed bandBit %d preamble %u bandwidth %u",
                   __FUNCTION__, bandBit, node.preamble, node.bandwidth);
      break;
    }
    log_debug (TAG, "%s, Valid BW(%s), Channel(%u) Center_freq(%u:%u) and preamble(%s) for AP:" LOWI_MACADDR_FMT,
               __FUNCTION__, LOWIUtils::to_string(node.bandwidth),
               chanInfo.mhz, chanInfo.band_center_freq1, chanInfo.band_center_freq2,
               LOWIUtils::to_string(node.preamble), LOWI_MACADDR(node.bssid));
    retVal = true;
  } while(0);

  if (retVal == false)
  {
    log_debug (TAG, "%s, Invalid BW(%s), Channel and preamble(%s), Aborting! AP:" LOWI_MACADDR_FMT
              " frequency: %u, center_freq1: %u",
              __FUNCTION__, LOWIUtils::to_string(node.bandwidth),
              LOWIUtils::to_string(node.preamble),
              LOWI_MACADDR(node.bssid),
              chanInfo.mhz, chanInfo.band_center_freq1);
  }

  return retVal;
}

bool LOWIRangingFSM::validRangingNode(LOWIRangingNode &rangingNode)
{
  FSM_ENTER()
  bool retVal = false;
  LOWINodeInfo &node = rangingNode.targetNode;

  do
  {
    if ((node.rttType < RTT1_RANGING) ||
        (node.rttType > RTT3_RANGING))
    {
      log_debug (TAG, "%s, Invalid Ranging type requested: %s",
                 __FUNCTION__,
                 LOWIStrings::to_string(node.rttType));
      break;
    }

    if ((TMR_GET_TYPE(node.tmrParams) == TMR_LEGACY) &&
        (node.num_pkts_per_meas > MAX_RTT_MEAS_PER_DEST))
    {
      log_debug (TAG, "%s, Invalid number fo FTM frames requested: %u",
                 __FUNCTION__,
                 node.num_pkts_per_meas);
      break;
    }

    if (!validChannelBWPreambleCombo(rangingNode))
    {
     log_debug (TAG, "%s, Invalid Combination of Channel/BW/Preamble",
                __FUNCTION__);
     break;

    }
    if (node.frequency == BAND_2G_FREQ_LAST)
    {
      if (node.rttType == RTT3_RANGING)
      {
        log_debug (TAG, "%s, Reject 2 sided RTT for channel 14",
                   __FUNCTION__);
        break;
      }
      else if (!((node.rttType < RTT3_RANGING) &&
                 (node.bandwidth == BW_20MHZ) &&
                 (node.preamble == RTT_PREAMBLE_LEGACY)))
      {
        log_debug (TAG, "%s, reject one sided ranging request for channel 14"
                   "with BW %d and preamble %d",
                   __FUNCTION__, node.bandwidth, node.preamble);
        break;
      }
    }
    retVal = true;
  } while(0);

  FSM_EXIT()
  return retVal;
}

int LOWIRangingFSM::ValidateRangingRequest()
{
  FSM_ENTER()
  if ((mCurReq == NULL) ||
      (eRequestType != LOWIRequest::RANGING_SCAN))
  {
    log_debug(TAG, "%s: No New request to handle, Aborting call", __FUNCTION__);
    return -1;
  }
  vector <LOWIRangingNode>& v = mRomeRangingReq.vecRangingNodes;
  unsigned int numNonAsapTargets = 0;

  if (0 == v.getNumOfElements())
  {
    log_warning (TAG, "%s:No AP's in range request", __FUNCTION__);
    return -1;
  }
  else
  {
    for(unsigned int n = 0; n < v.getNumOfElements(); n++)
    {
      LOWINodeInfo& node = v[n].targetNode;
      /* Check if it is an ASAP 0 request */
      if (FTM_GET_ASAP(node.ftmRangingParameters) == 0 &&
          node.rttType == RTT3_RANGING)
      {
        numNonAsapTargets++;
      }

      if (0 != ValidatePeriodicity(node))
      {
        return -1;
      }
    }

    /* Only 1 ASAP 0 target allowed per request*/
    if (numNonAsapTargets > 1)
    {
      log_debug (TAG, "%s, Aborting, numNonAsapTargets: %u, Max supported = 1",
                 __FUNCTION__, numNonAsapTargets);
      return -1;
    }
  }

  FSM_EXIT()
  return 0;
}

int LOWIRangingFSM::ValidatePeriodicity(LOWINodeInfo const & /* info */)
{
  return 0;
}

bool LOWIRangingFSM::setPhyMode(LOWIRangingNode &rangingNode)
{
  bool retVal = false;
  eLOWIPhyMode phyMode = LOWI_RTT_PHY_MODE_11G;
  LOWIDiscoveryScanRequest::eBandBit bandBit =
                LOWIUtils::freqToBandBit(rangingNode.chanInfo.wmiChannelInfo.mhz);

  if ((LOWIUtils::isValidCenFreq(rangingNode.chanInfo.wmiChannelInfo.mhz) == 0) ||
      (rangingNode.targetNode.bandwidth <  BW_20MHZ) ||
      (rangingNode.targetNode.bandwidth >= BW_MAX) ||
      (rangingNode.targetNode.preamble  <  RTT_PREAMBLE_LEGACY) ||
      (rangingNode.targetNode.preamble  >= RTT_PREAMBLE_MAX) ||
      (bandBit == LOWIDiscoveryScanRequest::BAND_INVALID_BIT))
  {
    log_debug(TAG, "%s, Not a valid freq: %u, preamble: %u, Bw: %u BandBit: %d",
              __FUNCTION__,
              rangingNode.chanInfo.wmiChannelInfo.mhz,
              rangingNode.targetNode.preamble,
              rangingNode.targetNode.bandwidth, bandBit);
  }
  else
  {
    wmi_channel chanInfo = rangingNode.chanInfo.wmiChannelInfo;
    eLOWIPhyMode lowiPhyMode =
     validPhyModeTable[bandBit][rangingNode.targetNode.preamble][rangingNode.targetNode.bandwidth];
    phyMode = lowiPhyMode;
    // Handle Special 80 + 80 -> 160MHz case
    if ((phyMode == LOWI_RTT_PHY_MODE_11AC_VHT160) &&
        IS_VALID_80P80MHZ_CHAN_SPACING(chanInfo.mhz, chanInfo.band_center_freq1,
            chanInfo.band_center_freq2, max5GHzFreq))
    {
      // change to 80 + 80 if band_center_frequency2 is non zero,
      // because this implies 160 MHz BW is achieved using 80 + 80
      phyMode = LOWI_RTT_PHY_MODE_11AC_VHT80_80;
    }
    /* Default to LOWI_RTT_PHY_MODE_11G if invalid preamble, BW & band combination */
    phyMode = (phyMode == LOWI_RTT_PHY_MODE_UNKNOWN) ? LOWI_RTT_PHY_MODE_11G : phyMode;
    log_debug(TAG, "%s: validPhyModeTable[] PhyMode - %s, phyMode set - %s", __FUNCTION__,
              LOWIUtils::to_string(lowiPhyMode), LOWIUtils::to_string(phyMode));
    retVal = true;
  }

  rangingNode.chanInfo.wmiChannelInfo.info &= PHY_MODE_MASK;
  rangingNode.chanInfo.wmiChannelInfo.info |= phyMode;
  return retVal;
}

int LOWIRangingFSM::AddTargetToResult(LOWINodeInfo node,
                    LOWIScanMeasurement::eTargetStatus errorCode, bool is_last_resp_for_peer)
{
  log_debug(TAG, "%s: Add " LOWI_MACADDR_FMT " errorCode %d is_last_resp_for_peer %d",
            __FUNCTION__, LOWI_MACADDR(node.bssid), errorCode, is_last_resp_for_peer);
  vector <LOWIScanMeasurement*> *scanMeasurements = &mLowiMeasResult->scanMeasurements;
  LOWIScanMeasurement* rangingMeasurement = new (std::nothrow) LOWIScanMeasurement;
  if (rangingMeasurement == NULL)
  {
    log_warning(TAG, "%s: rangingMeasurement - allocation faliure", __FUNCTION__);
    return -1;
  }
  rangingMeasurement->bssid.setMac(node.bssid);
  rangingMeasurement->frequency = node.frequency;
  rangingMeasurement->isSecure = false;
  rangingMeasurement->msapInfo = NULL;
  rangingMeasurement->cellPowerLimitdBm = 0;

  rangingMeasurement->type = ACCESS_POINT;
  rangingMeasurement->rttType = node.rttType;
  rangingMeasurement->tmr_subtype = 0;
  rangingMeasurement->security_mode = 0;
  rangingMeasurement->num_frames_attempted = 0;
  rangingMeasurement->actual_burst_duration = 0;
  rangingMeasurement->negotiated_num_frames_per_burst = 0;
  rangingMeasurement->retry_after_duration = 0;
  rangingMeasurement->negotiated_burst_exp = 0;

  /* 11az params */
  rangingMeasurement->i2r_lmr_feedback = 0;
  rangingMeasurement->ranging_priority = 0;
  rangingMeasurement->r2i_toa_type = 0;
  rangingMeasurement->i2r_toa_type = 0;
  rangingMeasurement->format_and_bw = 0;
  rangingMeasurement->imdt_r2i_feedback = 0;
  rangingMeasurement->imdt_i2r_feedback = 0;
  rangingMeasurement->max_i2r_repetition = 0;
  rangingMeasurement->max_r2i_repetition = 0;
  rangingMeasurement->device_class = 0;
  rangingMeasurement->full_bw_ul_mumimo = 0;
  rangingMeasurement->max_r2i_sts_le_80mhz = 0;
  rangingMeasurement->max_r2i_sts_gr_80mhz = 0;
  rangingMeasurement->max_r2i_ltf_total = 0;
  rangingMeasurement->max_i2r_sts_le_80mhz = 0;
  rangingMeasurement->max_i2r_sts_gr_80mhz = 0;
  rangingMeasurement->max_i2r_ltf_total = 0;
  rangingMeasurement->min_time_between_measurements = 0;
  rangingMeasurement->is_last_resp_for_peer= is_last_resp_for_peer;
  rangingMeasurement->tb_max_session_expiry = 0;

  rangingMeasurement->targetStatus = errorCode;

  scanMeasurements->push_back(rangingMeasurement);

  return 0;

}

void LOWIRangingFSM::loadInfoFromCache(LOWIRangingNode &rangingNode,
                                                   LOWIScanMeasurement &scanMeasurement)
{
  rangingNode.chanInfo.wmiChannelInfo.mhz = scanMeasurement.frequency;
  rangingNode.chanInfo.wmiChannelInfo.band_center_freq1 = 0;
  rangingNode.chanInfo.wmiChannelInfo.band_center_freq2 = 0;
  // Load Band Center Frequency based on requested BW from cache
  if(rangingNode.targetNode.bandwidth < BW_MAX)
  {
    rangingNode.chanInfo.wmiChannelInfo.band_center_freq1 =
      scanMeasurement.band_center_freq1[rangingNode.targetNode.bandwidth];
  }

  // If requested BW doesn't have a valid band center frequency, set it to primary freq by default
  if(rangingNode.chanInfo.wmiChannelInfo.band_center_freq1 == 0)
  {
    rangingNode.chanInfo.wmiChannelInfo.band_center_freq1 = scanMeasurement.frequency;
  }
  if (rangingNode.targetNode.bandwidth == BW_160MHZ)
  {
    rangingNode.chanInfo.wmiChannelInfo.band_center_freq2 = scanMeasurement.band_center_freq2;
  }

    // get the frequency band and proceed accordingly
  LOWIDiscoveryScanRequest::eBandBit bandBit =
         LOWIUtils::freqToBandBit(scanMeasurement.frequency);

  if ((rangingNode.targetNode.tmrParams == TMR_LEGACY) &&
       ((LOWIDiscoveryScanRequest::BAND_5G_BIT == bandBit) ||
        (LOWIDiscoveryScanRequest::BAND_2P4G_BIT == bandBit)))
  {
    rangingNode.chanInfo.wmiChannelInfo.info = scanMeasurement.nonHeInfo;
  } else {
    rangingNode.chanInfo.wmiChannelInfo.info = scanMeasurement.info;
  }
}

int LOWIRangingFSM::PrepareRangingRequest()
{
  log_verbose(TAG, "%s", __FUNCTION__);
  if (mCurReq != NULL && (eRequestType == LOWIRequest::RANGING_SCAN))
  {
    if (0 != CheckRangingSupport())
    {
      return -1;
    }

    LOWIRangingScanRequest *rangingrequest = (LOWIRangingScanRequest *)mCurReq;
    vector<LOWINodeInfo> v = rangingrequest->getNodes();
    if (0 == v.getNumOfElements())
    {
      log_warning(TAG, "%s: No AP's in range request", __FUNCTION__);
      return -1;
    }

    mLowiMeasResult = new(std::nothrow) LOWIMeasurementResult;
    if (mLowiMeasResult == NULL)
    {
      log_debug(TAG, "%s:Ranging Measurment Results - allocation faliure", __FUNCTION__);
      return -1;
    }
    if (InitializeMeasResult(mLowiMeasResult) != 0)
    {
      log_info(TAG, "%s:initialize LOWI Meas Results - failed", __FUNCTION__);
      return -1;
    }
    log_verbose(TAG, "Ranging scan requested with : %u APs", v.getNumOfElements());

    mRomeRangingReq.vecRangingNodes.flush();
    for (unsigned int oof = 0; oof < v.getNumOfElements(); oof++)
    {
      LOWIScanMeasurement scanMeasurement;
      LOWIRangingNode rangingNode;
      wmi_channel &chanInfo = rangingNode.chanInfo.wmiChannelInfo;
      rangingNode.targetNode = v[oof];
      rangingNode.targetNode.interface = v[oof].interface;

      /* Skip target if it is not supported */
      if (false == targetParamsSupported(v[oof]))
      {
        AddTargetToResult(v[oof],
           LOWIScanMeasurement::LOWI_TARGET_STATUS_RTT_FAIL_TARGET_NOT_CAPABLE,
           true);
        continue;
      }

      std::map<tANI_U32, ChannelInfo>::iterator it;
      ChannelInfo value;
      it = mChannelInfoArray.find(v[oof].frequency);
      if (it == mChannelInfoArray.end())
      {
        log_warning(TAG, "cannot find the freq:%u in DRV Freq map", v[oof].frequency);
        AddTargetToResult(v[oof],
           LOWIScanMeasurement::LOWI_TARGET_STATUS_RTT_FAIL_TARGET_NOT_CAPABLE,
           true);
        continue;
      }
      else
      {
        value = it->second;
        chanInfo.info = value.wmiChannelInfo.info;
        chanInfo.reg_info_1 = value.wmiChannelInfo.reg_info_1;
        chanInfo.reg_info_2 = value.wmiChannelInfo.reg_info_2;
      }

      if (v[oof].paramControl == LOWI_USE_PARAMS_FROM_CACHE)
      {
         if (mCacheManager &&
              mCacheManager->getFromCache(v[oof].bssid, scanMeasurement) == true)
         {
            if (validateTmrParams(rangingNode.targetNode, scanMeasurement) == false)
            {
                AddTargetToResult(v[oof],
                        LOWIScanMeasurement::LOWI_TARGET_STATUS_RTT_FAIL_TARGET_NOT_CAPABLE);
                continue;
            }
            loadInfoFromCache(rangingNode, scanMeasurement);
            log_verbose(TAG, "%s: AP - " LOWI_MACADDR_FMT " Found in Cache",
                        __FUNCTION__, LOWI_MACADDR(v[oof].bssid));
         }
         else
         {
            log_verbose(TAG, "%s: AP - " LOWI_MACADDR_FMT " NOT Found in Cache",
                        __FUNCTION__, LOWI_MACADDR(v[oof].bssid));
            if ((LOWIUtils::freqToBandBit(v[oof].frequency) ==
                 LOWIDiscoveryScanRequest::BAND_2P4G_BIT) ||
                 (rangingNode.targetNode.bandwidth == BW_160MHZ)) {
                 chanInfo.mhz = v[oof].frequency;
                 chanInfo.band_center_freq1 = v[oof].band_center_freq1;
                 chanInfo.band_center_freq2 = v[oof].band_center_freq2;
                 // set the phy mode for the case when no cache was used. When cache is used,
                 // the phy mode is set inside loadInfoFromCache().
                 setPhyMode(rangingNode);
            } else {
                 loadDefaultChanInfo(rangingNode, v[oof].frequency);
            }
        }
      }
      else
      {
        log_verbose(TAG, "%s: AP - " LOWI_MACADDR_FMT " From User Conf",
                      __FUNCTION__, LOWI_MACADDR(v[oof].bssid));
        chanInfo.mhz = v[oof].frequency;
        chanInfo.band_center_freq1 = v[oof].band_center_freq1;
        chanInfo.band_center_freq2 = v[oof].band_center_freq2;
        // set the phy mode for the case when no cache was used. 
        setPhyMode(rangingNode);
      }

      eLOWIPhyMode pM = (eLOWIPhyMode)(chanInfo.info & ~PHY_MODE_MASK);
      log_info(TAG, "%s: AP: " LOWI_MACADDR_FMT " MHz: %u freq1: %u freq2: %u BW: %u phy mode: %s info=0x%x info_1=0x%x info_2=0x%x"
            " %s Overwrite chanInfo from cache",
            __FUNCTION__,
            LOWI_MACADDR(rangingNode.targetNode.bssid),
            rangingNode.targetNode.frequency, rangingNode.targetNode.band_center_freq1,
            rangingNode.targetNode.band_center_freq2, rangingNode.targetNode.bandwidth,
            LOWIUtils::to_string(pM),
            chanInfo.info, chanInfo.reg_info_1, chanInfo.reg_info_2,
            (rangingNode.targetNode.paramControl == LOWI_USE_PARAMS_FROM_CACHE ? " " : "DONOT"));

      if (validRangingNode(rangingNode))
      {
        mRomeRangingReq.vecRangingNodes.push_back(rangingNode);
      }
      else
      {
        log_verbose(TAG,
              "%s: Invalid Ranging Target, skipping target: " LOWI_MACADDR_FMT,
            __FUNCTION__, LOWI_MACADDR(rangingNode.targetNode.bssid));
        AddTargetToResult(v[oof],
            LOWIScanMeasurement::LOWI_TARGET_STATUS_RTT_FAIL_TARGET_NOT_CAPABLE, true);
      }
      // Do not worry, Scheduler will ensure one channel request at a time
      // So we can safely copy this node here instead of while doing in
      // Group by Channel Request.
      if (rangingNode.targetNode.isRttBgNode)
      {
        AddRttBgNodeInList(&rangingNode);
      }
    }

    /** Group APs by Channel Structure */
    for (unsigned int i = 0; i < MAX_DIFFERENT_CHANNELS_ALLOWED; i++)
    {
      mRomeRangingReq.arrayVecRangingNodes[i].flush();
    }
    mRomeRangingReq.totChs = groupByChannel(mRomeRangingReq.vecRangingNodes,
                                            &mRomeRangingReq.arrayVecRangingNodes[0],
                                            MAX_DIFFERENT_CHANNELS_ALLOWED);
    mRomeRangingReq.curChIndex = 0;
    mRomeRangingReq.totAp = mRomeRangingReq.arrayVecRangingNodes[mRomeRangingReq.curChIndex].getNumOfElements();
    mRomeRangingReq.curAp = 0;
    mRomeRangingReq.validRangingScan = TRUE;
    for (unsigned int foo = 0; foo < mRomeRangingReq.totChs; ++foo)
    {
      unsigned int apCount = mRomeRangingReq.vecChGroup[foo].getNumOfElements();
      for (unsigned int bar = 0; bar < apCount; bar++)
      {
        log_info(TAG,
          "BSSID: " LOWI_MACADDR_FMT " frequency: %u, Center Frequency 1: %u isRttBgNode %d",
          LOWI_MACADDR(mRomeRangingReq.arrayVecRangingNodes[foo][bar].targetNode.bssid),
          mRomeRangingReq.arrayVecRangingNodes[foo][bar].chanInfo.wmiChannelInfo.mhz,
          mRomeRangingReq.arrayVecRangingNodes[foo][bar].chanInfo.wmiChannelInfo.band_center_freq1,
          mRomeRangingReq.arrayVecRangingNodes[foo][bar].targetNode.isRttBgNode);
      }
    }
  }
  else
  {
    log_debug(TAG, "%s:No New request", __FUNCTION__);
    return -1;
  }

  return 0;
}

unsigned int LOWIRangingFSM::groupByChannel(vector <LOWIRangingNode>& origVec,
                                         vector <LOWIRangingNode> *vec,
                                         unsigned int maxChannels)
{
  unsigned int chCount = 0;

  log_verbose (TAG, "%s:Num Aps: %u", __FUNCTION__, origVec.getNumOfElements());
  for(unsigned int i = 0; i < origVec.getNumOfElements(); ++i)
  {
    ChannelInfo chanInfo = origVec[i].chanInfo;
    log_verbose (TAG, "%s:Current AP's Frequency: %u, BSSID: " LOWI_MACADDR_FMT, __FUNCTION__,
                 chanInfo.wmiChannelInfo.mhz, LOWI_MACADDR(origVec[i].targetNode.bssid));

    if(chanInfo.wmiChannelInfo.mhz == 0)
    {
      /** Note:  This should never happen, but simply,
        *  continuing to gracefully handle this case
        */
        log_debug(TAG, "AP with Channel Frequency of 0 was sent for Ranging!");
        continue;
    }

    boolean found = false;
    /* Search already gathered channels (last discovered channel first) */
    if (chCount != 0)
    {
      for(int j = (chCount - 1); j >= 0; j--)
      {
        LOWIRangingNode& topNode = vec[j][0];

        if (memcmp(&(topNode.chanInfo), &chanInfo, sizeof(chanInfo)) == 0)
        {
          /* Match Found vector already exists so add to vector */
          log_verbose (TAG, "%s:Adding to existing vector [%u]",  __FUNCTION__, j);
          vec[j].push_back(origVec[i]);
          found = true;
          break;
        }
      }
    }
    if(found == false) /* No matching freq was found, create a new vector for this frequency */
    {
      chCount++;
      if(chCount < maxChannels)
      {
        vec[chCount - 1].push_back(origVec[i]);
        log_verbose (TAG, "%s:Creating new vector for new channel", __FUNCTION__);
      }
      else
      {
        log_debug (TAG, "%s:Reached MAX number of different Channels per RTT request!", __FUNCTION__);
        return chCount;
      }
    }
  }

  log_verbose(TAG, "%s - Tot Channels %u", __FUNCTION__, chCount);
  return chCount;
}

bool LOWIRangingFSM::targetParamsSupported(LOWINodeInfo &node)
{
  LOWI_RangingCapabilities lowiRangingCap;
  lowiRangingCap = GetRangingCap();
  bool supported = false;

  do
  {
    /* Check if the reqeusted Ranging Type is supported by HW */
    if ((false == lowiRangingCap.dualSidedSupported11mc) && (RTT3_RANGING == node.rttType))
    {

      log_error (TAG, "%s: Dual sided RTT NOT supported "
                "Skipping AP - " LOWI_MACADDR_FMT, __FUNCTION__,
                LOWI_MACADDR(node.bssid));
      break;
    }
    /* Check if the requested Preamble  is supported by HW */
    if (!(lowiRangingCap.preambleSupport & (1 << node.preamble)))
    {
      log_error (TAG, "%s: Preamble %s NOT supported (%x)"
                "Skipping this AP - " LOWI_MACADDR_FMT, __FUNCTION__,
                LOWIUtils::to_string(node.preamble),
                lowiRangingCap.preambleSupport,
                LOWI_MACADDR(node.bssid));
      break;
    }
    /* Check if the requested BW is supported by HW */
    if (node.bandwidth > lowiRangingCap.bwSupport)
    {
      log_error (TAG, "%s: BW %s NOT supported (%s)"
                "Skipping this AP - " LOWI_MACADDR_FMT, __FUNCTION__,
                LOWIUtils::to_string(LOWIUtils::to_eRangingBandwidth(node.bandwidth)),
                LOWIUtils::to_string(LOWIUtils::to_eRangingBandwidth(lowiRangingCap.bwSupport)),
                LOWI_MACADDR(node.bssid));
      break;
    }

    /* Check if the requested BW is supported by HW */
    if (!LOWIUtils::isValidCenFreq(node.frequency))
    {
      log_error (TAG, "%s: not a valid freq %u skipping this AP: "
                  LOWI_MACADDR_FMT, __FUNCTION__,
                  node.frequency, LOWI_MACADDR(node.bssid));
      break;
    }

    /* get the frequency band and proceed accordingly */
    LOWIDiscoveryScanRequest::eBandBit bandBit =
         LOWIUtils::freqToBandBit(node.frequency);

    if ((node.preamble == RTT_PREAMBLE_HE) &&
        (TMR_GET_TYPE(node.tmrParams) == TMR_LEGACY) &&
        ((bandBit == LOWIDiscoveryScanRequest::BAND_2P4G_BIT) ||
         (bandBit == LOWIDiscoveryScanRequest::BAND_5G_BIT)))
    {
      log_error (TAG, "%s: HE preamble not allowed, skipping this AP: "
                 LOWI_MACADDR_FMT, __FUNCTION__, LOWI_MACADDR(node.bssid));
      break;
    }

    if ((bandBit == LOWIDiscoveryScanRequest::BAND_6G_BIT) &&
        (node.preamble != RTT_PREAMBLE_HE))

    {
      log_error (TAG, "%s: Non-HE preamble not allowed in 6G, skipping this AP: "
                 LOWI_MACADDR_FMT, __FUNCTION__, LOWI_MACADDR(node.bssid));
      break;
    }

    /* Check if the requested NTB supported by HW */
    if ((false == lowiRangingCap.isNtbSupp) &&
        (TMR_GET_TYPE(node.tmrParams) == TMR_NTB))
    {
      log_error (TAG, "%s: 11AZ NTB not allowed, skipping this AP: "
                 LOWI_MACADDR_FMT, __FUNCTION__, LOWI_MACADDR(node.bssid));
      break;
    }

    /* Check if the requested TB is supported by HW */
    if ((false == lowiRangingCap.isTbSupp) &&
        (TMR_GET_TYPE(node.tmrParams) == TMR_TB))
    {
      log_error (TAG, "%s: 11AZ TB not allowed, skipping this AP: "
                 LOWI_MACADDR_FMT, __FUNCTION__, LOWI_MACADDR(node.bssid));
      break;
    }

    /* Check if the MAC security is supported by HW */
    if ((false == lowiRangingCap.isMacSecSupp) &&
        (TMR_GET_SEC(node.tmrParams) == SEC_FTM))
    {
      log_error (TAG, "%s: Mac level security not allowed, skipping this AP: "
                 LOWI_MACADDR_FMT, __FUNCTION__, LOWI_MACADDR(node.bssid));
      break;
    }

    /* Check if the MAC+PHY security is supported by HW */
    if ((false == lowiRangingCap.isPhySecSupp) &&
        (TMR_GET_SEC(node.tmrParams) == SEC_FTM_LTF))
    {
      log_error (TAG, "%s: Mac+Phy level sec not allowed, skipping this AP: "
                 LOWI_MACADDR_FMT, __FUNCTION__, LOWI_MACADDR(node.bssid));
      break;
    }
    /* Todo: Add check for Multiburst */

    /* If all above tests pass then set supported to true */
    supported = true;
  } while(0);

  return supported;
}

int LOWIRangingFSM::SendRangingReq()
{
  int retVal = 0;

  DestInfo   bssidsToScan[MAX_BSSIDS_TO_SCAN];
  vector <LOWIRangingNode>::Iterator it = mRomeRangingReq.arrayVecRangingNodes[mRomeRangingReq.curChIndex].begin();
  ChannelInfo chanInfo = (*it).chanInfo;
  unsigned int bssidIdx = 0;
  std::string interface = "wifi0";
  bool ntb_req =false, tb_req =false;


  if (!mRangingReqRspInfo.lastResponseRecv)
  {
    /* Don't send another Request because FW is still handling previous request */
    log_debug(TAG, "%s:FW Still handling previous request", __FUNCTION__);
    return 0;
  }

  log_debug(TAG, "%s: curChIndex: %u, totChs: %u, curAp: %u, totAp: %u",
            __FUNCTION__, mRomeRangingReq.curChIndex,
            mRomeRangingReq.totChs,
            mRomeRangingReq.curAp,
            mRomeRangingReq.totAp);

  if ((mRomeRangingReq.curChIndex == 0) && (mRomeRangingReq.curAp == 0))
  {
    log_info(TAG, "%s: Starting RTT with %u APs over %u channels",__FUNCTION__,
             mRomeRangingReq.totAp, mRomeRangingReq.totChs);
  }
  mRangingReqRspInfo.nonAsapTargetPresent = FALSE;
  mRangingReqRspInfo.timeOut = 0;

  if (mRomeRangingReq.curChIndex < mRomeRangingReq.totChs &&
      mRomeRangingReq.curAp < mRomeRangingReq.totAp)
  {
    vector <LOWIRangingNode>& vNodes = mRomeRangingReq.arrayVecRangingNodes[mRomeRangingReq.curChIndex];
    unsigned int spaceLeftInRsp = MAX_WMI_MESSAGE_SIZE;
    while (mRomeRangingReq.curAp < mRomeRangingReq.totAp)
    {
      LOWIRangingNode & rangingNode = vNodes[mRomeRangingReq.curAp];

      unsigned int spaceNeeded;
      unsigned int numframes;

      if (TMR_GET_TYPE(rangingNode.targetNode.tmrParams) == TMR_LEGACY) {
          numframes = MIN(rangingNode.targetNode.num_pkts_per_meas, MAX_RTT_MEAS_PER_DEST);
      } else {
          numframes = rangingNode.targetNode.num_pkts_per_meas;
      }

      spaceNeeded = ExpectedMeasSizeForTarget(numframes,
              rangingNode.targetNode.ftmRangingParameters);
      if ((spaceNeeded < spaceLeftInRsp) && (bssidIdx < MAX_BSSIDS_TO_SCAN))
      {
        spaceLeftInRsp -= spaceNeeded;
        log_verbose(TAG, "%s: Adding AP at index %d - spaceUsed: %u spaceLeftInRsp: %u",
                     __FUNCTION__, bssidIdx, spaceNeeded, spaceLeftInRsp);
      }
      else
      {
        log_verbose(TAG, "%s: Done with this request, AP Cnt %d, spaceUsed: %u, spaceLeftInRsp: %u",
                    __FUNCTION__, bssidIdx, spaceNeeded, spaceLeftInRsp);
        break;
      }
      interface = rangingNode.targetNode.interface;
      switch (rangingNode.targetNode.rttType)
      {
        case RTT3_RANGING:
        {
          bssidsToScan[bssidIdx].rttFrameType  = RTT_MEAS_FRAME_TMR;
          break;
        }
        case RTT1_RANGING:
        case RTT2_RANGING:
        case BEST_EFFORT_RANGING: /* For now Best effor will default to RTT V2 */
        default: /* This should never happen, but just in case RTTV2 is the default */
        {
          bssidsToScan[bssidIdx].rttFrameType  = RTT_MEAS_FRAME_QOSNULL;
          break;
        }
      }
      for (int ii = 0; ii < BSSID_LEN; ++ii)
      {
        bssidsToScan[bssidIdx].mac[ii]  = rangingNode.targetNode.bssid[ii];
      }
      bssidsToScan[bssidIdx].bandwidth = rangingNode.targetNode.bandwidth;
      bssidsToScan[bssidIdx].preamble  = rangingNode.targetNode.preamble;
      bssidsToScan[bssidIdx].numFrames = numframes;
      bssidsToScan[bssidIdx].numFrameRetries = rangingNode.targetNode.num_retries_per_meas;
      bssidsToScan[bssidIdx].vDevType = rangingNode.targetNode.nodeType;

      /* FTM Parameters*/
      bssidsToScan[bssidIdx].ftmParams = rangingNode.targetNode.ftmRangingParameters;
      bssidsToScan[bssidIdx].tmrParams = rangingNode.targetNode.tmrParams;
      bssidsToScan[bssidIdx].tmrMinDelta = rangingNode.targetNode.tmrMinDelta;
      bssidsToScan[bssidIdx].tmrMaxDelta = rangingNode.targetNode.tmrMaxDelta;
      bssidsToScan[bssidIdx].tsfDelta  = rangingNode.tsfDelta;
      bssidsToScan[bssidIdx].tsfValid  = rangingNode.tsfValid;
      bssidsToScan[bssidIdx].isQtiPeer = rangingNode.isQtiPeer;
      bssidsToScan[bssidIdx].tbPeriodicity = rangingNode.targetNode.tbPeriodicity;
      bssidsToScan[bssidIdx].tbDuration = rangingNode.targetNode.tbDuration;
      bssidsToScan[bssidIdx].tbMaxExpiry = rangingNode.targetNode.tbMaxSessionExpiry;
      bssidsToScan[bssidIdx].rttChannelSwitchPriority = rangingNode.targetNode.rttChannelSwitchPriority;
      bssidsToScan[bssidIdx].txrxChainIndexInitiator  = rangingNode.targetNode.txrxChainIndexInitiator;
      bssidsToScan[bssidIdx].txrxChainIndexResponder  = rangingNode.targetNode.txrxChainIndexResponder;
      bssidsToScan[bssidIdx].iteratorInitiator  = rangingNode.targetNode.iteratorInitiator;
      bssidsToScan[bssidIdx].iteratorResponder  = rangingNode.targetNode.iteratorResponder;
      bssidsToScan[bssidIdx].beaconPeriod = rangingNode.targetNode.beaconPeriod;

      log_debug(TAG, "%s:bssidsToScan[%d].txrxChainIndexInitiator = 0x%x",
                     __FUNCTION__, bssidIdx, bssidsToScan[bssidIdx].txrxChainIndexInitiator);
      log_debug(TAG, "%s:bssidsToScan[%d].txrxChainIndexResponder = 0x%x",
                     __FUNCTION__, bssidIdx, bssidsToScan[bssidIdx].txrxChainIndexResponder);
      log_debug(TAG, "%s:bssidsToScan[%d].iteratorInitiator = 0x%x",
                     __FUNCTION__, bssidIdx, bssidsToScan[bssidIdx].iteratorInitiator);
      log_debug(TAG, "%s:bssidsToScan[%d].iteratorResponder = 0x%x",
                     __FUNCTION__, bssidIdx, bssidsToScan[bssidIdx].iteratorResponder);
      mRomeRangingReq.reportType = rangingNode.targetNode.reportType;
      /** Check if target is an ASAP 0 target */
      if ((rangingNode.targetNode.rttType == RTT3_RANGING) &&
          (FTM_GET_ASAP(rangingNode.targetNode.ftmRangingParameters) == 0))
      {
        mRangingReqRspInfo.nonAsapTargetPresent = TRUE;
      }

      if (TMR_GET_TYPE(bssidsToScan[bssidIdx].tmrParams) == TMR_NTB) {
          mRangingReqRspInfo.timeOut += (bssidsToScan[bssidIdx].numFrames *
                                         bssidsToScan[bssidIdx].tmrMinDelta);
          ntb_req = true;
      }

      if (TMR_GET_TYPE(bssidsToScan[bssidIdx].tmrParams) == TMR_TB) {
          mRangingReqRspInfo.timeOut += (bssidsToScan[bssidIdx].tbDuration * 100);
          tb_req = true;
      }

      mRomeRangingReq.curAp++;
      bssidIdx++;
    }

    if (ntb_req && tb_req)
    {
       /* Invalid request */
       log_error(TAG, "%s:Invalid request with TB and NTB session", __FUNCTION__);
       return 0;
    }

    if (ntb_req) {
       /** For NTB tmrMinDelta is in units of 100usec,
           so convert it to millisec by diving with 10 **/
       mRangingReqRspInfo.timeOut = mRangingReqRspInfo.timeOut/10;
    }

    log_verbose(TAG, "%s: Sending %u APs to FW for Ranging, ASAP %d",
                __FUNCTION__, bssidIdx, mRangingReqRspInfo.nonAsapTargetPresent);

    mRangingReqRspInfo.bssidCount = bssidIdx;
    mRangingReqRspInfo.measPerTarget = MAX_RTT_MEAS_PER_DEST;
    mRangingReqRspInfo.lastResponseRecv = FALSE;
    mRangingReqRspInfo.totalRspFromFw = 0;

    getNewRtsCtsTag(); //Increment the tag before sending request to FW.
    // send the request for this set of targets
    retVal = SendRttRequest(chanInfo, bssidIdx, bssidsToScan, bssidsToScan, mRomeRangingReq.reportType, interface);
    if (retVal)
       mRangingReqRspInfo.lastResponseRecv = TRUE;

    /* If we still have APs left in the current channel */
    if (mRomeRangingReq.curAp < mRomeRangingReq.totAp)
    {
      log_verbose(TAG,"Staying on current Channel Channel");
    }
    else /* Move to APs on the next channel if we are done with all APs in this channel*/
    {
      mRomeRangingReq.curChIndex++;
      /* if we have Channels left, change AP count and iterator to the list of APs in the new Channel */
      if (mRomeRangingReq.curChIndex < mRomeRangingReq.totChs)
      {
        log_verbose(TAG,"Moving to Next Channel");
        mRomeRangingReq.curAp = 0;
        mRomeRangingReq.totAp = mRomeRangingReq.arrayVecRangingNodes[mRomeRangingReq.curChIndex].getNumOfElements();
      }
      else /* We are done with all channels, mark this request as invalid */
      {
        log_verbose(TAG,"Done with All channels and APs");
        mRomeRangingReq.validRangingScan = FALSE;
      }
    }
  }
  else
  {
    mRomeRangingReq.validRangingScan = FALSE;
    log_verbose(TAG," %s: not valid range scan", __FUNCTION__);
  }

  return retVal;
}

int LOWIRangingFSM::SendRttRequest(ChannelInfo chanInfo, unsigned int bssidIdx,
                                   DestInfo *bssidsToScan, DestInfo *spoofBssids, unsigned int reportType,
                                   std::string interface)
{
  int retVal = -1;

    retVal = mLOWIRanging->RomeSendRttReq(mRtsCtsTag,
                                          chanInfo,
                                          bssidIdx,
                                          bssidsToScan,
                                          spoofBssids,
                                          reportType,
                                          interface  );

  mRangingReqRspInfo.reportType = reportType;

  if(reportType == RTT_AGGREGATE_REPORT_NON_CFR)
  {
     mRangingReqRspInfo.expectedRspFromFw = RESP_REPORT_TYPE2;
  }
  else
  {
     mRangingReqRspInfo.expectedRspFromFw = (bssidIdx * MAX_RTT_MEAS_PER_DEST);
  }

  return retVal;
}

int LOWIRangingFSM::ProcessRangingMeas(bool rxAllChainSupport)
{
  log_verbose(TAG, "%s", __FUNCTION__);

  /* Increment the total number of Measurement responses received from FW */
  mRangingReqRspInfo.totalRspFromFw++;

  if (mLOWIRanging->RomeParseRangingMeas((char*)mRangingFsmData, &mLowiMeasResult->scanMeasurements, rxAllChainSupport) != 0)
  {
    log_info (TAG, "%s, Process Ranging Meas - Failed", __FUNCTION__);
    return -1;
  }

  return 0;
}

int LOWIRangingFSM::ExtractReqId()
{
  return mLOWIRanging->RomeExtractReqId((char *)mRangingFsmData);
}

/*=============================================================================================
 * Function description:
 *   Waits on the Private Netlink Socket and the LOWI controller pipe.
 *   If Messages/Data arrive, collects and processes them accordingly.
 *   If LOWI controller requests a shutdown then exits and picks up the new pequest to process
 *
 * Return value:
 *    error code
 *
 =============================================================================================*/
int LOWIRangingFSM::ListenForEvents()
{
  FSM_ENTER()
  int retVal = -1;

  int timeout_val = SELECT_TIMEOUT_NEVER;

  if (mRangingFsmContext.timeEnd)
  {
    uint64 timeNow = get_time_rtc_ms();
    timeout_val = (mRangingFsmContext.timeEnd - timeNow) / 1000;
    log_verbose(TAG, "%s:Time left to wait: %u sec.\n", __FUNCTION__, timeout_val);
  }

  if (NULL != mLOWIRanging)
  {
    retVal = mLOWIRanging->RomeWaitOnActivityOnSocketOrPipe(timeout_val);
  }

  if(retVal > 0) /* Some Valid Message Arrived on Socket */
  {
    RomeNlMsgType msgType;
    log_verbose(TAG, "%s: Valid msg receive on Socket curEvent %u curState %u",
                __FUNCTION__, mRangingFsmContext.curEvent,
                mRangingFsmContext.curState);
    mLOWIRanging->RomeNLRecvMessage(&msgType, mRangingFsmData, sizeof(mRangingFsmData));
    mRangingFsmContext.curEvent = RomeCLDToRomeFSMEventMap[msgType];
  }
  else if(retVal == ERR_SELECT_TIMEOUT)      /* Timeout Occured */
  {
    log_debug(TAG, "%s: Timeout for netlink socket msg \n", __FUNCTION__);
    mRangingFsmContext.curEvent = EVENT_TIMEOUT;
  }
  else if(retVal == ERR_SELECT_TERMINATED)   /* Shutdown Through Pipe */
  {
    if (mRangingPipeEvents == TERMINATE_THREAD)
    {
      mRangingFsmContext.curEvent = EVENT_TERMINATE_REQ;
    }
    else if (mRangingPipeEvents == NEW_REQUEST_ARRIVED)
    {
      LowiReqToEvent(mNewReq);
    }
    log_verbose(TAG, "%s: Got Event over PIPE from controller: %s", __FUNCTION__,
                LOWIStrings::to_string(mRangingFsmContext.curEvent));
  }
  else                                       /* Some Error Occured */
  {
    log_warning(TAG, "%s: %s %d", __FUNCTION__,
                retVal == ERR_NOT_READY ? "Driver not ready" :
                "Error when waiting for event", retVal);
    mRangingFsmContext.curEvent = EVENT_NOT_READY;
  }

  FSM_EXIT()
  return retVal;
}

int LOWIRangingFSM::LowiReqToEvent(const LOWIRequest * const pReq)
{
  int ret_val = -1;
  if (NULL == pReq)
  {
    mRangingFsmContext.curEvent = EVENT_INVALID_REQ;
  }
  else if (mRangingFsmContext.curState >= STATE_READY_AND_IDLE)
  {
    eRequestType = pReq->getRequestType();
    mCurReq = mNewReq;
    switch(pReq->getRequestType())
    {
      case LOWIRequest::RANGING_SCAN:
        {
          mRangingFsmContext.curEvent = EVENT_RANGING_REQ;
          ret_val = 0;
        }
        break;
      case LOWIRequest::SET_LCI_INFORMATION:
        {
          mRangingFsmContext.curEvent = EVENT_CONFIGURATION_REQ;
          ret_val = 0;
        }
        break;
      case LOWIRequest::LOWI_RTT_RM_CHANNEL_REQUEST:
        {
          mRangingFsmContext.curEvent = EVENT_RTT_AVAILABLE_CHANNEL_REQ;
          ret_val = 0;
        }
        break;
      case LOWIRequest::LOWI_ENABLE_RESPONDER_REQUEST:
        {
          mRangingFsmContext.curEvent = EVENT_ENABLE_RESPONDER_REQ;
          ret_val = 0;
        }
        break;
      case LOWIRequest::LOWI_DISABLE_RESPONDER_REQUEST:
        {
          mRangingFsmContext.curEvent = EVENT_DISABLE_RESPONDER_REQ;
          ret_val = 0;
        }
        break;
      case LOWIRequest::LOWI_START_RESPONDER_MEAS_REQUEST:
        {
          mRangingFsmContext.curEvent = EVENT_START_RESPONDER_MEAS_REQ;
          ret_val = 0;
        }
        break;
      case LOWIRequest::LOWI_STOP_RESPONDER_MEAS_REQUEST:
        {
          mRangingFsmContext.curEvent = EVENT_STOP_RESPONDER_MEAS_REQ;
          ret_val = 0;
        }
        break;
      case LOWIRequest::SET_LCR_INFORMATION:
        {
          mRangingFsmContext.curEvent = EVENT_CONFIGURATION_REQ;
          ret_val = 0;
        }
        break;
      case LOWIRequest::SEND_LCI_REQUEST:
        {
          mRangingFsmContext.curEvent = EVENT_CONFIGURATION_REQ;
          ret_val = 0;
        }
        break;
      case LOWIRequest::FTM_RANGE_REQ:
        {
          mRangingFsmContext.curEvent = EVENT_CONFIGURATION_REQ;
          ret_val = 0;
        }
        break;
      case LOWIRequest::CANCEL_RANGING_SCAN:
        {
          mRangingFsmContext.curEvent = EVENT_CANCEL_RANGING_REQ;
          ret_val = 0;
        }
        break;
      default:
        {
          mRangingFsmContext.curEvent = EVENT_INVALID_REQ;
        }
        break;
    }
    SetLOWIRequest(NULL);
  }
  else
  {
    mRangingFsmContext.curEvent = EVENT_RANGING_REQ;
  }
  return ret_val;
}

int LOWIRangingFSM::FSM()
{
  int retVal = -1;
  do
  {
    log_debug(TAG, "%s: Received Event %s, Current state: %s", __FUNCTION__,
             LOWIStrings::to_string(mRangingFsmContext.curEvent),
             LOWIStrings::to_string(mRangingFsmContext.curState));

    if (mRangingFsmContext.curState < STATE_MAX &&
        mRangingFsmContext.curEvent < EVENT_MAX)
    {
      retVal = stateTable[mRangingFsmContext.curState][mRangingFsmContext.curEvent](this);
      log_debug(TAG, "%s: New FSM state: %s", __FUNCTION__,
               LOWIStrings::to_string(mRangingFsmContext.curState));
    }
    else
    {
      log_warning(TAG, "%s:Received a bad State: %u or Event: %u", __FUNCTION__,
                  mRangingFsmContext.curState, mRangingFsmContext.curEvent);
    }

    /* Check to see if there any any pending internal Events */
    if (mRangingFsmContext.internalFsmEventPending)
    {
      /* clear the flag and allow the event to be procesed by FSM */
      clearFsmInternalEvent();
    }
    else
    {
      /** After FSM has processed current event, wait listening for
       *  new events from Timer, NL socket & LOWI Controller */
      ListenForEvents();
    }
    /** If a terminate request arrived then stop FSM and return
     *  to caller */
    if (mRangingFsmContext.curEvent == EVENT_TERMINATE_REQ)
    {
      log_debug(TAG, "%s: Received Terminate Thread request from Controller",
                __FUNCTION__);
      break;
    }

  } while(1);
  return retVal;
}

int LOWIRangingFSM::SetNewPipeEvent(RangingPipeEvents newEvent)
{
  mRangingPipeEvents = newEvent;
  return mLOWIRanging->RomeUnblockRangingThread();
}

LOWIRangingFSM* LOWIRangingFSM::createInstance(LOWIScanResultReceiverListener *scanResultListener,
                                               LOWICacheManager *cacheManager,
                                               LOWIRanging *lowiRanging,
                                               LOWIRangingFSM::eLowiRangingInterface lowiRangingInterface)
{
  LOWIRangingFSM *fsmInstance = NULL;
  switch (lowiRangingInterface)
  {
    case LOWIRangingFSM::LOWI_ROME_RANGING_INTERFACE:
      if (NULL == mWifiInstance)
      {
        mWifiInstance = new LOWIRangingFSM(scanResultListener, cacheManager, lowiRanging);
      }
      fsmInstance = mWifiInstance;
      break;

      case LOWIRangingFSM::LOWI_PRONTO_RANGING_INTERFACE:
#if !defined(LOWI_ON_ACCESS_POINT) && !defined(LOWI_ON_LE)
      if (NULL == mWifiInstance)
      {
        mWifiInstance = new LOWIRangingProntoFSM(scanResultListener, cacheManager, lowiRanging);
      }
      fsmInstance = mWifiInstance;
#endif
      break;

    case LOWIRangingFSM::LOWI_HELIUM_RANGING_INTERFACE:
      if (NULL == mWifiInstance)
      {
        mWifiInstance = new LOWIRangingHeliumFSM(scanResultListener, cacheManager, lowiRanging);
      }
      fsmInstance = mWifiInstance;
      break;

    case LOWIRangingFSM::LOWI_SPARROW_RANGING_INTERFACE:
#if !defined(LOWI_ON_ACCESS_POINT) && !defined(LOWI_ON_LE)
      if (NULL == mWigigInstance)
      {
        mWigigInstance = new LOWIRangingSparrowFSM(scanResultListener, cacheManager, lowiRanging);
      }
      fsmInstance = mWigigInstance;
#endif
      break;

    default:
      log_debug(TAG, "%s: LOWI Ranging Interface Unknown", __FUNCTION__);
      break;
  }

  if (NULL == fsmInstance)
  {
    log_warning(TAG, "Failed to create FSM of type: %d", lowiRangingInterface);
  }
  return fsmInstance;
} // createInstance

int LOWIRangingFSM::ProcessResponderChannelMeas()
{
  return 0;
}

int LOWIRangingFSM::CheckRangingSupport()
{
  if (mRomeRttCapabilities.rangingTypeMask == 0)
  {
    // No support for Ranging
    log_debug(TAG, "Ranging Not supported: %d",
              mRomeRttCapabilities.rangingTypeMask);
    return -1;
  }
  return 0;
}

