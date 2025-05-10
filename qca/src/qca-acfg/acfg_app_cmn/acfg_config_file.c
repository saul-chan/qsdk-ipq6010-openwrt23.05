/*
 * Copyright (c) 2015,2018,2020-2023 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * 2015 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
*/

#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include <string.h>
#include <libgen.h>
#include <acfg_types.h>
#include "acfg_tool.h"
#include <stdint.h>
#include "acfg_config_file.h"
#include "acfg_api.h"
#include <acfg_api_pvt.h>

struct acfg_radio_params radio_params[] =
{
    {"hostapd_ctrl_interface",  OFFSET(acfg_wlan_profile_t, ctrl_hapd),
        ACFG_TYPE_STR, ACFG_PARAM_GLOBAL},
    {"supplicant_ctrl_interface",  OFFSET(acfg_wlan_profile_t, ctrl_wpasupp),
        ACFG_TYPE_STR, ACFG_PARAM_GLOBAL},
    {"radio_name",  OFFSET_RADIOPARAM(acfg_wlan_profile_t, radio_name),
        ACFG_TYPE_STR, ACFG_PARAM_RADIO},
    {"channel", OFFSET_RADIOPARAM(acfg_wlan_profile_t, chan), ACFG_TYPE_CHAR,
        ACFG_PARAM_RADIO},
    {"band", OFFSET_RADIOPARAM(acfg_wlan_profile_t, chan_band), ACFG_TYPE_CHAR,
        ACFG_PARAM_RADIO},
    {"freq", OFFSET_RADIOPARAM(acfg_wlan_profile_t, freq), ACFG_TYPE_INT,
        ACFG_PARAM_RADIO},
    {"r_ampdu", OFFSET_RADIOPARAM(acfg_wlan_profile_t, ampdu),
        ACFG_TYPE_STR, ACFG_PARAM_RADIO},
    {"ampdu_size", OFFSET_RADIOPARAM(acfg_wlan_profile_t, ampdu_limit_bytes),
        ACFG_TYPE_INT, ACFG_PARAM_RADIO},
    {"ampdu_sub_frames", OFFSET_RADIOPARAM(acfg_wlan_profile_t,
        ampdu_subframes), ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"txpower", OFFSET(acfg_wlan_profile_vap_params_t, txpow),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"fstype", OFFSET(acfg_wlan_profile_vap_params_t, fstype),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"countrycode", OFFSET_RADIOPARAM(acfg_wlan_profile_t, country_code),
        ACFG_TYPE_INT16, ACFG_PARAM_RADIO},
    {"tpscale", OFFSET_RADIOPARAM(acfg_wlan_profile_t, tpscale),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"radiomac", OFFSET_RADIOPARAM(acfg_wlan_profile_t, radio_mac),
        ACFG_TYPE_STR, ACFG_PARAM_RADIO},
    {"macreq_enabled", OFFSET_RADIOPARAM(acfg_wlan_profile_t, macreq_enabled),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"aggr_burst", OFFSET_RADIOPARAM(acfg_wlan_profile_t, aggr_burst),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"aggr_burst_dur", OFFSET_RADIOPARAM(acfg_wlan_profile_t, aggr_burst_dur),
        ACFG_TYPE_INT, ACFG_PARAM_RADIO},
    {"stadfs", OFFSET_RADIOPARAM(acfg_wlan_profile_t, sta_dfs_enable),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"ccathena", OFFSET_RADIOPARAM(acfg_wlan_profile_t, ccathena),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"ccadetlevel", OFFSET_RADIOPARAM(acfg_wlan_profile_t, cca_det_level),
        ACFG_TYPE_INT16, ACFG_PARAM_RADIO},
    {"ccadetmargin", OFFSET_RADIOPARAM(acfg_wlan_profile_t, cca_det_margin),
        ACFG_TYPE_INT16, ACFG_PARAM_RADIO},
    {"beacon_burst_mode", OFFSET_RADIOPARAM(acfg_wlan_profile_t, beacon_burst_mode),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"atfstrictsched", OFFSET_RADIOPARAM(acfg_wlan_profile_t, atf_strict_sched),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"preCAC", OFFSET_RADIOPARAM(acfg_wlan_profile_t, preCAC),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"interCACChan", OFFSET_RADIOPARAM(acfg_wlan_profile_t, interCACChan),
        ACFG_TYPE_INT, ACFG_PARAM_RADIO},
    {"preCACTimeout", OFFSET_RADIOPARAM(acfg_wlan_profile_t, preCACtimeout),
        ACFG_TYPE_INT, ACFG_PARAM_RADIO},
    {"dbdc_enable", OFFSET_RADIOPARAM(acfg_wlan_profile_t, dbdc_enable),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"bw_reduceEn", OFFSET_RADIOPARAM(acfg_wlan_profile_t, bw_reduceEn),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"mark_subchan", OFFSET_RADIOPARAM(acfg_wlan_profile_t, mark_subchan),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"vap_name", OFFSET(acfg_wlan_profile_vap_params_t, vap_name),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"opmode", OFFSET(acfg_wlan_profile_vap_params_t, opmode),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"vapid", OFFSET(acfg_wlan_profile_vap_params_t, vapid),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"phymode", OFFSET(acfg_wlan_profile_vap_params_t, phymode),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"vap_ampdu", OFFSET(acfg_wlan_profile_vap_params_t, ampdu),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"vap_amsdu", OFFSET(acfg_wlan_profile_vap_params_t, amsdu),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"max_clients", OFFSET(acfg_wlan_profile_vap_params_t, max_clients),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"atf_options", OFFSET(acfg_wlan_profile_vap_params_t, atf_options),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"ssid", OFFSET(acfg_wlan_profile_vap_params_t, ssid),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"bitrate", OFFSET(acfg_wlan_profile_vap_params_t, bitrate),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"rate", OFFSET(acfg_wlan_profile_vap_params_t, rate),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"retries", OFFSET(acfg_wlan_profile_vap_params_t, retries),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"beaconint", OFFSET(acfg_wlan_profile_vap_params_t, beacon_interval),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"dtim_period", OFFSET(acfg_wlan_profile_vap_params_t, dtim_period),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"rtsthresh", OFFSET(acfg_wlan_profile_vap_params_t, rts_thresh),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"fragthresh", OFFSET(acfg_wlan_profile_vap_params_t, frag_thresh),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"vapmac", OFFSET(acfg_wlan_profile_vap_params_t, vap_mac),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"pureg", OFFSET(acfg_wlan_profile_vap_params_t, pureg),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"hidessid", OFFSET(acfg_wlan_profile_vap_params_t, hide_ssid),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"acs_6g_only_psc", OFFSET(acfg_wlan_profile_vap_params_t, acs_6g_only_psc),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"acs_freq_list", OFFSET(acfg_wlan_profile_vap_params_t, acs_freq_list),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"doth", OFFSET(acfg_wlan_profile_vap_params_t, doth),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"puren", OFFSET(acfg_wlan_profile_vap_params_t, puren),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"ht2040coext", OFFSET(acfg_wlan_profile_vap_params_t, coext),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"client_isolation", OFFSET(acfg_wlan_profile_vap_params_t,
        client_isolation), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"uapsd", OFFSET(acfg_wlan_profile_vap_params_t, uapsd),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"shortgi", OFFSET(acfg_wlan_profile_vap_params_t, shortgi),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"vlanid", OFFSET(acfg_wlan_profile_vap_params_t, vlanid),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"sha256", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, sha256),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"ieee80211w", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, ieee80211w),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"group_mgmt_cipher", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, group_mgmt_cipher),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"assoc_sa_query_max_timeout", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, assoc_sa_query_max_timeout),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"assoc_sa_query_retry_timeout", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, assoc_sa_query_retry_timeout),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"authtype", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, sec_method),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"cipher", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, cipher_method),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"group_cipher", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t,
        g_cipher_method), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"psk", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, psk),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wep_key0", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, wep_key0),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wep_key1", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, wep_key1),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wep_key2", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, wep_key2),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wep_key3", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, wep_key3),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wep_key_default", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t,
        wep_key_defidx), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"wps_pin", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, wps_pin),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"wps_flag", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, wps_flag),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"wps_manufacturer",  OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, wps_manufacturer),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wps_model_name",  OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, wps_model_name),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wps_model_number",  OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, wps_model_number),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wps_serial_number",  OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, wps_serial_number),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wps_device_type",  OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, wps_device_type),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wps_config_methods",  OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, wps_config_methods),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wps_upnp_iface", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t,
        wps_upnp_iface), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wps_friendly_name", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t,
        wps_friendly_name), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wps_man_url", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t,
        wps_man_url), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wps_model_desc", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t,
        wps_model_desc), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wps_upc", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t,
        wps_upc), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wps_pbc_in_m1", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t,
        wps_pbc_in_m1), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"wps_device_name", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t,
        wps_device_name), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wps_rf_bands", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t,
        wps_rf_bands), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"eap_type", OFFSET_EAPPARAM(acfg_wlan_profile_vap_params_t, eap_type),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"eap_identity", OFFSET_EAPPARAM(acfg_wlan_profile_vap_params_t,
        identity), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"eap_password", OFFSET_EAPPARAM(acfg_wlan_profile_vap_params_t,
        password), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"ca_cert_path", OFFSET_EAPPARAM(acfg_wlan_profile_vap_params_t, ca_cert),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"client_cert_path", OFFSET_EAPPARAM(acfg_wlan_profile_vap_params_t,
        client_cert), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"private_key_path", OFFSET_EAPPARAM(acfg_wlan_profile_vap_params_t,
        private_key), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"private_key_password", OFFSET_EAPPARAM(acfg_wlan_profile_vap_params_t,
        private_key_passwd), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"radius_retry_primary_interval", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t,
        radius_retry_primary_interval), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"pri_radius_ip", OFFSET_PRI_RADIUSPARAM(acfg_wlan_profile_vap_params_t,
        radius_ip), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"pri_radius_port", OFFSET_PRI_RADIUSPARAM(acfg_wlan_profile_vap_params_t,
        radius_port), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"pri_radius_sharedsecret", OFFSET_PRI_RADIUSPARAM(acfg_wlan_profile_vap_params_t,
        shared_secret), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"sec_1_radius_ip", OFFSET_SEC1_RADIUSPARAM(acfg_wlan_profile_vap_params_t,
        radius_ip), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"sec_1_radius_port", OFFSET_SEC1_RADIUSPARAM(acfg_wlan_profile_vap_params_t,
        radius_port), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"sec_1_radius_sharedsecret", OFFSET_SEC1_RADIUSPARAM(acfg_wlan_profile_vap_params_t,
        shared_secret), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"sec_2_radius_ip", OFFSET_SEC2_RADIUSPARAM(acfg_wlan_profile_vap_params_t,
        radius_ip), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"sec_2_radius_port", OFFSET_SEC2_RADIUSPARAM(acfg_wlan_profile_vap_params_t,
        radius_port), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"sec_2_radius_sharedsecret", OFFSET_SEC2_RADIUSPARAM(acfg_wlan_profile_vap_params_t,
        shared_secret), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"pri_acct_ip", OFFSET_PRI_ACCTPARAM(acfg_wlan_profile_vap_params_t,
        acct_ip), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"pri_acct_port", OFFSET_PRI_ACCTPARAM(acfg_wlan_profile_vap_params_t,
        acct_port), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"pri_acct_sharedsecret", OFFSET_PRI_ACCTPARAM(acfg_wlan_profile_vap_params_t,
        shared_secret), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"sec_1_acct_ip", OFFSET_SEC1_ACCTPARAM(acfg_wlan_profile_vap_params_t,
        acct_ip), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"sec_1_acct_port", OFFSET_SEC1_ACCTPARAM(acfg_wlan_profile_vap_params_t,
        acct_port), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"sec_1_acct_sharedsecret", OFFSET_SEC1_ACCTPARAM(acfg_wlan_profile_vap_params_t,
        shared_secret), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"sec_2_acct_ip", OFFSET_SEC2_ACCTPARAM(acfg_wlan_profile_vap_params_t,
        acct_ip), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"sec_2_acct_port", OFFSET_SEC2_ACCTPARAM(acfg_wlan_profile_vap_params_t,
        acct_port), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"sec_2_acct_sharedsecret", OFFSET_SEC2_ACCTPARAM(acfg_wlan_profile_vap_params_t,
        shared_secret), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"hs_enabled", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        hs_enabled), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"iw_enabled", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        iw_enabled), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"iw_network_type", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        network_type), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"iw_internet", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        internet), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"iw_asra", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        asra), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"iw_esr", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        esr), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"iw_uesa", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        uesa), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"iw_venue_group", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        venue_group), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"iw_venue_type", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        venue_type), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"iw_hessid", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        hessid), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"iw_roaming_consortium", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        roaming_consortium), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"iw_roaming_consortium2", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        roaming_consortium2), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"iw_venue_name", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        venue_name), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"manage_p2p", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        manage_p2p), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"disable_dgaf", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        disable_dgaf), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"network_auth_type", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        network_auth_type), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"ipaddr_type_availability", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        ipaddr_type_availability), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"nai_realm", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        nai_realm_list), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"anqp_3gpp_cell_net", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        anqp_3gpp_cellular_network), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"domain_name", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        domain_name_list), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"gas_frag_limit", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        gas_frag_limit), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"gas_comeback_delay", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        gas_comeback_delay), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"qos_map_set", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        qos_map_set), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"proxy_arp", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        proxy_arp), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"osen", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        osen), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"anqp_domain_id", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        anqp_domain_id), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"hs20_deauth_req_timeout", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        hs20_deauth_req_timeout), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"hs20_oper_friendly_name", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        hs20_operator_friendly_name), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"hs20_wan_metrics", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        hs20_wan_metrics), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"hs20_conn_capab", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        hs20_conn_capab), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"hs20_operating_class", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        hs20_operating_class), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"hs20_icon", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        hs20_icon), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"osu_ssid", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        osu_ssid), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"osu_server_uri", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        osu_server_uri), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"osu_friendly_name", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        osu_friendly_name), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"osu_nai", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        osu_nai), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"osu_method_list", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        osu_method_list), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"osu_icon", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        osu_icon), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"osu_service_desc", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        osu_service_desc), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"subscr_remediation_url", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        subscr_remediation_url), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"subscr_remediation_method", OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
        subscr_remediation_method), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"acl_node_sec", OFFSET_ACLPARAM(acfg_wlan_profile_vap_params_t,
        acfg_acl_node_list_sec), ACFG_TYPE_ACL, ACFG_PARAM_VAP},
    {"acl_policy_sec", OFFSET_ACLPARAM(acfg_wlan_profile_vap_params_t,
        node_acl_sec), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"acl_node", OFFSET_ACLPARAM(acfg_wlan_profile_vap_params_t,
        acfg_acl_node_list), ACFG_TYPE_ACL, ACFG_PARAM_VAP},
    {"acl_policy", OFFSET_ACLPARAM(acfg_wlan_profile_vap_params_t,
        node_acl), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"wds_enabled", OFFSET_WDSPARAM(acfg_wlan_profile_vap_params_t,
        enabled), ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"wds_addr", OFFSET_WDSPARAM(acfg_wlan_profile_vap_params_t,
        wds_addr), ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"wds_flag", OFFSET_WDSPARAM(acfg_wlan_profile_vap_params_t,
        wds_flags), ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"bridge", OFFSET(acfg_wlan_profile_vap_params_t, bridge),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"atf_percent", OFFSET_ATFPARAM(acfg_wlan_profile_vap_params_t, atf_percent),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"atf_stalist", OFFSET_ATFPARAM(acfg_wlan_profile_vap_params_t, atf_stalist),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"vendor_param_print", OFFSET_VENDORPARAM(acfg_wlan_profile_vap_params_t),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"vendor_param_int", OFFSET_VENDORPARAM(acfg_wlan_profile_vap_params_t),
        ACFG_TYPE_INT, ACFG_PARAM_VAP},
    {"vendor_param_mac", OFFSET_VENDORPARAM(acfg_wlan_profile_vap_params_t),
        ACFG_TYPE_MACADDR, ACFG_PARAM_VAP},
    {"primary_vap", OFFSET(acfg_wlan_profile_vap_params_t, primary_vap),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"bcn_rate", OFFSET(acfg_wlan_profile_vap_params_t, bcn_rate),
        ACFG_TYPE_INT16, ACFG_PARAM_VAP},
    {"implicitbf", OFFSET(acfg_wlan_profile_vap_params_t, implicitbf),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"wnm", OFFSET(acfg_wlan_profile_vap_params_t, wnm),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"rrm", OFFSET(acfg_wlan_profile_vap_params_t, rrm),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"he_rx_mcsmap", OFFSET(acfg_wlan_profile_vap_params_t, he_rx_mcsmap),
        ACFG_TYPE_UINT32, ACFG_PARAM_VAP},
    {"he_tx_mcsmap", OFFSET(acfg_wlan_profile_vap_params_t, he_tx_mcsmap),
        ACFG_TYPE_UINT32, ACFG_PARAM_VAP},
    {"owe_transition_ifname", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, owe_transition_ifname),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"owe_transition_ssid", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, owe_transition_ssid),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"owe_transition_bssid", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, owe_transition_bssid),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"owe_groups", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, owe_groups),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"sae_groups", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, sae_groups),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"sae_password", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, sae_password),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"sae_pwe", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, sae_pwe),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"acfg_standard",  OFFSET(acfg_wlan_profile_t,
        acfg_standard), ACFG_TYPE_CHAR, ACFG_PARAM_GLOBAL},
    {"ant_ps_on", OFFSET(acfg_wlan_profile_vap_params_t, greenap_ps_enable),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"ps_timeout", OFFSET(acfg_wlan_profile_vap_params_t, greenap_ps_trans_time),
        ACFG_TYPE_UINT32, ACFG_PARAM_VAP},
    {"rpt_max_phy", OFFSET_RADIOPARAM(acfg_wlan_profile_t, rpt_max_phy),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"scan_over_cac_en", OFFSET_RADIOPARAM(acfg_wlan_profile_t, scan_over_cac),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"acs_pcaconly", OFFSET_RADIOPARAM(acfg_wlan_profile_t, acs_pcaconly),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"vap_doth", OFFSET(acfg_wlan_profile_vap_params_t, vap_doth),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"en_6g_sec_comp", OFFSET(acfg_wlan_profile_vap_params_t, sec_comp_6g),
        ACFG_TYPE_UINT32, ACFG_PARAM_VAP},
    {"keymgmt_mask_6g", OFFSET(acfg_wlan_profile_vap_params_t, keymgmt_mask_6g),
        ACFG_TYPE_UINT32, ACFG_PARAM_VAP},
    {"dpp_connector", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, dpp_connector),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"dpp_csign", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, dpp_csign),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"dpp_netaccesskey", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, dpp_netaccesskey),
        ACFG_TYPE_STR, ACFG_PARAM_VAP},
    {"update_config", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, update_config),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"dpp_config_processing", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, dpp_config_processing),
        ACFG_TYPE_CHAR, ACFG_PARAM_VAP},
    {"dpp_configurator_connectivity", OFFSET_SECPARAM(acfg_wlan_profile_vap_params_t, dpp_configurator_connectivity),
        ACFG_TYPE_UINT32, ACFG_PARAM_VAP},
    {"dcs_enable", OFFSET_RADIOPARAM(acfg_wlan_profile_t, dcs_enable),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"CSwOpts", OFFSET_RADIOPARAM(acfg_wlan_profile_t, cswopts),
        ACFG_TYPE_UINT32, ACFG_PARAM_RADIO},
    {"mld_id", OFFSET(acfg_wlan_profile_vap_params_t, mld_id),
        ACFG_TYPE_UINT32, ACFG_PARAM_VAP},
    {"mld_macs",  OFFSET(acfg_wlan_profile_t, mld_macs),
        ACFG_TYPE_STR, ACFG_PARAM_GLOBAL},
    {"puncture_bitmap",  OFFSET_RADIOPARAM(acfg_wlan_profile_t, puncture_bitmap),
        ACFG_TYPE_INT16, ACFG_PARAM_RADIO},
    {"chan_num_seg1",  OFFSET_RADIOPARAM(acfg_wlan_profile_t, chan_num_seg1),
        ACFG_TYPE_UINT32, ACFG_PARAM_RADIO},
    {"chan_num_seg2",  OFFSET_RADIOPARAM(acfg_wlan_profile_t, chan_num_seg2),
        ACFG_TYPE_INT16, ACFG_PARAM_RADIO},
    {"dfs_puncture", OFFSET_RADIOPARAM(acfg_wlan_profile_t, dfs_puncture),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
    {"post_acs_evt", OFFSET_RADIOPARAM(acfg_wlan_profile_t, en_post_acs_evt),
        ACFG_TYPE_CHAR, ACFG_PARAM_RADIO},
};

/* vendors can fill this table */
const acfg_vendor_param_cmd_map_t acfg_vendor_param_cmd_mapping[] = {
    /*=====================================================
      vendor param name           command id
      /=====================================================*/
    {"vendor_param_print", ACFG_VENDOR_PARAM_CMD_PRINT},
    {"vendor_param_int",   ACFG_VENDOR_PARAM_CMD_INT},
    {"vendor_param_mac",   ACFG_VENDOR_PARAM_CMD_MAC},
};

extern void
acfg_mac_to_str(uint8_t *addr, char *str, uint16_t str_max_len);

int
acfg_write_text_file(FILE *fp, void *bin_bptr, void *def_bptr,
                struct acfg_radio_params *param)
{
    uint8_t *pos1, *pos2 = NULL;
    int ret = 0, status = 0, i;
    uint8_t *pos  = NULL;
    char mac_string[18];
    uint8_t num_nai_realm_data = 0, num_node_acl = 0;
    acfg_wlan_profile_node_params_t *node_params = NULL;
    acfg_wlan_profile_vap_params_t *vap_params = NULL;

    if (param->param_type == ACFG_PARAM_VAP) {
        vap_params = (acfg_wlan_profile_vap_params_t *)bin_bptr;
        node_params = &vap_params->node_params;
    }

    pos1 = (uint8_t *)bin_bptr + param->offset;
    pos2 = (uint8_t *)def_bptr + param->offset;

    if(param->offset == OFFSET_VENDORPARAM(acfg_wlan_profile_vap_params_t)) {
        pos1 = (uint8_t *)bin_bptr;
        pos2 = (uint8_t *)def_bptr;
    }

    pos = pos1;

    switch (param->type) {
        case ACFG_TYPE_INT:
            ret = memcmp(pos1, pos2, sizeof(int));

            if (ret != 0) {
                status = fprintf(fp, "%s=%d\n", param->name, *(int *)pos);

                if (status < 0)
                    acfg_log_errstr("%s param write failed\n", param->name);
            }
            break;
        case ACFG_TYPE_CHAR:
            ret = memcmp(pos1, pos2, sizeof(char));

            if (ret != 0) {
                status = fprintf(fp, "%s=%d\n", param->name, *(uint8_t *)pos);

                if (status < 0)
                    acfg_log_errstr("%s param write failed\n", param->name);
            }
            break;
        case ACFG_TYPE_INT16:
            ret = memcmp(pos1, pos2, sizeof(uint16_t));

            if (ret != 0) {
                status = fprintf(fp, "%s=%d\n", param->name, *(uint16_t *)pos);

                if (status < 0)
                    acfg_log_errstr("%s param write failed\n", param->name);
            }
            break;
        case ACFG_TYPE_MACADDR:
            ret = memcmp(pos1, pos2, ACFG_MACADDR_LEN);

            if (ret != 0) {
                memset(mac_string, 0, sizeof(mac_string));
                acfg_mac_to_str(pos, mac_string, sizeof(mac_string));

                status = fprintf(fp, "%s=%s\n", param->name, mac_string);

                if (status < 0)
                    acfg_log_errstr("%s param write failed\n", param->name);
            }
            break;
        case ACFG_TYPE_STR:
        {
            if ((param->offset == OFFSET_HSPARAM(acfg_wlan_profile_vap_params_t,
                        nai_realm_list)) && (vap_params)) {
                num_nai_realm_data = vap_params->num_nai_realm_data;

                for (i = 0; i < num_nai_realm_data; i++) {
                    ret = memcmp(pos1, pos2, (strlen((char *)pos1) + 1));

                    if (ret != 0) {
                        status = fprintf(fp, "%s=%s\n", param->name, pos1);

                        if (status < 0)
                            acfg_log_errstr("%s param write failed\n", param->name);
                    }
                    pos1 += HS_MAX_NAI_REALM_DATA_LEN;
                    pos2 += HS_MAX_NAI_REALM_DATA_LEN;
                }
            } else {
                ret = memcmp(pos1, pos2, (strlen((char *)pos) + 1));

                if (ret != 0) {
                    status = fprintf(fp, "%s=%s\n", param->name, pos);

                    if (status < 0)
                        acfg_log_errstr("%s param write failed\n", param->name);
                }
            }
        }
        break;
        case ACFG_TYPE_ACL:
            {
                if ((param->offset == OFFSET_ACLPARAM(acfg_wlan_profile_vap_params_t,
                            acfg_acl_node_list)) && (node_params)) {
                    num_node_acl = node_params->num_node;
                } else if ((param->offset == OFFSET_ACLPARAM(acfg_wlan_profile_vap_params_t,
                            acfg_acl_node_list_sec)) && (node_params)) {
                    num_node_acl = node_params->num_node_sec;
                }

                for (i = 0; i < num_node_acl; i++) {
                    ret = memcmp(pos1, pos2, ACFG_MACADDR_LEN);

                    if (ret != 0) {
                        memset(mac_string, 0, sizeof(mac_string));
                        acfg_mac_to_str(pos1, mac_string, sizeof(mac_string));

                        if (!i)
                            status = fprintf(fp, "%s=%s", param->name, mac_string);
                        else if (i == (num_node_acl - 1))
                            status = fprintf(fp, "\t%s\n", mac_string);
                        else
                            status = fprintf(fp, "\t%s", mac_string);

                        if (status < 0)
                            acfg_log_errstr("%s param write failed\n", param->name);
                    }
                    pos1 += ACFG_MACADDR_LEN;
                    pos2 += ACFG_MACADDR_LEN;
                }
            }
            break;
        case ACFG_TYPE_UINT32:
            ret = memcmp(pos1, pos2, sizeof(uint32_t));

            if (ret != 0) {
                status = fprintf(fp, "%s=%u\n", param->name, *(uint32_t *)pos);

                if (status < 0)
                    acfg_log_errstr("%s param write failed\n", param->name);
            }
            break;
    }

    return status;
}

void acfg_set_profile_param(void *buf,
        uint8_t *val1,
        uint8_t type)
{
    char *val = (char *)val1;
    char *ptr;
    uint32_t result;

    switch (type) {
        case ACFG_TYPE_INT:
            *(int *)buf = atol(val);
            break;
        case ACFG_TYPE_CHAR:
            *(char *)buf = atoi(val);
            break;
        case ACFG_TYPE_INT16:
            *(uint16_t *)buf = atoi(val);
            break;
        case ACFG_TYPE_MACADDR:
            acfg_mac_str_to_octet((uint8_t *)val, buf);
            break;
        case ACFG_TYPE_STR:
            strlcpy((char *)buf, val, strlen(val) + 1);
            break;
        case ACFG_TYPE_ACL:
            {
                uint8_t *mac_str, *pos;
                uint8_t i = 0;


                mac_str = pos = (uint8_t *) val;
                while ((pos) && i < ACFG_MAX_ACL_NODE) {
                    if ((*pos == ' ') || (*pos == '\t')) {
                        *pos = '\0';
                        pos++;
                        acfg_mac_str_to_octet(mac_str, buf);
                        buf += ACFG_MACADDR_LEN;
                        mac_str = pos;
                        i++;
                    }
                    if (*pos == '\n' || *pos == '\0') {
                        acfg_mac_str_to_octet(mac_str, buf);
                        buf += ACFG_MACADDR_LEN;
                        i++;
                        break;
                    }
                    pos++;
                }
            }
        case ACFG_TYPE_UINT32:
            result = (uint32_t)strtoul(val,&ptr,0);
            *(uint32_t *)buf = result;
            break;
    }
}

int
acfg_compare_profile(char *radioname, char *bin_file, acfg_wlan_profile_t *bin_profile)
{
    uint16_t i = 0, j = 0, k = 0;
    uint8_t num_vaps;
    FILE *fp;
    char* path = strdup(bin_file);
    char* file_dir = dirname(path);
    char text_file[64];
    acfg_wlan_profile_t *def_profile;
    acfg_wlan_profile_vendor_param_t *vendor_param;
    uint8_t num_vendor_params;

    acfg_os_snprintf(text_file, sizeof(text_file), "%s/acfg_%s.conf", file_dir,
            radioname);

    fp =  fopen (text_file, "w+");
    if(fp == NULL) {
        acfg_os_snprintf(text_file, sizeof(text_file), "/tmp/acfg_%s.conf", radioname);

        fp =  fopen (text_file, "w+");
        if(fp == NULL) {
            acfg_log_errstr("Unable to open %s. Pls check access. \n\r", text_file);
            return -1;
        }
    }

    printf("output_text_file=%s\n",text_file);

    def_profile = (acfg_wlan_profile_t *)bin_profile->priv;
    num_vaps = bin_profile->num_vaps;

    /* Radio and global parameters */
    fprintf(fp, "#Radio parameters starts here\n");
    for (i = 0; i < (sizeof(radio_params) / sizeof(struct acfg_radio_params)); i++) {

        if (radio_params[i].param_type == ACFG_PARAM_RADIO) {

            acfg_write_text_file(fp, (void *)bin_profile,
                      (void *)def_profile, &radio_params[i]);

        }
    }

    for (i = 0; i < (sizeof(radio_params) / sizeof(struct acfg_radio_params)); i++) {

        if (radio_params[i].param_type == ACFG_PARAM_GLOBAL) {

            acfg_write_text_file(fp, (void *)bin_profile,
                      (void *)def_profile, &radio_params[i]);

        }
    }
    fprintf(fp, "\n");

    /* Vap parameters */
    for (j = 0; j < num_vaps; j++) {

        fprintf(fp, "#VAP parameters starts here\n");

        for (k = 0; k < (sizeof(radio_params) / sizeof(struct acfg_radio_params)); k++) {

            if ((radio_params[k].param_type == ACFG_PARAM_VAP) &&
                (radio_params[k].offset != OFFSET_VENDORPARAM(acfg_wlan_profile_vap_params_t))) {

                acfg_write_text_file(fp, (void *)(&bin_profile->vap_params[j]),
                          (void *)(&def_profile->vap_params[j]), &radio_params[k]);

            }
        }

        /* vap vendor params */
        num_vendor_params = bin_profile->vap_params[j].num_vendor_params;

        for (i = 0; i < num_vendor_params; i++) {

            vendor_param = &bin_profile->vap_params[j].vendor_param[i];

            for (k = 0; k < (sizeof(radio_params) / sizeof(struct acfg_radio_params)); k++) {

                if ((radio_params[k].param_type == ACFG_PARAM_VAP) &&
                    (radio_params[k].offset == OFFSET_VENDORPARAM(acfg_wlan_profile_vap_params_t)) &&
                    (radio_params[k].type == vendor_param->type)) {

                    acfg_write_text_file(fp, (void *)(&vendor_param->data),
                              (void *)(&def_profile->vap_params[j].vendor_param[i].data),
                              &radio_params[k]);

                }
            }
        }

        fprintf(fp, "\n");
    }

    fclose(fp);

    return 0;

}

int acfg_read_file(char *filename, acfg_wlan_profile_t *profile)
{
    FILE *fp;
    char buf[512];
    uint8_t *pos  = 0;
    uint8_t i = 0, vap_index = 0, num_node, vendor_param_index = 0;
    uint8_t nai_realm_data_index = 0;
    uint32_t offset = 0;
    uint8_t zero_mac[ACFG_MACADDR_LEN] = {0,0,0,0,0,0};
    acfg_wlan_profile_vap_params_t *vap_params;
    acfg_wlan_profile_node_params_t *node_params;

    if (filename == NULL) {
        return -1;
    }
    fp = fopen(filename, "r");
    if (fp == NULL) {
        return -1;
    }
    while (fgets(buf, sizeof (buf), fp)) {
        if(buf[0] == '#') {
            continue;
        }
        pos = (uint8_t *)buf;
        while (*pos != '\0') {
            if (*pos == '\n') {
                *pos = '\0';
                break;
            }
            pos++;
        }
        pos = (uint8_t *) strchr(buf, '=');
        if (pos == NULL) {
            continue;
        }
        *pos = '\0';
        pos++;
        if (strcmp(buf, "vap_name") == 0) {
            vap_index++;
            /* set the vapid to auto, so that MAC addr allocated automatically */
            profile->vap_params[vap_index - 1].vapid = VAP_ID_AUTO;
            vendor_param_index = 0;
            nai_realm_data_index = 0;
        }
        for (i = 0; i < (sizeof (radio_params) /
                    sizeof (struct acfg_radio_params)); i++) {
            if (strcmp(buf, (char *)radio_params[i].name) == 0) {
                if (vap_index == 0) {
                    offset =  radio_params[i].offset;
                } else {
                    if(radio_params[i].offset == OFFSET_VENDORPARAM(acfg_wlan_profile_vap_params_t))
                    {
                        /* This is a vendor parameter "no name!", so handle differently */
                        if(vendor_param_index < ACFG_MAX_VENDOR_PARAMS)
                        {
                            int j;
                            acfg_wlan_profile_vendor_param_t *vendor_param =
                                &profile->vap_params[vap_index - 1].vendor_param[vendor_param_index];

                            if(radio_params[i].type == ACFG_TYPE_INT)
                            {
                                vendor_param->len = sizeof(uint32_t);
                                vendor_param->type = ACFG_TYPE_INT;
                            }
                            else if(radio_params[i].type == ACFG_TYPE_STR)
                            {
                                vendor_param->len = strlen((char *)pos) + 1;
                                vendor_param->type = ACFG_TYPE_STR;
                            }
                            else if(radio_params[i].type == ACFG_TYPE_MACADDR)
                            {
                                vendor_param->len = ACFG_MACADDR_LEN;
                                vendor_param->type = ACFG_TYPE_MACADDR;
                            }
                            for(j = 0; j < (sizeof(acfg_vendor_param_cmd_mapping) /
                                        sizeof(acfg_vendor_param_cmd_map_t)); j++)
                            {
                                if(strcmp((char *)acfg_vendor_param_cmd_mapping[j].name, buf) == 0)
                                    vendor_param->cmd = acfg_vendor_param_cmd_mapping[j].cmd;
                            }
                            offset = (uint32_t)((uint8_t *)&vendor_param->data - (uint8_t *)profile);
                            profile->vap_params[vap_index-1].num_vendor_params = ++vendor_param_index;
                        }
                    }
                    else if(strcmp(buf, "nai_realm") == 0)
                    {
                        offset = (uint32_t)((uint8_t *)&profile->vap_params[vap_index - 1].security_params.hs_iw_param.nai_realm_list[nai_realm_data_index] -
                                (uint8_t *)profile);
                        profile->vap_params[vap_index-1].num_nai_realm_data = ++nai_realm_data_index;

                    }
                    else
                    {
                        offset = OFFSET(acfg_wlan_profile_t, vap_params[0]);
                        offset += (vap_index - 1) *
                            sizeof (acfg_wlan_profile_vap_params_t);
                        offset += radio_params[i].offset;
                    }
                }
                acfg_set_profile_param(((uint8_t *)profile) +
                        offset,
                        pos, radio_params[i].type);
                break;
            }
        }
    }
    fclose(fp);
    profile->num_vaps = vap_index;
    for (i = 0; i < profile->num_vaps; i++) {
        vap_params = &profile->vap_params[i];
        node_params = &vap_params->node_params;

        /*  For first ACL list   */
        for (num_node = 0; num_node < ACFG_MAX_ACL_NODE; num_node++) {
            if (!memcmp(node_params->acfg_acl_node_list[num_node],
                        zero_mac, ACFG_MACADDR_LEN))
            {

                break;
            }
        }
        node_params->num_node = num_node;

        /*  For second ACL list   */
        for (num_node = 0; num_node < ACFG_MAX_ACL_NODE; num_node++) {
            if (!memcmp(node_params->acfg_acl_node_list_sec[num_node],
                        zero_mac, ACFG_MACADDR_LEN))
            {

                break;
            }
        }
        node_params->num_node_sec = num_node;
    }

    if (strlen((char *)profile->radio_params.radio_name) == 0) {
        return -1;
    }
    return 0;
}
