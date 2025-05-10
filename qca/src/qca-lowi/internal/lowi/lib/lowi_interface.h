/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

        LOWI API

GENERAL DESCRIPTION
  This file contains the class definition for the LOWIClientListener
  used in the LOWI interface for RTT and BGSCAN measurements

Copyright (c) 2014-2016, 2021 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
=============================================================================*/
#ifndef __LOWI_INTERFACE_H__
#define __LOWI_INTERFACE_H__

#include "lowi_version.h"
#include "inc/lowi_external.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct lowi_capabilities_s {
    int bgscan_supported;
} lowi_capabilities_t;

/*=============================================================================================
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
 *   0: initialization success
 *   else: initializatin failure
 =============================================================================================*/
int lowi_init();

/*=============================================================================================
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
 *   else: failed to lowi_destroy the LOWI interface
 =============================================================================================*/
int lowi_destroy();

/*=============================================================================================
 * lowi_get_lowi_capabilities
 *
 * Description:
 *   This function gets capabilities that are related to the LOWI interface that
 *   are needed to support the GSCAN and RTT APIs. This is a blocking call. No further
 *   action should take place on the part of the client until the LOWI supported
 *   capabilities are returned successfully.
 *   NOTE: to be called only init() has returned successfully
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
int lowi_get_lowi_capabilities(u32* caps, std::string interface);

/*=============================================================================================
 * lowi_rtt_range_request
 *
 * Description:
 *   This function requests RTT measurements.
 *
 * Parameters:
 *    request_id: unique request identification number
 *    iface: wifi interface handle (not needed, not used, kept for consistency)
 *    num_rtt_config: number of destination BSSIDs to measure RTT on
 *    rtt_config: array of BSSIDs to measure RTT on
 *    handler: callback used to return the results
 *
 * Return value:
 *   0: call was successfully made by lowi client, this says nothing about whether the
 *      range request itself is successful
 *   else: lowi client failed to make the call
 =============================================================================================*/
int lowi_rtt_range_request(uint32_t request_id,
                                   u32 num_rtt_config,
                                   lowi_rtt_config rtt_config[],
                                   lowi_rtt_event_handler handler);


/*=============================================================================================
 * lowi_rtt_range_cancel
 *
 * Description:
 *   This function cancels RTT measurement request on destination bssids that have been
 *   configured for periodic measurements
 *
 * Parameters:
 *    request_id: unique request identification number
 *    num_devices: number of devices to do in addr[]
 *    addr: number of destination BSSIDs to stop ranging on
 *
 * Return value:
 *   0: request sent successfully
 *   else: failed to send request
 =============================================================================================*/
int lowi_rtt_range_cancel(u32 request_id,
                                  u32 num_devices,
                                  mac_addr addr[]);

/*=============================================================================================
 * lowi_get_rtt_capabilities
 *
 * Description:
 *   This function requests RTT capabilities of the device.
 *   NOTE: to be called only init() has returned successfully
 *
 * Parameters:
 *  iface: wifi interface handle (not needed, not used, kept for consistency)
 *  capabilities: ptr to rtt capabilities struct
 *
 * Return value:
 *   0: call was successfully sent by lowi client, this says nothing about whether the
 *      capabilities were retrieved successfully
 *   else: lowi client failed to send the request for capabilities
 =============================================================================================*/
int lowi_get_rtt_capabilities(lowi_rtt_capabilities *capabilities, std::string interface);

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
int lowi_get_lowi_version(u16 *major, u16 *minor, u16 *micro);

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
lowi_status lowi_set_lci(lowi_request_id id, lowi_lci_information *lci, std::string interface);

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
lowi_status lowi_set_lcr(lowi_request_id id, lowi_lcr_information *lcr, std::string interface);

#ifdef __cplusplus
}
#endif

#endif //__LOWI_INTERFACE_H__
