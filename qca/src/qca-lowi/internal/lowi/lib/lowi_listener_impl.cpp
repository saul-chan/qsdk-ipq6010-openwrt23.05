/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

        LOWI Client Listener Implementation

GENERAL DESCRIPTION
  This file contains the implementation for the LOWIClientListener
  used in the LOWI interface for RTT and BGSCAN measurements

Copyright (c) 2014,2016-2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
=============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cmath>
#include <base_util/log.h>
#include <common/lowi_utils.h>
#include <base_util/config_file.h>
#include <inc/lowi_response.h>
#include <inc/lowi_external.h>
#include "lowi_listener_impl.h"
#include <algorithm>

/** This is the minimum RTT value provided to the user */
#define RTT3_MIN_DEFAULT 1

// msg strings used for printing/debugging PREAMBLE from LOWI
const char* PREAMBLE_STR[4] = {
  "PREAMBLE_LEGACY",
  "PREAMBLE_CCK",
  "PREAMBLE_HT",
  "PREAMBLE_VHT"
};

// msg strings used for printing/debugging RTT type from LOWI
const char* RTT_STR[3] = {
  "RTT1",
  "RTT2",
  "RTT3",
};

// msg strings used for printing/debugging BW type from LOWI
const char* BW_STR[4] = {
  "BW_20MHz",
  "BW_40MHZ",
  "BW_80MHZ",
  "BW_160MHZ"
};

// msg strings used for printing/debugging wifi node type from LOWI
const char* NODE_STR[3] = {
  "ACCESS_POINT",
  "PEER_DEVICE",
  "NAN_DEVICE"
};

// msg strings used for printing/debugging wifi node type
const char* LOWI_RTT_STR[2] = {
  "RTT_TYPE_1_SIDED",
  "RTT_TYPE_2_SIDED",
};

// msg strings used for printing/debugging wifi node type
const char* LOWI_PEER_STR[7] = {
   "WIFI_PEER_STA",
   "WIFI_PEER_AP",
   "WIFI_PEER_P2P_GO",
   "WIFI_PEER_P2P_CLIENT",
   "WIFI_PEER_NAN",
   "WIFI_PEER_TDLS",
   "WIFI_PEER_INVALID"
};

// msg strings used for printing/debugging
const char* LOWI_PREAMBLE_STR[6] = {
  "OFDM",
  "CCK",
  "HT",
  "VHT",
  "HE",
  "MAX"
};

// msg strings used for printing/debugging
const char* LOWI_NSS_STR[4] = {
  "1x1",
  "2x2",
  "3x3",
  "4x4"
};

// msg strings used for printing/debugging
const char *LOWI_STATUS[RTT_STATUS_NAN_RANGING_CONCURRENCY_NOT_SUPPORTED+1] = {
  "RTT_STATUS_SUCCESS",
  "RTT_STATUS_FAILURE",
  "RTT_STATUS_FAIL_NO_RSP",
  "RTT_STATUS_FAIL_REJECTED",
  "RTT_STATUS_FAIL_NOT_SCHEDULED_YET",
  "RTT_STATUS_FAIL_TM_TIMEOUT",
  "RTT_STATUS_FAIL_AP_ON_DIFF_CHANNEL",
  "RTT_STATUS_FAIL_NO_CAPABILITY",
  "RTT_STATUS_ABORTED",
  "RTT_STATUS_FAIL_INVALID_TS",
  "RTT_STATUS_FAIL_PROTOCOL",
  "RTT_STATUS_FAIL_SCHEDULE",
  "RTT_STATUS_FAIL_BUSY_TRY_LATER",
  "RTT_STATUS_INVALID_REQ",
  "RTT_STATUS_NO_WIFI",
  "RTT_STATUS_FAIL_FTM_PARAM_OVERRIDE",
  "RTT_STATUS_NAN_RANGING_PROTOCOL_FAILURE",
  "RTT_STATUS_NAN_RANGING_CONCURRENCY_NOT_SUPPORTED",
};

// msg strings used for printing/debugging
const char *TARGET_STATUS[LOWIScanMeasurement::LOWI_TARGET_STATUS_MAX+1] = {
  "LOWI_TARGET_STATUS_SUCCESS",
  "LOWI_TARGET_STATUS_FAILURE",
  "LOWI_TARGET_STATUS_RTT_FAIL_NO_RSP",
  "LOWI_TARGET_STATUS_RTT_FAIL_REJECTED",
  "LOWI_TARGET_STATUS_RTT_FAIL_FTM_TIMEOUT",
  "LOWI_TARGET_STATUS_RTT_TARGET_ON_DIFF_CHANNEL",
  "LOWI_TARGET_STATUS_RTT_FAIL_TARGET_NOT_CAPABLE",
  "LOWI_TARGET_STATUS_RTT_FAIL_INVALID_TS",
  "LOWI_TARGET_STATUS_RTT_FAIL_TARGET_BUSY_TRY_LATER",
  "LOWI_TARGET_STATUS_RTT_NAN_RANGING_PROTOCOL_FAILURE",
  "LOWI_TARGET_STATUS_RTT_NAN_RANGING_CONCURRENCY_NOT_SUPPORTED",
  "LOWI_TARGET_STATUS_MAX"
};

const char* CHANNEL_WIDTH_BW[5] = {
   "WIFI_CHAN_WIDTH_20",
   "WIFI_CHAN_WIDTH_40",
   "WIFI_CHAN_WIDTH_80",
   "WIFI_CHAN_WIDTH_160",
   "WIFI_CHAN_WIDTH_INVALID"
};

const char *CHANNEL_BW(u32 bw)
{
    switch (bw) {
        case LOWI_RTT_BW_20:
            return "BW_20";
        case LOWI_RTT_BW_40:
            return "BW_40";
        case LOWI_RTT_BW_80:
            return "BW_80";
        case LOWI_RTT_BW_160:
            return "BW_160";
        default:
            return "BW_INVALID";
    }
}

extern vector <struct req_map> g_req_cache;
extern int32 g_discard_bw_mis_match;
extern int32 g_weighted_mean;
extern lowi_rtt_capabilities *rttCachedCapabilities;

//  preamble types
typedef enum {
  OFDM,
  CCK,
  HT,
  VHT,
  HE,
  MAX
} wifi_preamble;

// nss types
typedef enum {
  nss_1x1,
  nss_2x2,
  nss_3x3,
  nss_4x4
} wifi_nss;

using namespace qc_loc_fw;

const char * const LOWIClientListenerImpl::TAG = "LOWIClientListener";
/*
 * Packet BW (MHz)  OTARangeErrorSTDKPI(m)  RTT KPI (ns)  Recommended STD Th (ns)
 * 80+80            0.84                    5.60          28.00 (5.60 * 5 )
 * 80               1.05                    7.00          35.00 ( 7 * 5 )
 * 40               1.39                    9.27          46.33 (9.27 * 5 )
 * 20               3.15                    21.00         105.00 ( 21 * 5 )
 *
 * standard deviation of 6m, 3m, 1.5m, 1m for different bandwidths.
 * Values are in nano sec's
 */
const int32 LOWIClientListenerImpl::RTT_STDEV[qc_loc_fw::BW_MAX] =
  {21000, 9270, 7000, 5600};

extern lowi_rtt_event_handler* getRttHandler(uint32 reqId);
extern int getReqId(uint32 reqId);
extern void cleanupRttHandlers();
extern LOWIClient* client;

// convenience macro for releasing rtt results memory
#define FREE_RTT_RESULTS(rtt_results) if (NULL != rtt_results) \
    {                               \
      if (NULL != rtt_results->LCI) \
      {                             \
        free(rtt_results->LCI);     \
      }                             \
      if (NULL != rtt_results->LCR) \
      {                             \
        free(rtt_results->LCR);     \
      }                             \
      free(rtt_results);            \
    }


LOWIClientListenerImpl::~LOWIClientListenerImpl ()
{
  log_verbose (TAG, "~LOWIClientListenerImpl");
}

void LOWIClientListenerImpl::responseReceived(LOWIResponse* response)
{
  if(NULL == response)
  {
    log_error (TAG, "%s: invalid response: NULL pointer received ", __func__);
    return;
  }

  log_verbose (TAG, "%s: lowiReqid(%u)", __FUNCTION__, response->getRequestId());

  if (response->getResponseType() == LOWIResponse::RANGING_SCAN)
  {
    LOWIRangingScanResponse* resp = (LOWIRangingScanResponse*) response;
    vector <LOWIScanMeasurement*> scanMeasurements = resp->scanMeasurements;
    printLowiResponse (scanMeasurements);

    uint32 numRttResults = resp->scanMeasurements.getNumOfElements();

    // convert from measurements returned by LOWI to struct used by
    lowi_rtt_result *rtt_results = scanMeasToResult(resp->scanMeasurements,
                                                           resp->scanStatus);

    // get callback to return the measurements
    lowi_rtt_event_handler* rttHandler;
    rttHandler = getRttHandler(response->getRequestId());
    if (NULL == rttHandler)
    {
      log_error (TAG, "@responseReceived(): got a rsp but NULL handler...drop the rsp");

      FREE_RTT_RESULTS(rtt_results)
      return;
    }

    // get the proper request id
    if( -1 == getReqId(response->getRequestId()) )
    {
      log_error (TAG, "@responseReceived(): no reqid for this rsp...drop the rsp");
      FREE_RTT_RESULTS(rtt_results)
      return;
    }

    lowi_rtt_result **rttResultsPtr = NULL;
    // get memory for the results
    rttResultsPtr = (lowi_rtt_result**)malloc( sizeof(lowi_rtt_result*) * numRttResults );
    if (NULL == rttResultsPtr)
    {
      log_error (TAG, "@responseReceived(): memory allocation failure...drop the rsp");
      FREE_RTT_RESULTS(rtt_results)
      return;
    }

    for (uint32 ii = 0; ii < numRttResults; ii++)
    {
      rttResultsPtr[ii] = &rtt_results[ii];
    }

    // all good...call the handler...
    log_verbose(TAG, "@responseReceived(): calling rtt results callback(%p)",
                &rttHandler->on_rtt_results);
    rttHandler->on_rtt_results(getReqId(response->getRequestId()),
                               resp->scanMeasurements.getNumOfElements(),
                               rttResultsPtr);

    // release the memory
    FREE_RTT_RESULTS(rtt_results)
    free(rttResultsPtr); // free the array of ptrs

    cleanupRttHandlers();
  }

}

lowi_rtt_result* LOWIClientListenerImpl::
scanMeasToResult(vector <LOWIScanMeasurement*> & scanMeasurements,
                        LOWIResponse::eScanStatus scanStatus)
{
  // calculate memory for results
  lowi_rtt_result *rtt_results // freed by caller
    = (lowi_rtt_result*)calloc(scanMeasurements.getNumOfElements(), sizeof(lowi_rtt_result));
  struct req_map current_node_map;

  if( NULL == rtt_results )
  {
    log_error(TAG, "@scanMeasToResult(): memory allocation error");
    return rtt_results;
  }

  lowi_rtt_result *nodeResult = (lowi_rtt_result *)rtt_results;
  for(unsigned int ii = 0; ii < scanMeasurements.getNumOfElements(); ii++)
  {
    LOWIScanMeasurement* node = scanMeasurements[ii];

    for (int jj = 0; jj < BSSID_LEN; ++jj)
    {
      nodeResult->addr[jj] = node->bssid[jj];
    }

    if (g_discard_bw_mis_match)
    {
      for (uint32 index = 0; index < g_req_cache.getNumOfElements(); index++)
      {
        if (node->bssid.compareTo(g_req_cache[index].bssid) == 0)
        {
          current_node_map = g_req_cache[index];
          log_debug(TAG, "requstedBW for node %u", current_node_map.requestedBW);
          current_node_map.bssid.print();
          break;
        }
      }
    }
    // get measurement status
    mapLowiStatusToStatus(nodeResult, node, scanStatus);

    mapRttType(nodeResult, node);

    computeRssiMetrics(nodeResult, node);

    if (rttCachedCapabilities->rxAllChainSupport)
        computeRssiMetrics_perchain(nodeResult, node);

    processTxRate(nodeResult, node);

    processRxRate(nodeResult, node);

    computeDistanceMetrics(nodeResult, node, current_node_map);

    // time of the measurement (in microseconds since boot)
    nodeResult->ts = get_time_boot_ms() * 1000;

    // add the measurement number
    nodeResult->burst_num = node->measurementNum;

    //default values for other parameters
    nodeResult->measurement_number    = node->num_frames_attempted;
    nodeResult->tot_successful_meas   = node->measurementsInfo.getNumOfElements();

    if (!nodeResult->meas_as_per_config)
         nodeResult->meas_as_per_config = node->measurementsInfo.getNumOfElements();

    nodeResult->number_per_burst_peer = node->negotiated_num_frames_per_burst;
    nodeResult->retry_after_duration  = node->retry_after_duration;
    nodeResult->burst_duration        = node->actual_burst_duration;
    nodeResult->negotiated_burst_num  = node->negotiated_burst_exp;
    nodeResult->tmr_subtype           = node->tmr_subtype;
    nodeResult->security_mode         = node->security_mode;

  /* 11az params */
    nodeResult->i2r_lmr_feedback = node->i2r_lmr_feedback;
    nodeResult->ranging_priority = node->ranging_priority;
    nodeResult->r2i_toa_type = node->r2i_toa_type;
    nodeResult->i2r_toa_type = node->i2r_toa_type;
    nodeResult->format_and_bw = node->format_and_bw;
    nodeResult->imdt_r2i_feedback = node->imdt_r2i_feedback;
    nodeResult->imdt_i2r_feedback = node->imdt_i2r_feedback;
    nodeResult->max_i2r_repetition = node->max_i2r_repetition;
    nodeResult->max_r2i_repetition = node->max_r2i_repetition;
    nodeResult->device_class = node->device_class;
    nodeResult->full_bw_ul_mumimo = node->full_bw_ul_mumimo;
    nodeResult->max_r2i_sts_le_80mhz = node->max_r2i_sts_le_80mhz;
    nodeResult->max_r2i_sts_gr_80mhz = node->max_r2i_sts_gr_80mhz;
    nodeResult->max_r2i_ltf_total = node->max_r2i_ltf_total;
    nodeResult->max_i2r_sts_le_80mhz = node->max_i2r_sts_le_80mhz;
    nodeResult->max_i2r_sts_gr_80mhz = node->max_i2r_sts_gr_80mhz;
    nodeResult->max_i2r_ltf_total = node->max_i2r_ltf_total;
    nodeResult->min_time_between_measurements = node->min_time_between_measurements;

    if ((nodeResult->rtt == 0) && (nodeResult->tot_successful_meas != 0))
    {
      // All measurements were outlier
      nodeResult->tot_successful_meas = 0;
      nodeResult->status         = RTT_STATUS_FAIL_INVALID_TS;
    }
    processLocationIeData(nodeResult, node);

    printNodeResult(nodeResult);
    nodeResult = nodeResult + 1;
  }

  return rtt_results;
}

void LOWIClientListenerImpl::printLowiResponse(vector <LOWIScanMeasurement*> & scanMeasurements)
{
  for( unsigned int ii = 0; ii < scanMeasurements.getNumOfElements(); ++ii )
  {
    LOWIScanMeasurement* scan = scanMeasurements[ii];
    scan->bssid.print();
    log_debug (TAG, "Frequency = %d, Is Secure = %d, Node type = %d", scan->frequency, scan->isSecure, scan->type);

    vector<LOWIMeasurementInfo*> measurements = scan->measurementsInfo;
    for( unsigned int jj = 0; jj < measurements.getNumOfElements(); ++jj )
    {
      LOWIMeasurementInfo * meas = measurements[jj];
      log_debug (TAG, "RSSI = %d, RSSI_TIMESTAMP = %llu, RTT = %dps, RTT_TIMESTAMP = %llu",
                 meas->rssi,
                 meas->rssi_timestamp,
                 meas->rtt_ps,
                 meas->rtt_timestamp);
      log_debug (TAG, "PREAMBLE = %d, BW = %d, MCS_IDX = %d, BIT_RATE = %d",
                 meas->tx_preamble,
                 meas->tx_bw,
                 meas->tx_mcsIdx,
                 meas->tx_bitrate);

      if (rttCachedCapabilities->rxAllChainSupport){
          for( unsigned int kk = 0; kk < 4; kk++)
          {
              log_debug (TAG, "RSSI_PER_CHAIN = %d, RTT_PS_PER_CHAIN = %d, RTT_PER_CHAIN = %d",
                      meas->perchinfo[kk].rssi_perchain,
                      meas->perchinfo[kk].rtt_ps_perchain,
                      meas->perchinfo[kk].rtt_perchain);
          }
      }
    }
  }
}

void LOWIClientListenerImpl::
printNodeResult(lowi_rtt_result* rtt_node_result)
{
  log_debug(TAG, "@printNodeResult(): BSSID: %02x:%02x:%02x:%02x:%02x:%02x, timestamp:(%llu)",
            rtt_node_result->addr[0],
            rtt_node_result->addr[1],
            rtt_node_result->addr[2],
            rtt_node_result->addr[3],
            rtt_node_result->addr[4],
            rtt_node_result->addr[5],
            (lowi_timespan)rtt_node_result->ts);

  if( (rtt_node_result->status < RTT_STATUS_SUCCESS) ||
      (rtt_node_result->status > RTT_STATUS_FAIL_FTM_PARAM_OVERRIDE) )
  {
    log_debug(TAG, "@printNodeResult(): status out of range");
  }
  else if( (rtt_node_result->type != RTT_TYPE_1_SIDED) &&
           (rtt_node_result->type != RTT_TYPE_2_SIDED) )
  {
    log_debug(TAG, "@printNodeResult(): rtt out of range");
  }
  else
  {
    log_debug(TAG, "@printNodeResult(): measurement_num(%u), status(%s), type(%s)",
              rtt_node_result->burst_num,
              LOWI_TO_STRING(rtt_node_result->status, LOWI_STATUS),
              LOWI_TO_STRING(rtt_node_result->type-1, LOWI_RTT_STR));
  }

  // only successful nodes will have measurement contents
  if( rtt_node_result->status ==  RTT_STATUS_SUCCESS )
  {
    log_error(TAG, "%s: rssi(%d), rssi_spread(%d), rtt(%llu), rtt_sd(%llu), rtt_spread(%llu)",
            __func__,
            (lowi_rssi)rtt_node_result->rssi,
            (lowi_rssi)rtt_node_result->rssi_spread,
            (lowi_timespan)rtt_node_result->rtt,
            (lowi_timespan)rtt_node_result->rtt_sd,
            (lowi_timespan)rtt_node_result->rtt_spread);
  }

  if( rtt_node_result->tx_rate.preamble >= MAX )
  {
    log_debug(TAG, "@printNodeResult(): preamble out of range");
  }
  else if( (rtt_node_result->tx_rate.nss < 0) ||
           (rtt_node_result->tx_rate.nss > 3) )
  {
    log_debug(TAG, "@printNodeResult(): nss out of range");
  }
  else if((int)rtt_node_result->tx_rate.bw >= LOWI_RTT_BW_160)
  {
    log_debug(TAG, "@printNodeResult(): tx_rate info: preamble(%s), nss(%s), bw(%s), mcs_idx(%u), bitrate(%u)",
          LOWI_TO_STRING(rtt_node_result->tx_rate.preamble, LOWI_PREAMBLE_STR),
          LOWI_TO_STRING(rtt_node_result->tx_rate.nss, LOWI_NSS_STR),
          "WIFI_CHAN_WIDTH_INVALID",
          rtt_node_result->tx_rate.rateMcsIdx,
          rtt_node_result->tx_rate.bitrate);
  }
  else
  {
    log_debug(TAG, "@printNodeResult(): tx_rate info: preamble(%s), nss(%s), bw(%s), mcs_idx(%u), bitrate(%u)",
              LOWI_TO_STRING(rtt_node_result->tx_rate.preamble, LOWI_PREAMBLE_STR),
              LOWI_TO_STRING(rtt_node_result->tx_rate.nss, LOWI_NSS_STR),
              CHANNEL_BW(rtt_node_result->tx_rate.bw),
              rtt_node_result->tx_rate.rateMcsIdx,
              rtt_node_result->tx_rate.bitrate);
  }

  log_debug(TAG, "@printNodeResult(): distance_mm(%d), distance_spread_mm(%d), distance_sd_mm(%d)",
            rtt_node_result->distance_mm,
            rtt_node_result->distance_spread_mm,
            rtt_node_result->distance_sd_mm);

  log_debug(TAG, "11az params: tmr_subtype:%u security_mode:%u "
                 "i2r_lmr_feedback:%u ranging_priority:%u "
                 "r2i_toa_type:%u i2r_toa_type:%u format_and_bw:%u "
                 "imdt_r2i_feedback:%u imdt_i2r_feedback:%u "
                 "max_i2r_repetition:%u max_r2i_repetition:%u device_class:%u "
                 "full_bw_ul_mumimo:%u max_r2i_sts_le_80mhz:%u "
                 "max_r2i_sts_gr_80mhz:%u max_r2i_ltf_total:%u "
                 "max_i2r_sts_le_80mhz:%u max_i2r_sts_gr_80mhz:%u "
                 "max_i2r_ltf_total:%u min_time_between_measurements:%u",
                 rtt_node_result->tmr_subtype,
                 rtt_node_result->security_mode,
                 rtt_node_result->i2r_lmr_feedback,
                 rtt_node_result->ranging_priority,
                 rtt_node_result->r2i_toa_type,
                 rtt_node_result->i2r_toa_type,
                 rtt_node_result->format_and_bw,
                 rtt_node_result->imdt_r2i_feedback,
                 rtt_node_result->imdt_i2r_feedback,
                 rtt_node_result->max_i2r_repetition,
                 rtt_node_result->max_r2i_repetition,
                 rtt_node_result->device_class,
                 rtt_node_result->full_bw_ul_mumimo,
                 rtt_node_result->max_r2i_sts_le_80mhz,
                 rtt_node_result->max_r2i_sts_gr_80mhz,
                 rtt_node_result->max_r2i_ltf_total,
                 rtt_node_result->max_i2r_sts_le_80mhz,
                 rtt_node_result->max_i2r_sts_gr_80mhz,
                 rtt_node_result->max_i2r_ltf_total,
                 rtt_node_result->min_time_between_measurements);
}

void LOWIClientListenerImpl::
mapLowiStatusToStatus(lowi_rtt_result* rtt_node,
                             LOWIScanMeasurement* info,
                             LOWIResponse::eScanStatus scanStatus)
{
  // We need to add status on a per wifi node basis. For each wifi node, translate from
  // lowi status to status.
  log_verbose(TAG, "%s: status IN(%d)(%s) ReqStatus(%d)", __FUNCTION__,
              (int)info->targetStatus,
              LOWI_TO_STRING(info->targetStatus, TARGET_STATUS),
              scanStatus);
  switch (scanStatus)
  {
    case LOWIResponse::SCAN_STATUS_SUCCESS:
      switch (info->targetStatus)
      {
        case LOWIScanMeasurement::LOWI_TARGET_STATUS_SUCCESS:                                    rtt_node->status = RTT_STATUS_SUCCESS;                                    break;
        case LOWIScanMeasurement::LOWI_TARGET_STATUS_FAILURE:                                    rtt_node->status = RTT_STATUS_FAILURE;                                    break;
        case LOWIScanMeasurement::LOWI_TARGET_STATUS_RTT_FAIL_NO_RSP:                            rtt_node->status = RTT_STATUS_FAIL_NO_RSP;                                break;
        case LOWIScanMeasurement::LOWI_TARGET_STATUS_RTT_FAIL_REJECTED:                          rtt_node->status = RTT_STATUS_FAIL_REJECTED;                              break;
        case LOWIScanMeasurement::LOWI_TARGET_STATUS_RTT_FAIL_FTM_TIMEOUT:                       rtt_node->status = RTT_STATUS_FAIL_TM_TIMEOUT;                            break;
        case LOWIScanMeasurement::LOWI_TARGET_STATUS_RTT_TARGET_ON_DIFF_CHANNEL:                 rtt_node->status = RTT_STATUS_FAIL_AP_ON_DIFF_CHANNEL;                    break;
        case LOWIScanMeasurement::LOWI_TARGET_STATUS_RTT_FAIL_TARGET_NOT_CAPABLE:                rtt_node->status = RTT_STATUS_FAIL_NO_CAPABILITY;                         break;
        case LOWIScanMeasurement::LOWI_TARGET_STATUS_RTT_FAIL_INVALID_TS:                        rtt_node->status = RTT_STATUS_FAIL_INVALID_TS;                            break;
        case LOWIScanMeasurement::LOWI_TARGET_STATUS_RTT_FAIL_TARGET_BUSY_TRY_LATER:             rtt_node->status = RTT_STATUS_FAIL_BUSY_TRY_LATER;                        break;
        case LOWIScanMeasurement::LOWI_TARGET_STATUS_RTT_NAN_RANGING_PROTOCOL_FAILURE:           rtt_node->status = RTT_STATUS_NAN_RANGING_PROTOCOL_FAILURE;               break;
        case LOWIScanMeasurement::LOWI_TARGET_STATUS_RTT_NAN_RANGING_CONCURRENCY_NOT_SUPPORTED:  rtt_node->status = RTT_STATUS_NAN_RANGING_CONCURRENCY_NOT_SUPPORTED;      break;
        default:
          rtt_node->status = RTT_STATUS_FAILURE;
          log_debug(TAG, "%s: Status type unknown", __FUNCTION__);
        break;
      }
      break;
    case LOWIResponse::SCAN_STATUS_INVALID_REQ:
      rtt_node->status = RTT_STATUS_INVALID_REQ;
      break;
    case LOWIResponse::SCAN_STATUS_NOT_SUPPORTED:
      rtt_node->status = RTT_STATUS_FAIL_NO_CAPABILITY;
      break;
    case LOWIResponse::SCAN_STATUS_NO_WIFI:
      rtt_node->status = RTT_STATUS_NO_WIFI;
      break;
    case LOWIResponse::SCAN_STATUS_UNKNOWN:
    case LOWIResponse::SCAN_STATUS_BUSY:
    case LOWIResponse::SCAN_STATUS_DRIVER_ERROR:
    case LOWIResponse::SCAN_STATUS_DRIVER_TIMEOUT:
    case LOWIResponse::SCAN_STATUS_INTERNAL_ERROR:
    case LOWIResponse::SCAN_STATUS_TOO_MANY_REQUESTS:
    case LOWIResponse::SCAN_STATUS_OUT_OF_MEMORY:
    default:
      rtt_node->status = RTT_STATUS_FAILURE;
      log_debug(TAG, "%s: Status type unknown", __FUNCTION__);
      break;
  }
  log_verbose(TAG, "@mapLowiStatusToStatus(): status OUT(%d)(%s)",
              (int)rtt_node->status,
              LOWI_TO_STRING(rtt_node->status, LOWI_STATUS));
}

void LOWIClientListenerImpl::
mapRttType(lowi_rtt_result* rtt_node, LOWIScanMeasurement* info)
{
  switch(info->rttType)
  {
    case RTT1_RANGING: rtt_node->type = RTT_TYPE_1_SIDED; break;
    case RTT2_RANGING: rtt_node->type = RTT_TYPE_1_SIDED; break;
    case RTT3_RANGING: rtt_node->type = RTT_TYPE_2_SIDED; break;
    default:
      rtt_node->type = RTT_TYPE_1_SIDED;
      log_debug(TAG, "%s: RTT type unknown", __func__);
  }
}

void LOWIClientListenerImpl::
computeRssiMetrics_perchain(lowi_rtt_result* rtt_node, LOWIScanMeasurement* info)
{
    unsigned int i,k;

    double rssi_mwatt;
    int16 rssi_perchain;
    double rssiCum_perchain[4] = {0.0};
    int32 rssiMax_perchain[4] = {0};
    int32 rssiMin_perchain[4] = {0};

    log_verbose(TAG, "%s: NumOfElements:%d",
                __FUNCTION__, info->measurementsInfo.getNumOfElements());

    if(info->measurementsInfo.getNumOfElements() <= 0) {
        log_warning(TAG, "%s: no measurements available to calculate rssi metrics",
                    __FUNCTION__);
        return;
    }

    // iterate through all the chains
    for (k = 0; k < 4; k++)
    {
        rssiMax_perchain[k] = info->measurementsInfo[0]->perchinfo[k].rssi_perchain;
        rssiMin_perchain[k] = info->measurementsInfo[0]->perchinfo[k].rssi_perchain;

        // for all the measurements, iterate through rssi numbers of per chain
        // wise to find the avg, min and max.
        for(i = 0; i < info->measurementsInfo.getNumOfElements(); i++)
        {
            rssi_perchain = info->measurementsInfo[i]->perchinfo[k].rssi_perchain;

            if (rssi_perchain == 0)
                continue;

            // find average rssi by converting to mwatts first
            rssi_mwatt = pow(10, (double)rssi_perchain/20);

            rssiCum_perchain[k] += rssi_mwatt;

            // find min and max rssi
            if(rssi_perchain > rssiMax_perchain[k]) {
                rssiMax_perchain[k] = rssi_perchain;
            } else if (rssi_perchain < rssiMin_perchain[k]) {
                rssiMin_perchain[k] = rssi_perchain;
            }
        }
    }

    for (int ii = 0; ii < 4; ii++)
    {
        rssi_mwatt = rssiCum_perchain[ii]/(double)i; // avg rssi in mwatts
        rtt_node->rssi_perchain[ii] = 10 * log10(rssi_mwatt); // avg rssi in dBm
        rtt_node->rssi_perchain[ii] *= -2; // avg rssi in steps of 0.5dB and positive

        rtt_node->rssi_spread_perchain[ii] = rssiMax_perchain[ii] - rssiMin_perchain[ii];

        log_verbose(TAG, "%s: CHAIN[%d]: avgRssi(%d), rssiMax(%d), rssiMin(%d), rssiSp(%d)",
                __FUNCTION__, ii, rtt_node->rssi_perchain[ii],
                rssiMax_perchain[ii], rssiMin_perchain[ii],
                rtt_node->rssi_spread_perchain[ii]);
    }
}

void LOWIClientListenerImpl::
computeRssiMetrics(lowi_rtt_result* rtt_node, LOWIScanMeasurement* info)
{
  unsigned int i;

  double rssi_mwatt = 0;
  double rssiCum = 0;
  int32 rssiMax = 0;
  int32 rssiMin = 0;

  if(info->measurementsInfo.getNumOfElements() > 0)
  {
    // initial values
    rssiMax = info->measurementsInfo[0]->rssi;
    rssiMin = info->measurementsInfo[0]->rssi;

    // iterate through rssi numbers to find the avg, min and max
    for(i = 0; i < info->measurementsInfo.getNumOfElements(); i++)
    {
      log_verbose(TAG, "@computeRssiMetrics(): rssi[%d] = %d", i, info->measurementsInfo[i]->rssi);

      // find average rssi by converting to mwatts first
      rssi_mwatt = pow(10, (double)info->measurementsInfo[i]->rssi/20);

      rssiCum += rssi_mwatt;

      // find min and max rssi
      if(info->measurementsInfo[i]->rssi > rssiMax)
      {
        rssiMax = info->measurementsInfo[i]->rssi;
      }
      else if (info->measurementsInfo[i]->rssi < rssiMin)
      {
        rssiMin = info->measurementsInfo[i]->rssi;
      }
    }
  }
  else
  {
    log_warning(TAG, "@computeRssiMetrics(): no measurements available to calculate rssi metrics");
    return;
  }

  rssi_mwatt = rssiCum/(double)i; // avg rssi in mwatts
  rtt_node->rssi = 10*log10(rssi_mwatt); // avg rssi in dBm
  rtt_node->rssi *= -2; // avg rssi in steps of 0.5dB and positive

  rtt_node->rssi_spread = rssiMax - rssiMin;
  log_verbose(TAG, "@computeRssiMetrics(): avgRssi(%d), rssiMax(%d), rssiMin(%d), rssiSp(%d)",
              rtt_node->rssi, rssiMax, rssiMin, rtt_node->rssi_spread);
}

/*========================================================================================
 * mapLOWIBw
 *
 * Description:
 *   This function the bandwidth type received from the Lowi in an rtt ranging response
 *   to a correspoding WIFI HAL bandwidth type
 *
 * Parameters:
 * in_bw: value of lowi bw
 *
 * Return bw:
 ========================================================================================*/
lowi_rtt_bw LOWIClientListenerImpl::mapLOWIBw(uint8 in_bw)
{
  lowi_rtt_bw op_bw = LOWI_RTT_BW_20;

  switch(in_bw)
  {
    case BW_20MHZ:  op_bw = LOWI_RTT_BW_20; break;
    case BW_40MHZ:  op_bw = LOWI_RTT_BW_40; break;
    case BW_80MHZ:  op_bw = LOWI_RTT_BW_80; break;
    case BW_160MHZ: op_bw = LOWI_RTT_BW_160; break;
    default:
      op_bw = LOWI_RTT_BW_20;
      log_warning(TAG, "LOWI_IF", "UNKNOWN BW TYPE defaulting to BW_20MHZ");
      break;
  }

  return op_bw;
}


void LOWIClientListenerImpl::
processTxRate(lowi_rtt_result* rtt_node, LOWIScanMeasurement* info)
{
  if( info->measurementsInfo.getNumOfElements() != 0 )
  {
    // Since interface allows only for one set of tx_rate values per BSSID,
    // use the tx_rate parameters of the first measurement packet.
    rtt_node->tx_rate.preamble   = info->measurementsInfo[0]->tx_preamble;
    rtt_node->tx_rate.nss        = info->measurementsInfo[0]->tx_nss;
    rtt_node->tx_rate.bitrate    = info->measurementsInfo[0]->tx_bitrate;
    rtt_node->tx_rate.rateMcsIdx = info->measurementsInfo[0]->tx_mcsIdx;

    //Add Sanity check for TX BW
    if (info->measurementsInfo[0]->tx_bw >= BW_MAX)
    {
        log_error(TAG, "%s invalid Tx Bw", __func__, info->measurementsInfo[0]->tx_bw);
    }
    rtt_node->tx_rate.bw = mapLOWIBw(info->measurementsInfo[0]->tx_bw);
  }
}

void LOWIClientListenerImpl::
processRxRate(lowi_rtt_result* rtt_node, LOWIScanMeasurement* info)
{
  if( info->measurementsInfo.getNumOfElements() != 0 )
  {
    // Since interface allows only for one set of tx_rate values per BSSID,
    // use the rx_rate parameters of the first measurement packet.
    rtt_node->rx_rate.preamble   = info->measurementsInfo[0]->rx_preamble;
    rtt_node->rx_rate.nss        = info->measurementsInfo[0]->rx_nss;
    rtt_node->rx_rate.bitrate    = info->measurementsInfo[0]->rx_bitrate;
    rtt_node->rx_rate.rateMcsIdx = info->measurementsInfo[0]->rx_mcsIdx;

    //Add Sanity check for RX BW
    if (info->measurementsInfo[0]->rx_bw >= BW_MAX)
    {
       log_error(TAG, "%s invalid Rx Bw", __func__, info->measurementsInfo[0]->rx_bw);
    }
     rtt_node->rx_rate.bw = mapLOWIBw((qc_loc_fw::eRangingBandwidth)info->measurementsInfo[0]->rx_bw);
  }
}


/*========================================================================================
 * getRTTWeightedFactor
 *
 * Description:
 *   This function the gets required factor for preamble and bandwidth combination
 * Parameters:
 * filtData: It has information of the given measurement
 *
 * Return weighted factor
 *
 *   FTM_RX_PREAMBLE   FTM_RX_BW   ACK_TX_PREAMBLE   ACK_TX_BW   Weight_Factor
 *  --------------------------------------------------------------------------
 *      Legacy         Don't care      Legacy        Don't care       1/64
 *        HT             20MHZ           HT             20MHZ         1/64
 *        HT             20MHZ         Legacy        Don't care       1/64
 *        HT             40MHZ           HT             40MHZ         1/16
 *        HT             40MHZ         LegacyDup     Don't care       1/40
 *        VHT            20MHZ           VHT            20MHZ         1/64
 *        VHT            20MHZ         Legacy        Don't care       1/64
 *        VHT            40MHZ           VHT            40MHZ         1/16
 *        VHT            40MHZ         LegacyDup     Don't care       1/40
 *        VHT            80MHZ           VHT            80MHZ         1/4
 *        VHT            80MHZ         LegacyDup     Don't care       1/34
 *        VHT            160MHZ          VHT            160MHZ        1
 *        VHT            160MHZ        LegacyDup     Don't care       2/65
 *        HE             20MHZ           HE             20MHZ         1/64
 *        HE             20MHZ         Legacy        Don't care       1/64
 *        HE             40MHZ           HE             40MHZ         1/16
 *        HE             40MHZ         LegacyDup     Don't care       1/40
 *        HE             80MHZ           HE             80MHZ         1/4
 *        HE             80MHZ         LegacyDup     Don't care       1/34
 *        HE             160MHZ          HE             160MHZ        1
 *        HE             160MHZ        LegacyDup     Don't care       2/65
 *
 ========================================================================================*/


float LOWIClientListenerImpl::
getRTTWeightedFactor(LOWIMeasurementInfo* filtData)
{
  if (filtData->tx_preamble == RTT_PREAMBLE_LEGACY)
  {
    if (filtData->rx_preamble == RTT_PREAMBLE_LEGACY)
    {
      return (float)FACTOR_20MHZ_DUP_LEGACY_ACK;
    }
    switch (filtData->rx_bw)
    {
      case BW_20MHZ:
         return (float)FACTOR_20MHZ_DUP_LEGACY_ACK;
      case BW_40MHZ:
         return (float)FACTOR_40MHZ_DUP_LEGACY_ACK;
      case BW_80MHZ:
         return (float)FACTOR_80MHZ_DUP_LEGACY_ACK;
      case BW_160MHZ:
         return (float)FACTOR_160MHZ_DUP_LEGACY_ACK;
      default:
         return 0;
    }
  }
  else if ((filtData->rx_bw == filtData->tx_bw) &&
           (filtData->rx_preamble == filtData->tx_preamble))
  {
    switch (filtData->rx_bw)
    {
      case BW_20MHZ:
         return (float)FACTOR_20MHZ_ACK;
      case BW_40MHZ:
         return (float)FACTOR_40MHZ_ACK;
      case BW_80MHZ:
         return (float)FACTOR_80MHZ_ACK;
      case BW_160MHZ:
         return (float)FACTOR_160MHZ_ACK;
      default:
         return 0;
    }
  }
  return 0;
}

int32 LOWIClientListenerImpl::
computeMedian(vector <int32> &rtt_array)
{
  uint32 array_size = rtt_array.getNumOfElements();

  log_verbose(TAG, "%s: Befor sort rtt ps array ", __FUNCTION__);
  for(uint32 ii = 0; ii < array_size; ii++)
  {
    log_verbose(TAG, "%d", rtt_array[ii]);
  }
  std::sort(rtt_array.begin(), rtt_array.end());
  log_verbose(TAG, "%s: After sort rtt ps array ", __FUNCTION__);
  for(uint32 ii = 0; ii < array_size; ii++)
  {
    log_verbose(TAG, "%d", rtt_array[ii]);
  }

  if((array_size%2) != 0)
  {
    /* odd array length, middle position will be median */
    return rtt_array[array_size/2];
  }
  else
  {
    /*
     * even array lenght median will be
     * average sum of two middle index values
     */
    return ((rtt_array[array_size/2]+rtt_array[(array_size/2)-1])/2);
  }
}

void LOWIClientListenerImpl::
computeStatsPerChain(vector <LOWIMeasurementInfo*>& rttVector,
                     float &rttMean, int32 &rttStdev,
                     int32 &rttMax, int32 &rttMin,
                     int32 &rttMedian, uint32 chain_num_idx)
{
    int64 sum = 0;
    int64 sqSum = 0;
    int32 numMeas = 0;
    vector <int32> rtt_ps_array;
    int32 rtt_ps = 0;

    if (rttVector.getNumOfElements() == 0)
    {
        log_verbose(TAG, "%s: Emtpy vector for stats computation", __FUNCTION__);
        return;
    }

    log_debug(TAG, "%s: NumOfElements:%d chain_num_idx:%d",
            __FUNCTION__, rttVector.getNumOfElements(), chain_num_idx);

    rttMin = rttMax = rttVector[0]->perchinfo[chain_num_idx].rtt_ps_perchain;

    for (uint8_t kk = 0; kk < rttVector.getNumOfElements(); kk++)
    {
        rtt_ps = rttVector[kk]->perchinfo[chain_num_idx].rtt_ps_perchain;

        if (rtt_ps < RTT_NEG20M_PS)
        {
            log_debug(TAG, "%s: Drop the measurement, rtt_ps_perchain: %d < %d chain_num_idx:%d",
                    __func__, rtt_ps, RTT_NEG20M_PS, chain_num_idx);
            continue;
        }

        sum += rtt_ps;
        sqSum += ((int64)rtt_ps)*rtt_ps;
        rttMin = (rtt_ps < rttMin ? rtt_ps : rttMin);
        rttMax = (rtt_ps > rttMax ? rtt_ps : rttMax);
        rtt_ps_array.push_back(rtt_ps);
        numMeas++;
    }

    if (numMeas > 0)
    {
        rttMean = sum / numMeas;
        sqSum -= ((int64)numMeas) * rttMean * rttMean;
        rttStdev  = (int)(sqrt((double)(sqSum/numMeas)));
        rttMedian = computeMedian(rtt_ps_array);
    }
    else
    {
        rttMean = rttStdev = rttMax = rttMin = rttMedian = 0;
    }
}

void LOWIClientListenerImpl::
computeStats(vector <LOWIMeasurementInfo*>& rttVector, float& mean,
             int32& stdev, int32& maxVal, int32& minVal, int32& medianVal, bool compute_median=false,
             eRttType rttType=BEST_EFFORT_RANGING)
{
  if (rttVector.getNumOfElements() == 0)
  {
    log_verbose(TAG, "%s: Emtpy vector for stats computation", __FUNCTION__);
    return;
  }
  int64 sum     = 0;
  int64 sqSum   = 0;
  int32 numElem = 0;
  minVal = maxVal = rttVector[0]->rtt_ps;
  vector <int32> rtt_ps_array;
  for(uint32 ii = 0; ii < rttVector.getNumOfElements(); ii++)
  {
    int32 rtt_ps = rttVector[ii]->rtt_ps;

    if (rtt_ps < RTT_NEG20M_PS)
    {
        log_debug(TAG, "%s: Drop the measurement, rtt_ps:%d < %d", __func__, rtt_ps, RTT_NEG20M_PS);
        continue;
    }

    sum   += rtt_ps;
    sqSum += ((int64)rtt_ps)*rtt_ps;
    minVal = (rtt_ps < minVal ? rtt_ps : minVal);
    maxVal = (rtt_ps > maxVal ? rtt_ps : maxVal);
    rtt_ps_array.push_back(rtt_ps);
    numElem++;
  }

  if (numElem > 0)
  {
    mean = sum / numElem;
    sqSum -= ((int64)numElem) * mean * mean;
    stdev  = (int)(sqrt((double)(sqSum/numElem)));
    if(compute_median)
      medianVal = computeMedian(rtt_ps_array);
  }
  else
  {
    mean = stdev = maxVal = minVal = medianVal = 0;
  }
}

void LOWIClientListenerImpl::
computeMean(vector <LOWIMeasurementInfo*>&  filtData, float& mean)
{
  /* Calculate mean of both chain separatley */
  float factor = 1;
  float rtt_sum = 0, count = 0;

  log_info(TAG, "%s: Weighted mean %s", __FUNCTION__, g_weighted_mean?"Enabled":"Disabled");

  for (uint32 ii = 0; ii < filtData.getNumOfElements(); ii++)
  {
    if (g_weighted_mean)
    {
      factor = getRTTWeightedFactor(filtData[ii]);
    }
    log_debug(TAG, "%s: TX chain: %d RX: chain: %d rtt_ps: %d, factor: %.4f", __FUNCTION__,
              filtData[ii]->tx_chain_no, filtData[ii]->rx_chain_no,
              filtData[ii]->rtt_ps, factor);

    rtt_sum += factor*(float)filtData[ii]->rtt_ps;
    count += factor;
  }

  if (count)
      mean = rtt_sum/count;

  log_verbose(TAG, "sum:%.4f count:%.4f mean:%.4f", rtt_sum, count, mean);
  return;
}

void LOWIClientListenerImpl::
updateResultPerBW(vector <LOWIMeasurementInfo*>& rttVector, uint32 *resultPerBW,
                  uint32 *filteredBW, e_chain_mask_lib chain_mask,
                  uint32 *chainIndexReq, uint32 *chainIndexUsed)
{
  int tx_chain_mask, rx_chain_mask;
  for(uint32 ii = 0; ii < rttVector.getNumOfElements(); ii++)
  {
    ++(resultPerBW[rttVector[ii]->rx_bw]);

    uint8 bw = MIN(rttVector[ii]->rx_bw, rttVector[ii]->tx_bw);

    if (bw >= BW_MAX)
    {
      log_debug(TAG, "%s: Invalid bandwidth %d", __FUNCTION__, bw);
      continue;
    }

    ++(filteredBW[bw]);

    // Split frames per chain wise
    tx_chain_mask = (rttVector[ii]->tx_chain_no == -1) ? -1 :
                           (1 << rttVector[ii]->tx_chain_no);
    rx_chain_mask = (rttVector[ii]->rx_chain_no == -1) ? -1 :
                           (1 << rttVector[ii]->rx_chain_no);
    if ((tx_chain_mask != -1) && (tx_chain_mask & chain_mask))
    {
        if (rttVector[ii]->tx_chain_no != -1)
            chainIndexReq[rttVector[ii]->tx_chain_no]++;
    }
    if ((rx_chain_mask != -1) && (rx_chain_mask & chain_mask))
    {
        if (rttVector[ii]->rx_chain_no != -1)
            chainIndexUsed[rttVector[ii]->rx_chain_no]++;
    }
  }
}

void LOWIClientListenerImpl::
filterOutliersPerChain(vector <LOWIMeasurementInfo*>& rttVector,
                       float& rttMean_PerChain, int32& rttStdev_PerChain,
                       int32& rttMax_PerChain, int32& rttMin_PerChain,
                       int32& rttMedian_PerChain,
                       struct req_map current_node_map,
                       uint32 *filteredBW_PerChain,
                       eRttType rttType,
                       vector<LOWIMeasurementInfo *>& filtData_PerChain,
                       uint32 chain_num_idx)
{
  int32 stdevmult = 0;

  if (rttVector.getNumOfElements() < 3)
  {
    log_verbose(TAG, "%s: Too few elements %d",
                __FUNCTION__, rttVector.getNumOfElements());
    return;
  }

  log_debug(TAG, "%s: CHAIN: %d",__FUNCTION__, chain_num_idx);

  log_debug(TAG, "%s: Per Chain RTT before Filter Mean:%.4f Max:%d Min:%d Spread:%d Stdev:%d medianVal: %d",
            __FUNCTION__, rttMean_PerChain, rttMax_PerChain,
            rttMin_PerChain, rttMax_PerChain - rttMin_PerChain,
            rttStdev_PerChain, rttMedian_PerChain);

  memset(filteredBW_PerChain, 0, (BW_MAX)*sizeof(uint32));

  if (rttType == RTT2_RANGING)
      stdevmult = RTT2_mStdevMult;
  else
      stdevmult = mStdevMult;

  for(uint32 ii = 0; ii < rttVector.getNumOfElements(); ii++)
  {

    if ( (g_discard_bw_mis_match) &&
        ((current_node_map.requestedBW != rttVector[ii]->rx_bw) ||
        (current_node_map.requestedBW != rttVector[ii]->tx_bw)) &&
        (rttType == RTT3_RANGING))
    {
      log_debug(TAG, "Dropping due to Tx/Rx BW is not equals of req BW");
      continue;
    }

    uint8 bw = MIN(rttVector[ii]->rx_bw, rttVector[ii]->tx_bw);
    if (bw >= BW_MAX)
    {
      log_debug(TAG, "%s: Invalid bandwidth %d", __FUNCTION__, bw);
      continue;
    }

    int32 highThresh = rttMedian_PerChain + stdevmult*RTT_STDEV[bw];
    int32 lowThresh  = rttMedian_PerChain - stdevmult*RTT_STDEV[bw];
    int32 rtt_ps_perchain = rttVector[ii]->perchinfo[chain_num_idx].rtt_ps_perchain;

    if ((rtt_ps_perchain > highThresh) || (rtt_ps_perchain < lowThresh))
    {
      log_debug(TAG, "%s: Removed outlier at %d, val %d",
                __FUNCTION__, ii, rtt_ps_perchain);
      continue;
    }

    ++(filteredBW_PerChain[bw]);
    filtData_PerChain.push_back(rttVector[ii]);
  }

  if (filtData_PerChain.getNumOfElements() == 0)
  {
    log_warning(TAG, "%s: No measurements meet criteria\n", __FUNCTION__);
    rttMean_PerChain = rttStdev_PerChain =
    rttMax_PerChain = rttMin_PerChain = 0;
    return;
  }

  log_debug(TAG, "%s:Per Chain RTT after Filter Mean:%.4f Max:%d Min:%d Spread:%d Stdev:%d medianVal: %d",
            __FUNCTION__, rttMean_PerChain, rttMax_PerChain,
            rttMin_PerChain, rttMax_PerChain - rttMin_PerChain,
            rttStdev_PerChain, rttMedian_PerChain);

  computeStats(filtData_PerChain, rttMean_PerChain,
               rttStdev_PerChain, rttMax_PerChain,
               rttMin_PerChain, rttMedian_PerChain);

  computeMean(filtData_PerChain, rttMean_PerChain);
}

void LOWIClientListenerImpl::
filterOutliers(vector <LOWIMeasurementInfo*>& rttVector, float& mean, int32& stdev, int32& maxVal,
               int32& minVal, int32& medianVal, struct req_map current_node_map,
               uint32 *filteredBW, eRttType rttType, vector<LOWIMeasurementInfo *>& filtData)
{
  int32 stdevmult = 0;
  if (rttVector.getNumOfElements() < 3)
  {
    log_verbose(TAG, "%s: Too few elements %d",
                __FUNCTION__, rttVector.getNumOfElements());
    return;
  }

  log_debug(TAG, "%s: RTT before Filter Mean:%.4f Max:%d Min:%d Spread:%d Stdev:%d medianVal: %d",
            __FUNCTION__, mean, maxVal, minVal, maxVal-minVal, stdev, medianVal);

  memset(filteredBW, 0, (BW_MAX)*sizeof(uint32));

  if (rttType == RTT2_RANGING)
      stdevmult = RTT2_mStdevMult;
  else
      stdevmult = mStdevMult;

  for(uint32 ii = 0; ii < rttVector.getNumOfElements(); ii++)
  {

    if ( (g_discard_bw_mis_match) &&
        ((current_node_map.requestedBW != rttVector[ii]->rx_bw) ||
        (current_node_map.requestedBW != rttVector[ii]->tx_bw)) &&
        (rttType == RTT3_RANGING))
    {
      log_debug(TAG, "Dropping due to Tx/Rx BW is not equals of req BW");
      continue;
    }

    uint8 bw = MIN(rttVector[ii]->rx_bw, rttVector[ii]->tx_bw);
    if (bw >= BW_MAX)
    {
      log_debug(TAG, "%s: Invalid bandwidth %d", __FUNCTION__, bw);
      continue;
    }

    int32 highThresh = medianVal + stdevmult*RTT_STDEV[bw];
    int32 lowThresh  = medianVal - stdevmult*RTT_STDEV[bw];
    int32 rtt_ps = rttVector[ii]->rtt_ps;
    if ((rtt_ps > highThresh) || (rtt_ps < lowThresh))
    {
      log_debug(TAG, "%s: Removed outlier at %d, val %d", __FUNCTION__, ii, rtt_ps);
      continue;
    }
    ++(filteredBW[bw]);
    filtData.push_back(rttVector[ii]);
  }
  if (filtData.getNumOfElements() == 0)
  {
    log_warning(TAG, "%s: No measurements meet criteria\n", __FUNCTION__);
    mean = stdev = maxVal = minVal = 0;
    return;
  }

  log_debug(TAG, "%s: RTT after Filter Mean:%.4f Max:%d Min:%d Spread:%d Stdev:%d medianVal: %d",
            __FUNCTION__, mean, maxVal, minVal, maxVal-minVal, stdev, medianVal);

  computeStats(filtData, mean, stdev, maxVal, minVal, medianVal, rttType);
  computeMean(filtData, mean);
}

void LOWIClientListenerImpl::
computeRttMetrics(lowi_rtt_result* rtt_node, LOWIScanMeasurement* info, struct req_map current_node_map)
{
  log_verbose(TAG, "@computeRttMetrics()");
  vector<LOWIMeasurementInfo *> meas_info;
  vector<LOWIMeasurementInfo *> filtData;
  vector <lowi_rtt_result*> rtt_chain_db;

  // initialize to zero.
  rtt_node->rtt        = 0;
  rtt_node->rtt_sd     = 0;
  rtt_node->rtt_spread = 0;

  for (int ii = 0; ii < 4; ii++)
  {
    rtt_node->rtt_perchain[ii] = 0;
    rtt_node->rtt_sd_perchain[ii] = 0;
    rtt_node->rtt_spread_perchain[ii] = 0;
  }


  e_chain_mask_lib chain_arr[] = {CHAIN_0_lib, CHAIN_1_lib, CHAIN_2_lib, CHAIN_3_lib, CHAIN_4_lib,
                              CHAIN_5_lib, CHAIN_6_lib, CHAIN_7_lib, CHAIN_ALL_lib, CHAIN_MAX_lib};


  for (uint32 chain_num_idx = 0; chain_num_idx < (sizeof(chain_arr)/sizeof(e_chain_mask_lib))-1; chain_num_idx++)
  {
      meas_info = info->measurementsInfo;

      int32 rttMax= 0;
      int32 rttMin = 0;
      float rttMean = 0;
      int32 rttStdev = 0;
      int32 rttMedian = 0;
      uint32 resultPerBW[BW_MAX] = {0};
      uint32 filteredBW[BW_MAX] = {0};
      uint32 chainIndexReq[LOWI_CHAIN_IDX_MAX] = {0};
      uint32 chainIndexUsed[LOWI_CHAIN_IDX_MAX] = {0};

      if (rttCachedCapabilities->rxAllChainSupport && chain_num_idx < 4)
      {
          computeStatsPerChain(meas_info, rttMean,
                               rttStdev,
                               rttMax,
                               rttMin,
                               rttMedian,
                               chain_num_idx);

          updateResultPerBW(meas_info, resultPerBW,
                                    filteredBW,
                                    (e_chain_mask_lib)chain_arr[chain_num_idx],
                                    chainIndexReq, chainIndexUsed);

          filterOutliersPerChain(meas_info, rttMean,
                                 rttStdev,
                                 rttMax,
                                 rttMin,
                                 rttMedian,
                                 current_node_map, filteredBW,
                                 info->rttType, filtData, chain_num_idx);

          rtt_node->rtt_perchain[chain_num_idx] = (int)rttMean;
          rtt_node->rtt_sd_perchain[chain_num_idx] = rttStdev;
          rtt_node->rtt_spread_perchain[chain_num_idx] =
              rttMax - rttMin;

          if (g_discard_bw_mis_match)
          {
              rtt_node->meas_as_per_config_perchain[chain_num_idx] =
                  filteredBW[current_node_map.requestedBW];
          }

          if (filtData.getNumOfElements())
          {
              for ( int i = 0; i < BW_MAX ; i++)
              {
                  rtt_node->tot_meas_per_bw_perchain[chain_num_idx][i] =
                      resultPerBW[i];
                  rtt_node->final_meas_per_bw_perchain[chain_num_idx][i] =
                      filteredBW[i];
              }
              for (int j = 0; j < LOWI_CHAIN_IDX_MAX; j++)
              {
                  rtt_node->request_chain_index_PerChain[chain_num_idx][j] =
                      chainIndexReq[j];
                  rtt_node->used_chain_index_PerChain[chain_num_idx][j] =
                      chainIndexUsed[j];
              }
          }
          filtData.flush();
      }

      if ((e_chain_mask_lib)chain_arr[chain_num_idx] != CHAIN_ALL_lib)
          continue;

      computeStats(meas_info, rttMean, rttStdev, rttMax,
              rttMin, rttMedian, true, info->rttType);

      updateResultPerBW(meas_info, resultPerBW, filteredBW, (e_chain_mask_lib)chain_arr[chain_num_idx],
              chainIndexReq, chainIndexUsed);

      filterOutliers(meas_info, rttMean, rttStdev, rttMax,
              rttMin, rttMedian, current_node_map, filteredBW, info->rttType, filtData);
      rtt_node->rtt        = (int)rttMean;
      rtt_node->rtt_sd     = rttStdev;
      rtt_node->rtt_spread = rttMax - rttMin;

      if (g_discard_bw_mis_match)
          rtt_node->meas_as_per_config = filteredBW[current_node_map.requestedBW];

      if (filtData.getNumOfElements())
      {
          for ( int i=0; i< LOWI_CHAIN_IDX_MAX ; i++) {
              rtt_node->request_chain_index[i] = chainIndexReq[i];
              rtt_node->used_chain_index[i] = chainIndexUsed[i];
          }
          for ( int j=0 ; j< BW_MAX ; j++) {
              rtt_node->tot_meas_per_bw[j] = resultPerBW[j];
              rtt_node->final_meas_per_bw[j] = filteredBW[j];
          }
      }

      filtData.flush();
      meas_info.flush();
  }

}

void LOWIClientListenerImpl::
computeDistanceMetrics(lowi_rtt_result* rtt_node, LOWIScanMeasurement* info, struct req_map current_node_map)
{
  // ensure that rtt metrics are computed first
  computeRttMetrics(rtt_node, info, current_node_map);
  // distance metrics in mm
  rtt_node->distance_mm        = int((float)rtt_node->rtt * RTT_DIST_CONST_PS_MM);
  rtt_node->distance_sd_mm     = int((float)rtt_node->rtt_sd * RTT_DIST_CONST_PS_MM);
  rtt_node->distance_spread_mm = int((float)rtt_node->rtt_spread * RTT_DIST_CONST_PS_MM);

  if (rttCachedCapabilities->rxAllChainSupport) {
      int allChainSize = 4;

      for (int ii = 0; ii < allChainSize; ++ii)
      {
          rtt_node->distance_mm_perchain[ii] = int((float)rtt_node->rtt_perchain[ii] * RTT_DIST_CONST_PS_MM);
          rtt_node->distance_sd_mm_perchain[ii] = int((float)rtt_node->rtt_sd_perchain[ii] * RTT_DIST_CONST_PS_MM);
          rtt_node->distance_spread_mm_perchain[ii] = int((float)rtt_node->rtt_spread_perchain[ii] * RTT_DIST_CONST_PS_MM);
      }
  }
}

void LOWIClientListenerImpl::
processLocationIeData(lowi_rtt_result *rtt_node, LOWIScanMeasurement *info)
{
  rtt_node->LCI = NULL;
  rtt_node->LCR = NULL;

  do
  {
    if ((NULL != info->lciInfo) && (0 != info->lciInfo->len))
    {
      log_verbose(TAG, "@processLocationIeData(): lciInfo received len(%u)", info->lciInfo->len);
      // allocate memory for the results
      // size of wifi_information_element is 2 bytes
      rtt_node->LCI = (lowi_information_element *)malloc(sizeof(lowi_information_element) +
                                                         sizeof(byte) * info->lciInfo->len);
      if (NULL == rtt_node->LCI)
      {
        log_debug(TAG, "@processLocationIeData(): memory allocation error");
        break;
      }

      // copy results over
      rtt_node->LCI->id  = (byte)info->lciInfo->id;
      rtt_node->LCI->len = (byte)info->lciInfo->len;
      memcpy(rtt_node->LCI->data, info->lciInfo->locData, info->lciInfo->len);
    }
  } while (0);

  do
  {
    if ((NULL != info->lcrInfo) && (0 != info->lcrInfo->len))
    {
      log_verbose(TAG, "@processLocationIeData(): lcrInfo received len(%u)", info->lcrInfo->len);

      // allocate memory for the results
      // size of wifi_information_element is 2 bytes
      rtt_node->LCR = (lowi_information_element *)malloc(sizeof(lowi_information_element) +
                                                         sizeof(byte) * info->lcrInfo->len);
      if (NULL == rtt_node->LCR)
      {
        log_debug(TAG, "@processLocationIeData(): memory allocation error");
        break;
      }

      // copy results over
      rtt_node->LCR->id  = (byte)info->lcrInfo->id;
      rtt_node->LCR->len = (byte)info->lcrInfo->len;
      memcpy(rtt_node->LCR->data, info->lcrInfo->locData, info->lcrInfo->len);
    }
  } while (0);
}


