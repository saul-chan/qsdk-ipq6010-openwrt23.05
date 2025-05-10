/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

        LOWI API

GENERAL DESCRIPTION
  This file contains the structure definition for external application to use

Copyright (c) 2021-2024 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

Not a Contribution.

Copyright (C) 2005-2008 The Android Open Source Project

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
=============================================================================*/
#ifndef __LOWI_EXTERNAL_H__
#define __LOWI_EXTERNAL_H__

#include <string>

#ifdef __cplusplus
extern "C"
{
#endif

#define LOWI_COUNTRY_CODE_LEN 2
#define CIVIC_EXT_INFO_LEN 256 // civic information size in bytes

/* Macros for min/max.  */
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef unsigned char byte;
typedef unsigned char u8;
typedef signed char s8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef int lowi_request_id;
typedef int lowi_channel;                       // indicates channel frequency in MHz
typedef int lowi_rssi;
typedef byte mac_addr[6];
typedef byte oui[3];
typedef int64_t lowi_timestamp;                 // In microseconds (us)
typedef int64_t lowi_timespan;

typedef struct {
   u32 preamble   :3;   // 0: OFDM, 1:CCK, 2:HT 3:VHT 4:HE 5..7 reserved
   u32 nss        :2;   // 0:1x1, 1:2x2, 3:3x3, 4:4x4
   u32 bw         :6;   // 0x4:20MHz, 0x8:40Mhz, 0x10:80Mhz, 0x20:160Mhz
   u32 rateMcsIdx :8;   // OFDM/CCK rate code would be as per ieee std in the units of 0.5mbps
                        // HT/VHT/HE it would be mcs index
   u32 reserved  :13;   // reserved
   u32 bitrate;         // units of 100 Kbps
} lowi_rate;

/* Ranging status */
 typedef enum {
    RTT_STATUS_SUCCESS       = 0,
    RTT_STATUS_FAILURE       = 1,           // general failure status
    RTT_STATUS_FAIL_NO_RSP   = 2,           // target STA does not respond to request
    RTT_STATUS_FAIL_REJECTED = 3,           // request rejected. Applies to 2-sided RTT only
    RTT_STATUS_FAIL_NOT_SCHEDULED_YET  = 4,
    RTT_STATUS_FAIL_TM_TIMEOUT         = 5, // timing measurement times out
    RTT_STATUS_FAIL_AP_ON_DIFF_CHANNEL = 6, // Target on different channel, cannot range
    RTT_STATUS_FAIL_NO_CAPABILITY  = 7,     // ranging not supported
    RTT_STATUS_ABORTED             = 8,     // request aborted for unknown reason
    RTT_STATUS_FAIL_INVALID_TS     = 9,     // Invalid T1-T4 timestamp
    RTT_STATUS_FAIL_PROTOCOL       = 10,    // 11mc protocol failed
    RTT_STATUS_FAIL_SCHEDULE       = 11,    // request could not be scheduled
    RTT_STATUS_FAIL_BUSY_TRY_LATER = 12,    // responder cannot collaborate at time of request
    RTT_STATUS_INVALID_REQ         = 13,    // bad request args
    RTT_STATUS_NO_WIFI             = 14,    // WiFi not enabled
    RTT_STATUS_FAIL_FTM_PARAM_OVERRIDE = 15, // Responder overrides param info, cannot range with new params
    RTT_STATUS_NAN_RANGING_PROTOCOL_FAILURE =16, //Negotiation failure
    RTT_STATUS_NAN_RANGING_CONCURRENCY_NOT_SUPPORTED=17, //concurrency not supported (NDP+RTT)
} lowi_rtt_status;

/* RTT peer type */
typedef enum {
     RTT_PEER_AP         = 0x1,
     RTT_PEER_STA        = 0x2,
     RTT_PEER_P2P_GO     = 0x3,
     RTT_PEER_P2P_CLIENT = 0x4,
     RTT_PEER_NAN        = 0x5,
} rtt_peer_type;

 /* RTT Measurement Bandwidth */
 typedef enum {
     LOWI_RTT_BW_20  = 0x04,
     LOWI_RTT_BW_40  = 0x08,
     LOWI_RTT_BW_80  = 0x10,
     LOWI_RTT_BW_160 = 0x20,
 } lowi_rtt_bw;

 /* RTT Measurement Preamble */
 typedef enum {
     LOWI_RTT_PREAMBLE_LEGACY = 0x1,
     LOWI_RTT_PREAMBLE_HT     = 0x2,
     LOWI_RTT_PREAMBLE_VHT    = 0x4,
     LOWI_RTT_PREAMBLE_HE     = 0x8,
} lowi_rtt_preamble;

typedef enum {
     RTT_TYPE_1_SIDED = 0x1,
     RTT_TYPE_2_SIDED = 0x2,
} lowi_rtt_type;

typedef enum {
     LOWI_CHAN_WIDTH_20    = 0,
     LOWI_CHAN_WIDTH_40    = 1,
     LOWI_CHAN_WIDTH_80    = 2,
     LOWI_CHAN_WIDTH_160   = 3,
     LOWI_CHAN_WIDTH_MAX   = 4,
     LOWI_CHAN_WIDTH_INVALID = -1
} lowi_channel_width;

typedef enum {
     LOWI_CHAIN_IDX_0          = 0,
     LOWI_CHAIN_IDX_1          = 1,
     LOWI_CHAIN_IDX_2          = 2,
     LOWI_CHAIN_IDX_3          = 3,
     LOWI_CHAIN_IDX_4          = 4,
     LOWI_CHAIN_IDX_5          = 5,
     LOWI_CHAIN_IDX_6          = 6,
     LOWI_CHAIN_IDX_7          = 7,
     LOWI_CHAIN_IDX_MAX        = 8
} lowi_chain_index;

/* channel information */
typedef struct {
   lowi_channel_width width;   // channel width (20, 40, 80, 80+80, 160)
   lowi_channel center_freq;   // primary 20 MHz channel
   lowi_channel center_freq0;  // center frequency (MHz) first segment
   lowi_channel center_freq1;  // center frequency (MHz) second segment
} lowi_channel_info;

/** Defines supported TMR types for 11AZ Ranging scan request */
enum eTmrType
{
  TMR_LEGACY = 0, /* Legacy/EDCA based TMR Ranging */
  TMR_NTB    = 1, /* Non triggered based Ranging */
  TMR_TB     = 2, /* Triggered based Ranging */
};

/** Defines supported security types for 11AZ Ranging scan request */
enum eSecType
{
  SEC_NONE    = 0, /* No security while using 11AZ */
  SEC_FTM     = 1, /* Mac level security while using 11AZ */
  SEC_FTM_LTF = 2, /* Mac + Phy level security  while using 11AZ */
};

typedef enum {
    LOWI_SUCCESS = 0,
    LOWI_ERROR_NONE = 0,
    LOWI_ERROR_UNKNOWN = -1,
    LOWI_ERROR_UNINITIALIZED = -2,
    LOWI_ERROR_NOT_SUPPORTED = -3,
    LOWI_ERROR_NOT_AVAILABLE = -4,              // Not available right now, but try later
    LOWI_ERROR_INVALID_ARGS = -5,
    LOWI_ERROR_INVALID_REQUEST_ID = -6,
    LOWI_ERROR_TIMED_OUT = -7,
    LOWI_ERROR_TOO_MANY_REQUESTS = -8,          // Too many instances of this request
    LOWI_ERROR_OUT_OF_MEMORY = -9,
    LOWI_ERROR_BUSY = -10,
} lowi_status;

/** This enum defines the Phy mode */
enum LOWIPhyMode
{
  LOWI_PHY_MODE_UNKNOWN       = -1,
  LOWI_PHY_MODE_11A           = 0,  /* 11a Mode */
  LOWI_PHY_MODE_11G           = 1,  /* 11b/g Mode */
  LOWI_PHY_MODE_11B           = 2,  /* 11b Mode */
  LOWI_PHY_MODE_11GONLY       = 3,  /* 11g only Mode */
  LOWI_PHY_MODE_11NA_HT20     = 4,  /* 11na HT20 mode */
  LOWI_PHY_MODE_11NG_HT20     = 5,  /* 11ng HT20 mode */
  LOWI_PHY_MODE_11NA_HT40     = 6,  /* 11na HT40 mode */
  LOWI_PHY_MODE_11NG_HT40     = 7,  /* 11ng HT40 mode */
  LOWI_PHY_MODE_11AC_VHT20    = 8,  /* 5G 11ac VHT20 mode */
  LOWI_PHY_MODE_11AC_VHT40    = 9,  /* 5G 11ac VHT40 mode */
  LOWI_PHY_MODE_11AC_VHT80    = 10, /* 5G 11ac VHT80 mode */
  LOWI_PHY_MODE_11AC_VHT20_2G = 11, /* 2G 11ac VHT20 mode */
  LOWI_PHY_MODE_11AC_VHT40_2G = 12, /* 2G 11ac VHT40 mode */
  LOWI_PHY_MODE_11AC_VHT80_2G = 13, /* 2G 11ac VHT80 mode */
  LOWI_PHY_MODE_11AC_VHT80_80 = 14, /* 5G 11ac VHT80_80 mode */
  LOWI_PHY_MODE_11AC_VHT160   = 15, /* 5G 11ac VHT160 mode */
  LOWI_PHY_MODE_11AX_HE20     = 16, /* 5G 11ax HE20 mode */
  LOWI_PHY_MODE_11AX_HE40     = 17, /* 5G 11ax HE40 mode */
  LOWI_PHY_MODE_11AX_HE80     = 18, /* 5G 11ax HE80 mode */
  LOWI_PHY_MODE_11AX_HE80_80  = 19, /* 5G 11ax HE80_80 mode */
  LOWI_PHY_MODE_11AX_HE160    = 20, /* 5G 11ax HE160 mode */
  LOWI_PHY_MODE_11AX_HE20_2G  = 21, /* 2G 11ax HE20 mode */
  LOWI_PHY_MODE_11AX_HE40_2G  = 22, /* 2G 11ax HE40 mode */
  LOWI_PHY_MODE_11AX_HE80_2G  = 23, /* 2G 11ax HE80 mode */

  LOWI_PHY_MODE_MAX           = LOWI_PHY_MODE_11AX_HE80_2G
};

/* RTT configuration */
struct lowi_rtt_config {
     mac_addr addr;                 // peer device mac address
     lowi_rtt_type type;            // 1-sided or 2-sided RTT
     rtt_peer_type peer;            // optional - peer device hint (STA, P2P, AP)
     lowi_channel_info channel;     // Required for STA-AP mode, optional for P2P, NBD etc.
     unsigned burst_period;         // Time interval between bursts (units: 100 ms).
                                    // Applies to 1-sided and 2-sided RTT multi-burst requests.
                                    // Range: 0-31, 0: no preference by initiator (2-sided RTT)
     unsigned num_burst;            // Total number of RTT bursts to be executed. It will be
                                   // specified in the same way as the parameter "Number of
                                   // Burst Exponent" found in the FTM frame format. It
                                   // applies to both: 1-sided RTT and 2-sided RTT. Valid
                                   // values are 0 to 15 as defined in 802.11mc std.
                                   // 0 means single shot
                                    // The implication of this parameter on the maximum
                                   // number of RTT results is the following:
                                   // for 1-sided RTT: max num of RTT results = (2^num_burst)*(num_frames_per_burst)
                                    // for 2-sided RTT: max num of RTT results = (2^num_burst)*(num_frames_per_burst - 1)
    unsigned num_frames_per_burst; // num of frames per burst.
                                    // Minimum value = 1, Maximum value = 31
                                   // For 2-sided this equals the number of FTM frames
                                   // to be attempted in a single burst. This also
                                   // equals the number of FTM frames that the
                                    // initiator will request that the responder send
                                    // in a single frame.
    unsigned num_retries_per_rtt_frame; // number of retries for a failed RTT frame. Applies
                                         // to 1-sided RTT only. Minimum value = 0, Maximum value = 3
    byte asap;                      // Perform rtt measurements instantly
    byte force_legacy_ack;          // Force ACK received for FTM frames to be of legacy type only

    //following fields are only valid for 2-side RTT
     unsigned num_retries_per_ftmr; // Maximum number of retries that the initiator can
                                  // retry an FTMR frame.
                                   // Minimum value = 0, Maximum value = 3
     byte LCI_request;              // 1: request LCI, 0: do not request LCI
     byte LCR_request;              // 1: request LCR, 0: do not request LCR
     unsigned burst_duration;       // Applies to 1-sided and 2-sided RTT. Valid values will
                                  // be 2-11 and 15 as specified by the 802.11mc std for
                                   // the FTM parameter burst duration. In a multi-burst
                                   // request, if responder overrides with larger value,
                                   // the initiator will return failure. In a single-burst
                                    // request if responder overrides with larger value,
                                   // the initiator will sent TMR_STOP to terminate RTT
                                    // at the end of the burst_duration it requested.
     lowi_rtt_preamble preamble;    // RTT preamble to be used in the RTT frames
     lowi_rtt_bw bw;                // RTT BW to be used in the RTT frames
     std::string Interface;
     LOWIPhyMode phyMode;
     eTmrType tmrtype;              // 11az TMR type
     eSecType sectype;              // 11az Security type
     unsigned tbPriority;           // 11az TB priority
     unsigned tmrMinDelta;          // 11az Min time between measurements
     unsigned tmrMaxDelta;          // 11az Max time between measurements
     uint32_t txrxChainIndexInitiator; // Tx Rx chain index for Initiator
     uint32_t txrxChainIndexResponder; // Tx Rx chain index for Responder
     uint32_t iteratorInitiator;        // Pattern followed by Initiator
     uint32_t iteratorResponder;        // Pattern followed by Responder
     bool isRttBgNode;                 // Is rtt bg node
     uint32_t tbPeriodicity;           // TB measurement periodicity in units of 10 TUs
     uint32_t tbDuration;             // TB measurement duration in units of 100 Msec
     uint8_t tbMaxSessionExpiry; // TB max session expiry, Max delay between continous measurements
     bool I2RFbPolicy;	// I2R LMR Feedback Policy is set or not.
     uint32_t rttChannelSwitchPriority; // RTT channel switch priority

     // Default initialization
     lowi_rtt_config()
     {
       type = RTT_TYPE_1_SIDED;
       channel.center_freq1 = 0;
       burst_period = 0;
       num_burst = 0;
       num_frames_per_burst = 8;
       num_retries_per_rtt_frame = 2;
       num_retries_per_ftmr = 2;
       asap = 1;
       force_legacy_ack = 0;
       burst_duration = 15;
       Interface = "wifi0";
       LCI_request = 0;
       LCR_request = 0;
       tmrtype = TMR_LEGACY;
       sectype = SEC_NONE;
       tbPriority = 0;
       tmrMinDelta = 0;
       tmrMaxDelta = 0;
       txrxChainIndexInitiator = 0;
       txrxChainIndexResponder = 0;
       iteratorInitiator = 0;
       iteratorResponder = 0;
       isRttBgNode = false;
       tbPeriodicity = 0;
       tbDuration = 0;
       tbMaxSessionExpiry = 0;
       I2RFbPolicy = 0;
       rttChannelSwitchPriority = 0;

     }
};

/* Format of information elements found in the beacon */
typedef struct {
   byte id;                            // element identifier
   byte len;                           // number of bytes to follow
   byte data[];
} lowi_information_element;

/* RTT results */
typedef struct {
     mac_addr addr;                // device mac address
     unsigned burst_num;           // burst number in a multi-burst request
     unsigned measurement_number;  // Total RTT measurement frames attempted
     unsigned tot_successful_meas;      // Total successful RTT measurement frames
     unsigned meas_as_per_config;  // Total number of successful measurements considered for
                                   //measurment results when weighted mean/skip bandwidth is enabled
     unsigned meas_as_per_config_perchain[4]; // Per chain successful measurements
     unsigned tot_meas_per_bw[LOWI_CHAN_WIDTH_MAX];  // Total number of measurments obtained for each bandwidth
     unsigned final_meas_per_bw[LOWI_CHAN_WIDTH_MAX];  // Total number of measurments obtained for
                                                        //each bandwidth after filtering outliers
     unsigned tot_meas_per_bw_perchain[4][LOWI_CHAN_WIDTH_MAX];  // Total number of measurments obtained for each bandwidth per chain
     unsigned final_meas_per_bw_perchain[4][LOWI_CHAN_WIDTH_MAX];  // Total number of measurments obtained for
                                                        //each bandwidth after filtering outliers per chain
     unsigned request_chain_index[LOWI_CHAIN_IDX_MAX];  // Requested chain index
     unsigned used_chain_index[LOWI_CHAIN_IDX_MAX];     // Used chain index
     unsigned request_chain_index_PerChain[4][LOWI_CHAIN_IDX_MAX];  // Requested chain index per chain
     unsigned used_chain_index_PerChain[4][LOWI_CHAIN_IDX_MAX];     // Used chain index per chain
     lowi_rssi rssi_perchain[4];        // average rssi in 0.5 dB steps e.g. 143 implies -71.5 dB per chain wise
     lowi_rssi rssi_spread_perchain[4]; // difference between max and min rtt times recorded in picoseconds per chain wise
     lowi_timespan rtt_perchain[4];     // round trip time in picoseconds per chain wise
     lowi_timespan rtt_sd_perchain[4];  // rtt standard deviation in picoseconds per chain wise
     lowi_timespan rtt_spread_perchain[4];     // difference between max and min rtt times recorded in picoseconds per chain wise
     int distance_mm_perchain[4];     // distance in mm (optional) per chain wise
     int distance_sd_mm_perchain[4];   // standard deviation in mm (optional) per chain wise
     int distance_spread_mm_perchain[4];  // difference between max and min distance recorded in mm (optional) per chain wise
     byte  number_per_burst_peer;  // Maximum number of "FTM frames per burst" supported by
                                   // the responder STA. Applies to 2-sided RTT only.
                                   // If reponder overrides with larger value:
                                   // - for single-burst request initiator will truncate the
                                   // larger value and send a TMR_STOP after receiving as
                                   // many frames as originally requested.
                                   // - for multi-burst request, initiator will return
                                   // failure right away.
     lowi_rtt_status status;       // ranging status
     byte retry_after_duration;    // When status == RTT_STATUS_FAIL_BUSY_TRY_LATER,
                               // this will be the time provided by the responder as to
                                   // when the request can be tried again. Applies to 2-sided
                                   // RTT only. In sec, 1-31sec.
     lowi_rtt_type type;           // RTT type
     lowi_rssi rssi;               // average rssi in 0.5 dB steps e.g. 143 implies -71.5 dB
     lowi_rssi rssi_spread;        // rssi spread in 0.5 dB steps e.g. 5 implies 2.5 dB spread (optional)
     lowi_rate tx_rate;            // 1-sided RTT: TX rate of RTT frame.
                               // 2-sided RTT: TX rate of initiator's Ack in response to FTM frame.
     lowi_rate rx_rate;            // 1-sided RTT: TX rate of Ack from other side.
                               // 2-sided RTT: TX rate of FTM frame coming from responder.
     lowi_timespan rtt;            // round trip time in picoseconds
     lowi_timespan rtt_sd;         // rtt standard deviation in picoseconds
     lowi_timespan rtt_spread;     // difference between max and min rtt times recorded in picoseconds
     int distance_mm;              // distance in mm (optional)
     int distance_sd_mm;           // standard deviation in mm (optional)
     int distance_spread_mm;       // difference between max and min distance recorded in mm (optional)
     lowi_timestamp ts;            // time of the measurement (in microseconds since boot)
     int burst_duration;           // in ms, actual time taken by the FW to finish one burst
                               // measurement. Applies to 1-sided and 2-sided RTT.
     int negotiated_burst_num;     // Number of bursts allowed by the responder. Applies
                               // to 2-sided RTT only.
     lowi_information_element *LCI; // for 11mc only
     lowi_information_element *LCR; // for 11mc only

     /* TMR subtype, TB or NTB */
     uint8_t tmr_subtype;

     /* Security mode, FTM, sec LTF,*/
     uint8_t security_mode;

     /**
      * Value 0: ISTA does not transmit I2R LMR at the end of each measurement
      * exchange, if requested by RSTA.
      * Value 1: ISTA transmits I2R LMR at the end of each measurement exchange,
      * if requested by RSTA
      */
     bool i2r_lmr_feedback;

     /**
      * Value 0: The RSTA will be able to prioritize ranging operations to match
      * the ranging priority requested by the ISTA.
      * Value 1: The RSTA will not be able to prioritize ranging operations to
      * match the ranging priority requested by the ISTA.
      * Value 2 : Reserved
      * Value 3 : Reserved
      */
     uint8_t ranging_priority;
     /**
      * Value 0: RSTA estimates TOA using first path reporting.
      * Value 1: RSTA estimates TOA using phase shift.
      */
     bool r2i_toa_type;
     /**
      * Value 0: The feedback type in the I2R will be of the first path reporting.
      * Value 1: TOA feedback type in the I2R LMR to be phase shift type of TOA,
      *          corresponding to the average linear phase across the subcarriers.
      */
     bool i2r_toa_type;
     /**
      * The Format and Bandwidth subfield indicates the requested or allocated PPDU
      * format and bandwidth used to transmit the I2R/R2I NDP exchanged as part of
      * the Non-TB Ranging, or TB Ranging measurement exchange.
      *
      *    Field value  Format                     Bandwidth
      *    0              HE                            20
      *    1              HE                            40
      *    2              HE                            80
      *    3              HE                            80+80
      *    4              HE (two separate RF Los)      160
      *    5              HE (single RF LO)             160
      *    6-63           Reserved                      Reserved
      */
     uint8_t format_and_bw;
     /**
      * This subfield indicate if the measurement results from the current
      * measurement exchange are reported immediately by setting it to 1 (from the
      * current  measurement) or delayed by setting it to 0 (from the previous
      * measurement) in the R2I Location Measurement Report (LMR).
      */
     bool imdt_r2i_feedback;
     /**
      * This subfield indicate if the measurement results from the current
      * measurement exchange are reported immediately by setting it to 1 (from the
      * current measurement) or delayed by setting it to 0 (from the previous
      * measurement) in the I2R Location Measurement Report (LMR).
      */
     bool imdt_i2r_feedback;
     /**
      * Maximum number of LTF repetitions that the ISTA uses in the preamble of I2R
      * NDP.
      */
     uint8_t max_i2r_repetition;
     /**
      * Maximum number of LTF repetitions that the RSTA uses in the preamble of R2I
      * NDP.
      */
     uint8_t max_r2i_repetition;
     /**
      * For a non-AP STA:
      *  Set to 1 for a Class A device.
      *  Set to 0 for a Class B device.
      */
     bool device_class;
     /**
      * For an AP, indicates support for MU-MIMO reception of an HE TB PPDU on
      * an RU that spans the entire PPDU bandwidth (UL MU-MIMO).
      *
      * For a non-AP STA, indicates support for the transmission of an HE TB
      * PPDU on an RU that spans the entire PPDU bandwidth (UL MU-MIMO).
      *
      * Set to 0 if not supported.
      * Set to 1 if supported.
      */
     bool full_bw_ul_mumimo;
     /**
      * For bandwidth less than or equal to 80MHz the maximum number of
      * spatial-time streams to be used in R2I NDP in the session.
      */
     uint8_t max_r2i_sts_le_80mhz;
     /**
      * For bandwidth greater than 80MHz the maximum number of spatial-time streams
      * to be used in R2I NDP in the session.
      */
     uint8_t max_r2i_sts_gr_80mhz;
     /**
      * Maximum number of LTFs used in R2I NDP.
      */
     uint8_t max_r2i_ltf_total;
     /**
      * For bandwidth less than or equal to 80MHz the maximum number of
      * spatial-time streams to be used in I2R NDP in the session.
      */
     uint8_t max_i2r_sts_le_80mhz;
     /**
      * For bandwidth greater than 80MHz the maximum number of spatial-time streams
      * to be used in I2R NDP in the session.
      */
     uint8_t max_i2r_sts_gr_80mhz;
     /**
      * Maximum number of LTFs used in I2R NDP.
      */
     uint8_t max_i2r_ltf_total;
     /**
      * Indicates the minimum time between two consecutive range measurements
      * initiated by ISTA. (in units of 100us)
      */
     uint32_t min_time_between_measurements;
} lowi_rtt_result;

 /* RTT result callback */
typedef struct {
     void (*on_rtt_results) (lowi_request_id id, unsigned num_results, lowi_rtt_result *rtt_result[]);
} lowi_rtt_event_handler;

/* RTT Capabilities */
typedef struct {
    byte rtt_one_sided_supported;  // if 1-sided rtt data collection is supported
    byte rtt_ftm_supported;        // if ftm rtt data collection is supported
    byte lci_support;              // if initiator supports LCI request. Applies to 2-sided RTT
    byte lcr_support;              // if initiator supports LCR request. Applies to 2-sided RTT
    byte preamble_support;         // bit mask indicates what preamble is supported by initiator
    byte bw_support;               // bit mask indicates what BW is supported by initiator
    byte responder_supported;      // if 11mc responder mode is supported
    byte mc_version;               // draft 11mc spec version supported by chip. For instance,
                                   // version 4.0 should be 40 and version 4.3 should be 43 etc.
    bool rxAllChainSupport;        // if initiator supports rtt data collection on all the chains
} lowi_rtt_capabilities;

enum lowi_motion_pattern
{
  LOWI_MOTION_NOT_EXPECTED = 0, // Not expected to change location
  LOWI_MOTION_EXPECTED     = 1, // Expected to change location
  LOWI_MOTION_UNKNOWN      = 2  // Movement pattern unknown
};

typedef struct {
    int64_t   latitude;
    int64_t   longitude;
    int64_t   altitude;
    int8_t    latitude_unc;
    int8_t    longitude_unc;
    int8_t    altitude_unc;
    lowi_motion_pattern motion_pattern;
    int32_t   floor;
    int32_t   height_above_floor;
    int32_t   height_unc;
} lowi_lci_information;

typedef struct lowi_lcr_information {
  uint8_t  country_code[LOWI_COUNTRY_CODE_LEN]; // country code
  uint32_t length;                              // length of the info field
  int8_t   civic_info[CIVIC_EXT_INFO_LEN];          // Civic info to be copied in FTM frame
} lowi_lcr_information;


#define LOWI_MAJOR_VERSION      2
#define LOWI_MINOR_VERSION      1
#define LOWI_MICRO_VERSION      1

/* LOWI supported capabilities bit masks */
#define ONE_SIDED_RANGING_SUPPORTED   0x00000001
#define DUAL_SIDED_RANGING_SUPPORED   0x00000002
#define GSCAN_SUPPORTED               0x00000004

/*
 * This structure is a table of function pointers to the functions
 * used for the interface with LOWI
 */
typedef struct
{
  /* lowi-client interface functions */
  int (*init)();
  int (*destroy)();
  /* rtt functions */
  int (*get_rtt_capabilities)(lowi_rtt_capabilities *capabilities, std::string interface);
  int (*rtt_range_request)(u32 request_id,
                           u32 num_rtt_config,
                           lowi_rtt_config rtt_config[],
                           lowi_rtt_event_handler handler);
  int (*rtt_range_cancel)(u32 request_id,
                          u32 num_devices,
                          mac_addr addr[]);
  /* Additional lowi-client interface functions */
  int (*get_lowi_version) (u16* major_version,
                           u16* minor_version,
                           u16* micro_version);
  int (*get_lowi_capabilities)(u32* capabilities, std::string interface);

   // API to configure the LCI. Used in RTT Responder mode only
   lowi_status (*rtt_set_lci)(lowi_request_id id,
                              lowi_lci_information *lci,
                              std::string interface);

   // API to configure the LCR. Used in RTT Responder mode only.
   lowi_status (*rtt_set_lcr)(lowi_request_id id,
                              lowi_lcr_information *lcr,
                              std::string interface);
}lowi_cb_table_t;

 /*
   * This is a function pointer to a function that gets the table
   * of callback functions populated by LOWI
   */
lowi_cb_table_t* lowi_get_cb_table();

#ifdef __cplusplus
}
#endif

#endif //__LOWI_EXTERNAL_H__
