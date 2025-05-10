/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

        LOWI Client Listener

GENERAL DESCRIPTION
  This file contains the class definition for the LOWIClientListener
  used in the LOWI interface for RTT and BGSCAN measurements

Copyright (c) 2014,2016-2021 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
=============================================================================*/
#ifndef __LISTENER_IMPL_H__
#define __LISTENER_IMPL_H__

#include <inc/lowi_scan_measurement.h>
#include <inc/lowi_client.h>
#include <inc/lowi_const.h>
#include <inc/lowi_mac_address.h>
#include <inc/lowi_response.h>
#include "inc/infra/lowi_response_extn.h"

/* Weighted mean factors. HT, VHT and HE has same factors */

#define FACTOR_20MHZ_ACK                1/64
#define FACTOR_20MHZ_DUP_LEGACY_ACK     1/64
#define FACTOR_40MHZ_ACK                1/16
#define FACTOR_40MHZ_DUP_LEGACY_ACK     1/40
#define FACTOR_80MHZ_ACK                1/4
#define FACTOR_80MHZ_DUP_LEGACY_ACK     1/34
#define FACTOR_160MHZ_ACK               1
#define FACTOR_160MHZ_DUP_LEGACY_ACK    2/65

// msg strings used for printing/debugging
extern const char* PREAMBLE_STR[4];
extern const char* RTT_STR[3];
extern const char* BW_STR[4];
extern const char* NODE_STR[3];
extern const char* LOWI_RTT_STR[2];
extern const char* LOWI_PEER_STR[7];
extern const char* LOWI_PREAMBLE_STR[6];
extern const char* LOWI_NSS_STR[4];
extern const char* LOWI_STATUS[18];

using namespace qc_loc_fw;

enum e_chain_bit_lib
{
  CHAIN_BIT_0 = 0,
  CHAIN_BIT_1 = 1,
  CHAIN_BIT_2 = 2,
  CHAIN_BIT_3 = 3,
  CHAIN_BIT_4 = 4,
  CHAIN_BIT_5 = 5,
  CHAIN_BIT_6 = 6,
  CHAIN_BIT_7 = 7,
};

enum e_chain_mask_lib
{
  CHAIN_0_lib  = 1 << CHAIN_BIT_0,
  CHAIN_1_lib  = 1 << CHAIN_BIT_1,
  CHAIN_2_lib  = 1 << CHAIN_BIT_2,
  CHAIN_3_lib  = 1 << CHAIN_BIT_3,
  CHAIN_4_lib  = 1 << CHAIN_BIT_4,
  CHAIN_5_lib  = 1 << CHAIN_BIT_5,
  CHAIN_6_lib  = 1 << CHAIN_BIT_6,
  CHAIN_7_lib  = 1 << CHAIN_BIT_7,
  CHAIN_ALL_lib = (CHAIN_0_lib | CHAIN_1_lib | CHAIN_2_lib | CHAIN_3_lib |
                   CHAIN_4_lib | CHAIN_5_lib | CHAIN_6_lib | CHAIN_7_lib),
  CHAIN_MAX_lib,
};

struct req_map
{
  LOWIMacAddress bssid;
  enum eRangingBandwidth requestedBW;

  req_map()
  {
    requestedBW = BW_20MHZ;
  }

  req_map(LOWIMacAddress mac, enum eRangingBandwidth bw)
  {
    bssid = mac;
    requestedBW = bw;
  }
};


// LOWI client listener implementation class
class LOWIClientListenerImpl : public LOWIClientListener
{
private:
  static const char * const TAG;
  // -20m in pico second units (approximately).
  static const int32 RTT_NEG20M_PS = -150000;

  //RTT standard deviation in picoseconds for each bw 20, 40, 80, 160 MHz.
  static const int32 RTT_STDEV[qc_loc_fw::BW_MAX];

  const int32 mStdevMult;  // stdev multiplier for outlier detection for two-sided RTT
  const int32 RTT2_mStdevMult;  // stdev multiplier for outlier detection for single-sided RTT

/*========================================================================================
 * processLocationIeData
 *
 * Description:
 * This function processes the LCI/LCR location data
 *
 * Parameters:
 *  rtt_node: rtt results for a single node
 *  info: scan measurement from LOWI
 *
 * Return value:
 ========================================================================================*/
  void processLocationIeData(lowi_rtt_result *rtt_node, LOWIScanMeasurement *info);

/*========================================================================================
 * scanMeasToResult
 *
 * Description:
 *   This function converts the scan measurements into results
 *
 * Parameters:
 *  scanMeasurements: scan measurements
 *  scanStatus: Overall Request status
 *
 * Return value:
 *   ptr to results
 ========================================================================================*/
  lowi_rtt_result* scanMeasToResult(vector <LOWIScanMeasurement*> & scanMeasurements,
                                           LOWIResponse::eScanStatus scanStatus);

/*========================================================================================
 * printLowiResponse
 *
 * Description:
 *   This function prints the scan measurements received from lowi-server into
 *   lowi-client before conversion to results
 *
 * Parameters:
 *  scanMeasurements: scan measurements
 *
 * Return value:
 ========================================================================================*/
  void printLowiResponse (vector <LOWIScanMeasurement*> & scanMeasurements);

/*========================================================================================
 * printNodeResult
 *
 * Description:
 *   This function prints the rtt scan results for a single wifi node as defined in the
 *   structure wifi_rtt_result
 *
 * Parameters:
 *  rtt_node_result: rtt results for a single node
 *
 * Return value:
 ========================================================================================*/
  void printNodeResult(lowi_rtt_result *rtt_node_result);

/*========================================================================================
 * mapLowiStatusToStatus
 *
 * Description:
 *   This function maps LOWI status to type understood for each wifi node in
 *   the response
 *
 * Parameters:
 *  rtt_node: rtt results for a single node
 *  info: scan measurement from LOWI
 *  scanStatus: Overall Request status
 *
 * Return value:
 ========================================================================================*/
  void mapLowiStatusToStatus(lowi_rtt_result *rtt_node,
                                    LOWIScanMeasurement *info,
                                    LOWIResponse::eScanStatus scanStatus);

/*========================================================================================
 * mapRttType
 *
 * Description:
 *   This function maps LOWI RTT type
 *
 * Parameters:
 *  rtt_node: rtt results for a single node
 *  info: scan measurement from LOWI
 *
 * Return value:
 ========================================================================================*/
  void mapRttType(lowi_rtt_result *rtt_node, LOWIScanMeasurement *info);

/*========================================================================================
 * computeRssiMetrics
 *
 * Description:
 * This function calculates the following rssi metrics: average rssi, minimum rssi,
 * maximum rssi, rssi spread and rssi standard deviation. Average rssi is calculated by
 * converting the dB readings to mwatts first.
 *
 * Parameters:
 *  rtt_node: rtt results for a single  node
 *  info: scan measurement from LOWI
 *
 * Return value:
 ========================================================================================*/
  void computeRssiMetrics(lowi_rtt_result *rtt_node, LOWIScanMeasurement *info);

/*========================================================================================
 * computeRssiMetrics_perchain
 *
 * Description:
 * This function calculates the following rssi metrics: average rssi, minimum rssi,
 * maximum rssi, rssi spread and rssi standard deviation. Average rssi is calculated by
 * converting the dB readings to mwatts first for per chain.
 *
 * Parameters:
 *  rtt_node: rtt results for a single  node
 *  info: scan measurement from LOWI
 *
 * Return value:
 ========================================================================================*/
  void computeRssiMetrics_perchain(lowi_rtt_result *rtt_node, LOWIScanMeasurement *info);

/*========================================================================================
 * mapLOWIBw
 *
 * Description:
 *   This function the bandwidth type received from the Lowi in an rtt ranging response
 *   to a correspoding  bandwidth type
 *
 * Parameters:
 * in_bw: value of lowi bw
 *
 * Return bw:
 ========================================================================================*/
  lowi_rtt_bw  mapLOWIBw(uint8 in_bw);

/*========================================================================================
 * processTxRate
 *
 * Description:
 * This function calculates maps the tx rate parameters from LOWI types to types.
 * The parameters mapped are the following:
 *   mcsIndex
 *   bitrate
 *   bandwidth
 *   preamble
 *   nss
 *
 * Parameters:
 *  rtt_node: rtt results for a single node
 *  info: scan measurement from LOWI
 *
 * Return value:
 ========================================================================================*/
  void processTxRate(lowi_rtt_result *rtt_node, LOWIScanMeasurement *info);

/*========================================================================================
 * processRxRate
 *
 * Description:
 * This function calculates maps the tx rate parameters from LOWI types to types.
 * The parameters mapped are the following:
 *   mcsIndex
 *   bitrate
 *   bandwidth
 *   preamble
 *   nss
 *
 * Parameters:
 *  rtt_node: rtt results for a single node
 *  info: scan measurement from LOWI
 *
 * Return value:
 ========================================================================================*/
  void processRxRate(lowi_rtt_result *rtt_node, LOWIScanMeasurement *info);

/*========================================================================================
 * computeRttMetrics
 *
 * Description:
 * This function calculates the following rtt metrics: average rtt, minimum rtt,
 * maximum rtt and rtt spread. Average rssi is calculated by converting the dB
 * readings to mwatts first.
 *
 * Parameters:
 *  rtt_node: rtt results for a single node
 *  info: scan measurement from LOWI
 * Return value:
 ========================================================================================*/
  void computeRttMetrics(lowi_rtt_result *rtt_node, LOWIScanMeasurement *info, struct req_map current_node_map);

/*========================================================================================
 * computeRttMetrics
 *
 * Description:
 * This function calculates the following distance metrics: distance, distance spread and
 * distance standard deviation.
 *
 * Parameters:
 *  rtt_node: rtt results for a single node
 *  info: scan measurement from LOWI
 *
 * Return value:
 ========================================================================================*/
  void computeDistanceMetrics(lowi_rtt_result *rtt_node, LOWIScanMeasurement *info, struct req_map current_node_map);

/*========================================================================================
 * getRTTWeightedFactor
 *
 * Description:
 * This function decides the factor based on tx/rx preamble and given bandwidths
 *
 * Parameters:
 * filtData: Measurement info from LOWI
 *
 * Return value: factor
 ========================================================================================*/
  float getRTTWeightedFactor(LOWIMeasurementInfo * filtData);

/*========================================================================================
 * computeStats
 *
 * Description:
 *  This function calculates the following rtt metrics:
 *  Mean, Min, Max and standard deviation
 *
 * Parameters:
 *  rttVector: rtt results for a single node
 *  mean   : Mean of the RTT measurements
 *  stdev  : Standard deviation of the RTT measurements
 *  maxVal : Max of the RTT measurements
 *  minVal : Min of the RTT measurements
 *  median: median value of all rtt measurements
 *  compute_median: Set for computing median else false
 *  rttType: RTT type
 *
 * Return value:
 ========================================================================================*/
  void computeStats(vector <LOWIMeasurementInfo*>& rttVector, float& mean,
             int32& stdev, int32& maxVal, int32& minVal, int32& medianVal, bool compute_median,
             eRttType rttType);

/*========================================================================================
 * computeStatsPerChain
 *
 * Description:
 *  This function calculates the following rtt metrics:
 *  Mean, Min, Max and standard deviation
 *
 * Parameters:
 *  rttVector: rtt results for a single node
 *  rttMean_PerChain   : Mean of the RTT measurements per chain
 *  rttStdev_PerChain  : Standard deviation of the RTT measurements per chain
 *  rttMax_PerChain : Max of the RTT measurementsm per chain
 *  rttMin_PerChain : Min of the RTT measurements per chain
 *  rttMedian_PerChain: median value of all rtt measurements per chain
 *  chain_num_idx: chain index number
 *
 * Return value:
 ========================================================================================*/
  void computeStatsPerChain(vector <LOWIMeasurementInfo*>& rttVector, float &rttMean,
             int32 &rttStdev, int32 &rttMax, int32 &rttMin,
             int32 &rttMedian, uint32 chain_num_idx);

/*========================================================================================
   * computeMedian
   *
   * Description:
   *  This function calculates thec compute Median.
   * Parameters:
      rtt_array: rtt ps array
     ========================================================================================*/
 int32 computeMedian(vector <int32> &rtt_array);

/*========================================================================================
 * updateResultPerBW
 *
 * Description:
 *  This function captures the number of measurements received in each BW.
 * Parameters:
 *  rttVector: rtt results for a single node
 *  resultPerBW : RTT Measurements obtained on each BW
 *  filteredBW : RTT Measurements obtained on each BW after Skip BW and filtering outliers
 *  chain_mask : chain mask
 *  chainIndexReq : Requested chain index
 *  chainIndexUsed : Used chain index
 ========================================================================================*/

void  updateResultPerBW(vector <LOWIMeasurementInfo*>& rttVector, uint32 *resultPerBW,
                        uint32* filteredBW, e_chain_mask_lib chain_mask,
                        uint32 *chainIndexReq, uint32 *chainIndexUsed);

/*========================================================================================
 * filterOutliers
 *
 * Description:
 *  This function filters the RTT measurement for outliers. Updates the stats
 *  with the filtered measurements.
 *
 * Parameters:
 *  rttVector: rtt results for a single node
 *  mean   : Mean of the RTT measurements
 *  stdev  : Standard deviation of the RTT measurements
 *  maxVal : Max of the RTT measurements
 *  minVal : Min of the RTT measurements
 *  medianVal : Median of the RTT measurements
 *  current_node_map : Current node map
 *  filteredBW : RTT Measurements obtained on each BW after filtering outliers
 *  rttType: RTT type
 *  filtData : Filtered data
 * Return value:
 ========================================================================================*/
  void filterOutliers(vector <LOWIMeasurementInfo*>& rttVector, float& mean, int32& stdev, int32& maxVal,
                      int32& minVal, int32& medianVal, struct req_map current_node_map,
                      uint32 *filteredBW, eRttType rttType, vector<LOWIMeasurementInfo *>& filtData);

/*========================================================================================
 * filterOutliers
 *
 * Description:
 *  This function filters the RTT measurement for outliers. Updates the stats
 *  with the filtered measurements.
 *
 * Parameters:
 *  rttVector: rtt results for a single node
 *  mean   : Mean of the RTT measurements
 *  stdev  : Standard deviation of the RTT measurements
 *  maxVal : Max of the RTT measurements
 *  minVal : Min of the RTT measurements
 *  medianVal : Median of the RTT measurements
 *  current_node_map : Current node map
 *  filteredBW : RTT Measurements obtained on each BW after filtering outliers
 *  rttType: RTT type
 *  filtData : Filtered data
 *  chain_num_idx : chain index number
 * Return value:
 ========================================================================================*/
  void filterOutliersPerChain(vector <LOWIMeasurementInfo*>& rttVector, float& rttMean_PerChain,
                              int32& rttStdev_PerChain, int32& rttMax_PerChain, int32& rttMin_PerChain,
                              int32& rttMedian_PerChain, struct req_map current_node_map,
                              uint32 *filteredBW_PerChain, eRttType rttType,
                              vector<LOWIMeasurementInfo *>& filtData_PerChain, uint32 chain_num_idx);
/*========================================================================================
 * computePerChainMean
 *
 * Description:
 *  This function compute the Mean Rtt per chain wise
 *
 * Parameters:
 *  filtData: rtt results for a single node
 *  mean   : Mean of the RTT measurements
 *
 * Return value:
 ========================================================================================*/
  void computeMean(vector <LOWIMeasurementInfo*>&  filtData, float& mean);

public:
  /* Divisor to use when computing outliers */
  static const int32 STDEV_DIV = 10;

/*========================================================================================
 * responseReceived
 *
 * Description:
 *   Notifies the listener whenever the response is received for a requests sent by the
 *   client. The notification happens in a different thread as compared to the thread in
 *   which the requests was made.
 *
 * Parameters:
 *  response: response received
 *
 * Usage:
 *  Client upon receiving this callback should inspect the type of response by
 *  response->getResponseType () and then type cast the response to retrieve the response
 *  data.
 *
 * Return value:
 *   none
 ========================================================================================*/
  void responseReceived(LOWIResponse *response);

  // ctor
  LOWIClientListenerImpl(const int32 stdevMult, const int32 RTT2_stdevMult) : mStdevMult(stdevMult), RTT2_mStdevMult(RTT2_stdevMult)
  {
  }
  // dtor
  virtual ~LOWIClientListenerImpl();
};

#endif // __LISTENER_IMPL_H__
