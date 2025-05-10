/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

        Wifi Driver Utilities

GENERAL DESCRIPTION
  This file contains the implementation of utilities for Wifi Driver

Copyright (c) 2012-2013, 2015-2018, 2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

(c) 2012-2013 Qualcomm Atheros, Inc.
  All Rights Reserved.
  Qualcomm Atheros Confidential and Proprietary.

=============================================================================*/

#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include "lowi_utils_defines.h"
#include <base_util/string_routines.h>
#include <common/lowi_utils.h>
#include "innavService.h"
#include "lowi_wifidriver_utils.h"
#include "wifiscanner.h"

#define RX_BUFF_SIZE  1024
#define MAX_CAPABILITY_REQ_RETRY_COUNT 3
#define CAPABILITY_RETRY_WAIT_TIME_IN_SEC 1

using namespace qc_loc_fw;

// initialization of static variables
const char * const LOWIWifiDriverUtils::TAG                = "LOWIWifiDriverUtils";
char LOWIWifiDriverUtils::wlan_ifname[PROPERTY_VALUE_MAX]  = "";
char LOWIWifiDriverUtils::wigig_ifname[PROPERTY_VALUE_MAX] = "";
int LOWIWifiDriverUtils::ioctlSock = 0;
uint32 LOWIWifiDriverUtils::band_mask = (uint32)LOWIDiscoveryScanRequest::INVALID_MASK;

int* LOWIWifiDriverUtils::getSupportedFreqs (uint32 band_mask,
    s_ch_info* p_ch_info,
    unsigned char & num_channels, struct contConfig config)
{

  log_debug (TAG, "%s: num of 2g_ch=(%u) 5g_ch=(%u) 6g_ch=(%u) band_mask(%u) user_6ghz_cfgs %d\n",
                __FUNCTION__, p_ch_info->num_2g_ch,
                p_ch_info->num_5g_ch, p_ch_info->num_6g_ch,
                band_mask, config.user_6ghz_cfgs);

  if (0 == p_ch_info->num_2g_ch && 0 == p_ch_info->num_5g_ch &&
      0 == p_ch_info->num_6g_ch)
  {
    log_debug (TAG, "getSupportedFreqs - supported channel list not found"
        " using the default");
    return LOWIUtils::getDefaultFreqs(band_mask, num_channels,
             (LOWIDiscoveryScanRequest::e6GChannelType) config.user_6ghz_cfgs);
  }
  int * freqs = NULL;
  unsigned char cnt_2g_ch = 0, cnt_5g_ch = 0, cnt_6g_ch = 0;
  int *freq_2g = NULL, *freq_5g = NULL, *freq_6g = NULL,  *freq_6g_temp = NULL;

  /** Let us fill the 2G band if supported by kernel */
  if (band_mask & LOWIDiscoveryScanRequest::BAND_2P4_GHZ)
  {
    cnt_2g_ch = p_ch_info->num_2g_ch;
    freq_2g = new (std::nothrow) int [cnt_2g_ch];
    if (NULL != freq_2g)
    {
      for (unsigned char ii = 0; ii < cnt_2g_ch; ++ii)
      {
        freq_2g [ii] = p_ch_info->arr_2g_ch [ii];
      }
    }
  }

  /** Let us fill the 5G band if supported by kernel */
  if (band_mask & LOWIDiscoveryScanRequest::BAND_5_GHZ)
  {
    cnt_5g_ch = p_ch_info->num_5g_ch;
    freq_5g = new (std::nothrow) int [cnt_5g_ch];
    if (NULL != freq_5g)
    {
      for (unsigned char ii = 0; ii < cnt_5g_ch; ++ii)
      {
        freq_5g [ii] = p_ch_info->arr_5g_ch [ii];
      }
    }
  }

  /** Let us fill the 6G band if supported by kernel and
      configured by user too */
  if (config.user_6ghz_cfgs &&
      (band_mask & LOWIDiscoveryScanRequest::BAND_6_GHZ))
  {
    freq_6g_temp = new (std::nothrow) int [p_ch_info->num_6g_ch];
    if (!freq_6g_temp)
    {
      log_error (TAG, "Mem alloc failed for num_6g_ch %u", p_ch_info->num_6g_ch);
      goto out;
    }
    if (config.user_6ghz_cfgs == LOWIDiscoveryScanRequest::USE_ALL_6G_CHANNEL)
    {
      cnt_6g_ch = p_ch_info->num_6g_ch;
      for (unsigned char ii = 0; ii < p_ch_info->num_6g_ch; ii++)
        freq_6g_temp [ii] =  p_ch_info->arr_6g_ch[ii];
    }
    else
    {
      cnt_6g_ch = 0;
      for (unsigned char ii = 0; ii < p_ch_info->num_6g_ch; ii++)
      {
        if (LOWIUtils::valid6gPSCFreq(p_ch_info->arr_6g_ch[ii]))
          freq_6g_temp [cnt_6g_ch++] = p_ch_info->arr_6g_ch[ii];
      }
    }
    freq_6g = freq_6g_temp;
  }

out:
  num_channels = cnt_2g_ch + cnt_5g_ch + cnt_6g_ch;
  if (num_channels)
  {
    freqs = new (std::nothrow) int [num_channels];
    if (NULL != freqs && NULL != freq_2g && NULL != freq_5g && NULL != freq_6g)
    {
      unsigned int total_index = 0;
      for (unsigned int index_2g = 0; index_2g < cnt_2g_ch; ++index_2g, ++total_index)
        freqs[total_index] = freq_2g[index_2g];
      for (unsigned int index_5g = 0; index_5g < cnt_5g_ch; ++index_5g, ++total_index)
        freqs[total_index] = freq_5g[index_5g];
      for (unsigned int index_6g = 0; index_6g < cnt_6g_ch; ++index_6g, ++total_index)
        freqs[total_index] = freq_6g[index_6g];
    }
  }

  if (freq_2g) delete []freq_2g;
  if (freq_5g) delete []freq_5g;
  if (freq_6g) delete []freq_6g;

  return freqs;
}

char * LOWIWifiDriverUtils::get_interface_name()
{
  if (0 == wlan_ifname[0])
  {
#ifdef __ANDROID__
    if (property_get("wifi.interface", wlan_ifname, DEFAULT_WLAN_INTERFACE) != 0)
    {
      log_debug("LOWIWpaInterface", "Using interface '%s'\n", wlan_ifname);
      return wlan_ifname;
    }
#else
    strlcpy(wlan_ifname, DEFAULT_WLAN_INTERFACE, sizeof(wlan_ifname));
#endif
  }
  return wlan_ifname;
}

char* LOWIWifiDriverUtils::get_wigig_interface_name()
{
  if (0 == wigig_ifname[0])
  {
    strlcpy(wigig_ifname, DEFAULT_WIGIG_INTERFACE, sizeof(wigig_ifname));
  }
  return wigig_ifname;
}

eWifiIntfState LOWIWifiDriverUtils::get_intfc_state(char *intfName)
{

  eWifiIntfState state = INTF_UNKNOWN;
  struct ifreq req;

    if (ioctlSock <= 0) /* Open IOCTL socket if socket not already open or failed to open the last time */
    {
      ioctlSock = socket(AF_INET, SOCK_DGRAM, 0);
      if (ioctlSock < 0)
      {
        log_error(TAG, "%s - Failed to open IOCTL socket", __FUNCTION__);
        return INTF_UNKNOWN;
      }
    }

    memset(&req, 0, sizeof(req));
    strlcpy(req.ifr_name, intfName, IFNAMSIZ);
    errno = 0;
    int ret = ioctl(ioctlSock, SIOCGIFFLAGS, &req);
    if (ret < 0)
    {
      log_debug(TAG, "%s - %s ioctl(GET_IFFLAGS) returned: %d, Errno: %s(%d)",
                __FUNCTION__, intfName, ret, strerror(errno), errno);
    }
    else
    {
    log_debug(TAG, "%s: ifname %s, flags 0x%x (%s%s)", __FUNCTION__,
              req.ifr_name, req.ifr_flags,
              (req.ifr_flags & IFF_UP) ? "[UP]" : "",
              (req.ifr_flags & IFF_RUNNING) ? "[RUNNING]" : "" );
    state = (req.ifr_flags & IFF_RUNNING ? INTF_RUNNING :
             (req.ifr_flags & IFF_UP     ? INTF_UP      : INTF_DOWN));
     if ((state == INTF_UP) || (state == INTF_RUNNING))
     {
       return state;
     }
    }

   return INTF_UNKNOWN;
}

eWifiIntfState LOWIWifiDriverUtils::getInterfaceState(char const *intfcName)
{
  eWifiIntfState state = INTF_UNKNOWN;
  ConfigFile *mConfig = NULL;
  const char *config_iface;
  char intfName[128];
  mConfig = ConfigFile::createInstance(CONFIG_NAME);

  if(NULL != mConfig && mConfig->loaded() &&
      (0 == mConfig->getString("LOWI_USE_INTERFACE", &config_iface))) {
       strlcpy(intfName, config_iface, IFNAMSIZ);
       state = get_intfc_state(intfName);
  } else {
#ifdef LOWI_ON_ACCESS_POINT
       const char* ifr_num[] = { "0", "1", "2", "3", "4" };
#else
       const char* ifr_num[] = { "" };
#endif
       uint32 ifr_count = 0;
       do {
           strlcpy(intfName, intfcName, IFNAMSIZ);
           LOWI_BREAK_ON_COND((NULL==intfName), debug, "NULL pointer received")
           strlcat(intfName, ifr_num[ifr_count], IFNAMSIZ);
           state = get_intfc_state(intfName);
           if (state != INTF_UNKNOWN)
               break;
       } while ((++ifr_count) < LOWI_ARR_SIZE(ifr_num));
  }

  if (mConfig)
      delete mConfig;

  return state;
}

/** Driver send 0 for all band, 1 for 2P4 only  and 2 for 5G only*/
void LOWIWifiDriverUtils::set_band_mask(s_ch_info *ch_info)
{
  uint32 mask = 0;
  if (ch_info->num_2g_ch)
    mask |= LOWIDiscoveryScanRequest::BAND_2P4_GHZ;
  if (ch_info->num_5g_ch)
    mask |= LOWIDiscoveryScanRequest::BAND_5_GHZ;
  if (ch_info->num_6g_ch)
    mask |= LOWIDiscoveryScanRequest::BAND_6_GHZ;
  band_mask = mask;

  log_info(TAG, "set the band mask from wiphy %u", band_mask);
}

uint32 LOWIWifiDriverUtils::get_band_mask()
{
  log_info(TAG, "return band_mask based on wiphy %u", band_mask);
  return band_mask;
}
