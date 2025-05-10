/*
 * @@-COPYRIGHT-START-@@
 *
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @@-COPYRIGHT-END-@@
 */
#include <sys/socket.h>
#include <sys/file.h>
#include <fcntl.h>
#include <netinet/ether.h>
#include <string.h>
#include "apac_map.h"
#include "wlanif_cmn.h"
#include "apac_priv.h"
#ifdef SON_MEMORY_DEBUG

#include "qca-son-mem-debug.h"
#undef QCA_MOD_INPUT
#define QCA_MOD_INPUT QCA_MOD_WSPLCD
#include "son-mem-debug.h"

#endif /* SON_MEMORY_DEBUG */
extern struct wlanif_config *wlanIfWd;

u8 ieee1905MapParseNonOperableChannel(u8 *val, u_int32_t Len, u8 bandType);

/**
 * @brief Convert a string to an unsigned integer, performing proper error
 *        checking.
 *
 * @param [in] buf  the string to convert
 * @param [in] base  the base to use for the conversion
 * @param [in] paramName  the name of the parameter to use in any error logs
 * @param [in] line  the line number to use in any error logs
 * @param [out] val  the converted value on success
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
static apacBool_e apacHyfiMapParseInt(const char *buf, int base, const char *paramName,
                                      int line, unsigned long *val) {
    char *endptr;

    errno = 0;
    *val = strtoul(buf, &endptr, base);
    if (errno != 0 || *endptr != '\0') {
        dprintf(MSG_ERROR, "%s: Failed to parse %s '%s' on line %d\n",
                __func__, paramName, buf, line);
        return APAC_FALSE;
    }

    return APAC_TRUE;
}

#if DOT1X_SUPPORT
/*
 * @brief Parsing and storing the suiteb credential detail
 * @param [in] buf  portion of the line to parse
 * @param [in] line  line number
 * @param [in] eProfile  the entry to populate
 *
 * @return  APAC_TRUE on success; otherwise APAC_FALSE
 */
static apacBool_e apacHyfiMapParseAndStoreSuiteBEProfile(const char *buf, int line,
                                                        apacMapEProfile_t *eProfile) {
    char tag[MAX_SSID_LEN];
        int len = 0;
    const char *pos = buf;
    unsigned long val;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract suite-b on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    if (!apacHyfiMapParseInt(tag, 0, "suite-b", line, &val)) {
        return APAC_FALSE;
    }
    eProfile->suite_b = val;
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract group management cipher on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    eProfile->group_mgmt_cipher = strdup(tag);
    dprintf(MSG_DEBUG, "%s:%d> eProfile=%p group_mgmt_cipher=%s\n", __func__, __LINE__,
              eProfile, eProfile->group_mgmt_cipher);
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract group_rekey_period on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    if (!apacHyfiMapParseInt(tag, 0, "group rekey", line, &val)) {
        return APAC_FALSE;
    }
    eProfile->group_rekey_period = val;
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, 0, tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract ieee80211w on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    if (!apacHyfiMapParseInt(tag, 0, "ieee80211w", line, &val)) {
        return APAC_FALSE;
    }
    eProfile->ieee80211w = val;
    pos += len + 1;

    return APAC_TRUE;
}

/*
 * @brief Parsing and storing the dot1x credential detail
 * @param [in] buf  portion of the line to parse
 * @param [in] line  line number
 * @param [in] eProfile  the entry to populate
 *
 * @return  APAC_TRUE on success; otherwise APAC_FALSE
 */

static apacBool_e apacHyfiMapParseAndStoreDot1xEProfile(const char *buf, int line,
                                                        apacMapEProfile_t *eProfile) {
    char tag[MAX_DOT1X_TAG_LEN];
	int len = 0;
    const char *pos = buf;
    unsigned long val;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract primary server ip on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    eProfile->auth_primary_server = strdup(tag);
    dprintf(MSG_DEBUG, "%s:%d> eProfile=%p Primary Server=%s\n", __func__, __LINE__,
              eProfile, eProfile->auth_primary_server);
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract primary port on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    if (!apacHyfiMapParseInt(tag, 0, "auth primary port", line, &val)) {
        return APAC_FALSE;
    }
    eProfile->auth_primary_port = val;
    dprintf(MSG_MSGDUMP, "%s:%d> Primary Port=%d\n", __func__, __LINE__,
              eProfile->auth_primary_port);
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not primary secret on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    eProfile->auth_primary_secret = strdup(tag);
    dprintf(MSG_MSGDUMP, "%s:%d> Primary Secret=%s\n", __func__, __LINE__,
              eProfile->auth_primary_secret);
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_MSGDUMP, "%s: Could not extract secondary server ip on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    eProfile->auth_secondary_server = strdup(tag);
    dprintf(MSG_MSGDUMP, "%s:%d> Secondary Server=%s\n", __func__, __LINE__,
              eProfile->auth_secondary_server);
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract primary port on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    if (!apacHyfiMapParseInt(tag, 0, "auth secondary port", line, &val)) {
        return APAC_FALSE;
    }
    eProfile->auth_secondary_port = val;
    dprintf(MSG_MSGDUMP, "%s:%d> Secondary Port=%d\n", __func__, __LINE__,
              eProfile->auth_secondary_port);
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not secondary Secret on line %d len=%d\n",
                __func__, line, len);
        return APAC_FALSE;
    }
    eProfile->auth_secondary_secret = strdup(tag);
    dprintf(MSG_MSGDUMP, "%s:%d> Secondary Secret=%s\n", __func__, __LINE__,
              eProfile->auth_secondary_secret);
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract rsn_preauth  on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    if (!apacHyfiMapParseInt(tag, 0, "rsn preauth", line, &val)) {
        return APAC_FALSE;
    }
    eProfile->rsn_preauth = val;
    dprintf(MSG_MSGDUMP, "%s:%d> rsn preauth=%d\n", __func__, __LINE__,
              eProfile->rsn_preauth);
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not rn_preauth interfaces on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    eProfile->rsn_preauth_interfaces = strdup(tag);
    dprintf(MSG_MSGDUMP, "%s:%d> rsn preauth interface=%s\n", __func__, __LINE__,
              eProfile->rsn_preauth_interfaces);
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if(len <=0) {
        dprintf(MSG_ERROR, "%s: Could not ft_auth on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    if(!apacHyfiMapParseInt(tag, 0, "ft auth", line, &val)){
        return APAC_FALSE;
    }
    eProfile->ft_auth = val;
    dprintf(MSG_MSGDUMP, "%s:%d> ft auth=%d\n", __func__, __LINE__,
                eProfile->ft_auth);
    pos += len + 1;
 
    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    // Check kh key only if FT is eanbled
    if( eProfile->ft_auth && len <=0) {
        dprintf(MSG_ERROR, "%s: Could not kh_key_hex on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    if (len > 0) {
        eProfile->kh_key_hex = strdup(tag);
        dprintf(MSG_MSGDUMP, "%s:%d> kh key hex=%s\n", __func__, __LINE__,
                eProfile->kh_key_hex);
    }
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, 0, tag, sizeof(tag));
    if(len <=0) {
        dprintf(MSG_ERROR, "%s: Could not mobility_domain on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
    eProfile->mobility_domain = strdup(tag);
    dprintf(MSG_MSGDUMP, "%s:%d> mobility domain=%s\n", __func__, __LINE__,
                eProfile->mobility_domain);
    pos += len + 1;


    return APAC_TRUE;
}

/*
 * @brief to find correct profile index number to store the dot1x details
 * @param [in] map  map data for profile
 * @param [in] strIndex  ssid to compare
 *
 * @return  numIndex profile index number
 */

static uint8_t apacHyfiMapGetIndexNumber ( apacMapData_t *map, const char *strIndex ) {
    uint8_t numIndex = -1, count = 0;
    int ret;

    while ( count < map->ssidCnt ) {
        ret = strncmp(map->eProfileSSID[count].ssidKey, strIndex, 3);
        if ( !ret) {
            numIndex = count;
            break;
        }
        ++count;
    }

    return numIndex;
}

/*
 * @brief to confirm suiteb profile or not
 * @param [in] strTag  tag to confirm suiteb profile
 *
 * @return 1 if suiteb, 0 otherwise
 */

static uint8_t apacHyfiMap_IsSuiteB(const char *strTag) {
    if(strTag[3] == 'S' && strTag[4] == 'B' )
        return 1;

    return 0;
}

/*
 * @brief Parsing and storing the dot1x credential detail
 * @param [in] buf  portion of the line to parse
 * @param [in] line  line number
 * @param [in] eProfile  the entry to populate
 *
 * @return  APAC_TRUE on success; otherwise APAC_FALSE
 */

static apacBool_e apacHyfiMapParseAndStoreGenericEProfileDot1x(apacMapData_t *map,
                                                               const char *buf, int line)
{
    char tag[MAX_SSID_LEN];
    const char *pos = buf;
    const char *ssidIndex = NULL;
    apacMapEProfile_t *eProfile;
    size_t len;
    uint8_t index;

    if (!apacHyfiMapIsDot1xEnabled(map))
        return APAC_TRUE;

    while (isspace(*pos)) {
        pos++;
    }

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract INDEX on line %d\n", __func__, line);
        return APAC_FALSE;
    }
    ssidIndex = strdup(tag);
    dprintf(MSG_MSGDUMP, "%s:%d> Parsed Index=%s\n", __func__, __LINE__, ssidIndex);
    if (!ssidIndex) {
        dprintf(MSG_ERROR, "%s: Failed to copy SSID Index\n", __func__);
        return APAC_FALSE;
    }
    pos += len + 1;
    index = apacHyfiMapGetIndexNumber(map, ssidIndex);

    if ( index >= IEEE1905_QCA_VENDOR_MAX_INTERFACE ) {
        dprintf(MSG_ERROR, "%s: Invalid SSID Index\n", __func__);
        return APAC_FALSE;
    }
    dprintf(MSG_MSGDUMP, "%s:%d> Parsed Index=%d\n", __func__, __LINE__, index);

    eProfile = &map->eProfileSSID[index].eprofile;

    if (!apacHyfiMap_IsSuiteB(ssidIndex)) {
        if (apacHyfiMapParseAndStoreDot1xEProfile(pos, line, eProfile)) {
            dprintf(MSG_ERROR, "%s: Storing Dot1x '%s' at index %u\n", __func__,
                    ssidIndex, index);
            return APAC_TRUE;
        }
    } else if (apacHyfiMap_IsSuiteB(ssidIndex)){
        if (apacHyfiMapParseAndStoreSuiteBEProfile(pos, line, eProfile)) {
            dprintf(MSG_ERROR, "%s: Storing Dot1x '%s' at index %u\n", __func__,
                    ssidIndex, index);
            return APAC_TRUE;
        }
    }
    return APAC_FALSE;
}
#endif

/**
 * @brief Handle a line for the AL-specific bsta setting file format.
 *
 * @param [in] buf  the line read from the config file
 * @param [in] line  the line number (for use in error messages)
 * @param [out] eProfileMatcher  the entry to populate
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
static apacBool_e apacHyfiMapParseAndStoreALSpecificEBstaSettings(
    const char *buf, int line, apacMapEProfileMatcher_t *eProfileMatcher) {
    apacMapEProfile_t *eProfile = NULL;
    char tag[MAX_SSID_LEN];
    int len = 0;
    const char *pos = buf;
    const char *ruid;
    unsigned long val;
    struct ether_addr ea;
    u8 i = 0;

    eProfileMatcher->matcherType = APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC;
    eProfileMatcher->terminateMatching = APAC_FALSE;

    eProfile = &eProfileMatcher->typeParams.alParams.eprofile;

    eProfile->isBsta = APAC_TRUE;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract AL ID on line %d\n",
                __func__, line);
        return APAC_FALSE;
    } else {
        eProfileMatcher->typeParams.alParams.alId = strdup(tag);
        pos += len + 1;
    }

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract number of radios on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }

    if (!apacHyfiMapParseInt(tag, 0, "num of radios", line, &val)) {
        return APAC_FALSE;
    }

    eProfile->numBstaRadios = val;
    pos += len + 1;

    for( i=0; i<eProfile->numBstaRadios-1; i++) {
        len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
        if (len <= 0) {
            dprintf(MSG_ERROR, "%s: Could not extract RUID%d on line %d\n",
                    __func__, i, line);
            return APAC_FALSE;
        } else {
            ruid = strdup(tag);
            if (!ruid) {
                dprintf(MSG_ERROR, "%s: %u strdup failed\n", __func__, i);
                return APAC_FALSE;
            }
            sscanf(ruid, "0x%02x%02x%02x%02x%02x%02x", (unsigned int *)&ea.ether_addr_octet[0], (unsigned int *)&ea.ether_addr_octet[1],
                                                       (unsigned int *)&ea.ether_addr_octet[2], (unsigned int *)&ea.ether_addr_octet[3],
                                                       (unsigned int *)&ea.ether_addr_octet[4], (unsigned int *)&ea.ether_addr_octet[5]);
            memcpy(eProfile->bstaRuids[i].ether_addr_octet, &ea, ETH_ALEN);
            pos += len + 1;
        }
    }

    len = apac_atf_config_line_getparam(pos, 0, tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract RUID%d on line %d\n", __func__, i, line);
        return APAC_FALSE;
    }

    ruid = strdup(tag);
    if (!ruid) {
        dprintf(MSG_ERROR, "%s: strdup failed\n", __func__);
        return APAC_FALSE;
    }

    sscanf(ruid, "0x%02x%02x%02x%02x%02x%02x", (unsigned int *)&ea.ether_addr_octet[0], (unsigned int *)&ea.ether_addr_octet[1],
                                               (unsigned int *)&ea.ether_addr_octet[2], (unsigned int *)&ea.ether_addr_octet[3],
                                               (unsigned int *)&ea.ether_addr_octet[4], (unsigned int *)&ea.ether_addr_octet[5]);
    memcpy(eProfile->bstaRuids[i].ether_addr_octet, &ea, ETH_ALEN);
    pos += len + 1;

    return APAC_TRUE;
}

/**
 * @brief Parse and store the settings for a single SSID.
 *
 * This captures the security settings for the SSID, along with whether it is
 * acting as backhaul, fronthaul, or both.
 *
 * @param [in] buf  the portion of the line to parse
 * @param [in] line  the line number (for use in error messages)
 * @param [out] eProfile  the entry to populate
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
static apacBool_e apacHyfiMapParseAndStoreEProfile(const char *buf, int line,
                                                   apacMapEProfile_t *eProfile) {
    char tag[MAX_SSID_LEN];
    char psk[MAX_PASSPHRASE_LEN+1];
    int len = 0;
    const char *pos = buf;
    unsigned long val;
#if 0
    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag)); // touch here
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract SSID on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }
#endif


    len = apac_atf_config_line_getparam_modified(pos, ',', tag, sizeof(tag)); // touch here
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract SSID on line %d now\n",
                 __func__, line);
        return APAC_FALSE;
    }

    eProfile->ssid = strdup(tag);
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract auth mode on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }

    if (!apacHyfiMapParseInt(tag, 16, "auth mode", line, &val)) {
        return APAC_FALSE;
    }

    eProfile->auth = val;
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract encryption mode on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }

    if (!apacHyfiMapParseInt(tag, 16, "encryption mode", line, &val)) {
        return APAC_FALSE;
    }

    eProfile->encr = val;
    pos += len + 1;

    len = apac_atf_config_line_getparam_modified(pos, ',', psk, MAX_PASSPHRASE_LEN);
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract PSK on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }

    eProfile->nw_key = strdup(psk);
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract backhaul flag on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }

    if (!apacHyfiMapParseInt(tag, 0, "backhaul flag", line, &val)) {
        return APAC_FALSE;
    }

    eProfile->isBackhaul = val;
    pos += len + 1;

    if (strlen(pos) == 1) {
        len = apac_atf_config_line_getparam(pos, 0, tag, sizeof(tag));
    } else {
        len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    }
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract fronthaul flag on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }

    if (!apacHyfiMapParseInt(tag, 0, "fronthaul flag", line, &val)) {
        return APAC_FALSE;
    }

    eProfile->isFronthaul = val;
    pos += len + 1;
    /// Return when profile is extracted for mapR1
    if (strlen(pos) == 0) {
        return APAC_TRUE;
    }

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (!strncasecmp(tag, "b5", strlen("b5"))) {
        goto parseTLV;
    }

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract MAP profile 1 bSTAAssocDisallowed on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }

    if (!apacHyfiMapParseInt(tag, 0, "Profile-1 STA Assoc DisAllowed", line, &val)) {
        return APAC_FALSE;
    }

    eProfile->map1bSTAAssocDisallowed = val;
    pos += len + 1;

    if (strlen(pos) == 1) {
        len = apac_atf_config_line_getparam(pos, 0, tag, sizeof(tag));
    } else {
        len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    }
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract MAP profile 2 bSTAAssocDisallowed on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }

    if (!apacHyfiMapParseInt(tag, 0, "Profile-2 STA Assoc DisAllowed", line, &val)) {
        return APAC_FALSE;
    }

    eProfile->map2bSTAAssocDisallowed = val;
    pos += len + 1;
    /// Return when profile is extracted for mapR1
    if (strlen(pos) == 0) {
        return APAC_TRUE;
    }

parseTLV:
    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (strncasecmp(tag, "b5", strlen("b5"))) {
        dprintf(MSG_ERROR, "%s: Could not extract TLV Type B5 on line %d\n", __func__, line);
        return APAC_FALSE;
    }
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract TLV Value for B5 on line %d\n", __func__, line);
        return APAC_FALSE;
    }

    if (!apacHyfiMapParseInt(tag, 0, "Primary VLAN ID", line, &val)) {
        return APAC_FALSE;
    }

    eProfile->primaryVlanID = val;
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract TLV Value for B5 on line %d\n", __func__, line);
        return APAC_FALSE;
    }

    if (!apacHyfiMapParseInt(tag, 0, "PCP", line, &val)) {
        return APAC_FALSE;
    }

    eProfile->pcp = val;
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (strncasecmp(tag, "b6", strlen("b5"))) {
        dprintf(MSG_ERROR, "%s: Could not extract TLV Type B6 on line %d\n", __func__, line);
        return APAC_FALSE;
    }
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        len = apac_atf_config_line_getparam(pos, 0, tag, sizeof(tag));
        if (len <= 0) {
            dprintf(MSG_ERROR, "%s: Could not extract TLV Value for B6 on line %d\n", __func__, line);
            return APAC_FALSE;
        }
    }

    if (!apacHyfiMapParseInt(tag, 0, "VLAN ID", line, &val)) {
        return APAC_FALSE;
    }

    eProfile->vlanID = val;
    pos += len + 1;

    /// Return when vlan ID extracting and no MLD info
    if (strlen(pos) == 0) {
        return APAC_TRUE;
    }

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (strncasecmp(tag, "mld", strlen("mld"))) {
        dprintf(MSG_ERROR, "%s: Could not extract mld on line %d\n", __func__, line);
        return APAC_FALSE;
    }
    pos += len + 1;

    len = apac_atf_config_line_getparam(pos, 0, tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract Value for mld on line %d\n", __func__, line);
        return APAC_FALSE;
    }

    eProfile->mldGrouping = strdup(tag);
    pos += len + 1;

    dprintf(MSG_ERROR, "%s: MLD Group : %s\n", __func__, eProfile->mldGrouping);

    return APAC_TRUE;
}

/**
 * @brief Handle a line for the AL-specific encryption profile file format.
 *
 * @param [in] buf  the line read from the config file
 * @param [in] line  the line number (for use in error messages)
 * @param [out] eProfileMatcher  the entry to populate
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
static apacBool_e apacHyfiMapParseAndStoreALSpecificEProfile(
    const char *buf, int line, apacMapEProfileMatcher_t *eProfileMatcher) {
    apacMapEProfile_t *eProfile = NULL;
    char tag[MAX_SSID_LEN];
    int len = 0;
    const char *pos = buf;

    eProfileMatcher->matcherType = APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC;
    eProfileMatcher->terminateMatching = APAC_FALSE;

    eProfile = &eProfileMatcher->typeParams.alParams.eprofile;

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract AL ID on line %d\n",
                __func__, line);
        return APAC_FALSE;
    } else {
        eProfileMatcher->typeParams.alParams.alId = strdup(tag);
        pos += len + 1;
    }

    len = apac_atf_config_line_getparam(pos,',',tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract operating class on line %d\n",
                __func__, line);
        return APAC_FALSE;
    } else {
        eProfileMatcher->typeParams.alParams.opclass = strdup(tag);
        pos += len + 1;
    }

    return apacHyfiMapParseAndStoreEProfile(pos, line, eProfile);
}

/**
 * @brief Parse a line that specifies the settings for a single SSID.
 *
 * @param [inout] map  the structure into which to place the associated data
 * @param [in] buf  the line read from the config file
 * @param [in] line  the line number (for use in error messages)
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
static apacBool_e apacHyfiMapParseAndStoreGenericEProfileSSID(apacMapData_t *map,
                                                              const char *buf, int line) {
    char tag[MAX_SSID_LEN];
    size_t len;
    const char *pos = buf;
    const char *ssidKey = NULL;

    apacMapEProfile_t *eProfile = NULL;

    while (isspace(*pos)) {
        pos++;
    }

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract SSID key on line %d\n", __func__, line);
        return APAC_FALSE;
    }

    ssidKey = strdup(tag);
    if (!ssidKey) {
        dprintf(MSG_ERROR, "%s: Failed to copy SSID key\n", __func__);
        return APAC_FALSE;
    }

    pos += len + 1;

    do {
        // Place this into the SSID table. We assume the file contains no
        // duplicates.
        if (map->ssidCnt == sizeof(map->eProfileSSID) / sizeof(map->eProfileSSID[0])) {
            dprintf(MSG_ERROR, "%s: Too many SSIDs specified (max=%u)\n", __func__,
                   map->ssidCnt);
            break;
        }

        eProfile = &map->eProfileSSID[map->ssidCnt].eprofile;
        if (apacHyfiMapParseAndStoreEProfile(pos, line, eProfile)) {
            dprintf(MSG_INFO, "%s: Storing SSID key '%s' at index %u ssid=%s\n", __func__,
                    ssidKey, map->ssidCnt, eProfile->ssid);

            map->eProfileSSID[map->ssidCnt].ssidKey = ssidKey;
            map->ssidCnt++;
            return APAC_TRUE;
        }

    } while (0);

    // If we reach here, there was an error. Perform cleanup.
    free((char *)ssidKey);
    return APAC_FALSE;
}

/**
 * @brief Search for the SSID key in the table, returning its index if found.
 *
 * @param [in] map  the structure storing the SSIDs
 * @param [in] key  the SSID key to lookup
 *
 * @return the index at which the SSID key is stored, or -1 if not found
 */
static ssize_t apacHyfiMapLookupSSIDKey(apacMapData_t *map, const char *key) {
    u8 i;
    for (i = 0; i < map->ssidCnt; ++i) {
        if (strcmp(map->eProfileSSID[i].ssidKey, key) == 0) {
            return i;
        }
    }

    // not found
    return -1;
}

/**
 * @brief Search for the MLO key in the table, returning its index if found.
 *
 * @param [in] map  the structure storing the MLOs
 * @param [in] key  the MLO key to lookup
 *
 * @return the index at which the MLO key is stored, or -1 if not found
 */
static ssize_t apacHyfiMapLookupMLOKey(apacMapData_t *map, const char *key) {
    u8 i;
    for (i = 0; i < map->mloCnt; ++i) {
        if (strcmp(map->eProfileMLO[i].mloKey, key) == 0) {
            return i;
        }
    }

    // not found
    return -1;
}

/**
 * @brief Create a single generic encryption profile policy from the template.
 *
 * @param [in] map  the structure storing the encryption profiles
 * @param [in] ssidKey  the key of the SSID to use for the profile to be stored
 * @param [in] matcherTemplate  the template matching parameters to use for
 *                              this profile matcher
 * @param [in] terminateMatching  whether to mark this entry as terminating the
 *                                matches
 * @param [inout] index  the next index to store
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
static apacBool_e apacHyfiMapStoreGenericEProfilePolicy(
    apacMapData_t *map, const char *ssidKey,
    struct apacMapEProfileMatcherGenericParams_t *matcherTemplate,
    apacBool_e terminateMatching, int *index) {
    int RadioIdx;
    // Find the matching SSID entry
    int ssidIndex = apacHyfiMapLookupSSIDKey(map, ssidKey);
    if (ssidIndex < 0) {
        dprintf(MSG_ERROR, "%s: Unknown SSID key '%s' specified\n", __func__, ssidKey);
        return APAC_FALSE;
    }

    if (*index == APAC_MAXNUM_NTWK_NODES) {
        dprintf(MSG_ERROR,
                "%s: Cannot store all encryption profiles; only %d supported\n", __func__,
                APAC_MAXNUM_NTWK_NODES);
        return APAC_FALSE;
    }

    // Finalize the matcher entry and store it in the complete array
    matcherTemplate->ssidIndex = ssidIndex;
    map->eProfileMatcher[*index].matcherType = APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC;
    map->eProfileMatcher[*index].terminateMatching = terminateMatching;

    RadioIdx = apacMapGetRadioIdxByOpclass(map, matcherTemplate->numOpClassRanges,
            matcherTemplate->opClassRanges[0].minOpClass, matcherTemplate->opClassRanges[0].maxOpClass,
            matcherTemplate->bandType);
    matcherTemplate->mibVAPIndex = apacMibGetVapIdxbySSID(RadioIdx,
            map->eProfileSSID[ssidIndex].eprofile.ssid);

    map->eProfileMatcher[*index].typeParams.genericParams = *matcherTemplate;

    (*index)++;
    return APAC_TRUE;
}

/**
 * @brief Update MLO capability to a generic encryption profile policy
 *        from the template.
 *
 * @param [in] map  the structure storing the encryption profiles
 * @param [in] mloKey  the key of the MLO to use for the profile to be stored
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
static apacBool_e apacHyfiMapStoreGenericEProfileMLOPolicy(
    apacMapData_t *map, const char *mloKey,
    struct apacMapEProfileMatcherGenericParams_t *matcherTemplate) {
    int ssidIndex, mloIndex, i, j, foundOpClassRanges = 0;
    apacMapEProfileMatcher_t *eProfileMatcher = NULL;

    mloIndex = apacHyfiMapLookupMLOKey(map, mloKey);
    if (mloIndex < 0) {
        dprintf(MSG_ERROR, "%s: Unknown MLO key '%s' specified\n", __func__, mloKey);
        return APAC_FALSE;
    }

    while (map->eProfileMLO[mloIndex].ssidKey) {
        if (strcmp(map->eProfileMLO[mloIndex].mloKey, mloKey)) {
            mloIndex++;
            continue;
        }
        // Find the matching SSID entry
        ssidIndex = apacHyfiMapLookupSSIDKey(map, map->eProfileMLO[mloIndex].ssidKey);
        if (ssidIndex < 0) {
            dprintf(MSG_ERROR, "%s: Unknown SSID key '%s' specified\n", __func__,
                    map->eProfileMLO[mloIndex].ssidKey);
            return APAC_FALSE;
        }

        for (i = 0; i < APAC_MAXNUM_NTWK_NODES; i++) {
            eProfileMatcher = &map->eProfileMatcher[i];

            if ( ssidIndex == eProfileMatcher->typeParams.genericParams.ssidIndex &&
                    eProfileMatcher->typeParams.genericParams.numOpClassRanges ==
                    matcherTemplate->numOpClassRanges) {
                    foundOpClassRanges = 0;
                for (j=0; j < matcherTemplate->numOpClassRanges; j++) {
                    if((eProfileMatcher->typeParams.genericParams.opClassRanges[j].minOpClass ==
                                matcherTemplate->opClassRanges[j].minOpClass) &&
                            (eProfileMatcher->typeParams.genericParams.opClassRanges[j].maxOpClass ==
                             matcherTemplate->opClassRanges[j].maxOpClass)) {
                        foundOpClassRanges++;
                    }
                }

                if (foundOpClassRanges == matcherTemplate->numOpClassRanges &&
                    eProfileMatcher->typeParams.genericParams.bandType == matcherTemplate->bandType) {
                    eProfileMatcher->typeParams.genericParams.mloEnabled = APAC_TRUE;
                    return APAC_TRUE;
                }
            }
        }
        mloIndex++;
    }

    return APAC_TRUE;
}

/**
 * @brief Parse a line that specifies the matching parameters for SSIDs
 *        to instantiate.
 * @param [inout] map  the structure into which to place the associated data
 * @param [in] buf  the line read from the config file
 * @param [in] line  the line number (for use in error messages)
 * @param [inout] index  the next available encryption profile matcher index;
 *                       this will be updated for each profile stored
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
static apacBool_e apacHyfiMapParseAndStoreGenericEProfilePolicy(apacMapData_t *map,
                                                                const char *buf, int line,
                                                                int *index) {
    char tag[MAX_SSID_LEN];
    int len;
    const char *pos = buf;
    unsigned long val;
    u8 i;

    struct apacMapEProfileMatcherGenericParams_t matcherTemplate = {0};

    while (isspace(*pos)) {
        pos++;
    }

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR,
                "%s: Could not extract number of operating class ranges on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }

    if (!apacHyfiMapParseInt(tag, 0, "number of operating class ranges", line, &val)) {
        return APAC_FALSE;
    }

    matcherTemplate.numOpClassRanges = val;
    pos += len + 1;

    for (i = 0; i < matcherTemplate.numOpClassRanges; ++i) {
        // Min operating class parameter
        len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
        if (len <= 0) {
            dprintf(MSG_ERROR,
                    "%s: Could not extract min operating class on line %d\n",
                    __func__, line);
            return APAC_FALSE;
        }

        if (!apacHyfiMapParseInt(tag, 0, "min operating class", line, &val)) {
            return APAC_FALSE;
        }

        matcherTemplate.opClassRanges[i].minOpClass = val;
        pos += len + 1;

        // Max operating class parameter
        len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
        if (len <= 0) {
            dprintf(MSG_ERROR,
                    "%s: Could not extract max operating class on line %d\n",
                    __func__, line);
            return APAC_FALSE;
        }

        if (!apacHyfiMapParseInt(tag, 0, "max operating class", line, &val)) {
            return APAC_FALSE;
        }

        matcherTemplate.opClassRanges[i].maxOpClass = val;
        pos += len + 1;
    }

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR,
                "%s: Could not extract Band Type on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }

    apacHyfi20GetBandTypeFromPolicyConfig(tag, &matcherTemplate.bandType);
    dprintf(MSG_INFO,"%s: Band type detected %s and value=%d\n", __func__, tag, matcherTemplate.bandType);

    pos += len + 1;

    // Now keep extracting SSIDs until we run out of space or find a
    // non-matching one
    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    while (len > 0) {  // more to follow
        if (!apacHyfiMapStoreGenericEProfilePolicy(
                map, tag, &matcherTemplate, APAC_FALSE /* terminateMatching */, index)) {
            return APAC_FALSE;
        }

        pos += len + 1;
        len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    }

    // Last entry
    return apacHyfiMapStoreGenericEProfilePolicy(
        map, pos, &matcherTemplate, APAC_TRUE /* terminateMatching */, index);
}

/**
 * @brief Parse a line that specifies the matching parameters for MLO
 *        to instantiate.
 * @param [inout] map  the structure into which to place the associated data
 * @param [in] buf  the line read from the config file
 * @param [in] line  the line number (for use in error messages)
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
static apacBool_e apacHyfiMapParseAndStoreGenericEProfileMLOPolicy(apacMapData_t *map,
                                                            const char *buf, int line) {
    char tag[MAX_SSID_LEN];
    int len;
    const char *pos = buf;
    unsigned long val;
    u8 i;
    struct apacMapEProfileMatcherGenericParams_t matcherTemplate = {0};

    if (!apacHyfiMapIsMLOEnabled(map))
        return APAC_TRUE;

    while (isspace(*pos)) {
        pos++;
    }

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR,
                "%s: Could not extract number of operating class ranges on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }

    if (!apacHyfiMapParseInt(tag, 0, "number of operating class ranges", line, &val)) {
        return APAC_FALSE;
    }

    matcherTemplate.numOpClassRanges = val;
    pos += len + 1;

    for (i = 0; i < matcherTemplate.numOpClassRanges; ++i) {
        // Min operating class parameter
        len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
        if (len <= 0) {
            dprintf(MSG_ERROR,
                    "%s: Could not extract min operating class on line %d\n",
                    __func__, line);
            return APAC_FALSE;
        }

        if (!apacHyfiMapParseInt(tag, 0, "min operating class", line, &val)) {
            return APAC_FALSE;
        }

        matcherTemplate.opClassRanges[i].minOpClass = val;
        pos += len + 1;

        // Max operating class parameter
        len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
        if (len <= 0) {
            dprintf(MSG_ERROR,
                    "%s: Could not extract max operating class on line %d\n",
                    __func__, line);
            return APAC_FALSE;
        }

        if (!apacHyfiMapParseInt(tag, 0, "max operating class", line, &val)) {
            return APAC_FALSE;
        }

        matcherTemplate.opClassRanges[i].maxOpClass = val;

        pos += len + 1;
    }

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR,
                "%s: Could not extract Band Type on line %d\n",
                __func__, line);
        return APAC_FALSE;
    }

    apacHyfi20GetBandTypeFromPolicyConfig(tag, &matcherTemplate.bandType);
    dprintf(MSG_INFO,"%s: Band type detected %s and value=%d\n", __func__, tag, matcherTemplate.bandType);

    pos += len + 1;

    // Now keep extracting SSIDs until we run out of space or find a
    // non-matching one
    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    while (len > 0) {  // more to follow
        if (!apacHyfiMapStoreGenericEProfileMLOPolicy(map, tag, &matcherTemplate)) {
            return APAC_FALSE;
        }

        pos += len + 1;
        len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    }

    // Last entry
    return apacHyfiMapStoreGenericEProfileMLOPolicy(map, pos, &matcherTemplate);

}

/**
 * @brief Parse a line that specifies the matching parameters for MLO
 *        to instantiate.
 * @param [inout] map  the structure into which to place the associated data
 * @param [in] buf  the line read from the config file
 * @param [in] line  the line number (for use in error messages)
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
static apacBool_e apacHyfiMapParseAndStoreGenericEProfileMLO(apacMapData_t *map,
                                                            const char *buf, int line) {
    char tag[MAX_SSID_LEN];
    const char deLimiter[2] = ",";
    size_t len;
    const char *pos = buf;
    char *mloKey = NULL;
    char *ssidKey = NULL;
    char *token=NULL, *savePtr=NULL;

    if (!apacHyfiMapIsMLOEnabled(map))
        return APAC_TRUE;

    while (isspace(*pos)) {
        pos++;
    }

    len = apac_atf_config_line_getparam(pos, ',', tag, sizeof(tag));
    if (len <= 0) {
        dprintf(MSG_ERROR, "%s: Could not extract MLO key on line %d\n", __func__, line);
        return APAC_FALSE;
    }

    mloKey = strdup(tag);
    if (!mloKey) {
        dprintf(MSG_ERROR, "%s: Failed to copy MLO key\n", __func__);
        return APAC_FALSE;
    }

    pos += len + 1;

    do {
        // Place this into the SSID table. We assume the file contains no
        // duplicates.
        if (map->mloCnt >= sizeof(map->eProfileMLO) / sizeof(map->eProfileMLO[0])) {
            dprintf(MSG_ERROR, "%s: Too many MLO specified (max=%u)\n", __func__,
                   map->mloCnt);
            break;
        }

        ssidKey = strdup(pos);
        if (!ssidKey) {
            dprintf(MSG_ERROR, "%s: Failed to copy SSID key\n", __func__);
            free((char *)mloKey);
            return APAC_FALSE;
        }

        dprintf(MSG_INFO, "%s: Found MLO and SSID key pair (%s,%s)\n", __func__, mloKey, ssidKey);

        token = strtok_r(ssidKey, deLimiter, &savePtr);
        while (token != NULL) {
            map->eProfileMLO[map->mloCnt].mloKey = mloKey;
            map->eProfileMLO[map->mloCnt].ssidKey = token;
            map->mloCnt++;
            token = strtok_r(NULL, deLimiter, &savePtr);
        }

        return APAC_TRUE;

    } while (0);

    // If we reach here, there was an error. Perform cleanup.
    if (mloKey) {
        free((char *)mloKey);
    }
    if (ssidKey) {
        free((char *)ssidKey);
    }
    return APAC_FALSE;


}

/**
 * @brief Handle a line for the generic encryption profile file format.
 *
 * @param [inout] map  the structure into which to place the associated data
 * @param [in] buf  the line read from the config file
 * @param [in] line  the line number (for use in error messages)
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
static apacBool_e apacHyfiMapParseAndStoreGenericEProfile(apacMapData_t *map,
                                                          const char *buf, int line,
                                                          int *index) {
    // First determine the type of the line
    const char *ssidTag = "SSID:";
    const char *policyTag = "Generic-Policy:";
#if DOT1X_SUPPORT
    const char *TagDot1x = "8021X:";
#endif
    const char *mloTag = "MLO:";
    const char *mloPolicyTag = "MLO-Policy:";
    if (strncmp(buf, ssidTag, strlen(ssidTag)) == 0) {
        return apacHyfiMapParseAndStoreGenericEProfileSSID(map, buf + strlen(ssidTag),
                                                           line);
    } else if (strncmp(buf, policyTag, strlen(policyTag)) == 0) {
        return apacHyfiMapParseAndStoreGenericEProfilePolicy(map, buf + strlen(policyTag),
                                                             line, index);
#if DOT1X_SUPPORT
    } else if (strncmp(buf, TagDot1x, strlen(TagDot1x)) == 0) {
        return apacHyfiMapParseAndStoreGenericEProfileDot1x(map, buf + strlen(TagDot1x),
                                                            line);
#endif
    } else if (strncmp(buf, mloTag, strlen(mloTag)) == 0) {
        return apacHyfiMapParseAndStoreGenericEProfileMLO(map, buf + strlen(mloTag),
                                                            line);
    } else if (strncmp(buf, mloPolicyTag, strlen(mloPolicyTag)) == 0) {
        return apacHyfiMapParseAndStoreGenericEProfileMLOPolicy(map,
                                buf + strlen(mloPolicyTag), line);
    } else {
        dprintf(MSG_ERROR, "%s: Unexpected line: '%s'\n", __func__, buf);
        return APAC_FALSE;
    }
}

int apacHyfiMapStoreRadioChannel(apacMapData_t *map)
{
    apacHyfi20Data_t *hyfi20;
    apacHyfi20IF_t  *hif;
    int i=0,len;

    apacHyfi20TRACE();
    hyfi20 = MAPToHYFI20(map);
    hif=hyfi20->hyif;
    len=strlen("wifi0");

    while(i < APAC_MAXNUM_HYIF && hif[i].valid) {
        if(strncmp("wifi0",hif[i].radioName,len)==0 && hif[i].channel!=0) {
            map->CurrentRadioOpChannel[0] = hif[i].channel;
            map->CurrentRadioFreq[0] = hif[i].wifiFreq;
            map->CurrentRadioBandType[0] = apacHyfi20GetLocalBandFromRadioName(hif[i].radioName,hif[i].wifiFreq);
        } else if (strncmp("wifi1",hif[i].radioName,len)==0 && hif[i].channel!=0) {
            map->CurrentRadioOpChannel[1] = hif[i].channel;
            map->CurrentRadioFreq[1] = hif[i].wifiFreq;
            map->CurrentRadioBandType[1] = apacHyfi20GetLocalBandFromRadioName(hif[i].radioName,hif[i].wifiFreq);
        } else if (strncmp("wifi2",hif[i].radioName,len)==0 && hif[i].channel!=0) {
            map->CurrentRadioOpChannel[2] = hif[i].channel;
            map->CurrentRadioFreq[2] = hif[i].wifiFreq;
            map->CurrentRadioBandType[2] = apacHyfi20GetLocalBandFromRadioName(hif[i].radioName,hif[i].wifiFreq);
        } else if (strncmp("wifi3",hif[i].radioName,len)==0 && hif[i].channel!=0) {
            map->CurrentRadioOpChannel[3] = hif[i].channel;
            map->CurrentRadioFreq[3] = hif[i].wifiFreq;
            map->CurrentRadioBandType[3] = apacHyfi20GetLocalBandFromRadioName(hif[i].radioName,hif[i].wifiFreq);
        } else if (strncmp("wifi4",hif[i].radioName,len)==0 && hif[i].channel!=0) {
            map->CurrentRadioOpChannel[4] = hif[i].channel;
            map->CurrentRadioFreq[4] = hif[i].wifiFreq;
            map->CurrentRadioBandType[4] = apacHyfi20GetLocalBandFromRadioName(hif[i].radioName,hif[i].wifiFreq);
        }

        i++;
    }
    if (map->map6GEnabled) {
        dprintf(MSG_INFO,"%s: CurrentRadioOpChannel: wifi0:%d wifi1:%d wifi2:%d wifi3:%d wifi4:%d\n",
                __func__, map->CurrentRadioOpChannel[0], map->CurrentRadioOpChannel[1],
                map->CurrentRadioOpChannel[2], map->CurrentRadioOpChannel[3], map->CurrentRadioOpChannel[4]);
        dprintf(MSG_INFO,"%s: CurrentRadioFreq: wifi0:%d wifi1:%d wifi2:%d wifi3:%d wifi4:%d\n",
                __func__, map->CurrentRadioFreq[0], map->CurrentRadioFreq[1],
                map->CurrentRadioFreq[2], map->CurrentRadioFreq[3],
                map->CurrentRadioFreq[4]);
        dprintf(MSG_INFO,"%s: CurrentRadioBandType: wifi0:%d wifi1:%d wifi2:%d wifi3:%d wifi4:%d\n",
                __func__, map->CurrentRadioBandType[0], map->CurrentRadioBandType[1],
                map->CurrentRadioBandType[2], map->CurrentRadioBandType[3],
                map->CurrentRadioBandType[4]);


    } else {
        dprintf(MSG_INFO,"%s: CurrentRadioOpChannel: wifi0:%d wifi1:%d wifi2:%d wifi3:%d wifi4:%d",
                __func__, map->CurrentRadioOpChannel[0], map->CurrentRadioOpChannel[1],
                map->CurrentRadioOpChannel[2], map->CurrentRadioOpChannel[3],
                map->CurrentRadioOpChannel[4]);
    }
    return 0;
}

/**
 * @brief Resolve the encryption profiles from the config file.
 *
 * @param [inout] map  the structure into which to place the encryption profiles
 * @param [in] matcherType  the format of the profiles within the file
 * @param [in] fname  the name of the file to read
 *
 * @return APAC_TRUE on success; otherwise APAC_FALSE
 */
static apacBool_e apacHyfiMapParseAndStoreConfig(apacMapData_t *map,
                                                 apacMapEProfileMatcherType_e matcherType,
                                                 const char *fname,
                                                 const char *bstafname) {
    FILE *f = NULL;
    FILE *file = NULL;
    char buf[256] = {0};
    int index = 0;
    int index1 = 0;
    int line = 1;
    int errors = 0;
    apacMapEProfileMatcher_t *eProfileMatcher = NULL;

    apacHyfi20TRACE();

    apacHyfiMapStoreRadioChannel(map); //Store the current op-channel of each radio
    int lock_fd = open(APAC_LOCK_FILE_PATH, O_RDONLY);
    if (lock_fd < 0) {
        dprintf(MSG_ERROR, "Failed to open lock file %s\n", APAC_LOCK_FILE_PATH);
        return APAC_FALSE;
    }

    if (flock(lock_fd, LOCK_EX) == -1) {
        dprintf(MSG_ERROR, "Failed to flock lock file %s\n", APAC_LOCK_FILE_PATH);
        close(lock_fd);
        return APAC_FALSE;
    }

    dprintf(MSG_INFO, "Reading Map 1.0 configuration file %s ...\n", fname);

    f = fopen(fname, "r");

    if (f == NULL) {
        dprintf(MSG_ERROR,
                "Could not open configuration file '%s' for reading.\n",
                fname);
        if (flock(lock_fd, LOCK_UN) == 1) {
            dprintf(MSG_ERROR, "Failed to unlock file %s\n", APAC_LOCK_FILE_PATH);
            errors++;
        }
        close(lock_fd);
        return APAC_FALSE;
    }

    while (fgets(buf, sizeof(buf), f) != NULL) {
        // remove the trailing carriage return and/or newline
        buf[strcspn(buf, "\r\n")] = '\0';

        if (strlen(buf) == 0) {
            continue;
        }
        eProfileMatcher = &map->eProfileMatcher[index];

        if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == g_map_cfg_file_format) {
            if (!apacHyfiMapParseAndStoreALSpecificEProfile(buf, line, eProfileMatcher)) {
                errors++;
                break;
            }

            index++;
        } else if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC == g_map_cfg_file_format) {
            if (!apacHyfiMapParseAndStoreGenericEProfile(map, buf, line, &index)) {
                errors++;
                break;
            }
        } else {
            dprintf(MSG_ERROR, "%s: Unexpected encryption profile matcher type: %d\n",
                    __func__, g_map_cfg_file_format);
            if (flock(lock_fd, LOCK_UN) == 1) {
                dprintf(MSG_ERROR, "Failed to unlock file %s\n", APAC_LOCK_FILE_PATH);
                errors++;
            }
            close(lock_fd);
            fclose(f);
            return APAC_FALSE;
        }

        if (index >= APAC_MAXNUM_NTWK_NODES) {
            dprintf(MSG_ERROR,
                    "%s: Cannot store all encryption profiles; only %d supported\n",
                    __func__, APAC_MAXNUM_NTWK_NODES);
            break;
        }

        line++;
        memset(buf, 0x00, sizeof(buf));
    }

    dprintf(MSG_DEBUG, "Reading Map bsta configuration file %s ...\n", bstafname);

    line = 1;
    file = fopen("/etc/config/map_bsta.conf", "r");

    if (file == NULL) {
        dprintf(MSG_ERROR,
                "Could not open configuration file '%s' for reading.\n",
                bstafname);
        goto out;
    }

    while (fgets(buf, sizeof(buf), file) != NULL) {
        // remove the trailing carriage return and/or newline
        buf[strcspn(buf, "\r\n")] = '\0';

        if (strlen(buf) == 0) {
            continue;
        }
        eProfileMatcher = &map->eBstaProfileMatcher[index1];

        if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == g_map_cfg_file_format) {
            if (!apacHyfiMapParseAndStoreALSpecificEBstaSettings(buf, line, eProfileMatcher)) {
                errors++;
                break;
            }
            index1++;
        } else {
            dprintf(MSG_ERROR, "%s: Unexpected encryption profile matcher type: %d\n",
                    __func__, g_map_cfg_file_format);
            if (flock(lock_fd, LOCK_UN) == 1) {
                dprintf(MSG_ERROR, "Failed to unlock file %s\n", APAC_LOCK_FILE_PATH);
                errors++;
            }
            fclose(file);
            goto out;
        }

        if (index1 >= APAC_MAXNUM_NTWK_NODES) {
            dprintf(MSG_ERROR,
                    "%s: Cannot store all bsta mlds; only %d supported\n",
                    __func__, APAC_MAXNUM_NTWK_NODES);
            break;
        }

        line++;
        memset(buf, 0x00, sizeof(buf));
    }

out:
    if (flock(lock_fd, LOCK_UN) == 1) {
        dprintf(MSG_ERROR, "Failed to unlock file %s\n", APAC_LOCK_FILE_PATH);
        errors++;
    }

    map->eProfileCnt = index;
    map->eBstaMldCnt = index1;
    dprintf(MSG_INFO, "%s: Read %u MAP encryption profiles\n", __func__,
            map->eProfileCnt);
    close(lock_fd);
    fclose(f);

    if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == g_map_cfg_file_format) {
        if (map->eProfileCnt == 0 && errors == 0) {
            dprintf(MSG_INFO, "%s: No Profiles found in configuration file '%s', Tear Down \n ",
                    __func__, fname);
            return APAC_TRUE;
        }
    }

    if (errors) {
        dprintf(MSG_ERROR,
                "%s: %d errors found in configuration file '%s'\n",
                __func__, errors, fname);
    }

    return (errors == 0);
}

apacBool_e apacHyfiMapDInit(apacMapData_t *map) {
    u8 i = 0;
    apacMapEProfileMatcher_t *eProfileMatcher = NULL;
    apacMapEProfile_t *eProfile = NULL;

    apacHyfi20TRACE();

    for (i = 0; i < map->eProfileCnt; i++) {
        eProfileMatcher = &map->eProfileMatcher[i];
        if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC ==
            eProfileMatcher->matcherType) {
            eProfile = &eProfileMatcher->typeParams.alParams.eprofile;

            free((char *)eProfileMatcher->typeParams.alParams.alId);
            free((char *)eProfileMatcher->typeParams.alParams.opclass);

            free((char *)eProfile->ssid);
            free((char *)eProfile->nw_key);
        }
    }
    map->eProfileCnt = 0;

    for (i = 0; i < map->ssidCnt; i++) {
        free((char *) map->eProfileSSID[i].ssidKey);
        free((char *) map->eProfileSSID[i].eprofile.ssid);
        free((char *) map->eProfileSSID[i].eprofile.nw_key);
    }
    map->ssidCnt = 0;

    for(i = 0; i < map->mloCnt; i++) {
        free((char *) map->eProfileMLO[i].ssidKey);
        free((char *) map->eProfileMLO[i].mloKey);
    }
    map->mloCnt = 0;

    return APAC_TRUE;
}

int convertStrToArray(char *str, char *array, int length)
{
    int i = 0;
    char *token;

    if (!str || !array)
       return 0;

    char *rest = (char *)str;
    while ((token = strtok_r(rest, " ", &rest))) {
        token[strcspn(token, "\r\n")] = 0;
        if( i < length ) {
            int num = (int)strtol(token, NULL, 16);
            array[i++] = num;
        }
    }
    return i;
}

/**
 * @brief read Basic capability from conf file that matches key and
 * returns the length of array which contains hex bytes
 *
 * @param [in] filename name of conf file [wsplcd-lan.conf]
 *
 * @param [out] lineptr poniter to array of hex bytes. Caller off this
 *              function needs to release this memeory.
 *
 * @param [in] key name of key in conf file (key=value)
 *
 * @return length of the bytes array
 */
int readLineFromFile(char *filename, char **lineptr, char *key)
{
    FILE * fp = fopen(filename, "r");
    char *line = NULL;
    char  *linerslt = NULL;
    size_t length = 0;
    ssize_t read = 0;
    int bytesCnt = 0;

    if (!fp) {
        perror("basic capability file: ");
        return -1;
    }
    fseek(fp, 0L, SEEK_SET);
    while ((read = getline(&line, &length, fp)) != -1) {
        if(!strncmp(key, line, strlen(key))) {
            linerslt = line + strlen(key)+1;
            break;
        }
    }
    char *basic = malloc(read);
    bytesCnt = convertStrToArray(linerslt, basic, read);
    *lineptr = basic;

    if(line)
        free(line);
    fclose(fp);
    return bytesCnt;
}

int apacGetPIFMapCap(apacHyfi20Data_t *pData) {
    apacPifMap_t *pIFMapData = NULL;
    int32_t Sock;
    struct iwreq iwr;
    struct ifreq ifr;
    struct mesh_dbg_req_t mesh_req = {0};
    int j, i = 0, k = 0;
    apacHyfi20AP_t *pAP = &pData->ap[0];
    struct ether_addr radioAddr;

    apacHyfi20TRACE();

    for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {

        if (!pAP[i].valid) {
            continue;
        }

        if (!pAP[i].ifName) {
            dprintf(MSG_ERROR, "%s - Invalid arguments: ifName is NULL", __func__);
            goto out;
        }

        dprintf(MSG_ERROR, "%s  %s \n", __func__, pAP[i].ifName);

        if ((Sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            dprintf(MSG_ERROR, "%s: Create ioctl socket failed!", __func__);
            goto out;
        }

        if (fcntl(Sock, F_SETFL, fcntl(Sock, F_GETFL) | O_NONBLOCK)) {
            dprintf(MSG_ERROR, "%s: fcntl() failed", __func__);
            goto err;
        }

        pIFMapData = &pAP[i].pIFMapData;
        strlcpy(iwr.ifr_name, pAP[i].ifName, IFNAMSIZ);
        strlcpy(ifr.ifr_name, pAP[i].radioName, IFNAMSIZ);
        iwr.u.data.pointer = (void *)&mesh_req;
        iwr.u.data.length = (sizeof(struct mesh_dbg_req_t));
        mesh_req.mesh_cmd =  MESH_MAP_OPCLASS_FOR_CUR_HWMODE;
        mesh_req.needs_reply = DBGREQ_REPLY_IS_REQUIRED;

        if (!wlanIfWd) {
            dprintf(MSG_ERROR, "%s - wlanif nl init failed\n", __func__);
            goto err;
        }

        if (strlen(pAP[i].ifName) &&
            getMeshreq_cfg80211(wlanIfWd->ctx, pAP[i].ifName, (void *)&mesh_req, (sizeof(struct mesh_dbg_req_t))) < 0)  {
            dprintf(MSG_ERROR, "%s: ioctl() failed, ifName: %s\n", __func__, pAP[i].ifName);
            goto err;
        }

        if (ioctl(Sock, SIOCGIFHWADDR, &ifr) < 0) {
            goto err;
        }

        os_memcpy(radioAddr.ether_addr_octet, ifr.ifr_hwaddr.sa_data, 6);
        os_memcpy(pAP[i].radio_mac, radioAddr.ether_addr_octet, 6);

        if (pAP[i].radioName && !strlen(pAP[i].ifName)) {
            int read=0;
            char *basic = NULL; // use it for pointing to array of hex bytes
            int x = 0;
            int y = 0;
            int offset = 0;

            if(APAC_WIFI_FREQ_2 == pAP[i].freq)
                read = readLineFromFile(g_cfg_file, &basic, "2GBasic");
            else if(APAC_WIFI_FREQ_5 == pAP[i].freq || APAC_WIFI_FREQ_5_OTHER == pAP[i].freq)
                read = readLineFromFile(g_cfg_file, &basic, "5GBasic");
            if (!read || !basic) {
                if(basic)
                   free(basic); // free it if read is zero
                continue;
            }

            dprintf(MSG_INFO, "Length of basic cap bytes = %d\n", read);
            pIFMapData->apcap.hwcap.max_supported_bss = (u_int8_t)basic[0];
            pIFMapData->apcap.hwcap.num_supported_op_classes = (u_int8_t)basic[1];
            dprintf(MSG_INFO, "max_supported_bss = %02x, num_supported_op_classes = %02x\n", basic[0], basic[1]);
            offset = 2; // max_supported_bss+num_supported_op_classes

            for (x = 0; x < basic[1]; x++) {
                pIFMapData->apcap.hwcap.opclasses[x].opclass = (u_int8_t)basic[offset+0];

                pIFMapData->apcap.hwcap.opclasses[x].max_tx_pwr_dbm = (int8_t)basic[offset+1];

                pIFMapData->apcap.hwcap.opclasses[x].num_non_oper_chan = (u_int8_t)basic[offset+2];
                dprintf(MSG_INFO, "%d opclass = %02x, max_tx_pwr_dbm = %d, num_non_oper_chan = %02x\n",
                            x, basic[offset+0], (int8_t)basic[offset+1], basic[offset+2]);

                for(y = 0; y < basic[offset+2]; y++) {
                    dprintf(MSG_INFO, "non_oper_chan_num[%d] = %02x\n", y, basic[offset+3+y]);
                    pIFMapData->apcap.hwcap.opclasses[x].non_oper_chan_num[y] = (u_int8_t)basic[offset+3+y];
                }
                offset += 3+basic[offset+2];  // 3 for opclass+max_tx_pwr_dbm+num_non_oper_chan
                dprintf(MSG_INFO, "offset = %d\n", offset);
            }
            if (basic) // our responsibility to free the memory
                free(basic);
        }
        else {
            pIFMapData->apcap.hwcap.max_supported_bss = mesh_req.mesh_data.mapapcap.hwcap.max_supported_bss;
            pIFMapData->apcap.hwcap.num_supported_op_classes =
                mesh_req.mesh_data.mapapcap.hwcap.num_supported_op_classes;

            /// Very rarely we saw this case driver is returning back
            /// ioctl as success but operating classe as Zero
            /// making more stringent error checks now
            if (!mesh_req.mesh_data.mapapcap.hwcap.num_supported_op_classes) goto err;

            for (k = 0; k < mesh_req.mesh_data.mapapcap.hwcap.num_supported_op_classes; k++) {
                pIFMapData->apcap.hwcap.opclasses[k].opclass =
                    mesh_req.mesh_data.mapapcap.hwcap.opclasses[k].opclass;
                pIFMapData->apcap.hwcap.opclasses[k].max_tx_pwr_dbm =
                    mesh_req.mesh_data.mapapcap.hwcap.opclasses[k].max_tx_pwr_dbm;
                pIFMapData->apcap.hwcap.opclasses[k].num_non_oper_chan =
                    mesh_req.mesh_data.mapapcap.hwcap.opclasses[k].num_non_oper_chan;

                dprintf(MSG_INFO, "Operating class  %d Txpwer %x Number of unoperable channel %d\n",
                        pIFMapData->apcap.hwcap.opclasses[k].opclass,
                        pIFMapData->apcap.hwcap.opclasses[k].max_tx_pwr_dbm,
                pIFMapData->apcap.hwcap.opclasses[k].num_non_oper_chan);

                for (j = 0; j < pIFMapData->apcap.hwcap.opclasses[k].num_non_oper_chan; j++) {
                    pIFMapData->apcap.hwcap.opclasses[k].non_oper_chan_num[j] =
                        mesh_req.mesh_data.mapapcap.hwcap.opclasses[k].non_oper_chan_num[j];
                    dprintf(MSG_INFO, "Unoperable channel list %d \n",
                            pIFMapData->apcap.hwcap.opclasses[k].non_oper_chan_num[j]);
                }
            }
        }
        // Warning:Types are assumed to be identical and thus any change made in the driver,
        // needs to be reflected in the user space type and vice versa.
        if (mesh_req.mesh_data.mapapcap.map_ap_ht_capabilities_valid) {
            memcpy(&pIFMapData->aphtcap, &mesh_req.mesh_data.mapapcap.htcap,
                sizeof(map_ap_ht_capabilities_t));
            pIFMapData->htCapabilitiesValid = APAC_TRUE;
        }

        if (mesh_req.mesh_data.mapapcap.map_ap_vht_capabilities_valid) {
            memcpy(&pIFMapData->apvhtcap, &mesh_req.mesh_data.mapapcap.vhtcap,
                sizeof(map_ap_vht_capabilities_t));
            pIFMapData->vhtCapabilitiesValid = APAC_TRUE;
        }

        if (mesh_req.mesh_data.mapapcap.map_ap_he_capabilities_valid) {
            memcpy(&pIFMapData->aphecap, &mesh_req.mesh_data.mapapcap.hecap,
                sizeof(map_ap_he_capabilities_t));
            pIFMapData->heCapabilitiesValid = APAC_TRUE;
        }

        if (mesh_req.mesh_data.mapapcap.map_ap_wifi6_capabilites_valid) {
            memcpy(&pIFMapData->apwifi6cap, &mesh_req.mesh_data.mapapcap.wifi6cap,
                sizeof(map_ap_wifi6_capabilities_t));
            pIFMapData->wifi6CapabilitiesValid = APAC_TRUE;
        }
        close(Sock);
    }

    return 0;
err:
    close(Sock);
out:
    return -1;
}

apacBool_e apacHyfiMapInit(apacMapData_t *map) {
    apacHyfi20Data_t *hyfi20;
    apacHyfi20MapVersion_e vEnabled;

    apacHyfi20TRACE();

    hyfi20 = MAPToHYFI20(map);

    if (hyfi20->config.role != APAC_REGISTRAR) {
        dprintf(MSG_INFO, "%s: not registrar! nothing to do\n", __func__);
        return APAC_TRUE;
    }

    vEnabled = apacHyfiMapIsEnabled(map);

    if (vEnabled)
        return apacHyfiMapParseAndStoreConfig(map, g_map_cfg_file_format, g_map_cfg_file, g_map_cfg_bsta_file);
    else
        return APAC_TRUE;  // to bail out init process without MAP
}

/**
 * @brief Dump the parameters for an encryption profile.
 *
 * @param [in] prefix  the string prefix to use prior to the encryption
 *                     profile parameters
 * @param [in] eProfile  the encryption profile to dump
 */
static void apacHyfiMapConfigDumpEProfile(const char *prefix,
                                          const apacMapEProfile_t *eProfile) {
    dprintf(MSG_MSGDUMP, "%s%s,0x%04x,0x%04x,%s,%d,%d,%d,%d,%d,%d,%d\n", prefix, eProfile->ssid,
            eProfile->auth, eProfile->encr, eProfile->nw_key, eProfile->isBackhaul,
            eProfile->isFronthaul, eProfile->map1bSTAAssocDisallowed,
            eProfile->map2bSTAAssocDisallowed, eProfile->primaryVlanID, eProfile->pcp,
            eProfile->vlanID);
}

void apacHyfiMapConfigDump(const apacMapData_t *map) {
    u8 i = 0, j = 0;
    const apacMapEProfileMatcher_t *eProfileMatcher = NULL;
    const apacMapEProfile_t *eProfile = NULL;
    const struct apacMapEProfileMatcherGenericParams_t *genericParams = NULL;
    apacHyfi20Data_t *hyfi20;
    apacHyfi20TRACE();

    hyfi20 = MAPToHYFI20(map);

    if (hyfi20->config.role != APAC_REGISTRAR) {
        dprintf(MSG_ERROR, "%s, not registrar!\n", __func__);
        return;
    }

    for (i = 0; i < map->eProfileCnt; i++) {
        eProfileMatcher = &map->eProfileMatcher[i];
        dprintf(MSG_INFO, "Profile #%u type = %u terminate = %u\n", i,
                eProfileMatcher->matcherType, eProfileMatcher->terminateMatching);

        if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC ==
            eProfileMatcher->matcherType) {
            eProfile = &eProfileMatcher->typeParams.alParams.eprofile;

            dprintf(MSG_INFO, "AL-specific EProfile: %s,%s\n",
                    eProfileMatcher->typeParams.alParams.alId,
                    eProfileMatcher->typeParams.alParams.opclass);
            apacHyfiMapConfigDumpEProfile("  ", eProfile);
        } else if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC ==
                   eProfileMatcher->matcherType) {
            genericParams = &eProfileMatcher->typeParams.genericParams;
            dprintf(MSG_INFO,
                    "Generic Profile #%u: numOpClassRanges=%u terminateMatching=%u\n", i,
                    genericParams->numOpClassRanges, eProfileMatcher->terminateMatching);

            for (j = 0; j < genericParams->numOpClassRanges; ++j) {
                dprintf(
                    MSG_INFO, "  OpClassRange #%u: MinOpClass=%u MaxOpClass=%u\n", j,
                    genericParams->opClassRanges[j].minOpClass,
                    genericParams->opClassRanges[j].maxOpClass);
            }

            dprintf(MSG_INFO, "  SSID Index=%u mibVAP_index:%d BandType=%u MLO VAP=%u\n",
                    genericParams->ssidIndex, genericParams->mibVAPIndex,
                    genericParams->bandType, genericParams->mloEnabled);
            eProfile = &map->eProfileSSID[genericParams->ssidIndex].eprofile;
            apacHyfiMapConfigDumpEProfile("  ", eProfile);
        }
    }
}

int apacMapGetRadioIdxByOpclass(apacMapData_t *map, int numOpClassRanges,
                    int minOpClass, int maxOpClass, apacHyfi20WifiBandType_e bandType)
{
    int i,RadioIdx=0;

    for(i=0; i < MAX_RADIO_CONFIGURATION; i++) {
        if (numOpClassRanges > 1 && map->CurrentRadioOpChannel[i] >= 36 &&
              map->CurrentRadioFreq[i] != APAC_WIFI_FREQ_6) { //5G Full Band
            RadioIdx = i+1;
        } else if(minOpClass >= 80 && maxOpClass <= 89 &&
             map->CurrentRadioFreq[i] != APAC_WIFI_FREQ_6) { //2G Bang
            if( map->CurrentRadioOpChannel[i] >= 1 && map->CurrentRadioOpChannel[i] <= 13 ) {
                RadioIdx = i+1;
            }
        } else if(minOpClass >= 115 && maxOpClass <= 120 &&
             map->CurrentRadioFreq[i] != APAC_WIFI_FREQ_6) { //5G low
            if( map->CurrentRadioOpChannel[i] >= 36 && map->CurrentRadioOpChannel[i] <= 64 ) {
                RadioIdx = i+1;
            }
        } else if(map->map6GEnabled == 1 && minOpClass >= 131 && maxOpClass <= 136 &&
                  map->CurrentRadioFreq[i] == APAC_WIFI_FREQ_6 &&
                  (map->CurrentRadioBandType[i] == bandInfo_6G || map->skip6GSplitBandCheck)) { //6G
            if( (map->CurrentRadioOpChannel[i] >= 1 && map->CurrentRadioOpChannel[i] <= 233)
                 || map->skip6GSplitBandCheck ) {
                RadioIdx = i+1;
            }
        } else if(map->map6GEnabled == 1 && minOpClass >= 131 && maxOpClass <= 136 &&
                  map->CurrentRadioBandType[i] == bandInfo_6G_Low &&
                  bandType == APAC_WIFI_BANDTYPE_LOW) { //6G
            if( map->CurrentRadioOpChannel[i] >= 1 && map->CurrentRadioOpChannel[i] <= 113  ) {
                RadioIdx = i+1;
            }
        } else if(map->map6GEnabled == 1 && minOpClass >= 131 && maxOpClass <= 136 &&
                  map->CurrentRadioBandType[i] == bandInfo_6G_High &&
                  bandType == APAC_WIFI_BANDTYPE_HIGH) { //6G
            if( map->CurrentRadioOpChannel[i] >= 117 && map->CurrentRadioOpChannel[i] <= 233  ) {
                RadioIdx = i+1;
            }
        } else if(minOpClass >= 121 && maxOpClass <= 127 &&
             map->CurrentRadioFreq[i] != APAC_WIFI_FREQ_6){ //5G High
            if( map->CurrentRadioOpChannel[i] >= 100 ) {
                RadioIdx = i+1;
            }
        }

        if(RadioIdx > 0)
            break;
    }
    return RadioIdx;
}

int apac_map_get_mib_vap_index(apacMapData_t *map, int idx)
{
    if(!map || idx == 0xff) {
        return -1;
    }

    return map->eProfileMatcher[idx].typeParams.genericParams.mibVAPIndex;
}

apacHyfi20MapVersion_e apacHyfiMapIsEnabled(apacMapData_t *map)
{
    return map->vEnabled;
}

apacBool_e apacHyfiMapPfComplianceEnabled(apacMapData_t *map)
{
    return map->mapPfCompliant;
}

apacBool_e apacHyfiMapIsTrafficSeparationEnabled(apacMapData_t *map)
{
    return map->map2TrafficSepEnabled;
}

apacBool_e apacHyfiMapIsSendPacketOnBridge(apacMapData_t *map)
{
    return map->send_packet_onbridge;
}

apacBool_e apacHyfiMapIsExternalController(apacMapData_t *map)
{
    return map->external_controller;
}

apacBool_e apacHyfiMapConfigServiceEnabled(apacMapData_t *map)
{
    return map->mapConfigServiceEnabled;
}

apacBool_e apacHyfiMapIsUseLoInterface(apacMapData_t *map)
{
    return map->useLoInterface;
}

u8 *apacHyfiGetControllerAlid(apacMapData_t *map)
{
    return map->controllerAlid.ether_addr_octet;
}

apacBool_e apacHyfiMapEarlyAPCapabilitySupported(apacMapData_t *map)
{
    return map->earlyAPCapabilitySupported;
}

u8 apacHyfiMapIsCtrlSpecMloOnbSupported(apacMapData_t *map)
{
    return map->ctrlSupportSpecMloOnb;
}

void apacHyfiMapCtrlSetSpecMloOnbSupported(apacMapData_t *map, u8 enable)
{
    map->ctrlSupportSpecMloOnb = enable;
}

#if DOT1X_SUPPORT
/**
 * @brief To confirm dot1x is enabled or not.
 * @param [in] map mapData

 * @return 1 if dot1x is enabled, 0 otherwise
 */

apacHyfi20MapVersion_e apacHyfiMapIsDot1xEnabled(apacMapData_t *map)
{
    return map->dot1xSupport;
}
#endif

apacBool_e apacHyfiMapIsMLOEnabled(apacMapData_t *map)
{
    return map->mloEnabled;
}

apacBool_e apacHyfiMapIsSLOEnabled(apacMapData_t *map)
{
    return map->sloEnabled;
}

ieee1905TLV_t *ieee1905MapAddAPCapabilityTLV(ieee1905TLV_t *TLV, u_int32_t *Len,
                                             apacHyfi20Data_t *pData) {
    u_int16_t tlvLen = 0;
    u_int8_t *ptr = NULL;
    apacMapData_t *map = NULL;
    u_int8_t apCapability = 0;

    apacHyfi20TRACE();

    map = HYFI20ToMAP(pData);

    TLV = ieee1905TLVGetNext(TLV);
    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_AP_CAP);
    ptr = ieee1905TLVValGet(TLV);

    /*unassocStaLinkMetricsOnCurrChan*/
    if (map->smartMonitorEnabled) {
        apCapability |= (1 << 7);
        apCapability |= (1 << 6);
    }

    /*agentInitiatedRssiBasedSteering*/
    if (map->mapPfCompliant) {
        apCapability |= (1 << 5);
    }

    *ptr = apCapability;
    tlvLen++;
    ptr++;

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    dprintf(MSG_INFO, "Added AP capability TLV(Enrollee) framelen %d tlvlen %d \n", *Len, tlvLen);

    printMsg((u8 *)TLV, tlvLen, MSG_DEBUG);
    return TLV;
}

ieee1905TLV_t *ieee1905MapAddBasicRadioTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                           apacHyfi20Data_t *pData) {
    u_int16_t tlvLen = 0;
    u_int8_t *ptr = NULL;
    u_int16_t i = 0, j = 0;
    apacPifMap_t *pIFMapData = NULL;
    apacMapData_t *map = NULL;

    apacHyfi20TRACE();

    map = HYFI20ToMAP(pData);

    TLV = ieee1905TLVGetNext(TLV);
    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_AP_RADIO_BASIC_CAP);
    ptr = ieee1905TLVValGet(TLV);

    pIFMapData = &pData->ap[band].pIFMapData;

    if (!pIFMapData) return TLV;

    os_memcpy(ptr, pData->ap[band].radio_mac, ETH_ALEN);
    tlvLen += ETH_ALEN; /* MAC addr len */
    ptr += ETH_ALEN;

    *ptr++ = map->MapConfMaxBss;  // configured value
    tlvLen++;
    *ptr++ = pIFMapData->apcap.hwcap.num_supported_op_classes;
    tlvLen++;

    for (i = 0; i < pIFMapData->apcap.hwcap.num_supported_op_classes; i++) {
        *ptr++ = pIFMapData->apcap.hwcap.opclasses[i].opclass;
        tlvLen++;
        *ptr++ = pIFMapData->apcap.hwcap.opclasses[i].max_tx_pwr_dbm;
        tlvLen++;
        *ptr++ = pIFMapData->apcap.hwcap.opclasses[i].num_non_oper_chan;
        tlvLen++;

        for (j = 0; j < pIFMapData->apcap.hwcap.opclasses[i].num_non_oper_chan; j++) {
            *ptr++ = pIFMapData->apcap.hwcap.opclasses[i].non_oper_chan_num[j];
            tlvLen++;
        }
    }

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    dprintf(MSG_INFO, "Added basic radio TLV(Enrollee) framelen %d tlvlen %d \n", *Len, tlvLen);

    printMsg((u8 *)TLV, tlvLen, MSG_DEBUG);
    return TLV;
}

ieee1905TLV_t *ieee1905MapAddAPHTCapabilitiesTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                 apacHyfi20Data_t *pData) {
    u_int16_t tlvLen = 0;
    u_int8_t *ptr = NULL;
    apacPifMap_t *pIFMapData = NULL;

    apacHyfi20TRACE();

    TLV = ieee1905TLVGetNext(TLV);
    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_AP_HT_CAP);
    ptr = ieee1905TLVValGet(TLV);

    pIFMapData = &pData->ap[band].pIFMapData;

    if (!pIFMapData) return TLV;

    os_memcpy(ptr, pData->ap[band].radio_mac, ETH_ALEN);
    tlvLen += ETH_ALEN; /* MAC addr len */
    ptr += ETH_ALEN;

    *ptr = ((pIFMapData->aphtcap.max_tx_nss - 1) & 0x3) << 6 |
           ((pIFMapData->aphtcap.max_rx_nss - 1) & 0x03) << 4 | pIFMapData->aphtcap.short_gi_support_20_mhz << 3 |
             pIFMapData->aphtcap.short_gi_support_40_mhz << 2 | pIFMapData->aphtcap.ht_support_40_mhz << 1;
    ptr++; tlvLen++;

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    dprintf(MSG_INFO, "Added AP HT capability TLV(Enrollee) framelen %d tlvlen %d \n", *Len, tlvLen);

    printMsg((u8 *)TLV, tlvLen, MSG_DEBUG);
    return TLV;
}

ieee1905TLV_t *ieee1905MapAddAPVHTCapabilitiesTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                  apacHyfi20Data_t *pData) {
    u_int16_t tlvLen = 0;
    u_int8_t *ptr = NULL;
    apacPifMap_t *pIFMapData = NULL;

    apacHyfi20TRACE();

    TLV = ieee1905TLVGetNext(TLV);
    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_AP_VHT_CAP);
    ptr = ieee1905TLVValGet(TLV);

    pIFMapData = &pData->ap[band].pIFMapData;

    if (!pIFMapData) return TLV;

    os_memcpy(ptr, pData->ap[band].radio_mac, ETH_ALEN);
    tlvLen += ETH_ALEN; /* MAC addr len */
    ptr += ETH_ALEN;

    u_int16_t txMcs = htons(pIFMapData->apvhtcap.supported_tx_mcs);
    memcpy(ptr, &txMcs, sizeof(txMcs));
    ptr += sizeof(txMcs); tlvLen += sizeof(txMcs);

    u_int16_t rxMcs = htons(pIFMapData->apvhtcap.supported_rx_mcs);
    memcpy(ptr, &rxMcs, sizeof(rxMcs));
    ptr += sizeof(rxMcs); tlvLen += sizeof(rxMcs);

    *ptr =
        ((pIFMapData->apvhtcap.max_tx_nss - 1) & 0x07) << 5 | ((pIFMapData->apvhtcap.max_rx_nss - 1) & 0x07) << 2 |
          pIFMapData->apvhtcap.short_gi_support_80_mhz << 1 | pIFMapData->apvhtcap.short_gi_support_160_mhz_80p_80_mhz;
    ptr++; tlvLen++;

    *ptr = pIFMapData->apvhtcap.support_80p_80_mhz << 7 | pIFMapData->apvhtcap.support_160_mhz << 6 |
           pIFMapData->apvhtcap.su_beam_former_capable << 5 | pIFMapData->apvhtcap.mu_beam_former_capable << 4;
    ptr++; tlvLen++;

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    dprintf(MSG_INFO, "Added AP VHT capability TLV(Enrollee) framelen %d tlvlen %d \n", *Len, tlvLen);

    printMsg((u8 *)TLV, tlvLen, MSG_DEBUG);
    return TLV;
}

ieee1905TLV_t *ieee1905MapAddAPHECapabilitiesTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                 apacHyfi20Data_t *pData) {
    u_int16_t tlvLen = 0;
    u_int8_t *ptr = NULL;
    apacPifMap_t *pIFMapData = NULL;
    size_t k = 0;

    apacHyfi20TRACE();

    TLV = ieee1905TLVGetNext(TLV);
    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_AP_HE_CAP);
    ptr = ieee1905TLVValGet(TLV);

    pIFMapData = &pData->ap[band].pIFMapData;

    if (!pIFMapData) return TLV;

    os_memcpy(ptr, pData->ap[band].radio_mac, ETH_ALEN);
    tlvLen += ETH_ALEN; /* MAC addr len */
    ptr += ETH_ALEN;

    // Each entry is 2 bytes
    *ptr = pIFMapData->aphecap.num_mcs_entries * 2;
    ptr++; tlvLen++;

    for (k = 0; k < pIFMapData->aphecap.num_mcs_entries; k++) {
        u_int16_t heMcs = htons(pIFMapData->aphecap.supported_he_mcs[k]);
        memcpy(ptr, &heMcs, sizeof(heMcs));
        ptr += sizeof(heMcs);
        tlvLen += sizeof(heMcs);
    }

    *ptr = ((pIFMapData->aphecap.max_tx_nss - 1) & 0x07) << 5 |
                ((pIFMapData->aphecap.max_rx_nss - 1) & 0x07) << 2 |
                (pIFMapData->aphecap.support_80p_80_mhz << 1) | pIFMapData->aphecap.support_160_mhz;
    ptr++; tlvLen++;

    *ptr =  pIFMapData->aphecap.su_beam_former_capable << 7 | pIFMapData->aphecap.mu_beam_former_capable << 6 |
                pIFMapData->aphecap.ul_mu_mimo_capable << 5 | pIFMapData->aphecap.ul_mu_mimo_ofdma_capable << 4 |
                pIFMapData->aphecap.dl_mu_mimo_ofdma_capable << 3 | pIFMapData->aphecap.ul_ofdma_capable << 2 |
                pIFMapData->aphecap.dl_ofdma_capable << 1;
    ptr++; tlvLen++;

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    dprintf(MSG_INFO, "Added AP HE capability TLV(Enrollee) framelen %d tlvlen %d \n", *Len, tlvLen);

    printMsg((u8 *)TLV, tlvLen, MSG_DEBUG);
    return TLV;
}

ieee1905TLV_t *ieee1905MapAddWifi6CapabilitiesTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                  apacHyfi20Data_t *pData) {
    u_int16_t tlvLen = 0;
    u_int8_t *ptr = NULL;
    apacPifMap_t *pIFMapData = NULL;
    size_t k = 0;

    apacHyfi20TRACE();

    TLV = ieee1905TLVGetNext(TLV);
    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_AP_WIFI6_CAP);
    ptr = ieee1905TLVValGet(TLV);

    pIFMapData = &pData->ap[band].pIFMapData;

    if (!pIFMapData) return TLV;

    os_memcpy(ptr, pData->ap[band].radio_mac, ETH_ALEN);
    tlvLen += ETH_ALEN; /* MAC addr len */
    ptr += ETH_ALEN;

    *ptr = pIFMapData->apwifi6cap.numofroles;
    tlvLen++; ptr++;

    for (k = 0; k < pIFMapData->apwifi6cap.numofroles; k++) {
        u_int16_t heMcs = 0;
        size_t i = 0;
        size_t maxMCS = 4;                                  // minimum MCS

        if (pIFMapData->apwifi6cap.role_cap[k].he160) maxMCS += 4;       // as per 802.11 ax spec
        if (pIFMapData->apwifi6cap.role_cap[k].he80plus80) maxMCS += 4;  // as per 802.11 ax spec

        *ptr = pIFMapData->apwifi6cap.role_cap[k].role << 6 |
                   pIFMapData->apwifi6cap.role_cap[k].he160 << 5 |
                   pIFMapData->apwifi6cap.role_cap[k].he80plus80 << 4 |
                   ((u_int8_t)maxMCS & 0x0f);                  // To add MCS NSS Length from (bit 0-3)
        ptr++; tlvLen++;

        maxMCS = maxMCS / 2;
        for (i = 0; i < maxMCS; i++) {
            heMcs = htons(pIFMapData->apwifi6cap.role_cap[k].supported_he_mcs[i]);
            memcpy(ptr, &heMcs, sizeof(heMcs));
            ptr += sizeof(heMcs);
            tlvLen += sizeof(heMcs);
            heMcs = 0;
        }

        *ptr = pIFMapData->apwifi6cap.role_cap[k].subeamformer << 7 |
                    pIFMapData->apwifi6cap.role_cap[k].subeamformee << 6 |
                    pIFMapData->apwifi6cap.role_cap[k].mu_beam_former_status << 5 |
                    pIFMapData->apwifi6cap.role_cap[k].beam_formee_sts_less_than_80supported << 4 |
                    pIFMapData->apwifi6cap.role_cap[k].beam_formee_sts_more_than_80supported << 3 |
                    pIFMapData->apwifi6cap.role_cap[k].ulmumimosupported << 2 |
                    pIFMapData->apwifi6cap.role_cap[k].ulofdmasupported << 1 |
                    pIFMapData->apwifi6cap.role_cap[k].dlofdmasupported << 0;
        ptr++; tlvLen++;

        *ptr = pIFMapData->apwifi6cap.role_cap[k].maxuser_per_dl_mumimotxap << 4 |
                    pIFMapData->apwifi6cap.role_cap[k].max_user_per_dl_mumimorxap;
        ptr++; tlvLen++;

        *ptr = pIFMapData->apwifi6cap.role_cap[k].maxuserdlofdmatxap;
        ptr++; tlvLen++;

        *ptr = pIFMapData->apwifi6cap.role_cap[k].maxuserdlofdmarxap;
        ptr++; tlvLen++;

        *ptr = pIFMapData->apwifi6cap.role_cap[k].rtssupported << 7 |
               pIFMapData->apwifi6cap.role_cap[k].murtssupported << 6 |
               pIFMapData->apwifi6cap.role_cap[k].multibssidsupported << 5 |
               pIFMapData->apwifi6cap.role_cap[k].muedcasupported << 4 |
               pIFMapData->apwifi6cap.role_cap[k].twtrequestersupprted << 3 |
               pIFMapData->apwifi6cap.role_cap[k].twtrespondersupported << 2 ;
        ptr++; tlvLen++;
    }

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    dprintf(MSG_INFO, "Added WiFi 6 capability TLV(Enrollee) framelen %d tlvlen %d \n", *Len, tlvLen);

    printMsg((u8 *)TLV, tlvLen, MSG_DEBUG);
    return TLV;
}

ieee1905TLV_t *ieee1905MapAddMetricCollectionTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                 apacHyfi20Data_t *pData) {
    u_int16_t tlvLen = 0;
    u_int8_t *ptr = NULL;

    apacHyfi20TRACE();

    TLV = ieee1905TLVGetNext(TLV);
    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_METRIC_COLLECTION_INTERVAL);
    ptr = ieee1905TLVValGet(TLV);

    u_int32_t collectionInterval = htonl(300);
    memcpy(ptr, &collectionInterval, sizeof(collectionInterval));
    ptr += sizeof(collectionInterval);
    tlvLen += sizeof(collectionInterval);

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    dprintf(MSG_INFO, "Added Metric collection TLV(Enrollee) "
                        "framelen %d tlvlen %d \n", *Len, tlvLen);

    return TLV;
}

ieee1905TLV_t *ieee1905MapAddDeviceInventoryTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                apacHyfi20Data_t *pData) {

    u_int16_t tlvLen = 0;
    u_int8_t *ptr = NULL;
    size_t lenstr = 0;
    apacMapData_t *map = NULL;
    apacHyfi20AP_t *pAP = pData->ap;
    u_int16_t j = 0;

    apacHyfi20TRACE();

    map = HYFI20ToMAP(pData);

    TLV = ieee1905TLVGetNext(TLV);
    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_DEVICE_INVENTORY);
    ptr = ieee1905TLVValGet(TLV);

    *ptr = strlen(map->serialNumber);
    ptr++; tlvLen++;

    lenstr = strlen(map->serialNumber);
    memcpy(ptr, map->serialNumber, lenstr);
    tlvLen += lenstr; ptr += lenstr;

    *ptr = strlen(map->softwareVersion);
    ptr++; tlvLen++;

    lenstr = strlen(map->softwareVersion);
    memcpy(ptr, map->softwareVersion, lenstr);
    tlvLen += lenstr; ptr += lenstr;

    *ptr = strlen(map->executionEnvironment);
    ptr++; tlvLen++;

    lenstr = strlen(map->executionEnvironment);
    memcpy(ptr, map->executionEnvironment, lenstr);
    tlvLen += lenstr; ptr += lenstr;

    *ptr = map->numRadios;
    ptr++; tlvLen++;

    for (j = 0; j < APAC_NUM_WIFI_FREQ; j++) {

        if (!pAP[j].valid) {
            continue;
        }

        os_memcpy(ptr, pAP[j].radio_mac, ETH_ALEN);
        tlvLen += ETH_ALEN; /* MAC addr len */
        ptr += ETH_ALEN;

        *ptr = strlen(map->chipsetVendor);
        ptr++; tlvLen++;

        lenstr = strlen(map->chipsetVendor);
        memcpy(ptr, map->chipsetVendor, lenstr);
        tlvLen += lenstr; ptr += lenstr;
    }

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    dprintf(MSG_INFO, "Added Device Inventory TLV(Enrollee) "
                        "framelen %d tlvlen %d \n", *Len, tlvLen);

    return TLV;
}

void serializeWifi7AgentCapabilitiesTLV(u8 *wifi7CapMsg, u32 *wifi7CapMsgLen,
                                        apacHyfi20Data_t *pData) {
    u_int16_t i = 0, j = 0;
    u_int8_t *ptrToLaterUpdateCount1 = NULL;
    u_int8_t *ptrToLaterUpdateCount2 = NULL;
    apacMapData_t *map = NULL;

    apacHyfi20TRACE();

    map = HYFI20ToMAP(pData);

    /* MaxNumMLDs */
    u8 maxNumMLDs = APAC_MAP_MAX_SUPPORTED_MLDS;
    *wifi7CapMsg++ = maxNumMLDs;
    (*wifi7CapMsgLen)++;

    /* AP Maximum Links - bits 7-4
       bSTA Maximum Links - bits 3-0 */
    u8 maxLinks = 0;
    u8 apMaxLinks = map->maxAPLinks;
    u8 bstaMaxLinks = APAC_MAP_MAX_BSTA_LINKS;
    maxLinks |= (apMaxLinks & 0x0F) << 4;
    maxLinks |= (bstaMaxLinks & 0x0F);
    *wifi7CapMsg++ = maxLinks;
    (*wifi7CapMsgLen)++;

    /* TID-To-Link_ Mapping_ Capability
       0: The Agent does not support TID-to-Llink mapping
       1: The Agent supports the mapping of each TID to the same or different link set.
       2: The Agent only supports the mapping of all TIDs to the same link set.
       3 – Reserved
    */
    u8 t2lmCapability = APAC_MAP_HOMOGENEOUS_T2LM_SUPPORT;
    *wifi7CapMsg++ = (t2lmCapability&0x03) << 6;
    (*wifi7CapMsgLen)++;

    // Reserved
    memset(wifi7CapMsg, 0x00, 13);
    wifi7CapMsg += 13;
    *wifi7CapMsgLen += 13;

    *wifi7CapMsg++ = map->numRadios;
    (*wifi7CapMsgLen)++;

    for (i = 0; i < APAC_NUM_WIFI_FREQ; i++) {

        if (!pData->ap[i].valid) {
            continue;
        }

        os_memcpy(wifi7CapMsg, pData->ap[i].radio_mac, ETH_ALEN);
        *wifi7CapMsgLen += ETH_ALEN; /* MAC addr len */
        wifi7CapMsg += ETH_ALEN;

        // Reserved
        memset(wifi7CapMsg, 0x00, 24);
        wifi7CapMsg += 24;
        *wifi7CapMsgLen += 24;

        *wifi7CapMsg = 0;
        // ap str support
        apacBool_e apStrSupported = APAC_TRUE;
        *wifi7CapMsg |= apStrSupported ? (1 << 7) : 0;
        // ap nstr support
        apacBool_e apNStrSupported = APAC_TRUE;
        *wifi7CapMsg |= apNStrSupported ? (1 << 6) : 0;
        // ap emlsr support
        apacBool_e apEmlsrSupported = APAC_TRUE;
        *wifi7CapMsg |= apEmlsrSupported ? (1 << 5) : 0;
        // ap emlmr support
        apacBool_e apEmlmrSupported = APAC_TRUE;
        *wifi7CapMsg |= apEmlmrSupported ? (1 << 4) : 0;
        wifi7CapMsg++;
        (*wifi7CapMsgLen)++;

        *wifi7CapMsg = 0;
        // bsta str support
        apacBool_e bstaStrSupported = APAC_TRUE;
        *wifi7CapMsg |= bstaStrSupported ? (1 << 7) : 0;
        // bsta nstr support
        apacBool_e bstaNStrSupported = APAC_TRUE;
        *wifi7CapMsg |= bstaNStrSupported ? (1 << 6) : 0;
        // bsta emlsr support
        apacBool_e bstaEmlsrSupported = APAC_TRUE;
        *wifi7CapMsg |= bstaEmlsrSupported ? (1 << 5) : 0;
        // bsta emlmr support
        apacBool_e bstaEmlmrSupported = APAC_TRUE;
        *wifi7CapMsg |= bstaEmlmrSupported ? (1 << 4) : 0;
        wifi7CapMsg++;
        (*wifi7CapMsgLen)++;

        // num of ap str records
        *wifi7CapMsg = 0;
        ptrToLaterUpdateCount1 = wifi7CapMsg;
        wifi7CapMsg++;
        (*wifi7CapMsgLen)++;
        if(apacHyfiMapIsMLOEnabled(map)) {
            for (j = 0; j < APAC_NUM_WIFI_FREQ; j++) {
                if (!pData->ap[j].valid) {
                    continue;
                }
                if(os_memcmp(pData->ap[i].radio_mac, pData->ap[j].radio_mac, ETH_ALEN) == 0)
                    continue;
                if(pData->ap[j].isMLORadio == APAC_FALSE)
                    continue;
                (*ptrToLaterUpdateCount1)++;
                os_memcpy(wifi7CapMsg, pData->ap[j].radio_mac, ETH_ALEN);
                *wifi7CapMsgLen += ETH_ALEN; /* MAC addr len */
                wifi7CapMsg += ETH_ALEN;
                // ap str freq seperation - bits  7:3
                *wifi7CapMsg++ = 0;
                (*wifi7CapMsgLen)++;
            }
        }

        // num of ap nstr records
        *wifi7CapMsg++ = 0;
        (*wifi7CapMsgLen)++;

        // num of ap emlsr records
       *wifi7CapMsg = 0;
        ptrToLaterUpdateCount1 = wifi7CapMsg;
        wifi7CapMsg++;
        (*wifi7CapMsgLen)++;
        if(apacHyfiMapIsMLOEnabled(map)) {
            for (j = 0; j < APAC_NUM_WIFI_FREQ; j++) {
                if (!pData->ap[j].valid) {
                    continue;
                }
                if(os_memcmp(pData->ap[i].radio_mac, pData->ap[j].radio_mac, ETH_ALEN) == 0)
                    continue;
                if(pData->ap[j].isMLORadio == APAC_FALSE)
                    continue;
                (*ptrToLaterUpdateCount1)++;
                os_memcpy(wifi7CapMsg, pData->ap[j].radio_mac, ETH_ALEN);
                *wifi7CapMsgLen += ETH_ALEN; /* MAC addr len */
                wifi7CapMsg += ETH_ALEN;
                // ap str freq seperation - bits  7:3
                *wifi7CapMsg++ = 0;
                (*wifi7CapMsgLen)++;
            }
        }

        // num of ap emlmr records
        *wifi7CapMsg++ = 0;
        (*wifi7CapMsgLen)++;

        // num of bsta str records
        *wifi7CapMsg = 0;
        ptrToLaterUpdateCount2 = wifi7CapMsg;
        wifi7CapMsg++;
        (*wifi7CapMsgLen)++;
        if(apacHyfiMapIsMLOEnabled(map)) {
            for (j = 0; j < APAC_NUM_WIFI_FREQ; j++) {
                if (!pData->ap[j].valid) {
                    continue;
                }
                if(os_memcmp(pData->ap[i].radio_mac, pData->ap[j].radio_mac, ETH_ALEN) == 0)
                    continue;
                if(pData->ap[j].isMLORadio == APAC_FALSE)
                    continue;
                (*ptrToLaterUpdateCount2)++;
                os_memcpy(wifi7CapMsg, pData->ap[j].radio_mac, ETH_ALEN);
                *wifi7CapMsgLen += ETH_ALEN; /* MAC addr len */
                wifi7CapMsg += ETH_ALEN;
                // bsta str freq seperation - bits  7:3
                *wifi7CapMsg++ = 0;
                (*wifi7CapMsgLen)++;
            }
        }

        // num of bsta nstr records
        *wifi7CapMsg++ = 0;
        (*wifi7CapMsgLen)++;

        // num of bsta emlsr records
        *wifi7CapMsg = 0;
        ptrToLaterUpdateCount1 = wifi7CapMsg;
        wifi7CapMsg++;
        (*wifi7CapMsgLen)++;
        if(apacHyfiMapIsMLOEnabled(map)) {
            for (j = 0; j < APAC_NUM_WIFI_FREQ; j++) {
                if (!pData->ap[j].valid) {
                    continue;
                }
                if(os_memcmp(pData->ap[i].radio_mac, pData->ap[j].radio_mac, ETH_ALEN) == 0)
                    continue;
                if(pData->ap[j].isMLORadio == APAC_FALSE)
                    continue;
                (*ptrToLaterUpdateCount1)++;
                os_memcpy(wifi7CapMsg, pData->ap[j].radio_mac, ETH_ALEN);
                *wifi7CapMsgLen += ETH_ALEN; /* MAC addr len */
                wifi7CapMsg += ETH_ALEN;
                // ap str freq seperation - bits  7:3
                *wifi7CapMsg++ = 0;
                (*wifi7CapMsgLen)++;
            }
        }

        // num of bsta emlmr records
        *wifi7CapMsg++ = 0;
        (*wifi7CapMsgLen)++;
    }
}

ieee1905TLV_t *ieee1905MapAddMLOVendorTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
            apacHyfi20Data_t *pData)
{
    u_int16_t tlvLen = 0;
    ieee1905QCAMessage_t *qcaMessage = NULL;

    apacHyfi20TRACE();

    ieee1905MLOCapabilities_t *mloCap;
    TLV = ieee1905TLVGetNext(TLV);
    qcaMessage = (ieee1905QCAMessage_t *)ieee1905TLVValGet(TLV);

    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_VENDOR_SPECIFIC);

    ieee1905QCAOUIAndTypeSet(qcaMessage, IEEE1905_QCA_TYPE_AP_MLO_CAP, tlvLen);
    mloCap = (ieee1905MLOCapabilities_t *)qcaMessage->content;

    memset(mloCap, 0 , sizeof(ieee1905MLOCapabilities_t));
    os_memcpy(mloCap->radioAddr.ether_addr_octet, pData->ap[band].radio_mac, ETH_ALEN);

    mloCap->MLOSupported = pData->ap[band].isMLORadio;
    mloCap->T2LMSupported = 0;

    tlvLen += sizeof(ieee1905MLOCapabilities_t);

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    dprintf(MSG_INFO, "MLO Cap TLV : MLO Support %d T2LM Support %d for band %d \n",
                    mloCap->MLOSupported, mloCap->T2LMSupported, band);

    printMsg((u8 *)TLV, tlvLen, MSG_DEBUG);
    return TLV;
}

ieee1905TLV_t *ieee1905MapAddRadioIdTLV(ieee1905TLV_t *TLV,
        u_int32_t *Len,
        struct apac_wps_session *sess)
{

    u_int8_t *ptr = NULL;
    u8 *Data = NULL;

    apacHyfi20TRACE();

    if(sess->basicRadioCapLen)
        Data = sess->basicRadioCap;
    else
        return TLV;

    ieee1905TLVTypeSet( TLV, IEEE1905_TLV_TYPE_RADIO_IDENTIFIER);
    ptr = ieee1905TLVValGet(TLV);
    os_memcpy(ptr, Data, ETH_ALEN);

    ieee1905TLVLenSet(TLV, ETH_ALEN, *Len);
    printMsg((u8 *)TLV, ETH_ALEN,MSG_MSGDUMP);

    return ieee1905TLVGetNext(TLV);
}

ieee1905TLV_t *ieee1905MapAddTrafficSeparationPolicyTLV(ieee1905TLV_t *TLV, u_int32_t *Len,
                                                        struct apac_wps_session *sess) {
    u8 i = 0, mapEprofileList[MAX_WLAN_CONFIGURATION] = {0};
    u8 eMaxBss = 0, matchCnt = 0, numSSID = 0;
    u16 primaryVlanID = 0, pcp = 0;
    apacHyfi20Data_t *pApacData = sess->pData;
    apacMapData_t *mapData = HYFI20ToMAP(pApacData);
    apacBool_e add8021QTLV = APAC_FALSE;
    apacBool_e addTrafficSepTLV = APAC_FALSE;
    apacBool_e isDedicatedBHRadio = APAC_FALSE;
    const apacMapEProfile_t *eProfile = NULL;
    const struct apacMapEProfileMatcherGenericParams_t *genericParams = NULL;
    apacMapEProfileMatcher_t *eProfileMatcher = NULL;
    u_int8_t *ptr = NULL;
    u_int16_t tlvLen = 0;

    apacHyfi20TRACE();
    matchCnt = apac_map_get_eprofile(sess, mapEprofileList, &eMaxBss);

    for (i = 0; i < matchCnt; i++) {
        eProfileMatcher = &mapData->eProfileMatcher[mapEprofileList[i]];
        if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == eProfileMatcher->matcherType) {
            eProfile = &eProfileMatcher->typeParams.alParams.eprofile;
        } else if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC == eProfileMatcher->matcherType) {
            genericParams = &eProfileMatcher->typeParams.genericParams;
            if (genericParams) {
                eProfile = &mapData->eProfileSSID[genericParams->ssidIndex].eprofile;
            }
        }

        /* Primary VlanID will be 0 for backhaul eprofile while populating
           from BSS policy configuration */
        if (eProfile->primaryVlanID > 0) {
            primaryVlanID = eProfile->primaryVlanID;
            pcp = eProfile->pcp;
            add8021QTLV = APAC_TRUE;
        }
        if (eProfile->vlanID > 0) {
            numSSID++;
            addTrafficSepTLV = APAC_TRUE;
        }
        /* FIX Traffic Sepearation Policy TLV for dedicated backhaul radio */
        if (eProfile->vlanID == 0) {
            addTrafficSepTLV = APAC_TRUE;
        }

        if (!apacHyfiMapPfComplianceEnabled(mapData)) {
            if (matchCnt == 1 && eProfile->isBackhaul) {
                isDedicatedBHRadio = APAC_TRUE;
            }
        }
    }

    /* If it is dedicated backhaul radio, fill primary vlan info from FH profile */
    if (isDedicatedBHRadio && !add8021QTLV) {
        for(i = 0 ; i < mapData->eProfileCnt; i++) {
            eProfileMatcher = &mapData->eProfileMatcher[i];
            if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == eProfileMatcher->matcherType) {
                eProfile = &eProfileMatcher->typeParams.alParams.eprofile;
            } else if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC == eProfileMatcher->matcherType) {
                genericParams = &eProfileMatcher->typeParams.genericParams;
                if (genericParams) {
                    eProfile = &mapData->eProfileSSID[genericParams->ssidIndex].eprofile;
                }
            }

            if (eProfile->isBackhaul) {
                continue;
            }

            /*Fetch primary vlan id */
            if (eProfile->primaryVlanID > 0) {
                primaryVlanID = eProfile->primaryVlanID;
                pcp = eProfile->pcp;
                add8021QTLV = APAC_TRUE;
                dprintf(MSG_INFO, "%s: Update the primary vlan id from eprofile FH %d ssid %s\n",
                            __func__ ,eProfile->isFronthaul,eProfile->ssid);
                break;
            }
        }
    }

    if (add8021QTLV) {
        ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_8021Q_RULES);
        ptr = ieee1905TLVValGet(TLV);
        tlvLen = 0;

        u_int16_t pVlanID = htons(primaryVlanID);
        memcpy(ptr, &pVlanID, sizeof(pVlanID));
        ptr += sizeof(pVlanID);
        tlvLen += sizeof(pVlanID);

        *ptr++ = pcp << 5;
        tlvLen++;

        ieee1905TLVLenSet(TLV, tlvLen, *Len);
        TLV = ieee1905TLVGetNext(TLV);
    }

    if (addTrafficSepTLV) {
        tlvLen = 0;
        ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TRAFFIC_SEPARATON_POLICY);
        ptr = ieee1905TLVValGet(TLV);

        *ptr++ = numSSID;
        tlvLen++;

        for (i = 0; i < matchCnt; i++) {
            eProfileMatcher = &mapData->eProfileMatcher[mapEprofileList[i]];
            if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == eProfileMatcher->matcherType) {
                eProfile = &eProfileMatcher->typeParams.alParams.eprofile;
            } else if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC == eProfileMatcher->matcherType) {
                genericParams = &eProfileMatcher->typeParams.genericParams;
                if (genericParams) {
                    eProfile = &mapData->eProfileSSID[genericParams->ssidIndex].eprofile;
                }
            }
#if MAP_ENABLED
#if DOT1X_SUPPORT
                dprintf(MSG_DEBUG, "%s:%d> dot1xSupport=%d \n", __func__, __LINE__,
                        sess->dot1xSupport);
                dprintf(MSG_DEBUG, "%s:%d> auth_primary_server=%s \n", __func__, __LINE__,
                        eProfile->auth_primary_server);
                dprintf(MSG_DEBUG, "%s:%d> eProfile->auth = %x \n", __func__, __LINE__,
                        eProfile->auth);
                dprintf(MSG_DEBUG, "%s:%d> eProfile->encr = %x \n", __func__, __LINE__,
                        eProfile->encr);
                dprintf(MSG_DEBUG, "%s:%d> eProfile->ssid = %s \n", __func__, __LINE__,
                        eProfile->ssid);
                dprintf(MSG_DEBUG, "%s:%d> eProfile->ft_auth     = %d \n", __func__, __LINE__,
                        eProfile->ft_auth);
                dprintf(MSG_DEBUG, "%s:%d> eProfile->kh_key_hex  = %s \n", __func__, __LINE__,
                        eProfile->kh_key_hex);
                dprintf(MSG_DEBUG, "%s:%d> eProfile->mobility_domain = %s \n", __func__, __LINE__,
                        eProfile->mobility_domain);
                if (eProfile->auth & WPS_AUTHTYPE_WPA ||
                    eProfile->auth & WPS_AUTHTYPE_WPA2 ||
                    eProfile->auth & WPS_AUTHTYPE_WPAMIXED ||
                    eProfile->auth & WPS_AUTHTYPE_WPA3_SUITEB) {
                    if (eProfile->encr & WPS_ENCRTYPE_TKIP ||
                        eProfile->encr & WPS_ENCRTYPE_AES ||
                        eProfile->suite_b) {
                        if (!sess->dot1xSupport ||
                            !apacHyfiMapIsDot1xEnabled(mapData)) {
                            continue;
                        }
                    }
                }
#endif
#endif
            if (eProfile->vlanID && eProfile->vlanID != -1) {
                *ptr++ = strlen(eProfile->ssid);
                tlvLen++;

                memcpy(ptr, eProfile->ssid, strlen(eProfile->ssid));
                ptr += strlen(eProfile->ssid);
                tlvLen += strlen(eProfile->ssid);

                u_int16_t vlanID = htons(eProfile->vlanID);
                memcpy(ptr, &vlanID, sizeof(vlanID));
                ptr += sizeof(vlanID);
                tlvLen += sizeof(vlanID);
            }
        }

        ieee1905TLVLenSet(TLV, tlvLen, *Len);
        TLV = ieee1905TLVGetNext(TLV);
    }

    return TLV;
}

u8 ieee1905MapGetRadioAddrBasicRadioTLV(struct apac_wps_session *sess,
                                        const char *op, u8 ruid[]) {

    u8 *ptr = NULL;
    u_int8_t i = 0;
    u_int8_t j = 0;
    u_int8_t k = 0;
    u_int8_t unoperable = 0;
    u8 opclassCnt;
    u8 opclass;

    if (!sess || !op || !ruid) {
        return APAC_FALSE;
    }

    if(sess->radioCount == 4) {

        for(i = 0; i < APAC_NUM_WIFI_FREQ; i++)
        {
            ptr = sess->basicRadioCapMLO[i];

            os_memcpy(ruid, ptr, ETH_ALEN);
            ptr += ETH_ALEN; /* MAC addr len */

            ptr++;
            opclassCnt = *ptr++;

            for (j = 0; j < opclassCnt; j++) {
                opclass = *ptr++;

                ptr++;  // skipping tx power
                unoperable = *ptr++;

                for (k = 0; k < unoperable; k++) {
                    ptr++;
                }

                // Does this operating class fall within the range?
                if((!strcmp(op,"8x") && opclass >= 80 && opclass <= 89) ||
                (!strcmp(op,"11x") && opclass >= 110 && opclass <= 120) ||
                (!strcmp(op,"12x") && opclass >= 121 && opclass <= 129) ||
                (!strcmp(op,"13x") && opclass >= 131 && opclass <= 136)) {
                    dprintf(MSG_DEBUG, "%s: Matching opclass %s : RUID %02x:%02x:%02x:%02x:%02x:%02x \n", __func__,
                                        op, ruid[0],ruid[1],ruid[2],ruid[3],ruid[4],ruid[5]);
                    return APAC_TRUE;
                } else {
                    break;
                }
            }
        }
    }
    else if(sess->radioCount == 3) {

        for(i = 0; i < APAC_NUM_WIFI_FREQ; i++)
        {
            ptr = sess->basicRadioCapMLO[i];

            os_memcpy(ruid, ptr, ETH_ALEN);
            ptr += ETH_ALEN; /* MAC addr len */

            ptr++;
            opclassCnt = *ptr++;

            for (j = 0; j < opclassCnt; j++) {
                opclass = *ptr++;

                ptr++;  // skipping tx power
                unoperable = *ptr++;

                for (k = 0; k < unoperable; k++) {
                    ptr++;
                }

                // Does this operating class fall within the range?
                if((!strcmp(op,"8x") && opclass >= 80 && opclass <= 89) ||
                (!strcmp(op,"11x") && opclass >= 110 && opclass <= 129) ||
                (!strcmp(op,"12x") && opclass >= 110 && opclass <= 129) ||
                (!strcmp(op,"13x") && opclass >= 131 && opclass <= 136)) {
                    dprintf(MSG_DEBUG, "%s: Matching opclass %s : RUID %02x:%02x:%02x:%02x:%02x:%02x \n", __func__,
                                        op, ruid[0],ruid[1],ruid[2],ruid[3],ruid[4],ruid[5]);
                    return APAC_TRUE;
                } else {
                    break;
                }
            }
        }
    }

    return APAC_FALSE;
}

u8 ieee1905MapGetRadioAddrBasicRadioTLVGeneric(struct apac_wps_session *sess,
                                               u8 minop, u8 maxop, u8 ruid[],
                                               u8 bandType) {

    u8 *ptr = NULL;
    u_int8_t i = 0;
    u_int8_t j = 0;
    u_int8_t k = 0;
    u_int8_t unoperable = 0;
    u8 opclassCnt;
    u8 opclass;

    for(i = 0; i < APAC_NUM_WIFI_FREQ; i++)
    {
        ptr = sess->basicRadioCapMLO[i];

        os_memcpy(ruid, ptr, ETH_ALEN);
        ptr += ETH_ALEN; /* MAC addr len */

        ptr++;
        opclassCnt = *ptr++;

        for (j = 0; j < opclassCnt; j++) {
            opclass = *ptr++;

            ptr++;  // skipping tx power
            unoperable = *ptr++;

            for (k = 0; k < unoperable; k++) {
                ptr++;
            }

            // Does this operating class fall within the range?
            if (opclass >= minop && opclass <= maxop) {
                if (opclass >= 131 && opclass <= 136) {
                    if (ieee1905MapParseNonOperableChannel(sess->basicRadioCapMLO[i],
                        MAX_BASIC_RADIO_CAP_LEN, bandType) == APAC_TRUE)
                        return APAC_TRUE;
                }
                else {
                    return APAC_TRUE;
                }
            }
        }
    }
    return APAC_FALSE;
}

u8 ieee1905MapGetMaxNumLinksFromWiFi7TLV(struct apac_wps_session *sess) {
    u8 *ptr = NULL;
    u_int8_t maxNumAPLinks = 0;

    for(int i = 0; i < 1024; i++) {
        dprintf(MSG_ERROR, "%d ", sess->wifi7AgentCap[i]);
    }

    ptr = sess->wifi7AgentCap;
    ptr++;
    // AP max num links
    maxNumAPLinks = (*ptr & 0xF0) >> 4;
    return maxNumAPLinks;
}

ieee1905TLV_t *ieee1905MapAddMLOPolicyTLV(ieee1905TLV_t *TLV, u_int32_t *Len,
                                          struct apac_wps_session *sess) {

    u8 i = 0, j = 0, k = 0, l = 0;
    apacHyfi20Data_t *pApacData = sess->pData;
    apacMapData_t *mapData = HYFI20ToMAP(pApacData);
    const apacMapEProfile_t *eProfile1 = NULL;
    const apacMapEProfile_t *eProfile2 = NULL;
    apacMapEProfileMatcher_t *eProfileMatcher1 = NULL;
    apacMapEProfileMatcher_t *eProfileMatcher2 = NULL;
    const struct apacMapEProfileMatcherGenericParams_t *genericParams1 = NULL;
    const struct apacMapEProfileMatcherGenericParams_t *genericParams2 = NULL;
    u_int8_t *ptr = NULL;
    u_int8_t *ptrToLaterUpdateCount1 = NULL;
    u_int8_t *ptrToLaterUpdateCount2 = NULL;
    u_int16_t tlvLen = 0;
    uint8_t mldsprocessed[26] = {0};
    char mldsprocessedGeneric[10][1024];
    uint8_t mldsprocessedGenericCount = 0;
    const char *opclass = NULL;
    u8 ruid[ETH_ALEN] = {0};
    char alID[1024] = { 0 };
    apacBool_e foundBstaMld = APAC_FALSE;
    apacBool_e match = APAC_FALSE;

    apacHyfi20TRACE();

    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_AGENT_AP_MLD_CONFIG);
    ptr = ieee1905TLVValGet(TLV);
    tlvLen = 0;

    /* numAPMLDs - will be updated as we fill AP MLD information*/
    *ptr = 0;
    ptrToLaterUpdateCount1 = ptr;
    ptr++;
    tlvLen++;

    /* For PF Compliant */
    if (apacHyfiMapPfComplianceEnabled(mapData)) {
        for (i = 0; i < mapData->eProfileCnt; i++) {
            eProfileMatcher1 = &mapData->eProfileMatcher[i];
            if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == eProfileMatcher1->matcherType) {
                eProfile1 = &eProfileMatcher1->typeParams.alParams.eprofile;
            } else {
                continue;
            }

            if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == eProfileMatcher1->matcherType) {
                snprintf(alID, sizeof(alID), "%02x%02x%02x%02x%02x%02x ", sess->dest_addr[0], sess->dest_addr[1],
                    sess->dest_addr[2], sess->dest_addr[3], sess->dest_addr[4], sess->dest_addr[5]);
                if (strncasecmp(alID, eProfileMatcher1->typeParams.alParams.alId,
                                                            IEEE80211_ADDR_LEN * 2)) {
                    continue;
                }
            }

            /* Check if AP vap with a valid MLD group that hasnt been processed yet*/
            if(strlen(eProfile1->mldGrouping) &&
            (mldsprocessed[eProfile1->mldGrouping[0] - 'A'] == 0)) {

                (*ptrToLaterUpdateCount1)++;

                /* bit7 - AP_MLD_MAC_Addr_Valid
                bit6 - Disabled_Subchannel_Valid
                We wont send out valid mac address */
                *ptr = 0x00;
                ptr++;
                tlvLen++;
                /* ssidLength */
                *ptr = strlen(eProfile1->ssid);
                ptr++;
                tlvLen++;
                /* ssid */
                memcpy(ptr, eProfile1->ssid, strlen(eProfile1->ssid));
                ptr += strlen(eProfile1->ssid);
                tlvLen += strlen(eProfile1->ssid);
                /* ap mld mac address */
                os_memset(ptr, 0x00, ETH_ALEN);
                tlvLen += ETH_ALEN; /* MAC addr len */
                ptr += ETH_ALEN;

                /* STR - bit7
                NSTR - bit6
                EMLSR - bit5
                EMLMR - bit4 */
                // For now we only set STR Enabled
                *ptr = 0xA0;
                ptr++;
                tlvLen++;

                // Reserved
                os_memset(ptr, 0x00, 20);
                ptr += 20;
                tlvLen += 20;

                u8 maxNumAPLinks = ieee1905MapGetMaxNumLinksFromWiFi7TLV(sess);

                /* numAffiliatedAPs - will be updated as we fill AP MLD information*/
                *ptr = 0;
                ptrToLaterUpdateCount2 = ptr;
                ptr++;
                tlvLen++;

                // Update that we have processed this MLD group
                mldsprocessed[eProfile1->mldGrouping[0] - 'A'] = 1;

                for (j = 0; j < mapData->eProfileCnt && maxNumAPLinks > 0; j++) {

                    eProfileMatcher2 = &mapData->eProfileMatcher[j];
                    if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == eProfileMatcher2->matcherType) {
                        eProfile2 = &eProfileMatcher2->typeParams.alParams.eprofile;

                        /* Free the memory if already pointing to allocated memory */
                        if (opclass) {
                            free((void *)opclass);
                            opclass = NULL;
                        }
                        opclass = strdup(eProfileMatcher2->typeParams.alParams.opclass);
                    } else {
                        continue;
                    }

                    if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == eProfileMatcher2->matcherType) {
                        snprintf(alID, sizeof(alID), "%02x%02x%02x%02x%02x%02x ", sess->dest_addr[0], sess->dest_addr[1],
                            sess->dest_addr[2], sess->dest_addr[3], sess->dest_addr[4], sess->dest_addr[5]);
                        if (strncasecmp(alID, eProfileMatcher2->typeParams.alParams.alId,
                                                                    IEEE80211_ADDR_LEN * 2)) {
                            continue;
                        }
                    } else {
                        continue;
                    }

                    if(eProfile2->isBsta == APAC_TRUE)
                        continue;

                    if(strlen(eProfile2->mldGrouping) &&
                    (!strcmp(eProfile1->mldGrouping, eProfile2->mldGrouping))) {

                        (*ptrToLaterUpdateCount2)++;

                        /* AP_MLD_MAC_Addr
                        We wont send out valid mac address */
                        *ptr = 0x00;
                        ptr++;
                        tlvLen++;

                        /* RUID */
                        ieee1905MapGetRadioAddrBasicRadioTLV(sess,
                                        opclass, ruid);
                        os_memcpy(ptr, ruid, ETH_ALEN);
                        tlvLen += ETH_ALEN; /* MAC addr len */
                        ptr += ETH_ALEN;

                        /* ap mld mac address */
                        os_memset(ptr, 0x00, ETH_ALEN);
                        tlvLen += ETH_ALEN; /* MAC addr len */
                        ptr += ETH_ALEN;

                        /* Link ID 0-255 */
                        *ptr = 0x00;
                        ptr++;
                        tlvLen++;

                        // Reserved
                        os_memset(ptr, 0x00, 18);
                        ptr += 18;
                        tlvLen += 18;

                        maxNumAPLinks--;
                    }
                }
                if (opclass) {
                    free((void*)opclass);
                    opclass = NULL;
                }
            }
        }
    } else {
        for (i = 0; i < mapData->eProfileCnt; i++) {
            eProfileMatcher1 = &mapData->eProfileMatcher[i];
            if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC == eProfileMatcher1->matcherType) {
                genericParams1 = &eProfileMatcher1->typeParams.genericParams;
                if (genericParams1) {
                    eProfile1 = &mapData->eProfileSSID[genericParams1->ssidIndex].eprofile;
                }
            } else {
                continue;
            }

            if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC == eProfileMatcher1->matcherType) {
                if( eProfileMatcher1->typeParams.genericParams.mloEnabled == APAC_FALSE )
                    continue;
            }

            /* Check if AP vap with a valid MLD group that hasnt been processed yet*/
            for(k = 0; k < mldsprocessedGenericCount; k++) {
                if(strncmp(eProfile1->ssid, mldsprocessedGeneric[k], strlen(eProfile1->ssid)) == 0)
                    break;
            }

            if(k == mldsprocessedGenericCount) {

                (*ptrToLaterUpdateCount1)++;

                /* bit7 - AP_MLD_MAC_Addr_Valid
                bit6 - Disabled_Subchannel_Valid
                We wont send out valid mac address */
                *ptr = 0x00;
                ptr++;
                tlvLen++;
                /* ssidLength */
                *ptr = strlen(eProfile1->ssid);
                ptr++;
                tlvLen++;
                /* ssid */
                memcpy(ptr, eProfile1->ssid, strlen(eProfile1->ssid));
                ptr += strlen(eProfile1->ssid);
                tlvLen += strlen(eProfile1->ssid);
                /* ap mld mac address */
                os_memset(ptr, 0x00, ETH_ALEN);
                tlvLen += ETH_ALEN; /* MAC addr len */
                ptr += ETH_ALEN;

                /* STR - bit7
                NSTR - bit6
                EMLSR - bit5
                EMLMR - bit4 */
                // For now we only set STR Enabled
                *ptr = 0xA0;
                ptr++;
                tlvLen++;

                // Reserved
                os_memset(ptr, 0x00, 20);
                ptr += 20;
                tlvLen += 20;

                u8 maxNumAPLinks = ieee1905MapGetMaxNumLinksFromWiFi7TLV(sess);

                dprintf(MSG_INFO, "%s: maxNumAPLinks: %d\n", __func__, maxNumAPLinks);

                /* numAffiliatedAPs - will be updated as we fill AP MLD information*/
                *ptr = 0;
                ptrToLaterUpdateCount2 = ptr;
                ptr++;
                tlvLen++;

                // Update that we have processed this MLD group
                strlcpy(mldsprocessedGeneric[mldsprocessedGenericCount], eProfile1->ssid, 1024);
                mldsprocessedGenericCount++;

                dprintf(MSG_INFO, "%s: AP SSID: %s\n", __func__, eProfile1->ssid);

                u8 ruidList[MAX_AFFILIATED_AP_LINKS][IEEE80211_ADDR_LEN] = {0};

                for (j = 0; j < mapData->eProfileCnt && maxNumAPLinks > 0; j++) {

                    eProfileMatcher2 = &mapData->eProfileMatcher[j];
                    if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC == eProfileMatcher2->matcherType) {
                        genericParams2 = &eProfileMatcher2->typeParams.genericParams;
                        if (genericParams2) {
                            eProfile2 = &mapData->eProfileSSID[genericParams2->ssidIndex].eprofile;
                        }
                    }
                    else {
                        continue;
                    }

                    if( eProfileMatcher2->typeParams.genericParams.mloEnabled == APAC_FALSE ) {
                        continue;
                    }

                    match = APAC_TRUE;
                    for (l = 0; l < eProfileMatcher2->typeParams.genericParams.numOpClassRanges; l++) {
                        match &= ieee1905MapGetRadioAddrBasicRadioTLVGeneric(sess,
                            eProfileMatcher2->typeParams.genericParams.opClassRanges[l].minOpClass,
                            eProfileMatcher2->typeParams.genericParams.opClassRanges[l].maxOpClass, ruid,
                            eProfileMatcher2->typeParams.genericParams.bandType);
                    }

                    /* Ensure opclass match else move onto next entry */
                    if(match == APAC_FALSE)
                        continue;

                    for (l = 0; l < MAX_AFFILIATED_AP_LINKS; l++) {
                        if (hyAreEqualMACAddrs(ruidList[l], ruid))
                            break;
                    }

                    /* Ensure that this RUID entry hasnt already been made for this MLD group*/
                    if(l != MAX_AFFILIATED_AP_LINKS)
                        continue;

                    if(strlen(eProfile2->ssid) &&
                    (!strcmp(eProfile1->ssid, eProfile2->ssid))) {

                        (*ptrToLaterUpdateCount2)++;

                        /* AP_MLD_MAC_Addr
                        We wont send out valid mac address */
                        *ptr = 0x00;
                        ptr++;
                        tlvLen++;


                        dprintf(MSG_INFO, "%s: Affiliated AP RUID: %02x:%02x:%02x:%02x:%02x:%02x\n", __func__,
                            ruid[0], ruid[1], ruid[2], ruid[3], ruid[4], ruid[5]);

                        if (maxNumAPLinks <= MAX_AFFILIATED_AP_LINKS) {
                            hyCopyMACAddr(ruid, ruidList[MAX_AFFILIATED_AP_LINKS - maxNumAPLinks]);
                        }
                        os_memcpy(ptr, ruid, ETH_ALEN);
                        tlvLen += ETH_ALEN; /* MAC addr len */
                        ptr += ETH_ALEN;

                        /* ap mld mac address */
                        os_memset(ptr, 0x00, ETH_ALEN);
                        tlvLen += ETH_ALEN; /* MAC addr len */
                        ptr += ETH_ALEN;

                        /* Link ID 0-255 */
                        *ptr = 0x00;
                        ptr++;
                        tlvLen++;

                        // Reserved
                        os_memset(ptr, 0x00, 18);
                        ptr += 18;
                        tlvLen += 18;

                        maxNumAPLinks--;
                    }
                }
            }
        }
    }

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    TLV = ieee1905TLVGetNext(TLV);
    tlvLen = 0;
    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_BSTA_MLD_CONFIG);
    ptr = ieee1905TLVValGet(TLV);

    /* bit7 - bSTA_MLD_MAC_Addr_Valid
       bit6 - AP_MLD_MAC_Addr_valid
       We wont send out valid mac address */
    *ptr = 0x00;
    ptr++;
    tlvLen++;

    /* bsta mld mac address */
    os_memset(ptr, 0x00, ETH_ALEN);
    tlvLen += ETH_ALEN; /* MAC addr len */
    ptr += ETH_ALEN;

    /* ap mld mac address */
    os_memset(ptr, 0x00, ETH_ALEN);
    tlvLen += ETH_ALEN; /* MAC addr len */
    ptr += ETH_ALEN;

    /* STR - bit7
       NSTR - bit6
       EMLSR - bit5
       EMLMR - bit4 */
    // For now we only set EMLMR Enabled
    *ptr = 0xA0;
    ptr++;
    tlvLen++;

    // 17 Reserved bytes:
    os_memset(ptr, 0x00, 17);
    ptr += 17;
    tlvLen += 17;

    /* For PF Compliant */
    if (apacHyfiMapPfComplianceEnabled(mapData)) {
        for (i = 0; i < mapData->eBstaMldCnt; i++) {
            eProfileMatcher1 = &mapData->eBstaProfileMatcher[i];
            if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == eProfileMatcher1->matcherType) {
                eProfile1 = &eProfileMatcher1->typeParams.alParams.eprofile;
            }

            if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == eProfileMatcher1->matcherType) {
                snprintf(alID, sizeof(alID), "%02x%02x%02x%02x%02x%02x ", sess->dest_addr[0], sess->dest_addr[1],
                    sess->dest_addr[2], sess->dest_addr[3], sess->dest_addr[4], sess->dest_addr[5]);
                if (strncasecmp(alID, eProfileMatcher1->typeParams.alParams.alId,
                                                            IEEE80211_ADDR_LEN * 2)) {
                    continue;
                }
            }

            foundBstaMld = APAC_TRUE;

            /* Num_AffiliatedbSTA */
            if (eProfile1)
                *ptr = eProfile1->numBstaRadios;
            else
                *ptr = 0;
            //* KW check */
            u8 eP1numBstaRadios = *ptr;
            ptr++;
            tlvLen++;

            for(j = 0; j < eP1numBstaRadios; j++) {
                /* Affiliated_bSTA_MAC_Addr_Valid */
                *ptr = 0x00;
                ptr++;
                tlvLen++;

                /* RUID */
                os_memcpy(ptr, eProfile1->bstaRuids[j].ether_addr_octet, ETH_ALEN);
                tlvLen += ETH_ALEN; /* MAC addr len */
                ptr += ETH_ALEN;

                /* Affilated_bSTA_MAC_Addr
                We wont send out valid mac address */
                os_memset(ptr, 0xFF, ETH_ALEN);
                tlvLen += ETH_ALEN; /* MAC addr len */
                ptr += ETH_ALEN;

                // Reserved
                os_memset(ptr, 0x00, 19);
                ptr += 19;
                tlvLen += 19;
            }
        }
    } else {
        for (i = 0; i < mapData->eProfileCnt; i++) {
            eProfileMatcher1 = &mapData->eProfileMatcher[i];
            if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC == eProfileMatcher1->matcherType) {
                genericParams1 = &eProfileMatcher1->typeParams.genericParams;
                if (genericParams1) {
                    eProfile1 = &mapData->eProfileSSID[genericParams1->ssidIndex].eprofile;
                }
            } else {
                continue;
            }

            if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC == eProfileMatcher1->matcherType) {
                if( eProfileMatcher1->typeParams.genericParams.mloEnabled == APAC_FALSE )
                    continue;
            }

            if(eProfile1->isBackhaul == APAC_FALSE)
                    continue;

            dprintf(MSG_INFO, "%s: BSTA SSID: %s\n", __func__, eProfile1->ssid);

            foundBstaMld = APAC_TRUE;

           *ptr = 0;
            ptrToLaterUpdateCount2 = ptr;
            ptr++;
            tlvLen++;

            u8 ruidList[MAX_AFFILIATED_BSTA_LINKS][IEEE80211_ADDR_LEN] = {0};
            u8 ruidEntryCount = 0;

            for (j = 0; j < mapData->eProfileCnt; j++) {

                eProfileMatcher2 = &mapData->eProfileMatcher[j];
                if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC == eProfileMatcher2->matcherType) {
                    genericParams2 = &eProfileMatcher2->typeParams.genericParams;
                    if (genericParams2) {
                        eProfile2 = &mapData->eProfileSSID[genericParams2->ssidIndex].eprofile;
                    }
                }
                else {
                    continue;
                }

                if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC == eProfileMatcher2->matcherType) {
                    if( eProfileMatcher2->typeParams.genericParams.mloEnabled == APAC_FALSE )
                        continue;
                } else {
                    continue;
                }

                if(eProfile2->isBackhaul == APAC_FALSE)
                    continue;

                if(strlen(eProfile2->ssid) &&
                  (strcmp(eProfile1->ssid, eProfile2->ssid)))
                  continue;

                match = APAC_TRUE;
                for (l = 0; l < eProfileMatcher2->typeParams.genericParams.numOpClassRanges; l++) {
                    match &= ieee1905MapGetRadioAddrBasicRadioTLVGeneric(sess,
                        eProfileMatcher2->typeParams.genericParams.opClassRanges[l].minOpClass,
                        eProfileMatcher2->typeParams.genericParams.opClassRanges[l].maxOpClass, ruid,
                        eProfileMatcher2->typeParams.genericParams.bandType);
                }

                /* Ensure opclass match else move onto next entry */
                if(match == APAC_FALSE)
                    continue;

                for (l = 0; l < MAX_AFFILIATED_BSTA_LINKS; l++) {
                    if (hyAreEqualMACAddrs(ruidList[l], ruid))
                        break;
                }

                /* Ensure that this RUID entry hasnt already been made for this MLD group*/
                if(l != MAX_AFFILIATED_BSTA_LINKS)
                    continue;

                (*ptrToLaterUpdateCount2)++;

                /* Affiliated_bSTA_MAC_Addr_Valid */
                *ptr = 0x00;
                ptr++;
                tlvLen++;

                os_memcpy(ptr, ruid, ETH_ALEN);
                tlvLen += ETH_ALEN; /* MAC addr len */
                ptr += ETH_ALEN;

                hyCopyMACAddr(ruid, ruidList[ruidEntryCount]);
                ruidEntryCount++;

                dprintf(MSG_INFO, "%s: Affliated bSTA RUID: %02x:%02x:%02x:%02x:%02x:%02x\n", __func__,
                                            ruid[0], ruid[1], ruid[2], ruid[3], ruid[4], ruid[5]);

                /* Affilated_bSTA_MAC_Addr
                We wont send out valid mac address */
                os_memset(ptr, 0xFF, ETH_ALEN);
                tlvLen += ETH_ALEN; /* MAC addr len */
                ptr += ETH_ALEN;

                // Reserved
                os_memset(ptr, 0x00, 19);
                ptr += 19;
                tlvLen += 19;
            }
            break;
        }
    }

    if(foundBstaMld == APAC_FALSE) {
        // Found no bsta mld configuration. Set Num_AffiliatedbSTA to 0
        *ptr = 0;
        ptr++;
        tlvLen++;
    }

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    TLV = ieee1905TLVGetNext(TLV);

    return TLV;
}

/// AKM Suites
#define DPP_AKM_SUITE_OUI_PSK 0x000fac
#define DPP_AKM_SUITE_OUI_SAE 0x000fac
#define AKM_SUITE_TYPE_SAE 8
#define AKM_SUITE_TYPE_PSK 2
#define MAP_NUM_AKM 2
apacBool_e ieee1905MapGetAkmSuiteCap(ieee1905AKMSuiteCapabilities_t *akmSuiteCap) {
    u_int8_t akm_oui_psk[IEEE1905_SIZE_OF_OUI] = {0x00, 0x0f, 0xac};
    u_int8_t akm_oui_sae[IEEE1905_SIZE_OF_OUI] = {0x00, 0x0f, 0xac};

    // AKM for BH
    akmSuiteCap->numBhAKM = MAP_NUM_AKM;

    // Add PSK AKM
    memcpy(akmSuiteCap->bhAKMSuite[0].oui, akm_oui_psk, IEEE1905_SIZE_OF_OUI);
    akmSuiteCap->bhAKMSuite[0].type = AKM_SUITE_TYPE_PSK;

    // Add SAE AKM
    memcpy(akmSuiteCap->bhAKMSuite[1].oui, akm_oui_sae, IEEE1905_SIZE_OF_OUI);
    akmSuiteCap->bhAKMSuite[1].type = AKM_SUITE_TYPE_SAE;

    // AKM for FH
    akmSuiteCap->numFhAKM = MAP_NUM_AKM;

    // Add PSK AKM
    memcpy(akmSuiteCap->fhAKMSuite[0].oui, akm_oui_psk, IEEE1905_SIZE_OF_OUI);
    akmSuiteCap->fhAKMSuite[0].type = AKM_SUITE_TYPE_PSK;

    // Add SAE AKM
    memcpy(akmSuiteCap->fhAKMSuite[1].oui, akm_oui_sae, IEEE1905_SIZE_OF_OUI);
    akmSuiteCap->fhAKMSuite[1].type = AKM_SUITE_TYPE_SAE;

    return APAC_TRUE;
}

ieee1905TLV_t *ieee1905MapAddAKMSuiteCapabilitiesTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                     apacHyfi20Data_t *pData) {
    u_int16_t tlvLen = 0;
    u_int8_t *ptr = NULL;

    apacHyfi20TRACE();

    TLV = ieee1905TLVGetNext(TLV);
    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_AKM_SUITE_CAP);
    ptr = ieee1905TLVValGet(TLV);

    ieee1905AKMSuiteCapabilities_t akmSuiteCap;
    if (ieee1905MapGetAkmSuiteCap(&akmSuiteCap) == APAC_FALSE) {
        return TLV;
    }

    *ptr = akmSuiteCap.numBhAKM;
    ptr++;
    tlvLen++;

    for (u_int8_t k = 0; k < akmSuiteCap.numBhAKM; k++) {
        memcpy(ptr, akmSuiteCap.bhAKMSuite[k].oui, IEEE1905_SIZE_OF_OUI);
        ptr += IEEE1905_SIZE_OF_OUI;
        tlvLen += IEEE1905_SIZE_OF_OUI;

        *ptr = akmSuiteCap.bhAKMSuite[k].type;
        ptr++;
        tlvLen++;
    }

    *ptr = akmSuiteCap.numFhAKM;
    ptr++;
    tlvLen++;

    for (u_int8_t k = 0; k < akmSuiteCap.numFhAKM; k++) {
        memcpy(ptr, akmSuiteCap.fhAKMSuite[k].oui, IEEE1905_SIZE_OF_OUI);
        ptr += IEEE1905_SIZE_OF_OUI;
        tlvLen += IEEE1905_SIZE_OF_OUI;

        *ptr = akmSuiteCap.fhAKMSuite[k].type;
        ptr++;
        tlvLen++;
    }

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    dprintf(MSG_INFO, "Added AKM Suite Capabilities framelen %d tlvlen %d \n", *Len,
            tlvLen);

    printMsg((u8 *)TLV, tlvLen, MSG_DEBUG);
    return TLV;
}

ieee1905TLV_t *ieee1905Map2AddApCapTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                       apacHyfi20Data_t *pData) {
    u_int16_t tlvLen = 0;
    u_int8_t *ptr = NULL;
    apacPifMap_t *pIFMapData = NULL;
    apacMapData_t *map = NULL;

    apacHyfi20TRACE();

    map = HYFI20ToMAP(pData);

    TLV = ieee1905TLVGetNext(TLV);
    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_R2_APCAP);
    ptr = ieee1905TLVValGet(TLV);

    pIFMapData = &pData->ap[band].pIFMapData;

    if (!pIFMapData) {
        return TLV;
    }

    u_int8_t maxNumSerPriorRules = map->mapMaxServicePRules;
    u_int8_t byteCounterUnits = (map->vEnabled >= APAC_MAP_VERSION_2) ? 1 : 0;
    u_int8_t servicePrioritizationEnabled = (map->vEnabled >= APAC_MAP_VERSION_3) ? 1 : 0;
    u_int8_t trafficSepEnabled = apacHyfiMapIsTrafficSeparationEnabled(map) ? 1 : 0;
    u_int8_t maxTotalNumVID = map->numVlanSupported;

    *ptr++ = maxNumSerPriorRules;
    tlvLen++;

    /* reserved */
    ptr++;
    tlvLen++;

    *ptr = 0;
    *ptr |= ((byteCounterUnits & 0x03) << 6);
    *ptr |= servicePrioritizationEnabled == 1 ? (1 << 5) : 0;
    *ptr |= trafficSepEnabled == 1 ? (1 << 3) : 0;
    ptr++;
    tlvLen++;

    *ptr++ = maxTotalNumVID;
    tlvLen++;

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    dprintf(MSG_INFO, "Added MAPv2 AP Capability TLV(Enrollee) framelen %d tlvlen %d \n", *Len,
            tlvLen);

    printMsg((u8 *)TLV, tlvLen, MSG_DEBUG);
    return TLV;
}

ieee1905TLV_t *ieee1905Map2AddApRadioAdvancedCapTLV(ieee1905TLV_t *TLV, u_int32_t *Len, u8 band,
                                                    apacHyfi20Data_t *pData) {
    u_int16_t tlvLen = 0;
    u_int8_t *ptr = NULL;
    apacPifMap_t *pIFMapData = NULL;
    apacMapData_t *map = NULL;

    apacHyfi20TRACE();

    map = HYFI20ToMAP(pData);

    TLV = ieee1905TLVGetNext(TLV);
    ieee1905TLVTypeSet(TLV, IEEE1905_TLV_TYPE_AP_RADIO_ADVANCED_CAP);
    ptr = ieee1905TLVValGet(TLV);

    pIFMapData = &pData->ap[band].pIFMapData;

    if (!pIFMapData) {
        return TLV;
    }

    os_memcpy(ptr, pData->ap[band].radio_mac, ETH_ALEN);
    tlvLen += ETH_ALEN; /* MAC addr len */
    ptr += ETH_ALEN;

    u_int8_t mapMixNotSupported = map->mapR1R2MixNotSupported;
    *ptr = mapMixNotSupported ? 1 << 7 : 0 << 7;
    //mapMixNotSupported
    *ptr |= 1 << 6;
    // MSCS
    *ptr |= 1 << 5;
    // SCS
    *ptr |= 1 << 4;
    // QoS MAP
    *ptr |= 1 << 3;
    // DSCP Policy
    *ptr |= 1 << 2;
    // QM SCS Traffic Description
    *ptr |= 1 << 1;
    ptr++;
    tlvLen++;

    ieee1905TLVLenSet(TLV, tlvLen, *Len);
    dprintf(MSG_INFO, "Added MAPv2 Advanced Radio Cap TLV(Enrollee) framelen %d tlvlen %d \n", *Len,
            tlvLen);

    printMsg((u8 *)TLV, tlvLen, MSG_DEBUG);
    return TLV;
}

u8 ieee1905MapParseBasicRadioTLV(u8 *val, u_int32_t Len, u8 *maxBss, u8 minop, u8 maxop,
                                 apacBool_e checkAllOpClasses) {
    u8 *ptr = NULL;
    u_int16_t i = 0, j = 0, unoperable = 0;
    u8 opclassCnt = 0, opclass = 0;
    apacBool_e retv = APAC_FALSE;

    apacHyfi20TRACE();

    ptr = val;
    ptr += ETH_ALEN;  // skipping radio mac address.

    *maxBss = *ptr++;
    opclassCnt = *ptr++;

    dprintf(MSG_INFO, "Received M1 maxbss %d opclasscnt %d  \n", *maxBss, opclassCnt);
    dprintf(MSG_INFO, "Checking match against minop %u maxop %u checkAllOpClasses %u\n", minop,
            maxop, checkAllOpClasses);

    for (i = 0; i < opclassCnt; i++) {
        opclass = *ptr++;
        ptr++;  // skipping tx power
        unoperable = *ptr++;

        for (j = 0; j < unoperable; j++) {
            ptr++;
        }

        // Does this operating class fall within the range?
        if (opclass >= minop && opclass <= maxop) {
            retv = APAC_TRUE;
        } else {
            retv = APAC_FALSE;
        }

        if (retv || !checkAllOpClasses) {
            break;
        }
    }

    return retv;
}

#define REG_MAX_CHANNELS_PER_OPERATING_CLASS        70

typedef struct reg_opclass_map_t {
    uint8_t op_class;
    uint8_t channels[REG_MAX_CHANNELS_PER_OPERATING_CLASS];
}reg_opclass_map_t;

reg_opclass_map_t g_reg_global_tbl[] = {
        {131,
            {1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41, 45, 49, 53, 57, 61, 65, 69, 73, 77, 81, 85, 89,
             93, 97, 101, 105, 109, 113, 117, 121, 125, 129, 133, 137, 141, 145, 149, 153, 157, 161,
             165, 169, 173, 177, 181, 185, 189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233 }},
        {0, {0}},
};

#define CHAN_6GHZ_LOW_END 113
#define CHAN_6GHZ_HIGH_START 117

u8 ieee1905MapParseNonOperableChannel(u8 *val, u_int32_t Len, u8 bandType) {
    u8 *ptr = NULL;
    u_int16_t i = 0, j = 0, k = 0, m = 0, unoperable = 0;
    u8 opclassCnt = 0, opclass = 0;
    apacBool_e retv = APAC_FALSE;
    ieee1905APRadioBasicCapabilities_t radioCap = {0};
    u_int8_t opChanMin=0xff, opChanMax=0, matchFound;
    apacHyfi20WifiBandType_e opBandType = APAC_WIFI_BANDTYPE_FULL;

    apacHyfi20TRACE();

    ptr = val;
    ptr += ETH_ALEN;  // skipping radio mac address.

    ptr++; // skip maxBss
    opclassCnt = *ptr++;
    radioCap.numSupportedOpClasses = opclassCnt;

    for (i = 0; i < opclassCnt; i++) {
        opclass = *ptr++;
        radioCap.opClasses[i].opClass = opclass;
        ptr++;  // skipping tx power
        unoperable = *ptr++;
        radioCap.opClasses[i].numNonOperChan = unoperable;

        for (j = 0; j < unoperable; j++) {
            radioCap.opClasses[i].nonOperChanNum[j] = *ptr++;
        }
    }

    for (i = 0; i < radioCap.numSupportedOpClasses; i++) {
        opclass = radioCap.opClasses[i].opClass;
        for(j = 0; j < sizeof(g_reg_global_tbl)/sizeof(g_reg_global_tbl[0]) ; j++) {
            if ( opclass== g_reg_global_tbl[j].op_class) {
                for (k = 0; k < REG_MAX_CHANNELS_PER_OPERATING_CLASS; k++) {
                    if(!g_reg_global_tbl[j].channels[k])
                        break;
                    matchFound = APAC_FALSE;
                    for(m = 0; m < radioCap.opClasses[i].numNonOperChan; m++) {
                        if(g_reg_global_tbl[j].channels[k] == radioCap.opClasses[i].nonOperChanNum[m]) {
                            matchFound = APAC_TRUE;
                        }
                        if(matchFound)
                            break;
                    }

                    if(matchFound)
                        continue;
                    if (opChanMin > g_reg_global_tbl[j].channels[k]) {
                        opChanMin = g_reg_global_tbl[j].channels[k];
                    }

                    if (opChanMax < g_reg_global_tbl[j].channels[k]) {
                        opChanMax = g_reg_global_tbl[j].channels[k];
                    }
                }
            }
        }
    }

    dprintf(MSG_INFO, "%s: opChanMin %d opChanMax %d bandType %d\n", __func__, opChanMin, opChanMax, bandType);
    if(opChanMin < CHAN_6GHZ_LOW_END && opChanMax <= CHAN_6GHZ_LOW_END) {
        dprintf(MSG_INFO, "%s: 6G Low detected\n", __func__);
        opBandType = APAC_WIFI_BANDTYPE_LOW;
    }
    if (opChanMin >= CHAN_6GHZ_HIGH_START && opChanMax > CHAN_6GHZ_HIGH_START) {
            dprintf(MSG_INFO, "%s: 6G high detected\n", __func__);
        opBandType = APAC_WIFI_BANDTYPE_HIGH;
    }

    if(opBandType == bandType) {
        dprintf(MSG_INFO, "%s: matched band type %d \n", __func__, bandType);
        retv = APAC_TRUE;
    }
    return retv;
}

u8 ieee1905MapParseTrafficSepTLV(struct apac_wps_session *sess,
                                 mapServiceTrafficSepPolicy_t *trafficSepPolicy) {
    u_int8_t *content = NULL;
    content = sess->trafficSepPolicy;

    trafficSepPolicy->numOfSSIDs = *content;
    content++;

    size_t k = 0;
    for (k = 0; k < trafficSepPolicy->numOfSSIDs; k++) {
        trafficSepPolicy->interfaceConf[k].ssidLen = *content;
        content++;

        memcpy(trafficSepPolicy->interfaceConf[k].ssid, content,
               trafficSepPolicy->interfaceConf[k].ssidLen);
        content += trafficSepPolicy->interfaceConf[k].ssidLen;

        u_int16_t vlanID;
        memcpy(&vlanID, content, sizeof(vlanID));
        trafficSepPolicy->interfaceConf[k].vlanID = ntohs(vlanID);
        content += sizeof(vlanID);
    }

    return APAC_TRUE;
}

u8 apac_map_get_configured_maxbss(struct apac_wps_session *sess)
{
    apacHyfi20Data_t *hyfi20 = NULL;
    apacMapData_t *map = NULL;

    apacHyfi20TRACE();

    hyfi20 = sess->pData;

    map = HYFI20ToMAP(hyfi20);

    return map->MapConfMaxBss;
}

u8 ieee1905MapParseAgentApMldConfigTLV(struct apac_wps_session *sess,
                                       mapServiceApAgentMldConfig_t *apAgentMldConfig)
{
    u_int8_t *content = NULL;
    content = sess->agentApMldConfig;

    apAgentMldConfig->numAPMLDs = *content;
    content++;

    size_t k = 0;
    for (k = 0; k < apAgentMldConfig->numAPMLDs; k++) {

        apAgentMldConfig->apMLDConf[k].apMldMacAddrValid = (*content & 0x80) ? APAC_TRUE : APAC_FALSE;
        content++;

        apAgentMldConfig->apMLDConf[k].ssidLen = *content;
        content++;

        memcpy(apAgentMldConfig->apMLDConf[k].ssid, content,
               apAgentMldConfig->apMLDConf[k].ssidLen);
        content += apAgentMldConfig->apMLDConf[k].ssidLen;

        memcpy(apAgentMldConfig->apMLDConf[k].apMldMacAddr, content,
               ETH_ALEN);
        content += ETH_ALEN;

        apAgentMldConfig->apMLDConf[k].strEnable = (*content & 0x80) ? APAC_TRUE : APAC_FALSE;
        apAgentMldConfig->apMLDConf[k].nstrEnable = (*content & 0x40) ? APAC_TRUE : APAC_FALSE;
        apAgentMldConfig->apMLDConf[k].emlsrEnable = (*content & 0x20) ? APAC_TRUE : APAC_FALSE;
        apAgentMldConfig->apMLDConf[k].emlmrEnable = (*content & 0x10) ? APAC_TRUE : APAC_FALSE;
        content++;

        // Skip reserved bytes
        content += 20;

        apAgentMldConfig->apMLDConf[k].numAffiliatedAPs = *content;
        content++;

        size_t l = 0;
        for (l = 0; l < apAgentMldConfig->apMLDConf[k].numAffiliatedAPs; l++) {

            apAgentMldConfig->apMLDConf[k].affiliatedAPConf[l].affiliatedApMacAddrValid =
                                                (*content & 0x80) ? APAC_TRUE : APAC_FALSE;
            content++;

            memcpy(apAgentMldConfig->apMLDConf[k].affiliatedAPConf[l].RUID,
                                                content, ETH_ALEN);
            content += ETH_ALEN;

            memcpy(apAgentMldConfig->apMLDConf[k].affiliatedAPConf[l].affiliatedApMacAddr,
                                                content, ETH_ALEN);
            content += ETH_ALEN;

            apAgentMldConfig->apMLDConf[k].affiliatedAPConf[l].LinkID = *content;
            content++;

            // Skip reserved bytes
            content += 18;
        }
    }

    return APAC_TRUE;
}

u8 ieee1905MapParseBstaMldConfigTLV(struct apac_wps_session *sess,
                                    mapServiceBstaMldConfig_t *bstaMldConfig)
{
    u_int8_t *content = NULL;
    content = sess->bstaMldConfig;

    bstaMldConfig->bstaMldMacAddrValid = (*content & 0x80) ? APAC_TRUE : APAC_FALSE;
    bstaMldConfig->apMldMacAddrValid = (*content & 0x40) ? APAC_TRUE : APAC_FALSE;
    content++;

    memcpy(bstaMldConfig->bstaMldMacAddr, content, SIZE_MAC_ADDR);
    content += SIZE_MAC_ADDR;

    memcpy(bstaMldConfig->apMldMacAddr, content, SIZE_MAC_ADDR);
    content += SIZE_MAC_ADDR;

    bstaMldConfig->strEnable = (*content & 0x80) ? APAC_TRUE : APAC_FALSE;
    bstaMldConfig->nstrEnable = (*content & 0x40) ? APAC_TRUE : APAC_FALSE;
    bstaMldConfig->emlsrEnable = (*content & 0x20) ? APAC_TRUE : APAC_FALSE;
    bstaMldConfig->emlmrEnable = (*content & 0x10) ? APAC_TRUE : APAC_FALSE;
    content++;

    // Skip reserved bytes
    content += 17;

    bstaMldConfig->numAffiliatedbSTAs = *content;
    content++;

    size_t k = 0;
    for (k = 0; k < bstaMldConfig->numAffiliatedbSTAs; k++) {

        bstaMldConfig->affiliatedBstaConf[k].affiliatedBstaMacAddrValid = (*content & 0x80) ? APAC_TRUE : APAC_FALSE;
        content++;

        memcpy(bstaMldConfig->affiliatedBstaConf[k].RUID,
                                            content, SIZE_MAC_ADDR);
        content += SIZE_MAC_ADDR;

        memcpy(bstaMldConfig->affiliatedBstaConf[k].affiliatedBstaMacAddr,
                                                        content, SIZE_MAC_ADDR);
        content += SIZE_MAC_ADDR;

        // Skip reserved bytes
        content += 19;

    }

    return APAC_TRUE;
}

/**
 * @brief Populate the parameters for instantiating a BSS from an encryption
 *        profile.
 *
 * @param [in] eProfile  the encryption profile to copy from
 * @param [out] ap  the structure into which to copy the data
 */
static void apac_map_copy_apinfo_from_eprofile(const apacMapEProfile_t *eProfile,
                                               apacHyfi20AP_t *ap, u8 DbgLvl) {
    ap->ssid_len = os_strlen((const char *)eProfile->ssid);
    memcpy(ap->ssid, eProfile->ssid, ap->ssid_len);
    ap->ssid[ap->ssid_len] = '\0';  // only for logging

    ap->auth = eProfile->auth;
    ap->encr = eProfile->encr;

    if (eProfile->isFronthaul) {
        ap->mapBssType = MAP_BSS_TYPE_FRONTHAUL;
    }
    if (eProfile->isBackhaul) {
        ap->mapBssType |= MAP_BSS_TYPE_BACKHAUL;
    }
    if (eProfile->map1bSTAAssocDisallowed) {
        ap->mapBssType |= MAP2_R1_BSTA_ASSOC_DISALLOW;
    }
    if (eProfile->map2bSTAAssocDisallowed) {
        ap->mapBssType |= MAP2_R2_ABOVE_BSTA_ASSOC_DISALLOW;
    }
    ap->nw_key_len = os_strlen(eProfile->nw_key);
    os_memcpy(ap->nw_key, eProfile->nw_key, ap->nw_key_len);
    ap->nw_key[ap->nw_key_len] = '\0';  // only for logging

#if DOT1X_SUPPORT
    if (eProfile->isFronthaul) {
        int length;
        if (eProfile->auth & WPS_AUTHTYPE_WPA2 ||
            eProfile->auth & WPS_AUTHTYPE_WPA ||
            eProfile->auth & WPS_AUTHTYPE_WPAMIXED ||
            eProfile->auth & WPS_AUTHTYPE_WPA3_SUITEB) {
            if (eProfile->encr & WPS_ENCRTYPE_AES ||
                eProfile->encr & WPS_ENCRTYPE_TKIP ||
                eProfile->suite_b) {

                if ( !eProfile->auth_primary_server )
                    return;

                dprintf(MSG_MSGDUMP, "%s:%d> primary server=%s\n", __func__, __LINE__,
                          eProfile->auth_primary_server );
                length = os_strlen(eProfile->auth_primary_server);
                os_memcpy(ap->auth_server, eProfile->auth_primary_server, length);
                ap->auth_server[length]='\0';

                length = os_strlen(eProfile->auth_primary_secret);
                os_memcpy(ap->auth_secret, eProfile->auth_primary_secret, length);
                ap->auth_secret[length]='\0';

                ap->auth_port = eProfile->auth_primary_port;

                ap->ft_auth = eProfile->ft_auth;

                if (ap->ft_auth && eProfile->kh_key_hex) {
                    length = os_strlen(eProfile->kh_key_hex);
                    os_memcpy(ap->kh_key_hex, eProfile->kh_key_hex,length);
                    ap->kh_key_hex[length]='\0';

                    length = os_strlen(eProfile->mobility_domain);
                    os_memcpy(ap->mobility_domain, eProfile->mobility_domain,length);
                    ap->mobility_domain[length]='\0';
                }
                if ( eProfile->auth_secondary_server ) {
                    length = os_strlen(eProfile->auth_secondary_server);
                    os_memcpy(ap->auth_server2, eProfile->auth_secondary_server, length);
                    ap->auth_server2[length]='\0';

                    length = os_strlen(eProfile->auth_secondary_secret);
                    os_memcpy(ap->auth_secret2, eProfile->auth_secondary_secret, length);
                    ap->auth_secret2[length]='\0';

                    ap->auth_port2 = eProfile->auth_secondary_port;
                    dprintf(MSG_MSGDUMP, "MAP AUTH SERVER2 = %s \n", ap->auth_server2 );
                    dprintf(MSG_MSGDUMP, "MAP AUTH SECRET2 = %s \n", ap->auth_secret2 );
                    dprintf(MSG_MSGDUMP, "MAP AUTH PORT2 = %d \n", ap->auth_port2);
                }
                if (eProfile->auth & WPS_AUTHTYPE_WPA3_SUITEB && eProfile->suite_b) {
                    ap->suite_b = eProfile->suite_b;
                    length = os_strlen(eProfile->group_mgmt_cipher);
                    os_memcpy(ap->group_mgmt_cipher, eProfile->group_mgmt_cipher, length);
                    ap->group_mgmt_cipher[length]='\0';
                    ap->group_rekey_period = eProfile->group_rekey_period;
                    ap->ieee80211w = eProfile->ieee80211w;

                    dprintf(MSG_MSGDUMP," MAP SUITEB = %d\n", ap->suite_b);
                    dprintf(MSG_MSGDUMP," MAP GROUP MGMT CIPHER = %s\n", ap->group_mgmt_cipher);
                    dprintf(MSG_MSGDUMP," MAP GROUP REKEY PERIOD = %d\n", ap->group_rekey_period);
                    dprintf(MSG_MSGDUMP," IEEE80211W = %d\n", ap->ieee80211w);
                }

                dprintf(MSG_MSGDUMP, "MAP AUTH SERVER = %s \n", ap->auth_server );
                dprintf(MSG_MSGDUMP, "MAP AUTH SECRET = %s \n", ap->auth_secret );
                dprintf(MSG_MSGDUMP, "MAP AUTH PORT = %d \n", ap->auth_port );
                dprintf(MSG_MSGDUMP, "MAP FT AUTH FLAG = %d \n",ap->ft_auth);
                dprintf(MSG_MSGDUMP, "MAP KH KEY HEX = %s \n", ap->kh_key_hex);
                dprintf(MSG_MSGDUMP, "MAP MOBILITY DOMAIN = %s", ap->mobility_domain);
            }
        }
    }
#endif
    dprintf(DbgLvl, "MAP SSID %s ssid len %d  \n", ap->ssid, ap->ssid_len);
    dprintf(DbgLvl, "MAP AUTH %x  \n", ap->auth);
    dprintf(DbgLvl, "MAP ENCR  %x \n", ap->encr);
    dprintf(DbgLvl, "MAP nw_key %s \n", ap->nw_key);
    dprintf(DbgLvl, "MAP nw_key len %d \n", ap->nw_key_len);
    dprintf(DbgLvl, "MAP Fronthaul  %d  \n", ap->mapBssType & MAP_BSS_TYPE_FRONTHAUL);
    dprintf(DbgLvl, "MAP Backhaul  %d  \n", ap->mapBssType & MAP_BSS_TYPE_BACKHAUL);
    dprintf(DbgLvl, "MAP R1 STA Assoc DisAllowed  %d  \n",
            ap->mapBssType & MAP2_R1_BSTA_ASSOC_DISALLOW);
    dprintf(DbgLvl, "MAP R2 STA Assoc DisAllowed  %d  \n",
            ap->mapBssType & MAP2_R2_ABOVE_BSTA_ASSOC_DISALLOW);

}

/**
 * @brief Populate the parameters for instantiating a BSS from the AL-specific
 *        encryption profile.
 *
 * @param [in] eProfileMatcher  the encryption profile to copy from
 * @param [out] ap  the structure into which to copy the data
 */
static void apac_map_copy_apinfo_al_specific(
    const apacMapEProfileMatcher_t *eProfileMatcher, apacHyfi20AP_t *ap) {
    const apacMapEProfile_t *eProfile = &eProfileMatcher->typeParams.alParams.eprofile;

    apac_map_copy_apinfo_from_eprofile(eProfile, ap, MSG_DEBUG);
}

/**
 * @brief Populate the parameters for instantiating a BSS from a generic
 *        encryption profile.
 *
 * @param [in] map  the overall structure for Multi-AP state
 * @param [in] eProfileMatcher  the generic encryption profile to copy from
 * @param [out] ap  the structure into which to copy the data
 */
static void apac_map_copy_apinfo_generic(
    const apacMapData_t *map, const apacMapEProfileMatcher_t *eProfileMatcher,
    apacHyfi20AP_t *ap, u8 DbgLvl) {
    const apacMapEProfile_t *eProfile =
        &map->eProfileSSID[eProfileMatcher->typeParams.genericParams.ssidIndex].eprofile;

    apac_map_copy_apinfo_from_eprofile(eProfile, ap, DbgLvl);
}

apacBool_e apac_map_copy_apinfo(apacMapData_t *map, u8 index, apacHyfi20AP_t *ap, u8 DbgLvl) {
    apacMapEProfileMatcher_t *eProfileMatcher = NULL;

    apacHyfi20TRACE();

    if (index >= APAC_MAXNUM_NTWK_NODES && (index != 0xff)) return APAC_FALSE;

    /// Default Profile for Teardown
    if (index == 0xff) {
        ap->ssid_len = os_strlen("teardown");
        memcpy(ap->ssid, "teardown", ap->ssid_len);
        ap->ssid[ap->ssid_len] = '\0';

        ap->auth = WPS_AUTHTYPE_WPA2PSK;
        ap->encr = WPS_ENCRTYPE_AES;

        ap->mapBssType = 0;

        ap->nw_key_len = os_strlen("teardown");
        os_memcpy(ap->nw_key, "teardown", ap->nw_key_len);
        ap->nw_key[ap->nw_key_len] = '\0';
        return APAC_TRUE;
    }

    eProfileMatcher = &map->eProfileMatcher[index];
    if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == eProfileMatcher->matcherType) {
        apac_map_copy_apinfo_al_specific(eProfileMatcher, ap);
        return APAC_TRUE;
    } else if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC == eProfileMatcher->matcherType) {
        apac_map_copy_apinfo_generic(map, eProfileMatcher, ap, DbgLvl);
        return APAC_TRUE;
    } else {
        dprintf(MSG_ERROR, "%s: Invalid profile matcher type %u for index %u\n",
                __func__, eProfileMatcher->matcherType, index);
        return APAC_FALSE;
    }
}

static apacBool_e apac_map_match_eprofile_al_specific(
    struct apac_wps_session *sess, const apacMapEProfileMatcher_t *eProfileMatcher,
    u8 *maxBSS) {
    char buf[1024] = { 0 };
    u8 minop = 0 , maxop = 0;
    const apacMapEProfile_t *eProfile = NULL;
    apacBool_e checkAllOperatingClasses = APAC_FALSE;

    snprintf(buf, sizeof(buf), "%02x%02x%02x%02x%02x%02x ", sess->dest_addr[0], sess->dest_addr[1],
            sess->dest_addr[2], sess->dest_addr[3], sess->dest_addr[4], sess->dest_addr[5]);

    if (!strncasecmp(buf, eProfileMatcher->typeParams.alParams.alId,
                     IEEE80211_ADDR_LEN * 2)) {
        eProfile = &eProfileMatcher->typeParams.alParams.eprofile;

        dprintf(MSG_MSGDUMP,
                "Checking AL-specific EProfile for %s: "
                "%s,%s,%s,0x%04x,0x%04x,%s,%d,%d \n",
                buf, eProfileMatcher->typeParams.alParams.alId,
                eProfileMatcher->typeParams.alParams.opclass, eProfile->ssid,
                eProfile->auth, eProfile->encr, eProfile->nw_key,
                eProfile->isBackhaul, eProfile->isFronthaul);

        if (!strncasecmp(eProfileMatcher->typeParams.alParams.opclass, "11x",
                         3)) {
            minop = 110;
            maxop = 120;
        } else if (!strncasecmp(eProfileMatcher->typeParams.alParams.opclass,
                                "12x", 3)) {
            minop = 121;
            /* Set max limit upto 40Mhz as 80Mhz opclass will have both low and high band
               channel information*/
            maxop = 127;//to compliant with WFA , ideally it should be 129
            checkAllOperatingClasses = APAC_TRUE;
        } else if (!strncasecmp(eProfileMatcher->typeParams.alParams.opclass,
                                "8x", 2)) {
            minop = 80;
            maxop = 89;
        } else if (!strncasecmp(eProfileMatcher->typeParams.alParams.opclass,
                                "13x", 3)) {
            minop = 131;
            maxop = 136;
        } else { //opclass not there
            dprintf(MSG_ERROR, "Unexpected op class %s\n",
                    eProfileMatcher->typeParams.alParams.opclass);
            return APAC_FALSE;
        }

        dprintf(MSG_DEBUG, "%s Profile based MinOp %d Maxop %d \n",__func__,
                minop, maxop);

        if (ieee1905MapParseBasicRadioTLV(
                sess->basicRadioCap, sess->basicRadioCapLen, maxBSS, minop, maxop,
                checkAllOperatingClasses /* checkAllOperatingClasses */)) {
            return APAC_TRUE;
        }
    }

    return APAC_FALSE;
}

/**
 * @brief Attempt to match the radio capabilities against this generic
 *        encryption profile matcher.
 *
 * @param [in] sess  the information for the WPS session (where the radio
 *                   capabilities can be found)
 * @param [in] eProfileMatcher  the matcher to evaluate against the radio
 *                              capabilities
 * @param [out] maxBSS  the number of BSSes the agent indicates it can support
 *                      on the radio
 *
 * @return APAC_TRUE if there was a match; otherwise APAC_FALSE
 */
static apacBool_e apac_map_match_eprofile_generic(
    struct apac_wps_session *sess, const apacMapEProfileMatcher_t *eProfileMatcher,
    u8 *maxBSS) {
    apacBool_e match = APAC_TRUE;
    apacHyfi20Data_t *hyfi20 = NULL;
    apacMapData_t *map = NULL;

    hyfi20 = sess->pData;

    map = HYFI20ToMAP(hyfi20);

    // Only is a match if all of the operating class ranges specified match
    u8 i;
    for (i = 0; i < eProfileMatcher->typeParams.genericParams.numOpClassRanges; ++i) {
        match &= ieee1905MapParseBasicRadioTLV(
            sess->basicRadioCap, sess->basicRadioCapLen, maxBSS,
            eProfileMatcher->typeParams.genericParams.opClassRanges[i].minOpClass,
            eProfileMatcher->typeParams.genericParams.opClassRanges[i].maxOpClass,
            APAC_TRUE /* checkAllOperatingClasses */);

        /* For 6GHz radio, do the profile match again with operating channel
           and opclass range*/
        if (match && (sess->pData->config.wpsConf->rf_bands == WPS_RFBAND_6GHZ ||
            sess->pData->config.wpsConf->rf_bands == WPS_RFBAND_6GHZ_LOW)) {

            if(map->skip6GSplitBandCheck) {
                if (sess->pData->config.wpsConf->rf_bands == WPS_RFBAND_6GHZ &&
                    eProfileMatcher->typeParams.genericParams.bandType == APAC_WIFI_BANDTYPE_FULL) {
                    dprintf(MSG_DEBUG, "%s Skip 6G SplitBand check and match full band (M1 RF band %d) \n",
                                    __func__,
                                    sess->pData->config.wpsConf->rf_bands);
                    return match;
                }
            } else {
                match &= ieee1905MapParseNonOperableChannel(sess->basicRadioCap, sess->basicRadioCapLen,
                                        eProfileMatcher->typeParams.genericParams.bandType);
            }
        }
    }

    return match;
}

/**
 * @brief Find the matching encryption profiles for the given radio based on
 *        its capabilities.
 *
 * @param [in] sess  the information for the WPS session (where the radio
 *                   capabilities can be found)
 * @param [out] list  array into which the matching encryption profiles will
 *                    be placed; must be at least APAC_MAXNUM_NTRK_NODES in
 *                    length
 * @param [out] requested_m2  the number of M2 messages (and thus the number of
 *                            BSSes) that the agent can support on the radio
 *
 * @return the number of profile matches (equivalently, the number of elements
 *         in list that are valid)
 */
u8 apac_map_get_eprofile(struct apac_wps_session *sess, u8 *list, u8 *requested_m2) {
    u8  maxBSS = 0, *listptr = NULL;
    u8 i = 0, profilecnt = 0, ssidIdx;
    apacMapEProfileMatcher_t *eProfileMatcher = NULL;
    apacHyfi20Data_t *hyfi20 = NULL;
    apacMapData_t *map = NULL;
    apacMapEProfile_t *eProfile = NULL;
    apacHyfi20Data_t *pApacData = sess->pData;
    apacBool_e match;

    apacHyfi20TRACE();

    hyfi20 = sess->pData;

    map = HYFI20ToMAP(hyfi20);

    listptr = list;
    *listptr = 0xff; //default value used in teardown
    *requested_m2 = 1; // max bss to send one teardown

    ieee1905MapParseBasicRadioTLV(sess->basicRadioCap, sess->basicRadioCapLen, &maxBSS, 0, 0,
                                  APAC_FALSE /* checkAllOpClasses */);

    if (hyfi20->config.role != APAC_REGISTRAR || map->eProfileCnt == 0 ) {
        dprintf(MSG_ERROR, "%s, not registrar or map file not found !\n", __func__);
        if (*listptr == 0xff) {
            for (i = 0; i < maxBSS; i++) {
                *listptr++ = 0xff;
            }
        }
        *requested_m2 = maxBSS;
        return profilecnt;
    }

    for(i = 0 ; i < map->eProfileCnt; i++) {
        match = APAC_FALSE;
        eProfileMatcher = &map->eProfileMatcher[i];

        if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_AL_SPECIFIC == eProfileMatcher->matcherType) {
            match = apac_map_match_eprofile_al_specific(sess, eProfileMatcher, &maxBSS);
        } else if (APAC_E_MAP_EPROFILE_MATCHER_TYPE_GENERIC == eProfileMatcher->matcherType) {
            match = apac_map_match_eprofile_generic(sess, eProfileMatcher, &maxBSS);
        } else {
            dprintf(MSG_ERROR, "%s: Unexpected matcher type at index %u\n", __func__, i);
        }

        if (match) {
            /* MapMaxBss count will be increased to clone additional FH VAP, in this case if Controller
               is not R1 then prevent cloning of R2 specific FH/BH VAP to R1 Agent */
            if (sess->mapVersion == APAC_MAP_VERSION_1 &&
                    apacHyfiMapIsEnabled(HYFI20ToMAP(pApacData)) >= APAC_MAP_VERSION_2) {
                ssidIdx = eProfileMatcher->typeParams.genericParams.ssidIndex;
                eProfile = &map->eProfileSSID[ssidIdx].eprofile;

                if ( eProfile && (eProfile->primaryVlanID != eProfile->vlanID
                        || eProfile->map1bSTAAssocDisallowed) ) {
                    dprintf(MSG_INFO, "%s: skipping R2 specific BSS information for R1 agent"
                            "(ssidKey:%s Primary/SecondaryVlanID-%d:%d R1/R2dissallowed-%d:%d)\n",
                            __func__, map->eProfileSSID[ssidIdx].ssidKey, eProfile->primaryVlanID, eProfile->vlanID,
                            eProfile->map1bSTAAssocDisallowed, eProfile->map2bSTAAssocDisallowed);
                    continue;
                }
            }

            dprintf(MSG_INFO, "%s: Matched eprofile #%u\n", __func__, i);

            *listptr++ = i;
            profilecnt++;

            if (eProfileMatcher->terminateMatching) {
                // No further matching should be done for this radio
                break;
            }
        }
    }

    if (profilecnt < maxBSS) {
        dprintf(MSG_INFO, "%s: No profile match found for remaining BSS = %u \n", __func__,
                maxBSS - profilecnt);
        *listptr += profilecnt;
        for (i = 0; i < maxBSS - profilecnt; i++) {
            *listptr++ = 0xff;
        }
    }
    *requested_m2 = maxBSS;

    return profilecnt;
}

u8 apac_map_parse_vendor_ext(struct apac_wps_session *sess,
        u8 *pos,
        u8 len, u8 *mapBssType)
{
    u32 vendor_id;
#define WPS_VENDOR_ID_WFA 14122 //37 2a
#define WFA_ELEM_MAP_BSS_CONFIGURATION 0x06
    apacHyfi20TRACE();

    if (len < 3) {
        dprintf(MSG_DEBUG, "WPS: Skip invalid Vendor Extension");
        return 0;
    }

    vendor_id = WPA_GET_BE24(pos);
    switch (vendor_id) {
        case WPS_VENDOR_ID_WFA:
            len -=3;
            pos +=3;
            const u8 *end = pos + len;
            u8 id, elen;

            while (end - pos >= 2) {
                id = *pos++;
                elen = *pos++;
                if (elen > end - pos)
                    break;

                switch(id) {
                    case WFA_ELEM_MAP_BSS_CONFIGURATION:
                        dprintf(MSG_MSGDUMP,"Received Map Bss Type %x \n", *pos);
                        /* Some third party devices includes BSTA bit for BH AP VAPs
                           which is not expected but still QC Agent is handling it by
                           clearing it. */
                        *mapBssType = (*pos & (~(MAP_BSS_TYPE_BSTA)));
                        if (apacHyfiMapIsEnabled(HYFI20ToMAP(sess->pData)) >= APAC_MAP_VERSION_2) {
                            if (*mapBssType & MAP_BSS_TYPE_BACKHAUL) {
                                /* If proper assoc disallowed bit is not sent by third party
                                   devices then Agent will by default consider it as R1 disallowed VAP.
                                   If the bit is set that will be taken care */
                                /* checking the R2 and above disallowed bit */
                                if (*mapBssType & MAP2_R2_ABOVE_BSTA_ASSOC_DISALLOW) {
                                    *mapBssType |= MAP2_R2_ABOVE_BSTA_ASSOC_DISALLOW;
                                } else {
                                /* setting R1 bSta disallowed */
                                    *mapBssType |= MAP2_R1_BSTA_ASSOC_DISALLOW;
                                }
                            }
                        }
                        dprintf(MSG_MSGDUMP,"After disallowed bit Map Bss Type %x \n", *mapBssType);
                        break;
                }
                pos += elen;
            }
    }
    return 0;
}

static apac_alid_db *apac_map_alid_db_read_db() {

    FILE *file = NULL;
    u8 alid_count = 0; // keep the type same as in apac_alid_db
    u32 alloc_size = 0;

    file = fopen(APAC_MAP_ALID_DB_FILE, "rb");
    if (!file) {
        dprintf(MSG_ERROR, "%s: failed to open file: %s\n", __func__, APAC_MAP_ALID_DB_FILE);
        return NULL;
    }

    if (fread(&alid_count, sizeof(alid_count), 1, file) < 0) {
        dprintf(MSG_ERROR, "%s: failed read alid count from file\n", __func__);
        fclose(file);
        return NULL;
    }

    if (alid_count > APAC_MAP_ALID_DB_MAX_ALID_COUNT) {
        dprintf(MSG_ERROR, "%s: invalid alid count %u\n", __func__, alid_count);
        fclose(file);
        return NULL;
    }

    // allocate memory
    alloc_size = sizeof(u8) + (alid_count * sizeof(apac_alid));
    apac_alid_db *alid_db = (apac_alid_db*) calloc(1, alloc_size);
    if (!alid_db) {
        dprintf(MSG_DEBUG, "%s: failed to allocate %u bytes of memory\n", __func__, alloc_size);
        return NULL;
    }

    // read the db entries
    for (u8 count = 0; count < alid_count; count++) {
        fread(&alid_db->alid[count], 1, sizeof(apac_alid), file);
    }
    alid_db->alid_count = alid_count;
    dprintf(MSG_DEBUG, "%s: read %u alid entries from file\n", __func__, alid_count);

    fclose(file);
    return alid_db;
}

static void apac_map_alid_db_write_db(apac_alid_db *alid_db) {
    FILE *file = NULL;
    u32 write_size = 0;
    u32 ret = 0;

    file = fopen(APAC_MAP_ALID_DB_FILE, "wb");
    if (!file) {
        dprintf(MSG_ERROR, "%s: failed to open file: %s\n", __func__, APAC_MAP_ALID_DB_FILE);
        return;
    }

    write_size = sizeof(u8) + (alid_db->alid_count * sizeof(apac_alid));
    ret = fwrite(alid_db, 1, write_size, file);
    if (ret != write_size) {
        dprintf(MSG_ERROR, "%s: failed write alid db to file, expected: %u bytes return: %u bytes\n", __func__, write_size, ret);
    }
    else {
        dprintf(MSG_DEBUG, "%s: written %u bytes to file\n", __func__, ret);
    }

    fclose(file);
}

static s8 apac_map_alid_db_find_alid(apac_alid_db *alid_db, apac_alid *alid) {
    for (u8 count = 0; count < alid_db->alid_count; count++) {
        if (!memcmp(alid, &alid_db->alid[count], sizeof(apac_alid))){
            return count;
        }
    }

    return -1;
}

apacBool_e apac_map_alid_db_store_alid(apac_alid *alid) {
    if (!alid) {
        dprintf(MSG_ERROR, "%s: invalid input", __func__);
        return APAC_FALSE;
    }
    dprintf(MSG_INFO, "%s: storing alid: %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n", __func__,
           alid->addr[0], alid->addr[1], alid->addr[2], alid->addr[3], alid->addr[4], alid->addr[5]);

    // read alid db
    apac_alid_db *alid_db = apac_map_alid_db_read_db();
    if (alid_db) {
        // check if alid is already present in alid db
        s8 db_pos = apac_map_alid_db_find_alid(alid_db, alid);
        if (db_pos >= 0) {
            dprintf(MSG_DEBUG, "%s: alid is already present in db at position %u\n", __func__, db_pos);
            free(alid_db);
            return APAC_TRUE;
        }

        // realloc to fit the new entry
        u32 realloc_size = sizeof(u8) + (alid_db->alid_count + 1) * sizeof(apac_alid);
        apac_alid_db *alid_db_tmp;
        alid_db_tmp = realloc(alid_db, realloc_size);
        if (!alid_db_tmp) {
            dprintf(MSG_ERROR, "%s: failed to reallocate memory for new entry\n", __func__);
            free(alid_db);
            return APAC_FALSE;
        }
        alid_db = alid_db_tmp;
        memcpy(alid_db->alid[alid_db->alid_count].addr, alid, sizeof(apac_alid));
        alid_db->alid_count++;

        // write the updated db to file
        apac_map_alid_db_write_db(alid_db);
        free(alid_db);
    }
    else {
        dprintf(MSG_DEBUG, "%s: reading alid db file failed, create a new one\n", __func__);

        // allocate memory for the first entry
        u32 alloc_size = sizeof(u8) + sizeof(apac_alid);
        alid_db = (apac_alid_db*) calloc(1, alloc_size);
        if (!alid_db) {
            dprintf(MSG_ERROR, "%s: failed to allocate memory for new entry\n", __func__);
            return APAC_FALSE;
        }
        memcpy(alid_db->alid[0].addr, alid, sizeof(apac_alid));
        alid_db->alid_count++;

        // write the updated db to file
        apac_map_alid_db_write_db(alid_db);
        free(alid_db);
    }

    return APAC_TRUE;
}

apacBool_e apac_map_alid_db_get_all_alid(apac_alid *alid, u8 *count) {
    u8 index = 0;

    if (!alid || *count > APAC_MAP_ALID_DB_MAX_ALID_COUNT) {
        dprintf(MSG_ERROR, "%s: invalid inputs", __func__);
        return APAC_FALSE;
    }

    // read alid db
    apac_alid_db *alid_db = apac_map_alid_db_read_db();
    if (!alid_db) {
        dprintf(MSG_ERROR, "%s: reading alid db failed", __func__);
        return APAC_FALSE;
    }

    for (index = 0; index < alid_db->alid_count && index < *count; index++) {
        memcpy(&alid[index], &alid_db->alid[index], sizeof(apac_alid));
    }

    *count = alid_db->alid_count;
    free(alid_db);
    dprintf(MSG_INFO, "%s: copied %u alid(s) from db\n", __func__, index);

    return APAC_TRUE;
}
