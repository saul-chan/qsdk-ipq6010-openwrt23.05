/*
 * @File: syscfg_wireless.c
 *
 * Copyright (c) 2023 Qualcomm Technologies, Inc.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include "sys/stat.h"
#include <sys/socket.h>
#include <linux/wireless.h>
#include <fcntl.h>
#include "compat.h"
#include "../storage.h"
#include "wlanif_cmn.h"
#include "package.h"
#include "syscfg/syscfg.h"
#include <glob.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <inttypes.h>
#include "stdbool.h"

#ifndef _BYTE_ORDER
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define _BYTE_ORDER _LITTLE_ENDIAN
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
#define _BYTE_ORDER _BIG_ENDIAN
#endif
#endif  // _BYTE_ORDER

#ifdef NOT_UMAC
#include "net80211/ieee80211.h"
#include "net80211/ieee80211_crypto.h"
#include "net80211/ieee80211_ioctl.h"
#else
#include "ieee80211_external.h"
#endif

#define IEEE80211_MAX_HTMODE_LEN 30 // maximum HT mode length used in driver
#define MAX_RADIO 4

#define dprintf(args...) \
    dprintf(0, args)

enum BANDTYPE {
    BAND_UNKNOWN,
    BAND_2G,
    BAND_5G,
    BAND_6G,
};

extern struct wlanif_config *wlanIfLS;

enum BEACONTYPE {
    BEACONTYPE_eNone = 1,
    BEACONTYPE_eBasic = 2,
    BEACONTYPE_eWPA = 3,
    BEACONTYPE_e11i = 4,
    BEACONTYPE_eWPAand11i = 7,
};

enum BASICENCRYPTIONMODES {
    BASICENCRYPTIONMODES_eNone = 1,
    BASICENCRYPTIONMODES_eWEPEncryption = 2,
};

enum BASICAUTHENTICATIONMODE {
    BASICAUTHENTICATIONMODE_eNone = 1,
    BASICAUTHENTICATIONMODE_eEAPAuthentication = 2,
    BASICAUTHENTICATIONMODE_eSharedAuthentication = 3,
    BASICAUTHENTICATIONMODE_eBoth = 4,
};

enum WPAENCRYPTIONMODES {
    WPAENCRYPTIONMODES_eTKIPEncryption = 1,
    WPAENCRYPTIONMODES_eAESEncryption = 2,
    WPAENCRYPTIONMODES_eTKIPandAESEncryption = 3,
};

enum WPAAUTHENTICATIONMODE {
    WPAAUTHENTICATIONMODE_ePSKAuthentication = 1,
    WPAAUTHENTICATIONMODE_eEAPAuthentication = 2,
};

enum IEEE11IENCRYPTIONMODES {
    IEEE11IENCRYPTIONMODES_eTKIPEncryption = 1,
    IEEE11IENCRYPTIONMODES_eAESEncryption = 2,
    IEEE11IENCRYPTIONMODES_eTKIPandAESEncryption = 3,
};

enum IEEE11IAUTHENTICATIONMODE {
    IEEE11IAUTHENTICATIONMODE_ePSKAuthentication = 1,
    IEEE11IAUTHENTICATIONMODE_eEAPAuthentication = 2,
    IEEE11IAUTHENTICATIONMODE_eEAPandPSKAuthentication = 3,
    IEEE11IAUTHENTICATIONMODE_eWPA3Authentication = 4,
    IEEE11IAUTHENTICATIONMODE_eEAPandWPA3Authentication = 5,
};

enum WPA3SAEPWEVALUE {
    WPA3SAEPWEVALUE_eHuntingPecking = 0,
    WPA3SAEPWEVALUE_eHashToElement = 1,
    WPA3SAEPWEVALUE_eBothHuntPeckAndHashElem = 2,
    WPA3SAEPWEVALUE_eNotSet = -1,
};

enum WPA3SAEVALUE {
    WPA3SAEVALUE_eDisable = 0,
    WPA3SAEVALUE_eEnable = 1,
    WPA3SAEVALUE_eNotSet = -1,
};

enum WPA3OWEVALUE {
    WPA3OWEVALUE_eDisable = 0,
    WPA3OWEVALUE_eEnable = 1,
    WPA3OWEVALUE_eNotSet = -1,
};

enum VAPTYPE {
    VAPTYPE_AP,
    VAPTYPE_STA,
    VAPTYPE_INVALID_DEVICE_MODE = ~0
};

enum LISTACTIONFLAG {
    LISTACTION_ADD = 0,
    LISTACTION_DEL = 1,
    LISTACTION_MATCH = 2,
};

struct list_data {
    int action;   // add or delete
    char *data;
    struct list_data *next;
};

struct vapsec_s {
    int vapidx;
    char ifname[IFNAMSIZ+1];
    bool changed;
    bool opti_changed;
    struct vapsec_s *next;
    int bcn_type;
    int wep_encr;
    int wep_auth;
    int wpa_encr;
    int wpa_auth;
    int wpa2_encr;
    int wpa2_auth;
    int wep_keyidx;
    char *passphrase;
    char *psk;

    /* WPA3 */
    int wpa3_auth;
    int wpa3_sae;
    int wpa3_owe;
    int vap_type;
    struct list_data *wpa3_pass_list;
    struct list_data *wpa3_sae_groups;
    struct list_data *wpa3_owe_groups;

    /*optimization for non-destructive operations on vaps*/
    char *channel;
    char *vapind;
    char *htmode;
    bool  isdown;

    int wpa3_en_6g_sec_comp;
    int wpa3_sae_pwe;
};

struct wireless_s{
    bool changed;
    struct vapsec_s *vsec;
};

struct wl_translate_option{
    char *optname;
    char *uciname;
    int (*opt2uci)(char *optval, char *ucival);   /*direct mapping*/
    int (*optparser)(struct wireless_s *wl, int index, char *name, char *value); /*indirect mapping*/
};

#define MAX_NUM_RADIOS 2
#define RDKB_NUM_VAPS_SUPPORTED_PER_RADIO 17
#define RDKB_MAX_SUPPORTED_VAPS RDKB_NUM_VAPS_SUPPORTED_PER_RADIO * MAX_NUM_RADIOS

int g_2g_rdk_vap_list[RDKB_MAX_SUPPORTED_VAPS] = {-1};
int g_5g_rdk_vap_list[RDKB_MAX_SUPPORTED_VAPS] = {-1};
int *ptr = NULL;

typedef struct {
    int radioIndex;
    char radioName[IFNAMSIZ + 1];
    int mibIndx[RDKB_NUM_VAPS_SUPPORTED_PER_RADIO];
    int numVAPs;
} radio_vap_config;

typedef struct  {
    radio_vap_config radio[MAX_NUM_RADIOS];
} wireless_dynamic_vap_config; 

wireless_dynamic_vap_config g_wireless_vap_config = {0};

static int wireless_vap_ifctl(const char *ifname, int up_down)
{
    struct ifreq ifr = {};
    int ret;
    int sock;

    if (snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname) < 0)
    {
        return -1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if(sock < 0) {
        return -1;
    }

    ret = ioctl(sock, SIOCGIFFLAGS, &ifr);
    if (ret < 0)
    {
        close(sock);
        return ret;
    }

    if(up_down == 1) {
        ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
    } else  {
        ifr.ifr_flags &= ~IFF_UP;
    }

    ret = ioctl(sock, SIOCSIFFLAGS, &ifr);
    close(sock);

    return ret;
}

/*The mapping between vapidx and ifname is not stored in UCI configuration.
  It is generated dynamically by qcawifi.sh script. And it is stored in the
  state file of UCI. So here we need to get it by uci command.
*/
int wireless_get_ifname_by_vapidx(int vapidx, char *ifname)
{
    char optv[UCI_MAX_STR_LEN+1] = {0};
    char ucival[UCI_MAX_STR_LEN+1] = {0};
    int len;

    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wireless.ath%d.ifname", vapidx);
    syscfg_get(NULL, optv, ucival, sizeof(ucival));

    len = strlen(ucival);
    if (len > 0) {
        snprintf(ifname, IFNAMSIZ, "%s", ucival);
        return 0;
    }
    else {
        return -1;
    }
}

int wireless_set_vapchannel(const char *ifname, int channel, int band)
{
    if (setFreq_cfg80211(wlanIfLS->ctx ,ifname , channel, band, 0 ,0) < 0) {
        printf(" %s: setFreq command failed \n",__func__);
        return -1;
    }

    return 0;
}

int wireless_set_vapind(const char *ifname, int vapind)
{
    int setvapind[2];

    setvapind[0] = IEEE80211_PARAM_VAP_ENHIND;
    setvapind[1] = vapind;

    if (setParamVapInd_cfg80211(wlanIfLS->ctx ,ifname , setvapind, 2*sizeof(int)) < 0) {
        printf(" %s: setvapind command failed \n",__func__);
        return -1;
    }

    return 0;
}

/**
 * @brief Configure the HT mode on given interface using "iwpriv mode" cmd
 *
 * @param [in] ifname  the given interface name
 * @param [in] htmode  the driver HT mode derived from "Standard" TLV
 */
static int wireless_set_vaphtmode(const char *ifname, const char *htmode)
{
    char cmd[30 + IFNAMSIZ + IEEE80211_MAX_HTMODE_LEN];
    if (snprintf(cmd, sizeof(cmd), "iwpriv %s mode %s", ifname, htmode) < sizeof(cmd)) {
        return system(cmd);
    }
    // Else not long enough, return error. It should not happen unless the driver
    // definition changes but the constants are not updated to reflect it.

    return -1;
}

static struct vapsec_s *wireless_get_vapsec(struct wireless_s *wl, int vapidx)
{
    struct vapsec_s *vsec = wl->vsec;
    while (vsec)
    {
        if (vsec->vapidx == vapidx)
            break;
        vsec = vsec->next;
    }

    if (!vsec)
    {
        vsec = malloc(sizeof(struct vapsec_s));
        if (!vsec)
        {
            return NULL;
        }

        memset(vsec, 0, sizeof(struct vapsec_s));
        vsec->wpa3_pass_list = NULL;
        vsec->wpa3_sae_groups = NULL;
        vsec->wpa3_owe_groups = NULL;
        vsec->wpa3_en_6g_sec_comp = -1;
        vsec->wpa3_sae_pwe = -1;
        vsec->vapidx = vapidx;
        vsec->vap_type = VAPTYPE_INVALID_DEVICE_MODE;
        vsec->next = wl->vsec;
        wl->vsec = vsec;
    }
    return vsec;
}

static void wireless_free_list_data(struct list_data *list)
{
     while (list) {
         if (list->data)
             free (list->data);
         list = list->next;
     }
}

static int wireless_free_vapsec(struct wireless_s *wl)
{
    struct vapsec_s *vsec = wl->vsec;
    struct vapsec_s *tmp;

    while (vsec)
    {
        tmp =vsec;
        vsec = vsec->next;

        if(tmp->psk)
            free(tmp->psk);
        if (tmp->passphrase)
            free(tmp->passphrase);
        if (tmp->channel)
            free(tmp->channel);
        if (tmp->vapind)
            free(tmp->vapind);
        if (tmp->htmode)
            free(tmp->htmode);
        if (tmp->wpa3_pass_list)
            wireless_free_list_data(tmp->wpa3_pass_list);
        if (tmp->wpa3_sae_groups)
            wireless_free_list_data(tmp->wpa3_sae_groups);
        if (tmp->wpa3_owe_groups)
            wireless_free_list_data(tmp->wpa3_owe_groups);

        free(tmp);
    }

    wl->vsec = NULL;
    return 0;
}

static int opt_bool_revert(char *optval, char *ucival)
{
    if (!optval || !ucival)
        return -1;

    if (strcmp(optval, "0" ) == 0)
    {
        snprintf(ucival, UCI_MAX_STR_LEN, "1");
    }
    else if (strcmp(optval, "1" ) == 0)
    {
        snprintf(ucival, UCI_MAX_STR_LEN, "0");
    }
    else
    {
        return -1;
    }

    return 0;
}

static int opt_radidx_to_device(char *optval, char *ucival)
{
    if (!optval || !ucival)
        return -1;

    snprintf(ucival, UCI_MAX_STR_LEN, "wifi%d", atoi(optval) - 1);
    return 0;
}

static int opt_parse_bcntype(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    struct vapsec_s *vsec = wireless_get_vapsec(wl, vapidx);

    if (!vsec)
    {
        return -1;
    }

    if (strcmp(value, "None") == 0)
    {
        vsec->bcn_type = BEACONTYPE_eNone;
    }
    else if (strcmp(value, "Basic") == 0)
    {
        vsec->bcn_type = BEACONTYPE_eBasic;
    }
    else if (strcmp(value, "WPA") == 0)
    {
        vsec->bcn_type = BEACONTYPE_eWPA;
    }
    else if (strcmp(value, "11i") == 0)
    {
        vsec->bcn_type = BEACONTYPE_e11i;
    }
    else if (strcmp(value, "WPAand11i") == 0)
    {
        vsec->bcn_type = BEACONTYPE_eWPAand11i;
    }
    else
    {
        return -1;
    }

    return 0;
}

static int opt_parse_wepencr(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    struct vapsec_s *vsec = wireless_get_vapsec(wl, vapidx);

    if (!vsec)
    {
        return -1;
    }

    if (strcmp(value, "None") == 0)
    {
        vsec->wep_encr = BASICENCRYPTIONMODES_eNone;
    }
    else if (strcmp(value, "WEPEncryption") == 0)
    {
        vsec->wep_encr = BASICENCRYPTIONMODES_eWEPEncryption;
    }
    else
    {
        return -1;
    }

    return 0;
}

static int opt_parse_wepauth(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    struct vapsec_s *vsec = wireless_get_vapsec(wl, vapidx);

    if (!vsec)
    {
        return -1;
    }

    if (strcmp(value, "None") == 0)
    {
        vsec->wep_auth = BASICAUTHENTICATIONMODE_eNone;
    }
    else if (strcmp(value, "EAPAuthentication") == 0)
    {
        vsec->wep_auth = BASICAUTHENTICATIONMODE_eEAPAuthentication;
    }
    else if (strcmp(value, "SharedAuthentication") == 0)
    {
        vsec->wep_auth = BASICAUTHENTICATIONMODE_eSharedAuthentication;
    }
    else if (strcmp(value, "Both") == 0)
    {
        vsec->wep_auth = BASICAUTHENTICATIONMODE_eBoth;
    }
    else
    {
        return -1;
    }

    return 0;
}

static int opt_parse_wpaencr(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    struct vapsec_s *vsec = wireless_get_vapsec(wl, vapidx);

    if (!vsec)
    {
        return -1;
    }

    if (strcmp(value, "TKIPEncryption") == 0)
    {
        vsec->wpa_encr = WPAENCRYPTIONMODES_eTKIPEncryption;
    }
    else if (strcmp(value, "AESEncryption") == 0)
    {
        vsec->wpa_encr = WPAENCRYPTIONMODES_eAESEncryption;
    }
    else if (strcmp(value, "TKIPandAESEncryption") == 0)
    {
        vsec->wpa_encr = WPAENCRYPTIONMODES_eTKIPandAESEncryption;
    }
    else
    {
        return -1;
    }

    return 0;
}

static int opt_parse_wpaauth(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    struct vapsec_s *vsec = wireless_get_vapsec(wl, vapidx);

    if (!vsec)
    {
        return -1;
    }

    if (strcmp(value, "PSKAuthentication") == 0)
    {
        vsec->wpa_auth = WPAAUTHENTICATIONMODE_ePSKAuthentication;
    }
    else if (strcmp(value, "EAPAuthentication") == 0)
    {
        vsec->wpa_auth = WPAAUTHENTICATIONMODE_eEAPAuthentication;
    }
    else
    {
        return -1;
    }

    return 0;
}

static int opt_parse_11iencr(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    struct vapsec_s *vsec = wireless_get_vapsec(wl, vapidx);

    if (!vsec)
    {
        return -1;
    }

    if (strcmp(value, "TKIPEncryption") == 0)
    {
        vsec->wpa2_encr = IEEE11IENCRYPTIONMODES_eTKIPEncryption;
    }
    else if (strcmp(value, "AESEncryption") == 0)
    {
        vsec->wpa2_encr = IEEE11IENCRYPTIONMODES_eAESEncryption;
    }
    else if (strcmp(value, "TKIPandAESEncryption") == 0)
    {
        vsec->wpa2_encr = IEEE11IENCRYPTIONMODES_eTKIPandAESEncryption;
    }
    else
    {
        return -1;
    }

    return 0;
}

static int opt_parse_11iauth(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    struct vapsec_s *vsec = wireless_get_vapsec(wl, vapidx);

    if (!vsec)
    {
        return -1;
    }

    if (strcmp(value, "PSKAuthentication") == 0)
    {
        vsec->wpa2_auth = IEEE11IAUTHENTICATIONMODE_ePSKAuthentication;
    }
    else if (strcmp(value, "EAPAuthentication") == 0)
    {
        vsec->wpa2_auth = IEEE11IAUTHENTICATIONMODE_eEAPAuthentication;
    }
    else if (strcmp(value, "EAPandPSKAuthentication") == 0)
    {
        vsec->wpa2_auth = IEEE11IAUTHENTICATIONMODE_eEAPandPSKAuthentication;
    }
    else if (strcmp(value, "WPA3Authentication") == 0)
    {
        vsec->wpa3_auth = IEEE11IAUTHENTICATIONMODE_eWPA3Authentication;
    }
    else if (strcmp(value, "EAPandWPA3Authentication") == 0)
    {
        vsec->wpa3_auth = IEEE11IAUTHENTICATIONMODE_eEAPandWPA3Authentication;
    }
    else
    {
        return -1;
    }

    return 0;
}

static int opt_parse_psk(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    struct vapsec_s *vsec = wireless_get_vapsec(wl, vapidx);

    if (!vsec)
    {
        return -1;
    }

    if (strcmp(name, "KeyPassphrase") == 0 )
    {
        vsec->passphrase = strdup(value);
    }
    if (strcmp(name, "PreSharedKey.1.PreSharedKey") == 0 )
    {
        vsec->psk = strdup(value);
    }

    return 0;
}

static int opt_parse_wepkeyidx(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    struct vapsec_s *vsec = wireless_get_vapsec(wl, vapidx);

    if (!vsec || !value)
    {
        return -1;
    }

    vsec->wep_keyidx = atoi(value);
    return 0;
}

static int opt_parse_standard(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    char hwmode[10];
    char htmode[10];
    char optv[UCI_MAX_STR_LEN+1] = {0};
    char ucival[UCI_MAX_STR_LEN+1] = {0};
    char device[UCI_MAX_STR_LEN+1] = {0};
    char orig_hwmode[UCI_MAX_STR_LEN + 1] = {0};
    // Translate standard into the format required for
    // "iwpriv athx mode" command.
    char wireless_htmode[IEEE80211_MAX_HTMODE_LEN];
    int ret;
    struct vapsec_s *vsec;

    memset(hwmode, 0, sizeof(hwmode));
    memset(htmode, 0, sizeof(htmode));
    memset(optv, '\0', sizeof(optv));
    memset(wireless_htmode, 0, sizeof(wireless_htmode));

    snprintf(optv, sizeof(optv), "wireless.ath%d.device", vapidx);
    ret = syscfg_get(NULL, optv, device, sizeof(device));

    if (ret || strlen(device) > IFNAMSIZ )
        return -1;

    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wireless.%s.hwmode", device);
    ret = syscfg_get(NULL, optv, orig_hwmode, sizeof(orig_hwmode));

    if (ret) { return -1; }

    if (strcmp(value, "a") == 0 ||
        strcmp(value, "b") == 0 ||
        strcmp(value, "g") == 0 ||
        strcmp(value, "alow") == 0)
    {
        snprintf(hwmode, sizeof(hwmode), "11%s", value);
    }
    else if (strcmp(value, "na20") == 0 ||
            strcmp(value, "na20low") == 0)
    {
        strlcpy(hwmode, "11na", sizeof(hwmode));
        strlcpy(htmode, "HT20", sizeof(htmode));
        strlcpy(wireless_htmode, "11NAHT20", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "na40") == 0 ||
            strcmp(value, "na40low") == 0)
    {
        strlcpy(hwmode, "11na", sizeof(hwmode));
        strlcpy(htmode, "HT40",  sizeof(htmode));
        strlcpy(wireless_htmode, "11NAHT40", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "na40plus") == 0 ||
            strcmp(value, "na40pluslow") == 0)
    {
        strlcpy(hwmode, "11na", sizeof(hwmode));
        strlcpy(htmode, "HT40+", sizeof(htmode));
        strlcpy(wireless_htmode, "11NAHT40PLUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "na40minus") == 0 ||
            strcmp(value, "na40minuslow") == 0)
    {
        strlcpy(hwmode, "11na", sizeof(hwmode));
        strlcpy(htmode, "HT40-", sizeof(htmode));
        strlcpy(wireless_htmode, "11NAHT40MINUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "ng20") == 0 ||
            strcmp(value, "ng20low") == 0)
    {
        strlcpy(hwmode, "11ng", sizeof(hwmode));
        strlcpy(htmode, "HT20", sizeof(htmode));
        strlcpy(wireless_htmode, "11NGHT20", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "ng40") == 0 ||
            strcmp(value, "ng40low") == 0)
    {
        strlcpy(hwmode, "11ng", sizeof(hwmode));
        strlcpy(htmode, "HT40", sizeof(htmode));
        strlcpy(wireless_htmode, "11NGHT40", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "ng40plus") == 0 ||
            strcmp(value, "ng40pluslow") == 0)
    {
        strlcpy(hwmode, "11ng", sizeof(hwmode));
        strlcpy(htmode, "HT40+", sizeof(htmode));
        strlcpy(wireless_htmode, "11NGHT40PLUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "ng40minus") == 0 ||
            strcmp(value, "ng40minuslow") == 0)
    {
        strlcpy(hwmode, "11ng", sizeof(hwmode));
        strlcpy(htmode, "HT40-", sizeof(htmode));
        strlcpy(wireless_htmode, "11NGHT40MINUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "acvht20") == 0 ||
            strcmp(value, "acvht20low") == 0)
    {
        strlcpy(hwmode, "11ac", sizeof(hwmode));
        strlcpy(htmode, "HT20", sizeof(htmode));
        strlcpy(wireless_htmode, "11ACVHT20", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "acvht40") == 0 ||
            strcmp(value, "acvht40low") == 0)
    {
        strlcpy(hwmode, "11ac", sizeof(hwmode));
        strlcpy(htmode, "HT40", sizeof(htmode));
        strlcpy(wireless_htmode, "11ACVHT40", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "acvht40plus") == 0 ||
            strcmp(value, "acvht40pluslow") == 0)
    {
        strlcpy(hwmode, "11ac", sizeof(hwmode));
        strlcpy(htmode, "HT40+", sizeof(htmode));
        strlcpy(wireless_htmode, "11ACVHT40PLUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "acvht40minus") == 0 ||
            strcmp(value, "acvht40minuslow") == 0)
    {
        strlcpy(hwmode, "11ac", sizeof(hwmode));
        strlcpy(htmode, "HT40-", sizeof(htmode));
        strlcpy(wireless_htmode, "11ACVHT40MINUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "acvht80") == 0 ||
            strcmp(value, "acvht80low") == 0)
    {
        strlcpy(hwmode, "11ac", sizeof(hwmode));
        strlcpy(htmode, "HT80", sizeof(htmode));
        strlcpy(wireless_htmode, "11ACVHT80", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "acvht160") == 0 ||
            strcmp(value, "acvht160low") == 0)
    {
        strlcpy(hwmode, "11ac", sizeof(hwmode));
        strlcpy(htmode, "HT160", sizeof(htmode));
        strlcpy(wireless_htmode, "11ACVHT160", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "acvht80_80") == 0 ||
            strcmp(value, "acvht80_80low") == 0)
    {
        strlcpy(hwmode, "11ac", sizeof(hwmode));
        strlcpy(htmode, "HT80_80", sizeof(htmode));
        strlcpy(wireless_htmode, "11ACVHT80_80", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe20_6g") == 0 )
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT20", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE20", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe40_6g") == 0 )
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT40", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE40", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe40plus_6g") == 0 )
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT40+", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE40PLUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe40minus_6g") == 0 )
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT40-", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE40MINUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe80_6g") == 0 )
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT80", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE80", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe160_6g") == 0 )
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT160", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE160", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe80_80_6g") == 0 )
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT80_80", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE80_80", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht20_6g") == 0 )
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT20", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT20", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht40_6g") == 0 )
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT40", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT40", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht40plus_6g") == 0 )
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT40+", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT40PLUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht40minus_6g") == 0 )
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT40-", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT40MINUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht80_6g") == 0 )
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT80", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT80", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht160_6g") == 0 )
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT160", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT160", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht320_6g") == 0 )
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT320", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT320", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe20") == 0 ||
            strcmp(value, "axahe20low") == 0)
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT20", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE20", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe40") == 0 ||
            strcmp(value, "axahe40low") == 0)
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT40", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE40", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe40plus") == 0 ||
            strcmp(value, "axahe40pluslow") == 0)
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT40+", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE40PLUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe40minus") == 0 ||
            strcmp(value, "axahe40minuslow") == 0)
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT40-", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE40MINUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe80") == 0 ||
            strcmp(value, "axahe80low") == 0)
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT80", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE80", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe160") == 0 ||
            strcmp(value, "axahe160low") == 0)
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT160", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE160", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axahe80_80") == 0 ||
            strcmp(value, "axahe80_80low") == 0)
    {
        strlcpy(hwmode, "11axa", sizeof(hwmode));
        strlcpy(htmode, "HT80_80", sizeof(htmode));
        strlcpy(wireless_htmode, "11AHE80_80", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axghe20") == 0 )
    {
        strlcpy(hwmode, "11axg", sizeof(hwmode));
        strlcpy(htmode, "HT20", sizeof(htmode));
        strlcpy(wireless_htmode, "11GHE20", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axghe40") == 0 )
    {
        strlcpy(hwmode, "11axg", sizeof(hwmode));
        strlcpy(htmode, "HT40", sizeof(htmode));
        strlcpy(wireless_htmode, "11GHE40", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axghe40plus") == 0 )
    {
        strlcpy(hwmode, "11axg", sizeof(hwmode));
        strlcpy(htmode, "HT40+", sizeof(htmode));
        strlcpy(wireless_htmode, "11GHE40PLUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "axghe40minus") == 0 )
    {
        strlcpy(hwmode, "11axg", sizeof(hwmode));
        strlcpy(htmode, "HT40-", sizeof(htmode));
        strlcpy(wireless_htmode, "11GHE40MINUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht20") == 0 ||
            strcmp(value, "beaeht20low") == 0)
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT20", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT20", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht40") == 0 ||
            strcmp(value, "beaeht40low") == 0)
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT40", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT40", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht40plus") == 0 ||
            strcmp(value, "beaeht40pluslow") == 0)
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT40+", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT40PLUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht40minus") == 0 ||
            strcmp(value, "beaeht40minuslow") == 0)
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT40-", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT40MINUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht80") == 0 ||
            strcmp(value, "beaeht80low") == 0)
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT80", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT80", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht160") == 0 ||
            strcmp(value, "beaeht160low") == 0)
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT160", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT160", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "beaeht320") == 0 ||
            strcmp(value, "beaeht320low") == 0)
    {
        strlcpy(hwmode, "11bea", sizeof(hwmode));
        strlcpy(htmode, "EHT320", sizeof(htmode));
        strlcpy(wireless_htmode, "11AEHT320", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "begeht20") == 0 )
    {
        strlcpy(hwmode, "11beg", sizeof(hwmode));
        strlcpy(htmode, "EHT20", sizeof(htmode));
        strlcpy(wireless_htmode, "11GEHT20", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "begeht40") == 0 )
    {
        strlcpy(hwmode, "11beg", sizeof(hwmode));
        strlcpy(htmode, "EHT40", sizeof(htmode));
        strlcpy(wireless_htmode, "11GEHT40", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "begeht40plus") == 0 )
    {
        strlcpy(hwmode, "11beg", sizeof(hwmode));
        strlcpy(htmode, "EHT40+", sizeof(htmode));
        strlcpy(wireless_htmode, "11GEHT40PLUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "begeht40minus") == 0 )
    {
        strlcpy(hwmode, "11beg", sizeof(hwmode));
        strlcpy(htmode, "EHT40-", sizeof(htmode));
        strlcpy(wireless_htmode, "11GEHT40MINUS", sizeof(wireless_htmode));
    }
    else if (strcmp(value, "auto") == 0 )
    {
        // When Registrar is operating in AUTO mode, keep original hwmode and
        // set htmode to AUTO.
        strlcpy(hwmode, orig_hwmode, sizeof(hwmode));
        strlcpy(htmode, "AUTO", sizeof(htmode));
        strlcpy(wireless_htmode, "AUTO", sizeof(wireless_htmode));
    }
    else
    {
        printf("unhandled standard %s\n", value);
        return 0;
    }

    if (strcmp(hwmode, orig_hwmode))
    {
        ret = syscfg_set(NULL, optv, hwmode);
        wl->changed = true;
    }

    if (ret)
        return ret;

    if (!strlen(htmode))
        return 0;

    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wireless.%s.htmode", device);
    ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

    if (ret ||
        strcmp(htmode, ucival))
    {
        ret = syscfg_set(NULL, optv, htmode);
        vsec = wireless_get_vapsec(wl, vapidx);

        if (!vsec) { return -1; }

        if (vsec->htmode) {
            free(vsec->htmode);
        }
        vsec->htmode = strdup(wireless_htmode);
        vsec->opti_changed = true;
    }
    syscfg_commit();
    return ret;
}

static int opt_parse_upstream_version(struct wireless_s *wl, int radioidx,
                                      char *name, char *value) {
    char optv[UCI_MAX_STR_LEN];
    char ucival[UCI_MAX_STR_LEN+1] = {0};
    int ret;

    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wireless.wifi%d.upstream_version", radioidx - 1);
    ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

    if ( strcmp(value, ucival) ) {
         ret = syscfg_set(NULL, optv, value);
    }
    syscfg_commit();
    return ret;
}
static int opt_parse_wifi_reset(struct wireless_s *wl, int radioidx, char *name, char *value) {
    struct vapsec_s *vsec = wl->vsec;
    /* non zero values will execute wifi restart while applying uci ,
       zero means wifi restart shall be avoided */
    wl->changed = false;
    vsec = wl->vsec;
    while (vsec)
    {
        vsec->changed = false;
        vsec->opti_changed = false;
        vsec = vsec->next;
    }
    return 0;
}

static int opt_parse_channel(struct wireless_s *wl, int radioidx, char *name, char *value)
{
        char optv[UCI_MAX_STR_LEN];
        char ucival[UCI_MAX_STR_LEN];
        char newval[UCI_MAX_STR_LEN];
        char device[UCI_MAX_STR_LEN];
        char vapphy[UCI_MAX_STR_LEN];
        char independchannelval[UCI_MAX_STR_LEN];
        int ret;
        bool foundvap = false,independchannelenable = false;
        struct vapsec_s *vsec;

        memset(independchannelval,'\0',UCI_MAX_STR_LEN);

        if (strcmp(value, "0") == 0 )
                snprintf(newval, sizeof(newval), "auto");
        else
                snprintf(newval, UCI_MAX_STR_LEN, "%s", value);

        snprintf(device, sizeof(device), "wifi%d", radioidx -1);
        memset(optv, '\0', sizeof(optv));
        snprintf(optv, sizeof(optv), "wireless.%s.independent_channel_set", device);

        ret = syscfg_set(NULL, optv, independchannelval);

        if (!ret)
        {
               if (strcmp(independchannelval, "1") == 0 )
                    independchannelenable=true;
        }

        memset(optv, '\0', sizeof(optv));
        snprintf(optv, sizeof(optv), "wireless.%s.channel", device);

        ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

        if ( (ret || strcmp(newval, ucival)) && !independchannelenable)
        {
                memset(optv, '\0', sizeof(optv));
                snprintf(optv, sizeof(optv), "wireless.%s.channel", device);
                ret = syscfg_set(NULL, optv, newval);
                /* set channel to vap if it exists*/
                vsec = wl->vsec;
                while (vsec)
                {
                        memset(optv, '\0', sizeof(optv));
                        snprintf(optv, sizeof(optv), "wireless.ath%d.device", vsec->vapidx);
                        ret = syscfg_get(NULL, optv, vapphy, sizeof(vapphy));

            if (ret == 0 && strcmp(device, vapphy) == 0) {
                if (vsec->channel) {
                    free(vsec->channel);
                }
                vsec->channel = strdup(newval);
                vsec->opti_changed = true;
                foundvap = true;
            }

            vsec = vsec->next;
        }

        /* As long as a VAP was found on which to set the channel for
         * this radio, use the optimized restart mechanism.
         *
         * If no VAP was found (which generally should not happen), do
         * a full restart even though it may not be strictly necessary.
         * This full restart is the conservative strategy in this unexpected
         * circumstance.
         */
        if (!foundvap)
        {
            wl->changed = true;
        }
    }
    syscfg_commit();
    return ret;
}

static int opt_parse_vapind(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    char optv[UCI_MAX_STR_LEN];
    char ucival[UCI_MAX_STR_LEN];
    int ret;
    struct vapsec_s *vsec;

    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wireless.ath%d.athnewind", vapidx);
    ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

    if (ret ||
        strcmp(value, ucival))
    {
        ret = syscfg_set(NULL, optv, value);
        vsec = wireless_get_vapsec(wl, vapidx);
        if (!vsec)
        {
            return -1;
        }
        if (vsec->vapind)
            free(vsec->vapind);
        vsec->vapind = strdup(value);
        vsec->opti_changed = true;
    }
    syscfg_commit();
    return ret;
}

static int opt_parse_pbc(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    char optv[UCI_MAX_STR_LEN];
    char ucival[UCI_MAX_STR_LEN];
    int ret;

    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wireless.ath%d.wps_pbc", vapidx);
    ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

    if (ret ||
        strcmp(value, ucival))
    {
    ret = syscfg_set(NULL, optv, value);
        wl->changed = true;
    }
    syscfg_commit();
    return ret;
}

static int opt_parse_target_bssid(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    char optv[UCI_MAX_STR_LEN] = {0};
    char ucival[UCI_MAX_STR_LEN] = {0};
    char buf[UCI_MAX_STR_LEN] = {0};
    int ret;

    struct vapsec_s *vsec;
    vsec = wireless_get_vapsec(wl, vapidx);
    if (!vsec)
    {
        dprintf("%s: vsec is NULL \n", __func__);
        return -1;
    }

    if (strlen(vsec->ifname) <= 0
        && wireless_get_ifname_by_vapidx(vsec->vapidx, vsec->ifname) < 0)
    {
        dprintf("%s: ifname len <= 0 or Failed to get ifindex from ifname \n", __func__);
        return -1;
    }

    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wireless.ath%d.bssid", vapidx);
    ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

    snprintf(buf, sizeof(buf),"wpa_cli -p /var/run/wpa_supplicant-%s list_network | grep -i %s | awk '{print $1}' &> /tmp/wpa_netid",
             vsec->ifname, ucival);
    ret = system(buf);
    if (ret == -1) {
        dprintf("%s Failed\n", buf);
        return ret;
    }

    snprintf(buf, sizeof(buf),"if [ ! -e /tmp/last_netid ]; then echo '0' > /tmp/last_netid; fi");
    ret = system(buf);
    if (ret == -1) {
        dprintf("%s Failed\n", buf);
        return ret;
    }

    snprintf(buf, sizeof(buf), "i=$(cat /tmp/wpa_netid); if [ -n \"$i\" ]; then  mv /tmp/wpa_netid /tmp/last_netid; fi;");
    ret = system(buf);
    if (ret == -1) {
        dprintf("%s Failed\n", buf);
        return ret;
    }

    snprintf(buf, sizeof(buf), "i=$(cat /tmp/last_netid); wpa_cli -p /var/run/wpa_supplicant-%s disable_network $i;",
             vsec->ifname);
    ret = system(buf);
    if (ret == -1) {
        dprintf("%s Failed\n", buf);
        return ret;
    }

    snprintf(buf, sizeof(buf), "i=$(cat /tmp/last_netid); wpa_cli -p /var/run/wpa_supplicant-%s set_network $i bssid %s;",
             vsec->ifname, value);
    ret = system(buf);
    if (ret == -1) {
        dprintf("%s Failed\n", buf);
        return ret;
    }
    sleep(1); // sleep for 1 seconds

    snprintf(buf, sizeof(buf), "i=$(cat /tmp/last_netid); wpa_cli -p /var/run/wpa_supplicant-%s enable_network $i;",
             vsec->ifname);
    ret = system(buf);
    if (ret == -1) {
        dprintf("%s Failed\n", buf);
        return ret;
    }

    system("rm /tmp/last_netid");

    ret = syscfg_set(NULL, optv, value);

    syscfg_commit();

    return ret;
}

static int opt_parse_sae_parameter(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    struct vapsec_s *vsec = wireless_get_vapsec(wl, vapidx);

    if (!vsec)
    {
        return -1;
    }

    if (strcmp(name, "EnableSAE") == 0 )
    {
        if (strcmp(value, "0") == 0)
        {
            vsec->wpa3_sae = WPA3SAEVALUE_eDisable;
        }
        else if (strcmp(value, "1") == 0)
        {
            vsec->wpa3_sae = WPA3SAEVALUE_eEnable;
        }
        else
        {
            vsec->wpa3_sae = WPA3SAEVALUE_eNotSet;
        }
    }
    else if (strcmp(name, "SAEPassword") == 0 || strcmp(name, "DeleteSAEPassword") == 0)
    {
        struct list_data *new_data = (struct list_data*) calloc (1, sizeof(struct list_data));
        if (new_data == NULL) {
            return -1;
        }
        if (strcmp(name, "SAEPassword") == 0 ) {
            new_data->action = LISTACTION_ADD;
        } else {
            new_data->action = LISTACTION_DEL;
        }
        new_data->data = strdup(value);
        if (!new_data->data) {
            free (new_data);
            return -1;
        }
        new_data->next = vsec->wpa3_pass_list;
        vsec->wpa3_pass_list = new_data;
    }
    else if (strcmp(name, "SAEGroups") == 0 || strcmp(name, "DeleteSAEGroups") == 0)
    {
        struct list_data *new_data = (struct list_data*) calloc (1, sizeof(struct list_data));
        if (new_data == NULL) {
            return -1;
        }
        if (strcmp(name, "SAEGroups") == 0 ) {
            new_data->action = LISTACTION_ADD;
        } else {
            new_data->action = LISTACTION_DEL;
        }
        new_data->data = strdup(value);
        if (!new_data->data) {
            free (new_data);
            return -1;
        }
        new_data->next = vsec->wpa3_sae_groups;
        vsec->wpa3_sae_groups = new_data;
    }
    else if ( strcmp(name, "en_6g_sec_comp") == 0 ) {
        if (strcmp(value, "0") == 0)
        {
            vsec->wpa3_en_6g_sec_comp = 0;
        }
        else if (strcmp(value, "1") == 0)
        {
            vsec->wpa3_en_6g_sec_comp = 1;
        }
        else
        {
            vsec->wpa3_en_6g_sec_comp = -1;
        }
    }
    else if ( strcmp(name, "sae_pwe") == 0 ) {
        if (strcmp(value, "0") == 0)
        {
            vsec->wpa3_sae_pwe = 0;
        }
        else if (strcmp(value, "1") == 0)
        {
            vsec->wpa3_sae_pwe = 1;
        }
        else if (strcmp(value, "2") == 0)
        {
            vsec->wpa3_sae_pwe = 2;
        }
    else
        {
            vsec->wpa3_sae_pwe = -1;
        }

    }

    return 0;
}

static int opt_parse_owe_parameter(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    struct vapsec_s *vsec = wireless_get_vapsec(wl, vapidx);

    if (!vsec)
    {
        return -1;
    }

    if (strcmp(name, "EnableOWE") == 0 )
    {
        if (strcmp(value, "0") == 0)
        {
            vsec->wpa3_owe = WPA3OWEVALUE_eDisable;
        }
        else if (strcmp(value, "1") == 0)
        {
            vsec->wpa3_owe = WPA3OWEVALUE_eEnable;
        }
        else
        {
            vsec->wpa3_owe = WPA3OWEVALUE_eNotSet;
        }
    }
    else if (strcmp(name, "OWEGroups") == 0 || strcmp(name, "DeleteOWEGroups") == 0)
    {
        struct list_data *new_data = (struct list_data*) calloc (1, sizeof(struct list_data));
        if (new_data == NULL) {
            return -1;
        }
        if (strcmp(name, "OWEGroups") == 0 ) {
            new_data->action = LISTACTION_ADD;
        } else {
            new_data->action = LISTACTION_DEL;
        }
        new_data->data = strdup(value);
        if (!new_data->data) {
            free (new_data);
            return -1;
        }
        new_data->next = vsec->wpa3_owe_groups;
        vsec->wpa3_owe_groups = new_data;
    }

    return 0;
}

static int opt_parse_network(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    char optv[UCI_MAX_STR_LEN];
    char ucival[UCI_MAX_STR_LEN];
    int ret;

    struct vapsec_s *vsec;
    vsec = wireless_get_vapsec(wl, vapidx);
    if (!vsec)
    {
        return -1;
    }
    
    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wireless.ath%d.network", vapidx);
    ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));
    if (ret ||
        strcmp(value, ucival))
    {
    ret = syscfg_set(NULL, optv, value);
        vsec->changed = true;
    }
    syscfg_commit();
    return ret;
}

static int opt_parse_vap_type(struct wireless_s *wl, int vapidx, char *name, char *value)
{
    struct vapsec_s *vsec = wireless_get_vapsec(wl, vapidx);

    if (!vsec)
    {
        return -1;
    }
    if (strcmp(name, "VAPType") == 0 )
    {
        vsec->vap_type = atoi(value);
    }
    return 0;
}

static struct wl_translate_option radioTbl[] =
{
    { "RdioEnabled",            "disabled",     opt_bool_revert,      NULL},
    { "Channel",                "channel",      NULL,                 NULL},
    { "ClonedChannel",          NULL,           NULL,                 opt_parse_channel},
    { "X_ATH-COM_Powerlevel",   "txpower",      NULL,                 NULL},
    { "X_ATH-COM_Rxchainmask",  "rxchainmask",  NULL,                 NULL},
    { "X_ATH-COM_Txchainmask",  "txchainmask",  NULL,                 NULL},
    { "X_ATH-COM_TBRLimit",     NULL,           NULL,                 NULL},
    { "ATH-COM_AMPDUEnabled",   "AMPDU",        NULL,                 NULL},
    { "X_ATH-COM_AMPDULimit",   "AMPDULim",     NULL,                 NULL},
    { "X_ATH-COM_AMPDUFrames",  NULL,           NULL,                 NULL},
    { "upstream_version",      "upstream_version", NULL,              opt_parse_upstream_version},
    { "wifi_reset",             NULL,           NULL,                 opt_parse_wifi_reset},
    { NULL }
};

static struct wl_translate_option mldTbl[] =
{
    { "mld_ssid",           "mld_ssid",      NULL,                 NULL},
    { "mld_mac",            "mld_macaddr",   NULL,                 NULL},
    { "mld_role",           "role",          NULL,                 NULL},
    { NULL }
};

static struct wl_translate_option vapTbl[] =
{
    { "Enable",                       "disabled",     opt_bool_revert,         NULL },
    { "X_ATH-COM_RadioIndex",         "device",       opt_radidx_to_device,    NULL },
    { "SSID",                         "ssid",         NULL,                    NULL },
    { "BeaconType",                   NULL,           NULL,                    opt_parse_bcntype },
    { "Standard",                     NULL,           NULL,                    opt_parse_standard  },
    { "WEPKeyIndex",                  NULL,           NULL,                    opt_parse_wepkeyidx },
    { "KeyPassphrase",                NULL,           NULL,                    opt_parse_psk },
    { "BasicEncryptionModes",         NULL,           NULL,                    opt_parse_wepencr },
    { "BasicAuthenticationMode",      NULL,           NULL,                    opt_parse_wepauth },
    { "WPAEncryptionModes",           NULL,           NULL,                    opt_parse_wpaencr },
    { "WPAAuthenticationMode",        NULL,           NULL,                    opt_parse_wpaauth },
    { "IEEE11iEncryptionModes",       NULL,           NULL,                    opt_parse_11iencr },
    { "IEEE11iAuthenticationMode",    NULL,           NULL,                    opt_parse_11iauth },
    { "BasicDataTransmitRates",       "mcast_rate",   NULL,                    NULL },
    { "RTS",                          "rts",          NULL,                    NULL },
    { "Fragmentation",                "frag",         NULL,                    NULL },
    { "VAPIndependent",               "athnewind",    NULL,                    opt_parse_vapind },
    { "PeerBSSID",                    "bssid",        NULL,                    NULL },
    { "AuthenticationServiceMode",    NULL,           NULL,                    NULL },
    { "X_ATH-COM_EAPReauthPeriod",    "eap_reauth_period", NULL,               NULL },
    { "X_ATH-COM_WEPRekeyPeriod",     "wep_rekey",    NULL,                    NULL },
    { "X_ATH-COM_AuthServerAddr",     "auth_server",       NULL,                    NULL },
    { "X_ATH-COM_AuthServerPort",     "auth_port",         NULL,                    NULL },
    { "X_ATH-COM_AuthServerSecret",   "auth_secret",       NULL,                    NULL },
    { "X_ATH-COM_AuthServerAddr2",    "auth_server2",      NULL,                    NULL },
    { "X_ATH-COM_AuthServerPort2",    "auth_port2",        NULL,                    NULL },
    { "X_ATH-COM_AuthServerSecret2",  "auth_secret2",      NULL,                    NULL },
    { "X_ATH-COM_RSNPreAuth",         "rsn_preauth",  NULL,                    NULL },
    { "X_ATH-COM_RSNPreAuthIfaces",   "rsn_preauth_interfaces", NULL,          NULL },
    { "X_ATH-COM_GroupMgmtCipher",    "group_mgmt_cipher", NULL,               NULL },
    { "X_ATH-COM_SSIDHide",           "hidden",       NULL,                    NULL },
    { "X_ATH-COM_APModuleEnable",     NULL,           NULL,                    NULL },
    { "X_ATH-COM_WPSPin",             "wps_pin",      NULL,                    NULL },
    { "X_ATH-COM_WPSConfigured",      NULL,           NULL,                    NULL },
    { "X_ATH-COM_ShortGI",            "shortgi",      NULL,                    NULL },
    { "X_ATH-COM_CWMEnable",          NULL,           NULL,                    NULL },
    { "X_ATH-COM_WMM",                "wmm",          NULL,                    NULL },
    { "X_ATH-COM_HT40Coexist",        "disablecoext", opt_bool_revert,         NULL },
    { "X_ATH-COM_HBREnable",          NULL,           NULL,                    NULL },
    { "X_ATH-COM_HBRPERLow",          NULL,           NULL,                    NULL },
    { "X_ATH-COM_HBRPERHigh",         NULL,           NULL,                    NULL },
    { "X_ATH-COM_MEMode",             NULL,           NULL,                    NULL },
    { "X_ATH-COM_MELength",           NULL,           NULL,                    NULL },
    { "X_ATH-COM_METimer",            NULL,           NULL,                    NULL },
    { "X_ATH-COM_METimeout",          NULL,           NULL,                    NULL },
    { "X_ATH-COM_MEDropMcast",        NULL,           NULL,                    NULL },
    { "X_ATH-COM_MOBILITYDOMAIN",     "mobility_domain",        NULL,                    NULL },
    { "X_ATH-COM_KH_KEY_HEX",         "kh_key_hex",   NULL,                    NULL },
    { "WEPKey.1.WEPKey",              "key1",         NULL,                    NULL },
    { "WEPKey.2.WEPKey",              "key2",         NULL,                    NULL },
    { "WEPKey.3.WEPKey",              "key3",         NULL,                    NULL },
    { "WEPKey.4.WEPKey",              "key4",         NULL,                    NULL },
    { "X_ATH-COM_GroupRekeyPeriod",  "wpa_group_rekey", NULL,                  NULL },
    { "PreSharedKey.1.PreSharedKey",  NULL,           NULL,                    opt_parse_psk },
    { "Channel",                      NULL,           NULL,                    NULL },
    { "network",                      "network",      NULL,                    opt_parse_network },
    { "mode",                         "mode",         NULL,                    NULL },
    { "map",                          "map",          NULL,                    NULL},
    { "MapBSSType",                   "MapBSSType",   NULL,                    NULL},
    { "wps_pbc",                      "wps_pbc",      NULL,                    opt_parse_pbc },
    { "wps_pbc_skip",                 "wps_pbc_skip", NULL,                    NULL},
    { "wps_pbc_enable",               "wps_pbc_enable", NULL,                  NULL},
    { "wps_pbc_start_time",           "wps_pbc_start_time", NULL,              NULL},
    { "wps_pbc_duration",             "wps_pbc_duration", NULL,                NULL},
    { "wds",                          "wds",          NULL,                    NULL},
    { "rrm",                          "rrm",          NULL,                    NULL},
    { "mscs",                         "mscs",         NULL,                    NULL},
    { "scs",                          "scs",          NULL,                    NULL},
    { "dscp_action_policy",           "dscp_action_policy", NULL,              NULL},
    { "mapTargetBSSID",               "bssid",        NULL,                    opt_parse_target_bssid },
    { "EnableSAE",                    NULL,           NULL,                    opt_parse_sae_parameter },
    { "SAEPassword",                  NULL,           NULL,                    opt_parse_sae_parameter },
    { "DeleteSAEPassword",            NULL,           NULL,                    opt_parse_sae_parameter },
    { "SAEAntiCloggingThreshold",     "sae_anti_clogging_threshold", NULL,     NULL},
    { "SAESync",                      "sae_sync",     NULL,                    NULL},
    { "SAERequireMFP",                "sae_require_mfp", NULL,                 NULL},
    { "EnableOWE",                    NULL,           NULL,                    opt_parse_owe_parameter },
    { "SuiteB",                       "suite_b",      NULL,                    NULL},
    { "FT_AUTH",                      "ft_auth",      NULL,                    NULL},
    { "FT_80211R",                    "ieee80211r",   NULL,                    NULL},
    { "SAEGroups",                    NULL,           NULL,                    opt_parse_sae_parameter },
    { "DeleteSAEGroups",              NULL,           NULL,                    opt_parse_sae_parameter },
    { "OWEGroups",                    NULL,           NULL,                    opt_parse_owe_parameter },
    { "DeleteOWEGroups",              NULL,           NULL,                    opt_parse_owe_parameter },
    { "OWETransIfname",               "owe_transition_ifname", NULL,           NULL},
    { "OWETransSSID",                 "owe_transition_ssid", NULL,             NULL},
    { "OWETransBSSID",                "owe_transition_bssid", NULL,            NULL},
    { "IEEE80211w",                   "ieee80211w",   NULL,                    NULL},
    { "mbo",                          "mbo",          NULL,                    NULL},
    { "oce",                          "oce",          NULL,                    NULL},
    { "mapVlanID",                    "mapVlanID",    NULL,                    NULL},
    { "map8021qvlan",                 "map8021qvlan", NULL,                    NULL},
    { "tx_stbc",                      "tx_stbc",      NULL,                    NULL},
    { "rx_stbc",                      "rx_stbc",      NULL,                    NULL},
    { "VAPType",                      NULL,           NULL,                    opt_parse_vap_type },
    { "vlan_bridge",                  "vlan_bridge",  NULL,                    NULL},
    { "amsdu",                        "amsdu",        NULL,                    NULL},
    { "backhaul_ap",                  "backhaul_ap",  NULL,                    NULL},
    { "SteeringDisabled",             "SteeringDisabled",  NULL,               NULL},
    { "sae_pwe",                      NULL,           NULL,                    opt_parse_sae_parameter},
    { "en_6g_sec_comp",               NULL,           NULL,                    opt_parse_sae_parameter},
    { "mld",                          "mld",          NULL,                    NULL},
    { "SkipVapRestart",               "map_SkipVapRestart", NULL,              NULL},
    { "mbss_tx_vdev",                 "mbss_tx_vdev", NULL,                    NULL},
    { "root_distance",                "root_distance", NULL,                   NULL},
    { "dpp",                          "dpp",          NULL,                    NULL},
    { "dpp_map",                      "dpp_map",      NULL,                    NULL},
    { "gas_comeback_delay",           "gas_comeback_delay", NULL,                    NULL},
    { "dpp_controller",               "dpp_controller",     NULL,                    NULL},
    { "dpp_configurator_connectivity","dpp_configurator_connectivity", NULL,     NULL},
    //QCA_EXT_EM_CONTROLLER specific option
    { "emex_isolate",                 "isolate",      NULL,                    NULL},
    { NULL }
};

static int wireless_set_radio(struct wireless_s *wl, char *name, char *value)
{
    char optv[UCI_MAX_STR_LEN];
    char ucival[UCI_MAX_STR_LEN];
    char newval[UCI_MAX_STR_LEN];
    int radidx;
    struct wl_translate_option *iopt;
    int ret;

    radidx = strtoul(name, &name, 0);
    if (!name || *name != '.')
    {
        return -1;
    }

    name ++;

    iopt = radioTbl;
    while(iopt->optname  != NULL)
    {
        if (strcmp(name, iopt->optname) == 0)
            break;
        iopt ++;
    }

    if (!iopt->optname)
    {
        printf("unhandled option %s\n", name);
        return 0;
    }

    if (iopt->optparser &&
        iopt->optparser(wl, radidx, name, value) != 0)
        return -1;

    if (!iopt->uciname)
        return 0;

    if (iopt->opt2uci)
    {
        if (iopt->opt2uci(value, newval) != 0)
            return -1;
        value = newval;
    }

    memset(optv, '\0', sizeof(optv));

    snprintf(optv, sizeof(optv), "wireless.wifi%d.%s", radidx - 1, iopt->uciname);

    ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));
    if (ret
        || strcmp(value, ucival))
    {
    ret = syscfg_set(NULL, optv, value);
        wl->changed = true;
    }
    syscfg_commit();
    return ret;
}

static int wireless_set_mld(struct wireless_s *wl, char *name, char *value)
{
    char optv[UCI_MAX_STR_LEN];
    char ucival[UCI_MAX_STR_LEN];
    char newval[UCI_MAX_STR_LEN];
    int mldidx;
    struct wl_translate_option *iopt;
    int ret;

    mldidx = strtoul(name, &name, 0);
    if (!name || *name != '.')
    {
        return -1;
    }

    name ++;

    iopt = mldTbl;
    while(iopt->optname  != NULL)
    {
        if (strcmp(name, iopt->optname) == 0)
            break;
        iopt ++;
    }

    if (!iopt->optname)
    {
        printf("unhandled option %s\n", name);
        return 0;
    }

    if (iopt->optparser &&
        iopt->optparser(wl, mldidx, name, value) != 0)
        return -1;

    if (!iopt->uciname)
        return 0;

    if (iopt->opt2uci)
    {
        if (iopt->opt2uci(value, newval) != 0)
            return -1;
        value = newval;
    }

    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wireless.mld%d.%s", mldidx - 1, iopt->uciname);
    ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

    if (ret
        || strcmp(value, ucival))
    {
        ret = syscfg_set(NULL, optv, value);
        wl->changed = true;
    }
    syscfg_commit();
    return ret;
}

static int set_vsec_config_changed(struct vapsec_s *vsec)
{
    int vap_specific_restart=0;
    int map_fast_onboard = 0;
    char ucival[UCI_MAX_STR_LEN];
    char optv[UCI_MAX_STR_LEN];

    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wsplcd.config.VapSpecificRestart");

    if (!syscfg_get(NULL, optv, ucival, sizeof(ucival))) {
        vap_specific_restart = atoi(ucival);
    }
    if (vap_specific_restart && vsec->changed == true) {
        /* config applied to wireless file but skipped to restart the VAP due to race condition between multiple wsplcd process.
           To handle this, the vap that needs restart is marked in uci */
        memset(optv, '\0', sizeof(optv));
        snprintf(optv, sizeof(optv), "wireless.ath%d.config_changed", vsec->vapidx);
        syscfg_set(NULL, optv, "1");
        if (!syscfg_get(NULL, optv, ucival, sizeof(ucival))) {
            map_fast_onboard = atoi(ucival);
        }
        fprintf(stderr, "Map Fast Onboard:%s \n", ucival);
 
        if (map_fast_onboard) {
            memset(optv, '\0', sizeof(optv));
            snprintf(optv, sizeof(optv), "wireless.ath%d.map_config_changed",vsec->vapidx);
            syscfg_set(NULL,optv, "1");
        }
    }
    syscfg_commit();
    return 0;
}

int get_radio(char *radio)
{
    char cmd[255] = {0};
    char buff[255] = {0};
    FILE *fp = NULL;
    char *ctx = NULL;

    snprintf(cmd, sizeof(cmd), "cat /sys/class/net/%s/supported_bands",radio);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return -1;
    }
    fgets(buff, 255, fp);
    pclose(fp);
    strtok_r(buff, "\n", &ctx);
    if (strstr(buff, "2G") != NULL) {
        return BAND_2G;
    }
    else if (strstr(buff, "5G") != NULL) {
        return BAND_5G;
    }
    else if (strstr(buff, "6G") != NULL) {
        return BAND_6G;
    }

    return -1;
}

void wifi_rdkeasymesh_get_valid_vaps(char *radio, int *vap_list)
{
    int vap_list_2g[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32};
    int vap_list_5g[] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33};
    int i = 0, *ptr = NULL, valid_index = 0, enable = 0;
    char buff[256],buf[64] = {0};
    FILE *fp = NULL;

    int band = get_radio(radio);
    if (band == BAND_2G) //2.4G  Hardcoded ..?
        ptr = vap_list_2g;
    else if (band == BAND_5G) //5G
        ptr = vap_list_5g;

    for (i = 0; i < RDKB_NUM_VAPS_SUPPORTED_PER_RADIO; i++)  //buffer overflow
    {
        memset(buff, '\0', sizeof(buff));
        memset(buf, 0, sizeof(buf));
        snprintf(buff, sizeof(buff), "wifi_api wifi_getApEnable %d ", *(ptr+i));
        fp = popen(buff,"r");
        while(!feof(fp))
        {
            fgets(buf,sizeof(buf),fp);
            if(strstr(buf,"output : ") != NULL) {
                sscanf(buf,"output : %d",&(enable));
                if (enable == 1) {
                    vap_list[valid_index] = *(ptr+i);
                    valid_index++;
                    break;
                }
            }
        }

        pclose(fp);
    }
    for (i = valid_index; i < RDKB_MAX_SUPPORTED_VAPS; i++)
    {
        vap_list[i] = -1;
    }

    return;
}

void wireless_flush_stale_vap_config()
{
    char buff[256], buf[64] = {0}, bsstype[128] = {0}, optv[128] = {0};
    FILE *fp = NULL;
    int  enable = 0, interface = 0;

    memset(buff, '\0', sizeof(buff));
    memset(buf, 0, sizeof(buf));
    memset(bsstype, 0, sizeof(bsstype));

    snprintf(buff, sizeof(buff), "syscfg show | grep -i MapBssType");

    fp = popen(buff,"r");
    while(!feof(fp))
    {
        fgets(buf,sizeof(buf),fp);
        sscanf(buf,"wireless.ath%d.MapBSSType=%d", &interface, &(enable));
        snprintf(optv, sizeof(optv), "syscfg set wireless.ath%d.MapBSSType 16", interface);
        system(optv);
    }
    fclose(fp);

    system("syscfg commit");

    return;
}

static int wireless_fill_radio_vap_config(struct wireless_s *wl, char *name, char *value)
{
    char newval[UCI_MAX_STR_LEN];
    int vapidx_tmp, *ptr = NULL;
    struct wl_translate_option *iopt;
    char *pos = NULL, *radioname = NULL;

    vapidx_tmp = strtoul(name, &pos, 0);

    if (!pos || *pos != '.')
    {
        return -1;
    }

    pos ++;

    iopt = vapTbl;
    while(iopt->optname  != NULL)
    {
        if (strcmp(pos, iopt->optname) == 0)
            break;
        iopt ++;
    }

    if (!iopt->optname)
    {
        fprintf(stderr, "unhandled option %s\n", name);
        return -1;
    }

    if (iopt->optparser &&
            iopt->optparser(wl, vapidx_tmp, pos, value) != 0) {
        return -1;
    }
    if (!iopt->uciname)
    {
        return 0;
    }

    if (iopt->opt2uci)
    {
        if (iopt->opt2uci(value, newval) != 0) {
            return -1;
        }
        radioname = newval;
    }

    if (vapidx_tmp == 1) {
        //wireless_flush_stale_vap_config(); /* To do re-visit*/
        memset(&g_wireless_vap_config, 0, sizeof(wireless_dynamic_vap_config));
        g_wireless_vap_config.radio[0].numVAPs = 0;
        g_wireless_vap_config.radio[1].numVAPs = 0;
    }

    int band = get_radio(radioname);
    if (band == BAND_2G) //2.4G
    {
        if(g_wireless_vap_config.radio[0].numVAPs == 0) {
            ptr = g_2g_rdk_vap_list;
            wifi_rdkeasymesh_get_valid_vaps(radioname, ptr);
        }

        g_wireless_vap_config.radio[0].radioIndex = atoi(value);
        g_wireless_vap_config.radio[0].numVAPs++;
        g_wireless_vap_config.radio[0].mibIndx[vapidx_tmp-1] = vapidx_tmp;

        strlcpy(g_wireless_vap_config.radio[0].radioName, radioname, sizeof(g_wireless_vap_config.radio[0].radioName));
    } else if (band == BAND_5G) { //5G
        if(g_wireless_vap_config.radio[1].numVAPs == 0) {
            ptr = g_5g_rdk_vap_list;
            wifi_rdkeasymesh_get_valid_vaps(radioname, ptr);
        }

        g_wireless_vap_config.radio[1].radioIndex = atoi(value);
        g_wireless_vap_config.radio[1].mibIndx[g_wireless_vap_config.radio[1].numVAPs] = vapidx_tmp;
        g_wireless_vap_config.radio[1].numVAPs++;
        strlcpy(g_wireless_vap_config.radio[1].radioName, radioname, sizeof(g_wireless_vap_config.radio[1].radioName));
    }

    return 0;
}

static int wireless_set_vap(struct wireless_s *wl, char *name, char *value)
{
	char optv[UCI_MAX_STR_LEN];
	char ucival[UCI_MAX_STR_LEN];
	char newval[UCI_MAX_STR_LEN];
	int vapidx_tmp;
	int vapidx = -1;
	struct wl_translate_option *iopt;
	char *pos = NULL;
	int ret = 0, managedvap_diff = 0;
	struct vapsec_s *vsec;
	int map_fast_onboard = 0;
	int OldVapidxValue;

	vapidx_tmp = strtoul(name, &pos, 0);

	//Identify the valid vaps and map dynamically to the RDK system
	// - Check for the VAPS that are UP [there is a chance of VAP disabled from the WiFiAgent Data model]
	// - Airties Controller will send the FH, BH info for the Valid Vaps only.
	// - Order of RDK valid VAPs and Airties Controller's M2 message order of VAPs is same, for now.
	// - If suppose there is a change in order of FH, BH and RDK VAPs index, onboarding will be done in the order of how FH, BH is sent
	// move this to librdkeasymesh.so //componentization

	if (vapidx_tmp == g_wireless_vap_config.radio[0].mibIndx[vapidx_tmp - 1]) {

		ptr=g_2g_rdk_vap_list;
		vapidx = ptr[vapidx_tmp - 1];
	} else {
		OldVapidxValue=vapidx_tmp;  //compare vapidxtmp and mibIndex to set valid vapidx
		managedvap_diff = g_wireless_vap_config.radio[1].mibIndx[0] - g_wireless_vap_config.radio[0].numVAPs;
		vapidx_tmp = vapidx_tmp - g_wireless_vap_config.radio[0].numVAPs;
		if(OldVapidxValue == g_wireless_vap_config.radio[1].mibIndx[vapidx_tmp-managedvap_diff]) //check valid vap condtion
		{
			ptr=g_5g_rdk_vap_list;
			vapidx = ptr[vapidx_tmp - managedvap_diff];

		}
	}
	if(vapidx != -1 )
	{
		if (!pos || *pos != '.')
		{
			return -1;
		}

		pos ++;

		iopt = vapTbl;
		while(iopt->optname  != NULL)
		{
			if (strcmp(pos, iopt->optname) == 0)
				break;
			iopt ++;
		}

		if (!iopt->optname)
		{
			fprintf(stderr, "unhandled option %s\n", name);
			return -1;
		}

		if (iopt->optparser &&
				iopt->optparser(wl, vapidx, pos, value) != 0) {
			return -1;
		}
		if (!iopt->uciname)
		{
			return 0;
		}

		if (iopt->opt2uci)
		{
			if (iopt->opt2uci(value, newval) != 0) {
				return -1;
			}
			value = newval;
		}
		char mapbssval[] = "64";
		char buf[256] = "";
		char syscfgval[256] = "";
		int val = 0;
		char *temp = NULL;
		if (name && (strstr(name, "MapBSSType") != NULL)) {
			snprintf(buf, sizeof(buf), "ezmesh.MultiAP.MapVersion");
			ret = syscfg_get(NULL, buf, syscfgval, sizeof(syscfgval));
			if (strstr(syscfgval,"2") != NULL) {
				memset(buf, 0, sizeof(buf));
				memset(syscfgval, 0, sizeof(syscfgval));
				snprintf(buf, sizeof(buf), "ezmesh.MultiAP.Map2TrafficSepEnabled");
				ret = syscfg_get(NULL, buf, syscfgval, sizeof(syscfgval));
				if (strstr(syscfgval, "0") != NULL) {
					val = atoi(value);
					if (val > 64) {
						temp = value;
						value = mapbssval;
					}
				}
			}
		}
		memset(optv, '\0', sizeof(optv));
		snprintf(optv, sizeof(optv), "wireless.ath%d.%s", vapidx, iopt->uciname);
		ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

		if ( ret || strcmp(value, ucival))
		{
			if(ret && !strlen(value)) {  /* Handle NULL string to prevent unwanted wifi restart */
				if (temp != NULL){
					value = temp;
				}
				return 0;
			}
			ret = syscfg_set(NULL, optv, value);
			if (temp != NULL){
				value = temp;
			}
			vsec = wireless_get_vapsec(wl, vapidx);
			if (vsec == NULL) { /* no memory */
				return -1;
			}
			vsec->changed = true;
			set_vsec_config_changed(vsec);
			memset(optv, '\0', sizeof(optv));
			snprintf(optv, sizeof(optv), "wsplcd.config.MapFastOnboarding");
			if (!syscfg_get(NULL, optv, value, sizeof(value))) {
				map_fast_onboard = atoi(value);
			}
			if (map_fast_onboard) {
				memset(optv, '\0', sizeof(optv));
				snprintf(optv, sizeof(optv), "wireless.ath%d.map_config_changed", vapidx);
				syscfg_set(NULL, optv, "1");
			}
		}
		if (temp != NULL){
			value = temp;
		}

		syscfg_commit();
	}
	return ret;
}

static int wireless_apply_optimized(struct vapsec_s *vsec, int band)
{
    int vapind;
    int channel;

    if (!vsec->channel
        && !vsec->vapind
        && !vsec->htmode)
        return 0;

    if (strlen(vsec->ifname) <= 0
        && wireless_get_ifname_by_vapidx(vsec->vapidx, vsec->ifname) < 0)
    {
        return 0;
    }

    /*set channel*/
    if (vsec->channel)
    {
        channel = atoi(vsec->channel);

        if (wireless_set_vapchannel(vsec->ifname, channel, band)!= 0)
            perror("set channel failed\n");

    }
    /*set athnewind
      down-up is only needed when athnewind changed from 0->1, so that the AP vap
      could be changed to running state.
      If it is set to a STA, we need to wake up the ap in same radio.
    */
    if (vsec->vapind)
    {
        vapind = atoi(vsec->vapind);
        if (!vsec->isdown
            && atoi(vsec->vapind) == 1)
        {
            wireless_vap_ifctl(vsec->ifname, 0);
            vsec->isdown = true;
        }

        if (wireless_set_vapind(vsec->ifname, vapind) != 0)
            perror("set athnewind failed\n");
    }

    if (vsec->htmode)
    {
        if (wireless_set_vaphtmode(vsec->ifname, vsec->htmode) != 0)
            perror("set mode failed\n");
    }

    return 0;
}

void remove_list_duplicates(char *string, char *remove, char *final_value)
{
    char buf [UCI_MAX_STR_LEN] = {0};
    int len = 0;
    char *context;
    char *token = strtok_r(string, " ", &context);
    while(token) {
          if (strcmp(token, remove) != 0) {
              strlcat(buf, token, sizeof(buf));
              strlcat(buf, " ", sizeof(buf));
           }
           token = strtok_r(NULL, " ", &context);
    }
    len = strlen(buf);
    buf[len] = '\0';
    strlcpy(final_value, buf, len);
}

static int wireless_apply_list_parameter(struct vapsec_s *vsec, struct list_data *list, char* parameter)
{
    int ret = 0;
    char newval[UCI_MAX_STR_LEN];
    char optv[UCI_MAX_STR_LEN];
    char get_list_val[UCI_MAX_STR_LEN];
    char set_list_val[UCI_MAX_STR_LEN];

    while (list) {
        memset(newval, 0, sizeof(newval));
        memset(optv, '\0', sizeof(optv));
        memset(get_list_val, '\0', sizeof(get_list_val));
        memset(set_list_val, '\0', sizeof(set_list_val));

        if ( list->data && strlen(list->data))
        {
            snprintf(newval, sizeof(newval), "%s", list->data);
            snprintf(optv, sizeof(optv), "wireless.ath%d.%s", vsec->vapidx, parameter);

            if (vsec->vap_type == VAPTYPE_AP)
            {
                if (list->action == LISTACTION_ADD) {
                    syscfg_get(NULL, optv, get_list_val, sizeof(get_list_val));
                    snprintf(set_list_val, sizeof(set_list_val), "%s %s", get_list_val, newval);
                    ret = syscfg_set(NULL, optv, set_list_val);
                    vsec->changed = true;
                }
                else if (list->action == LISTACTION_DEL) {
                    syscfg_get(NULL, optv, get_list_val, sizeof(get_list_val));
                    remove_list_duplicates(get_list_val, newval, set_list_val);
                    ret = syscfg_set(NULL, optv, set_list_val);
                    vsec->changed = true;
                }
            }
            else if (vsec->vap_type == VAPTYPE_STA)
            {
                if (list->action == LISTACTION_ADD) {
                    ret = syscfg_set(NULL, optv, newval);
                    vsec->changed = true;
                }
                else if (list->action == LISTACTION_DEL) {
                    ret = syscfg_unset(NULL, optv); 
                    vsec->changed = true;
                }
            }
        }
        list = list->next;
    }
    syscfg_commit();
    return ret;
}

static int wireless_apply_vapsec(struct package_s *pkg)
{
    struct wireless_s *wl = (struct wireless_s *)pkg->priv;
    struct vapsec_s *vsec = wl->vsec;
    int ret = 0;
    char optv[UCI_MAX_STR_LEN] = {0};
    char ucival[UCI_MAX_STR_LEN];
    char newval[UCI_MAX_STR_LEN];

    memset(newval, 0, sizeof(newval));
    while (vsec)
    {
        /* wifi security */
        if (vsec->bcn_type == 0) {
            goto next;
        }
        memset(newval, 0, sizeof(newval));
        switch (vsec->wpa3_en_6g_sec_comp)
        {
            case WPA3SAEVALUE_eEnable:
                snprintf(newval, UCI_MAX_STR_LEN, "1");
                break;
            case WPA3SAEVALUE_eDisable:
                snprintf(newval, UCI_MAX_STR_LEN, "0");
                break;
        }

        memset(optv, '\0', sizeof(optv));
        snprintf(optv, sizeof(optv), "wireless.ath%d.en_6g_sec_comp", vsec->vapidx);
        ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

        if (ret ||
            strcmp(ucival, newval))
        {
            if( vsec->wpa3_en_6g_sec_comp == WPA3SAEVALUE_eEnable || vsec->wpa3_en_6g_sec_comp == WPA3SAEVALUE_eDisable )
            {
                ret = syscfg_set(NULL, optv, newval);
                vsec->changed = true;
            }
            else if(vsec->wpa3_en_6g_sec_comp == WPA3SAEVALUE_eNotSet && !ret)
            {
                ret = syscfg_unset(NULL, optv);
                vsec->changed = true;
            }
            else
            {
                ret = 0;
            }
        }
        if (ret) {
            break;
        }
        memset(newval, 0, sizeof(newval));
        /* option: encryption */
        switch (vsec->bcn_type)
        {
            case BEACONTYPE_eNone:
                snprintf(newval, UCI_MAX_STR_LEN, "none");
                break;
            case BEACONTYPE_eBasic:
                if (vsec->wep_auth == BASICAUTHENTICATIONMODE_eNone)
                    snprintf(newval, UCI_MAX_STR_LEN, "wep");
                else if (vsec->wep_auth == BASICAUTHENTICATIONMODE_eSharedAuthentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "wep+shared");
                else if (vsec->wep_auth == BASICAUTHENTICATIONMODE_eEAPAuthentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "8021x");
                else if (vsec->wep_auth == BASICAUTHENTICATIONMODE_eBoth)
                    snprintf(newval, UCI_MAX_STR_LEN, "wep+mixed");
                else
                    snprintf(newval, UCI_MAX_STR_LEN, "wep");
                break;
            case BEACONTYPE_eWPA:
                if (vsec->wpa2_auth == WPAAUTHENTICATIONMODE_eEAPAuthentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "wpa");
                else if (vsec->wpa2_auth == WPAAUTHENTICATIONMODE_ePSKAuthentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "psk");
                else if (vsec->wpa_auth == WPAAUTHENTICATIONMODE_eEAPAuthentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "wpa");
                else if (vsec->wpa_auth == WPAAUTHENTICATIONMODE_ePSKAuthentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "psk");
                else
                    snprintf(newval, UCI_MAX_STR_LEN, "psk");

                if (vsec->wpa2_encr == WPAENCRYPTIONMODES_eTKIPEncryption)
                    snprintf(newval + 3, UCI_MAX_STR_LEN - 3, "+tkip");
                else if (vsec->wpa2_encr == WPAENCRYPTIONMODES_eAESEncryption)
                    snprintf(newval + 3, UCI_MAX_STR_LEN - 3, "+ccmp");
                else if (vsec->wpa2_encr == WPAENCRYPTIONMODES_eTKIPandAESEncryption)
                    snprintf(newval + 3, UCI_MAX_STR_LEN - 3, "+tkip+ccmp");
                else if (vsec->wpa_encr == WPAENCRYPTIONMODES_eTKIPEncryption)
                    snprintf(newval + 3, UCI_MAX_STR_LEN - 3, "+tkip");
                else if (vsec->wpa_encr == WPAENCRYPTIONMODES_eAESEncryption)
                    snprintf(newval + 3, UCI_MAX_STR_LEN - 3, "+ccmp");
                else if (vsec->wpa_encr == WPAENCRYPTIONMODES_eTKIPandAESEncryption)
                    snprintf(newval + 3, UCI_MAX_STR_LEN - 3, "+tkip+ccmp");
                else
                    snprintf(newval + 3, UCI_MAX_STR_LEN - 3, "+ccmp");
                break;

            case BEACONTYPE_e11i:
                if (vsec->wpa2_auth == IEEE11IAUTHENTICATIONMODE_eEAPAuthentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "wpa2");
                else if (vsec->wpa2_auth == IEEE11IAUTHENTICATIONMODE_ePSKAuthentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "psk2");
                else if (vsec->wpa2_auth == IEEE11IAUTHENTICATIONMODE_eEAPandPSKAuthentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "psk2"); /*not defined in uci*/
                else if(vsec->wpa3_auth == IEEE11IAUTHENTICATIONMODE_eEAPandWPA3Authentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "wpa3"); /*not defined in uci*/
                else if(vsec->wpa3_auth != IEEE11IAUTHENTICATIONMODE_eWPA3Authentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "psk2");

                if (vsec->wpa2_encr == IEEE11IENCRYPTIONMODES_eTKIPEncryption)
                    snprintf(newval + strlen(newval), UCI_MAX_STR_LEN - strlen(newval), "+tkip");
                else if (vsec->wpa2_encr == IEEE11IENCRYPTIONMODES_eAESEncryption && vsec->wpa3_auth != IEEE11IAUTHENTICATIONMODE_eWPA3Authentication)
                    snprintf(newval + strlen(newval), UCI_MAX_STR_LEN - strlen(newval), "+ccmp");
                else if (vsec->wpa2_encr == IEEE11IENCRYPTIONMODES_eAESEncryption)
                    snprintf(newval + strlen(newval), UCI_MAX_STR_LEN - strlen(newval), "ccmp");

                if (vsec->wpa2_encr == IEEE11IENCRYPTIONMODES_eTKIPandAESEncryption)
                    snprintf(newval + strlen(newval), UCI_MAX_STR_LEN - strlen(newval), "+tkip+ccmp");

                break;
            case BEACONTYPE_eWPAand11i:
                if (vsec->wpa2_auth == IEEE11IAUTHENTICATIONMODE_eEAPAuthentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "wpa-mixed");
                else if (vsec->wpa2_auth == IEEE11IAUTHENTICATIONMODE_ePSKAuthentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "psk-mixed");
                else if (vsec->wpa2_auth == IEEE11IAUTHENTICATIONMODE_eEAPandPSKAuthentication)
                    snprintf(newval, UCI_MAX_STR_LEN, "psk-mixed");/*not defined in uci*/
                else
                    snprintf(newval, sizeof(newval),  "psk-mixed");

                if (vsec->wpa2_encr == IEEE11IENCRYPTIONMODES_eTKIPEncryption)
                    snprintf(newval + strlen(newval), UCI_MAX_STR_LEN - strlen(newval), "+tkip");
                else if (vsec->wpa2_encr == IEEE11IENCRYPTIONMODES_eAESEncryption)
                    snprintf(newval + strlen(newval), UCI_MAX_STR_LEN - strlen(newval), "+ccmp");
                else if (vsec->wpa2_encr == IEEE11IENCRYPTIONMODES_eTKIPandAESEncryption)
                    snprintf(newval + strlen(newval), UCI_MAX_STR_LEN - strlen(newval), "+tkip+ccmp");
                else
                    snprintf(newval + strlen(newval), UCI_MAX_STR_LEN - strlen(newval), "+ccmp");

                break;
        }

        memset(optv, '\0', sizeof(optv));
        snprintf(optv, sizeof(optv), "wireless.ath%d.encryption", vsec->vapidx);
        ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));
        if (ret ||
            strcmp(ucival, newval))
        {
            ret = syscfg_set(NULL, optv, newval);
            vsec->changed=true;
        }

        if (ret) {
           break;
        }
        /* option: key */
        memset(newval, 0, sizeof(newval));
        if (vsec->psk || vsec->passphrase || vsec->wep_keyidx)
        {
            if (vsec->bcn_type == BEACONTYPE_eBasic
                && vsec->wep_keyidx )
            {
                snprintf(newval, sizeof(newval), "%d", vsec->wep_keyidx);
            }
            else if (vsec->psk && strlen(vsec->psk))
            {
                snprintf(newval, sizeof(newval), "%s", vsec->psk);
            }
            else if (vsec->passphrase && strlen(vsec->passphrase))
            {
                snprintf(newval, sizeof(newval), "%s", vsec->passphrase);
            }

        }

        // Remove Mandatory "key" check for WPA3-SAE, WPA3-OWE, WPA3-SUITE_B
        if (!strlen(newval))
        {
            if(vsec->wpa3_sae == 1 && (vsec->wpa3_pass_list && !strlen(vsec->wpa3_pass_list->data))) {
                goto next;
            }
            else if( vsec->wpa3_sae != 1 && vsec->wpa3_owe != 1 && vsec->bcn_type > BEACONTYPE_eNone ) {
                goto next;
            }
            // Clearing the existing key to support Cloning of WPA3 Password less Encryption
            memset(optv, '\0', sizeof(optv));
            snprintf(optv, sizeof(optv), "wireless.ath%d.key", vsec->vapidx);
            ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));
            if (!ret)
            {
                ret = syscfg_unset(NULL, optv);
                vsec->changed = true;
            }
            else
                ret = 0;
            }
        else
        {
            memset(optv, '\0', sizeof(optv));
            snprintf(optv, sizeof(optv), "wireless.ath%d.key", vsec->vapidx);
            ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

            if (ret ||
                strcmp(ucival, newval))
            {
                ret = syscfg_set(NULL, optv, newval);    
                vsec->changed = true;
            }
        }

        if (ret) {
           break;
        }
        if (vsec->wpa3_pass_list) {
            if ((ret = wireless_apply_list_parameter(vsec, vsec->wpa3_pass_list, "sae_password")) != 0 ) {
                break;
            }
        }

        if (vsec->wpa3_sae_groups) {
            if ((ret = wireless_apply_list_parameter(vsec, vsec->wpa3_sae_groups, "sae_groups")) != 0 ) {
                break;
            }
        }

        if (vsec->wpa3_owe_groups) {
            if ((ret = wireless_apply_list_parameter(vsec, vsec->wpa3_owe_groups, "owe_groups")) != 0 ) {
                break;
            }
        }

        memset(newval, 0, sizeof(newval));
        switch (vsec->wpa3_sae)
        {
            case WPA3SAEVALUE_eEnable:
                snprintf(newval, UCI_MAX_STR_LEN, "1");
                break;
            case WPA3SAEVALUE_eDisable:
                snprintf(newval, UCI_MAX_STR_LEN, "0");
                break;
        }

        memset(optv, '\0', sizeof(optv));
        snprintf(optv, sizeof(optv), "wireless.ath%d.sae", vsec->vapidx);
        ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

        if (ret ||
            strcmp(ucival, newval))
        {
            if( vsec->wpa3_sae == WPA3SAEVALUE_eEnable || vsec->wpa3_sae == WPA3SAEVALUE_eDisable )
            {
                ret = syscfg_set(NULL, optv, newval);
                vsec->changed = true;
            }
            else if(vsec->wpa3_sae == WPA3SAEVALUE_eNotSet && !ret)
            {
                ret = syscfg_unset(NULL, optv);
                vsec->changed = true;
            }
            else
            {
                ret = 0;
            }
        }
        if (ret) {
            break;
        }
        memset(newval, 0, sizeof(newval));
        switch (vsec->wpa3_sae_pwe)
        {
            case WPA3SAEPWEVALUE_eHashToElement:
                snprintf(newval, UCI_MAX_STR_LEN, "1");
                break;
            case WPA3SAEPWEVALUE_eHuntingPecking:
                snprintf(newval, UCI_MAX_STR_LEN, "0");
                break;
            case WPA3SAEPWEVALUE_eBothHuntPeckAndHashElem:
                snprintf(newval, UCI_MAX_STR_LEN, "2");
                break;
        }


        memset(optv, '\0', sizeof(optv));
        snprintf(optv, sizeof(optv), "wireless.ath%d.sae_pwe", vsec->vapidx);
        ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

        if (ret ||
            strcmp(ucival, newval))
        {
            if( vsec->wpa3_sae_pwe == WPA3SAEPWEVALUE_eHashToElement ||
                vsec->wpa3_sae_pwe == WPA3SAEPWEVALUE_eHuntingPecking ||
                vsec->wpa3_sae_pwe == WPA3SAEPWEVALUE_eBothHuntPeckAndHashElem )
            {
                ret = syscfg_set(NULL, optv, newval);
                vsec->changed = true;
            }
            else if(vsec->wpa3_sae_pwe == WPA3SAEPWEVALUE_eNotSet && !ret)
            {
                ret = syscfg_unset(NULL, optv);
                vsec->changed = true;
            }
            else
            {
                ret = 0;
            }
        }
        if (ret) {
            break;
        }
        memset(newval, 0, sizeof(newval));
        switch (vsec->wpa3_owe)
        {
            case WPA3OWEVALUE_eEnable:
                snprintf(newval, UCI_MAX_STR_LEN, "1");
                break;
            case WPA3OWEVALUE_eDisable:
                snprintf(newval, UCI_MAX_STR_LEN, "0");
                break;
        }

        memset(optv, '\0', sizeof(optv));
        snprintf(optv, sizeof(optv), "wireless.ath%d.owe", vsec->vapidx);
        ret = syscfg_get(NULL, optv, ucival, sizeof(ucival));

        if (ret ||
            strcmp(ucival, newval))
        {
            if( vsec->wpa3_owe == WPA3OWEVALUE_eEnable || vsec->wpa3_owe == WPA3OWEVALUE_eDisable )
            {
                ret = syscfg_set(NULL, optv, newval);
                vsec->changed = true;
            }
            else if(vsec->wpa3_owe == WPA3OWEVALUE_eNotSet && !ret)
            {
                ret = syscfg_unset(NULL, optv);
                vsec->changed = true;
            }
            else
            {
                ret = 0;
            }
        }

        if (ret) {
            break;
        }
        memset(newval, 0, sizeof(newval));
        if (ret) {
            break;
        }
        set_vsec_config_changed(vsec);
next:
        vsec = vsec->next;

    }/*for each vap*/

    syscfg_commit();
    return ret;
}

static int wireless_init(struct package_s *pkg)
{
    struct wireless_s *wl = (struct wireless_s *)pkg->priv;
    memset(wl, 0, sizeof(struct wireless_s));
    return 0;
}

static int wireless_ifaction(struct package_s *pkg,
                             const char *name, const char *value)
{
    struct wireless_s *wl = (struct wireless_s *)pkg->priv;
    char optv[UCI_MAX_STR_LEN] = {0};
    int ret = 0;
    int idx = 0, fltlen = 0;
    char *endptr = NULL;

    if (strstr(name, "AddMld"))
        fltlen = strlen(pkg->flt3);
    else
        fltlen = strlen(pkg->flt1);

    name += fltlen;

    idx = strtoul(name, &endptr, 0);

    if (!endptr || *endptr != '.' || idx <= 0)  {
        return -1;
    }

    endptr++; // idx = radioidx for add

    if (!strncasecmp(endptr, "AddMld", 6)) {
        snprintf(optv, sizeof(optv), "%s.mld%d",pkg->name, idx-1);
        syscfg_set(NULL,optv,"1");
    }
    else if (!strncasecmp(endptr, "Add", 3)) {
        snprintf(optv, sizeof(optv), "%s.wifi%d",pkg->name, idx-1);
    } 
    else {
        return -1;
    }
    
    wl->changed = 1; //make sure radio goes for restart.
    syscfg_commit();

    return ret;
}

static int wireless_prepare(struct package_s *pkg, char *name, char *value)
{

    struct wireless_s *wl = (struct wireless_s *)pkg->priv;
    int ret = 0;
    int fltlen;
    if (fltlen = strlen(pkg->flt2),
            memcmp(name, pkg->flt2, fltlen) == 0)
    {
        if(strstr(name ,"X_ATH-COM_RadioIndex") != NULL) {
            ret = wireless_fill_radio_vap_config(wl, name + fltlen, value);
        }
    }

    return ret;
}

static int wireless_set(struct package_s *pkg, char *name, char *value)
{
    struct wireless_s *wl = (struct wireless_s *)pkg->priv;
    int ret = 0;
    int fltlen;

    if (fltlen = strlen(pkg->flt1),
        memcmp(name, pkg->flt1, fltlen) == 0)
    {
        ret = wireless_set_radio(wl, name + fltlen, value);
    }
    else if (fltlen = strlen(pkg->flt2),
        memcmp(name, pkg->flt2, fltlen) == 0)
    {
        ret = wireless_set_vap(wl, name + fltlen, value);
    }
    else if (fltlen = strlen(pkg->flt3),
        memcmp(name, pkg->flt3, fltlen) == 0)
    {
        ret = wireless_set_mld(wl, name + fltlen, value);
    }
    else
    {
        ret = -1;
    }

    return ret;
}

static bool wireless_reset_vap_flags(struct package_s *pkg,
                                     char *radio,const char *delim)
{
    bool ret = true;
    struct wireless_s *wl = (struct wireless_s *)pkg->priv;
    struct vapsec_s *vsec = wl->vsec;
    char vapphy[UCI_MAX_STR_LEN] = {0};
    char optv[UCI_MAX_STR_LEN] = {0};
    char *token = NULL;
    char *nextToken;

    token = strtok_r(radio, delim, &nextToken);
    while(token != NULL)
    {
        vsec = wl->vsec;
        while(vsec)
        {
            memset(optv, '\0', sizeof(optv));
            snprintf(optv, sizeof(optv), "wireless.ath%d.device", vsec->vapidx);
            ret = syscfg_get(NULL, optv, vapphy, sizeof(vapphy));
            if(!strcmp(token,vapphy))
            {
                vsec->isdown = false;
            }
            vsec = vsec->next;
        }
        token = strtok_r(NULL, delim, &nextToken);
    }
    return ret;
}

static int opt_set_anqp_elem(struct wireless_s *wl)
{
    char optv[UCI_MAX_STR_LEN] = {0};
    char buf[UCI_MAX_STR_LEN * 2];
    char mapVersion[UCI_MAX_STR_LEN];
    const char anqp_val[UCI_MAX_STR_LEN] = "272:34108cfdf0020df1f7000000733000030101";
    int ret = 0;

    struct vapsec_s *vsec;
    vsec = wl->vsec;
    while (vsec) {
        memset(optv, '\0', sizeof(optv));
        snprintf(optv, sizeof(optv), "wireless.ath%d.map", vsec->vapidx);
        ret = syscfg_get(NULL, optv, mapVersion, sizeof(mapVersion));

        if (atoi(mapVersion) >= 2) {
            /* Set internetworking to 1 */
            memset(optv, '\0', sizeof(optv));
            snprintf(optv, sizeof(optv), "wireless.ath%d.interworking", vsec->vapidx);
            ret = syscfg_set(NULL, optv, "1");
            if (ret == -1) {
                return ret;
            }

            /* As ANQP is a list. Always delete and add to control list build */
            memset(buf, '\0', sizeof(buf));
            snprintf(buf, sizeof(buf), "wireless.ath%d.anqp_elem", vsec->vapidx);
            ret = syscfg_unset(NULL, buf);
            if (ret == -1) {
                return ret;
            }

            memset(buf, '\0', sizeof(buf));
            snprintf(buf, sizeof(buf), "wireless.ath%d.anqp_elem", vsec->vapidx);
            ret = syscfg_set(NULL, buf, anqp_val);
            if (ret == -1) {
                return ret;
            }
        }

        vsec = vsec->next;
    }

    syscfg_commit();
    return ret;
}

static int clear_config_changed_flag(struct package_s *pkg)
{
    struct wireless_s *wl = (struct wireless_s *)pkg->priv;
    struct vapsec_s *vsec = wl->vsec;
    char optv[UCI_MAX_STR_LEN];
    char value[UCI_MAX_STR_LEN] = {0};

    vsec = wl->vsec;
    while (vsec)
    {
    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wireless.ath%d.config_changed", vsec->vapidx);
    if(!syscfg_get(NULL, optv, value, sizeof(value))){
            syscfg_set(NULL, optv, "0");
        // In UCI we are deleting the entire section since that
        // vap will be re-used but for RDK-B this needs to rechecked.
        //syscfg_unset(NULL, optv);
        }
        vsec = vsec->next;
    }

    syscfg_commit();
    return 0;
}

static int wireless_apply(struct package_s *pkg)
{
    struct wireless_s *wl = (struct wireless_s *)pkg->priv;
    struct vapsec_s *vsec = wl->vsec;
    char vapphy[MAX_RADIO][UCI_MAX_STR_LEN + 1] = {0};
    char vapbsstype[UCI_MAX_STR_LEN + 1] = {0};
    char device[UCI_MAX_STR_LEN] = { 0 };
    char optv[UCI_MAX_STR_LEN] = {0};
    char radio_name[UCI_MAX_STR_LEN + 1] = {0};
    char value[UCI_MAX_STR_LEN] = {0};
    char bandc[UCI_MAX_STR_LEN + 1] = {0};
    char *p = NULL;
    char bandRadio[UCI_MAX_STR_LEN + 1] = {0};
    char buf[UCI_MAX_STR_LEN * 2] = {0};
    int ret = 0;
    bool need_vap_restart = false;
    bool need_restart = false;
    const char *delim = " ";
    int band = BAND_UNKNOWN, rad_index, i, vap_specific_restart = 0, map_fast_onboard = 0, index_6G = -1;
    char vap_needs_restart=0;

    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wsplcd.config.VapSpecificRestart");
    if(!syscfg_get(NULL, optv, value, sizeof(value))) {
       vap_specific_restart = atoi(value);
    }   

    memset(optv, '\0', sizeof(optv));
    snprintf(optv, sizeof(optv), "wsplcd.config.MapFastOnboarding");
    if(!syscfg_get(NULL, optv, value, sizeof(value))) {
       map_fast_onboard = atoi(value);
       dprintf("Map Fast Onboard:%s \n", value);
    }   

    if (wireless_apply_vapsec(pkg)) {
        return -1;
    }
    /* Loop through all the VAPs and delete VAPs set to teardown */
    if (!map_fast_onboard) {
        while (vsec) {
            memset(optv, '\0', sizeof(optv));
            snprintf(optv, sizeof(optv), "wireless.ath%d.MapBSSType", vsec->vapidx);
            ret = syscfg_get(NULL, optv, vapbsstype, sizeof(vapbsstype));
            /* For Tear Down MapBSSType will be 0x10 */
            if (atoi(vapbsstype) & 0x10) {
                memset(optv, '\0', sizeof(optv));
                snprintf(device, sizeof(device), "ath%d", vsec->vapidx);
                snprintf(optv, sizeof(optv), "%s.%s", pkg->name, device);
                if (!syscfg_unset(NULL, optv)) {
                    wl->changed = true;
                }
            }

            /* For Change in backhaul VAP radio restart is required for MAP
               to update BH credentials to FH vap's hostapd-athX.conf file.
               For BH Vap MapBSSType will be 0x40*/
            if (atoi(vapbsstype) & 0x40) {
                if (vsec->changed) wl->changed = true;
            }
            vsec = vsec->next;
        }
    }

    /* Set ANQP and Interworking for MapR2 */
    opt_set_anqp_elem(wl);

    syscfg_commit();

    if (wl->changed)
    { /* phy related parameter changed , restarting all radios */
        storage_wifiLock();
        dprintf(" Do wifi restart, wl config changed\n");
        if (vap_specific_restart) {
            clear_config_changed_flag(pkg);
        }
        if (map_fast_onboard) {
        syscfg_set(NULL, "repacd.MAPConfig.restartWifi", "1");
        syscfg_commit();
        } else {
            system("/sbin/rdk_helper.sh");
        }

        storage_wifiUnlock();
    system("/sbin/rdk_helper.sh");
        return ret;
    }

    if (map_fast_onboard) {
        vsec = wl->vsec;
        while (vsec) {
            if (vsec->changed) {
                need_restart = true;
            }

            vsec = vsec->next;
        }

        dprintf( " Fast onboard need restart %d\n", need_restart);
        if (need_restart) {
            syscfg_set(NULL, "repacd.MAPConfig.restartWifi", "1");
            syscfg_commit();
        system("/sbin/rdk_helper.sh");
        }
        return ret;
    }

    vsec = wl->vsec;
    if (vap_specific_restart) { /* Enable VAP specific restart and avoid radio restart */
        while (vsec)
        {
            memset(optv, '\0', sizeof(optv));
            snprintf(optv, sizeof(optv), "wireless.ath%d.config_changed",vsec->vapidx);
            vap_needs_restart=0;
            if(!syscfg_get(NULL, optv, value, sizeof(value))) {
                vap_needs_restart=atoi(value);
                dprintf("vap_needs_restart:%d vapidx:%d\n",vap_needs_restart, vsec->vapidx);
        }    

            if (vsec->changed || vap_needs_restart)
            {
                if (strlen(vsec->ifname) <= 0
                        && wireless_get_ifname_by_vapidx(vsec->vapidx, vsec->ifname) < 0) {
                    dprintf("failed to get ifname\n");
                    storage_wifiLock();
                    clear_config_changed_flag(pkg);
                    storage_wifiUnlock();
            system("/sbin/rdk_helper.sh");
                    return -1;
                }

                memset(optv, '\0', sizeof(optv));
                snprintf(optv, sizeof(optv), "wireless.ath%d.device", vsec->vapidx);
                ret = syscfg_get(NULL, optv, radio_name, sizeof(radio_name));
                rad_index = atoi(&radio_name[4]); /* Get radio index from radio name "wifiX", where X is index */

                memset(optv, '\0', sizeof(optv));
                memset(bandc, 0, sizeof(bandc));

                snprintf(optv, sizeof(optv), "wireless.%s.band", radio_name);

                ret = syscfg_get(NULL, optv, bandc, sizeof(bandc));
                band = atoi(bandc);
                if (band == BAND_6G) {
                    index_6G = atoi(&radio_name[4]);
                }

                if (rad_index < 0 || rad_index >= MAX_RADIO ) {
                    fprintf(stderr,"invalid radio index:%d \n", rad_index);
                    return ret;
                }

                if (strlen(vapphy[rad_index]) + strlen(vsec->ifname) + strlen(delim) + 1 < sizeof(vapphy[rad_index]))
                {
                    strlcat(vapphy[rad_index], vsec->ifname, UCI_MAX_STR_LEN - 1);
                    strlcat(vapphy[rad_index], delim, UCI_MAX_STR_LEN - strlen(vapphy[rad_index]));
                    memset(radio_name,0,sizeof(radio_name));
                }
                else
        {
            fprintf(stderr," Too many interfaces \n");
            fprintf(stderr," restarting All radios \n");
            storage_wifiLock();
            clear_config_changed_flag(pkg);
            storage_wifiUnlock();
            syscfg_commit();

            system("/sbin/rdk_helper.sh");
            return ret;
        }
            }
            else if(vsec->opti_changed)
            {
                memset(optv, '\0', sizeof(optv));
                snprintf(optv, sizeof(optv), "wireless.ath%d.device", vsec->vapidx);
                ret = syscfg_get(NULL, optv, bandRadio, sizeof(bandRadio));

                memset(optv, '\0', sizeof(optv));
                snprintf(optv, sizeof(optv), "wireless.%s.band", bandRadio);
                ret = syscfg_get(NULL, optv, bandc, sizeof(bandc));

                band = atoi(bandc);
                if ( (band > BAND_6G) || (band < BAND_UNKNOWN) )
                    band=BAND_UNKNOWN;
                need_vap_restart = true;
                wireless_apply_optimized(vsec, band);
            }

            vsec = vsec->next;
        } /* while loop for vsec*/

        memset(radio_name,0,sizeof(radio_name));
        memset(buf,0,sizeof(buf));
        memset(value,0,sizeof(value));

        for (i=0; i < MAX_RADIO; i++) {
            if (strlen(vapphy[i]))
            {
                if (!strlen(buf))
                    system("/sbin/rdk_helper.sh");

                if (i != index_6G) {
                    snprintf(value, sizeof(value), "wifi%d %s ", i, vapphy[i]);
                    strlcat(buf, value, sizeof(buf) - strlen(buf));
                    memset(value,0,sizeof(value));
                }
                else {
                    snprintf(value, sizeof(value), "wifi%d ", i);
                    strlcat(buf, value, sizeof(buf) - strlen(buf));
                    memset(value,0,sizeof(value));
                }

                snprintf(value, sizeof(value), "%s%d ", "wifi",i);
                strlcat(radio_name, value, sizeof(radio_name) - strlen(radio_name));
                memset(value,0,sizeof(value));
            }
        }
    } else { /* Radio restart */
        while (vsec)
        {
            if (vsec->changed)
            {
                memset(optv, '\0', sizeof(optv));
                snprintf(optv, sizeof(optv), "wireless.ath%d.device", vsec->vapidx);
                ret = syscfg_get(NULL, optv, vapphy[0], sizeof(vapphy[0]));

                p = strstr(radio_name,vapphy[0]);
                if( p == NULL )
                { /*unique radio name */
                    if (strlen(radio_name) + strlen(vapphy[0]) + strlen(delim) + 1 < sizeof(radio_name))
                    {
                        strlcat(radio_name, vapphy[0], sizeof(radio_name) - strlen(radio_name));
                        strlcat(radio_name, delim, sizeof(radio_name) - strlen(radio_name));
                        memset(vapphy[0],0,sizeof(vapphy[0]));
                        p = NULL;
                    }
                    else
            {
                dprintf(" Too many interfaces, restarting All radios \n");
                storage_wifiLock();
                storage_wifiUnlock();
                syscfg_commit();
                system("/sbin/rdk_helper.sh");
                return ret;
            }
                }
            }
            else if(vsec->opti_changed)
            {
                memset(optv, '\0', sizeof(optv));
                snprintf(optv, sizeof(optv), "wireless.ath%d.device", vsec->vapidx);
                ret = syscfg_get(NULL, optv, bandRadio, sizeof(bandRadio));

                memset(optv, '\0', sizeof(optv));
                snprintf(optv, sizeof(optv), "wireless.%s.band", bandRadio);
                ret = syscfg_get(NULL, optv, bandc, sizeof(bandc));

                band = atoi(bandc);
                if ( (band > BAND_6G) || (band < BAND_UNKNOWN) )
                    band=BAND_UNKNOWN;
                need_vap_restart = true;
                wireless_apply_optimized(vsec, band);
            }

            vsec = vsec->next;
        } /* while loop for vsec*/

        memset(buf,0,sizeof(buf));
        // This is done for backup case. Most of the time we will not hit the case
        snprintf(buf, sizeof(buf), "/sbin/rdk_helper.sh");

        if(strlen(radio_name))
            snprintf(buf, sizeof(buf), "/sbin/rdk_helper.sh");
    }

    if (strlen(buf))
    {
        dprintf("%s: cmd: %s\n",__func__, buf);
        wireless_reset_vap_flags(pkg,radio_name,delim);
        storage_wifiLock();
        if (vap_specific_restart) {
            clear_config_changed_flag(pkg);
        }
        syscfg_commit();
        storage_wifiUnlock();
        system("/sbin/rdk_helper.sh");
    }

    if (need_vap_restart)
    { /* up vap to apply*/
        vsec = wl->vsec;
        while (vsec)
        {
            if (vsec->isdown)
            {
                wireless_vap_ifctl(vsec->ifname, 1);
                vsec->isdown = false;
            }
            vsec = vsec->next;
        }
    }
    return ret;
}

static int wireless_destroy(struct package_s *pkg)
{
    struct wireless_s *wl = (struct wireless_s *)pkg->priv;
    wireless_free_vapsec(wl);
    return 0;
}

static struct package_s wirelessPkg =
{
    "wireless",        /*name*/
    wireless_init,     /*init*/
    wireless_set,      /*set*/
    NULL,              /*get*/
    wireless_prepare,  /*prepare*/
    wireless_apply,    /*apply*/
    wireless_destroy,  /*destroy*/
    "RADIO.",          /*flt1*/
    "WLAN.",           /*flt2*/
    "MLD.",            /*flt3*/
    NULL,              /*priv*/
    NULL,              /*next*/
    wireless_ifaction  /*ifaction*/
};

int wireless_register()
{
    struct wireless_s *wl = malloc(sizeof(struct wireless_s));

    if (! wl)
        return -1;

    memset(wl, 0, sizeof(struct wireless_s));

    wirelessPkg.priv = (void *)wl;

    pkgRegister(&wirelessPkg);

    return 0;
}
