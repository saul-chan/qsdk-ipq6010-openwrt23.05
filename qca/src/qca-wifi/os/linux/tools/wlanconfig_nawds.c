/*
 * Copyright (c) 2018 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 */

#if UMAC_SUPPORT_NAWDS

#include <wlanconfig.h>

#if defined(WLAN_FEATURE_11BE) && defined(QCA_SUPPORT_WDS_EXTENDED) && \
    !defined(ENABLE_CFG80211_BACKPORTS_MLO)
#define CMD_MAX_LEN	100
static void nawds_add_new_sta(const char *ifname, char *addr)
{
    char cmd[CMD_MAX_LEN] = {'\0'};
    FILE *fRead = NULL;
    uint8_t wds_ext_en = 0;

    if (!addr)
        return;

    snprintf(cmd, CMD_MAX_LEN, "cfg80211tool %s get_wds_ext", ifname);
    fRead = popen(cmd, "r");
    if (fRead == NULL)
        return;

    /* athX	get_wds_ext:0 */
    fscanf(fRead,"%s", cmd);	/* athx 	*/
    fscanf(fRead,"%s", cmd);	/* get_wds_ext:0 */
    sscanf(cmd, "get_wds_ext:%hhu", &wds_ext_en);

    pclose(fRead);
    if (!wds_ext_en)
        return;

    memset(cmd, '\0', sizeof(cmd));
    snprintf(cmd, CMD_MAX_LEN, "hapd %s new_sta %s", ifname, addr);
    system(cmd);
}

static void nawds_del_sta(const char *ifname, char *addr)
{
    char cmd[CMD_MAX_LEN] = {'\0'};
    FILE *fRead = NULL;
    uint8_t wds_ext_en = 0;

    if (!addr)
        return;

    snprintf(cmd, CMD_MAX_LEN, "cfg80211tool %s get_wds_ext", ifname);
    fRead = popen(cmd, "r");
    if (fRead == NULL)
        return;

    /* athX	get_wds_ext:0 */
    fscanf(fRead,"%s", cmd);	/* athx 	*/
    fscanf(fRead,"%s", cmd);	/* get_wds_ext:0 */
    sscanf(cmd, "get_wds_ext:%hhu", &wds_ext_en);

    pclose(fRead);
    if (!wds_ext_en)
        return;

    memset(cmd, '\0', sizeof(cmd));
    snprintf(cmd, CMD_MAX_LEN, "hapd %s deauthenticate %s", ifname, addr);
    system(cmd);
}
#else
static void nawds_add_new_sta(const char *ifname, char *addr)
{
}

static void nawds_del_sta(const char *ifname, char *addr)
{
}
#endif

static int handle_nawds(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
                        IEEE80211_WLANCONFIG_CMDTYPE cmdtype, char *addr,
                        u_int64_t value, char *psk, u_int16_t value2)
{
    int i;
    struct ieee80211_wlanconfig config;
    char macaddr[MACSTR_LEN];
    uint8_t temp[MACSTR_LEN];

    memset(temp, '\0', sizeof(temp));
    if ((cmdtype == IEEE80211_WLANCONFIG_NAWDS_SET_ADDR ||
            cmdtype == IEEE80211_WLANCONFIG_NAWDS_CLR_ADDR ||
            cmdtype == IEEE80211_WLANCONFIG_NAWDS_KEY) && addr != NULL) {

        if(addr != NULL) {
            memset(macaddr, '\0', sizeof(macaddr));
            if (strlcpy(macaddr, addr, sizeof(macaddr)) >= sizeof(macaddr)) {
                printf("Invalid MAC address (format: xx:xx:xx:xx:xx:xx)\n");
                return -1;
            }

            if (ether_string2mac(temp, macaddr) != 0) {
                printf("Invalid MAC address\n");
                return -1;
            }
        }
    }

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = cmdtype;
    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_NAWDS_SET_MODE:
            config.data.nawds.mode = value;
            break;
        case IEEE80211_WLANCONFIG_NAWDS_SET_DEFCAPS:
            config.data.nawds.defcaps = value;
#ifdef WLAN_FEATURE_11BE
            config.data.nawds.def_puncture_bitmap = value2;
#endif
            break;
        case IEEE80211_WLANCONFIG_NAWDS_SET_OVERRIDE:
            config.data.nawds.override = value;
            break;
        case IEEE80211_WLANCONFIG_NAWDS_SET_ADDR:
            memcpy(config.data.nawds.mac, temp, QDF_MAC_ADDR_SIZE);
            config.data.nawds.caps = value;
#ifdef WLAN_FEATURE_11BE
            config.data.nawds.puncture_bitmap = value2;
#endif
            nawds_add_new_sta(ifname, addr);
            break;
        case IEEE80211_WLANCONFIG_NAWDS_CLR_ADDR:
            memcpy(config.data.nawds.mac, temp, QDF_MAC_ADDR_SIZE);
            break;
        case IEEE80211_WLANCONFIG_NAWDS_GET:
            config.data.nawds.num = 0;
            break;
        case IEEE80211_WLANCONFIG_NAWDS_KEY:
            if (addr != NULL)
                memcpy(config.data.nawds.mac, temp, QDF_MAC_ADDR_SIZE);
            if ((strlen(psk) == 16) || (strlen(psk) == 32)) {
                memset(config.data.nawds.psk, 0, sizeof(config.data.nawds.psk));
                memcpy(config.data.nawds.psk, psk, strlen(psk));
            } else {
                printf("invalid aes psk length\n"); /* valid string lengths: 16/32  */
                return -1;
            }
            break;
        default:
            errx(1, "invalid NAWDS command");
            break;
    }
    /* call NL80211 Vendor commnd frame work */
    if (send_command(sock_ctx, ifname, link, &config,
                     sizeof(struct ieee80211_wlanconfig),
                     NULL, QCA_NL80211_VENDOR_SUBCMD_NAWDS_PARAMS,
                     IEEE80211_IOCTL_CONFIG_GENERIC) < 0) {
         printf("%s(): failed to send nawds command\n", __func__);

         if (cmdtype == IEEE80211_WLANCONFIG_NAWDS_SET_ADDR)
             nawds_del_sta(ifname, addr);
    }

    if (cmdtype == IEEE80211_WLANCONFIG_NAWDS_GET) {
        /* output the current configuration */
        printf("NAWDS configuration: \n");
        printf("Num     : %d\n", config.data.nawds.num);
        printf("Mode    : %d\n", config.data.nawds.mode);
        printf("Defcaps : %lu\n", (long unsigned int)config.data.nawds.defcaps);
        printf("Override: %d\n", config.data.nawds.override);
#ifdef WLAN_FEATURE_11BE
        printf("defPunctureBitmap: %u\n", config.data.nawds.def_puncture_bitmap);
#endif
        for (i = 0; i < config.data.nawds.num; i++) {
            config.data.nawds.num = i;
            send_command(sock_ctx, ifname, link, &config, sizeof(struct ieee80211_wlanconfig),
                    NULL, QCA_NL80211_VENDOR_SUBCMD_NAWDS_PARAMS, IEEE80211_IOCTL_CONFIG_GENERIC);
#ifdef WLAN_FEATURE_11BE
            printf("%d: %02x:%02x:%02x:%02x:%02x:%02x %lx %x\n",
                   i,
                   config.data.nawds.mac[0], config.data.nawds.mac[1],
                   config.data.nawds.mac[2], config.data.nawds.mac[3],
                   config.data.nawds.mac[4], config.data.nawds.mac[5],
                   (long unsigned int)config.data.nawds.caps,
                   config.data.nawds.puncture_bitmap);
#else
            printf("%d: %02x:%02x:%02x:%02x:%02x:%02x %lx\n",
                    i,
                    config.data.nawds.mac[0], config.data.nawds.mac[1],
                    config.data.nawds.mac[2], config.data.nawds.mac[3],
                    config.data.nawds.mac[4], config.data.nawds.mac[5],
                    (long unsigned int)config.data.nawds.caps);
#endif
        }
    }

    return 0;
}

int handle_command_nawds (int argc, char *argv[], const char *ifname, uint8_t link,
    struct socket_context *sock_ctx)
{
    if (argc == 5 && streq(argv[3], "mode")) {
        return handle_nawds(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_NAWDS_SET_MODE,
                NULL, atoi(argv[4]), NULL, 0);
#ifdef WLAN_FEATURE_11BE
    } else if (streq(argv[3], "defcaps")) {
        u_int16_t def_preamble_bitmap;

        if (argc == 6) {
            def_preamble_bitmap = (u_int16_t)strtoul(argv[5], NULL, 0);
        } else if (argc == 5) {
            def_preamble_bitmap = 0;
        } else {
            errx(1, "invalid args to NAWDS defcaps command");
            return 0;
        }

        return handle_nawds(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_NAWDS_SET_DEFCAPS,
                NULL, strtoull(argv[4], NULL, 0), NULL, def_preamble_bitmap);
#else
    } else if (argc == 5 && streq(argv[3], "defcaps")) {
        return handle_nawds(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_NAWDS_SET_DEFCAPS,
                NULL, strtoul(argv[4], NULL, 0), NULL, 0);
#endif
    } else if (argc == 5 && streq(argv[3], "override")) {
        return handle_nawds(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_NAWDS_SET_OVERRIDE,
                NULL, atoi(argv[4]), NULL, 0);
#ifdef WLAN_FEATURE_11BE
    }  else if (streq(argv[3], "add-repeater")) {
        u_int16_t preamble_bitmap;

        if (argc == 7) {
            preamble_bitmap = (u_int16_t)strtoul(argv[6], NULL, 0);
        } else if (argc == 6) {
            preamble_bitmap = 0;
        } else {
            errx(1, "Invalid args to NAWDS command");
            return 0;
        }

        return handle_nawds(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_NAWDS_SET_ADDR,
                            argv[4], strtoull(argv[5], NULL, 0), NULL, preamble_bitmap);
#else
    } else if (argc == 6 && streq(argv[3], "add-repeater")) {
        return handle_nawds(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_NAWDS_SET_ADDR,
                argv[4], strtoul(argv[5], NULL, 0), NULL, 0);
#endif
    } else if (argc == 5 && streq(argv[3], "del-repeater")) {
        return handle_nawds(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_NAWDS_CLR_ADDR,
                argv[4], 0, NULL, 0);
    } else if (argc == 4 && streq(argv[3], "list")) {
        return handle_nawds(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_NAWDS_GET,
                argv[4], 0, NULL, 0);
    } else if (argc == 6 && streq(argv[3], "addnawdskey")) {
	if(strcmp(argv[4], "-r") == 0)
	    return handle_nawds(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_NAWDS_KEY,
                NULL, 0, argv[5], 0);
	else
            return handle_nawds(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_NAWDS_KEY,
                argv[4], 0, argv[5], 0);
    } else {
        errx(1, "invalid NAWDS command");
    }
    return 0;
}
#endif /* UMAC_SUPPORT_NAWDS */

#ifdef UMAC_SUPPORT_MLNAWDS
static int handle_mlnawds(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
                          IEEE80211_WLANCONFIG_CMDTYPE cmdtype, char *mld_addr,
                          u_int64_t value)
{
    int i;
    struct ieee80211_wlanconfig config;
    char macaddr[MACSTR_LEN];
    uint8_t temp[MACSTR_LEN];

    memset(temp, '\0', sizeof(temp));
    if ((cmdtype == IEEE80211_WLANCONFIG_MLNAWDS_CLR_ADDR ||
         cmdtype == IEEE80211_WLANCONFIG_MLNAWDS_SET_MODE ||
         cmdtype == IEEE80211_WLANCONFIG_MLNAWDS_SET_OVERRIDE ||
         cmdtype == IEEE80211_WLANCONFIG_MLNAWDS_START) &&
        mld_addr != NULL) {
        if (mld_addr != NULL) {
            memset(macaddr, '\0', sizeof(macaddr));
            if (strlcpy(macaddr, mld_addr, sizeof(macaddr)) >=
                     sizeof(macaddr)) {
                printf("Invalid MAC address (format: xx:xx:xx:xx:xx:xx)\n");
                return -1;
            }

            if (ether_string2mac(temp, macaddr) != 0) {
                printf("Invalid MAC address\n");
                return -1;
            }
        }
    }

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = cmdtype;
    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_MLNAWDS_SET_MODE:
            config.data.nawds.mode = value;
            memcpy(config.data.nawds.mld_mac, temp, QDF_MAC_ADDR_SIZE);
            break;

        case IEEE80211_WLANCONFIG_MLNAWDS_SET_OVERRIDE:
            config.data.nawds.override = value;
            memcpy(config.data.nawds.mld_mac, temp, QDF_MAC_ADDR_SIZE);
            break;

        case IEEE80211_WLANCONFIG_MLNAWDS_CLR_ADDR:
        case IEEE80211_WLANCONFIG_MLNAWDS_START:
            memcpy(config.data.nawds.peer_mld_mac, temp, QDF_MAC_ADDR_SIZE);
            break;

        case IEEE80211_WLANCONFIG_MLNAWDS_GET:
            config.data.nawds.num = 0;
            break;

        default:
            errx(1, "invalid MLNAWDS command\n");
            break;
    }

    /* call NL80211 Vendor commnd frame work */
    if (send_command(sock_ctx, ifname, link, &config,
                     sizeof(struct ieee80211_wlanconfig),
                     NULL, QCA_NL80211_VENDOR_SUBCMD_MLNAWDS_PARAMS,
                     IEEE80211_IOCTL_CONFIG_GENERIC) < 0) {
        printf("%s(): Failed to send mlnawds command\n", __func__);
        return -1;
    }

    if (cmdtype == IEEE80211_WLANCONFIG_MLNAWDS_GET) {
        /* output the current configuration */
        printf("NAWDS configuration: \n");
        printf("Num     : %d\n", config.data.nawds.num);
        printf("Mode    : %d\n", config.data.nawds.mode);
        printf("Defcaps : %lu\n", (long unsigned int)config.data.nawds.defcaps);
        printf("Override: %d\n", config.data.nawds.override);
        printf("DefPunctureBitMap: %d\n",
               config.data.nawds.def_puncture_bitmap);

        for (i = 0; i < config.data.nawds.num; i++) {
             config.data.nawds.num = i;
             if (send_command(sock_ctx, ifname, link, &config,
                              sizeof(struct ieee80211_wlanconfig), NULL,
                              QCA_NL80211_VENDOR_SUBCMD_MLNAWDS_PARAMS,
                              IEEE80211_IOCTL_CONFIG_GENERIC) < 0) {
                 printf("%s(): Failed to send mlnawds command\n", __func__);
                 return -1;
             }
             printf("%d: mld: %02x:%02x:%02x:%02x:%02x:%02x linkmac: %02x:%02x:%02x:%02x:%02x:%02x link_id: %02x caps:%lx\n",
                    i,
                    config.data.nawds.peer_mld_mac[0],
                    config.data.nawds.peer_mld_mac[1],
                    config.data.nawds.peer_mld_mac[2],
                    config.data.nawds.peer_mld_mac[3],
                    config.data.nawds.peer_mld_mac[4],
                    config.data.nawds.peer_mld_mac[5],
                    config.data.nawds.peer_link_mac[0],
                    config.data.nawds.peer_link_mac[1],
                    config.data.nawds.peer_link_mac[2],
                    config.data.nawds.peer_link_mac[3],
                    config.data.nawds.peer_link_mac[4],
                    config.data.nawds.peer_link_mac[5],
                    config.data.nawds.link_id,
                    (long unsigned int)config.data.nawds.caps);
        }
    }

    return 0;
}

static int handle_mlnawds_addkey(struct socket_context *sock_ctx,
                                 const char *ifname, uint8_t link, char *mld_mac,
                                 char *peer_mld_mac, char *psk)
{
    struct ieee80211_wlanconfig config;
    uint8_t temp_mld_mac[MACSTR_LEN];
    uint8_t temp_peer_mld_mac[MACSTR_LEN];
    char temp[MACSTR_LEN];

    memset(temp_mld_mac, '\0', sizeof(temp_mld_mac));
    memset(temp_peer_mld_mac, '\0', sizeof(temp_peer_mld_mac));

    if (mld_mac != NULL) {
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(temp, mld_mac, sizeof(temp)) >= sizeof(temp)) {
            printf("Invalid  MLD MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp_mld_mac, temp) != 0) {
            printf("Invalid MAC address\n");
            return -1;
        }
    }

    if (peer_mld_mac != NULL) {
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(temp, peer_mld_mac, sizeof(temp)) >= sizeof(temp)) {
            printf("Invalid  peer MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp_peer_mld_mac, temp) != 0) {
            printf("Invalid MAC address\n");
            return -1;
        }
    }

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));

    config.cmdtype = IEEE80211_WLANCONFIG_MLNAWDS_KEY;
    memcpy(config.data.nawds.mld_mac, temp_mld_mac, QDF_MAC_ADDR_SIZE);
    memcpy(config.data.nawds.peer_mld_mac, temp_peer_mld_mac, QDF_MAC_ADDR_SIZE);
    if ((strlen(psk) == 16) || (strlen(psk) == 32)) {
        memset(config.data.nawds.psk, 0, sizeof(config.data.nawds.psk));
        memcpy(config.data.nawds.psk, psk, strlen(psk));
    } else {
        printf("invalid aes psk length\n"); /* valid string lengths: 16/32  */
        return -1;
    }

    /* call NL80211 Vendor commnd frame work */
    if (send_command(sock_ctx, ifname, link, &config,
                     sizeof(struct ieee80211_wlanconfig),
                     NULL, QCA_NL80211_VENDOR_SUBCMD_MLNAWDS_PARAMS,
                     IEEE80211_IOCTL_CONFIG_GENERIC) < 0) {
        printf("%s(): Failed to send mlnawds cmmand\n", __func__);
        return -1;
    }
    return 0;
}

static int handle_mlnawds_add_repeater(struct socket_context *sock_ctx,
                                       const char *ifname, uint8_t link, char *link_mac,
                                       char *peer_mld_mac, char *peer_link_mac,
                                       u_int64_t value,
                                       u_int64_t puncture_bitmap,
                                       u_int64_t link_id)
{
    struct ieee80211_wlanconfig config;
    uint8_t temp_link_mac[MACSTR_LEN];
    uint8_t temp_peer_mld_mac[MACSTR_LEN];
    uint8_t temp_peer_link_mac[MACSTR_LEN];
    char temp[MACSTR_LEN];

    memset(temp_link_mac, '\0', sizeof(temp_link_mac));
    memset(temp_peer_mld_mac, '\0', sizeof(temp_peer_mld_mac));
    memset(temp_peer_link_mac, '\0', sizeof(temp_peer_link_mac));

    if (link_mac != NULL) {
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(temp, link_mac, sizeof(temp)) >= sizeof(temp)) {
            printf("Invalid link MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp_link_mac, temp) != 0) {
            printf("Invalid link MAC address\n");
            return -1;
        }
    }

    if (peer_mld_mac != NULL) {
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(temp, peer_mld_mac, sizeof(temp)) >= sizeof(temp)) {
            printf("Invalid peer mld MAC address(format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp_peer_mld_mac, temp) != 0) {
            printf("Invalid peer mld MAC address\n");
            return -1;
        }
    }

    if (peer_link_mac != NULL) {
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(temp, peer_link_mac, sizeof(temp)) >= sizeof(temp)) {
            printf("Invalid peer link MAC address(format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp_peer_link_mac, temp) != 0) {
            printf("Invalid peer link MAC address\n");
            return -1;
        }
    }

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_MLNAWDS_SET_ADDR;
    memcpy(config.data.nawds.mac, temp_link_mac, QDF_MAC_ADDR_SIZE);
    memcpy(config.data.nawds.peer_mld_mac,
           temp_peer_mld_mac,
           QDF_MAC_ADDR_SIZE);
    memcpy(config.data.nawds.peer_link_mac,
           temp_peer_link_mac,
           QDF_MAC_ADDR_SIZE);
    config.data.nawds.caps = value;
    config.data.nawds.puncture_bitmap = (u_int16_t)puncture_bitmap;
    config.data.nawds.link_id = (u_int8_t)link_id;

    nawds_add_new_sta(ifname, peer_link_mac);
    /* call NL80211 Vendor commnd frame work */
    if (send_command(sock_ctx, ifname, link, &config,
                     sizeof(struct ieee80211_wlanconfig),
                     NULL, QCA_NL80211_VENDOR_SUBCMD_MLNAWDS_PARAMS,
                     IEEE80211_IOCTL_CONFIG_GENERIC) < 0) {
        printf("%s(): failed to send mlnawds command\n", __func__);
        nawds_del_sta(ifname, peer_link_mac);
        return -1;
    }
    return 0;
}

int handle_command_mlnawds(int argc, char *argv[], const char *ifname, uint8_t link,
                           struct socket_context *sock_ctx)
{
    if (argc == 6 && streq(argv[4], "mode")) {
        return handle_mlnawds(sock_ctx, ifname, link,
                              IEEE80211_WLANCONFIG_MLNAWDS_SET_MODE,
                               argv[3], atoi(argv[5]));
    } else if (argc == 6 && streq(argv[4], "override")) {
        return handle_mlnawds(sock_ctx, ifname, link,
                              IEEE80211_WLANCONFIG_MLNAWDS_SET_OVERRIDE,
                              argv[3], atoi(argv[5]));
    } else if (argc == 10 && streq(argv[3], "add-repeater")) {
        return handle_mlnawds_add_repeater(sock_ctx, ifname, link,
                                           argv[4], argv[5], argv[6],
                                           strtoul(argv[7], NULL, 0),
                                           strtoul(argv[8], NULL, 0),
                                           strtoul(argv[9], NULL, 0));
    } else if (argc == 5 && streq(argv[3], "del-repeater")) {
        return handle_mlnawds(sock_ctx, ifname, link,
                              IEEE80211_WLANCONFIG_MLNAWDS_CLR_ADDR,
                              argv[4], 0);
    } else if (argc == 4 && streq(argv[3], "list")) {
        return handle_mlnawds(sock_ctx, ifname, link,
                              IEEE80211_WLANCONFIG_MLNAWDS_GET,
                              NULL, 0);
    } else if (argc == 7 && streq(argv[3], "addnawdskey")) {
        return handle_mlnawds_addkey(sock_ctx, ifname, link,
                                     argv[4], argv[5], argv[6]);
    } else if (argc == 5 && streq(argv[3], "start")) {
        return handle_mlnawds(sock_ctx, ifname, link,
                              IEEE80211_WLANCONFIG_MLNAWDS_START,
                              argv[4], 0);
    } else {
        errx(1, "invalid NAWDS command");
    }
    return 0;
}
#endif /* UMAC_SUPPORT_MLNAWDS */
