/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

         LOWI Wireless Scan Service Test Module Through Library API (LOWI Test)

GENERAL DESCRIPTION
  This file contains the implementation of LOWI Test which calls the lowi lib and exercises the LOWI
  module.

Copyright (c) 2021-2024 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
=============================================================================*/

/*--------------------------------------------------------------------------
 * Include Files
 * -----------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/param.h>
#include "lowi_external.h"
#include <algorithm>
#include <numeric>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <map>
#include <sys/timerfd.h>

/* Timeout for scan response */
#define LOWI_SCAN_TIMEOUT_MS 75000
#define LOWI_ASYNC_SCAN_TIMEOUT_MS 60000 // 1 minutes
#define LOWI_BATCHING_SCAN_TIMEOUT_MS 600000 // 10 minutes
#define LOWI_CAPABILITY_SUBS_TIMEOUT_MS 60000 // 1 minutes
#define NSECS_PER_SEC 1000000000
#define MSECS_PER_SEC 1000
#define NSECS_PER_MSEC (NSECS_PER_SEC/MSECS_PER_SEC)

/* Max Buffer length for Command */
#define LOWI_MAX_CMD_LEN 200
#define MAX_RTT_CONFIG_REQ 10
#define LOWI_TEST_DEFAULT_RTT_MEAS 5
#define MAX_BUF_LENGTH 1024

lowi_rtt_event_handler op;
static char const *TAG = "LOWI_INTERFACE_TEST";
static char const *g_tag = "QCALOG";

enum ERROR_LEVEL
{
  EL_LOG_OFF = 0, EL_ERROR = 1, EL_WARNING = 2, EL_INFO = 3, EL_DEBUG = 4, EL_VERBOSE = 5, EL_LOG_ALL = 100
};

#ifdef CLOCK_BOOTTIME
struct timespec ts;
int clock_id       = ( clock_gettime(CLOCK_BOOTTIME_ALARM, &ts) == 0 ?
                  CLOCK_BOOTTIME_ALARM : CLOCK_REALTIME );
#else
int clock_id       = CLOCK_REALTIME;
#endif
int timerFd        = timerfd_create(clock_id, 0);

ERROR_LEVEL g_log_level      = EL_INFO;
bool g_print_11az_params = false;

extern lowi_rtt_capabilities *rttCachedCapabilities;

/* Flag to keep track of number of RTT requests and to wait untill all Results are obtained */
uint32_t g_op_recvd = 0;

long long get_time_rtc_ms()
{
  struct timeval  present_time;
  long long       current_time_msec = 0;

  // present time: seconds, and microseconds
  if(0 == gettimeofday(&present_time, NULL))
  {

    // Calculate absolute expire time (to avoid data overflow)
    current_time_msec = present_time.tv_sec;
    current_time_msec *= 1000;  // convert to milli-seconds

    current_time_msec += (present_time.tv_usec + 500) / 1000;
  }
  return current_time_msec;
}

// Returns monotonic time
long long get_time_monotonic_ms()
{
#ifdef CLOCK_MONOTONIC
  uint64_t current_time_msec = 0;
  struct timespec tp;

  if (0 == clock_gettime(CLOCK_MONOTONIC, &tp))
  {

    current_time_msec = tp.tv_sec;
    current_time_msec *= 1000;  // convert to milli-seconds

    current_time_msec += ((tp.tv_nsec + 500000) / 1000000);
  }
  return current_time_msec;
#else
  return get_time_rtc_ms();
#endif
}

void log_display(int debug_code, const char * const log_tag, const char * const format, ...)
{
  if (debug_code <= g_log_level) {
      char buf[MAX_BUF_LENGTH];
      int len = 0;
      va_list args;
      va_start(args, format);
      len = snprintf(buf+len , sizeof(buf)-len, "[%.2f][%s-%s] ",
                         get_time_monotonic_ms() / 1000.0,
                         g_tag, log_tag);

      vsnprintf(buf+len, sizeof(buf)-len, format, args);
      puts(buf);
      va_end (args);
    }
}

#define log_error(...)   log_display(EL_ERROR, TAG, __VA_ARGS__);
#define log_info(...)    log_display(EL_INFO, TAG, __VA_ARGS__);
#define log_debug(...)   log_display(EL_DEBUG, TAG, __VA_ARGS__);
#define log_verbose(...) log_display(EL_VERBOSE, TAG, __VA_ARGS__);

int64_t lowi_test_get_time_ms(void)
{
    struct timespec ts;

    clock_gettime(clock_id, &ts);
    log_debug("%s: TIME Read = %ld.%09ld = %d msec",  __FUNCTION__, ts.tv_sec, ts.tv_nsec,
        (((int64_t) ts.tv_sec) * MSECS_PER_SEC) + (ts.tv_nsec/NSECS_PER_MSEC));

    return (((int64_t) ts.tv_sec) * MSECS_PER_SEC) + (ts.tv_nsec/NSECS_PER_MSEC);
}

static void lowi_test_start_timer(const uint16_t duration_ms)
{
    struct itimerspec its;
    int64_t start_time_ms;

    its.it_value.tv_sec = duration_ms/MSECS_PER_SEC;
    its.it_value.tv_nsec = (duration_ms%MSECS_PER_SEC) * NSECS_PER_MSEC;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    int ret = timerfd_settime(timerFd, 0, &its, NULL);

    if (ret != 0) {
        log_debug("%s: Timerfd_settime failed ret = %d %d, %s",
            __FUNCTION__, ret, errno, strerror(errno));
    }

    start_time_ms = lowi_test_get_time_ms();
    log_debug("%s: Timer Set at %d  ms for %d ms",
        __FUNCTION__, start_time_ms, duration_ms);

}

static void lowi_test_wait(const uint16_t duration_ms)
{
    int ret_val;
    int64_t end_time_ms;
    uint64_t exp;
    /* this variable will be greater than or equal to one if timer
                        expires,otherwise it will be returned zero at timefd read call   */

    if (duration_ms == 0) {
        return;
    }
    do {
        ret_val = read(timerFd, &exp, sizeof(exp));

        if (ret_val < 0) {
            log_debug("%s: lowi_test_wait: read of timerFd failed %s",
                        __FUNCTION__, strerror(errno));
            return;
        }

        if (exp >= 1) {
            end_time_ms = lowi_test_get_time_ms();
            log_debug("%s:timer ends at %d ms", __FUNCTION__, end_time_ms);
            break;
        }
    }
    while (1);

}

void signal_handler(int signal_id)
{
 log_debug("%s Received signal [%d][%s], ignore",
       __FUNCTION__, signal_id, strsignal(signal_id));
}

void rtt_results(lowi_request_id id, unsigned num_results, lowi_rtt_result * rtt_result[])
{
    lowi_information_element *LCI = NULL;
    lowi_information_element *LCR = NULL;

    log_info("%s  \t\t/************ LOWI RTT RESULTS FOR REQ ID %d ************/", __FUNCTION__, id);

    for (unsigned i = 0; i < num_results; i++) {
         if (rtt_result[i]->tot_successful_meas < 3) {
             log_info("\t Outlier detection and Skip mismatch BW not applied since received measurements are less than 3");
         }

         log_info("\n\t\tResponder's MAC: %02X:%02X:%02X:%02X:%02X:%02X"
                  "\n\t\tNo of Frames Attempted: %d \n\t\tNo of Successful RTT Measurements: %d"
                  "\n\t\tNo of Measurements taken for RTT results: %d",
                  rtt_result[i]->addr[0], rtt_result[i]->addr[1],
                  rtt_result[i]->addr[2], rtt_result[i]->addr[3],
                  rtt_result[i]->addr[4], rtt_result[i]->addr[5],
                  rtt_result[i]->measurement_number,
                  rtt_result[i]->tot_successful_meas,
                  rtt_result[i]->meas_as_per_config);

         if (rttCachedCapabilities->rxAllChainSupport && rtt_result[i]->tmr_subtype < 1){
           for (int ii = 0; ii < 4; ++ii)
           {
               log_info("\n\t\tChain No: %d",ii);
               log_info("\n\t\tMeasurements obtained Per Chain in each BW = [20]:%2d [40]:%2d [80]:%2d [160]:%2d"
                        "\n\t\tMeasurements after outliers Per Chain in each BW = [20]:%2d [40]:%2d [80]:%2d [160]:%2d"
                        "\n\t\tRequested Chain Index = [0]:%2d [1]:%2d [2]:%2d [3]:%2d [4]:%2d [5]:%2d [6]:%2d [7]:%2d"
                        "\n\t\tUsed Chain Index = [0]:%2d [1]:%2d [2]:%2d [3]:%2d [4]:%2d [5]:%2d [6]:%2d [7]:%2d"
                        "\n\t\tRSSI of Chain(0.5dB) : %d \n\t\tRSSI Spread(0.5dB) : %d"
                        "\n\t\tRTT (ps) of Chain: %lld \n\t\tRTT_SD (ps): %lld \n\t\tRTT_SPREAD (ps): %lld"
                        "\n\t\tDistance(mm): %d \n\t\tDistance SD(mm): %d \n\t\tDistance Spread(mm): %d",
                        rtt_result[i]->tot_meas_per_bw_perchain[ii][0],
                        rtt_result[i]->tot_meas_per_bw_perchain[ii][1],
                        rtt_result[i]->tot_meas_per_bw_perchain[ii][2],
                        rtt_result[i]->tot_meas_per_bw_perchain[ii][3],
                        rtt_result[i]->final_meas_per_bw_perchain[ii][0],
                        rtt_result[i]->final_meas_per_bw_perchain[ii][1],
                        rtt_result[i]->final_meas_per_bw_perchain[ii][2],
                        rtt_result[i]->final_meas_per_bw_perchain[ii][3],
                        rtt_result[i]->request_chain_index_PerChain[ii][0],
                        rtt_result[i]->request_chain_index_PerChain[ii][1],
                        rtt_result[i]->request_chain_index_PerChain[ii][2],
                        rtt_result[i]->request_chain_index_PerChain[ii][3],
                        rtt_result[i]->request_chain_index_PerChain[ii][4],
                        rtt_result[i]->request_chain_index_PerChain[ii][5],
                        rtt_result[i]->request_chain_index_PerChain[ii][6],
                        rtt_result[i]->request_chain_index_PerChain[ii][7],
                        rtt_result[i]->used_chain_index_PerChain[ii][0],
                        rtt_result[i]->used_chain_index_PerChain[ii][1],
                        rtt_result[i]->used_chain_index_PerChain[ii][2],
                        rtt_result[i]->used_chain_index_PerChain[ii][3],
                        rtt_result[i]->used_chain_index_PerChain[ii][4],
                        rtt_result[i]->used_chain_index_PerChain[ii][5],
                        rtt_result[i]->used_chain_index_PerChain[ii][6],
                        rtt_result[i]->used_chain_index_PerChain[ii][7],
                        rtt_result[i]->rssi_perchain[ii],
                        rtt_result[i]->rssi_spread_perchain[ii],
                        rtt_result[i]->rtt_perchain[ii],
                        rtt_result[i]->rtt_sd_perchain[ii],
                        rtt_result[i]->rtt_spread_perchain[ii],
                        rtt_result[i]->distance_mm_perchain[ii],
                        rtt_result[i]->distance_sd_mm_perchain[ii],
                        rtt_result[i]->distance_spread_mm_perchain[ii]);
           }
         }

         log_info("\n\t\tCombined chain output!\n");
         log_info("\n\t\tMeasurements obtained in each BW = [20]:%2d [40]:%2d [80]:%2d [160]:%2d"
                  "\n\t\tMeasurements after outliers in each BW = [20]:%2d [40]:%2d [80]:%2d [160]:%2d"
                  "\n\t\tRSSI (0.5dB) : %d \n\t\tRSSI Spread(0.5dB) : %d"
                  "\n\t\tRTT (ps): %lld \n\t\tRTT_SD (ps): %lld \n\t\tRTT_SPREAD (ps): %lld"
                  "\n\t\tDistance(mm): %d \n\t\tDistance SD(mm): %d \n\t\tDistance Spread(mm): %d",
                  rtt_result[i]->tot_meas_per_bw[0],
                  rtt_result[i]->tot_meas_per_bw[1],
                  rtt_result[i]->tot_meas_per_bw[2],
                  rtt_result[i]->tot_meas_per_bw[3],
                  rtt_result[i]->final_meas_per_bw[0],
                  rtt_result[i]->final_meas_per_bw[1],
                  rtt_result[i]->final_meas_per_bw[2],
                  rtt_result[i]->final_meas_per_bw[3],
                  rtt_result[i]->rssi,
                  rtt_result[i]->rssi_spread,
                  rtt_result[i]->rtt,
                  rtt_result[i]->rtt_sd,
                  rtt_result[i]->rtt_spread,
                  rtt_result[i]->distance_mm,
                  rtt_result[i]->distance_sd_mm,
                  rtt_result[i]->distance_spread_mm);

         if (rttCachedCapabilities->rxAllChainSupport && rtt_result[i]->tmr_subtype < 1) {
             log_info("\n\t\tRequested Chain Index = [0]:%2d [1]:%2d [2]:%2d [3]:%2d [4]:%2d [5]:%2d [6]:%2d [7]:%2d"
                      "\n\t\tUsed Chain Index = [0]:%2d [1]:%2d [2]:%2d [3]:%2d [4]:%2d [5]:%2d [6]:%2d [7]:%2d",
                      rtt_result[i]->request_chain_index[0],
                      rtt_result[i]->request_chain_index[1],
                      rtt_result[i]->request_chain_index[2],
                      rtt_result[i]->request_chain_index[3],
                      rtt_result[i]->request_chain_index[4],
                      rtt_result[i]->request_chain_index[5],
                      rtt_result[i]->request_chain_index[6],
                      rtt_result[i]->request_chain_index[7],
                      rtt_result[i]->used_chain_index[0],
                      rtt_result[i]->used_chain_index[1],
                      rtt_result[i]->used_chain_index[2],
                      rtt_result[i]->used_chain_index[3],
                      rtt_result[i]->used_chain_index[4],
                      rtt_result[i]->used_chain_index[5],
                      rtt_result[i]->used_chain_index[6],
                      rtt_result[i]->used_chain_index[7]);
         }

         if (g_print_11az_params) {
             log_info("11az params: \n\t\ttmr_subtype:%u \n\t\tsecurity_mode:%u\n\t\t"
                     "i2r_lmr_feedback:%u \n\t\tranging_priority:%u \n\t\t"
                     "r2i_toa_type:%u \n\t\ti2r_toa_type:%u \n\t\tformat_and_bw:%u \n\t\t"
                     "imdt_r2i_feedback:%u \n\t\timdt_i2r_feedback:%u \n\t\t"
                     "max_i2r_repetition:%u \n\t\tmax_r2i_repetition:%u device_class:%u \n\t\t"
                     "full_bw_ul_mumimo:%u \n\t\tmax_r2i_sts_le_80mhz:%u \n\t\t"
                     "max_r2i_sts_gr_80mhz:%u \n\t\tmax_r2i_ltf_total:%u \n\t\t"
                     "max_i2r_sts_le_80mhz:%u \n\t\tmax_i2r_sts_gr_80mhz:%u \n\t\t"
                     "max_i2r_ltf_total:%u \n\t\tmin_time_between_measurements:%u",
                     rtt_result[i]->i2r_lmr_feedback,
                     rtt_result[i]->ranging_priority,
                     rtt_result[i]->r2i_toa_type,
                     rtt_result[i]->i2r_toa_type,
                     rtt_result[i]->format_and_bw,
                     rtt_result[i]->imdt_r2i_feedback,
                     rtt_result[i]->imdt_i2r_feedback,
                     rtt_result[i]->max_i2r_repetition,
                     rtt_result[i]->max_r2i_repetition,
                     rtt_result[i]->device_class,
                     rtt_result[i]->full_bw_ul_mumimo,
                     rtt_result[i]->max_r2i_sts_le_80mhz,
                     rtt_result[i]->max_r2i_sts_gr_80mhz,
                     rtt_result[i]->max_r2i_ltf_total,
                     rtt_result[i]->max_i2r_sts_le_80mhz,
                     rtt_result[i]->max_i2r_sts_gr_80mhz,
                     rtt_result[i]->max_i2r_ltf_total,
                     rtt_result[i]->min_time_between_measurements);
         }

       if (rtt_result[i]->LCI != NULL) {
           LCI = rtt_result[i]->LCI;
           if (LCI->id) {
               log_info("%s \t\t LCI id %d", __FUNCTION__, (uint8_t)LCI->id);
           }
           if (LCI->len) {
               uint8_t len = (uint8_t)LCI->len;
               if ( len > 0 ) {
                    log_info("%s \t\t LCI DATA :", __FUNCTION__);
                    printf("\t\t");
                    for (int it = 0 ; it<len ; it++)
                         printf("%02X",(uint8_t)LCI->data[it]);
                    printf("\n");
               }
           }
       }

       if (rtt_result[i]->LCR != NULL) {
           LCR = rtt_result[i]->LCR;
           if (LCR->id) {
               log_info("%s \t\t LCR id %d", __FUNCTION__, (uint8_t)LCR->id);
           }
           if (LCR->len) {
               uint8_t len = (uint8_t)LCR->len;
               if ( len > 0 ) {
                    log_info("%s \t\t LCR DATA :", __FUNCTION__);
                    printf("\t\t");
                    for (int it = 0 ; it<len ; it++)
                         printf("%02X",(uint8_t)LCR->data[it]);
                    printf("\n");
               }
           }
       }

         log_debug("\n\t\tTx Preamble: %d NSS: %d Bandwidth: %d MCS: %d BitRate(100Kbps): %d"
                  "\n\t\tRx Preamble: %d NSS: %d Bandwidth: %d MCS: %d BitRate(100Kbps): %d"
                  "\n\t\tBurst num: %d"
                  "\n\t\tRTT Status: %d"
                  "\n\t\tNumber per burst peer %d"
                  "\n\t\tRetry after duration: %d \n\t\tRTT Type: %d"
                  "\n\t\tTimestamp: %lld \n\t\tActual Burst Duration: %d"
                  "\n\t\tNegotiated Burst Num: %d",
                  rtt_result[i]->tx_rate.preamble,
                  rtt_result[i]->tx_rate.nss,
                  rtt_result[i]->tx_rate.bw,
                  rtt_result[i]->tx_rate.rateMcsIdx,
                  rtt_result[i]->tx_rate.bitrate,
                  rtt_result[i]->rx_rate.preamble,
                  rtt_result[i]->rx_rate.nss,
                  rtt_result[i]->rx_rate.bw,
                  rtt_result[i]->rx_rate.rateMcsIdx,
                  rtt_result[i]->rx_rate.bitrate,
                  rtt_result[i]->burst_num,
                  rtt_result[i]->status,
                  rtt_result[i]->number_per_burst_peer,
                  rtt_result[i]->retry_after_duration,
                  rtt_result[i]->type,
                  rtt_result[i]->ts,
                  rtt_result[i]->burst_duration,
                  rtt_result[i]->negotiated_burst_num);

    }

    ++g_op_recvd;

}

uint32_t update_rtt_bw(int bw)
{
    uint32_t ret = 0;

    switch (bw) {
    case 20:
        ret = LOWI_RTT_BW_20;
        break;
    case 40:
        ret = LOWI_RTT_BW_40;
        break;
    case 80:
        ret = LOWI_RTT_BW_80;
        break;
    case 160:
        ret = LOWI_RTT_BW_160;
        break;
    default:
        ret = 0;
        break;
    }

    return ret;
}

uint32_t update_chwidth(int bw)
{
    switch (bw) {
    case 20:
        return LOWI_CHAN_WIDTH_20;
    case 40:
        return LOWI_CHAN_WIDTH_40;
    case 80:
        return LOWI_CHAN_WIDTH_80;
    case 160:
        return LOWI_CHAN_WIDTH_160;
    }

    return 0;
}

uint32_t update_preamble(std::string preamble)
{
    if (preamble == "legacy")
        return LOWI_RTT_PREAMBLE_LEGACY;
    if (preamble == "HT")
        return LOWI_RTT_PREAMBLE_HT;
    if (preamble == "VHT")
        return LOWI_RTT_PREAMBLE_VHT;
    if (preamble == "HE")
        return LOWI_RTT_PREAMBLE_HE;
    return 0;
}

uint32_t update_peer(std::string peer)
{
    if (peer == "ap")
        return RTT_PEER_AP;
    if (peer == "sta")
        return RTT_PEER_STA;
    if (peer == "go")
        return RTT_PEER_P2P_GO;
    if (peer == "client")
        return RTT_PEER_P2P_CLIENT;
    if (peer == "nan")
        return RTT_PEER_NAN;
    return 0;
}

eTmrType update_eTmrType(uint8_t a)
{
  switch (a)
  {
  case 0:
    return TMR_LEGACY;
  case 1:
    return TMR_NTB;
  case 2:
    return TMR_TB;
  default:
    log_verbose (TAG, "to_eTmrType - default case - TMR_LEGACY");
    return TMR_LEGACY;
  }
}

eSecType update_eSecType(uint8_t a)
{
  switch (a)
  {
  case 0:
    return SEC_NONE;
  case 1:
    return SEC_FTM;
  case 2:
    return SEC_FTM_LTF;
  default:
    log_verbose (TAG, "to_eSecType - default case - SEC_NONE");
    return SEC_NONE;
  }
}

uint32_t update_phymode(std::string mode)
{
    if (mode == "11a")
        return LOWI_PHY_MODE_11A;
    if (mode == "11b")
        return LOWI_PHY_MODE_11B;
    if (mode == "11g")
        return LOWI_PHY_MODE_11G;
    if (mode == "11gonly")
        return LOWI_PHY_MODE_11GONLY;

    if (mode == "11naht20")
        return LOWI_PHY_MODE_11NA_HT20;
    if (mode == "11nght20")
        return LOWI_PHY_MODE_11NG_HT20;
    if (mode == "11naht40")
        return LOWI_PHY_MODE_11NA_HT40;
    if (mode == "11nght40")
        return LOWI_PHY_MODE_11NG_HT40;

    if (mode == "11acvht20")
        return LOWI_PHY_MODE_11AC_VHT20;
    if (mode == "11acvht40")
        return LOWI_PHY_MODE_11AC_VHT40;
    if (mode == "11acvht80")
        return LOWI_PHY_MODE_11AC_VHT80;
    if (mode == "11acvht20_2g")
        return LOWI_PHY_MODE_11AC_VHT20_2G;
    if (mode == "11acvht40_2g")
        return LOWI_PHY_MODE_11AC_VHT40_2G;
    if (mode == "11acvht80_2g")
        return LOWI_PHY_MODE_11AC_VHT80_2G;
    if (mode == "11acvht80_80")
        return LOWI_PHY_MODE_11AC_VHT80_80;
    if (mode == "11acvht160")
        return LOWI_PHY_MODE_11AC_VHT160;

    if (mode == "11axhe20")
        return LOWI_PHY_MODE_11AX_HE20;
    if (mode == "11axhe40")
        return LOWI_PHY_MODE_11AX_HE40;
    if (mode == "11axhe80")
        return LOWI_PHY_MODE_11AX_HE80;
    if (mode == "11axhe80_80")
        return LOWI_PHY_MODE_11AX_HE80_80;
    if (mode == "11axhe160")
        return LOWI_PHY_MODE_11AX_HE160;
    if (mode == "11axhe20_2g")
        return LOWI_PHY_MODE_11AX_HE20_2G;
    if (mode == "11axhe40_2g")
        return LOWI_PHY_MODE_11AX_HE40_2G;
    if (mode == "11axhe80_2g")
        return LOWI_PHY_MODE_11AX_HE80_2G;

    return LOWI_PHY_MODE_UNKNOWN;
}

void print_config_values(u32 num_rtt_config,
                         lowi_rtt_config rtt_config[],
                         uint32_t send_lci_req,
                         uint32_t send_lcr_req,
                         lowi_lci_information *lci,
                         lowi_lcr_information *lcr)
{
   if (!(send_lci_req || send_lcr_req)) {
        for (u32 i = 0; i < num_rtt_config; ++i) {
            log_debug("%s: \t Configured values are:"
                "\n\t\tMacBssid: %02X:%02X:%02X:%02X:%02X:%02X"
                "\n\t\tType: %d"
                "\n\t\tPeer Type: %d"
                "\n\t\tChannel Freq: %d  CFreq0: %d CFreq1: %d Width: %d"
                "\n\t\tBurst Period: %d"
                "\n\t\tNum Burst: %d"
                "\n\t\tNum Frames per burst: %d"
                "\n\t\tNum Retries per RTT Frame: %d"
                "\n\t\tNum Retries per FTMR: %d"
                "\n\t\tAsap Info: %d"
                "\n\t\tForce Legacy ACK: %d"
                "\n\t\tTx Rx Chain Index for Initiator: %d"
                "\n\t\tTx Rx Chain Index for Responder: %d"
                "\n\t\tPattern followed by Initiator: %d"
                "\n\t\tPattern followed by Responder: %d"
                "\n\t\tLCI Request: %d"
                "\n\t\tLCR Request: %d"
                "\n\t\tBurst Duration: %d"
                "\n\t\tPreamble: %d"
                "\n\t\tBandwidth: %d"
                "\n\t\tInterface: %s"
                "\n\t\tPhymode: %d"
                "\n\t\tTMR type: %d"
                "\n\t\tSecurity type: %d"
                "\n\t\ttbPriority: %d"
                "\n\t\ttmrMinDelta: %d"
                "\n\t\ttmrMaxDelta: %d\n"
                "\n\t\tisRttBgNode: %d"
                "\n\t\ttbPeriodicity: %d"
                "\n\t\ttbDuration: %d"
                "\n\t\ttbMaxSessionExpiry: %d"
                "\n\t\tI2RFbPolicy: %d"
                "\n\t\trttChannelSwitchPriority: %d",
                __FUNCTION__,
                rtt_config[i].addr[0],
                rtt_config[i].addr[1],
                rtt_config[i].addr[2],
                rtt_config[i].addr[3],
                rtt_config[i].addr[4],
                rtt_config[i].addr[5],
                rtt_config[i].type,
                rtt_config[i].peer,
                rtt_config[i].channel.center_freq,
                rtt_config[i].channel.center_freq0,
                rtt_config[i].channel.center_freq1,
                rtt_config[i].channel.width,
                rtt_config[i].burst_period,
                rtt_config[i].num_burst,
                rtt_config[i].num_frames_per_burst,
                rtt_config[i].num_retries_per_rtt_frame,
                rtt_config[i].num_retries_per_ftmr,
                rtt_config[i].asap,
                rtt_config[i].force_legacy_ack,
                rtt_config[i].txrxChainIndexInitiator,
                rtt_config[i].txrxChainIndexResponder,
                rtt_config[i].iteratorInitiator,
                rtt_config[i].iteratorResponder,
                rtt_config[i].LCI_request,
                rtt_config[i].LCR_request,
                rtt_config[i].burst_duration,
                rtt_config[i].preamble,
                rtt_config[i].bw,
                rtt_config[i].Interface.c_str(),
                rtt_config[i].phyMode,
                rtt_config[i].tmrtype,
                rtt_config[i].sectype,
                rtt_config[i].tbPriority,
                rtt_config[i].tmrMinDelta,
                rtt_config[i].tmrMaxDelta,
                rtt_config[i].isRttBgNode,
                rtt_config[i].tbPeriodicity,
                rtt_config[i].tbDuration,
                rtt_config[i].tbMaxSessionExpiry,
                rtt_config[i].I2RFbPolicy,
                rtt_config[i].rttChannelSwitchPriority);
        }
    }

   if (send_lci_req) {
        if (lci !=NULL) {
           log_debug("%s: \t CONFIGURED LCI PARAMETERS:"
               "\n\t\tLatitude: %lld \n\t\tLongitude: %lld \n\t\tAltitude: %lld"
               "\n\t\tLatitude UNC: %d \n\t\tLongitude_UNC: %d \n\t\tAltitude_UNC: %d"
               "\n\t\tMotion pattern: %d \n\t\tFloor: %d \n\t\tHeight above Floor: %d"
               "\n\t\tHeight_UNC: %d",
               __FUNCTION__,
               lci->latitude,
               lci->longitude,
               lci->altitude,
               lci->latitude_unc,
               lci->longitude_unc,
               lci->altitude_unc,
               lci->motion_pattern,
               lci->floor,
               lci->height_above_floor,
               lci->height_unc);
       }
   }

   if (send_lcr_req) {
       if (lcr !=NULL) {
           log_debug("%s: \t CONFIGURED LCR PARAMETERS: \n\t\t Country Code : %c%c \n\t\t Civic info:",
               __FUNCTION__,
               (char)lcr->country_code[0],
               (char)lcr->country_code[1]);
          for (uint32_t i = 0; i<lcr->length; i++ )
              printf("%02X",lcr->civic_info[i]);
       }
   }
}

int main(int argc, char * argv[])
{
    unsigned request_id = 1;
    unsigned idx, num_rtt_config = 1;
    uint32_t send_lci_req = 0;
    uint32_t send_lcr_req = 0;
    uint32_t iter = 1;
    uint32_t delay = 3000;
    lowi_rtt_config rtt_config[MAX_RTT_CONFIG_REQ];
    lowi_rtt_capabilities caps;
    int j = 0;
    std::string Interface;
    lowi_lci_information lci_req;
    lowi_lcr_information lcr_req;
    int retval = 0;
    int i;

    for (i = 0; i < argc; i++) {

        /* Send LCI request */
        if (std::string(argv[i]) == "--lci_req") {
            send_lci_req = 1;
            continue;
        }

        /* Send LCR request */
        if (std::string(argv[i]) == "--lcr_req") {
            send_lcr_req = 1;
            continue;
        }

        /* Send LCI request - Latitude value */
        if (std::string(argv[i]) == "--latitude") {
            lci_req.latitude = atoi(argv[++i]);
            continue;
        }

        /* Send LCI request - Longitude value */
        if (std::string(argv[i]) == "--longitude") {
            lci_req.longitude = atoi(argv[++i]);
            continue;
        }

        /* Send LCI request - Altitude value */
        if (std::string(argv[i]) == "--altitude") {
            lci_req.altitude = atoi(argv[++i]);
            continue;
        }

        /* Send LCI request - Latitude unc value */
        if (std::string(argv[i]) == "--latitude_unc") {
            lci_req.latitude_unc = atoi(argv[++i]);
            continue;
        }

        /* Send LCI request - Longitude unc value */
        if (std::string(argv[i]) == "--longitude_unc") {
            lci_req.longitude_unc = atoi(argv[++i]);
            continue;
        }

        /* Send LCI request - Altitude unc value */
        if (std::string(argv[i]) == "--altitude_unc") {
            lci_req.altitude_unc = atoi(argv[++i]);
            continue;
        }

        /* Send LCI request - Motion Pattern */
        if (std::string(argv[i]) == "--motion") {
            lci_req.motion_pattern = (lowi_motion_pattern)atoi(argv[++i]);
            continue;
        }

        /* Send LCI request - Floor value */
        if (std::string(argv[i]) == "--floor") {
            lci_req.floor = atoi(argv[++i]);
            continue;
        }

        /* Send LCI request - Height above floor */
        if (std::string(argv[i]) == "--height_above_floor") {
            lci_req.height_above_floor = atoi(argv[++i]);
            continue;
        }

        /* Send LCI request - Height unc value */
        if (std::string(argv[i]) == "--height_unc") {
            lci_req.height_unc = atoi(argv[++i]);
            continue;
        }

        /* Send LCR request - Country Code */
        if (std::string(argv[i]) == "--country_code") {
            std::string(argv[++i]).copy((char *)lcr_req.country_code,
                                        LOWI_COUNTRY_CODE_LEN, 0);
            continue;
        }

        /* Send LCR request - Civic info */
        if (std::string(argv[i]) == "--civic_info") {
            size_t len;
            std::string civic = std::string(argv[++i]);
            len = civic.length();
            char * str = (char *)civic.c_str();

            if (len > 0) {
                 size_t k;
                 for (k = 0; k < len/2 && k < CIVIC_EXT_INFO_LEN; k++) {
                     if (sscanf(str + 2*k, "%2hhx", &(lcr_req.civic_info[k])) <= 0) {
                          break;
                     }
                 }
                 lcr_req.length = (uint32_t)k;
            } else {
                 log_error("%s: Could not parse Civic info",__FUNCTION__);
                 return -1;
            }
            continue;
        }

        /* Interface handle */
        if (std::string(argv[i]) == "--interface") {
            Interface = std::string(argv[++i]);
            rtt_config[j].Interface = Interface;
            continue;
        }

        /* Send RTT request - MAC address */
        if (std::string(argv[i]) == "--mac") {
            if (sscanf(argv[++i], "%p:%p:%p:%p:%p:%p",
                   (void **)&rtt_config[j].addr[0],
                   (void **)&rtt_config[j].addr[1],
                   (void **)&rtt_config[j].addr[2],
                   (void **)&rtt_config[j].addr[3],
                   (void **)&rtt_config[j].addr[4],
                   (void **)&rtt_config[j].addr[5]) < 6) {
                log_error("%s: Could not parse BSSID",__FUNCTION__);
                return -1;
            }
            continue;
        }

        /* Send RTT request - Bandwidth */
        if (std::string(argv[i]) == "--bw") {
            rtt_config[j].bw = (lowi_rtt_bw)update_rtt_bw(atoi(argv[++i]));
            continue;
        }

        /* Send RTT request - get LCI info */
        if (std::string(argv[i]) == "--lci") {
            rtt_config[j].LCI_request = atoi(argv[++i]);
            continue;
        }

        /* Send RTT request - get LCR info */
        if (std::string(argv[i]) == "--lcr") {
            rtt_config[j].LCR_request = atoi(argv[++i]);
            continue;
        }

        /* Send RTT request - set asap */
        if (std::string(argv[i]) == "--asap") {
            rtt_config[j].asap = atoi(argv[++i]);
            continue;
        }

        /* Send RTT request - set force legacy ack */
        if (std::string(argv[i]) == "--forceLegAck") {
            rtt_config[j].force_legacy_ack = atoi(argv[++i]);
            continue;
        }

        /* Send RTT request - Set Chain Index for Initiator */
        if(std:: string(argv[i]) == "--txrxChainIndexInitiator") {
           rtt_config[j].txrxChainIndexInitiator = atoi(argv[++i]);
           continue;
        }

        /* Send RTT request - Set Chain Index for Responder */
        if(std:: string(argv[i]) == "--txrxChainIndexResponder") {
           rtt_config[j].txrxChainIndexResponder = atoi(argv[++i]);
           continue;
        }

        /* Send RTT request - Set pattern for Initiator */
        if(std:: string(argv[i]) == "--iteratorInitiator") {
           rtt_config[j].iteratorInitiator = atoi(argv[++i]);
           continue;
        }

        /* Send RTT request - Set pattern for Responder */
        if(std:: string(argv[i]) == "--iteratorResponder") {
           rtt_config[j].iteratorResponder = atoi(argv[++i]);
           continue;
        }

        /* Send RTT request - set preamble */
        if (std::string(argv[i]) == "--preamble") {
            rtt_config[j].preamble = (lowi_rtt_preamble)update_preamble(std::string(argv[++i]));
            continue;
        }

        /* Send RTT request - set rtt type */
        if (std::string(argv[i]) == "--type") {
            rtt_config[j].type = (lowi_rtt_type)(atoi(argv[++i])-1);
            continue;
        }

        /* Send RTT request - set peer type */
        if (std::string(argv[i]) == "--peer") {
            rtt_config[j].peer = (rtt_peer_type)update_peer(std::string(argv[++i]));
            continue;
        }

        /* Send RTT request - Frequency value */
        if (std::string(argv[i]) == "--freq") {
            rtt_config[j].channel.center_freq = atoi(argv[++i]);
            continue;
        }

        /* Send RTT request - center Frequency value0 */
        if (std::string(argv[i]) == "--cfreq0") {
            rtt_config[j].channel.center_freq0 = atoi(argv[++i]);
            continue;
        }

        /* Send RTT request - center Frequency value1 */
        if (std::string(argv[i]) == "--cfreq1") {
            rtt_config[j].channel.center_freq1 = atoi(argv[++i]);
            continue;
        }

        /* Send RTT request - Channel width */
        if (std::string(argv[i]) == "--chwidth") {
            rtt_config[j].channel.width = (lowi_channel_width)update_chwidth(atoi(argv[++i]));
            continue;
        }

        /* Send RTT request - Burst duration */
        if (std::string(argv[i]) == "--burst_duration") {
            rtt_config[j].burst_duration = atoi(argv[++i]);
            continue;
        }

        /* Send RTT request - No frames per request */
        if (std::string(argv[i]) == "--numframes") {
            rtt_config[j].num_frames_per_burst = atoi(argv[++i]);
            continue;
        }

        /* Send RTT request - Phymode */
        if (std::string(argv[i]) == "--phymode") {
            rtt_config[j].phyMode = (LOWIPhyMode)update_phymode(argv[++i]);
            continue;
        }

        /* Send RTT request - TMR type for 11az Ranging scan request */
        if (std::string(argv[i]) == "--tmrtype") {
            rtt_config[j].tmrtype = update_eTmrType(atoi(argv[++i]));
            continue;
        }

        /* Send RTT request - Security type for 11az Ranging scan request */
        if (std::string(argv[i]) == "--sectype") {
            rtt_config[j].sectype = update_eSecType(atoi(argv[++i]));
            continue;
        }

        /* Send RTT request - TB priority */
        if (std::string(argv[i]) == "--tbPriority") {
            rtt_config[j].tbPriority = atoi(argv[++i]);
            continue;
        }

        /* Send RTT request - Min time between measurements */
        if (std::string(argv[i]) == "--tmrMinDelta") {
            rtt_config[j].tmrMinDelta = atoi(argv[++i]);
            continue;
        }

        /* Send RTT request - Max time between measurements */
        if (std::string(argv[i]) == "--tmrMaxDelta") {
            rtt_config[j].tmrMaxDelta = atoi(argv[++i]);
            continue;
        }

        /* Send RTT request - Set RTT BG Node */
        if(std:: string(argv[i]) == "--isRttBgNode") {
           rtt_config[j].isRttBgNode = atoi(argv[++i]);
           continue;
        }

        /* Send RTT request - Set TB Periodicity */
        if(std:: string(argv[i]) == "--tbPeriodicity") {
           rtt_config[j].tbPeriodicity = atoi(argv[++i]);
           continue;
        }

        /* Send RTT request - Set TB Deration */
        if(std:: string(argv[i]) == "--tbDuration") {
           rtt_config[j].tbDuration = atoi(argv[++i]);
           continue;
        }

        /* Send RTT request - Set TB Max Expiry */
        if(std:: string(argv[i]) == "--tbMaxSessionExpiry") {
           rtt_config[j].tbMaxSessionExpiry = atoi(argv[++i]);
           continue;
        }

        /* Send RTT request - Set I2R Feedback Policy */
        if(std:: string(argv[i]) == "--I2RFbPolicy") {
           rtt_config[j].I2RFbPolicy = atoi(argv[++i]);
           continue;
        }

        /* Send RTT request - Set RTT Channel Switch Priority */
        if(std:: string(argv[i]) == "--rttChannelSwitchPriority") {
           rtt_config[j].rttChannelSwitchPriority = atoi(argv[++i]);
           continue;
        }

        if (std::string(argv[i]) == "--print_11az_params") {
            g_print_11az_params = (bool)atoi(argv[++i]);
            continue;
        }

        /* Set Application log level  */
        if (std::string(argv[i]) == "--log") {
            g_log_level = (ERROR_LEVEL)atoi(argv[++i]);
            continue;
        }

        /* Send multiple times RTT request  */
        if (std::string(argv[i]) == "--n") {
            iter = atoi(argv[++i]);
            if (iter < 1 || iter > 30000) {
                iter = 1;
                log_error("%s: Number of scans is not in limit (1,30000), Hence proceeding with one",
                         __FUNCTION__);
            }
            continue;
        }

        /* Wait time before sending each request in multiple RTT requests  */
        if (std::string(argv[i]) == "--d") {
            delay = atoi(argv[++i]);
            if (delay < 10 || delay > 30000) {
                delay = 3000;
                log_error("%s: Wait time before sending another RTT req is not in limit (10,30000)ms, Hence proceeding with default value %dms",
                          __FUNCTION__, delay);
            }
            continue;
        }

        if (std::string(argv[i]) == "lowi-lib-test")
           continue;

        /* Send multiple vaps per request  */
        if (std::string(argv[i]) == "--next") {
            ++j;
            if (j >= MAX_RTT_CONFIG_REQ) {
                log_error("%s: Exceeds max request, Hence proceeding with already configured %d requests",__FUNCTION__, j);
                break;
            }
           continue;
        }

        log_error("%s Invalid Option %s",__FUNCTION__,std::string(argv[i]).c_str());
    }

    num_rtt_config = j+1;

    if (num_rtt_config > MAX_RTT_CONFIG_REQ)
         num_rtt_config = MAX_RTT_CONFIG_REQ;

    print_config_values(num_rtt_config, rtt_config, send_lci_req, send_lcr_req,
                        &lci_req, &lcr_req);

    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGILL, signal_handler);
    signal(SIGTRAP, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGIOT, signal_handler);
    signal(SIGBUS, signal_handler);
    signal(SIGFPE, signal_handler);
    signal(SIGPIPE, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGKILL, signal_handler);
    signal(SIGSTOP, signal_handler);
    signal(SIGTSTP, signal_handler);
    signal(SIGALRM, signal_handler);
    signal(NSIG, signal_handler);

    op.on_rtt_results = &rtt_results;

    if (!op.on_rtt_results)
        return 0;

    lowi_cb_table_t * func;
    func = lowi_get_cb_table();
    if (!func) {
        log_error("%s: Library API functions are null", __FUNCTION__);
        return 0;
    }

    func->init();

    if (send_lci_req) {
       retval = func->rtt_set_lci( request_id, &lci_req, Interface);
       if (retval)
           log_error("%s: Send LCI request failed ", __FUNCTION__);
    }

    if (send_lcr_req) {
        retval = func->rtt_set_lcr( ++request_id, &lcr_req, Interface);
       if (retval)
           log_error("%s: Send LCR request failed ", __FUNCTION__);
    }

    if (!(send_lci_req || send_lcr_req)) {
        for (uint32_t k = 0; k < iter; k++) {
            retval = func->get_rtt_capabilities( &caps, Interface);
            if (retval) {
                for (idx = 0; idx < num_rtt_config; idx++) {
                    log_error("%s: Get RTT Capabilities failed for MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                              __FUNCTION__, rtt_config[idx].addr[0], rtt_config[idx].addr[1],
                              rtt_config[idx].addr[2], rtt_config[idx].addr[3],
                              rtt_config[idx].addr[4], rtt_config[idx].addr[5]);
               }

               func->destroy();
               return 0;
            }

            retval = func->rtt_range_request(++request_id, num_rtt_config, rtt_config, op);
            if (retval) {
                for (idx = 0; idx < num_rtt_config; idx++) {
                    log_error("%s: Send RTT request failed for MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                            __FUNCTION__, rtt_config[idx].addr[0], rtt_config[idx].addr[1],
                            rtt_config[idx].addr[2], rtt_config[idx].addr[3],
                            rtt_config[idx].addr[4], rtt_config[idx].addr[5]);
                }

                func->destroy();
                return 0;
            }

            lowi_test_start_timer(delay);
            lowi_test_wait(delay);
        }
    }

    if (send_lci_req || send_lcr_req) {
        lowi_test_start_timer(delay);
        lowi_test_wait(delay);
    } else {
        while (g_op_recvd < iter) {
            log_debug("%s: Waiting till.. %d Counts passed %d", __func__, iter, g_op_recvd);
            lowi_test_start_timer(delay);
            lowi_test_wait(delay);
        }
    }

    func->destroy();
    return 0;

}
