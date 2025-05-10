/*
 * Copyright (c) 2015,2020 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * 2015 Qualcomm Atheros, Inc.
 *
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
*/

#include <acfg_api.h>

#define OFFSET(a,b) ((long)&((a *) 0)->b)
#define OFFSET_RADIOPARAM(a,b) ((long)&((a *) 0)->radio_params.b)
#define OFFSET_SECPARAM(a,b) ((long)&((a *) 0)->security_params.b)
#define OFFSET_EAPPARAM(a,b) ((long)&((a *) 0)->security_params.eap_param.b)
#define OFFSET_PRI_RADIUSPARAM(a,b) ((long)&((a *) 0)->security_params.pri_radius_param.b)
#define OFFSET_SEC1_RADIUSPARAM(a,b) ((long)&((a *) 0)->security_params.sec1_radius_param.b)
#define OFFSET_SEC2_RADIUSPARAM(a,b) ((long)&((a *) 0)->security_params.sec2_radius_param.b)
#define OFFSET_PRI_ACCTPARAM(a,b) ((long)&((a *) 0)->security_params.pri_acct_server_param.b)
#define OFFSET_SEC1_ACCTPARAM(a,b) ((long)&((a *) 0)->security_params.sec1_acct_server_param.b)
#define OFFSET_SEC2_ACCTPARAM(a,b) ((long)&((a *) 0)->security_params.sec2_acct_server_param.b)
#define OFFSET_HSPARAM(a,b) ((long)&((a *) 0)->security_params.hs_iw_param.b)
#define OFFSET_ACLPARAM(a,b) ((long)&((a *) 0)->node_params.b)
#define OFFSET_WDSPARAM(a,b) ((long)&((a *) 0)->wds_params.b)
#define OFFSET_VENDORPARAM(a)  ((long)&((a *) 0)->vendor_param)
#define OFFSET_ATFPARAM(a,b)  ((long)&((a *) 0)->atf.b)

/*
 * @Param Type
 */
enum {
     ACFG_PARAM_GLOBAL = 1,
     ACFG_PARAM_RADIO,
     ACFG_PARAM_VAP,
};

/*
 * struct acfg_radio_params
 * @name:        element name
 * @offset:      element offset
 * @type:        element data type
 * @param_type:  element param type
 */
struct acfg_radio_params {
    uint8_t name[32];
    uint32_t offset;
    uint8_t type;
    uint8_t param_type;
};

/*
 * acfg_read_file - read acfg data from input file
 * @filename: input file
 * @profile: acfg profile data
 *
 * ret: 0 on success and negative on failure
 */
int acfg_read_file(char *filename, acfg_wlan_profile_t *profile);

/*
 * acfg_compare_profile - compare acfg profile and write file
 * @radioname: radio name
 * @bin_file: binary file
 * @bin_profile: acfg profile data
 *
 * ret: 0 on success and negative on failure
 */
int acfg_compare_profile(char *radioname, char *bin_file,
            acfg_wlan_profile_t *bin_profile);
