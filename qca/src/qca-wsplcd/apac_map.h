/*
 * @@-COPYRIGHT-START-@@
 *
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @@-COPYRIGHT-END-@@
 */

#include "wsplcd.h"
#if HYFI10_COMPATIBLE
#include "apclone.h"
#endif
#include "eloop.h"

#include "apac_hyfi20_wps.h"
#include "apac_hyfi20_mib.h"
#include "apac_priv.h"

/* BackHaul Steering Parameters */
#define BACKHAUL_STA_MAC "BackhaulStaMac"
#define TARGET_BSSID "TargetBSSID"

/* Traffic Separation Parameters */
#define CONFIG_NETWORK_TS_ENABLED "TSEnabled"
#define MAP2_SERVICE_MAX_VLAN_SUPPORTED 4
#define CONFIG_NETWORK_PRIMARY_VLAN "PrimaryLAN"
#define CONFIG_NETWORK_SECONDARY_VLAN "SecondaryLAN"
#define CONFIG_NETWORK_PRIMARY_VLAN_ID "PrimaryVLANID"
#define CONFIG_NETWORK_SECONDARY_VLAN_ID "SecondaryVLANID"
#define CONFIG_NETWORK_BSTA_IFACE "bSTAIface"
#define CONFIG_NETWORK_BHBSS_IFACE "bhBSSIface"
#define CONFIG_NETWORK_BHBSS_IFACE_R1_ONLY "R1BhBSSIface"
#define CONFIG_NETWORK_UPSTREAM_DEVICE_VERSION "UpstreamDeviceVersion"


#define MAP_SERVICE_AGENT_MAX_BRIDGE_IFNAME_LENGTH 128
#define APAC_MAP_ALID_DB_FILE               "/tmp/alid_db.bin"
#define APAC_MAP_ALID_DB_MAX_ALID_COUNT      64

/* MAP R6 defines */
#define APAC_MAP_MAX_SUPPORTED_MLDS             10
#define APAC_MAP_MAX_BSTA_LINKS                 2
#define APAC_MAP_HOMOGENEOUS_T2LM_SUPPORT       2

typedef struct apac_alid {
    u8 addr[ETH_ALEN];
}apac_alid;

typedef struct apac_alid_list {
    u8 no_of_alid;
    u8 max_renew_retry;
    u16 mid;
    apac_alid alid[APAC_MAP_ALID_DB_MAX_ALID_COUNT];
    u8 renew_acknowledged[APAC_MAP_ALID_DB_MAX_ALID_COUNT];
}apac_alid_list;

typedef struct apac_alid_db {
    u8 alid_count;
    apac_alid alid[0];
}apac_alid_db;

enum map_wps_attribute {
    ATTR_AUTH_TYPE = 0x1003,
    ATTR_CRED = 0x100e,
    ATTR_NETWORK_INDEX = 0x1026,
    ATTR_SSID = 0x1045,
    ATTR_ENCR_TYPE = 0x100f,
    ATTR_NETWORK_KEY_INDEX = 0x1028,
    ATTR_NETWORK_KEY = 0x1027,
    ATTR_MAC_ADDR = 0x1020
};

struct credbuf {
    u8 len;
    u8 buf[200];
};

apacHyfi20MapVersion_e apacHyfiMapIsEnabled(apacMapData_t *map);
apacBool_e apacHyfiMapPfComplianceEnabled(apacMapData_t *map);
apacBool_e apacHyfiMapIsTrafficSeparationEnabled(apacMapData_t *map);
apacBool_e apacHyfiMapIsSendPacketOnBridge(apacMapData_t *map);
apacBool_e apacHyfiMapIsExternalController(apacMapData_t *map);
apacBool_e apacHyfiMapConfigServiceEnabled(apacMapData_t *map);
apacBool_e apacHyfiMapEarlyAPCapabilitySupported(apacMapData_t *map);
u8 apacHyfiMapIsCtrlSpecMloOnbSupported(apacMapData_t *map);
void apacHyfiMapCtrlSetSpecMloOnbSupported(apacMapData_t *map, u8 enable);
apacBool_e apacHyfiMapIsUseLoInterface(apacMapData_t *map);
u8 *apacHyfiGetControllerAlid(apacMapData_t *map);

#if MAP_ENABLED
#if DOT1X_SUPPORT
apacHyfi20MapVersion_e apacHyfiMapIsDot1xEnabled(apacMapData_t *map);
#endif
#endif
apacBool_e apacHyfiMapIsMLOEnabled(apacMapData_t *map);
apacBool_e apacHyfiMapIsSLOEnabled(apacMapData_t *map);
int apac_map_get_mib_vap_index(apacMapData_t *map, int idx);
int apacMapGetRadioIdxByOpclass(apacMapData_t *map, int numOpClassRanges, int minOpClass, int maxOpClass,
        apacHyfi20WifiBandType_e bandType);
ieee1905TLV_t *ieee1905MapAddRadioIdTLV(ieee1905TLV_t *TLV,
        u_int32_t *Len,
        struct apac_wps_session *sess);

ieee1905TLV_t *ieee1905MapAddTrafficSeparationPolicyTLV(ieee1905TLV_t *TLV, u_int32_t *Len,
                                                        struct apac_wps_session *sess);

ieee1905TLV_t *ieee1905MapAddMLOPolicyTLV(ieee1905TLV_t *TLV, u_int32_t *Len,
                                          struct apac_wps_session *sess);

/**
 * @brief Initialize the Multi-AP SIG component.
 *
 * @param [inout] map  the structure tracking all Multi-AP configuration
 *                     attributes and runtime state
 * @param [in] profileType  the type of profile specifications to expect
 *                          in the file
 * @param [in] filename  the file to read the profile specifications from
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
apacBool_e apacHyfiMapInit(apacMapData_t *map);

/**
 * @brief Terminate the Multi-AP SIG component.
 *
 * Clean up all memory and reset the internal state to its uninitialized
 * state.
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
apacBool_e apacHyfiMapDInit(apacMapData_t *map);

/**
 * @brief  Print the current configuration to the debug log stream/file.
 *
 * @param [in] map  the structure tracking all Multi-AP configuration
 *                  attributes and runtime state
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
void apacHyfiMapConfigDump(const apacMapData_t *map);

int apacGetPIFMapCap( apacHyfi20Data_t *pData);

int apacGetPIFMapChanScanCap(apacHyfi20Data_t *pData, u_int8_t opclass,
                             u_int8_t *num_oper_chan, u_int8_t oper_chan_num[],
                             u_int8_t *ch_width);

ieee1905TLV_t *ieee1905MapAddAPCapabilityTLV(ieee1905TLV_t *TLV,
        u_int32_t *Len,
        apacHyfi20Data_t *pData);

ieee1905TLV_t *ieee1905MapAddBasicRadioTLV(ieee1905TLV_t *TLV,
        u_int32_t *Len,
        u8 band,
        apacHyfi20Data_t *pData);

ieee1905TLV_t *ieee1905MapAddMLOVendorTLV(ieee1905TLV_t *TLV,
        u_int32_t *Len,
        u8 band,
        apacHyfi20Data_t *pData);

ieee1905TLV_t *ieee1905MapAddMetricCollectionTLV(ieee1905TLV_t *TLV,
        u_int32_t *Len,
        u8 band,
        apacHyfi20Data_t *pData);

ieee1905TLV_t *ieee1905MapAddDeviceInventoryTLV(ieee1905TLV_t *TLV,
        u_int32_t *Len,
        u8 band,
        apacHyfi20Data_t *pData);

void serializeWifi7AgentCapabilitiesTLV(u8 *wifi7CapMsg, u32 *wifi7CapMsgLen,
                                        apacHyfi20Data_t *pData);

ieee1905TLV_t *ieee1905MapAddAKMSuiteCapabilitiesTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                     apacHyfi20Data_t *pData);

ieee1905TLV_t *ieee1905MapAddAPHTCapabilitiesTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                 apacHyfi20Data_t *pData);

ieee1905TLV_t *ieee1905MapAddAPVHTCapabilitiesTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                 apacHyfi20Data_t *pData);

ieee1905TLV_t *ieee1905MapAddAPHECapabilitiesTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                 apacHyfi20Data_t *pData);

ieee1905TLV_t *ieee1905MapAddWifi6CapabilitiesTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                  apacHyfi20Data_t *pData);

ieee1905TLV_t *ieee1905Map2AddApCapTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                       apacHyfi20Data_t *pData);

ieee1905TLV_t *ieee1905Map2AddApRadioAdvancedCapTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                    apacHyfi20Data_t *pData);

u8 apac_map_get_eprofile(struct apac_wps_session* sess, u8 *list, u8 *requested_m2);

u8 apac_map_get_configured_maxbss(struct apac_wps_session *sess);

u8 apac_map_parse_vendor_ext(struct apac_wps_session *sess,
        u8 *vendor_ext,
        u8 vendor_ext_len,
        u8 *mapBssType);

/**
 * @brief Populate the information necessary to instantiate a BSS based on
 *        the encryption profile specified by its index.
 *
 * @param [in] map  the overall structure for Multi-AP state
 * @param [in] index  the index of the encryption profile
 * @param [out] ap  the structure into which to copy the data
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
apacBool_e apac_map_copy_apinfo(apacMapData_t *map, u8 index, apacHyfi20AP_t *ap, u8 DbgLvl);

#define MAP_SERVICE_MAX_SSID_LEN 32
#define MAP_SERVICE_MAX_INTERFACES 15
/**
 * @brief Representation of Traffic Seperation policy
 */
typedef struct mapServiceTrafficSepPolicy_t {
    /// Number of SSIDs
    u_int8_t numOfSSIDs;

    struct {
        /// Length of SSID name
        u_int8_t ssidLen;

        /// SSID name
        char ssid[MAP_SERVICE_MAX_SSID_LEN];

        /// 0x0000 – 0x0002: Reserved
        /// 0x0003 – 0x0FFE
        /// 0xFFF – 0xFFFF: Reserved
        u_int16_t vlanID;
    } interfaceConf[MAP_SERVICE_MAX_INTERFACES];
} mapServiceTrafficSepPolicy_t;

u8 ieee1905MapParseTrafficSepTLV(struct apac_wps_session *sess,
                                 mapServiceTrafficSepPolicy_t *trafficSepPolicy);

/**
 * @brief Stores the alid to alid db file. If the alid is
 *        already present it will not be overwritten.
 *
 * @param [in] alid pointer to alid to be stored
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
apacBool_e apac_map_alid_db_store_alid(apac_alid *alid);

/**
 * @brief Read all the alid entries available in the alid db file
 *
 * @param [out] alid  array in which alid read from db will be populated
 * @param [in] count  when calling, it is the maximum number of alid entries
 *                    that can be safely copied to the array provided by the
 *                    caller
 * @param [out] count when returning, it is number of actual entries read from
 *                    the db.
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
apacBool_e apac_map_alid_db_get_all_alid(apac_alid *alid, u8 *count);

#define MAP_SERVICE_MAX_AP_MLDS 16
#define MAP_SERVICE_MAX_AFFILIATED_AP 4
/**
 * @brief Representation of AP Agent MLD Config
 */
typedef struct mapServiceApAgentMldConfig_t {
    /// Number of AP MLDs
    u_int8_t numAPMLDs;

    struct {
        /// Indicates whether the AP_MLD_MAC_Addr field is valid
        apacBool_e apMldMacAddrValid;
        /// Length of SSID name
        u_int8_t ssidLen;
        /// SSID name
        char ssid[MAP_SERVICE_MAX_SSID_LEN];
        /// AP MLD mac addr
        u8 apMldMacAddr[SIZE_MAC_ADDR];
        /// STR Enable
        apacBool_e strEnable;
        /// NSTR Enable
        apacBool_e nstrEnable;
        /// EMLSR Enable
        apacBool_e emlsrEnable;
        /// EMLMR Enable
        apacBool_e emlmrEnable;
        /// Number of Affiliated APs belonging to this MLD
        u_int8_t numAffiliatedAPs;
        struct {
                /// Indicates whether the Affiliated_AP_MAC_Addr field is valid
                apacBool_e affiliatedApMacAddrValid;
                /// RUID
                u8 RUID[SIZE_MAC_ADDR];
                /// Affiliated AP mac addr
                u8 affiliatedApMacAddr[SIZE_MAC_ADDR];
                /// Link ID
                u8 LinkID;
        } affiliatedAPConf[MAP_SERVICE_MAX_AFFILIATED_AP];

    } apMLDConf[MAP_SERVICE_MAX_AP_MLDS];
} mapServiceApAgentMldConfig_t;

/**
 * @brief Representation of bsta MLD Config
 */
typedef struct mapServiceBstaMldConfig_t {
        /// Indicates whether the bSTA_MLD_MAC_Addr field is valid
        apacBool_e bstaMldMacAddrValid;
        /// Indicates whether the AP_MLD_MAC_Addr field is valid
        apacBool_e apMldMacAddrValid;
        /// bsta MLD mac addr
        u8 bstaMldMacAddr[SIZE_MAC_ADDR];
        /// AP MLD mac addr
        u8 apMldMacAddr[SIZE_MAC_ADDR];
        /// STR Enable
        apacBool_e strEnable;
        /// NSTR Enable
        apacBool_e nstrEnable;
        /// EMLSR Enable
        apacBool_e emlsrEnable;
        /// EMLMR Enable
        apacBool_e emlmrEnable;
        /// Number of Affiliated bSTAs belonging to this MLD
        u_int8_t numAffiliatedbSTAs;
        struct {
                /// Indicates whether the Affiliated_bsta_MAC_Addr field is valid
                apacBool_e affiliatedBstaMacAddrValid;
                /// RUID
                u8 RUID[SIZE_MAC_ADDR];
                /// Affiliated bSta mac addr
                u8 affiliatedBstaMacAddr[SIZE_MAC_ADDR];
                /// Link ID
                u8 LinkID;
        } affiliatedBstaConf[MAP_SERVICE_MAX_AFFILIATED_AP];
} mapServiceBstaMldConfig_t;

u8 ieee1905MapParseAgentApMldConfigTLV(struct apac_wps_session *sess,
                                       mapServiceApAgentMldConfig_t *apAgentMldConfig);

u8 ieee1905MapParseBstaMldConfigTLV(struct apac_wps_session *sess,
                                    mapServiceBstaMldConfig_t *bstaMldConfig);
