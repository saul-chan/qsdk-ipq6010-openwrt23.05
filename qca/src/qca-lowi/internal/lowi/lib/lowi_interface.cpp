/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

        LIB/LOWI API INTERFACE

GENERAL DESCRIPTION
  This file contains the implementation of the LOWI interface for RTT
  and BGSCAN measurements

Copyright (c) 2014-2024 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
=============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <sys/time.h>
#include <base_util/log.h>
#include <common/lowi_utils.h>
#include <base_util/config_file.h>
#include <inc/lowi_client.h>
#include <inc/lowi_defines.h>
#include "lowi_interface.h"
#include "lowi_listener_impl.h"
#include "inc/infra/lowi_request_extn.h"

using namespace qc_loc_fw;

LOWIClient* client;
LOWIClientListener* listener;
uint32 req_id;
boolean lowi_interface_initialized = FALSE;
lowi_rtt_capabilities *rttCachedCapabilities = NULL; //stores rtt capabilities

unsigned int const maxFramesPerBurst         = 31;
unsigned int const minFramesPerBurst         = 1;
unsigned int const maxRetriesPerRttFrame     = 3;
unsigned int const maxRetriesPerFtmr         = 3;
unsigned int const burstDurationNoPreference = 15;
unsigned int const maxBurstDuration          = 16;
unsigned int const minBurstDuration          = 2;
uint32 const capSubscriptionTimeout          = 60;

// callback table used to call lowi interface functions
static lowi_cb_table_t lowi_ftable =
{
  /* lowi-client interface functions */
  .init                             = lowi_init,
  .destroy                          = lowi_destroy,
  /* rtt functions */
  .get_rtt_capabilities             = lowi_get_rtt_capabilities,
  .rtt_range_request                = lowi_rtt_range_request,
  .rtt_range_cancel                 = lowi_rtt_range_cancel,
  /* other lowi-client interface functions */
  .get_lowi_version                 = lowi_get_lowi_version,
  .get_lowi_capabilities            = lowi_get_lowi_capabilities,
  /* bgscan functions */
  .rtt_set_lci                      = lowi_set_lci,
  .rtt_set_lcr                      = lowi_set_lcr,
};

// this structure contains the original request id provided by the client, the request id
// used by lowi to send to the request to the controller and that callback needed to pass
// the results back to the client
struct requestInfo
{
  int reqId;      //client request id
  int lowiReqId;  // id assigned by LOWI
  LOWIRequest::eRequestType reqType; // request type
};

// this vector stores id's and response handler information for some bgscan requests
List<requestInfo> rspHandlers;

// this structure contains rtt request id and request
// handler information needed to return the results
struct rttRequestInfo
{
  int reqId;
  int lowiReqId;
  lowi_rtt_event_handler rttResultsCb;
};

// this vector stores the id and handler information for the all requests
vector <rttRequestInfo> rttRequestHandlers;

vector <struct req_map> g_req_cache;
//Filter measurement if per BW mismatch happen
int32 g_discard_bw_mis_match = 0;

int32 g_weighted_mean = 0;

// local functions
static void mapNodeTypeToLOWI(lowi_rtt_config const *config, LOWIPeriodicNodeInfo* info);
static void mapMeasBwToLOWI(lowi_rtt_config const *config, LOWIPeriodicNodeInfo* info);
static void mapMeasPreambleToLOWI(lowi_rtt_config const *config, LOWIPeriodicNodeInfo* info);
static void mapRttTypeToLOWI(lowi_rtt_config const *config, LOWIPeriodicNodeInfo* info);
static void mapRetriesToLOWI(lowi_rtt_config const *config, LOWIPeriodicNodeInfo* info);
static void printLowiRequest(uint32 request_id, vector<LOWIPeriodicNodeInfo> nodes);
static void printRequestInfo(uint32 request_id, uint32 num_rtt_config, lowi_rtt_config rtt_config[]);
static void lowi_adjust_single_sided_rtt_packet_bw(lowi_rtt_config rtt_config[], uint32 num_rtt_config);
static int  wifiNodeParamChecking(uint32 request_id, uint32 num_rtt_config, lowi_rtt_config rtt_config[]);
static LOWICapabilityResponse* sendCapsRequest(uint32 req_id, std::string interface);
static void printCapabilitiesInfo(LOWICapabilityResponse* cap_resp);
static int setRttCapabilities(LOWICapabilities &caps);
static byte setBwCapabilityMask(uint8 maxBw);

// debugging definition
#undef LOWI_ALLOW_ENTER_EXIT
#ifdef LOWI_ALLOW_ENTER_EXIT
#define LOWI_ENTER() log_verbose(TAG, "ENTER: %s", __FUNCTION__);
#define LOWI_EXIT()  log_verbose(TAG, "EXIT: %s", __FUNCTION__);
#else
#define LOWI_ENTER()
#define LOWI_EXIT()
#endif

#define LOWI_BREAK_IF_NULL(x,err,s) if (NULL == x) \
      {                                                    \
        log_warning(TAG, "%s: %s", __FUNCTION__, s);       \
        retVal = err;                                      \
        break;                                             \
      }


static char const *TAG = LOWI_INTERFACE_VERSION;

/*========================================================================================
 * lowi_get_cb_table
 *
 * Description:
 *   This function returns a pointer to the lowi callback table struct.
 *   It is called from external applications to retrieve a table of functions that
 *   it can use to interface with LOWI
 *
 * Parameters:
 *  none
 *
 * Return value:
 *   ptr to callback table struct
 ========================================================================================*/
extern "C"
lowi_cb_table_t* lowi_get_cb_table()
{
  LOWI_ENTER()
  return &lowi_ftable;
}

/*========================================================================================
 * lowi_init
 *
 * Description:
 *   This function initializes the LOWI client interface.
 *   NOTE: this function shall be called only once before lowi_destroy is called
 *
 * Parameters:
 *  none
 *
 * Return value:
 *   0: initialization successful
 *   else: initializatin failed
 ========================================================================================*/
int lowi_init ()
{
  int retVal = LOWI_ERROR_UNKNOWN;
  listener = NULL;
  client   = NULL;
  ConfigFile *mConfig = NULL;
  int   log_level = qc_loc_fw::EL_INFO;
  /* Default value of standard deviation multiplier for Two-sided RTT */
  int32 stdevMult = 5;
  /* Standard deviation multiplier for single-sided RTT.
   * For single-sided RTT, we need to use 20 MHz RTT_STDDEV, i.e., 21 ns,
   * and default value of ‘10’ for stdevMult.
   */
  int32 RTT2_stdevMult = 10;

  LOWI_ENTER()
  do
  {
    if( lowi_interface_initialized )
    {
      log_warning(TAG, "%s: LOWI interface already initialized!", __FUNCTION__);
      retVal = LOWI_SUCCESS;
      break;
    }

    // Access the config file to get the log level flag
    mConfig = ConfigFile::createInstance(CONFIG_NAME);

    if(NULL == mConfig || !mConfig->loaded())
    {
      log_debug(TAG, "%s: config file not available", __FUNCTION__);
    }
    else
    {
      int retVal = mConfig->getInt32("LOWI_LOG_LEVEL", log_level);
      // if LOWI_LOG_LEVEL not found in conf file, use the LOWI_LOG_LEVEL
      if (retVal != 0)
      {
        mConfig->getInt32Default ("LOWI_LOG_LEVEL", log_level, log_level);
        log_debug(TAG, "%s: LOWI_LOG_LEVEL not found"
                    "set to lowi log level %d", __FUNCTION__, log_level);
      }
      // Read and set weighted mean algorithm from the config file
      mConfig->getInt32 ("LOWI_RTT_WEIGHTED_MEAN", g_weighted_mean);

      // Read and set the stdev multiplier from the config file for two-sided RTT
      mConfig->getInt32Default ("LOWI_STDEV_MULT", stdevMult, stdevMult);

      // Read and set the stdev multiplier from the config file for single-sided RTT
      mConfig->getInt32Default ("LOWI_RTT2_STDEV_MULT",
              RTT2_stdevMult, RTT2_stdevMult);

      retVal = mConfig->getInt32("LOWI_BW_MISMATCH", g_discard_bw_mis_match);
      // if LOWI_BW_MISMATCH not found in config file, use it as 0
      log_debug(TAG, "%s: LOWI_BW_MISMATCH %d",
                     __FUNCTION__, g_discard_bw_mis_match);
    }

    qc_loc_fw::log_set_global_level((qc_loc_fw::ERROR_LEVEL)log_level);

    // instantiate a client listener
    listener = new LOWIClientListenerImpl(stdevMult, RTT2_stdevMult);
    LOWI_BREAK_IF_NULL(listener, LOWI_ERROR_UNINITIALIZED,
                               "could not create the LOWIClientListener")

    // instantiate a LOWI client
    client = LOWIClient::createInstance(listener, true, (LOWIClient::eLogLevel)log_level);
    if( NULL == client )
    {
      log_warning (TAG, "%s: Could not create the LOWIClient", __FUNCTION__);
      delete listener;
      listener = NULL;
      retVal = LOWI_ERROR_UNINITIALIZED;
      break;
    }

    req_id = 0;

    // interface initialized successfully
    lowi_interface_initialized = TRUE;
    log_debug(TAG, "LOWI_INTERFACE_VERSION: (%s)", LOWI_INTERFACE_VERSION);

    retVal = LOWI_SUCCESS;
  } while( 0 );

  delete mConfig;
  LOWI_EXIT()
  return retVal;
}

/*========================================================================================
 * lowi_destroy
 *
 * Description:
 *   This function destroys the LOWI client interface.
 *
 * Parameters:
 *  none
 *
 * Return value:
 *   0: LOWI interface successfully destroyed
 *   else: failed to destroy the LOWI interface
 ========================================================================================*/
int lowi_destroy ()
{
  int retVal = LOWI_ERROR_UNKNOWN;

  LOWI_ENTER()

  if (NULL != client)
  {
    delete client;
    client = NULL;
  }

  if (NULL != listener)
  {
    delete listener;
    listener = NULL;
  }

  if (NULL != rttCachedCapabilities)
  {
    delete rttCachedCapabilities;
    rttCachedCapabilities = NULL;
  }

  /* clear table of request info */
  rttRequestHandlers.flush();

  /* clear the handler's list */
  rspHandlers.flush();

  lowi_interface_initialized = FALSE;

  retVal = LOWI_SUCCESS;
  return retVal;
}

/*=============================================================================================
 * lowi_get_lowi_capabilities
 *
 * Description:
 *   This function gets capabilities that are related to the LOWI interface that
 *   are needed to support the GSCAN and RTT APIs. This is a blocking call. No further
 *   action should take place on the part of the client until the LOWI supported
 *   capabilities are returned successfully.
 *   NOTE: to be called only lowi_init() has returned successfully
 *
 * Parameters:
 *   caps: placeholder for capabilities. Capabilities is set up as a bit mask. The values
 *         are defined in internal.h
 *
 * Return value:
 *   0: call was successfully sent by lowi client, this says nothing about whether the
 *      capabilities were retrieved successfully
 *   else: lowi client failed to send the request for capabilities
 =============================================================================================*/
int lowi_get_lowi_capabilities(uint32_t *lowi_caps, std::string interface)
{
  int retVal = LOWI_ERROR_UNKNOWN;
  uint32 capsMask = 0;
  LOWICapabilities caps;

  LOWI_ENTER()

  do
  {
    LOWI_BREAK_IF_NULL(lowi_caps, LOWI_ERROR_INVALID_ARGS, "got NULL from client")

    // send capabilities request to lowi-server then block the
    // client thread until the capabilities response is received
    LOWICapabilityResponse *cap_resp = sendCapsRequest(++req_id, interface);
    if( NULL != cap_resp )
    {
      printCapabilitiesInfo(cap_resp);

      // get the capabilities
      caps = cap_resp->getCapabilities();
      if (true == caps.bgscanSupported)
      {
        log_verbose(TAG, "%s: BGSCAN SUPPORT ENABLED", __FUNCTION__);
      }
      else
      {
        log_verbose(TAG, "%s: BGSCAN SUPPORT DISABLED", __FUNCTION__);
      }

#if LOWI_MAJOR_VERSION >= 1
      if(caps.bgscanSupported)               { capsMask |= GSCAN_SUPPORTED; }
      if(caps.oneSidedRangingSupported)      { capsMask |= ONE_SIDED_RANGING_SUPPORTED; }
      if(caps.dualSidedRangingSupported11mc) { capsMask |= DUAL_SIDED_RANGING_SUPPORED; }
#endif
      *lowi_caps = capsMask;
      log_debug(TAG, "%s: (0x%x)", __FUNCTION__, capsMask);

      delete (cap_resp);
    }
    else
    {
      log_debug(TAG, "%s: Got NULL rsp from LOWI", __FUNCTION__);
      retVal = LOWI_ERROR_UNKNOWN;
      break;
    }

    // cache rtt capabilities information
    if( (NULL == rttCachedCapabilities) && (0 != setRttCapabilities(caps)) )
    {
      retVal = LOWI_ERROR_UNKNOWN;
      break;
    }

    retVal = LOWI_SUCCESS;
  } while( 0 );

  LOWI_EXIT()
  return retVal;
}

/*========================================================================================
 * lowi_rtt_range_request
 *
 * Description:
 *   This function requests RTT measurements.
 *
 * Parameters:
 *    request_id: unique request identification number
 *    num_rtt_config: number of destination BSSIDs to measure RTT on
 *    rtt_config: array of BSSIDs to measure RTT on
 *    handler: callback used to return the results
 *
 * Return value:
 *   0: call was successfully sent by lowi client, this says nothing about whether the
 *      capabilities were retrieved successfully
 *   else: lowi client failed to send the request for capabilities
 ========================================================================================*/
int lowi_rtt_range_request(uint32_t request_id,
                                   uint32_t num_rtt_config,
                                   lowi_rtt_config rtt_config[],
                                   lowi_rtt_event_handler handler)
{
  int retVal = LOWI_ERROR_UNKNOWN;
  LOWI_ENTER()

  // Flush here so multi burst request can also served
  if (g_discard_bw_mis_match)
    g_req_cache.flush();

  // Can't make requests if LOWI interface has not been initialized
  if( !lowi_interface_initialized )
  {
    log_warning (TAG, "%s: Illegal rtt range request"
                 " - interface not initialized", __FUNCTION__);
    retVal = LOWI_ERROR_UNINITIALIZED;
    return(retVal);
  }

  // check input parameters
  if( NULL == rtt_config             ||
      NULL == handler.on_rtt_results ||
      0    == num_rtt_config )
  {
    log_info(TAG, "%s: Incorrect input parameter", __FUNCTION__);
    return(retVal);
  }

  lowi_adjust_single_sided_rtt_packet_bw(rtt_config, num_rtt_config);

  // what does the request look like coming in?
  printRequestInfo(request_id, num_rtt_config, rtt_config);

  if ( 0 != wifiNodeParamChecking(request_id, num_rtt_config, rtt_config) )
  {
    log_info(TAG, "%s: Incorrect rtt_config parameters", __FUNCTION__);
    return(retVal);
  }

  // save the response handler and request id
  rttRequestInfo reqInfo;
  reqInfo.reqId = request_id;
  memcpy( &reqInfo.rttResultsCb, &handler, sizeof(lowi_rtt_event_handler) );

  // store the set of destination BSSIDs in this vector
  vector <LOWIPeriodicNodeInfo> requestSet;

  // configure the request for each destination BSSID
  for( uint32 bssidIdx = 0; bssidIdx < num_rtt_config; bssidIdx++ )
  {
    // set all destination bssid information
    LOWIPeriodicNodeInfo info;

    info.interface = rtt_config[bssidIdx].Interface;
    info.paramControl = LOWI_NO_PARAMS_FROM_CACHE;
    info.phyMode = (qc_loc_fw::eLOWIPhyMode)rtt_config[bssidIdx].phyMode;

    // set the destination bssid
    info.bssid = LOWIMacAddress (rtt_config[bssidIdx].addr[0],
                                 rtt_config[bssidIdx].addr[1],
                                 rtt_config[bssidIdx].addr[2],
                                 rtt_config[bssidIdx].addr[3],
                                 rtt_config[bssidIdx].addr[4],
                                 rtt_config[bssidIdx].addr[5]);

    // set the RTT type
    mapRttTypeToLOWI(&rtt_config[bssidIdx], &info);

    // set the wifi peer type
    mapNodeTypeToLOWI(&rtt_config[bssidIdx], &info);

    // get the channel frequency
    info.frequency         = rtt_config[bssidIdx].channel.center_freq;
    info.band_center_freq1 = rtt_config[bssidIdx].channel.center_freq0;
    info.band_center_freq2 = rtt_config[bssidIdx].channel.center_freq1;

    // get the tx rx chain index and iterator for initiator and responder
    info.txrxChainIndexInitiator = rtt_config[bssidIdx].txrxChainIndexInitiator;
    info.txrxChainIndexResponder = rtt_config[bssidIdx].txrxChainIndexResponder;
    info.iteratorInitiator = rtt_config[bssidIdx].iteratorInitiator;
    info.iteratorResponder = rtt_config[bssidIdx].iteratorResponder;
    // check for multi-burst request
    if (rtt_config[bssidIdx].num_burst > 1)
    {
      info.periodic      = 1; // 1 if burst_num > 1
      info.meas_period   = 100 * rtt_config[bssidIdx].burst_period; // burst_period
      FTM_SET_BURST_PERIOD(info.ftmRangingParameters, info.meas_period);
      info.num_measurements = rtt_config[bssidIdx].num_burst;       // burst_num
      FTM_SET_BURSTS_EXP(info.ftmRangingParameters, info.num_measurements);
    }
    else
    { // single-shot request
      info.periodic    = 0;
      info.meas_period = 0;
      FTM_SET_BURST_PERIOD(info.ftmRangingParameters, info.meas_period);
      info.num_measurements = 0;
      FTM_SET_BURSTS_EXP(info.ftmRangingParameters, info.num_measurements);
    }

    // set retries per bssid measurement
    mapRetriesToLOWI(&rtt_config[bssidIdx], &info);

    // store pkts per rtt measurement
    info.num_pkts_per_meas = rtt_config[bssidIdx].num_frames_per_burst;

    // set the LCI and LCR request flags
    if (0 == rtt_config[bssidIdx].LCI_request)
    {
      FTM_CLEAR_LCI_REQ(info.ftmRangingParameters);
    }
    else
    {
      FTM_SET_LCI_REQ(info.ftmRangingParameters);
    }
    if (0 == rtt_config[bssidIdx].LCR_request)
    {
      FTM_CLEAR_LOC_CIVIC_REQ(info.ftmRangingParameters);
    }
    else
    {
      FTM_SET_LOC_CIVIC_REQ(info.ftmRangingParameters);
    }

    // set the burst duration
    FTM_SET_BURST_DUR(info.ftmRangingParameters, rtt_config[bssidIdx].burst_duration);
    // burst duration can't be "no preference" for 1-sided RTT, so check for this case
    if ( (rtt_config[bssidIdx].type == RTT_TYPE_1_SIDED) &&
         (rtt_config[bssidIdx].burst_duration == burstDurationNoPreference) )
    {
      FTM_SET_BURST_DUR(info.ftmRangingParameters, maxBurstDuration);
    }

    if (rtt_config[bssidIdx].force_legacy_ack)
        FTM_SET_LEG_ACK_ONLY(info.ftmRangingParameters);
    else
        FTM_CLEAR_LEG_ACK_ONLY(info.ftmRangingParameters);

    if (rtt_config[bssidIdx].asap)
        FTM_SET_ASAP(info.ftmRangingParameters);
    else
        FTM_CLEAR_ASAP(info.ftmRangingParameters);

    // store the measurement bandwidth
    mapMeasPreambleToLOWI(&rtt_config[bssidIdx], &info);

    // store the measurement bandwidth
    mapMeasBwToLOWI(&rtt_config[bssidIdx], &info);

    // store 11az params
    TMR_SET_TYPE(info.tmrParams, rtt_config[bssidIdx].tmrtype);
    TMR_SET_SEC(info.tmrParams, rtt_config[bssidIdx].sectype);
    TMR_SET_TB_PRI(info.tmrParams, rtt_config[bssidIdx].tbPriority);
    TMR_SET_MIN_DELTA(info.tmrMinDelta, rtt_config[bssidIdx].tmrMinDelta);
    TMR_SET_MAX_DELTA(info.tmrMaxDelta, rtt_config[bssidIdx].tmrMaxDelta);
    info.isRttBgNode = rtt_config[bssidIdx].isRttBgNode;
    info.tbPeriodicity = rtt_config[bssidIdx].tbPeriodicity;
    info.tbDuration = rtt_config[bssidIdx].tbDuration;
    info.tbMaxSessionExpiry = rtt_config[bssidIdx].tbMaxSessionExpiry;
    info.rttChannelSwitchPriority = rtt_config[bssidIdx].rttChannelSwitchPriority;
    TMR_SET_I2R_FB_POLICY(rtt_config[bssidIdx].I2RFbPolicy);
    // Use Map for BW
    if (g_discard_bw_mis_match)
    {
      struct req_map req(info.bssid, info.bandwidth);
      g_req_cache.push_back(req);
    }

    requestSet.push_back(info);
  }

  reqInfo.lowiReqId = ++req_id;
  LOWIPeriodicRangingScanRequest* req =
  new LOWIPeriodicRangingScanRequest(reqInfo.lowiReqId, requestSet, 0);

  if( NULL != req )
  {
    if( LOWIClient::STATUS_OK == client->sendRequest(req) )
    {
      // store request info
      rttRequestHandlers.push_back(reqInfo);
      // what does the LOWI request look like?
      printLowiRequest(reqInfo.lowiReqId, requestSet);
      retVal = LOWI_SUCCESS;
    }
    delete req;
  }

  return(retVal);
}

/*========================================================================================
 * lowi_rtt_range_cancel
 *
 * Description:
 *   This function cancels RTT measurement request on destination bssids that have been
 *   configured for periodic measurements
 *
 * Parameters:
 *    request_id: unique request identification number
 *    num_devices: number of BSSIDs in "addr" (to be cancelled)
 *    addr: array of BSSIDs to cancel rtt measurements on
 *
 * Return value:
 *   0: request sent successfully
 *   else: failed to send request
 ========================================================================================*/
int lowi_rtt_range_cancel(uint32_t request_id,
                               uint32_t num_devices,
                               mac_addr addr[])
{
  LOWI_ENTER()
  int retVal = LOWI_ERROR_UNKNOWN;

  // Can't make requests if LOWI interface has not been initialized
  if (!lowi_interface_initialized)
  {
    log_warning (TAG, "%s: - interface not initialized", __FUNCTION__);
    retVal = LOWI_ERROR_UNINITIALIZED;
    return retVal;
  }

  // check input parameters
  if((NULL == addr) || (0 == num_devices))
  {
    log_warning (TAG, "%s: Incorrect input parameter", __FUNCTION__);
    return retVal;
  }

  // save the response handler and request id
  rttRequestInfo reqInfo;
  reqInfo.reqId = request_id;

  // store the set of destination BSSIDs to cancel in this vector
  vector <LOWIMacAddress> bssidInfo;

  log_verbose (TAG, "%s: bssids to cancel:", __FUNCTION__);

  // configure the request for each destination BSSID
  for(uint32 bssidIdx = 0; bssidIdx < num_devices; bssidIdx++)
  {
    // store all destination bssid information
    LOWIMacAddress bssid;

    // store the destination bssid
    bssid = LOWIMacAddress (addr[bssidIdx][0],
                            addr[bssidIdx][1],
                            addr[bssidIdx][2],
                            addr[bssidIdx][3],
                            addr[bssidIdx][4],
                            addr[bssidIdx][5]);
    bssid.print();
    bssidInfo.push_back(bssid);
  }

  reqInfo.lowiReqId = ++req_id;
  LOWICancelRangingScanRequest* req =
    new LOWICancelRangingScanRequest(reqInfo.lowiReqId, bssidInfo);

  if(NULL != req)
  {
    // send the request to lowi-server
    if (LOWIClient::STATUS_OK == client->sendRequest(req))
    {
      // store request info
      rttRequestHandlers.push_back(reqInfo);
      retVal = LOWI_SUCCESS;
    }
    delete req;
  }

  return retVal;
}

/*========================================================================================
 * lowi_get_rtt_capabilities
 *
 * Description:
 *   This function requests the RTT capabilities of the device. This is a blocking call.
 *
 * Parameters:
 *  capabilities: ptr to capabilities struct
 *
 * Return value:
 *   0: call was successfully sent by lowi client, this says nothing about whether the
 *      capabilities were retrieved successfully
 *   else: lowi client failed to send the request for capabilities
 ========================================================================================*/
int lowi_get_rtt_capabilities(lowi_rtt_capabilities *capabilities, std::string interface)
{
  LOWI_ENTER()
  int retVal = LOWI_ERROR_UNKNOWN;
  LOWICapabilities caps;

  do
  {
    // Can't make requests if LOWI interface has not been initialized
    if( !lowi_interface_initialized )
    {
      log_warning (TAG, "%s: interface not initialized", __FUNCTION__);
      retVal = LOWI_ERROR_UNINITIALIZED;
      break;
    }

    // check input parameters
    LOWI_BREAK_IF_NULL(capabilities, LOWI_ERROR_INVALID_ARGS, "incorrect input parameter")

    // check if rtt capabilities have already been retrieved
    if( NULL != rttCachedCapabilities )
    {
      *capabilities = *rttCachedCapabilities;

      // return cached capabilities, no need to block the thread
      log_verbose(TAG, "%s: delivering cached capabilities: rtt_one_sided_supported(%d) "
                       "rtt_ftm_supported(%d)",
                  __FUNCTION__,
                  capabilities->rtt_one_sided_supported,
                  capabilities->rtt_ftm_supported);
      retVal = LOWI_SUCCESS;
      break;
    }

    // No rtt capabilities yet, request them.
    // Send capabilities request to lowi-server then block the
    // client thread until the capabilities response is received.
    LOWICapabilityResponse* cap_resp = sendCapsRequest(++req_id, interface);
    if( NULL != cap_resp )
    {
      printCapabilitiesInfo(cap_resp);

      caps = cap_resp->getCapabilities();

      // cache rtt capabilities information
      if( 0 != setRttCapabilities(caps) )
      {
        delete (cap_resp);
        break;
      }

      *capabilities = *rttCachedCapabilities;
      retVal = LOWI_SUCCESS;
      delete (cap_resp);
    }
  } while( 0 );

  return retVal;
}

/*========================================================================================
 * lowi_get_lowi_version
 *
 * Description:
 *   This function returns the lowi interface version supported by LOWI
 *
 * Parameters:
 * major: major lowi interface version supported by LOWI.
 * minor: minor lowi interface version supported by LOWI.
 * micro: micro lowi interface version supported by LOWI.
 *
 * Return value:
 *   int: 0: success, else: failure
 ========================================================================================*/
int lowi_get_lowi_version(u16 *major, u16 *minor, u16 *micro)
{
  LOWI_ENTER()
#if LOWI_MAJOR_VERSION >= 1
  log_debug(TAG, "%s: supported: major(%u) minor(%u) micro(%u)", __FUNCTION__,
            LOWI_MAJOR_VERSION,
            LOWI_MINOR_VERSION,
            LOWI_MICRO_VERSION);
#endif
  log_debug(TAG, "%s: LOWI supported: major(%u) minor(%u) micro(%u)", __FUNCTION__,
            LOWI_IF_MAJOR_VERSION_SUPPORTED,
            LOWI_IF_MINOR_VERSION_SUPPORTED,
            LOWI_IF_MICRO_VERSION_SUPPORTED);

  *major = LOWI_IF_MAJOR_VERSION_SUPPORTED;
  *minor = LOWI_IF_MINOR_VERSION_SUPPORTED;
  *micro = LOWI_IF_MICRO_VERSION_SUPPORTED;

  return 0;
}

/*========================================================================================
 * lowi_set_lci
 *
 * Description:
 *   This function sets the LCI information for an RTT responder
 *
 * Parameters:
 *  id: unique request identifier
 *  lci: pointer to LCI information to set
 *  interface: wifi interface handle
 *
 * Return value:
 *   wifi_error: error type coming from wifi driver
 ========================================================================================*/
lowi_status lowi_set_lci(lowi_request_id /* id */,
                         lowi_lci_information *lci,
                         std::string interface)
{
  LOWI_ENTER()
  lowi_status retVal = LOWI_ERROR_UNKNOWN;

  // Can't make requests if LOWI interface has not been initialized
  do
  {
    if( !lowi_interface_initialized )
    {
      log_warning (TAG, "%s: interface not initialized", __FUNCTION__);
      retVal = LOWI_ERROR_UNINITIALIZED;
      break;
    }

    LOWI_BREAK_IF_NULL(lci,LOWI_ERROR_INVALID_ARGS,"bad input")

    // print client's input
    log_debug (TAG, "%s: client's input: latitude(%" PRId64") longitude(%" PRId64") "
               "altitude(%d) latitude_unc(%d) longitude_unc(%d) altitude_unc(%d) "
               "motion_pattern(%d) floor(%d) height_above_floor(%d) height_unc(%d)",
               __FUNCTION__, lci->latitude, lci->longitude, lci->altitude,
               lci->latitude_unc, lci->longitude_unc, lci->altitude_unc,
               lci->motion_pattern, lci->floor, lci->height_above_floor, lci->height_unc);

    // set up the request
    LOWILciInformation params;
    params.latitude       = (int64)lci->latitude;
    params.longitude      = (int64)lci->longitude;
    params.altitude       = (int32)lci->altitude;
    params.latitude_unc   = (int8)lci->latitude_unc;
    params.longitude_unc  = (int8)lci->longitude_unc;
    params.altitude_unc   = (int8)lci->altitude_unc;
    params.motion_pattern = LOWIUtils::to_eLOWIMotionPattern((uint8)lci->motion_pattern);
    params.floor          = (int32)lci->floor;
    params.height_above_floor = (int32)lci->height_above_floor;
    params.height_unc         = (int32)lci->height_unc;

    uint32 usageRules = 0x01; //set to 1 by default.

    LOWISetLCILocationInformation *req =
      new LOWISetLCILocationInformation(++req_id, params, usageRules);
    LOWI_BREAK_IF_NULL(req, LOWI_ERROR_OUT_OF_MEMORY, "memory allocation error")
    req->set_interface(interface);

    // send the request. No blocking.
    if(client->sendRequest(req) != LOWIClient::STATUS_OK)
    {
      log_warning(TAG, "%s: failed to send the request", __FUNCTION__);
    }
    else
    {
      retVal = LOWI_SUCCESS;
    }
    delete req;
  } while( 0 );

  LOWI_EXIT()
  return retVal;
}

/*========================================================================================
 * lowi_set_lcr
 *
 * Description:
 *   This function sets the LCR information for an RTT responder
 *
 * Parameters:
 *  id: unique request identifier
 *  lcr: pointer to LCR information to set
 *  interface: wifi interface handle
 *
 * Return value:
 *   wifi_error: error type coming from wifi driver
 ========================================================================================*/
lowi_status lowi_set_lcr(lowi_request_id id,
                         lowi_lcr_information *lcr,
                         std::string interface)
{
  lowi_status retVal = LOWI_ERROR_UNKNOWN;

  LOWI_ENTER()

  // Can't make requests if LOWI interface has not been initialized
  do
  {
    if( !lowi_interface_initialized )
    {
      log_warning (TAG, "%s: interface not initialized", __FUNCTION__);
      retVal = LOWI_ERROR_UNINITIALIZED;
      break;
    }

    if( (NULL == lcr) || (lcr->length < 0) )
    {

      log_warning (TAG, "%s: bad input", __FUNCTION__);
      retVal = LOWI_ERROR_INVALID_ARGS;
      break;
    }

    // print client's input
    log_debug (TAG, "%s: client's input: country_code(0x%x%x) length(%d)", __FUNCTION__,
               lcr->country_code[0], lcr->country_code[1], lcr->length);
    log_debug (TAG, "%s: client's input: civic_info(%s)", __FUNCTION__, lcr->civic_info);

    // set up the request
    LOWILcrInformation params;
    params.country_code[0] = (uint8)lcr->country_code[0];
    params.country_code[1] = (uint8)lcr->country_code[1];
    params.length          = (uint32)lcr->length;
    memcpy(params.civic_info, lcr->civic_info, lcr->length * sizeof(int8));

    LOWISetLCRLocationInformation *req =
      new LOWISetLCRLocationInformation(++req_id, params);
    LOWI_BREAK_IF_NULL(req, LOWI_ERROR_OUT_OF_MEMORY, "memory allocation error")
    req->set_interface(interface);

    // send the request. No blocking.
    if(client->sendRequest(req) != LOWIClient::STATUS_OK)
    {
      log_warning(TAG, "%s: failed to send the request", __FUNCTION__);
    }
    else
    {
      retVal = LOWI_SUCCESS;
    }
    delete req;
  } while( 0 );

  LOWI_EXIT()
  return retVal;
}

/*========================================================================================
 * getRttHandler
 *
 * Description:
 *   This function returns the appropriate callback for returning the rtt results to the
 *   application.
 *
 * Parameters:
 *  reqId: request id of request that rtt results are being returned on
 *
 * Return value:
 *   wifi_rtt_event_handler: ptr to handler
 *   NULL: failed to find a handler that matched the request id
 ========================================================================================*/
lowi_rtt_event_handler* getRttHandler(uint32 reqId)
{
  LOWI_ENTER()

  for(unsigned int ii = 0; ii < rttRequestHandlers.getNumOfElements(); ii++)
  {
    if(rttRequestHandlers[ii].lowiReqId == (int)reqId)
    {
      return &rttRequestHandlers[ii].rttResultsCb;
    }
  }
  return NULL;
}

/*========================================================================================
 * getReqId
 *
 * Description:
 *   This function returns the appropriate request id given the lowi request id
 *
 * Parameters:
 *  reqId: lowi request id
 *
 * Return value:
 *   int: request id that is > 0
 *   -1: failed to get a request id, no match
 ========================================================================================*/
int getReqId(uint32 reqId)
{
  LOWI_ENTER()

  for(unsigned int ii = 0; ii < rttRequestHandlers.getNumOfElements(); ii++)
  {
    if(rttRequestHandlers[ii].lowiReqId == (int)reqId)
    {
      return rttRequestHandlers[ii].reqId;
    }
  }
  return -1;
}

/*========================================================================================
 * cleanupRttHandlers
 *
 * Description:
 *   This function removes wifi_rtt_event_handler's that have not been used for a "while"
 *
 * Parameters:
 *
 * Return value:
 ========================================================================================*/
void cleanupRttHandlers()
{
  LOWI_ENTER()
  // later where there is better definition of when to remove the rtt handlers from
  // the client.
}

/****************************************************
 *  LOCAL UTIL FUNCTIONS
 ****************************************************/

/*========================================================================================
 * mapNodeTypeToLOWI
 *
 * Description:
 *   This function the wifi node type received from the request in an rtt ranging request
 *   to a correspoding LOWI wifi node type
 *
 * Parameters:
 * config: pointer to rtt params provided
 * info: pointer to LOWI struct used in client request to lowi-server
 *
 * Return value:
 ========================================================================================*/
static void mapNodeTypeToLOWI(lowi_rtt_config const *config, LOWIPeriodicNodeInfo* info)
{
  switch(config->peer)
  {
    case RTT_PEER_STA:        info->nodeType = STA_DEVICE;   break;
    case RTT_PEER_AP:         info->nodeType = ACCESS_POINT; break;
    case RTT_PEER_P2P_CLIENT: info->nodeType = PEER_DEVICE;  break;
    case RTT_PEER_P2P_GO:     info->nodeType = PEER_DEVICE;  break;
    case RTT_PEER_NAN:        info->nodeType = NAN_DEVICE;   break;
    default:
      info->nodeType = NODE_TYPE_UNKNOWN;
      log_debug(TAG, "UNKNOWN PEER TYPE defaulting to NODE_TYPE_UNKNOWN");
      break;
  }
}

/*========================================================================================
 * mapMeasBwToLOWI
 *
 * Description:
 *   This function the bandwidth type received from the requestor in an rtt ranging request
 *   to a correspoding LOWI bandwidth type
 *
 * Parameters:
 * config: pointer to rtt params provided
 * info: pointer to LOWI struct used in client request to lowi-server
 *
 * Return value:
 ========================================================================================*/
static void mapMeasBwToLOWI(lowi_rtt_config const *config, LOWIPeriodicNodeInfo* info)
{
  switch(config->bw)
  {
    case LOWI_RTT_BW_20:  info->bandwidth = qc_loc_fw::BW_20MHZ;  break;
    case LOWI_RTT_BW_40:  info->bandwidth = qc_loc_fw::BW_40MHZ;  break;
    case LOWI_RTT_BW_80:  info->bandwidth = qc_loc_fw::BW_80MHZ;  break;
    case LOWI_RTT_BW_160: info->bandwidth = qc_loc_fw::BW_160MHZ; break;
    default:
      info->bandwidth = qc_loc_fw::BW_20MHZ;
      log_warning("LOWI_IF", "UNKNOWN BW TYPE defaulting to BW_20MHZ");
      break;
  }
}

/*========================================================================================
 * mapRttTypeToLOWI
 *
 * Description:
 *   This function the rtt type received from the requestor in an rtt ranging request
 *   to a correspoding LOWI rtt type
 *
 * Parameters:
 * config: pointer to rtt params provided
 * info: pointer to LOWI struct used in client request to lowi-server
 *
 * Return value:
 ========================================================================================*/
static void mapRttTypeToLOWI(lowi_rtt_config const *config, LOWIPeriodicNodeInfo* info)
{
  switch(config->type)
  {
    case RTT_TYPE_1_SIDED: info->rttType = qc_loc_fw::RTT2_RANGING;  break;
    case RTT_TYPE_2_SIDED: info->rttType = qc_loc_fw::RTT3_RANGING;  break;
    default:
      info->rttType = qc_loc_fw::RTT2_RANGING;
      log_debug("LOWI_IF", "UNKNOWN RTT TYPE defaulting to RTT_TYPE_1_SIDED");
      break;
  }
}


/*========================================================================================
 * mapMeasPreambleToLOWI
 *
 * Description:
 *   This function maps the preamble received from the requestor in an rtt ranging request
 *   to a correspoding LOWI preamble
 *
 * Parameters:
 * config: pointer to rtt params provided
 * info: pointer to LOWI struct used in client request to lowi-server
 *
 * Return value:
 ========================================================================================*/
static void mapMeasPreambleToLOWI(lowi_rtt_config const *config, LOWIPeriodicNodeInfo* info)
{
  switch(config->preamble)
  {
    case LOWI_RTT_PREAMBLE_LEGACY: info->preamble = RTT_PREAMBLE_LEGACY; break;
    case LOWI_RTT_PREAMBLE_HT:     info->preamble = RTT_PREAMBLE_HT;     break;
    case LOWI_RTT_PREAMBLE_VHT:    info->preamble = RTT_PREAMBLE_VHT;    break;
    case LOWI_RTT_PREAMBLE_HE:     info->preamble = RTT_PREAMBLE_HE;     break;
    default:
      info->preamble = qc_loc_fw::RTT_PREAMBLE_LEGACY;
      log_debug(TAG, "UNKNOWN PREAMBLE defaulting to RTT_PREAMBLE_LEGACY");
      break;
  }
}

/*========================================================================================
 * mapRetriesToLOWI
 *
 * Description:
 *   This function maps the retries parameter received from the requestor in an rtt ranging
 *   request to a correspoding LOWI retries parameter
 *
 * Parameters:
 * config: pointer to rtt params provided
 * info: pointer to LOWI struct used in client request to lowi-server
 *
 * Return value:
 ========================================================================================*/
static void mapRetriesToLOWI(lowi_rtt_config const *config, LOWIPeriodicNodeInfo* info)
{
  if (config->type == RTT_TYPE_1_SIDED)
  {
    info->num_retries_per_meas = config->num_retries_per_rtt_frame;
  }
  if (config->type == RTT_TYPE_2_SIDED)
  {
    info->num_retries_per_meas = config->num_retries_per_ftmr;
  }
}

/*========================================================================================
 * sendCapsRequest
 *
 * Description:
 *   This function instantiates a LOWI capabilities request and sends it to the IPC hub.
 *   Memory in the return to be freed by caller.
 *
 * Parameters:
 * req_id: request id for request sent by client
 *
 * Return value:
 * ptr to capabilities response
 ========================================================================================*/
static LOWICapabilityResponse* sendCapsRequest(uint32 req_id, std::string interface)
{
  LOWI_ENTER()
  LOWICapabilityResponse* cap_resp = NULL;
  LOWICapabilityRequest* cap = new LOWICapabilityRequest(req_id);

  do
  {
    if( NULL == cap )
    {
      log_warning(TAG, "%s: failed to create capabilities request", __FUNCTION__);
      break;
    }
    cap->Interface = interface;
    cap_resp = client->getCapabilities(cap);
    if( NULL == cap_resp )
    {
      log_warning(TAG, "%s: failed to get capabilities", __FUNCTION__);
      break;
    }
  } while( 0 );

  delete cap;

  LOWI_EXIT()
  return cap_resp;
}

/*========================================================================================
 * printLowiRequest
 *
 * Description:
 *   This function prints the current ranging request after conversion to LOWI parameters.
 *   This reflects what is sent to the lowi-server
 *
 * Parameters:
 * req_id: request id for request sent by client
 * nodes: wifi nodes sent in the ranging request
 *
 * Return value:
 ========================================================================================*/
void printLowiRequest(uint32 reqId, vector<LOWIPeriodicNodeInfo> nodes)
{
  log_debug(TAG, "%s: Request_id: %u, numBssids: %u", __FUNCTION__,
            reqId, nodes.getNumOfElements());

  for (int i = 0; i < (int)nodes.getNumOfElements(); i++)
  {
    nodes[i].bssid.print();

    log_debug(TAG, "%s: freq(%u)MHz, band_center_freq1(%u)MHz, band_center_freq2(%u)MHz, "
                   "rttType(%s), bwType(%s), pktsPerMeas(%u)", __FUNCTION__,
              nodes[i].frequency,
              nodes[i].band_center_freq1,
              nodes[i].band_center_freq2,
              LOWI_TO_STRING(nodes[i].rttType, RTT_STR),
              LOWI_TO_STRING(nodes[i].bandwidth, BW_STR),
              nodes[i].num_pkts_per_meas);

    log_debug(TAG, "%s: periodic(%u), burstPeriod(%u) ms, numBursts(%u), numRetries(%u)",
              __FUNCTION__,
              nodes[i].periodic,
              nodes[i].meas_period,
              nodes[i].num_measurements,
              nodes[i].num_retries_per_meas);

    log_debug(TAG, "%s: LCI Req(0x%x) Civic(0x%x) Bursts Exp(%u) Burst Duration(%u) "
                   "Burst Period(%u)", __FUNCTION__,
              FTM_GET_LCI_REQ(nodes[i].ftmRangingParameters),
              FTM_GET_LOC_CIVIC_REQ(nodes[i].ftmRangingParameters),
              FTM_GET_BURSTS_EXP(nodes[i].ftmRangingParameters),
              FTM_GET_BURST_DUR(nodes[i].ftmRangingParameters),
              FTM_GET_BURST_PERIOD(nodes[i].ftmRangingParameters));

  }
}

static void lowi_adjust_single_sided_rtt_packet_bw(lowi_rtt_config rtt_config[], uint32 num_rtt_config)
{
    for(uint32 i=0; i < num_rtt_config; i++) {
        if ((rtt_config[i].type == RTT_TYPE_1_SIDED) && (rtt_config[i].bw > LOWI_RTT_BW_20)) {
            rtt_config[i].bw = LOWI_RTT_BW_20;
            rtt_config[i].channel.width = LOWI_CHAN_WIDTH_20;
            rtt_config[i].channel.center_freq0 = rtt_config[i].channel.center_freq;
            rtt_config[i].channel.center_freq1 = 0;

            LOWIDiscoveryScanRequest::eBandBit bandbit =
                LOWIUtils::freqToBandBit(rtt_config[i].channel.center_freq);

            if (bandbit == LOWIDiscoveryScanRequest::BAND_2P4G_BIT) {
                switch(rtt_config[i].preamble) {
                    case LOWI_RTT_PREAMBLE_LEGACY:
                        rtt_config[i].phyMode = LOWI_PHY_MODE_11G;
                        break;
                    case LOWI_RTT_PREAMBLE_HT:
                        rtt_config[i].phyMode = LOWI_PHY_MODE_11NG_HT20;
                        break;
                    case LOWI_RTT_PREAMBLE_VHT:
                        rtt_config[i].phyMode = LOWI_PHY_MODE_11AC_VHT20_2G;
                        break;
                    default:
                        rtt_config[i].phyMode = LOWI_PHY_MODE_UNKNOWN;
                }
            } else if (bandbit == LOWIDiscoveryScanRequest::BAND_5G_BIT) {
                switch(rtt_config[i].preamble) {
                    case LOWI_RTT_PREAMBLE_LEGACY:
                        rtt_config[i].phyMode = LOWI_PHY_MODE_11A;
                        break;
                    case LOWI_RTT_PREAMBLE_HT:
                        rtt_config[i].phyMode = LOWI_PHY_MODE_11NA_HT20;
                        break;
                    case LOWI_RTT_PREAMBLE_VHT:
                        rtt_config[i].phyMode = LOWI_PHY_MODE_11AC_VHT20;
                        break;
                    default:
                        rtt_config[i].phyMode = LOWI_PHY_MODE_UNKNOWN;
                }
            } else if (bandbit == LOWIDiscoveryScanRequest::BAND_6G_BIT) {
                switch(rtt_config[i].preamble) {
                    case LOWI_RTT_PREAMBLE_HE:
                        rtt_config[i].phyMode = LOWI_PHY_MODE_11AX_HE20;
                        break;
                    default:
                        rtt_config[i].phyMode = LOWI_PHY_MODE_UNKNOWN;
                }
            } else
                rtt_config[i].phyMode = LOWI_PHY_MODE_UNKNOWN ;

            log_debug(TAG, "%s: Updated single-sided RTT input values for BSSID: " LOWI_MACADDR_FMT " bw:%d cfreq:%d cfreq1:%d cfreq2:%d phymode:%d chwidth:%d\n",
                    __FUNCTION__,
                    LOWI_MACADDR(rtt_config[i].addr), rtt_config[i].bw,
                    rtt_config[i].channel.center_freq,
                    rtt_config[i].channel.center_freq0,
                    rtt_config[i].channel.center_freq1, rtt_config[i].phyMode,
                    rtt_config[i].channel.width);
        }
    }
}

/*========================================================================================
 * printRequestInfo
 *
 * Description:
 *   This function prints the ranging request as it came in
 *
 * Parameters:
 * req_id: request id for request sent by client
 * numBssids: number of wifi nodes in the request
 * rtt_config: array of wifi nodes in the ranging request
 *
 * Return value:
 ========================================================================================*/
static void printRequestInfo(uint32 request_id, uint32 numBssids, lowi_rtt_config rtt_config[])
{
  log_debug(TAG, "%s: Request_id: %u, numBssids : %u", __FUNCTION__, request_id, numBssids);

  for(uint32 i=0; i < numBssids; i++)
  {
    log_verbose(TAG, "%s: BSSID: " LOWI_MACADDR_FMT " freq(%u) "
                     "Center_freq (%u,%u) MHz",
                __FUNCTION__,
                LOWI_MACADDR(rtt_config[i].addr),
                rtt_config[i].channel.center_freq,
                rtt_config[i].channel.center_freq0,
                rtt_config[i].channel.center_freq1);

    log_verbose(TAG, "%s: RTT_type(0x%x) PEER_type(%s) BW(%s)"
                     " Burst: num(%u), period(%u) 100ms, duration(%u) frames/burst(%u) ",
                __FUNCTION__,
                rtt_config[i].type,
                LOWI_TO_STRING(rtt_config[i].peer, LOWI_PEER_STR),
                LOWI_TO_STRING(rtt_config[i].channel.width, BW_STR),
                rtt_config[i].num_burst,
                rtt_config[i].burst_period,
                rtt_config[i].burst_duration,
                rtt_config[i].num_frames_per_burst);

    log_verbose(TAG, "%s: retries_per_measurement_frame(%u), retries_per_ftmr(%u),"
                     " LCI(%u), LCR(%u), preamble(0x%x), bw(0x%x) ASAP(%d) force_legacy_ack(%d)",
                __FUNCTION__,
                rtt_config[i].num_retries_per_rtt_frame,
                rtt_config[i].num_retries_per_ftmr,
                rtt_config[i].LCI_request,
                rtt_config[i].LCR_request,
                rtt_config[i].preamble,
                rtt_config[i].bw,
                rtt_config[i].asap,
                rtt_config[i].force_legacy_ack);

    log_verbose(TAG, "%s: 11az params: TMR type (%u) Security type (%u)"
                 "tbPriority (%u) tmrMinDelta (%u) tmrMaxDelta (%u) isRttBgNode (%d)"
                 "tbPeriodicity (%d) tbDuration (%d) tbMaxSessionExpiry (%d)"
                 "I2RFbPolicy (%d) rttChannelSwitchPriority (%d)",
                __FUNCTION__, rtt_config[i].tmrtype, rtt_config[i].sectype,
                rtt_config[i].tbPriority, rtt_config[i].tmrMinDelta,
                rtt_config[i].tmrMaxDelta, rtt_config[i].isRttBgNode,
                rtt_config[i].tbPeriodicity, rtt_config[i].tbDuration,
                rtt_config[i].tbMaxSessionExpiry, rtt_config[i].I2RFbPolicy,
                rtt_config[i].rttChannelSwitchPriority);
  }
}

/*========================================================================================
 * printCapabilitiesInfo
 *
 * Description:
 *   This function prints capabilities information when capabilities response is received
 *
 * Parameters:
 * cap_resp: ptr to capabilities response
 *
 * Return value:
 ========================================================================================*/
static void printCapabilitiesInfo(LOWICapabilityResponse* cap_resp)
{
  log_info (TAG, "%s: discovery_scan_supported(%d) ranging_scan_supported(%d)"
            "active_scan_supported(%d) bgscan_supported(%d) scans_supported(0x%x)",
            __FUNCTION__,
            cap_resp->getCapabilities().discoveryScanSupported,
            cap_resp->getCapabilities().rangingScanSupported,
            cap_resp->getCapabilities().activeScanSupported,
            cap_resp->getCapabilities().bgscanSupported,
            cap_resp->getCapabilities().supportedCapablities);

  log_info (TAG, "%s: one_sided_ranging_supported(%d) dual_sided_ranging_supported_11mc(%d) "
            "bwSupport(0x%x) preambleSupport(0x%x) mcVersion(%d) rxAllChainSupport(%d)",
            __FUNCTION__,
            cap_resp->getCapabilities().oneSidedRangingSupported,
            cap_resp->getCapabilities().dualSidedRangingSupported11mc,
            cap_resp->getCapabilities().bwSupport,
            cap_resp->getCapabilities().preambleSupport,
            cap_resp->getCapabilities().mcVersion,
            cap_resp->getCapabilities().rxAllChainSupport);
}

/*========================================================================================
 * wifiNodeParamChecking
 *
 * Description:
 *   This function checks the configuration parameters for each wifi node passed by the
 *   client for ranging. It rejects the entire request if any parameters are not according
 *   to the api (rtt.h)
 *
 * Parameters:
 * req_id: request id for request sent by client
 * numBssids: number of wifi nodes in the request
 * rtt_config: array of wifi nodes in the ranging request
 *
 * Return value: 0: all configuration parameters for all wifi nodes are correct
 *              -1: all/some configuration parameter is incorrect
 ========================================================================================*/
static int wifiNodeParamChecking(uint32 request_id, uint32 numBssids, lowi_rtt_config rtt_config[])
{
  log_debug(TAG, "%s: Request_id: %u, numBssids : %u", __FUNCTION__, request_id, numBssids);
  int retVal = 0;

  for (uint32 ii = 0; ii < numBssids; ii++)
  {
    // rtt check
    if ((rtt_config[ii].type != RTT_TYPE_1_SIDED) && (rtt_config[ii].type != RTT_TYPE_2_SIDED))
    {
      log_debug(TAG, "%s: Unknown RTT type %d", __FUNCTION__, rtt_config[ii].type);
      retVal = -1;
      break;
    }

    // RTT peer type check
    if (rtt_config[ii].peer != RTT_PEER_AP &&
        rtt_config[ii].peer != RTT_PEER_NAN &&
        rtt_config[ii].peer != RTT_PEER_STA)
    {
      log_debug(TAG, "%s: Bad RTT peer %d", __FUNCTION__, rtt_config[ii].peer);
      retVal = -1;
      break;
    }

    // num_burst check
    // only single-shot requests will be considered
    if (rtt_config[ii].num_burst != 0)
    {
      log_debug(TAG, "%s: num_burst (%d) not 0", __FUNCTION__, rtt_config[ii].num_burst);
      retVal = -1;
      break;
    }

    // burst_period check
    if (rtt_config[ii].burst_period != 0)
    {
      log_debug(TAG, "%s: burst_period (%d) set to 0", __FUNCTION__, rtt_config[ii].burst_period);
    }

    // num_frames_per_burst check
    if (rtt_config[ii].num_frames_per_burst < minFramesPerBurst ||
        rtt_config[ii].num_frames_per_burst > maxFramesPerBurst)
    {
      log_debug(TAG, "%s: num_frames_per_burst (%d) out of range", __FUNCTION__,
                rtt_config[ii].num_frames_per_burst);
      retVal = -1;
      break;
    }

    // num_retries_per_rtt_frame check
    if (rtt_config[ii].type == RTT_TYPE_1_SIDED)
    {
      if (rtt_config[ii].num_retries_per_rtt_frame > maxRetriesPerRttFrame)
      {
        log_debug(TAG, "%s: num_retries_per_rtt_frame (%d) out of range", __FUNCTION__,
                  rtt_config[ii].num_retries_per_rtt_frame);
        retVal = -1;
        break;
      }
    }

    // num_retries_per_ftmr check
    if (rtt_config[ii].type == RTT_TYPE_2_SIDED)
    {
      if (rtt_config[ii].num_retries_per_ftmr > maxRetriesPerFtmr)
      {
        log_debug(TAG, "%s: num_retries_per_ftmr (%d) out of range", __FUNCTION__,
                  rtt_config[ii].num_retries_per_ftmr);
        retVal = -1;
        break;
      }
    }

    // LCI and LCR request check
    if ( (rtt_config[ii].LCI_request != 0) && (rtt_config[ii].LCI_request != 1) )
    {
      log_debug(TAG, "%s: LCI_Request bad value", __FUNCTION__, rtt_config[ii].LCI_request);
      retVal = -1;
      break;
    }
    if ( (rtt_config[ii].LCR_request != 0) && (rtt_config[ii].LCR_request != 1) )
    {
      log_debug(TAG, "%s: LCR_Request bad value", __FUNCTION__, rtt_config[ii].LCR_request);
      retVal = -1;
      break;
    }

    // burst_duration check
    if ((rtt_config[ii].burst_duration < minBurstDuration) || (rtt_config[ii].burst_duration > maxBurstDuration))
    {
      // check for no preference option
      if (rtt_config[ii].type == RTT_TYPE_1_SIDED && rtt_config[ii].burst_duration == burstDurationNoPreference)
      {
        log_debug(TAG, "%s: 1-sided RTT, got illegal burst_duration (%d). "
                       "Set burst duration to maxBurstDuration(%u)",
                  __FUNCTION__, rtt_config[ii].burst_duration, maxBurstDuration);
      }
      else if (rtt_config[ii].type == RTT_TYPE_2_SIDED && rtt_config[ii].burst_duration == burstDurationNoPreference)
      {
        log_debug(TAG, "%s: 2-sided RTT, burst_duration no preference", __FUNCTION__);
      }
      else
      {
        log_debug(TAG, "%s: burst_duration out of range %d", __FUNCTION__,
                  rtt_config[ii].burst_duration);
        retVal = -1;
        break;
      }
    }

    // preamble check
    if (rtt_config[ii].preamble != LOWI_RTT_PREAMBLE_LEGACY &&
        rtt_config[ii].preamble != LOWI_RTT_PREAMBLE_HT &&
        rtt_config[ii].preamble != LOWI_RTT_PREAMBLE_VHT &&
        rtt_config[ii].preamble != LOWI_RTT_PREAMBLE_HE)
    {
      log_debug(TAG, "%s: preamble out of range %d", __FUNCTION__,
                rtt_config[ii].preamble);
      retVal = -1;
      break;
    }

    // bw check
    if (rtt_config[ii].bw != LOWI_RTT_BW_20 &&
        rtt_config[ii].bw != LOWI_RTT_BW_40 &&
        rtt_config[ii].bw != LOWI_RTT_BW_80 &&
        rtt_config[ii].bw != LOWI_RTT_BW_160)
    {
      log_debug(TAG, "%s: bw out of range %d", __FUNCTION__, rtt_config[ii].bw);
      retVal = -1;
      break;
    }
  }
  return retVal;
}


/*========================================================================================
 * setRttCapabilities
 *
 * Description:
 *   This function sets the RTT capabilities into a global variable.
 *
 * Parameters:
 * caps: capabilities returned by lowi-server
 *
 * Return value: 0: success
 *              -1: failure
 ========================================================================================*/
static int setRttCapabilities(LOWICapabilities &caps)
{
  int retVal = 0;

  log_verbose(TAG, "%s: filling in rttCachedCapabilities", __FUNCTION__);
  do
  {
    rttCachedCapabilities = (lowi_rtt_capabilities *)malloc(sizeof(lowi_rtt_capabilities));
    if (NULL == rttCachedCapabilities)
    {
      log_warning(TAG, "%s: memory allocation failure", __FUNCTION__);
      retVal = -2;
      break;
    }

    if (true == caps.rangingScanSupported)
    {
      rttCachedCapabilities->rtt_one_sided_supported = (byte)caps.oneSidedRangingSupported;
      rttCachedCapabilities->rtt_ftm_supported   = (byte)caps.dualSidedRangingSupported11mc;
      rttCachedCapabilities->bw_support          = setBwCapabilityMask(caps.bwSupport);
      rttCachedCapabilities->preamble_support    = (byte)caps.preambleSupport;
      rttCachedCapabilities->lci_support         = 1;
      rttCachedCapabilities->lcr_support         = 1;
#if LOWI_MAJOR_VERSION >= 2
      rttCachedCapabilities->mc_version          = (byte)caps.mcVersion;
      rttCachedCapabilities->responder_supported = 0;
      if(caps.supportedCapablities & LOWI_RTT_RESPONDER_SUPPORTED)
      {
        rttCachedCapabilities->responder_supported = 1;
      }
#endif
      rttCachedCapabilities->rxAllChainSupport = caps.rxAllChainSupport;
    }
    else
    {
      rttCachedCapabilities->rtt_one_sided_supported = 0;
      rttCachedCapabilities->rtt_ftm_supported       = 0;
      rttCachedCapabilities->preamble_support        = 0;
      rttCachedCapabilities->bw_support              = 0;
      rttCachedCapabilities->lci_support             = 0;
      rttCachedCapabilities->lcr_support             = 0;
#if LOWI_MAJOR_VERSION >= 2
      rttCachedCapabilities->responder_supported     = 0;
#endif
      rttCachedCapabilities->rxAllChainSupport = 0;
    }
  } while (0);

  return retVal;
}

/*========================================================================================
 * setBwCapabilityMask
 *
 * Description:
 *   This function sets bw capability mask depending on what the highest BW supported by
 *   the FW is.
 *
 * Parameters:
 * maxBw: max bw supported by FW
 *
 * Return value: bw capability mask
 ========================================================================================*/
static byte setBwCapabilityMask(uint8 maxBw)
{
  byte bw = 0;
  switch (maxBw)
  {
    case BW_160MHZ:
    {
      bw = LOWI_RTT_BW_20 | LOWI_RTT_BW_40 | LOWI_RTT_BW_80 | LOWI_RTT_BW_160;
      break;
    }
    case BW_80MHZ:
    {
      bw = LOWI_RTT_BW_20 | LOWI_RTT_BW_40 | LOWI_RTT_BW_80;
      break;
    }
    case BW_40MHZ:
    {
      bw = LOWI_RTT_BW_20 | LOWI_RTT_BW_40;
      break;
    }
    case BW_20MHZ:
    {
      bw = LOWI_RTT_BW_20;
      break;
    }
    default:
    {
      bw = 0;
      break;
    }
  }
  return bw;
}
