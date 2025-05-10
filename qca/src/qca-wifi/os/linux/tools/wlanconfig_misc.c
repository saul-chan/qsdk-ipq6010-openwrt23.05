/*
 * Copyright (c) 2018-2019, 2022-2024 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 */

#include <wlanconfig.h>
#include <inttypes.h>
#ifdef WLAN_CFR_ENABLE
#include <wlan_cfr_public_structs.h>
#endif
#define MAX_UINT8_t 255
#define MAX_IPV6_PREFIX 128
#define IPV6_UNIT32_SIZE 4
#define WLAN_FLOW_INVALID_SVC_ID 0xFFFFFFFF

enum me_list_type {
    HMMC,
    DENY_LIST
};

int strtoint(char *arg)
{
    int val;
    if (!strncmp (arg, "0x", 2)) {
        sscanf(arg, "%x", &val);
    } else {
        sscanf(arg, "%d", &val);
    }
    return val;
}

#if ATH_SUPPORT_HYFI_ENHANCEMENTS

static int handle_ald(struct socket_context *sock_ctx, const char *ifname, uint8_t link, IEEE80211_WLANCONFIG_CMDTYPE cmdtype,
        char *addr1, int enable)
{
    struct ieee80211_wlanconfig config;
    char ni_macaddr[MACSTR_LEN] = {0};
    uint8_t temp[MACSTR_LEN] = {0};

    if (addr1 && strlen(addr1) != 17) {
        printf("Invalid MAC address (format: xx:xx:xx:xx:xx:xx)\n");
        return -1;
    }

    if (addr1) {
        memset(ni_macaddr, '\0', sizeof(ni_macaddr));
        if(strlcpy(ni_macaddr, addr1, sizeof(ni_macaddr)) >= sizeof(ni_macaddr)) {
            printf("Invalid MAC address1\n");
            return -1;
        }
        if (ether_string2mac(temp, ni_macaddr) != 0) {
            printf("Invalid MAC address1\n");
            return -1;
        }
    }

    memset(&config, 0, sizeof config);
    /* fill up configuration */
    config.cmdtype = cmdtype;
    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_ALD_STA_ENABLE:
            memcpy(config.data.ald.data.ald_sta.macaddr, temp, QDF_MAC_ADDR_SIZE);
            config.data.ald.data.ald_sta.enable = (u_int32_t)enable;
            break;
        default:
            printf("%d command not handled yet", cmdtype);
            break;
    }
    send_command(sock_ctx, ifname, link, &config, sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_ALD_PARAMS, IEEE80211_IOCTL_CONFIG_GENERIC);

    return 0;
}

int handle_command_ald (int argc, char *argv[], const char *ifname, uint8_t link,
    struct socket_context *sock_ctx)
{
    if (argc == 6 && streq(argv[3], "sta-enable")) {
        return handle_ald(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_ALD_STA_ENABLE,
                argv[4], atoi(argv[5]));
    } else {
        errx(1, "invalid ALD command");
    }
    return 0;
}

static int handle_hmmc(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
    IEEE80211_WLANCONFIG_CMDTYPE cmdtype, char *ip_str, char *mask_str)
{
    int ip=0, mask=0;
    struct ieee80211_wlanconfig config;
    if (cmdtype == IEEE80211_WLANCONFIG_ME_LIST_ADD ||
            cmdtype == IEEE80211_WLANCONFIG_ME_LIST_DEL) {
        if ((ip = inet_addr(ip_str)) == -1 || !ip) {
            printf("Invalid ip string %s\n", ip_str);
            return -1;
        }

        if (!(mask = inet_addr(mask_str))) {
            printf("Invalid ip mask string %s\n", mask_str);
            return -1;
        }
    }

    config.cmdtype = cmdtype;
    config.data.me_list.me_list_type = IEEE80211_HMMC_LIST;
    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_ME_LIST_ADD:
        case IEEE80211_WLANCONFIG_ME_LIST_DEL:
            config.data.me_list.ip = ip;
            config.data.me_list.mask = mask;
            break;
        case IEEE80211_WLANCONFIG_ME_LIST_DUMP:
            break;
        default:
            perror("invalid cmdtype");
            return -1;
    }
    send_command(sock_ctx, ifname, link, &config, sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_ME_LIST, IEEE80211_IOCTL_CONFIG_GENERIC);

    return 0;
}

int handle_command_hmmc (int argc, char *argv[], const char *ifname, uint8_t link,
    struct socket_context *sock_ctx)
{
    if (argc == 4 && streq(argv[3], "dump")) {
        return handle_hmmc(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_ME_LIST_DUMP, NULL, NULL);
    } else if (argc == 6 && streq(argv[3], "add")) {
        return handle_hmmc(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_ME_LIST_ADD, argv[4], argv[5]);
    } else if (argc == 6 && streq(argv[3], "del")) {
        return handle_hmmc(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_ME_LIST_DEL, argv[4], argv[5]);
        errx(1, "invalid HMMC command");
    }
    return 0;
}

static int handle_command_hmmc_deny_list_v6(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
    IEEE80211_WLANCONFIG_CMDTYPE cmdtype, char *ip_str, char *mask_str, enum me_list_type typ)
{
    uint32_t ip[IPV6_UNIT32_SIZE] = {0};
    uint32_t mask = 0;
    struct ieee80211_wlanconfig config;

    if (cmdtype == IEEE80211_WLANCONFIG_ME_LIST_ADD ||
            cmdtype == IEEE80211_WLANCONFIG_ME_LIST_DEL) {
        if (!inet_pton(AF_INET6, ip_str, (struct in_addr *)ip)) {
            printf("Could not convert src IP address to network order\n");
            return -1;
        }
        mask = strtoul(mask_str, NULL, 10);
        if (mask > MAX_IPV6_PREFIX ) {
            printf("Invalid ip mask string %s\n", mask_str);
            return -1;
        }
    }

    config.cmdtype = cmdtype;
    if (typ == DENY_LIST) {
        config.data.me_list.me_list_type = IEEE80211_DENY_LIST_V6;
    }
    else {
        config.data.me_list.me_list_type = IEEE80211_HMMC_LIST_V6;
    }

    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_ME_LIST_ADD:
        case IEEE80211_WLANCONFIG_ME_LIST_DEL:
            memcpy(config.data.me_list.ipv6, ip, sizeof(uint32_t)*4);
            config.data.me_list.mask = mask;
            break;
        case IEEE80211_WLANCONFIG_ME_LIST_DUMP:
            break;
        default:
            perror("invalid cmdtype");
            return -1;
    }
    send_command(sock_ctx, ifname, link, &config, sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_ME_LIST, IEEE80211_IOCTL_CONFIG_GENERIC);

    return 0;
}

int handle_command_hmmc_v6(int argc, char *argv[], const char *ifname, uint8_t link,
    struct socket_context *sock_ctx)
{
    if (argc == 4 && streq(argv[3], "dump")) {
        return handle_command_hmmc_deny_list_v6(sock_ctx, ifname, link,
                         IEEE80211_WLANCONFIG_ME_LIST_DUMP, NULL, NULL, HMMC);
    } else if (argc == 6 && streq(argv[3], "add")) {
        return handle_command_hmmc_deny_list_v6(sock_ctx, ifname, link,
                    IEEE80211_WLANCONFIG_ME_LIST_ADD, argv[4], argv[5], HMMC);
    } else if (argc == 6 && streq(argv[3], "del")) {
        return handle_command_hmmc_deny_list_v6(sock_ctx, ifname, link,
                     IEEE80211_WLANCONFIG_ME_LIST_DEL, argv[4], argv[5], HMMC);
        errx(1, "invalid HMMC command");
    }
    return 0;
}

static int handle_deny_list(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
    IEEE80211_WLANCONFIG_CMDTYPE cmdtype, char *ip_str, char *mask_str)
{
    int ip=0, mask=0;
    struct ieee80211_wlanconfig config;
    if (cmdtype == IEEE80211_WLANCONFIG_ME_LIST_ADD ||
            cmdtype == IEEE80211_WLANCONFIG_ME_LIST_DEL) {
        if ((ip = inet_addr(ip_str)) == -1 || !ip) {
            printf("Invalid ip string %s\n", ip_str);
            return -1;
        }

        if (!(mask = inet_addr(mask_str))) {
            printf("Invalid ip mask string %s\n", mask_str);
            return -1;
        }
    }

    config.cmdtype = cmdtype;
    config.data.me_list.me_list_type = IEEE80211_DENY_LIST;
    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_ME_LIST_ADD:
        case IEEE80211_WLANCONFIG_ME_LIST_DEL:
            config.data.me_list.ip = ip;
            config.data.me_list.mask = mask;
            break;
        case IEEE80211_WLANCONFIG_ME_LIST_DUMP:
            break;
        default:
            perror("invalid cmdtype");
            return -1;
    }
    send_command(sock_ctx, ifname, link, &config, sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_ME_LIST, IEEE80211_IOCTL_CONFIG_GENERIC);

    return 0;
}

int handle_command_deny_list(int argc, char *argv[], const char *ifname, uint8_t link,
    struct socket_context *sock_ctx)
{
    if (argc == 4 && streq(argv[3], "dump")) {
        return handle_deny_list(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_ME_LIST_DUMP, NULL, NULL);
    } else if (argc == 6 && streq(argv[3], "add")) {
        return handle_deny_list(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_ME_LIST_ADD, argv[4], argv[5]);
    } else if (argc == 6 && streq(argv[3], "del")) {
        return handle_deny_list(sock_ctx, ifname, link, IEEE80211_WLANCONFIG_ME_LIST_DEL, argv[4], argv[5]);
        errx(1, "invalid deny_list command");
    }
    return 0;
}

int handle_command_deny_list_v6(int argc, char *argv[], const char *ifname, uint8_t link,
    struct socket_context *sock_ctx)
{
    if (argc == 4 && streq(argv[3], "dump")) {
        return handle_command_hmmc_deny_list_v6(sock_ctx, ifname, link,
                    IEEE80211_WLANCONFIG_ME_LIST_DUMP, NULL, NULL, DENY_LIST);
    } else if (argc == 6 && streq(argv[3], "add")) {
        return handle_command_hmmc_deny_list_v6(sock_ctx, ifname, link,
                 IEEE80211_WLANCONFIG_ME_LIST_ADD, argv[4], argv[5], DENY_LIST);
    } else if (argc == 6 && streq(argv[3], "del")) {
        return handle_command_hmmc_deny_list_v6(sock_ctx, ifname, link,
                 IEEE80211_WLANCONFIG_ME_LIST_DEL, argv[4], argv[5], DENY_LIST);
        errx(1, "invalid deny_list command");
    }
    return 0;
}
#endif

static int atf_show_stat(const char *ifname, char *macaddr)
{
    int s, i;
    struct iwreq iwr;
    struct ieee80211_wlanconfig config;
    uint8_t len, lbyte, ubyte;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        printf("socket error\n");
        return 1;
    }

    memset(&config, 0, sizeof(config));
    config.cmdtype = IEEE80211_PARAM_STA_ATF_STAT;
    len = strlen(macaddr);
    for (i = 0; i < len; i += 2) {
        lbyte = ubyte = 0;

        if ((macaddr[i] >= '0') && (macaddr[i] <= '9'))  {
            ubyte = macaddr[i] - '0';
        } else if ((macaddr[i] >= 'A') && (macaddr[i] <= 'F')) {
            ubyte = macaddr[i] - 'A' + 10;
        } else if ((macaddr[i] >= 'a') && (macaddr[i] <= 'f')) {
            ubyte = macaddr[i] - 'a' + 10;
        }

        if ((macaddr[i + 1] >= '0') && (macaddr[i + 1] <= '9'))  {
            lbyte = macaddr[i + 1] - '0';
        } else if ((macaddr[i + 1] >= 'A') && (macaddr[i + 1] <= 'F')) {
            lbyte = macaddr[i + 1] - 'A' + 10;
        } else if ((macaddr[i + 1] >= 'a') && (macaddr[i + 1] <= 'f')) {
            lbyte = macaddr[i + 1] - 'a' + 10;
        }

        config.data.atf.macaddr[i / 2] = (ubyte << 4) | lbyte;
    }
    memset(&iwr, 0, sizeof(iwr));

    if (strlcpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name)) >= sizeof(iwr.ifr_name)) {
        fprintf(stderr, "ifname too long: %s\n", ifname);
        close(s);
        return -1;
    }

    iwr.u.data.pointer = (void *)&config;
    iwr.u.data.length = sizeof(config);

    if (ioctl(s, IEEE80211_IOCTL_CONFIG_GENERIC, &iwr) < 0) {
        printf("unable to get stats\n");
        close(s);
        return 1;
    }

    fprintf(stderr, "Short Average %d, Total Used Tokens %"PRIu64"\n",
            config.data.atf.short_avg, config.data.atf.total_used_tokens);

    close(s);
    return 0;
}

int handle_command_atfstat (int argc, char *argv[], const char *ifname, uint8_t link,
    struct socket_context *sock_ctx)
{
        if (argc > 3)
            atf_show_stat(ifname, argv[3]);
        else
            fprintf(stderr,"\n\n     Missing parameters!!  \n\n");
        return 0;
}

int handle_command_lcr(int argc, char *argv[], const char *ifname, uint8_t link,
        struct socket_context *sock_ctx)
{
    struct ieee80211_wlanconfig config;
    struct ieee80211_wlanconfig_lcr *lcr = &(config.data.lcr_config);
    char buffer[256] = {0};
    int i =0;

    if (strlen(argv[4]) != COUNTRY_CODE_LEN) {
        fprintf(stderr, "Invalid country code length\n");
        return -1;
    }

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_LCR;

    memcpy(lcr->country_code, argv[4], COUNTRY_CODE_LEN);
    memcpy(buffer, argv[5], strlen(argv[5]));

    for (i = 0; (i < 256) && (i < strlen(buffer)/2); i++)
        if(sscanf(&buffer[2*i], "%2hhx" , &lcr->civic_info[i]) <= 0)
            break;

    lcr->civic_len = i;

    return send_command(sock_ctx, ifname, link, &config,
                        sizeof(struct ieee80211_wlanconfig),
                        NULL, QCA_NL80211_VENDOR_SUBCMD_RTT_CONFIG,
                        IEEE80211_IOCTL_CONFIG_GENERIC);
}

int handle_command_lci(int argc, char *argv[], const char *ifname, uint8_t link,
        struct socket_context *sock_ctx)
{
    struct ieee80211_wlanconfig config;
    struct ieee80211_wlanconfig_lci *lci = &(config.data.lci_config);

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_LCI;

    lci->latitude = atol(argv[4]);
    lci->longitude = atol(argv[5]);
    lci->altitude = atoi(argv[6]);
    lci->latitude_unc = atoi(argv[7]);
    lci->longitude_unc = atoi(argv[8]);
    lci->altitude_unc = atoi(argv[9]);
    lci->motion_pattern= atoi(argv[10]);
    lci->floor = atoi(argv[11]);
    lci->height_above_floor = atoi(argv[12]);
    lci->height_unc = atoi(argv[13]);

    return send_command(sock_ctx, ifname, link, &config,
                        sizeof(struct ieee80211_wlanconfig),
                        NULL, QCA_NL80211_VENDOR_SUBCMD_RTT_CONFIG,
                        IEEE80211_IOCTL_CONFIG_GENERIC);
}

int get_mac_addr(const char *src, char *dest)
{
    char macaddr[MACSTR_LEN] = {0};
    uint8_t temp[MACSTR_LEN] = {0};

    if (strlcpy(macaddr, src, sizeof(macaddr)) >= sizeof(macaddr)) {
        fprintf(stderr, "Invalid MAC address (format: xx:xx:xx:xx:xx:xx)\n");
        return -1;
    }

    if (ether_string2mac(temp, macaddr) != 0) {
        fprintf(stderr, "Invalid MAC address\n");
        return -1;
    }

    memcpy(dest, temp, QDF_MAC_ADDR_SIZE);

    return 0;
}

int handle_command_ftmrr(int argc, char *argv[], const char *ifname, uint8_t link,
                         struct socket_context *sock_ctx)
{
    struct ieee80211_wlanconfig config;
    struct ieee80211_wlanconfig_ftmrr *ftmrr = &(config.data.ftmrr_config);
    int retval = 0;
    int i = 0, index = 0;
    char buffer[8];
    uint8_t info[4];
    int j = 0;

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_FTMRR;

    retval = get_mac_addr(argv[4], (char *)&ftmrr->sta_mac);
    if (retval)
        return retval;


    ftmrr->random_interval = atoi(argv[5]);
    ftmrr->num_elements = atoi(argv[6]);

    index = 7;

    for (i = 0; i < ftmrr->num_elements; i++) {

        /* Check if all the eight variables from ieee80211_wlanconfig_ftmrr_elems
         * structure are provided by the user.
         */
        if ((argc - index) < FTMRR_ELEMS) {
            fprintf(stderr, "Invalid number of arguments\n");
            return -1;
        }

        retval = get_mac_addr(argv[index], (char *)&ftmrr->elem[i].bssid);
        if (retval)
            return retval;
        index++;

        memset(buffer, '\0', sizeof(buffer));
        memcpy(&buffer, argv[index], sizeof(buffer));
        index++;

        memset(info, '\0', sizeof(info));
        for (j = 0; j < 4; j++) {
            if(sscanf(&buffer[2*j], "%2hhx" , &info[j]) <= 0)
                break;
        }

        ftmrr->elem[i].bssid_info = *((uint32_t *)info);
        ftmrr->elem[i].chan = atoi(argv[index++]);
        ftmrr->elem[i].center_ch1 = atoi(argv[index++]);
        ftmrr->elem[i].center_ch2 = atoi(argv[index++]);
        ftmrr->elem[i].chwidth = atoi(argv[index++]);
        ftmrr->elem[i].opclass = atoi(argv[index++]);
        ftmrr->elem[i].phytype = atoi(argv[index++]);
    }

    /* User can provide ftmrr->num_elements as '1' and more than one set of
     * neighbor elements. ex: ' wlanconfig ath0 rtt ftmrr 00:03:7f:12:92:eb 20 1
     * {00:03:7f:12:92:eb c91c0000 149 155 0 2 128 9} {00:03:7f:12:92:eb}'.
     * In the above case, return error.
     */
    if ((argc - index) != 0) {
        fprintf(stderr, "Invalid number of arguments\n");
        return -1;
    }

    return send_command(sock_ctx, ifname, link, &config,
                        sizeof(struct ieee80211_wlanconfig),
                        NULL, QCA_NL80211_VENDOR_SUBCMD_RTT_CONFIG,
                        IEEE80211_IOCTL_CONFIG_GENERIC);
}

int handle_command_rtt(int argc, char *argv[], const char *ifname, uint8_t link,
        struct socket_context *sock_ctx)
{
    if ((argc == 6) && streq(argv[3], "lcr"))
        return handle_command_lcr(argc, argv, ifname, link, sock_ctx);
    else if ((argc == 14) && streq(argv[3], "lci"))
        return handle_command_lci(argc, argv, ifname, link, sock_ctx);
    if ((argc >= 15) && streq(argv[3], "ftmrr"))
        return handle_command_ftmrr(argc, argv, ifname, link, sock_ctx);
    else {
        fprintf(stderr, "Invalid RTT command format: Usage\n");
        fprintf(stderr, "wlanconfig athx rtt lcr <country code string> <civic info>\n");
        fprintf(stderr, "wlanconfig athx rtt lci <latitude> <longitude> <altitude> <latitude_unc> <longitude_unc> <altitude_unc> <motion_pattern> <floor> <height_above_floor> <height_unc>\n");
        fprintf(stderr, "wlanconfig athx rtt ftmrr <STA MAC_addr> <random_interval> <num_elements> [<BSSID> <BSSID info> <channel number> <center_ch1> <center_ch2> <chwidth> <opclass> <phytype>] [...]\n");
        return -1;
    }
}

#if WLAN_CFR_ENABLE
static int handle_cfr_start_command(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
        char *peer_mac, int band_width, int periodicity, int capture_type)
{
    struct ieee80211_wlanconfig config;
    char macaddr[MACSTR_LEN];
    uint8_t temp[MACSTR_LEN];

    memset(temp, '\0', sizeof(temp));
    config.cmdtype = IEEE80211_WLANCONFIG_CFR_START;
    if(peer_mac != NULL) {
        memset(macaddr, '\0', sizeof(macaddr));
        if (strlcpy(macaddr, peer_mac, sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf("Invalid MAC address\n");
            return -1;
        }
        memcpy(config.data.cfr_config.mac, temp, QDF_MAC_ADDR_SIZE);
    }

    config.data.cfr_config.bandwidth = band_width;
    config.data.cfr_config.periodicity = periodicity;
    config.data.cfr_config.capture_method = capture_type;

    send_command(sock_ctx, ifname, link, &config, sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG, IEEE80211_IOCTL_CONFIG_GENERIC);

    return 0;
}

static int handle_cfr_stop_command(struct socket_context *sock_ctx, const char *ifname,
        uint8_t link, char *peer_mac)
{
    struct ieee80211_wlanconfig config;
    char macaddr[MACSTR_LEN];
    uint8_t temp[MACSTR_LEN];

    memset(temp, '\0', sizeof(temp));
    config.cmdtype = IEEE80211_WLANCONFIG_CFR_STOP;
    if(peer_mac != NULL) {
        memset(macaddr, '\0', sizeof(macaddr));
        if (strlcpy(macaddr, peer_mac, sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf("Invalid MAC address\n");
            return -1;
        }
        memcpy(config.data.cfr_config.mac, temp, QDF_MAC_ADDR_SIZE);
    }

    send_command(sock_ctx, ifname, link, &config, sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG, IEEE80211_IOCTL_CONFIG_GENERIC);

    return 0;
}

static int handle_cfr_list_command(struct socket_context *sock_ctx, const char *ifname, uint8_t link)
{
    /*
     * List all peers that are CFR enabled & Display all CFR capture properties
     */
    struct ieee80211_wlanconfig config;
    config.cmdtype = IEEE80211_WLANCONFIG_CFR_LIST_PEERS;
    send_command(sock_ctx, ifname, link, &config, sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG, IEEE80211_IOCTL_CONFIG_GENERIC);
    return 0;
}
#endif

#ifdef WLAN_ENH_CFR_ENABLE
/*
 * Purpose of this handler is to display the last successfully committed
 * configuration.
 *
 * @sock_ctx: socket context
 * @ifname  : interface name (athX)
 *
 * Return: 0 on success, negative errno on failure
 *
 */

static int
handle_get_rcc(struct socket_context *sock_ctx, const char *ifname, uint8_t link, IEEE80211_WLANCONFIG_CMDTYPE cmdtype)
{
    struct ieee80211_wlanconfig config;
    int retv = 0;
    config.cmdtype = cmdtype;

    retv = send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig), NULL,
            QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}

/*
 * User given input for TA, TA_MASK, RA and RA_MASK will be processed here.
 *
 * @sock_ctx: socket context
 * @ifname  : interface name (athX)
 *
 * grp_id : Group id, can be of any value, in between 0 and 15
 * ta     : Tx address, should be the client MAC address, which will send the
 *         packet to the Root AP.
 * ta_mask: Tx address mask
 * ra     : Rx address, could be the MAC address of AP or the User can provide
 *          00:00:00:00:00:00, based on below logic:
 *
 *         Match = ((addr & expect_mask) == expect_addr)
 *
 * ra_mask: RX address mask
 *
 * Return: 0 on success, negative errno on failure
 */

static int
handle_taraAddr_config(struct socket_context *sock_ctx, const char *ifname,
        uint8_t link, int grp_id, char *ta, char *ta_mask, char *ra, char *ra_mask)
{
    struct ieee80211_wlanconfig config;
    char macaddr[MACSTR_LEN];
    uint8_t temp[MACSTR_LEN];
    int retv = 0;

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_CFR_RCC_TA_RA_ADDR;

    if (grp_id > 15 || grp_id < 0) {
        printf("Invalid Filter config group: GRP should be from 0 to 15.\n");
        return -EINVAL;
    }

    config.data.cfr_config.grp_id = grp_id;

    if (ta != NULL) {
        memset(macaddr, '\0', sizeof(macaddr));
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(macaddr, ta, sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid TX address (format: xx:xx:xx:xx:xx:xx)\n");
            return -EINVAL;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf("Invalid MAC address for TA\n");
            return -EINVAL;
        }
        memcpy(config.data.cfr_config.ta, temp, QDF_MAC_ADDR_SIZE);
    }

    if (ta_mask != NULL) {
        memset(macaddr, '\0', sizeof(macaddr));
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(macaddr, ta_mask, sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid TA_MASK (format: xx:xx:xx:xx:xx:xx)\n");
            return -EINVAL;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf("Invalid MAC address\n");
            return -EINVAL;
        }
        memcpy(config.data.cfr_config.ta_mask, temp, QDF_MAC_ADDR_SIZE);
    }

    if (ra != NULL) {
        memset(macaddr, '\0', sizeof(macaddr));
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(macaddr, ra, sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid MAC address for RA (format: xx:xx:xx:xx:xx:xx)\n");
            return -EINVAL;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf("Invalid MAC address\n");
            return -EINVAL;
        }
        memcpy(config.data.cfr_config.ra, temp, QDF_MAC_ADDR_SIZE);
    }

    if (ra_mask != NULL) {
        memset(macaddr, '\0', sizeof(macaddr));
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(macaddr, ra_mask, sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid RX Addr Mask (format: xx:xx:xx:xx:xx:xx)\n");
            return -EINVAL;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf("Invalid MAC address for RA_mask\n");
            return -EINVAL;
        }
        memcpy(config.data.cfr_config.ra_mask, temp, QDF_MAC_ADDR_SIZE);
    }

    retv = send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig), NULL,
            QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}

/*
 * User given inputs for BW, NSS, and type/subtypes will be preocessed here.
 *
 * @sock_ctx: socket context
 * @ifname  : interface name (athX)
 *
 * cmdtype: Specific cmdtypes representing BW & NSS, Type/ subtypes
 * val1: group id
 *
 * While configuring BW and NSS:
 * val2: BW
 * val3: NSS
 * val4: Null
 *
 * While configuring type/subtype:
 * val2: MGMT
 * val3: CTRL
 * val4: Data
 *
 * Return: 0 on success, negative errno on failure
 */

#define MAX_CFR_RCC_BW 0x3F
static int
handle_tara_config(struct socket_context *sock_ctx, const char *ifname,
        uint8_t link, IEEE80211_WLANCONFIG_CMDTYPE cmdtype,
        int val1, int val2, int val3, int val4)
{
    struct ieee80211_wlanconfig config;
    int retv = 0;


    if (val1 > 15 || val1 < 0) {
        printf("Invalid Filter config group: GRP should be from 0 to 15.\n");
        return -EINVAL;
    }

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = cmdtype;

    config.data.cfr_config.grp_id = val1;

    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_CFR_RCC_BW_NSS:
            {
                if (val2 <= MAX_CFR_RCC_BW && val2 >= 0) {
                    config.data.cfr_config.bw = val2;
                } else {
                    printf("Unsupported bandwidth.\n");
                }
                if (val3 >= 0 && val3 <= 255) {
                    config.data.cfr_config.nss = val3;
                } else {
                    printf("Unsupported NSS.\n");
                }
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_SUBTYPE:
            {
                if (val2 <= 0xFFFF && val2 >= 0) {
                    config.data.cfr_config.expected_mgmt_subtype = val2;
                } else {
                    printf("Unsupported mgmt subtype.\n");
                }
                if (val3 <= 0xFFFF && val3 >= 0) {
                    config.data.cfr_config.expected_ctrl_subtype = val3;
                } else {
                    printf("Unsupported ctrl subtype.\n");
                }
                if (val4 <= 0xFFFF && val4 >= 0) {
                    config.data.cfr_config.expected_data_subtype = val4;
                } else {
                    printf("Unsupported data subtype.\n");
                }
            }
            break;

        default:
            errx(1, "Invalid cfr rcc mode");
            break;
    }

    /* call NL80211 Vendor commnd frame work */
    retv = send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig), NULL,
            QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}


/*
 * User given inputs specific to fixed params and commit
 * will be preocessed here.
 *
 * @sock_ctx: socket context
 * @ifname  : interface name (athX)
 *
 * cmdtype: Specific cmdtypes representing the fixed parameters used for RCC,
 *          commit handler and handler to disable all capture modes at a time.
 *
 * val1 and val2: Values provided by user based on the cmdtype.
 *
 * Return: 0 on success, negative errno on failure
 *
 */

static int
handle_cfr_rcc_config(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
        IEEE80211_WLANCONFIG_CMDTYPE cmdtype, uint32_t val1, uint32_t val2)
{
    struct ieee80211_wlanconfig config;
    int retv = 0;

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = cmdtype;

    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_CFR_RCC_CAPT_DUR:
            {
                if (val1 > 0xffffff) {
                    printf("Invalid entry:\t"
                            "Capture duration should not be greater than\t"
                            "(16.8 sec) 0xFFFFFF\n");
                    return -EINVAL;
                }
                config.data.cfr_config.cap_dur = val1;
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_CAPT_INTVAL:
            {
                if (val1 > 0xffffff) {
                    printf("Invalid entry:\t"
                            "Capture interval should not be greater than\t"
                            "(16.8 sec) 0xFFFFFF\n");
                    return -EINVAL;
                }
                config.data.cfr_config.cap_intvl = val1;
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_EN_CFG:
            {
                if (val1 > 0xffff) {
                    printf("Invalid entry:\t"
                            "Valid entry should be in between 1 to 0xFFFF\n");
                    return -EINVAL;
                }
                config.data.cfr_config.en_cfg = val1;
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RESET_CFG:
            {
                if (val1 > 0xffff) {
                    printf("Invalid entry:\t"
                            "Valid entry should be in between 1 to 0xFFFF\n");
                    return -EINVAL;
                }
                config.data.cfr_config.reset_cfg = val1;
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_UL_MU_USER_MASK:
            {
                config.data.cfr_config.ul_mu_user_mask_lower = val1;
                config.data.cfr_config.ul_mu_user_mask_upper = val2;
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_FREEZE_TLV_DELAY_CNT:
            {
                if (val2 > 0xff) {
                    printf("Invalid entry:\t"
                            "Valid entry should be in between 1 to 0xFF\n");
                    return -EINVAL;
                }
                config.data.cfr_config.freeze_tlv_delay_cnt_en = val1;
                config.data.cfr_config.freeze_tlv_delay_cnt_thr = val2;
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_CAPT_COUNT:
            {
                if (val1 > MAX_CAPTURE_COUNT_VAL) {
                    printf("Invalid value: count must be <= 0x%x\n",
                           MAX_CAPTURE_COUNT_VAL);
                    return -EINVAL;
                }
                config.data.cfr_config.cap_count = val1;
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_CAPT_INTVAL_MODE_SEL:
            {
                config.data.cfr_config.cap_intval_mode_sel = !!val1;
                if (val1) {
                    printf("capture interval & count mode is set\n");
                } else {
                    printf("capture interval & duration mode is set\n");
                }
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_TARA_FILTER_AS_FP:
            config.data.cfr_config.en_ta_ra_filter_in_as_fp = !!val1;
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_DISABLE_ALL:
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_COMMIT:
            break;

        default:
            errx(1, "Invalid cfr rcc config");
            break;
    }

    retv = send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig), NULL,
            QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}
#endif

#ifdef CONFIG_SAWF_DEF_QUEUES
#define MAX_SAWF_CONFIG_VALUE 0xFFFFFFFF
#define DISABLED_MODE_MAX_LEN 128
static void
sawf_fill_svc_param(struct sawf_wlanconfig_create_param *config,
                    uint32_t svc_id, char *app_name,
                    char *min_thruput_rate, char *max_thruput_rate,
                    char *burst_size, char *service_interval,
                    char *delay_bound, char *msdu_ttl,
                    char *priority, char *tid, char *msdu_rate_loss,
                    char *ul_burst_size, char *ul_service_interval,
                    char *dl_disabled_modes, char *ul_disabled_modes,
                    char *ul_min_tput, char *ul_max_latency)
{
    uint32_t mask = 0;
    config->svc_id = svc_id;

    if (app_name)
        strlcpy(config->app_name, app_name, 64);

    strlcpy(config->ul_disabled_modes, ul_disabled_modes,
            DISABLED_MODE_MAX_LEN);
    if ((ul_disabled_modes[0] != 'X') && (ul_disabled_modes[0] != 'x'))
        mask |= (1 << WLAN_SVC_PARAM_UL_SCHED_DISABLE_MODES);

    strlcpy(config->dl_disabled_modes, dl_disabled_modes,
            DISABLED_MODE_MAX_LEN);
    if ((dl_disabled_modes[0] != 'X') && (dl_disabled_modes[0] != 'x'))
        mask |= (1 << WLAN_SVC_PARAM_DL_SCHED_DISABLE_MODES);

    if ((min_thruput_rate[0] == 'X') || (min_thruput_rate[0] == 'x')) {
        config->min_thruput_rate = MAX_SAWF_CONFIG_VALUE;
    } else {
        config->min_thruput_rate = atoi(min_thruput_rate);
        mask |= (1 << WLAN_SVC_PARAM_MIN_TPUT);
    }

    if ((max_thruput_rate[0] == 'X') || (max_thruput_rate[0] == 'x')) {
        config->max_thruput_rate = MAX_SAWF_CONFIG_VALUE;
    } else {
        config->max_thruput_rate = atoi(max_thruput_rate);
        mask |= (1 << WLAN_SVC_PARAM_MAX_TPUT);
    }

    if ((burst_size[0] == 'X') || (burst_size[0] == 'x')) {
       config->burst_size = MAX_SAWF_CONFIG_VALUE;
    } else {
        config->burst_size = atoi(burst_size);
        mask |= (1 << WLAN_SVC_PARAM_BURST_SIZE);
    }

    if ((service_interval[0] == 'X') || (service_interval[0] == 'x')) {
        config->service_interval = MAX_SAWF_CONFIG_VALUE;
    } else {
        config->service_interval = atoi(service_interval);
        mask |= (1 << WLAN_SVC_PARAM_SERVICE_INTERVAL);
    }

    if ((delay_bound[0] == 'X') || (delay_bound[0] == 'x')) {
        config->delay_bound = MAX_SAWF_CONFIG_VALUE;
    } else {
        config->delay_bound = atoi(delay_bound);
        mask |= (1 << WLAN_SVC_PARAM_DELAY_BOUND);
    }

    if ((msdu_ttl[0] == 'X') || (msdu_ttl[0] == 'x')) {
        config->msdu_ttl = MAX_SAWF_CONFIG_VALUE;
    } else {
        config->msdu_ttl = atoi(msdu_ttl);
        mask |= (1 << WLAN_SVC_PARAM_MSDU_TTL);
    }

    if ((priority[0] == 'X') || (priority[0] == 'x')) {
        config->priority = MAX_SAWF_CONFIG_VALUE;
    } else {
        config->priority = atoi(priority);
        mask |= (1 << WLAN_SVC_PARAM_PRIORITY);
    }

    if ((tid[0] == 'X') || (tid[0] == 'x')) {
        config->tid = MAX_SAWF_CONFIG_VALUE;
    } else {
        config->tid = atoi(tid);
        mask |= (1 << WLAN_SVC_PARAM_TID);
    }

    if ((msdu_rate_loss[0] == 'X') || (msdu_rate_loss[0] == 'x')) {
        config->msdu_rate_loss = MAX_SAWF_CONFIG_VALUE;
    } else {
        config->msdu_rate_loss = atoi(msdu_rate_loss);
        mask |= (1 << WLAN_SVC_PARAM_MSDU_RATE_LOSS);
    }

    if ((ul_burst_size[0] == 'X') || (ul_burst_size[0] == 'x')) {
        config->ul_burst_size = 0;
    } else {
        config->ul_burst_size = atoi(ul_burst_size);
        mask |= (1 << WLAN_SVC_PARAM_UL_BURST_SIZE);
    }

    if ((ul_service_interval[0] == 'X') || (ul_service_interval[0] == 'x')) {
        config->ul_service_interval = 0;
    } else {
        config->ul_service_interval = atoi(ul_service_interval);
        mask |= (1 << WLAN_SVC_PARAM_UL_SERVICE_INTERVAL);
    }

    if ((ul_min_tput[0] == 'X') || (ul_min_tput[0] == 'x')) {
        config->ul_min_tput = 0;
    } else {
        config->ul_min_tput = atoi(ul_min_tput);
        mask |= (1 << WLAN_SVC_PARAM_UL_MIN_TPUT);
    }

    if ((ul_max_latency[0] == 'X') || (ul_max_latency[0] == 'x')) {
        config->ul_max_latency = 0;
    } else {
        config->ul_max_latency = atoi(ul_max_latency);
        mask |= (1 << WLAN_SVC_PARAM_UL_MAX_LATENCY);
    }

    config->param_set_mask = mask;
}

static int
handle_service_class_create_command(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
        IEEE80211_WLANCONFIG_CMDTYPE cmdtype, uint8_t svc_id, char *app_name,
        char * min_thruput_rate, char * max_thruput_rate,
        char * burst_size, char * service_interval,
        char * delay_bound, char * msdu_ttl,
        char * priority, char * tid, char * msdu_rate_loss,
        char *ul_burst_size, char *ul_service_interval, char *dl_disabled_modes,
        char *ul_disabled_modes, char *ul_min_tput, char *ul_max_latency)
{
    struct ieee80211_wlanconfig config;
    int retv = 0;

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = cmdtype;

    sawf_fill_svc_param(&config.data.sawf_create_config,
                        svc_id, app_name, min_thruput_rate, max_thruput_rate,
                        burst_size, service_interval, delay_bound, msdu_ttl,
                        priority, tid, msdu_rate_loss,
                        ul_burst_size, ul_service_interval,
                        dl_disabled_modes, ul_disabled_modes,
                        ul_min_tput, ul_max_latency);

    /* Set the service class type to SAWF */
    config.data.sawf_create_config.svc_type = WLAN_SVC_TYPE_SAWF;

    /* call NL80211 Vendor commnd frame work */

    retv = send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig), NULL,
            QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}

static int
handle_service_class_disable_command(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
        IEEE80211_WLANCONFIG_CMDTYPE cmdtype, uint8_t value)
{
    struct ieee80211_wlanconfig config;
    int retv = 0;

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = cmdtype;

    config.data.sawf_disable_config.svc_id = value;
    config.data.sawf_disable_config.svc_type = WLAN_SVC_TYPE_SAWF;

    /* call NL80211 Vendor commnd frame work */
    retv = send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig), NULL,
            QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}

static int
handle_service_class_view_command(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
        IEEE80211_WLANCONFIG_CMDTYPE cmdtype)
{
    struct ieee80211_wlanconfig config;
    int retv = 0;

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = cmdtype;

    /* call NL80211 Vendor commnd frame work */
    retv = send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig), NULL,
            QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}

static void
sawf_svc_rsp_handler(struct cfg80211_data *buffer)
{
    struct sawf_wlanconfig_svc_rsp *rsp;
    uint8_t *buf = buffer->data;
    unsigned int len = buffer->length;

    if (len != sizeof(struct sawf_wlanconfig_svc_rsp)) {
        printf("Invalid length %d\n", len);
        return;
    }

    rsp = (struct sawf_wlanconfig_svc_rsp *)buf;

    printf("service class id : %u \n", rsp->svc_id);
}

static int
handle_sawf_svc_create_cmd(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
                           IEEE80211_WLANCONFIG_CMDTYPE cmdtype,
                           char *mac_addr, uint8_t type,
                           uint8_t direction, char *app_name,
                           char *min_thruput_rate, char *max_thruput_rate,
                           char *burst_size, char *service_interval,
                           char *delay_bound, char *msdu_ttl,
                           char *priority, char *tid, char *msdu_rate_loss,
                           char *ul_burst_size, char *ul_service_interval,
                           char *dl_disabled_modes, char *ul_disabled_modes,
                           char *ul_min_tput, char *ul_max_latency)
{
    struct ieee80211_wlanconfig config = {0};
    struct sawf_wlanconfig_svc_create *svc_create_param;
    struct sawf_wlanconfig_create_param params = {0};
    uint8_t svc_id = 0;
    int retv;

    sawf_fill_svc_param(&params, svc_id, app_name,
                        min_thruput_rate, max_thruput_rate,
                        burst_size, service_interval, delay_bound, msdu_ttl,
                        priority, tid, msdu_rate_loss,
                        ul_burst_size, ul_service_interval,
                        dl_disabled_modes, ul_disabled_modes,
                        ul_min_tput, ul_max_latency);

    config.cmdtype = cmdtype;

    svc_create_param = &config.data.sawf_svc_create;

    strlcpy(svc_create_param->app_name, params.app_name,
            sizeof(svc_create_param->app_name));
    svc_create_param->min_thruput_rate = params.min_thruput_rate;
    svc_create_param->max_thruput_rate = params.max_thruput_rate;

    svc_create_param->burst_size = params.burst_size;
    svc_create_param->service_interval = params.service_interval;
    svc_create_param->delay_bound = params.delay_bound;
    svc_create_param->msdu_ttl = params.msdu_ttl;
    svc_create_param->priority = params.priority;
    svc_create_param->tid = params.tid;
    svc_create_param->msdu_rate_loss = params.msdu_rate_loss;
    svc_create_param->ul_service_interval = params.ul_service_interval;
    svc_create_param->ul_burst_size = params.ul_burst_size;
    strlcpy(svc_create_param->dl_disabled_modes, params.dl_disabled_modes,
            sizeof(svc_create_param->dl_disabled_modes));
    strlcpy(svc_create_param->ul_disabled_modes, params.ul_disabled_modes,
            sizeof(svc_create_param->ul_disabled_modes));
    svc_create_param->ul_min_tput = params.ul_min_tput;
    svc_create_param->ul_max_latency = params.ul_max_latency;

    svc_create_param->mask = params.param_set_mask;

    svc_create_param->type = type;
    svc_create_param->direction = direction;

    ether_string2mac(svc_create_param->peer_mac_addr, mac_addr);

    retv = send_command(sock_ctx, ifname, link, &config,
                        sizeof(struct ieee80211_wlanconfig),
                        sawf_svc_rsp_handler,
                        QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG,
                        IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}

static int
handle_sawf_svc_delete_cmd(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
                           IEEE80211_WLANCONFIG_CMDTYPE cmdtype,
                           char *mac_addr, uint8_t type, uint32_t svc_id)
{
    struct ieee80211_wlanconfig config = {0};
    struct sawf_wlanconfig_svc_delete *svc_delete_param;
    int retv;

    config.cmdtype = cmdtype;

    svc_delete_param = &config.data.sawf_svc_delete;

    svc_delete_param->type = type;
    svc_delete_param->svc_id = svc_id;

    ether_string2mac(svc_delete_param->peer_mac_addr, mac_addr);

    retv = send_command(sock_ctx, ifname, link, &config,
                        sizeof(struct ieee80211_wlanconfig), NULL,
                        QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG,
                        IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}

static int
handle_sawf_svc_update_cmd(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
                           IEEE80211_WLANCONFIG_CMDTYPE cmdtype,
                           char *mac_addr, uint8_t type, uint32_t svc_id,
                           uint8_t direction, char *app_name,
                           char *min_thruput_rate, char *max_thruput_rate,
                           char *burst_size, char *service_interval,
                           char *delay_bound, char *msdu_ttl,
                           char *priority, char *tid, char *msdu_rate_loss,
                           char *ul_burst_size, char *ul_service_interval,
                           char *dl_disabled_modes, char *ul_disabled_modes,
                           char *ul_min_tput, char *ul_max_latency)
{
    struct ieee80211_wlanconfig config = {0};
    struct sawf_wlanconfig_svc_update *svc_update_param;
    struct sawf_wlanconfig_create_param params = {0};
    int retv;

    sawf_fill_svc_param(&params, svc_id, app_name,
                        min_thruput_rate, max_thruput_rate,
                        burst_size, service_interval, delay_bound, msdu_ttl,
                        priority, tid, msdu_rate_loss,
                        ul_burst_size, ul_service_interval,
                        dl_disabled_modes, ul_disabled_modes,
                        ul_min_tput, ul_max_latency);

    config.cmdtype = cmdtype;

    svc_update_param = &config.data.sawf_svc_update;

    strlcpy(svc_update_param->app_name, params.app_name,
            sizeof(svc_update_param->app_name));
    svc_update_param->min_thruput_rate = params.min_thruput_rate;
    svc_update_param->max_thruput_rate = params.max_thruput_rate;

    svc_update_param->burst_size = params.burst_size;
    svc_update_param->service_interval = params.service_interval;
    svc_update_param->delay_bound = params.delay_bound;
    svc_update_param->msdu_ttl = params.msdu_ttl;
    svc_update_param->priority = params.priority;
    svc_update_param->tid = params.tid;
    svc_update_param->msdu_rate_loss = params.msdu_rate_loss;
    svc_update_param->ul_service_interval = params.ul_service_interval;
    svc_update_param->ul_burst_size = params.ul_burst_size;
    strlcpy(svc_update_param->dl_disabled_modes, params.dl_disabled_modes,
            sizeof(svc_update_param->dl_disabled_modes));
    strlcpy(svc_update_param->ul_disabled_modes, params.ul_disabled_modes,
            sizeof(svc_update_param->ul_disabled_modes));
    svc_update_param->ul_min_tput = params.ul_min_tput;
    svc_update_param->ul_max_latency = params.ul_max_latency;

    svc_update_param->mask = params.param_set_mask;
    svc_update_param->svc_id = params.svc_id;
    svc_update_param->type = type;
    svc_update_param->direction = direction;

    ether_string2mac(svc_update_param->peer_mac_addr, mac_addr);

    retv = send_command(sock_ctx, ifname, link, &config,
                        sizeof(struct ieee80211_wlanconfig),
                        sawf_svc_rsp_handler,
                        QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG,
                        IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}

int handle_command_service_class(int argc, char *argv[], const char *ifname, uint8_t link,
        struct socket_context *sock_ctx)
{
	if (argc < 4)
	    goto err;

	if (streq(argv[3], "create")) {
		if (argc < 21) {
			printf("Invalid SAWF service class command format: Usage\n");
			printf("\t wlanconfig wifix service_class create <service_id> <app_name> ");
			printf("<dl_min_thruput_rate> <dl_max_thruput_rate> <dl_burst_size> <dl_service_interval>\n");
			printf("<dl_delay_bound> <dl_msdu_ttl> <dl_priority> <tid> <dl_msdu_rate_loss> ");
			printf("<ul_burst_size> <ul_service_interval> <dl_disabled_mode> <ul_disabled_mode>\n");
			printf("<ul_min_tput> <ul_max_latency>\n");
			return  -1;
		}
		return handle_service_class_create_command(
				sock_ctx, ifname, link, IEEE80211_WLANCONFIG_SVC_CLASS_CREATE,
				atoi(argv[4]), argv[5], argv[6], argv[7], argv[8],
				argv[9], argv[10], argv[11], argv[12], argv[13], argv[14],
				argv[15], argv[16], argv[17], argv[18], argv[19], argv[20]);
	} else if (streq(argv[3], "disable")) {
		if (argc == 4) {
			printf("Invalid SAWF service class command format: Usage\n");
			printf("\t wlanconfig wifix service_class disable <service_id>\n");
			return -1;
		}
		return handle_service_class_disable_command(
				sock_ctx, ifname, link,
				IEEE80211_WLANCONFIG_SVC_CLASS_DISABLE,
				atoi(argv[4]));
	}
	else if (streq(argv[3], "view")) {
		return handle_service_class_view_command(
				sock_ctx, ifname, link,
				IEEE80211_WLANCONFIG_SVC_CLASS_VIEW);
	} else if (streq(argv[3], "create_svc")) {
		if (argc < 23) {
			printf("Invalid SAWF service class command format: Usage\n");
			printf("\t wlanconfig wifix service_class create_svc <MAC> <type> <direction> <app_name> ");
			printf("<dl_min_thruput_rate> <dl_max_thruput_rate> <dl_burst_size> <dl_service_interval>\n");
			printf("<dl_delay_bound> <dl_msdu_ttl> <dl_priority> <tid> <dl_msdu_rate_loss> ");
			printf("<ul_burst_size> <ul_service_interval> <dl_disabled_mode> <ul_disabled_mode>\n");
			printf("<ul_min_tput> <ul_max_latency>\n");
			return  -1;
		}
		return handle_sawf_svc_create_cmd(sock_ctx, ifname, link,
				IEEE80211_WLANCONFIG_SVC_CREATE,
				argv[4], atoi(argv[5]), atoi(argv[6]), argv[7],
				argv[8], argv[9], argv[10], argv[11],
				argv[12], argv[13],
				argv[14], argv[15], argv[16],
				argv[17], argv[18],
				argv[19], argv[20],
				argv[21], argv[22]);
	} else if (streq(argv[3], "delete_svc")) {
		if (argc < 7) {
			printf("Invalid SAWF service class command format: Usage\n");
			printf("\t wlanconfig wifix service_class delete_svc <MAC> <type> <service_id>\n");
			return  -1;
		}
		return handle_sawf_svc_delete_cmd(sock_ctx, ifname, link,
				IEEE80211_WLANCONFIG_SVC_DELETE,
				argv[4], atoi(argv[5]), atoi(argv[6]));
	} else if (streq(argv[3], "update_svc")) {
		if (argc < 24) {
			printf("Invalid SAWF service class command format: Usage\n");
			printf("\t wlanconfig wifix service_class update_svc <MAC> <type> <svc_id> <direction> <app_name> ");
			printf("<dl_min_thruput_rate> <dl_max_thruput_rate> <dl_burst_size> <dl_service_interval>\n");
			printf("<dl_delay_bound> <dl_msdu_ttl> <dl_priority> <tid> <dl_msdu_rate_loss> ");
			printf("<ul_burst_size> <ul_service_interval> <dl_disabled_mode> <ul_disabled_mode>\n");
			printf("<ul_min_tput> <ul_max_latency>\n");
			return  -1;
		}
		return handle_sawf_svc_update_cmd(sock_ctx, ifname, link,
				IEEE80211_WLANCONFIG_SVC_UPDATE,
				argv[4], atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), argv[8],
				argv[9], argv[10], argv[11],
				argv[12], argv[13],
				argv[14], argv[15], argv[16],
				argv[17], argv[18],
				argv[19], argv[20],
				argv[21], argv[22], argv[23]);
	}
err:
		printf("Invalid SAWF service_class command format: Usage\n");
		printf("\t wlanconfig wifix service_class create <service_id> <app_name> ");
		printf("<dl_min_thruput_rate> <dl_max_thruput_rate> <dl_burst_size> <dl_service_interval>\n");
		printf("<dl_delay_bound> <dl_msdu_ttl> <dl_priority> <tid> <dl_msdu_rate_loss> ");
		printf("<ul_burst_size> <ul_service_interval> <dl_disabled_mode> <ul_disabled_mode>\n");
		printf("<ul_min_tput> <ul_max_latency>\n");
		printf("\t wlanconfig wifix service_class disable <service_id>\n");
		printf("\t wlanconfig wifix service_class view\n");
		printf("\t wlanconfig wifix service_class create_svc <MAC> <type> <direction> <app_name> ");
		printf("<dl_min_thruput_rate> <dl_max_thruput_rate> <dl_burst_size> <dl_service_interval>\n");
		printf("<dl_delay_bound> <dl_msdu_ttl> <dl_priority> <tid> <dl_msdu_rate_loss> ");
		printf("<ul_burst_size> <ul_service_interval> <dl_disabled_mode> <ul_disabled_mode>\n");
		printf("<ul_min_tput> <ul_max_latency>\n");
		printf("\t wlanconfig wifix service_class delete_svc <MAC> <type> <service_id>\n");
		printf("\t wlanconfig wifix service_class update_svc <MAC> <type> <svc_id> <direction> <app_name> ");
		printf("<dl_min_thruput_rate> <dl_max_thruput_rate> <dl_burst_size> <dl_service_interval>\n");
		printf("<dl_delay_bound> <dl_msdu_ttl> <dl_priority> <tid> <dl_msdu_rate_loss> ");
		printf("<ul_burst_size> <ul_service_interval> <dl_disabled_mode> <ul_disabled_mode>\n");
		printf("<ul_min_tput> <ul_max_latency>\n");

		return -1;
}
#endif

int handle_command_enable_ol_stats(int argc, char *argv[], const char *ifname, uint8_t link,
				   struct socket_context *sock_ctx)
{
	struct ieee80211_wlanconfig config;
	int retv = 0;
	uint8_t val = atoi(argv[3]);

	 /* fill up configuration */
	memset(&config, 0, sizeof(struct ieee80211_wlanconfig));

	config.data.enable_ol_val.value = val;
	config.cmdtype = IEEE80211_WLANCONFIG_ENABLE_OL_STATS;

	if (argc != 4)
		goto err;

	/* call NL80211 Vendor commnd frame work */
	retv = send_command(sock_ctx, ifname, link, &config,
			sizeof(struct ieee80211_wlanconfig), NULL,
			QCA_NL80211_VENDOR_SUBCMD_ENABLE_OL_STATS,
			IEEE80211_IOCTL_CONFIG_GENERIC);
	return retv;

err:
	printf("Invalid enable_ol_stats format\n");
	return -1;
}

#ifdef CONFIG_SAWF
int handle_command_sawf_telemetry(int argc, char *argv[], const char *ifname,
        uint8_t link, struct socket_context *sock_ctx)
{
	struct ieee80211_wlanconfig config;
	int retv = 0;

	if (argc != 7)
		goto err;

	/* fill up configuration */
	memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
	config.cmdtype = IEEE80211_WLANCONFIG_SAWF_TELEMETRY;

	/* Sanitize the input for non zero value */
	if (atoi(argv[3]) == 0 || atoi(argv[4]) == 0 ||
	    atoi(argv[5]) == 0 || atoi(argv[6]) == 0) {
		printf("Invalid SAWF telemetry config values\n");
		goto err;
	}

	config.data.sawf_telemetry.mv_avg_pkt = atoi(argv[3]);
	config.data.sawf_telemetry.mv_avg_win = atoi(argv[4]);
	config.data.sawf_telemetry.sla_num_pkt = atoi(argv[5]);
	config.data.sawf_telemetry.sla_time_sec = atoi(argv[6]);

	/* call NL80211 Vendor commnd frame work */
	retv = send_command(sock_ctx, ifname, link, &config,
		sizeof(struct ieee80211_wlanconfig), NULL,
		QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG,
		IEEE80211_IOCTL_CONFIG_GENERIC);

	return retv;

err:
	printf("Invalid SAWF telemetry config command format: Usage\n");
	printf("\t wlanconfig wifix sawf_telemetry <mv_avg_pkt> <mv_avg_window> <sla_num_pkt> <sla_time_sec>\n");
	return -1;
}


static void
update_sawf_sla_detect_param(struct sawf_wlanconfig_sla_detect *param,
			     enum sawf_sla_detect detect_param,
			     uint8_t min_thruput_rate, uint8_t max_thruput_rate,
			     uint8_t burst_size, uint8_t service_interval,
			     uint8_t delay_bound, uint8_t msdu_ttl, uint8_t msdu_rate_loss)
{
	param->sla_detect = detect_param;
	param->min_thruput_rate = min_thruput_rate;
	param->max_thruput_rate = max_thruput_rate;
	param->burst_size = burst_size;
	param->service_interval = service_interval;
	param->delay_bound = delay_bound;
	param->msdu_ttl = msdu_ttl;
	param->msdu_rate_loss = msdu_rate_loss;
}

static int
handle_sawf_sla_num_packet(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
			   enum sawf_sla_detect detect_param,
			   uint8_t min_thruput_rate, uint8_t max_thruput_rate,
			   uint8_t burst_size, uint8_t service_interval,
			   uint8_t delay_bound, uint8_t msdu_ttl, uint8_t msdu_rate_loss)
{
	struct ieee80211_wlanconfig config;

	if (min_thruput_rate || max_thruput_rate || burst_size
		|| service_interval) {
		printf("Unsupported SAWF SLA detect config  for number of packets.\n");
		return -1;
	}

	/* fill up configuration */
	memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
	config.cmdtype = IEEE80211_WLANCONFIG_SAWF_SLA_DETECT;

	update_sawf_sla_detect_param(&config.data.sawf_sla_detect,
				     detect_param,
				     min_thruput_rate, max_thruput_rate,
				     burst_size, service_interval,
				     delay_bound, msdu_ttl, msdu_rate_loss);

	return send_command(sock_ctx, ifname, link, &config,
		sizeof(struct ieee80211_wlanconfig), NULL,
		QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG,
		IEEE80211_IOCTL_CONFIG_GENERIC);
}

static int
handle_sawf_sla_per_second(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
			  enum sawf_sla_detect detect_param,
			  uint8_t min_thruput_rate, uint8_t max_thruput_rate,
			  uint8_t burst_size, uint8_t service_interval,
			  uint8_t delay_bound, uint8_t msdu_ttl, uint8_t msdu_rate_loss)
{
	struct ieee80211_wlanconfig config;

	if ( burst_size || service_interval || delay_bound || msdu_ttl || msdu_rate_loss) {
		printf("Unsupported SAWF SLA detect config for per second.\n");
		return -1;
	}

	/* fill up configuration */
	memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
	config.cmdtype = IEEE80211_WLANCONFIG_SAWF_SLA_DETECT;

	update_sawf_sla_detect_param(&config.data.sawf_sla_detect,
				     detect_param,
				     min_thruput_rate, max_thruput_rate,
				     burst_size, service_interval,
				     delay_bound, msdu_ttl, msdu_rate_loss);

	return send_command(sock_ctx, ifname, link, &config,
		sizeof(struct ieee80211_wlanconfig), NULL,
		QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG,
		IEEE80211_IOCTL_CONFIG_GENERIC);
}

static int
handle_sawf_sla_moving_avg(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
			   enum sawf_sla_detect detect_param,
			   uint8_t min_thruput_rate, uint8_t max_thruput_rate,
			   uint8_t burst_size, uint8_t service_interval,
			   uint8_t delay_bound, uint8_t msdu_ttl, uint8_t msdu_rate_loss)
{
	struct ieee80211_wlanconfig config;

	if (min_thruput_rate || max_thruput_rate || burst_size || service_interval
		|| msdu_ttl || msdu_rate_loss) {
		printf("Unsupported SAWF SLA detect config for moving average.\n");
		return -1;
	}

	/* fill up configuration */
	memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
	config.cmdtype = IEEE80211_WLANCONFIG_SAWF_SLA_DETECT;

	update_sawf_sla_detect_param(&config.data.sawf_sla_detect,
				     detect_param,
				     min_thruput_rate, max_thruput_rate,
				     burst_size, service_interval,
				     delay_bound, msdu_ttl, msdu_rate_loss);

	return send_command(sock_ctx, ifname, link, &config,
		sizeof(struct ieee80211_wlanconfig), NULL,
		QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG,
		IEEE80211_IOCTL_CONFIG_GENERIC);
}

static int
handle_sawf_sla_num_second(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
	enum sawf_sla_detect detect_param,
	uint8_t min_thruput_rate, uint8_t max_thruput_rate,
	uint8_t burst_size, uint8_t service_interval,
	uint8_t delay_bound, uint8_t msdu_ttl, uint8_t msdu_rate_loss)
{
	struct ieee80211_wlanconfig config;

	if (min_thruput_rate || max_thruput_rate || delay_bound) {
		printf("Unsupported SAWF SLA detect config for number of seconds.\n");
		return -1;
	}

	/* fill up configuration */
	memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
	config.cmdtype = IEEE80211_WLANCONFIG_SAWF_SLA_DETECT;

	update_sawf_sla_detect_param(&config.data.sawf_sla_detect,
				     detect_param,
				     min_thruput_rate, max_thruput_rate,
				     burst_size, service_interval,
				     delay_bound, msdu_ttl, msdu_rate_loss);

	return send_command(sock_ctx, ifname, link, &config,
		sizeof(struct ieee80211_wlanconfig), NULL,
		QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG,
		IEEE80211_IOCTL_CONFIG_GENERIC);
}

int handle_command_sawf_sla_detect(int argc, char *argv[], const char *ifname,
			    uint8_t link, struct socket_context *sock_ctx)
{
	if (argc != 11)
		goto err;

	if (streq(argv[3], "num_packet")) {
		return handle_sawf_sla_num_packet(
				sock_ctx, ifname, link, SAWF_SLA_DETECT_NUM_PACKET,
				atoi(argv[4]), atoi(argv[5]), atoi(argv[6]),
				atoi(argv[7]), atoi(argv[8]), atoi(argv[9]),
				atoi(argv[10]));
	}

	if (streq(argv[3], "per_second")) {
		return handle_sawf_sla_per_second(
				sock_ctx, ifname, link, SAWF_SLA_DETECT_PER_SECOND,
				atoi(argv[4]), atoi(argv[5]), atoi(argv[6]),
				atoi(argv[7]), atoi(argv[8]), atoi(argv[9]),
				atoi(argv[10]));
	}

	if (streq(argv[3], "moving_avg")) {
		return handle_sawf_sla_moving_avg(
				sock_ctx, ifname, link, SAWF_SLA_DETECT_MOV_AVG,
				atoi(argv[4]), atoi(argv[5]), atoi(argv[6]),
				atoi(argv[7]), atoi(argv[8]), atoi(argv[9]),
				atoi(argv[10]));
	}

	if (streq(argv[3], "num_second")) {
		return handle_sawf_sla_num_second(
				sock_ctx, ifname, link, SAWF_SLA_DETECT_NUM_SECOND,
				atoi(argv[4]), atoi(argv[5]), atoi(argv[6]),
				atoi(argv[7]), atoi(argv[8]), atoi(argv[9]),
				atoi(argv[10]));
	}

err:
	printf("Invalid SAWF sawf_sla_detect command format: Usage\n");
	printf("\t wlanconfig wifix sawf_sla <detect_param>");
	printf("<min_thruput_rate> <max_thruput_rate> <burst_size> <service_interval>");
	printf("<delay_bound> <msdu_ttl> <msdu_rate_loss>\n");

	return -1;
}

int handle_command_sawf_sla(int argc, char *argv[], const char *ifname,
			    uint8_t link, struct socket_context *sock_ctx)
{
	struct ieee80211_wlanconfig config;
	int retv = 0;

	if (argc < 4 || argc != 11)
		goto err;

	/* fill up configuration */
	memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
	config.cmdtype = IEEE80211_WLANCONFIG_SAWF_SLA;

	config.data.sawf_sla.svc_id = atoi(argv[3]);

	if (atoi(argv[4]) < 0 || atoi(argv[4]) > 100) {
		config.data.sawf_sla.min_thruput_rate = 0;
	}
	else {
		config.data.sawf_sla.min_thruput_rate = atoi(argv[4]);
	}

	if (atoi(argv[5]) < 0 || atoi(argv[5]) > 100) {
		config.data.sawf_sla.max_thruput_rate = 0;
	}
	else {
		config.data.sawf_sla.max_thruput_rate = atoi(argv[5]);
	}

	if (atoi(argv[6]) < 0 || atoi(argv[6]) > 100) {
		config.data.sawf_sla.burst_size = 0;
	}
	else {
		config.data.sawf_sla.burst_size = atoi(argv[6]);
	}

	if (atoi(argv[7]) < 0 || atoi(argv[7]) > 100) {
		config.data.sawf_sla.service_interval = 0;
	}
	else {
		config.data.sawf_sla.service_interval = atoi(argv[7]);
	}

	if (atoi(argv[8]) < 0 || atoi(argv[8]) > 100) {
		config.data.sawf_sla.delay_bound = 0;
	}
	else {
		config.data.sawf_sla.delay_bound = atoi(argv[8]);
	}

	if (atoi(argv[9]) < 0 || atoi(argv[9]) > 100) {
		config.data.sawf_sla.msdu_ttl = 0;
	}
	else {
		config.data.sawf_sla.msdu_ttl = atoi(argv[9]);
	}

	if (atoi(argv[10]) < 0 || atoi(argv[10]) > 100) {
		config.data.sawf_sla.msdu_rate_loss = 0;
	}
	else {
		config.data.sawf_sla.msdu_rate_loss = atoi(argv[10]);
	}

	return send_command(sock_ctx, ifname, link, &config,
		sizeof(struct ieee80211_wlanconfig), NULL,
		QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG,
		IEEE80211_IOCTL_CONFIG_GENERIC);

err:
	printf("Invalid SAWF sawf_sla command format: Usage\n");
	printf("\t wlanconfig wifix sawf_sla <service_id>");
	printf("<min_thruput_rate> <max_thruput_rate> <burst_size> <service_interval>");
	printf("<delay_bound> <msdu_ttl> <msdu_rate_loss>\n");

	return retv;
}
#endif

#ifdef WLAN_CFR_ENABLE
/*
 * There are totally 6 new capture modes introduced in RCC.
 * Capture modes:
 *          1) m_directed_ftm
 *          2) m_directed_ndpa_ndp
 *          3) m_ta_ra_filter
 *          4) m_ndpa_ndp_all
 *          5) m_all_ftm_ack  (Not supported in Cypress)
 *          6) m_all_pkt      (Not supported for now)
 *
 *
 * @sock_ctx: socket context
 * @ifname  : interface name (athX)
 * @link :link id of MLD netdev
 *
 * cmdtype: Specific cmdtypes representing different capture modes
 *          has been mentioned below as part of switch cases
 * val: Could be either 0 or 1
 *          0: To disable the capture mode
 *          1: To enable the capture mode
 *
 * Return: 0 on success, negative errno on failure
 * Note: Multiple capture modes can co-exist.
 *
 */

static int
handle_cfr_rcc_mode(struct socket_context *sock_ctx, const char *ifname,
        uint8_t link, IEEE80211_WLANCONFIG_CMDTYPE cmdtype, int val)
{
    struct ieee80211_wlanconfig config;
    int retv = 0;


    if ((val < 0)) {
        printf("Valid values: 0:Disable,\t"
                "non-zero:Enable/capture duration/capture interval\n");
        return -EINVAL;
    }

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = cmdtype;

    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_CFR_RCC_DIRECT_FTM:
            {
                if (val) {
                    config.data.cfr_config.en_directed_ftm = 1;
                } else {
                    config.data.cfr_config.dis_directed_ftm = 1;
                }
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_DIRECT_NDPA_NDP:
            {
                if (val) {
                    config.data.cfr_config.en_directed_ndpa_ndp = 1;
                } else {
                    config.data.cfr_config.dis_directed_ndpa_ndp = 1;
                }
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_TA_RA_FLITER:
            {
                if (val) {
                    config.data.cfr_config.en_ta_ra_filter = 1;
                } else {
                    config.data.cfr_config.dis_ta_ra_filter = 1;
                }
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_ALL_FTM_ACK:
            {
                if (val) {
                    config.data.cfr_config.en_all_ftm_ack = 1;
                } else {
                    config.data.cfr_config.dis_all_ftm_ack = 1;
                }
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_NDPA_NDP_ALL:
            {
                if (val) {
                    config.data.cfr_config.en_ndpa_ndp_all = 1;
                } else {
                    config.data.cfr_config.dis_ndpa_ndp_all = 1;
                }
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_ALL_PKT:
            {
                if (val) {
                    config.data.cfr_config.en_all_pkt = 1;
                } else {
                    config.data.cfr_config.dis_all_pkt = 1;
                }
            }
            break;

        default:
            errx(1, "Invalid cfr rcc mode");
            break;
    }

    /* call NL80211 Vendor commnd frame work */
    retv = send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig), NULL,
            QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}
#endif

#ifdef WLAN_WIFI_RADAR_ENABLE
static int
handle_wifi_radar_capture_cmd(struct socket_context *sock_ctx,
                              const char *ifname, int tx_chain_mask,
                              int rx_chain_mask, int num_ltf_tx,
                              int num_skip_ltf_rx, int num_ltf_accumulation,
                              int bw, int interval)
{
    struct ieee80211_wlanconfig config = {0};

    config.cmdtype = IEEE80211_WLANCONFIG_WIFI_RADAR_CAPTURE;
    config.data.wifi_radar_config.bandwidth = bw;
    config.data.wifi_radar_config.periodicity = interval;
    config.data.wifi_radar_config.tx_chainmask = tx_chain_mask;
    config.data.wifi_radar_config.rx_chainmask = rx_chain_mask;
    config.data.wifi_radar_config.num_ltf_tx = num_ltf_tx;
    config.data.wifi_radar_config.num_skip_ltf_rx = num_skip_ltf_rx;
    config.data.wifi_radar_config.num_ltf_accumulation = num_ltf_accumulation;

    send_command(sock_ctx, ifname, MLO_INVALID_LINK_ID, &config,
                 sizeof(struct ieee80211_wlanconfig), NULL,
                 QCA_NL80211_VENDOR_SUBCMD_WIFI_RADAR_CONFIG,
                 IEEE80211_IOCTL_CONFIG_GENERIC);
    return 0;
}

static int
handle_wifi_radar_capture_stop_cmd(struct socket_context *sock_ctx,
                                   const char *ifname)
{
    struct ieee80211_wlanconfig config = {0};

    config.cmdtype = IEEE80211_WLANCONFIG_WIFI_RADAR_CAPTURE_STOP;

    send_command(sock_ctx, ifname, MLO_INVALID_LINK_ID, &config,
                 sizeof(struct ieee80211_wlanconfig), NULL,
                 QCA_NL80211_VENDOR_SUBCMD_WIFI_RADAR_CONFIG,
                 IEEE80211_IOCTL_CONFIG_GENERIC);
    return 0;
}

static int
handle_wifi_radar_tx_cal_cmd(struct socket_context *sock_ctx,
                             const char *ifname)
{
    struct ieee80211_wlanconfig config = {0};

    config.cmdtype = IEEE80211_WLANCONFIG_WIFI_RADAR_TX_CAL;

    send_command(sock_ctx, ifname, MLO_INVALID_LINK_ID, &config,
                 sizeof(struct ieee80211_wlanconfig), NULL,
                 QCA_NL80211_VENDOR_SUBCMD_WIFI_RADAR_CONFIG,
                 IEEE80211_IOCTL_CONFIG_GENERIC);
    return 0;
}

static int
handle_wifi_radar_rx_cal_cmd(struct socket_context *sock_ctx,
                             const char *ifname, int tx_chain_mask,
                             int rx_chain_mask, int num_ltf_tx,
                             int num_skip_ltf_rx, int num_ltf_accumulation,
                             int bw)
{
    struct ieee80211_wlanconfig config = {0};

    config.cmdtype = IEEE80211_WLANCONFIG_WIFI_RADAR_RX_CAL;
    config.data.wifi_radar_config.bandwidth = bw;
    config.data.wifi_radar_config.tx_chainmask = tx_chain_mask;
    config.data.wifi_radar_config.rx_chainmask = rx_chain_mask;
    config.data.wifi_radar_config.num_ltf_tx = num_ltf_tx;
    config.data.wifi_radar_config.num_skip_ltf_rx = num_skip_ltf_rx;
    config.data.wifi_radar_config.num_ltf_accumulation = num_ltf_accumulation;

    send_command(sock_ctx, ifname, MLO_INVALID_LINK_ID, &config,
                 sizeof(struct ieee80211_wlanconfig), NULL,
                 QCA_NL80211_VENDOR_SUBCMD_WIFI_RADAR_CONFIG,
                 IEEE80211_IOCTL_CONFIG_GENERIC);
    return 0;
}

static int
handle_wifi_radar_get_rx_cal_status(struct socket_context *sock_ctx,
                                    const char *ifname)
{
    struct ieee80211_wlanconfig config = {0};
    int tx_chain, rx_chain;
    struct ieee80211_wifi_radar_cal_info *info = &config.data.wifi_radar_cal_info;

    config.cmdtype = IEEE80211_WLANCONFIG_WIFI_RADAR_GET_RX_CAL_STATUS;

    if (send_command(sock_ctx, ifname, MLO_INVALID_LINK_ID, &config,
                 sizeof(struct ieee80211_wlanconfig), NULL,
                 QCA_NL80211_VENDOR_SUBCMD_WIFI_RADAR_CONFIG,
                 IEEE80211_IOCTL_CONFIG_GENERIC) < 0) {
        printf("%s(): failed to send wifi radar command\n", __func__);
    }

    printf("wifi radar cal status info:\n");
    printf("timestap: %lu\n", (unsigned long int)info->timestamp);
    printf("wifi_radar_pkt_bw: %d\n", info->wifi_radar_pkt_bw);
    printf("channel_bw: %d\n", info->channel_bw);
    printf("band_center_freq: %d\n", info->band_center_freq);
    printf("cal_num_ltf_tx: %d\n", info->cal_num_ltf_tx);
    printf("cal_num_skip_ltf_rx: %d\n", info->cal_num_skip_ltf_rx);
    printf("cal_num_ltf_accumulation: %d\n", info->cal_num_ltf_accumulation);

    printf("************* per chain calibration status ***************\n");
    for (tx_chain = 0; tx_chain < HOST_MAX_CHAINS; tx_chain++) {
         for (rx_chain = 0; rx_chain < HOST_MAX_CHAINS; rx_chain++) {
              printf("tx_chain_%d : rx_chain_%d = %d\n",
                     tx_chain, rx_chain, info->per_tx_chain_cal_status[tx_chain][rx_chain]);
         }
         printf("\n");
    }
    return 0;
}

int
handle_command_wifi_radar(int argc, char *argv[],
                          const char *ifname, uint8_t link,
                          struct socket_context *sock_ctx)
{
    if (argc == 11 && streq(argv[3], "capture_start")) {
        return handle_wifi_radar_capture_cmd(sock_ctx, ifname, strtoint(argv[4]),
                                             strtoint(argv[5]), strtoint(argv[6]),
                                             strtoint(argv[7]), strtoint(argv[8]),
                                             strtoint(argv[9]), strtoint(argv[10]));
    } else if ((argc == 4) && (streq(argv[3], "capture_stop"))) {
        return handle_wifi_radar_capture_stop_cmd(sock_ctx, ifname);
    } else if ((argc == 4) && (streq(argv[3], "tx_cal"))) {
        return handle_wifi_radar_tx_cal_cmd(sock_ctx, ifname);
    } else if ((argc == 10) && (streq(argv[3], "rx_cal"))) {
        return handle_wifi_radar_rx_cal_cmd(sock_ctx, ifname, strtoint(argv[4]),
                                            strtoint(argv[5]), strtoint(argv[6]),
                                            strtoint(argv[7]), strtoint(argv[8]),
                                            strtoint(argv[9]));
    } else if ((argc == 4) && (streq(argv[3], "get_rx_cal_status"))) {
        return handle_wifi_radar_get_rx_cal_status(sock_ctx, ifname);
    } else {
        printf("Invalid wifi radar command\n");
        fprintf(stderr, "wlanconfig wifiX wifi_radar capture_start <tx_chain_mask> <rx_chain_mask> <num_ltf_tx> <num_skip_ltf_rx> <num_ltf_accumulation> <bw> <capture_interval_ms>\n");
        fprintf(stderr, "wlanconfig wifiX wifi_radar capture_stop\n");
        fprintf(stderr, "wlanconfig wifiX wifi_radar rx_cal <tx_chain_mask> <rx_chain_mask> <num_ltf_tx> <num_skip_ltf_rx> <num_ltf_accumulation> <bw>\n");
        fprintf(stderr, "wlanconfig wifiX wifi_radar tx_cal\n");
        fprintf(stderr, "wlanconfig wifiX wifi_radar get_rx_cal_status\n");
    }
    return 0;
}
#endif

#ifdef WLAN_CFR_ENABLE
int handle_command_cfr(int argc, char *argv[], const char *ifname, uint8_t link,
        struct socket_context *sock_ctx)
{
    if (argc == 8 && streq(argv[3], "start")) {
        return handle_cfr_start_command(sock_ctx, ifname, link,
                argv[4], atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
    } else if (argc == 5 && streq(argv[3], "stop")) {
        return handle_cfr_stop_command(sock_ctx, ifname, link,
                argv[4]);
#ifdef WLAN_ENH_CFR_ENABLE
    } else if (argc == 5 && streq(argv[3], "m_directed_ftm")) {
        return handle_cfr_rcc_mode(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_DIRECT_FTM,
                strtoint(argv[4]));
    } else if (argc == 5 && streq(argv[3], "m_directed_ndpa_ndp")) {
        return handle_cfr_rcc_mode(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_DIRECT_NDPA_NDP,
                strtoint(argv[4]));
    } else if (argc == 5 && streq(argv[3], "m_ta_ra_filter")) {
        return handle_cfr_rcc_mode(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_TA_RA_FLITER,
                strtoint(argv[4]));
    } else if (argc == 5 && streq(argv[3], "m_all_ftm_ack")) {
        return handle_cfr_rcc_mode(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_ALL_FTM_ACK,
                strtoint(argv[4]));
    } else if (argc == 5 && streq(argv[3], "m_ndpa_ndp_all")) {
        return handle_cfr_rcc_mode(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_NDPA_NDP_ALL,
                strtoint(argv[4]));
    } else if (argc == 5 && streq(argv[3], "m_all_pkt")) {
        return handle_cfr_rcc_mode(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_ALL_PKT,
                strtoint(argv[4]));
    } else if (argc == 9 && streq(argv[3], "ta_ra_addr")) {
        return handle_taraAddr_config(sock_ctx, ifname, link,
                strtoint(argv[4]), argv[5], argv[6], argv[7], argv[8]);
    } else if (argc == 7 && streq(argv[3], "bw_nss")) {
        return handle_tara_config(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_BW_NSS,
                strtoint(argv[4]), strtoint(argv[5]), strtoint(argv[6]), 0);
    } else if (argc == 8 && streq(argv[3], "subtype")) {
        return handle_tara_config(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_SUBTYPE, strtoint(argv[4]),
                strtoint(argv[5]), strtoint(argv[6]), strtoint(argv[7]));
    } else if (argc == 5 && streq(argv[3], "capture_dur")) {
        return handle_cfr_rcc_config(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_CAPT_DUR,
                strtoint(argv[4]), 0);
    } else if (argc == 5 && streq(argv[3], "capture_intval")) {
        return handle_cfr_rcc_config(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_CAPT_INTVAL,
                strtoint(argv[4]), 0);
    } else if (argc == 5 && streq(argv[3], "capture_count")) {
        return handle_cfr_rcc_config(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_CAPT_COUNT,
                strtoint(argv[4]), 0);
    } else if (argc == 5 && streq(argv[3], "capture_intervalmode_sel")) {
        return handle_cfr_rcc_config(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_CAPT_INTVAL_MODE_SEL,
                strtoint(argv[4]), 0);
    } else if (argc == 6 && streq(argv[3], "ul_mu_user_mask")) {
        return handle_cfr_rcc_config(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_UL_MU_USER_MASK,
                strtoint(argv[4]), strtoint(argv[5]));
    } else if (argc == 6 && streq(argv[3], "freeze_tlv_delay_cnt")) {
        return handle_cfr_rcc_config(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_FREEZE_TLV_DELAY_CNT,
                strtoint(argv[4]), strtoint(argv[5]));
    } else if (argc == 5 && streq(argv[3], "en_cfg")) {
        return handle_cfr_rcc_config(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_EN_CFG,
                strtoint(argv[4]), 0);
    } else if (argc == 5 && streq(argv[3], "reset_cfg")) {
        return handle_cfr_rcc_config(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RESET_CFG,
                strtoint(argv[4]), 0);
    } else if (argc == 4 && streq(argv[3], "disable_all")) {
        return handle_cfr_rcc_config(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_DISABLE_ALL,
                0, 0);
    } else if (argc == 4 && streq(argv[3], "commit")) {
        return handle_cfr_rcc_config(sock_ctx, ifname, link,
                IEEE80211_WLANCONFIG_CFR_RCC_COMMIT,
                0, 0);
    } else if (argc == 4 && streq(argv[3], "rcc_config_details")) {
            return handle_get_rcc(sock_ctx, ifname, link,
                                  IEEE80211_WLANCONFIG_CFR_GET_CFG);
    } else if (argc == 4 && streq(argv[3], "rcc_debug_counters")) {
            return handle_get_rcc(sock_ctx, ifname, link,
                                  IEEE80211_WLANCONFIG_CFR_RCC_DBG_COUNTERS);
    } else if (argc == 4 && streq(argv[3], "rcc_dump_lut")) {
            return handle_get_rcc(sock_ctx, ifname, link,
                                  IEEE80211_WLANCONFIG_CFR_RCC_DUMP_LUT);
    } else if (argc == 4 && streq(argv[3], "clr_dbg_counters")) {
            return handle_get_rcc(sock_ctx, ifname, link,
                                  IEEE80211_WLANCONFIG_CFR_RCC_CLR_COUNTERS);
#endif
    } else if (argc == 4 && streq(argv[3], "list")) {
        return handle_cfr_list_command(sock_ctx, ifname, link);
    } else {
        printf("Invalid CFR command format: Usage\n");
        printf("\t wlanconfig athX cfr start <peer MAC> <bw> <periodicity> <capture type>\n") ;
        printf("\t wlanconfig athX cfr stop <peer MAC>\n") ;
        printf("\t wlanconfig athX cfr list\n") ;
#ifdef WLAN_ENH_CFR_ENABLE
        printf("\t wlanconfig athx cfr m_ta_ra_filter <enable/disable>\n");
        printf("\t wlanconfig athx cfr m_directed_ndpa_ndp <enable/disable>\n");
        printf("\t wlanconfig athx cfr m_ndpa_ndp_all <enable/disable>\n");
        printf("\t wlanconfig athx cfr disable_all\n");
        printf("\t wlanconfig athx cfr ta_ra_addr <group_id> <ta> <ta_mask> <ra> <ra_mask>\n");
        printf("\t wlanconfig athx cfr bw_nss <group_id> <bw> <nss>\n");
        printf("\t wlanconfig athx cfr subtype <group_id> <mgmt> <ctrl> <data>\n");
        printf("\t wlanconfig athx cfr en_cfg <bitmask>\n");
        printf("\t wlanconfig athx cfr reset_cfg <bitmask>\n");
        printf("\t wlanconfig athx cfr capture_dur <value>\n");
        printf("\t wlanconfig athx cfr capture_intval <value>\n");
        printf("\t wlanconfig athx cfr ul_mu_user_mask <mask_lower_32> <mask_upper_32>\n");
        printf("\t wlanconfig athx cfr freeze_tlv_delay_cnt <freeze_tlv_delay_cnt_en> <threshold>\n");
        printf("\t wlanconfig athx cfr commit\n");
        printf("\t wlanconfig athx cfr rcc_config_details\n");
        printf("\t wlanconfig athx cfr rcc_debug_counters\n");
        printf("\t wlanconfig athx cfr clr_dbg_counters\n");
        printf("\t wlanconfig athx cfr rcc_dump_lut\n");
        printf("\t wlanconfig athx cfr capture_count <count_value>\n");
        printf("\t wlanconfig athx cfr capture_intervalmode_sel <value 0:duration mode 1:count mode>\n");
#endif
    }

    return 0;
}
#endif

#ifdef WLAN_SUPPORT_RX_FLOW_TAG
static int handle_rx_flow_tag_command(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
                                      int opcode, int ipver, char *srcip, char *dstip, int srcport,
                                      int dstport, int protocol, int tag, bool drop, uint8_t ring_id, char *dest_mac, int srvc_id, int tid)
{
    struct ieee80211_wlanconfig config;
    uint16_t inet_domain;
    uint8_t temp[MACSTR_LEN] = {0};
    char macaddr[MACSTR_LEN] = {0};

    if (dest_mac) {
        if (strlcpy(macaddr, dest_mac, sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return 1;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf("Invalid MAC address\n");
            return 1;
        }
      
        memcpy(&config.data.rx_flow_tag_info.flow_info.dstmac, temp, QDF_MAC_ADDR_SIZE);
    } 

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_RX_FLOW_TAG_OP;

    if (ipver != 0 && ipver != 1) {
        printf("Invalid IPversion\n");
        return 1;
    }
    if (protocol != 0 && protocol != 1) {
        printf("Invalid protocol\n");
        return 1;
    }

    config.data.rx_flow_tag_info.op_code = opcode;

    config.data.rx_flow_tag_info.ip_ver = ipver;
    inet_domain = (ipver == 0)? AF_INET: AF_INET6;
    if (AF_INET == inet_domain) {
        if (!inet_pton(inet_domain, srcip, (struct in_addr *) &config.data.rx_flow_tag_info.flow_tuple.source_ip[3])) {
            printf("Could not convert src IP address to network order\n");
            return 1;
        }

        if (!inet_pton(inet_domain, dstip, (struct in_addr *) &config.data.rx_flow_tag_info.flow_tuple.dest_ip[3])) {
            printf("Could not convert dst IP address to network order\n");
            return 1;
        }
    } else {
        if (!inet_pton(inet_domain, srcip, (struct in_addr *) config.data.rx_flow_tag_info.flow_tuple.source_ip)) {
            printf("Could not convert src IP address to network order\n");
            return 1;
        }

        if (!inet_pton(inet_domain, dstip, (struct in_addr *) config.data.rx_flow_tag_info.flow_tuple.dest_ip)) {
            printf("Could not convert dst IP address to network order\n");
            return 1;
        }
    }

    config.data.rx_flow_tag_info.flow_tuple.source_port = srcport;
    config.data.rx_flow_tag_info.flow_tuple.dest_port = dstport;
    config.data.rx_flow_tag_info.flow_tuple.protocol = protocol;
    config.data.rx_flow_tag_info.flow_metadata = tag;

    config.data.rx_flow_tag_info.flow_info.svc_id = srvc_id;
    config.data.rx_flow_tag_info.flow_info.tid = tid;
    config.data.rx_flow_tag_info.flow_info.drop_flow = drop;
    config.data.rx_flow_tag_info.flow_info.ring_id = ring_id;

    send_command(sock_ctx, ifname, link, &config, sizeof(struct ieee80211_wlanconfig),
           NULL, QCA_NL80211_VENDOR_SUBCMD_RX_FLOW_TAG_OP, IEEE80211_IOCTL_CONFIG_GENERIC);
    return 0;
}

int handle_command_flow_tag(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx)
{
    if (streq(argv[3], "0")) {
        if (argc != 13 && argc != 16)
            goto flow_tag_usage;
    }

    if (argc < 10 && (streq(argv[3], "1") || streq(argv[3], "2")))
        goto flow_tag_usage;

    if (streq(argv[3], "0")) {
        if (argc == 13)
            return handle_rx_flow_tag_command(sock_ctx, ifname, link,
                 atoi(argv[3]), atoi(argv[4]), argv[5], argv[6],
                 atoi(argv[7]), atoi(argv[8]), atoi(argv[9]), atoi(argv[10]),
                 atoi(argv[11]), atoi(argv[12]), NULL, WLAN_FLOW_INVALID_SVC_ID, 0);
        else
            return handle_rx_flow_tag_command(sock_ctx, ifname, link,
                 atoi(argv[3]), atoi(argv[4]), argv[5], argv[6],
                 atoi(argv[7]), atoi(argv[8]), atoi(argv[9]), atoi(argv[10]),
                 atoi(argv[11]), atoi(argv[12]), argv[13], atoi(argv[14]), atoi(argv[15]));
    } else if (streq(argv[3], "1") || streq(argv[3], "2")) {
        return handle_rx_flow_tag_command(sock_ctx, ifname, link,
                   atoi(argv[3]), atoi(argv[4]), argv[5], argv[6],
                   atoi(argv[7]), atoi(argv[8]), atoi(argv[9]), 0, 0, 0, NULL, WLAN_FLOW_INVALID_SVC_ID,
                   0);
    }

flow_tag_usage:
    printf("Invalid command format: Usage \n");
    printf("\t wlanconfig wifiX rx_flow_tag_op <opcode: 0-Add, 1-Del, 2-Dump> <ipversion 0-IPv4, 1-IPv6> <src ip> <dest ip> <src port> <dst port> <protocol: 0-TCP 1-UDP> <16-bit tag> <drop_flow: 0-keep 1-drop> <ring_id: <0-4>> [<dest mac> <service class> <tid>]\n") ;
    return 0;
}
#endif /* WLAN_SUPPORT_RX_FLOW_TAG */

static int handle_mu_snif(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
    IEEE80211_MU_SNIF_MODE mode, int argc, char *argv[])
{
    int i, j;
    struct ieee80211_wlanconfig config = {0};

    /* zero out configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));

    config.cmdtype = IEEE80211_WLANCONFIG_MU_SNIF;
    config.data.mu_snif.mode = mode;

    if (mode == IEEE80211_MU_SNIF_USER || mode == IEEE80211_MU_SNIF_WILD) {
        if (argc > 4) {
            config.data.mu_snif.num_user = atoi(argv[4]);

            if (config.data.mu_snif.num_user < 1 ||
                config.data.mu_snif.num_user > 4) {
                   printf("Invalid max user(1-4), %s\n", argv[4]);
                return -1;
            }
            if (argc > 5) {
                for(i=0, j=5; i < MU_SNIF_MAX_AID && j < argc ; i++, j++) {
                    config.data.mu_snif.aid[i] = atoi(argv[j]);
	        }
                config.data.mu_snif.num_aid = i;
            } else
                config.data.mu_snif.num_aid = 0;
        } else
            config.data.mu_snif.num_user = MU_SNIF_MAX_AID;
    }

    send_command(sock_ctx, ifname, link, &config, sizeof(struct ieee80211_wlanconfig),
        NULL, QCA_NL80211_VENDOR_SUBCMD_MU_SNIF, IEEE80211_IOCTL_CONFIG_GENERIC);
    return 0;
}

int handle_command_mu_snif(int argc, char *argv[], const char *ifname, uint8_t link, struct socket_context *sock_ctx)
{

    if((streq(argv[3], "dis") && argc == 4))
        return handle_mu_snif (sock_ctx, ifname, link, IEEE80211_MU_SNIF_DIS,
                argc , argv);
    else if(streq(argv[3], "match") && (argc > 4 && argc <=9))
        return handle_mu_snif (sock_ctx, ifname, link, IEEE80211_MU_SNIF_USER,
                argc , argv);
    else if(streq(argv[3], "wildcard") && (argc >= 4 && argc <=9))
        return handle_mu_snif (sock_ctx, ifname, link, IEEE80211_MU_SNIF_WILD,
                argc , argv);
    else
        errx(1, "Invalid mu snif command , check wlanconfig help");
}

#if QCA_SUPPORT_PEER_ISOLATION
static int handle_peer_isolation(struct socket_context *sock_ctx, const char *ifname, uint8_t link,
    IEEE80211_WLANCONFIG_CMDTYPE cmdtype, int argc, char *argv[])
{
    int i;
    struct ieee80211_wlanconfig config = {0};
    struct ieee80211_wlanconfig *config_ptr = NULL;
    struct ieee80211_wlanconfig_isolation_list *isl = NULL;
    char *data = NULL;
    int mac_cnt, len = 0;
    char macaddr[MACSTR_LEN + 1] = {0};
    uint8_t temp[MACSTR_LEN + 1] = {0};
    char nullmac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


    /* zero out configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));

    config_ptr = &config;
    len = sizeof(struct ieee80211_wlanconfig);

    if (cmdtype == IEEE80211_WLANCONFIG_PEER_ISOLATION_ADDR_ADD ||
        cmdtype == IEEE80211_WLANCONFIG_PEER_ISOLATION_ADDR_DEL) {

        if (strlcpy(macaddr, argv[4] , sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid MAC address (format: xx:xx:xx:xx:xx:xx), %s\n", argv[4]);
            return -1;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf( "Invalid MAC address\n");
            return -1;
        }

        memcpy(config.data.isolation.mac, temp, IEEE80211_ADDR_LEN);
    }
    else if (cmdtype == IEEE80211_WLANCONFIG_PEER_ISOLATION_ADDR_LIST) {
        /* Get the number of clients in the isolation list */
        len = sizeof(struct ieee80211_wlanconfig)
              + sizeof(struct ieee80211_wlanconfig_isolation_list);
        data = (char *)malloc(len);
        if (data == NULL) {
            printf("Out of memory\n");
            return -1;
        }
        memset(data, 0, len);
        config_ptr = (struct ieee80211_wlanconfig *)data;
        config_ptr->cmdtype = IEEE80211_WLANCONFIG_PEER_ISOLATION_NUM_CLIENT;
        send_command(sock_ctx, ifname, link, config_ptr, len,
                     NULL, QCA_NL80211_VENDOR_SUBCMD_PEER_ISOLATION,
                     IEEE80211_IOCTL_CONFIG_GENERIC);
        isl = (struct ieee80211_wlanconfig_isolation_list *)(config_ptr + 1);
        mac_cnt = isl->mac_cnt;
        free(data);

        /* Check any client in the isolation list, if not return */
        printf("Num clients in isolation list %d\n", mac_cnt);
        if (!mac_cnt) {
            return 0;
        }

        /* Calculate the size required and allocate it */
        len = sizeof(struct ieee80211_wlanconfig)
              + sizeof(struct ieee80211_wlanconfig_isolation_list)
              + (IEEE80211_ADDR_LEN * mac_cnt);
        data = (char *)malloc(len);
        if (data == NULL) {
            printf("Out of memory\n");
            return -1;
        }
        memset(data, 0, len);
        config_ptr = (struct ieee80211_wlanconfig *)data;
        isl = (struct ieee80211_wlanconfig_isolation_list *)(config_ptr + 1);
        isl->mac_cnt = mac_cnt;
    }
    else if (cmdtype == IEEE80211_WLANCONFIG_PEER_ISOLATION_FLUSH_LIST) {
        printf("Flushing peer isolation list\n");
    }
    else {
        printf("Invalid command\n");
        return -1;
    }

    /* fill up cmd */
    config_ptr->cmdtype = cmdtype;
    send_command(sock_ctx, ifname, link, config_ptr, len,
                 NULL, QCA_NL80211_VENDOR_SUBCMD_PEER_ISOLATION,
                 IEEE80211_IOCTL_CONFIG_GENERIC);

    if (cmdtype == IEEE80211_WLANCONFIG_PEER_ISOLATION_ADDR_LIST) {
        u_int8_t *mac;
        isl = (struct ieee80211_wlanconfig_isolation_list *)(config_ptr + 1);
        printf("wlan peer isolaton list count: %d\n", isl->mac_cnt);

        mac = &isl->buf[0];
        for (i = 0; i < isl->mac_cnt; i++) {
            if (memcmp(mac, nullmac, IEEE80211_ADDR_LEN) != 0) {
                printf(" [%d]\t %02x:%02x:%02x:%02x:%02x:%02x ", (i + 1),
                       mac[0], mac[1],
                       mac[2], mac[3],
                       mac[4], mac[5]);
                printf("\n");
            }
            mac += IEEE80211_ADDR_LEN;
        }
        free(data);
    }
    return 0;
}

int handle_command_peer_isolation(int argc, char *argv[], const char *ifname, uint8_t link,
                                  struct socket_context *sock_ctx)
{
    if(argc == 5 && streq(argv[3], "add")){
        return handle_peer_isolation(sock_ctx, ifname, link,
                                     IEEE80211_WLANCONFIG_PEER_ISOLATION_ADDR_ADD,
                                     argc , argv);
    } else if(argc == 5 && streq(argv[3], "del")){
        return handle_peer_isolation(sock_ctx, ifname, link,
                                     IEEE80211_WLANCONFIG_PEER_ISOLATION_ADDR_DEL,
                                     argc , argv);
    }else if(argc == 4 && streq(argv[3], "flush")){
        return handle_peer_isolation(sock_ctx, ifname, link,
                                     IEEE80211_WLANCONFIG_PEER_ISOLATION_FLUSH_LIST,
                                     argc , argv);
    } else if(argc == 4 && streq(argv[3], "list")){
        handle_peer_isolation(sock_ctx, ifname, link,
                              IEEE80211_WLANCONFIG_PEER_ISOLATION_ADDR_LIST,
                              argc , argv);
        return 0;
    } else {
        errx(1, "Invalid isolation command , check wlanconfig help");
    }
}
#endif

#if MESH_MODE_SUPPORT
#ifdef WLAN_FEATURE_11BE_MLO
static int handle_mesh_dellocalpeer(struct socket_context *sock_ctx,
                                    const char *ifname, uint8_t link, char *peer_mld_mac)
{
    struct ieee80211_wlanconfig config;
    uint8_t temp_mac[MACSTR_LEN];
    char temp[MACSTR_LEN];

    if (peer_mld_mac != NULL) {
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(temp, peer_mld_mac, sizeof(temp)) >= sizeof(temp)) {
            printf("Invalid MLD MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        memset(temp_mac, '\0', sizeof(temp_mac));
        if (ether_string2mac(temp_mac, temp) != 0) {
            printf("Invalid MLD MAC address\n");
            return -1;
        }
    } else {
        printf("MLD MAC addr passed is NULL\n");
        return -1;
    }

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_MESH_DELLOCALPEER;
    memcpy(config.data.mesh.peer_mld_mac, temp_mac, QDF_MAC_ADDR_SIZE);

    /* call NL80211 Vendor commnd frame work */
    if (send_command(sock_ctx, ifname, link, &config,
                     sizeof(struct ieee80211_wlanconfig),
                     NULL, QCA_NL80211_VENDOR_SUBCMD_MESH_PARAMS,
                     IEEE80211_IOCTL_CONFIG_GENERIC) < 0) {
        printf("%s(): failed to send mesh command\n", __func__);
        return -1;
    }
    return 0;
}

static int handle_mesh_allowdata(struct socket_context *sock_ctx,
                                 const char *ifname, uint8_t link, char *peer_mld_mac)
{
    struct ieee80211_wlanconfig config;
    uint8_t temp_mac[MACSTR_LEN];
    char temp[MACSTR_LEN];

    if (peer_mld_mac != NULL) {
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(temp, peer_mld_mac, sizeof(temp)) >= sizeof(temp)) {
            printf("Invalid MLD MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        memset(temp_mac, '\0', sizeof(temp_mac));
        if (ether_string2mac(temp_mac, temp) != 0) {
            printf("Invalid MLD MAC address\n");
            return -1;
        }
    } else {
        printf("MLD MAC addr passed is NULL\n");
        return -1;
    }

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_MESH_ALLOWDATA;
    memcpy(config.data.mesh.peer_mld_mac, temp_mac, QDF_MAC_ADDR_SIZE);

    /* call NL80211 Vendor commnd frame work */
    if (send_command(sock_ctx, ifname, link, &config,
                     sizeof(struct ieee80211_wlanconfig),
                     NULL, QCA_NL80211_VENDOR_SUBCMD_MESH_PARAMS,
                     IEEE80211_IOCTL_CONFIG_GENERIC) < 0) {
        printf("%s(): failed to send mesh command\n", __func__);
        return -1;
    }
    return 0;
}

static int handle_mesh_addlocalpeer(struct socket_context *sock_ctx,
                                    const char *ifname, uint8_t link, char *peer_mld_mac,
                                    u_int64_t num_links, char *peer_link_mac,
                                    u_int64_t caps, u_int64_t puncture_bitmap,
                                    u_int64_t link_id, char *partner_vap1_mac,
                                    char *pp1_link_mac, u_int64_t pp1_caps,
                                    u_int64_t pp1_puncture_bitmap, u_int64_t pp1_link_id,
                                    char *partner_vap2_mac, char *pp2_link_mac,
                                    u_int64_t pp2_caps, u_int64_t pp2_puncture_bitmap,
                                    u_int64_t pp2_link_id)
{
    struct ieee80211_wlanconfig config;
    uint8_t temp_mac[MACSTR_LEN];
    char temp[MACSTR_LEN];

    memset(temp_mac, '\0', sizeof(temp_mac));
    if (peer_mld_mac != NULL) {
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(temp, peer_mld_mac, sizeof(temp)) >= sizeof(temp)) {
            printf("Invalid MLD MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp_mac, temp) != 0) {
            printf("Invalid MLD MAC address\n");
            return -1;
        }
    }

    if (num_links > 3) {
        printf("Max number of links supported are 3, returning");
        return -1;
    }

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_MESH_ADDLOCALPEER;
    config.data.mesh.num_links = (u_int16_t)num_links;
    if (peer_mld_mac != NULL)
        memcpy(config.data.mesh.peer_mld_mac, temp_mac, QDF_MAC_ADDR_SIZE);
    memset(temp_mac, '\0', sizeof(temp_mac));

    if (peer_link_mac != NULL) {
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(temp, peer_link_mac, sizeof(temp)) >= sizeof(temp)) {
            printf("Invalid link MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp_mac, temp) != 0) {
            printf("Invalid link MAC address\n");
            return -1;
        }
    }
    memcpy(config.data.mesh.peers[0].peer_link_mac, temp_mac, QDF_MAC_ADDR_SIZE);
    config.data.mesh.peers[0].caps = caps;
    config.data.mesh.peers[0].puncture_bitmap = (u_int16_t)puncture_bitmap;
    config.data.mesh.peers[0].link_id = (u_int8_t)link_id;
    memset(temp_mac, '\0', sizeof(temp_mac));

    if (partner_vap1_mac != NULL) {
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(temp, partner_vap1_mac, sizeof(temp)) >= sizeof(temp)) {
            printf("Invalid VAP MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp_mac, temp) != 0) {
            printf("Invalid VAP MAC address\n");
            return -1;
        }
    }
    memcpy(config.data.mesh.peers[1].vap_mac, temp_mac, QDF_MAC_ADDR_SIZE);
    memset(temp_mac, '\0', sizeof(temp_mac));

    if (pp1_link_mac != NULL) {
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(temp, pp1_link_mac, sizeof(temp)) >= sizeof(temp)) {
            printf("Invalid link MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp_mac, temp) != 0) {
            printf("Invalid link MAC address\n");
            return -1;
        }
    }
    memcpy(config.data.mesh.peers[1].peer_link_mac, temp_mac, QDF_MAC_ADDR_SIZE);
    config.data.mesh.peers[1].caps = pp1_caps;
    config.data.mesh.peers[1].puncture_bitmap = (u_int16_t)pp1_puncture_bitmap;
    config.data.mesh.peers[1].link_id = (u_int8_t)pp1_link_id;
    memset(temp_mac, '\0', sizeof(temp_mac));

    if (partner_vap2_mac != NULL) {
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(temp, partner_vap2_mac, sizeof(temp)) >= sizeof(temp)) {
            printf("Invalid VAP MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp_mac, temp) != 0) {
            printf("Invalid VAP MAC address\n");
            return -1;
        }
    }
    memcpy(config.data.mesh.peers[2].vap_mac, temp_mac, QDF_MAC_ADDR_SIZE);
    memset(temp_mac, '\0', sizeof(temp_mac));

    if (pp2_link_mac != NULL) {
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(temp, pp2_link_mac, sizeof(temp)) >= sizeof(temp)) {
            printf("Invalid link MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp_mac, temp) != 0) {
            printf("Invalid link MAC address\n");
            return -1;
        }
    }
    memcpy(config.data.mesh.peers[2].peer_link_mac, temp_mac, QDF_MAC_ADDR_SIZE);
    config.data.mesh.peers[2].caps = pp2_caps;
    config.data.mesh.peers[2].puncture_bitmap = (u_int16_t)pp2_puncture_bitmap;
    config.data.mesh.peers[2].link_id = (u_int8_t)pp2_link_id;

    /* call NL80211 Vendor commnd frame work */
    if (send_command(sock_ctx, ifname, link, &config,
                     sizeof(struct ieee80211_wlanconfig),
                     NULL, QCA_NL80211_VENDOR_SUBCMD_MESH_PARAMS,
                     IEEE80211_IOCTL_CONFIG_GENERIC) < 0) {
        printf("%s(): failed to send mesh command\n", __func__);
        return -1;
    }
    return 0;
}

int handle_command_mesh(int argc, char *argv[], const char *ifname, uint8_t link,
                        struct socket_context *sock_ctx)
{
    if (((argc == 15) || (argc == 20) || (argc == 10) || (argc == 7)) && streq(argv[3], "addlocalpeer")) {
        if (argc == 10) {
            return handle_mesh_addlocalpeer(sock_ctx, ifname, link,
                                            argv[4],
                                            strtoul(argv[5], NULL, 0),
                                            argv[6],
                                            strtoul(argv[7], NULL, 0),
                                            strtoul(argv[8], NULL, 0),
                                            strtoul(argv[9], NULL, 0),
                                            NULL, NULL, 0, 0, 0,
                                            NULL, NULL, 0, 0, 0);
        } else if (argc == 15) {
            return handle_mesh_addlocalpeer(sock_ctx, ifname, link,
                                            argv[4],
                                            strtoul(argv[5], NULL, 0),
                                            argv[6],
                                            strtoul(argv[7], NULL, 0),
                                            strtoul(argv[8], NULL, 0),
                                            strtoul(argv[9], NULL, 0),
                                            argv[10], argv[11],
                                            strtoul(argv[12], NULL, 0),
                                            strtoul(argv[13], NULL, 0),
                                            strtoul(argv[14], NULL, 0),
                                            NULL, NULL, 0, 0, 0);
        } else if (argc == 7) {
            return handle_mesh_addlocalpeer(sock_ctx, ifname, link,
                                            NULL, 0, argv[5],
                                            strtoul(argv[4], NULL, 0),
                                            strtoul(argv[6], NULL, 0), 0,
                                            NULL, NULL, 0, 0, 0,
                                            NULL, NULL, 0, 0, 0);
        } else {
            return handle_mesh_addlocalpeer(sock_ctx, ifname, link,
                                            argv[4],
                                            strtoul(argv[5], NULL, 0),
                                            argv[6],
                                            strtoul(argv[7], NULL, 0),
                                            strtoul(argv[8], NULL, 0),
                                            strtoul(argv[9], NULL, 0),
                                            argv[10], argv[11],
                                            strtoul(argv[12], NULL, 0),
                                            strtoul(argv[13], NULL, 0),
                                            strtoul(argv[14], NULL, 0),
                                            argv[15], argv[16],
                                            strtoul(argv[17], NULL, 0),
                                            strtoul(argv[18], NULL, 0),
                                            strtoul(argv[19], NULL, 0));
        }
    } else if (argc == 5 && streq(argv[3], "allowdata")) {
        return handle_mesh_allowdata(sock_ctx, ifname, link, argv[4]);
    } else if (argc == 5 && streq(argv[3], "dellocalpeer")) {
        return handle_mesh_dellocalpeer(sock_ctx, ifname, link, argv[4]);
    } else {
        errx(1, "invalid MESH command");
    }
    return 0;
}
#endif
#endif

#ifdef WLAN_FEATURE_11BE
/* parse the below inputs
 * [dir <direction> default_mapping <default mapping> link_mapping_size <size>
 * num_tids <num_tids> [tid_num <TID_num> link_map <link mapping of the TID>]]
 * [dir <direction> ...]
 */
int parse_t2lm_arguments(int argc, char *argv[], uint8_t index,
                         struct ieee80211_wlanconfig_t2lm *t2lm)
{
    uint8_t tid = 0;
    uint8_t dir = 0;

    if (streq(argv[index], "dir")) {
        dir = atoi(argv[++index]);
        if (dir > T2LM_DIRECTION_BIDI) {
            fprintf(stderr, "%s:%d invalid dir value\n", __func__, __LINE__);
            return -1;
        }

        /* Do not set BIDI T2LM mapping when DL or UL T2LM mapping is provided.
         * For DL, the value of direction field is 0. Hence, use num_tids or
         * default_mapping to check if DL T2LM mapping is provided.
         */
        if (dir == T2LM_DIRECTION_BIDI) {
            if ((t2lm->t2lm_direction[T2LM_DIRECTION_DL].direction !=
                        T2LM_DIRECTION_INVALID) ||
                (t2lm->t2lm_direction[T2LM_DIRECTION_UL].direction !=
                        T2LM_DIRECTION_INVALID)) {
                fprintf(stderr, "%s:%d Invalid config, either set UL, DL T2LM or BIDI T2LM\n",
                        __func__, __LINE__);
                return -1;
            }
        } else {
            if (t2lm->t2lm_direction[T2LM_DIRECTION_BIDI].direction !=
                    T2LM_DIRECTION_INVALID) {
                fprintf(stderr, "%s:%d Invalid config, either set UL, DL T2LM or BIDI T2LM\n",
                        __func__, __LINE__);
                return -1;
            }
        }
        t2lm->t2lm_direction[dir].direction = dir;
        index++;
    } else {
        return -1;
    }

    if (streq(argv[index], "default_mapping")) {
        t2lm->t2lm_direction[dir].default_mapping = atoi(argv[++index]);
        index++;
    } else {
        return -1;
    }

    if (streq(argv[index], "link_mapping_size")) {
        t2lm->t2lm_direction[dir].link_mapping_size = atoi(argv[++index]);
        index++;
    } else {
        return -1;
    }

    if (streq(argv[index], "num_tids")) {
        t2lm->t2lm_direction[dir].num_tids = atoi(argv[++index]);
        index++;
    } else {
        return -1;
    }

    /* Check the remaining arguments */
    if ((argc - index) < (t2lm->t2lm_direction[dir].num_tids * T2LM_MAP_PARAMS)) {
        fprintf(stderr, "%s:%d Invalid number of arguments\n",
                __func__, __LINE__);
        return -1;
    }

    for (tid = 0; (tid < t2lm->t2lm_direction[dir].num_tids); tid++) {
        if (argv[index])
            t2lm->t2lm_direction[dir].map_tid_to_links[tid].tid_num = atoi(argv[index++]);
        if (argv[index])
            t2lm->t2lm_direction[dir].map_tid_to_links[tid].link_map = strtoint(argv[index++]);
    }

    if (index < argc) {
        return parse_t2lm_arguments(argc, argv, index, t2lm);
    }

    return 0;
}

int handle_command_t2lm_request(int argc, char *argv[], const char *ifname, uint8_t link,
                                struct socket_context *sock_ctx)
{
    struct ieee80211_wlanconfig config;
    struct ieee80211_wlanconfig_t2lm *t2lm = &(config.data.t2lm);
    uint8_t index = 0;
    int retval = -1;
    uint8_t dir = 0;

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_T2LM_REQUEST;

    for (dir = 0; dir < T2LM_DIRECTION_MAX; dir++) {
        t2lm->t2lm_direction[dir].direction = T2LM_DIRECTION_INVALID;
    }

    if (argc < 16) {
        fprintf(stderr, "Invalid number of arguments, argc:%d\n", argc);
        goto cmd_usage;
    }

    if (streq(argv[4], "assoc_frm")) {
        t2lm->assoc_frm = atoi(argv[5]);
    } else {
        goto cmd_usage;
    }

    if (streq(argv[6], "peer_mld_mac")) {
        retval = get_mac_addr(argv[7], (char *)&t2lm->peer_mld_mac);
        if (retval) {
            fprintf(stderr, "Invalid macaddr\n");
            goto cmd_usage;
        }
    } else {
        goto cmd_usage;
    }

    index = 8;
    retval = parse_t2lm_arguments(argc, argv, index, t2lm);
    if (retval) {
        fprintf(stderr, "failed to parse the T2LM arguments\n");
        goto cmd_usage;
    }

    return send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_T2LM,
            IEEE80211_IOCTL_CONFIG_GENERIC);

cmd_usage:
    fprintf(stderr, "Usage: wlanconfig athX t2lm_test_cmd request assoc_frm <1/0> peer_mld_mac <peer MLD macaddr> dir <direction> default_mapping <default mapping> link_mapping_size <size> num_tids <num_tids> <TID0> <TID0_link_map> <TID1> <TID1_link_map> [dir <direction> ...]\n");
    return retval;
}

int handle_command_t2lm_response(int argc, char *argv[], const char *ifname,
                                 uint8_t link, struct socket_context *sock_ctx)
{
    struct ieee80211_wlanconfig config;
    struct ieee80211_wlanconfig_t2lm *t2lm = &(config.data.t2lm);
    int retval = -1;
    uint8_t index = 0;
    uint8_t dir = 0;

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_T2LM_RESPONSE;

    for (dir = 0; dir < T2LM_DIRECTION_MAX; dir++) {
        t2lm->t2lm_direction[dir].direction = T2LM_DIRECTION_INVALID;
    }

    if (argc < 12) {
        fprintf(stderr, "Invalid number of arguments, argc:%d\n", argc);
        goto cmd_usage;
    }

    if (streq(argv[4], "assoc_frm")) {
        t2lm->assoc_frm = atoi(argv[5]);
    } else {
        goto cmd_usage;
    }

    if (streq(argv[6], "peer_mld_mac")) {
        retval = get_mac_addr(argv[7], (char *)&t2lm->peer_mld_mac);
        if (retval) {
            fprintf(stderr, "Invalid macaddr\n");
            goto cmd_usage;
        }
    } else {
        goto cmd_usage;
    }

    if (streq(argv[8], "status")) {
        t2lm->status = atoi(argv[9]);
    } else {
        goto cmd_usage;
    }

    if (streq(argv[10], "unsolicited_t2lm_resp")) {
        t2lm->unsolicited_t2lm_resp = atoi(argv[11]);
    } else {
        goto cmd_usage;
    }

    if (t2lm->status == T2LM_STATUS_PREFERRED_TID_TO_LINK_MAPPING) {
        if (argc < 18) {
            fprintf(stderr, "Invalid arguments provided for status:%d\n",
                    t2lm->status);
            goto cmd_usage;
        }

        index = 12;
        retval = parse_t2lm_arguments(argc, argv, index, t2lm);
        if (retval) {
            fprintf(stderr, "failed to parse the T2LM arguments\n");
            goto cmd_usage;
        }
    }

    return send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_T2LM,
            IEEE80211_IOCTL_CONFIG_GENERIC);
cmd_usage:
    fprintf(stderr, "Usage: wlanconfig athX t2lm_test_cmd response assoc_frm <1/0> peer_mld_mac <peer MLD macaddr> status <status> unsolicited_t2lm_resp <unsolicited T2LM response> [dir <direction> default_mapping <default mapping> link_mapping_size <size> num_tids <num_tids> <TID0> <TID0_link_map> <TID1> <TID1_link_map> [dir <direction> ...]\n");
    return retval;

}

int handle_command_t2lm_teardown(int argc, char *argv[], const char *ifname,
                                 uint8_t link, struct socket_context *sock_ctx)
{
    struct ieee80211_wlanconfig config;
    struct ieee80211_wlanconfig_t2lm *t2lm = &(config.data.t2lm);
    int retval = -1;
    uint8_t dir = 0;

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_T2LM_TEARDOWN;

    for (dir = 0; dir < T2LM_DIRECTION_MAX; dir++) {
        t2lm->t2lm_direction[dir].direction = T2LM_DIRECTION_INVALID;
    }

    if (argc < 6) {
        fprintf(stderr, "Invalid number of arguments, argc:%d\n", argc);
        goto cmd_usage;
    }

    if (streq(argv[4], "peer_mld_mac")) {
        retval = get_mac_addr(argv[5], (char *)&t2lm->peer_mld_mac);
        if (retval) {
            fprintf(stderr, "Invalid macaddr\n");
            goto cmd_usage;
        }
    } else {
        goto cmd_usage;
    }

    return send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_T2LM,
            IEEE80211_IOCTL_CONFIG_GENERIC);

cmd_usage:
    fprintf(stderr, "Usage: wlanconfig athX t2lm_test_cmd teardown peer_mld_mac <peer MLD macaddr>\n");
    return retval;
}

int handle_command_epcs_en_dis(int argc, char *argv[], const char *ifname,
                                 uint8_t link, struct socket_context *sock_ctx)
{
    struct ieee80211_wlanconfig config;
    struct ieee80211_wlanconfig_epcs *epcs = &(config.data.epcs);
    int retval = -1;

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_EPCS;

    epcs->authorize = 0;
    epcs->deauthorize = 0;
    epcs->set_edca = 0;
    epcs->set_muedca = 0;
    epcs->get_edca = 0;
    epcs->get_muedca = 0;

    if (streq(argv[3], "enable")) {
        if (argc < 6) {
            fprintf(stderr, "Invalid number of arguments, argc:%d\n", argc);
            goto cmd_usage;
        }
        epcs->enable = 1;
    } else if (streq(argv[3], "disable")) {
        if (argc < 6) {
            fprintf(stderr, "Invalid number of arguments, argc:%d\n", argc);
            goto cmd_usage;
        }
        epcs->enable = 0;
    } else if (streq(argv[3], "authorize")) {
        if (argc < 6) {
            fprintf(stderr, "Invalid number of arguments, argc:%d\n", argc);
            goto cmd_usage;
        }
        epcs->authorize = 1;
    } else if (streq(argv[3], "deauthorize")) {
        if (argc < 6) {
            fprintf(stderr, "Invalid number of arguments, argc:%d\n", argc);
            goto cmd_usage;
        }
        epcs->deauthorize = 1;
    } else if (streq(argv[3], "set_edca_param")) {
        if (argc < 13) {
            fprintf(stderr, "Invalid number of arguments, argc:%d\n", argc);
            goto cmd_usage;
        }
        epcs->set_edca = 1;
    } else if (streq(argv[3], "get_edca_param")) {
        if (argc < 4) {
            fprintf(stderr, "Invalid number of arguments, argc:%d\n", argc);
            goto cmd_usage;
        }
        epcs->get_edca = 1;
        goto end;
    } else if (streq(argv[3], "set_muedca_param")) {
        if (argc < 13) {
            fprintf(stderr, "Invalid number of arguments, argc:%d\n", argc);
            goto cmd_usage;
        }
        epcs->set_muedca = 1;
    } else if (streq(argv[3], "get_muedca_param")) {
        if (argc < 4) {
            fprintf(stderr, "Invalid number of arguments, argc:%d\n", argc);
            goto cmd_usage;
        }
        epcs->get_muedca = 1;
        goto end;
    } else {
        goto cmd_usage;
    }

    if (streq(argv[4], "peer_mld_mac")) {
        retval = get_mac_addr(argv[5], (char *)&epcs->peer_mld_mac);
        if (retval) {
            fprintf(stderr, "Invalid macaddr\n");
            goto cmd_usage;
        }
    }

    if (streq(argv[3], "set_edca_param")) {
        if (streq(argv[4], "BE")) {
            epcs->edca_param.aci = 0;
        } else if (streq(argv[4], "BK")) {
            epcs->edca_param.aci = 1;
        } else if (streq(argv[4], "VI")) {
            epcs->edca_param.aci = 2;
        } else if (streq(argv[4], "VO")) {
            epcs->edca_param.aci = 3;
        } else {
            goto cmd_usage;
        }

        if (streq(argv[5], "aifsn")) {
            epcs->edca_param.wmep_aifsn = atoi(argv[6]);
        } else {
            goto cmd_usage;
        }

        if (streq(argv[7], "cwmin")) {
            epcs->edca_param.wmep_logcwmin = atoi(argv[8]);
        } else {
            goto cmd_usage;
        }

        if (streq(argv[9], "cwmax")) {
            epcs->edca_param.wmep_logcwmax = atoi(argv[10]);
        } else {
            goto cmd_usage;
        }

        if (streq(argv[11], "txopLimit")) {
            epcs->edca_param.wmep_txopLimit = atoi(argv[12]);
        } else {
            goto cmd_usage;
        }
    }

    if (streq(argv[3], "set_muedca_param")) {
        if (streq(argv[4], "BE")) {
            epcs->muedca_param.aci = 0;
        } else if (streq(argv[4], "BK")) {
            epcs->muedca_param.aci = 1;
        } else if (streq(argv[4], "VI")) {
            epcs->muedca_param.aci = 2;
        } else if (streq(argv[4], "VO")) {
            epcs->muedca_param.aci = 3;
        } else {
            goto cmd_usage;
        }

        if (streq(argv[5], "aifsn")) {
            epcs->muedca_param.muedca_aifsn = atoi(argv[6]);
        } else {
            goto cmd_usage;
        }

        if (streq(argv[7], "cwmin")) {
            epcs->muedca_param.muedca_ecwmin = atoi(argv[8]);
        } else {
            goto cmd_usage;
        }

        if (streq(argv[9], "cwmax")) {
            epcs->muedca_param.muedca_ecwmax = atoi(argv[10]);
        } else {
            goto cmd_usage;
        }

        if (streq(argv[11], "timer")) {
            epcs->muedca_param.muedca_timer = atoi(argv[12]);
        } else {
            goto cmd_usage;
        }
    }

end:
    return send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_EPCS,
            IEEE80211_IOCTL_CONFIG_GENERIC);

cmd_usage:
    fprintf(stderr, "Usage: wlanconfig athX epcs_test_cmd enable/disable/authorize/deauthorize peer_mld_mac <peer MLD macaddr>\n");
    fprintf(stderr, "Usage: wlanconfig athX epcs_test_cmd set_edca_param BE/BK/VI/VO aifsn <aifsn> cwmin <cwmin> cwmax <cwmax> txoplimit <txoplimit>\n");
    fprintf(stderr, "Usage: wlanconfig athX epcs_test_cmd get_edca_param\n");
    fprintf(stderr, "Usage: wlanconfig athX epcs_test_cmd set_muedca_param BE/BK/VI/VO aifsn <aifsn> cwmin <cwmin> cwmax <cwmax> timer <timer>\n");
    fprintf(stderr, "Usage: wlanconfig athX epcs_test_cmd get_muedca_param\n");

    return retval;
}

int handle_command_t2lm(int argc, char *argv[], const char *ifname, uint8_t link,
                        struct socket_context *sock_ctx)
{
    if (argc <= 3)
        goto err;

    if (streq(argv[3], "request")) {
        return handle_command_t2lm_request(argc, argv, ifname, link, sock_ctx);
    } else if (streq(argv[3], "response")) {
        return handle_command_t2lm_response(argc, argv, ifname, link, sock_ctx);
    } else if (streq(argv[3], "teardown")) {
        return handle_command_t2lm_teardown(argc, argv, ifname, link, sock_ctx);
    }

err:
    fprintf(stderr, "T2LM command usage");
    fprintf(stderr, "wlanconfig athX t2lm_test_cmd request assoc_frm <1/0> peer_mld_mac <peer MLD macaddr> dir <direction> default_mapping <default mapping> link_mapping_size <size> num_tids <num_tids> <TID0> <TID0_link_map> <TID1> <TID1_link_map> [dir <direction> ...]\n");
    fprintf(stderr, "wlanconfig athX t2lm_test_cmd response assoc_frm <1/0> peer_mld_mac <peer MLD macaddr> status <status> unsolicited_t2lm_resp <unsolicited T2LM response> [dir <direction> default_mapping <default mapping> link_mapping_size <size> num_tids <num_tids> <TID0> <TID0_link_map> <TID1> <TID1_link_map> [dir <direction> ...]\n");
    fprintf(stderr, "wlanconfig athX t2lm_test_cmd teardown peer_mld_mac <peer MLD macaddr>\n");

    return 0;
}

int handle_command_t2lm_link_map_config(int argc, char *argv[], const char *ifname, uint8_t link,
                                        struct socket_context *sock_ctx)
{
    struct ieee80211_wlanconfig config;
    struct ieee80211_t2lm_link_map_value *t2lm_link_map_config = &(config.data.t2lm_link_map_config);
    int retval = -1;
    int i = 5;

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_T2LM_LINK_MAP_CONFIG;

    if (argc < 7 || argc == 8) {
        fprintf(stderr, "Invalid number of arguments, argc:%d\n", argc);
        goto cmd_usage;
    }

    if (streq(argv[3], "ieee_link_map")) {
        t2lm_link_map_config->link_map_value = strtoint(argv[4]);
    } else {
        goto cmd_usage;
    }

    while (i < argc)
    {
        if (streq(argv[i], "map_switch_time")) {
            t2lm_link_map_config->map_switch_time = atoi(argv[++i]);
        }

        if (streq(argv[i], "expected_dur")) {
            t2lm_link_map_config->expected_dur = atoi(argv[++i]);
        }

        if (streq(argv[i], "link_mapping_size")) {
            t2lm_link_map_config->link_mapping_size = atoi(argv[++i]);
        }

        i++;
    }

    return send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_T2LM_LINK_MAP_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

cmd_usage:
    fprintf(stderr, "wlanconfig athX link_map_config_test_cmd ieee_link_map <ieee_link_map_values> map_switch_time <map_switch_time> expected_dur <expected_dur> link_mapping_size <size>\n");
    fprintf(stderr, "wlanconfig wifiX link_map_config_test_cmd ieee_link_map <ieee_link_map_values> map_switch_time <map_switch_time> expected_dur <expected_dur> link_mapping_size <size>\n");
    return retval;
}

int handle_command_epcs(int argc, char *argv[], const char *ifname,
                        uint8_t link, struct socket_context *sock_ctx)
{
    if (argc <= 3) {
        goto err;
    }

    if (streq(argv[3], "enable") || streq(argv[3], "disable") || streq(argv[3], "authorize") || streq(argv[3], "deauthorize")|| streq(argv[3], "set_edca_param") || streq(argv[3], "get_edca_param") || streq(argv[3], "set_muedca_param") || streq(argv[3], "get_muedca_param")) {
        return handle_command_epcs_en_dis(argc, argv, ifname, link, sock_ctx);
    }

err:
    fprintf(stderr, "EPCS command usage");
    fprintf(stderr, "wlanconfig athX epcs_test_cmd enable peer_mld_mac <peer MLD macaddr]\n");
    fprintf(stderr, "wlanconfig athX epcs_test_cmd disable peer_mld_mac <peer MLD macaddr>\n");
    fprintf(stderr, "wlanconfig athX epcs_test_cmd authorize peer_mld_mac <peer MLD macaddr>\n");
    fprintf(stderr, "wlanconfig athX epcs_test_cmd deauthorize peer_mld_mac <peer MLD macaddr>\n");
    fprintf(stderr, "wlanconfig athX epcs_test_cmd set_edca_param BE/BK/VI/VO aifsn <aifsn> cwmin <cwmin> cwmax <cwmax> txoplimit <txoplimit>\n");
    fprintf(stderr, "wlanconfig athX epcs_test_cmd get_edca_param\n");
    fprintf(stderr, "wlanconfig athX epcs_test_cmd BE/BK/VI/VO set_muedca_param aifsn <aifsn> cwmin <cwmin> cwmax <cwmax> timer <timer>\n");
    fprintf(stderr, "wlanconfig athX epcs_test_cmd get_muedca_param\n");

    return 0;
}

int handle_command_link_map_config(int argc, char *argv[], const char *ifname, uint8_t link,
                                   struct socket_context *sock_ctx)
{
    if (argc <= 3)
        goto err;

    if (streq(argv[3], "ieee_link_map")) {
        return handle_command_t2lm_link_map_config(argc, argv, ifname, link, sock_ctx);
    }

err:
    fprintf(stderr, "T2LM link map config command usage");
    fprintf(stderr, "wlanconfig athX link_map_config_test_cmd ieee_link_map <ieee_link_map_values> map_switch_time <map_switch_time> expected_dur <expected_dur> link_mapping_size <size>\n");
    fprintf(stderr, "wlanconfig wifiX link_map_config_test_cmd ieee_link_map <ieee_link_map_values> map_switch_time <map_switch_time> expected_dur <expected_dur> link_mapping_size <size>\n");
    return -1;
}
#endif /* WLAN_FEATURE_11BE */

int handle_command_tdma_schedule_config(int argc, char *argv[], const char *ifname,
                                        uint8_t link, struct socket_context *sock_ctx)
{
    struct ieee80211_wlanconfig config;
    struct ieee80211_wlanconfig_tdma_schedule *tdma_schedule_config = &(config.data.tdma_schedule_config);
    int retval = -1;
    uint8_t SchType;

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));

    if (argc != 38) {
        fprintf(stderr, "Invalid number of arguments, argc:%d\n", argc);
        goto cmd_usage;
    }

    if (streq(argv[3], "sched_type")) {
        SchType = atoi(argv[4]);
    } else {
        goto cmd_usage;
    }

    if (streq(argv[5], "is_last_schedule")) {
        if (SchType == MAX_UINT8_t) {
            tdma_schedule_config->sched_type = SchType;
        } else {
            tdma_schedule_config->sched_type = ((atoi(argv[6]) << 1) | SchType);
        }
    } else {
        goto cmd_usage;
    }

    if (streq(argv[7], "sched_id") && ((atoi(argv[8]) >= 0) && (atoi(argv[8]) < 10))) {
        tdma_schedule_config->sched_id = atoi(argv[8]);
    } else {
        goto cmd_usage;
    }

    if (streq(argv[9], "bssid")) {
        retval = get_mac_addr(argv[10], (char *)&tdma_schedule_config->bssid);
        if (retval) {
            fprintf(stderr, "Invalid macaddr\n");
            goto cmd_usage;
        }
    } else {
        goto cmd_usage;
    }

    if (streq(argv[11], "start_time_tsf_low")) {
        tdma_schedule_config->start_time_tsf_low = atoi(argv[12]);
    } else {
        goto cmd_usage;
    }

    if (streq(argv[13], "start_time_tsf_high")) {
        tdma_schedule_config->start_time_tsf_high = atoi(argv[14]);
    } else {
        goto cmd_usage;
    }

    if (streq(argv[15], "num_busy_slots")) {
        tdma_schedule_config->num_busy_slots = atoi(argv[16]);
    } else {
        goto cmd_usage;
    }

    if (streq(argv[17], "busy_slot_dur_ms")) {
        tdma_schedule_config->busy_slot_dur_ms = atoi(argv[18]);
    } else {
        goto cmd_usage;
    }

    if (streq(argv[19], "busy_slot_intvl_ms")) {
        tdma_schedule_config->busy_slot_intvl_ms = atoi(argv[20]);
    } else {
        goto cmd_usage;
    }

    if (streq(argv[21], "edca_params_valid")) {
        tdma_schedule_config->edca_params_valid = atoi(argv[22]);
    } else {
        goto cmd_usage;
    }

    if (streq(argv[23], "aifsn")) {
        tdma_schedule_config->aifsn[0] = atoi(argv[24]);
        tdma_schedule_config->aifsn[1] = atoi(argv[25]);
        tdma_schedule_config->aifsn[2] = atoi(argv[26]);
        tdma_schedule_config->aifsn[3] = atoi(argv[27]);
    } else {
        goto cmd_usage;
    }
    if (streq(argv[28], "cwmin")) {
        tdma_schedule_config->cwmin[0] = atoi(argv[29]);
        tdma_schedule_config->cwmin[1] = atoi(argv[30]);
        tdma_schedule_config->cwmin[2] = atoi(argv[31]);
        tdma_schedule_config->cwmin[3] = atoi(argv[32]);
    } else {
        goto cmd_usage;
    }
    if (streq(argv[33], "cwmax")) {
        tdma_schedule_config->cwmax[0] = atoi(argv[34]);
        tdma_schedule_config->cwmax[1] = atoi(argv[35]);
        tdma_schedule_config->cwmax[2] = atoi(argv[36]);
        tdma_schedule_config->cwmax[3] = atoi(argv[37]);
    } else {
        goto cmd_usage;
    }

    return send_command(sock_ctx, ifname, link, &config,
            sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_TDMA_SCHEDULE_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

cmd_usage:
    fprintf(stderr, "TDMA schedule type command usage\n");
    fprintf(stderr, "wlanconfig wifiX tdma_test_cmd sched_type <value> is_last_schedule <value> sched_id <value> bssid <macaddr> start_time_tsf_low <value> start_time_tsf_high <value> num_busy_slots <value> busy_slot_dur_ms <value> busy_slot_intvl_ms <value> edca_params_valid <value> aifsn <val0> <val1> <val2> <val3> cwmin <val0> <val1> <val2> <val3> cwmax <val0> <val1> <val2> <val3>\n");
    return retval;
}

int handle_command_tdma_schedule(int argc, char *argv[], const char *ifname,
                                 uint8_t link, struct socket_context *sock_ctx)
{
    if (argc <= 3)
        goto err;

    return handle_command_tdma_schedule_config(argc, argv, ifname, link, sock_ctx);

err:
    fprintf(stderr, "TDMA schedule type command usage");
    fprintf(stderr, "wlanconfig wifiX tdma_test_cmd sched_type <value> is_last_schedule <value> sched_id <value> bssid <macaddr> start_time_tsf_low <value> start_time_tsf_high <value> num_busy_slots <value> busy_slot_dur_ms <value> busy_slot_intvl_ms <value> edca_params_valid <value> aifsn <val0> <val1> <val2> <val3> cwmin <val0> <val1> <val2> <val3> cwmax <val0> <val1> <val2> <val3>\n");
    return -1;
}
