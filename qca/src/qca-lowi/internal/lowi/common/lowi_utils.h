#ifndef __LOWI_UTILS_H__
#define __LOWI_UTILS_H__

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

        LOWI Utils Interface Header file

GENERAL DESCRIPTION
  This file contains the structure definitions and function prototypes for
  LOWI Utils

  Copyright (c) 2012-2013,2015-2019, 2021-2022 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

  (c) 2012-2013 Qualcomm Atheros, Inc.
  All Rights Reserved.
  Qualcomm Atheros Confidential and Proprietary.
=============================================================================*/

#include <base_util/postcard.h>
#include <inc/lowi_const.h>
#include <inc/lowi_request.h>
#include <inc/lowi_response.h>
#include <inc/lowi_scan_measurement.h>

// Size of an array of objects
#define LOWI_ARR_SIZE(arr) sizeof(arr)/sizeof((arr)[0])
// Check val is less than size of array, before accessing the array
#define LOWI_TO_STRING( val, arr ) LOWIUtils::to_string(val, arr, LOWI_ARR_SIZE(arr))
// Convert literal Constant define/Enum name to string
#define CONST2STR(x) case x: return #x
// Convert literal Constant define/Enum name to string
#define TAGCONST2STR(tag, x) case tag::x: return #x
// Break if condition is true and use a string to log the reason
#define LOWI_BREAK_ON_COND(cond, level, fmt, ...) if ((cond))         \
        {                                                             \
          log_##level (TAG, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__); \
          break;                                                      \
        }

// Return a given value if condition is true, print log before returning
#define LOWI_RETURN_ON_COND(cond, retVal, level, fmt, ...) if ((cond)) \
        {                                                              \
          log_##level (TAG, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__);  \
          return retVal;                                               \
        }

/* Channel spacing defines */
#define CHANNEL_SPACING_10MHZ 10
#define CHANNEL_SPACING_30MHZ 30
#define CHANNEL_SPACING_80MHZ 80
#define CHANNEL_SPACING_40MHZ 40
#define CHANNEL_SPACING_50MHZ 50
#define CHANNEL_SPACING_70MHZ 70

/* Macros to check for valid channel spacing */
#define IS_VALID_20MHZ_CHAN_SPACING(prime,second) \
        ((0 != prime) && ((second == prime) || (second == 0)))

#define IS_VALID_40MHZ_2GHZ_CHAN_SPACING(prime,second,max2GHzFreq) \
        (second == LOWIUtils::get2GHz40Pluscfreq(prime, max2GHzFreq)) || \
        (second == LOWIUtils::get2GHz40Minuscfreq(prime, max2GHzFreq))

#define IS_VALID_40MHZ_CHAN_SPACING(prime,second, max2GHzFreq, maxFreq)     \
        (((LOWIUtils::freqToBand(prime) == LOWIDiscoveryScanRequest::FIVE_GHZ) || \
         (LOWIUtils::freqToBand(prime) == LOWIDiscoveryScanRequest::SIX_GHZ)) ? \
         (second == LOWIUtils::get40MHzcfreq(prime, maxFreq)) : \
         IS_VALID_40MHZ_2GHZ_CHAN_SPACING(prime,second,max2GHzFreq))

#define IS_VALID_80MHZ_CHAN_SPACING(prime, second, maxFreq)     \
        (second == LOWIUtils::get80MHzcfreq(prime, maxFreq))

#define IS_VALID_160MHZ_CHAN_SPACING(prime,second, third, maxFreq) \
        ((IS_VALID_80MHZ_CHAN_SPACING(prime, second, maxFreq) && \
         (third == LOWIUtils::get160MHzcfreq(prime, maxFreq))) || \
         (second == LOWIUtils::get160MHzcfreq(prime, maxFreq)))

#define IS_VALID_80P80MHZ_CHAN_SPACING(prime,second, third, maxFreq) \
        (IS_VALID_80MHZ_CHAN_SPACING(prime, second, maxFreq) && \
         ((third  > (second + CHANNEL_SPACING_80MHZ)) || \
          (third  < (second - CHANNEL_SPACING_80MHZ))))

namespace qc_loc_fw
{

// Forward declaration
class LOWIUtilsExtn;

/**
 * Utility Class
 */
class LOWIUtils
{
private:
  // Making LOWIUtildExtn class friend class to be able to access private Utils functions
  friend class LOWIUtilsExtn;

  /**
   * Parses the Ranging Request Info from a postcard
   *
   * @param req_id: ranging scan request identifier
   * @param timeoutTimestamp: time out for the request
   * @param rttReportType: report type used for the response
   * @param num_of_nodes: Number of nodes in Ranging Request
   * @param card: postcard containing the ranging scan parameters
   *
   * @return None
   */
  static void parseRangReqInfo(uint32 &req_id,
                               int64 &timeoutTimestamp,
                               uint8 &rttReportType,
                               uint32 &num_of_nodes,
                               InPostcard &card);

  /**
   * Parses the LOWINodeInfo parameters from a postcard
   *
   * @param info: LOWINodeInfo object to store parameters
   * @param inner: postcard containing the LOWI Node Info
   *             Parameters
   *
   * @return None
   */
  static void parseLOWINodeInfo(LOWINodeInfo &info,
                                InPostcard *inner);

  /**
   * Extracts the mac address from the postcard
   *
   * @param InPostcard&: input postcard
   * @param LOWIMacAddress&: Mac Address extracted from post card
   *
   * @return bool success, false otherwise
   */
  static bool extractBssid(InPostcard &inner, LOWIMacAddress& bssid);

  /**
   * Extracts the mac addresses from the postcard
   *
   * @param InPostcard&: input postcard
   * @param vector<LOWIMacAddress>&: Mac Addresses extracted from post card
   *
   * @return bool success, false otherwise
   */
  static bool extractBssids(InPostcard &inner,
                            vector<LOWIMacAddress>& bssids);

  /**
   * Extracts the CFRCIR from the postcard
   *
   * @param InPostcard&: input postcard
   * @param uint8*: cfrcir extracted from post card
   *
   * @return bool success, false otherwise
   */

  static bool extractCFRCIR(InPostcard &inner, uint8 *cfrcir);
  /**
   * Extracts the SSID from the postcard
   *
   * @param InPostcard&: input postcard
   * @param LOWISsid&: SSID extracted from post card
   *
   * @return bool success, false otherwise
   */
  static bool extractSsid(InPostcard &inner, LOWISsid& ssid);

  /**
   * Extracts the SSIDs from the postcard
   *
   * @param InPostcard&: input postcard
   * @param vector<LOWISsid>&: SSIDs extracted from post card
   *
   * @return bool success, false otherwise
   */
  static bool extractSsids(InPostcard &inner,
                           vector<LOWISsid>& ssids);

  /**
   * parse the ranging scan parameters passed in the card which will be
   * used to populate the ranging scan request
   *
   * @param req_id: ranging scan request identifier
   * @param card: postcard containing the ranging scan parameters
   * @param request: LOWI Request Pointer
   * @param periodic: Flag indicating Periodic or non Periodic
   *                Request
   *
   * @return bool: true if success, else: false
   */
  static bool parseRangScanParams(uint32 &req_id,
                                  InPostcard &card,
                                  LOWIRequest *&request,
                                  bool periodic);

  /**
   * parse the discovery scan parameters passed in the card and use them to
   * populate the discovery scan request
   *
   * @param req_id: discovery scan request identifier
   * @param card: postcard containing the discovery scan parameters
   * @param dis: pointer to discovery scan request to populate
   *
   * @return bool: true if success, else: false
   */
  static bool parseDiscScanParams(uint32 &req_id,
                                  InPostcard &card,
                                  LOWIDiscoveryScanRequest *dis);
  /**
   * The following set of functions extract some value from the postcard
   * passed in the argument. The 4th arguments determines the type of the
   * value to be extracted.
   *
   * @param card: input postcard containing the value to be extracted
   * @param n: string with the name of the value to be extracted
   * @param s: string used for debug purposes
   * @param num: value extracted to be placed here
   */
  static void extractUInt8(InPostcard &card, const char* n, const char* s, uint8 &num);
  static void extractUInt16(InPostcard &card, const char* n, const char* s, uint16 &num);
  static void extractUInt32(InPostcard &card, const char* n, const char* s, uint32 &num);
  static void extractUInt64(InPostcard &card, const char* n, const char* s, uint64 &num);
  static void extractInt8(InPostcard &card, const char* n, const char* s, int8 &num);
  static void extractInt16(InPostcard &card, const char* n, const char* s, int16 &num);
  static void extractInt32(InPostcard &card, const char* n, const char* s, int32 &num);
  static void extractInt64(InPostcard &card, const char* n, const char* s, int64 &num);
  static void extractBool(InPostcard &card, const char* n, const char* s, bool &num);
  static void extractDouble(InPostcard &inner, const char *n, const char *s, double &num);

  /**
   * Parses the LCI parameters coming on the postcard and generates the inputs
   * to create a SET_LCI_INFORMATION request
   *
   * @param card: input postcard
   * @param params: LCI parameters
   * @param req_id: LOWIRequest request id
   */
  static void extractLciInfo(InPostcard *const card,
                             LOWILciInformation &params,
                             uint32 &req_id);

  /**
   * Parses the LCR parameters coming on the postcard and generates the inputs
   * to create a SET_LCR_INFORMATION request
   *
   * @param card: input postcard
   * @param params: LCR parameters
   * @param req_id: LOWIRequest request id
   */
  static void extractLcrInfo(InPostcard *const card,
                             LOWILcrInformation &params,
                             uint32 &req_id);

    /**
    * Parses the FTMRR parameters coming on the postcard and
    * generates the inputs to create a FTM ranging request
    *
    * @param card: input postcard
    * @param params: FTMRR parameters
    * @param params: randomization interval
    * @param req_id: LOWIRequest request id
    */
    static void extractFTMRRInfo(InPostcard *const card,
                                 vector<LOWIFTMRRNodeInfo> &params,
                                 LOWIMacAddress &bssid,
                                 uint16 &interval);

  /**
   * Adds the mac address to the postcard
   * @param OutPostcard&: postcard
   * @param LOWIMacAddress&: mac address to be added to the card
   */
  static void addBssidToCard(OutPostcard& card, const LOWIMacAddress& bssid);

  /**
   * Adds the mac addresses to the postcard
   * @param OutPostcard&: postcard
   * @param vector<LOWIMacAddress>&: mac addresses to be added to the card
   * @return bool true for success, false otherwise
   */
  static bool addBssidsToCard(OutPostcard& card, const vector<LOWIMacAddress>& bssids);
  /**
   * Adds the CFRCIR to the postcard
   * @param OutPostcard&: postcard
   * @param uint8*: cfrcir to be added to the card
   */
  static void addCFRCIRToCard(OutPostcard &card, uint8 *cfrcir, uint32 len);

  /**
   * Adds the SSID to the postcard
   * @param OutPostcard&: postcard
   * @param LOWISsid&: SSID to be added to the card
   */
  static void addSsidToCard(OutPostcard& card, const LOWISsid& ssid);

  /**
   * Adds the SSIDs to the postcard
   * @param OutPostcard&: postcard
   * @param vector<LOWISsid>&: SSIDs to be added to the card
   * @return bool true for success, false otherwise
   */
  static bool addSsidsToCard(OutPostcard& card, const vector<LOWISsid>& ssids);

  /**
   * Adds the FTMRR node to the postcard
   * @param card: input postcard
   * @param node: FTMRR node to be added to the card
   *
   * @return bool: true if success, else: false
   */
  static bool addFTMRRNodeToCard(OutPostcard &card, const LOWIFTMRRNodeInfo &node);

public:
    static uint32 convertBandToBandMask(LOWIDiscoveryScanRequest::eBand band);
    static LOWIDiscoveryScanRequest::eBand convertBandMaskToBand(uint32 band_mask);
    static uint32 convertBandBitToBandMask(LOWIDiscoveryScanRequest::eBandBit bandBit);
    inline static bool isChannelValid(uint32 channel,
                                      LOWIDiscoveryScanRequest::eBandBit bandBit)
    {
      return  (((bandBit == LOWIDiscoveryScanRequest::BAND_2P4G_BIT) &&
                (channel >= BAND_2G_CHAN_BEGIN) && (channel <= BAND_2G_CHAN_END)) ||
               ((bandBit == LOWIDiscoveryScanRequest::BAND_5G_BIT) &&
                (((channel >= BAND_5G_CHAN_BEGIN) && (channel < BAND_5G_INVALID_CHAN_START)) ||
                 ((channel > BAND_5G_INVALID_CHAN_END) && (channel <= BAND_5G_CHAN_END)))) ||
                  ((bandBit == LOWIDiscoveryScanRequest::BAND_6G_BIT) &&
                   ((channel >= BAND_6G_CHAN_BEGIN) && (channel <= BAND_6G_CHAN_END))));
    }

    // API to check whether input freq belongs to 6g psc channels or not
    static bool valid6gPSCFreq(uint32 freq);

    // API to check whether input freq belongs to 2G band or not
    static bool isValid2GFreq(uint32 freq);

    // API to check whether input freq belongs to 5G band or not
    static bool isValid5GFreq(uint32 freq);

    // API to check whether input freq belongs to 6G band or not
    static bool isValid6GFreq(uint32 freq);

    // API to check whether input freq belongs to any valid band or not
    static bool isValidCenFreq(uint32 freq);

    /**
     * Checks whether peer is wigig given a channel frequency. Valid channel
     * frequencies are computed as follows:
     *   BAND_60G_FREQ_BASE + BAND_60G_CHAN_SPACING * channelIndex
     *
     * @param freq : channel frequency of the peer in MHz
     * @return bool: true if freq matches a valid 60G channel, else false
     */
    inline static bool isWigigPeer(uint32 freq)
    {
      for (uint32 chIdx = BAND_60G_CHAN_BEGIN; chIdx <= BAND_60G_CHAN_END; chIdx++)
      {
        if (BAND_60G_FREQ_BASE + BAND_60G_CHAN_SPACING * chIdx == freq)
        {
          return true;
        }
      }
      return false;
    }

    /**
     * Adds the common ranging scan parameters to the postcard
     * @param req: Ranging scan request
     * @param bssid: mac address to be added to the card
     * @param: reqType: The ranging scan request type
     */
    static void rangeReqToCardCommonParams(LOWIRangingScanRequest* const req,
                                           OutPostcard * card, const char* reqType);
    /**
     * Adds ranging node info to the card
     * @param info: Ranging node info element
     * @param node_card: input node postcard
     */
    static void rangeReqToCardNodeInfo(LOWINodeInfo &info, OutPostcard * node_card);
    /**
     * Adds the mac address to the postcard
     * @param info: Periodic ranging node info element
     * @param node_card: input  node postcard
     */
    static void rangeReqToCardPeriodicNodeInfo(LOWIPeriodicNodeInfo &info, OutPostcard * node_card);
    /**
     * Log TAG
     */
    static const char * const TAG;

    /**
     * Parses the Scan Measurements from the InPostCard
     * @param InPostcard* InPostCard from which the scan measurements
     *                    to be parsed
     * @param  [in] vector<LOWIScanMeasurement*>& Measurement vector for all
     *                                       measurements.
     * @return true - success, false - otherwise
     */
    static bool parseScanMeasurements
      (InPostcard* const postcard, vector <LOWIScanMeasurement*> & scan);

    /**
     * Parses the Measurements Info from the InPostCard
     * @param InPostcard* InPostcard from which the measurement info to be parsed
     * @param  [in] vector<LOWIMeasurementInfo*>& Measurement info
     *                                            for all measurements.
     * @return true - success, false - otherwise
     */
    static bool parseMeasurementInfo
      (InPostcard* const card, vector <LOWIMeasurementInfo*>& meas_info);

    /**
     * Parses the information element data
     *
     * @param card: InPostCard from which measurements are to be parsed
     * @param meas_info: vector where measurement information is to be placed.
     *
     * @return bool: true if success, false if failure
     */
    static bool parseIEDataInfo
      (InPostcard* const card, vector <int8> &meas_info);

    /**
     * Parses the location information element data
     *
     * @param card: InPostCard from which measurements are parsed
     * @param info: location where information is to be placed
     * @param type: string indicatign the type of info: LCI or LCR
     *
     * @return bool: true if success, false if failure
     */
    static bool parseLocationIEDataInfo
      (InPostcard* const card, uint8 *info, uint8 len, char const *type);

    /** Parses ranging scan measurements
     * @param InPostcard* InPostcard from which needs to be parsed
     * @param  [in] LOWIRangingScanMeasurement& Reference to the object
     *                                          to be populated with parsed data
     * @return true - success, false - otherwise
     */
    static bool parseRangingScanMeasurements (InPostcard* const card,
                                              LOWIRangingScanMeasurement& ranging);

    /**
     * Parses the Location IEs from the InPostcard
     * @param InPostcard* InPostcard from which the Location IEs to be parsed
     * @param  [in] vector<LOWIMeasurementInfo*>& IE vector to get all IEs
     * @return true - success, false - otherwise
     */
    static bool parseLocationIEs (InPostcard* const card, vector <LOWILocationIE*>& lie);

    /**
     * Composes an OutPostCard from the Request created by the client.
     *
     * This API is intended for the clients that have there own socket based IPC
     * implementation and are registered with the IPC hub. Such clients just need
     * to call this API to convert the Request to an OutPostCard which could then
     * be sent to the IPC Hub by the client. The recipient field of the Postcard
     * is populated by this API.
     *
     * Note: Memory should be deallocated by the client
     *
     * @param LOWIRequest* Request to be converted to an OutPostCard
     * @param char* ID of the originator which will be added to the OutPostCard
     * @return OutPostCard
     */
    static OutPostcard* requestToOutPostcard (LOWIRequest* const request,
                                              const char* const originatorId);

    /**
     * Parses an InPostCard and generates the Response needed by the client.
     *
     * This API is intended for the clients that have there own socket based IPC
     * implementation and are registered with the IPC hub. Such clients communicate
     * with the uWifiPsoAPI process through IPC hub to send and receive Postcards
     * on there own and need this API to parse the InPostCard into a Response.
     *
     * Note: Memory should be deallocated by the client
     *
     * @param InPostcard* InPostcard to be parsed
     * @return LOWIResponse
     */
    static LOWIResponse* inPostcardToResponse (InPostcard* const postcard);

    /**
     * Creates a Request from a InPostcard
     * Used by the LOWI server to parse the InPostcard and create a Request
     * @param InPostcard* Postcard
     * @return LOWIRequest
     */
    static LOWIRequest* inPostcardToRequest (InPostcard* const card);

    /**
     * Creates a OutPostcard from the response.
     * Used by the LOWI server to create a OutPostcard to be sent to the Hub.
     * @param LOWIResponse* Response for which the Postcard is to be created
     * @param char* ID of the receiver of this postcard
     */
    static OutPostcard* responseToOutPostcard (LOWIResponse* resp,
                                               const char* to);

    /**
     * Injects the MeasurementInfo into the Post card
     * @param OutPostcard Card to be filled with Measurement Info
     * @param vector <LOWIMeasurementInfo*> Measurements container from where
     *        the measurement info is to be extracted
     * @return true - success, false otherwise
     */
    static bool injectMeasurementInfo (OutPostcard & card,
                                       vector <LOWIMeasurementInfo*> & info);

    /**
     * Injects the ScanMeasurements into the Postcard.
     * @param OutPostcard Card to be filled with Scan measurements
     * @param vector <ScanMeasurement*> Scan Measurements
     * @return true - success, false otherwise
     */
    static bool injectScanMeasurements (OutPostcard & card,
                                        vector <LOWIScanMeasurement*> & meas);

    /**
     * Injects the Infomation Element (IE) data into a postcard
     *
     * @param card: OutPostcard to be filled with IE data
     * @param info: IE data
     * @return bool: true if success, false otherwise
     */
    static bool injectIeData (OutPostcard & card, vector <int8> & info);

    /**
     * Injects the Infomation Element (IE) data into a postcard
     *
     * @param card: OutPostcard to be filled with location IE data
     * @param info: location IE data
     * @param len : length of IE data
     * @param type: string describing the type of data: LCI or LCR
     *
     * @return bool: true if success, false otherwise
     */
    static bool injectLocationIeData (OutPostcard & card, uint8 *info, uint8 len,
                                      char const *type);

    /**
     * Injects the Ranging measurements into the OutPostcard
     * @param OutPostcard&: Reference to OutPostcard to be filled with data
     * @param LOWIRangingScanMeasurement&: ranging measurements
     * @return bool: true if success, false otherwise
     */
     static bool injectRangingScanMeasurements (OutPostcard & card,
                                                LOWIRangingScanMeasurement& ranging);
    /**
     * Injects the LocationIE vector into the OutPostcard
     * @param OutPostcard&: Reference to OutPostcard to be filled with IE data
     * @param vector <LOWILocationIE*>&: Location IE vector
     * @return bool: true if success, false otherwise
     */
    static bool injectLocationIEs (OutPostcard & card,
                                   vector <LOWILocationIE*> & info);

    /** Various functions for type conversion, printing, etc */
    static LOWIResponse::eResponseType to_eResponseType (int a);
    static LOWIResponse::eScanStatus to_eScanStatus (int a);
    static LOWIDiscoveryScanResponse::eScanTypeResponse to_eScanTypeResponse(int a);
    static LOWIDiscoveryScanResponse::eScanTypeResponse to_eScanTypeResponse(LOWIDiscoveryScanRequest::eScanType a);
    static eNodeType to_eNodeType (int a);
    static eRttType to_eRttType (unsigned char a);
    static eTmrType to_eTmrType (unsigned char a);
    static eSecType to_eSecType (unsigned char a);
    static eRttReportType to_eRttReportType (unsigned char a);
    static eRangingBandwidth to_eRangingBandwidth (uint8 a);
    static eRangingPreamble to_eRangingPreamble (uint8 a);
    static eRangingPreamble phymodeToPreamble (uint32 a);
    static eRangingBandwidth phymodeToBw (uint32 a);
    static LOWIDiscoveryScanRequest::eBand to_eBand (int a);
    static LOWIDiscoveryScanRequest::eScanType to_eScanType (int a);
    static LOWIDiscoveryScanRequest::eRequestMode to_eRequestMode (int a);
    static LOWIConfigRequest::eConfigRequestMode to_eConfigRequestMode (uint8 a);
    static eLOWIVariant to_eLOWIVariant (uint8 a);
    static qc_loc_fw::ERROR_LEVEL to_logLevel (int a);
    static eLowiMotionPattern to_eLOWIMotionPattern(uint8 a);
    static LOWIResponse::eScanStatus to_eLOWIDriverStatus(uint8 a);
    static LOWIScanMeasurement::eScanMeasurementType to_eScanMeasurementType (uint8 a);
    static eLOWIPhyMode to_eLOWIPhyMode (int8 a);
    static LOWIScanMeasurement::eEncryptionType to_eEncryptionType (uint8 a);
    static LOWIScanMeasurement::ePeerOEM to_ePeerOEM (uint8 a);
    static eLowiWlanInterface to_eLowiWlanInterface (uint8 a);

    /* The following functions convert enumerations to Strings */
    static char const* to_string(LOWIResponse::eScanStatus a);
    static char const* to_string(LOWIRequest::eRequestType a);
    static char const* to_string(LOWIResponse::eResponseType a);
    static char const* to_string(eRttReportType a);
    static char const* to_string(eLOWIPhyMode a);
    static char const* to_string(eRangingPreamble a);
    static char const* to_string(LOWIScanMeasurement::ePeerOEM a);
    static char const* to_string(LOWIScanMeasurement::eEncryptionType a);
    static char const* to_string(eRangingBandwidth a);
    static char const* to_string(eTmrType a);
    static char const* to_string(eSecType a);
    static char const* to_string(uint32 band_mask);
    static char const* to_string(LOWIDiscoveryScanRequest::eBand a);
    static char const* to_string(LOWIDiscoveryScanRequest::eBandBit a);
    /**
     * Function description:
     *    This function will perform look up in a string table using
     *    a key value and respond with the corresponding string. The
     *    function will also protect against accesing values that
     *    are not within the range of the string table size.
     *
     * Parameters:
     *    size_t: Input key value used for stirng look up.
     *    const char*[]: String loop up table.
     *    size_t: max size of string look up table.
     *
     * Return value:
     *    const char*: String from the string table.
     */
    static char const* to_string(size_t val, const char * arr[], size_t arr_size);

    /**
     * Function description:
     *    This function will return current time in number of milli-seconds
     *    since Epoch (00:00:00 UTC, January 1, 1970Jan 1st, 1970).
     *
     * Parameters:
     *    none
     *
     * Return value:
     *    number of milli-seconds since epoch.
     */
    static int64 currentTimeMs ();

    /**
     * Returns Channel corresponding the frequency. No regulatory
     * domain check is performed. A valid channel number is returned
     * as long as the frequency is within the bounds.
     * @param uint32 Frequency which can be in 2.4 GHz, 5 GHz or 60
     *               GHz band
     * @return 0 if frequency that does not match a valid channel
     *         in the 2.4/5/60 GHz bands, valid channel number
     *         otherwise
     */
     static uint32 freqToChannel (uint32 freq);

     static std::pair<uint32, LOWIDiscoveryScanRequest::eBandBit> freqToChBandBit(uint32 freq);

    /**
     * Returns the frequency for the band and channel passed.
     * @param uint32 Channel number
     * @param LOWIDiscoveryScanRequest::eBand Band of the channel
     * @return Frequency
     */
    static uint32 channelBandToFreq (uint32 channel,
        LOWIDiscoveryScanRequest::eBand band = LOWIDiscoveryScanRequest::BAND_2P4_5_GHZ);

    /**
     * Returns the band for the frequency passed.
     * @param uint32 Frequency in 2.4 / 5 Ghz band
     * @return associated band
     */
    static LOWIDiscoveryScanRequest::eBand freqToBand( uint32 freq );

    static LOWIDiscoveryScanRequest::eBandBit getBandBit(
           LOWIDiscoveryScanRequest::eBand
           band = LOWIDiscoveryScanRequest::INVALID_BAND );

    static LOWIDiscoveryScanRequest::eBandBit freqToBandBit( uint32 freq = 0);
    static uint8 getBandBitFromOpClass (
           LOWIDiscoveryScanRequest::eBandBit &bandBit, uint16 op_class);
    /**
     * Returns the frequency for the band and channel passed.
     * @param uint32 Channel number
     * @param LOWIDiscoveryScanRequest::eBand Band of the channel
     * @return Frequency
     */
    static uint32 channelBandBitToFreq (uint32 channel,
           LOWIDiscoveryScanRequest::eBandBit bandBit = LOWIDiscoveryScanRequest::BAND_INVALID_BIT);

    /**
     * Returns the 60G frequency for the channel passed.
     * @param uint32 Channel number
     * @return valid 60G frequency if channel is valid, else 0.
     */
    static uint32 channelToFreq60G (uint32 channel);

    /**
     * Get Default frequency's corresponding to the band
     * here no regulatory rules are applied on this freq
     * @param LOWIDiscoveryScanRequest::eBand Band
     * @param unsigned char Num of channels
     * @param 6gh  user config to tell whether use PSC 6GH or all channels
     * @return Pointer to the freq array
     */
    static int * getDefaultFreqs (uint32 band_mask,
                      unsigned char & num_channels,
                      LOWIDiscoveryScanRequest::e6GChannelType cfg_6g_ch);

   /**
     * Get freqency's corresponding to the ChannelInfo
     * @param vector<LOWIChannelInfo> & Channels
     * @param unsigned char Num of channels
     * @return Pointer to the array of channels
     */
    static int * getFreqFromChInfo (vector<LOWIChannelInfo> & v,
                                     unsigned char & channels);

    /**
     *  Determine if the request contains a background scan type. Background scan
     *  types are the following: BGSCAN_CAPABILITIES, BGSCAN_START, BGSCAN_STOP,
     *  BGSCAN_CACHED_RESULTS, HOTLIST_SET, HOTLIST_CLEAR,
     *  SIGNIFINCANT_CHANGE_LIST_SET, SIGNIFINCANT_CHANGE_LIST_CLEAR, SCANNING_MAC_OUI_SET
     *
     * @param request: LOWI request
     * @return bool: true if request type is among the ones specified here, else
     *         false.
     */
    static bool isBackgroundScan(LOWIRequest const *request);

    /**
     * Checks if LOWIRequest is allowed to be processed at the LP
     *
     * @param request: LOWI request
     * @return bool: true if request type is allowed through the LP, else false
     */
    static bool isBgScanReqAllowedThroughLP(LOWIRequest const *request);

    /**
     * Prints the contents of a char buffer up to 2048 bytes long
     *
     * @param char*: msg to print
     * @param uint32: msg length. Must be <= 2048 bytes.
     */
    static void hexDump(char *msg, uint32 len);

    /**
     * Get the 80MHz channel center frequency (i.e. center_freq1) based on the
     * primary 20MHz frequency.
     *
     * @param uint32: primary 20MHz frequency
     * @param uint32: Max channel frequency
     * @return uint32: Returns 80MHz Center frequency
     */
    static uint32 get80MHzcfreq(uint32 primaryFreqMhz, uint32 maxFreq);

    /**
     * Get 40MHz channel center frequency based on primary 20MHz frequency.
     *
     * @param uint32: primary 20MHz frequency
     * @param uint32: Max channel frequency
     * @return uint32: Returns 40MHz Center frequency
     */
    static uint32 get40MHzcfreq(uint32 primaryFreqMhz, uint32 maxFreq);

    /**
     * Get 160MHz channel center frequency based on primary 20MHz frequency
     *
     * @param uint32: primary 20MHz frequency
     * @param uint32: Max channel frequency
     * @return uint32: Returns 160MHz Center frequency
     */
    static uint32 get160MHzcfreq(uint32 primaryFreqMhz, uint32 maxFreq);

    /**
     * Get 2GHz 40PLUS channel center frequency based on primary 20MHz frequency
     *
     * @param uint32: primary 20MHz frequency
     * @param uint32: Max 2GHz channel frequency
     * @return uint32: Returns 2GHz 40PLUS Center frequency
     */
    static uint32 get2GHz40Pluscfreq(uint32 primaryFreqMhz, uint32 max2ghzFreq);

    /**
     * Get 2GHz 40MINUS channel center frequency based on primary 20MHz
     * frequency
     *
     * @param uint32: primary 20MHz frequency
     * @param uint32: Max 2GHz channel frequency
     * @return uint32: Returns 2GHz 40MINUS Center frequency
     */
    static uint32 get2GHz40Minuscfreq(uint32 primaryFreqMhz,
            uint32 max2ghzFreq);
};

} // namespace qc_loc_fw

#endif //#ifndef __LOWI_UTILS_H__
