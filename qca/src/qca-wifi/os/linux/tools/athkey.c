/*
 * Copyright (c) 2017,2021-2022 Qualcomm Innovation Center, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * Copyright (c) 2010, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <qcatools_lib.h>
#if UMAC_SUPPORT_CFG80211
#include <netlink/genl/genl.h>
#include <netlink/attr.h>
#include <netlink/cache.h>
#endif

#include <ieee80211_external.h>

extern unsigned int if_nametoindex(const char *ifname);
int s = -1;

struct ieee80211_key{
	struct ieee80211req_key req_key;
	uint32_t suite;
	int seq_len;
	uint8_t seq[8];
};

/* cipher suite selectors */
#define WLAN_CIPHER_SUITE_WEP40         0x000FAC01
#define WLAN_CIPHER_SUITE_TKIP          0x000FAC02
#define WLAN_CIPHER_SUITE_RESERVED      0x000FAC03
#define WLAN_CIPHER_SUITE_CCMP          0x000FAC04
#define WLAN_CIPHER_SUITE_WEP104        0x000FAC05
#define WLAN_CIPHER_SUITE_AES_CMAC      0x000FAC06
#define WLAN_CIPHER_SUITE_GCMP          0x000FAC08
#define WLAN_CIPHER_SUITE_GCMP_256      0x000FAC09
#define WLAN_CIPHER_SUITE_CCMP_256      0x000FAC0A
#define WLAN_CIPHER_SUITE_BIP_GMAC_128  0x000FAC0B
#define WLAN_CIPHER_SUITE_BIP_GMAC_256  0x000FAC0C
#define WLAN_CIPHER_SUITE_BIP_CMAC_256  0x000FAC0D

#define ATHKEY_CPU_TO_LE64(x) __cpu_to_le64((x))

static void checksocket()
{
	if (s < 0 ? (s = socket(PF_INET, SOCK_DGRAM, 0)) == -1 : 0)
		perror("socket(SOCK_DRAGM)");
}

struct athkeyhandler{
#if UMAC_SUPPORT_CFG80211
	wifi_cfg80211_context cfg80211_sock_ctx; /* cfg80211 socket context */
	config_mode_type cfg_flag;               /* cfg flag */
#endif /* UMAC_SUPPORT_CFG80211 */
};

#if UMAC_SUPPORT_CFG80211
#define IS_CFG80211_ENABLED(p)      (((p)->cfg_flag == CONFIG_CFG80211)?1:0)
#define GET_ADDR_OF_CFGSOCKINFO(p)  (&(p)->cfg80211_sock_ctx)

static int init_cfg80211_socket(struct athkeyhandler *athkey);
static void destroy_cfg80211_socket(struct athkeyhandler *athkey);

/*
 * Function     : init_cfg80211_socket
 * Description  : initialize cfg80211 socket
 * Input params : pointer to athkeyhandler
 * Return       : 0:success, -1:failure
 *
 */
static int init_cfg80211_socket(struct athkeyhandler *athkey)
{
	wifi_cfg80211_context *pcfg80211_sock_ctx =
		GET_ADDR_OF_CFGSOCKINFO(athkey);

	/* Reset the private sockets to 0 if the application does not pass
	 * custom private event and command sockets. In this case, the default
	 * port is used for netlink communication.
	 */
	pcfg80211_sock_ctx->pvt_cmd_sock_id = 0;
	pcfg80211_sock_ctx->pvt_event_sock_id = 0;

	if (wifi_init_nl80211(pcfg80211_sock_ctx) != 0) {
		return -1;
	}
	return 0;
}

/*
 * Function     : destroy_cfg80211_socket
 * Description  : destroy cfg80211 socket
 * Input params : pointer to athkeyhandler
 * Return       : void
 *
 */
static void destroy_cfg80211_socket(struct athkeyhandler *athkey)
{
	nl_socket_free(athkey->cfg80211_sock_ctx.cmd_sock);
}
#endif /* UMAC_SUPPORT_CFG80211 */

const char *progname;

static int
set80211priv(const char *dev, int op, void *data, int len, int show_err)
{
	struct iwreq iwr;
	checksocket();

	memset(&iwr, 0, sizeof(iwr));
	strlcpy(iwr.ifr_name, dev, IFNAMSIZ);
	if (len < IFNAMSIZ) {
		/*
		 * Argument data fits inline; put it there.
		 */
		memcpy(iwr.u.name, data, len);
	} else {
		/*
		 * Argument data too big for inline transfer; setup a
		 * parameter block instead; the kernel will transfer
		 * the data for the driver.
		 */
		iwr.u.data.pointer = data;
		iwr.u.data.length = len;
	}

	if (ioctl(s, op, &iwr) < 0) {
		if (show_err) {
			static const char *opnames[] = {
				"ioctl[IEEE80211_IOCTL_SETPARAM]",
				"ioctl[IEEE80211_IOCTL_GETPARAM]",
				"ioctl[IEEE80211_IOCTL_SETKEY]",
				NULL,
				"ioctl[IEEE80211_IOCTL_DELKEY]",
				NULL,
				"ioctl[IEEE80211_IOCTL_SETMLME]",
				NULL,
				"ioctl[IEEE80211_IOCTL_SETOPTIE]",
				"ioctl[IEEE80211_IOCTL_GETOPTIE]",
				"ioctl[IEEE80211_IOCTL_ADDMAC]",
				"ioctl[IEEE80211_IOCTL_DELMAC]",
				"ioctl[IEEE80211_IOCTL_GETCHANLIST]",
				"ioctl[IEEE80211_IOCTL_SETCHANLIST]",
				NULL,
			};
			if (IEEE80211_IOCTL_SETPARAM <= op &&
			    op <= IEEE80211_IOCTL_SETCHANLIST)
				perror(opnames[op - SIOCIWFIRSTPRIV]);
			else if (op == IEEE80211_IOCTL_GETKEY)
				perror("IEEE80211_IOCTL_GETKEY[fail]");
			else
				perror("ioctl[unknown???]");
		}
		return -errno;
	}
	return 0;
}

/**
* athkey_send_command; function to send the cfg command or ioctl command.
* @athkey     : pointer to athkeyhandler
* @ifname    : interface name
* @buf       : buffer
* return     : 0 for sucess, -1 for failure
*/
static int athkey_send_command(struct athkeyhandler *athkey,
				const char *ifname, int op,
				void *buf)
{
#if UMAC_SUPPORT_CFG80211
	int err;
	struct cfg80211_data arg;

	if (IS_CFG80211_ENABLED(athkey)) {
		arg.data = (void *)buf;
		arg.flags = 0;
		arg.parse_data = 0;
		arg.callback = NULL;

		if (op == IEEE80211_IOCTL_DELKEY) {
			arg.length =  sizeof(struct ieee80211req_del_key);
			err = wifi_cfg80211_send_generic_command(&(athkey->cfg80211_sock_ctx),
				QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION,
				QCA_NL80211_VENDOR_SUBCMD_DEL_KEY, ifname, MLO_INVALID_LINK_ID, (char*)&arg, arg.length);
		} else if (op == IEEE80211_IOCTL_SETKEY) {
			arg.length =  sizeof(struct ieee80211_key);
			err = wifi_cfg80211_send_generic_command(&(athkey->cfg80211_sock_ctx),
				QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION,
				QCA_NL80211_VENDOR_SUBCMD_SET_KEY, ifname, MLO_INVALID_LINK_ID, (char*)&arg, arg.length);
		} else {
			arg.length =  sizeof(struct ieee80211req_key);
			err = wifi_cfg80211_send_generic_command(&(athkey->cfg80211_sock_ctx),
				QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_CONFIGURATION,
				QCA_NL80211_VENDOR_SUBCMD_GET_KEY, ifname, MLO_INVALID_LINK_ID, (char*)&arg, arg.length);
		}

		if (err < 0) {
			fprintf(stderr, "CFG80211 Command failed, status: %d\n", err);
		}
		return err;
	} else
#endif /* UMAC_SUPPORT_CFG80211 */
	{
		if (op == IEEE80211_IOCTL_DELKEY) {
			return set80211priv(ifname, op, buf,
						sizeof(struct ieee80211req_del_key), 1);
		} else {
			return set80211priv(ifname, op, buf,
						sizeof(struct ieee80211req_key), 1);
		}
	}
}

static int digittoint(int c)
{
	return isdigit(c) ? c - '0' : isupper(c) ? c - 'A' + 10 : c - 'a' + 10;
}

static int getdata(const char *arg, u_int8_t *data, size_t maxlen,
			int is_key)
{
	const char *cp = arg;
	int len;

	if (cp[0] == '0' && (cp[1] == 'x' || cp[1] == 'X'))
		cp += 2;
	len = 0;

	if (is_key) {
		int keyb0 = 0;
		if (!isxdigit(cp[0])) {
			fprintf(stderr, "%s: invalid key value %c (not hex)\n",
				progname, cp[0]);
			exit(-1);
		}
		keyb0 = digittoint(cp[0]);
		if (keyb0 == 0) {
			printf("Invalid key value, first bit must be non-zero\n");
			return -1;
		}
	}
	while (*cp) {
		int b0, b1;
		if (cp[0] == ':' || cp[0] == '-' || cp[0] == '.') {
			cp++;
			continue;
		}
		if (!isxdigit(cp[0])) {
			fprintf(stderr, "%s: invalid data value %c (not hex)\n",
				progname, cp[0]);
			exit(-1);
		}
		b0 = digittoint(cp[0]);
		if (cp[1] != '\0') {
			if (!isxdigit(cp[1])) {
				fprintf(stderr, "%s: invalid data value %c "
					"(not hex)\n", progname, cp[1]);
				exit(-1);
			}
			b1 = digittoint(cp[1]);
			cp += 2;
		} else {			/* fake up 0<n> */
			b1 = b0, b0 = 0;
			cp += 1;
		}
		if (len > maxlen) {
			fprintf(stderr,
				"%s: too much data in %s, max %zu bytes\n",
				progname, arg, maxlen);
		}
		data[len++] = (b0<<4) | b1;
	}
	return len;
}

static void
cipher_type(int type)
{
	switch(type) {
	case IEEE80211_CIPHER_WEP:
		printf("WEP");
		break;
	case IEEE80211_CIPHER_TKIP:
		printf("TKIP");
		break;
	case IEEE80211_CIPHER_AES_OCB:
		printf("AES-OCB");
		break;
	case IEEE80211_CIPHER_CKIP:
		printf("CKIP");
		break;
	case IEEE80211_CIPHER_AES_CCM:
		printf("CCMP");
		break;
	case IEEE80211_CIPHER_AES_CCM_256:
		printf("CCMP-256");
		break;
	case IEEE80211_CIPHER_AES_GCM:
		printf("GCMP");
		break;
	case IEEE80211_CIPHER_AES_GCM_256:
		printf("GCMP-256");
		break;
	case IEEE80211_CIPHER_AES_CMAC:
		printf("AES-CMAC");
		break;
	case IEEE80211_CIPHER_AES_GMAC_256:
		printf("GMAC-256");
		break;
	case IEEE80211_CIPHER_AES_CMAC_256:
		printf("CMAC-256");
		break;
	case IEEE80211_CIPHER_WAPI:
		printf("WAPI");
		break;
	case IEEE80211_CIPHER_NONE:
		printf("none");
		break;
	default :
		printf("undefined");
		break;
	}
}

static int
getcipher(const char *name)
{
//#define	streq(a,b)	(strcasecmp(a,b) == 0)

	if (streq(name, "wep"))
		return IEEE80211_CIPHER_WEP;
	if (streq(name, "tkip"))
		return IEEE80211_CIPHER_TKIP;
	if (streq(name, "aes-ocb") || streq(name, "ocb"))
		return IEEE80211_CIPHER_AES_OCB;
	if (streq(name, "aes-ccm") || streq(name, "ccm") ||
	    streq(name, "aes"))
		return IEEE80211_CIPHER_AES_CCM;
	if (streq(name, "ccm256"))
		return IEEE80211_CIPHER_AES_CCM_256;
	if (streq(name, "ckip"))
		return IEEE80211_CIPHER_CKIP;
	if (streq(name, "gcm"))
		return IEEE80211_CIPHER_AES_GCM;
	if (streq(name, "gcm256"))
		return IEEE80211_CIPHER_AES_GCM_256;
	if (streq(name, "cmac"))
		return IEEE80211_CIPHER_AES_CMAC;
	if (streq(name, "cmac256"))
		return IEEE80211_CIPHER_AES_CMAC_256;
	if (streq(name, "gmac256"))
		return IEEE80211_CIPHER_AES_GMAC_256;
	if (streq(name, "wapi"))
		return IEEE80211_CIPHER_WAPI;
	if (streq(name, "none") || streq(name, "clr"))
		return IEEE80211_CIPHER_NONE;

	fprintf(stderr, "%s: unknown cipher %s\n", progname, name);
	exit(-1);
#undef streq
}

static uint32_t cipher_to_suite(uint8_t cipher, uint8_t keylen) {
	switch(cipher) {
	case IEEE80211_CIPHER_WEP:
		if (keylen == 5)
			return WLAN_CIPHER_SUITE_WEP40;
		return WLAN_CIPHER_SUITE_WEP104;
		break;
	case IEEE80211_CIPHER_TKIP:
		return WLAN_CIPHER_SUITE_TKIP;
		break;
	case IEEE80211_CIPHER_AES_CCM:
		return WLAN_CIPHER_SUITE_CCMP;
		break;
	case IEEE80211_CIPHER_AES_CCM_256:
		return WLAN_CIPHER_SUITE_CCMP_256;
		break;
	case IEEE80211_CIPHER_AES_GCM:
		return WLAN_CIPHER_SUITE_GCMP;
		break;
	case IEEE80211_CIPHER_AES_GCM_256:
		return WLAN_CIPHER_SUITE_GCMP_256;
		break;
	case IEEE80211_CIPHER_AES_CMAC:
		return WLAN_CIPHER_SUITE_AES_CMAC;
		break;
	case IEEE80211_CIPHER_AES_CMAC_256:
		return WLAN_CIPHER_SUITE_BIP_CMAC_256;
		break;
	case IEEE80211_CIPHER_AES_GMAC_256:
		return WLAN_CIPHER_SUITE_BIP_GMAC_256;
		break;
	case IEEE80211_CIPHER_NONE:
		printf("no valid suite");
		break;
	default :
		printf("undefined suite");
		break;
	}
	return 0;
}

static void
macaddr_printf(u_int8_t *mac)
{
	printf("%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void
usage(void)
{
	fprintf(stderr, "usage to set: %s -i athX -s keyix cipher keyval [mac]\n",
		progname);
	fprintf(stderr, "usage to get: %s -i athX -x keyix [mac]\n",
                progname);
	fprintf(stderr, "usage to set default key as keyix: %s -i athX -c keyix [mac]\n",
		progname);
#if UMAC_SUPPORT_CFG80211
	fprintf(stderr, "usage to get for cfg: %s -cfg80211 -i athX -x keyix [mac]\n",
                progname);
	fprintf(stderr, "usage to set for cfg: %s -cfg80211 -i athX -s -e -u -r <rxsc> <keyidx> <cipher> <keyval> <mac>\n",
                progname);
	fprintf(stderr, "usage to set default key as keyix for cfg: %s -cfg80211 -i athX -c keyix [mac]\n",
		progname);
#endif
	exit(-1);
}

int
main(int argc, char *argv[])
{
	const char *ifname = NULL;
	struct ieee80211req_del_key delkey;
	struct ieee80211_key setkey;
	struct ieee80211req_key getkey;
	int c, keyix;
	int help = 0;
	int get_key = 0;
	int cipher = 0;
	int op = IEEE80211_IOCTL_SETKEY;
	int err = 0;
	int default_key = 0;
	int swencrypt = 0;
	int swdecrypt = 0;
	int seq_len = 0;
	uint64_t keytsc = 0;
	uint64_t keyrsc = 0;
	struct athkeyhandler athkey;
        int default_flag = 0;
        uint8_t is_bcast[QDF_MAC_ADDR_SIZE] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	progname = argv[0];

	memset(&athkey, 0, sizeof(athkey));
#if UMAC_SUPPORT_CFG80211
	/* figure out whether cfg80211 is enabled */
	if (argc > 1 && (strcmp(argv[1], "-cfg80211") == 0)) {
		athkey.cfg_flag = CONFIG_CFG80211;
		argc -= 1;
		argv += 1;
	}
	if (IS_CFG80211_ENABLED(&athkey)) {
	/* init cfg80211 socket */
		if (init_cfg80211_socket(&athkey) < 0) {
			fprintf(stderr, "unable to create cfg80211 socket");
			exit(EXIT_FAILURE);
		}
	}

	if (athkey.cfg_flag != get_config_mode_type()) {
		printf("Invalid command for current cfg/wext mode\n");
		printf("Add -cfg80211 option for cfg mode: %s -cfg80211 -i athX <set/get options>\n", progname);
	}

#endif /* UMAC_SUPPORT_CFG80211 */

	while ((c = getopt(argc, argv, "cdgsxheut:r:i:")) != -1)
		switch (c) {
		case 'c':
			default_key=1;
			op = IEEE80211_IOCTL_SETKEY;
			break;
		case 'd':
			op = IEEE80211_IOCTL_DELKEY;
			break;
		case 'g':
			get_key = 1;
			op = IEEE80211_IOCTL_GETKEY;
			break;
		case 's':
			op = IEEE80211_IOCTL_SETKEY;
			break;
		case 'i':
			ifname = optarg;
			break;
		case 'h':
			help = 1;
			break;
		case 'x':
			op = IEEE80211_IOCTL_GETKEY;
			cipher = 1;
			break;
		case 'e':
			swencrypt = 1;
			break;
		case 'u':
			swdecrypt = 1;
			break;
		case 't':
			keytsc = strtoull(optarg, (char **)NULL, 0);
			break;
		case 'r':
			keyrsc = strtoull(optarg, (char **)NULL, 0);
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	argc -= optind;
	argv += optind;
	if (argc < 1 || ifname == NULL)
		usage();

	if(help == 1)
		usage();
	/* 0 keyix for default keyix */
	keyix = atoi(argv[0]);
	if (!(0 <= keyix && keyix <= 8) )
		errx(-1, "%s: invalid key index %s, must be [0..8]",
			progname, argv[0]);
	switch (op) {
	case IEEE80211_IOCTL_DELKEY:
		memset(&delkey, 0, sizeof(delkey));
		delkey.idk_keyix = keyix-1;
		if (argv[1] == NULL)
			memset(delkey.idk_macaddr, 0xff, QDF_MAC_ADDR_SIZE);
		else {
			err = getdata(argv[1], delkey.idk_macaddr,
                                         QDF_MAC_ADDR_SIZE, 0);
                        if (err < 0)
                            goto exit;
                }

		err = athkey_send_command(&athkey, ifname, op,
						&delkey);
		goto exit;
	case IEEE80211_IOCTL_SETKEY:
		if (default_key) {
			if(argc < 1)
				usage();
			memset(&getkey, 0, sizeof(getkey));
			if (argv[1] == NULL) {
				memset(getkey.ik_macaddr, 0xff, QDF_MAC_ADDR_SIZE);
				default_flag = 1;
			} else {
				 (void) getdata(argv[1], getkey.ik_macaddr,
							QDF_MAC_ADDR_SIZE, 0);
                                 if (memcmp(getkey.ik_macaddr,(const void *)is_bcast, QDF_MAC_ADDR_SIZE) != 0)
                                         default_flag = 1;
			}
			getkey.ik_keyix = keyix-1;
			if ((err = athkey_send_command(&athkey, ifname, IEEE80211_IOCTL_GETKEY,
							&getkey)))
			{
				if (err != -ENOENT)
					printf("%s: Failed to get encryption data\n",
							__func__);
				err = -1;
				goto exit;
			}

			memset(&setkey, 0, sizeof(setkey));
			setkey.req_key = getkey;
			setkey.req_key.ik_flags = IEEE80211_KEY_XMIT | IEEE80211_KEY_RECV;
			if (default_flag)
				setkey.req_key.ik_flags |= IEEE80211_KEY_DEFAULT;
			err = athkey_send_command(&athkey, ifname, op,
							&setkey);
#if UMAC_SUPPORT_CFG80211
			if (IS_CFG80211_ENABLED(&athkey)) {
				destroy_cfg80211_socket(&athkey);
			} else
#endif /* UMAC_SUPPORT_CFG80211 */
			{
			close(s);
			}
			goto exit;
		}
		else {
		if (argc != 3 && argc != 4)
			usage();
		memset(&setkey, 0, sizeof(setkey));
		setkey.req_key.ik_flags = IEEE80211_KEY_XMIT | IEEE80211_KEY_RECV;
		if (swencrypt) {
			setkey.req_key.ik_flags |= IEEE80211_KEY_SWENCRYPT;
		}
		if (swdecrypt) {
			setkey.req_key.ik_flags |= IEEE80211_KEY_SWDECRYPT;
		}
		setkey.req_key.ik_keyix = keyix-1;
		setkey.req_key.ik_type = getcipher(argv[1]);
		setkey.req_key.ik_keylen = getdata(argv[2],
			setkey.req_key.ik_keydata,
			sizeof(setkey.req_key.ik_keydata), 1);
		if (argc == 4) {
			(void) getdata(argv[3], setkey.req_key.ik_macaddr,
				QDF_MAC_ADDR_SIZE, 0);
                        if (memcmp(setkey.req_key.ik_macaddr, (const void *)is_bcast, QDF_MAC_ADDR_SIZE) != 0)
                                default_flag = 1;
                }
		setkey.suite = cipher_to_suite(setkey.req_key.ik_type,
					setkey.req_key.ik_keylen);
		if (setkey.req_key.ik_type != IEEE80211_CIPHER_WEP &&
			setkey.req_key.ik_type != IEEE80211_CIPHER_WAPI)
			seq_len = 6;
		else if (setkey.req_key.ik_type == IEEE80211_CIPHER_WAPI)
			seq_len = 8;
		setkey.seq_len = seq_len;

		keytsc = ATHKEY_CPU_TO_LE64(keytsc);
		keyrsc = ATHKEY_CPU_TO_LE64(keyrsc);

		memset(setkey.seq, 0, sizeof(setkey.seq));
		memcpy(setkey.seq, (uint8_t *)&keyrsc, seq_len);

		setkey.req_key.ik_keyrsc = keyrsc;
		setkey.req_key.ik_keytsc = keytsc;

                if(default_flag)
                        setkey.req_key.ik_flags |= IEEE80211_KEY_DEFAULT;
		err = athkey_send_command(&athkey, ifname, op,
						&setkey);
		goto exit;
	}

	case IEEE80211_IOCTL_GETKEY:
		memset(&getkey, 0, sizeof(getkey));
		if (argv[1] == NULL)
			memset(getkey.ik_macaddr, 0xff, QDF_MAC_ADDR_SIZE);
		else
			(void) getdata(argv[1], getkey.ik_macaddr,
                                QDF_MAC_ADDR_SIZE, 0);
		getkey.ik_keyix = keyix-1;
		if ((err = athkey_send_command(&athkey, ifname, op,
				&getkey)))
		{
			if (err != -ENOENT)
				printf("%s: Failed to get encryption data\n",
					__func__);
			err = -1;
			goto exit;
		}
		if (getkey.ik_type == IEEE80211_CIPHER_NONE) {
			err = 0;
			goto exit;
		}
		if(get_key == 1)
		{
			printf("%-17.17s %3s %4s %4s\n"
				, "ADDR"
				, "KEYIDX"
				, "CIPHER"
				, "KEY"
				);
		}
		else if(cipher == 1)
		{
			printf("%-17.17s %3s %4s\n"
			, "ADDR"
			, "KEYIDX"
			, "CIPHER"
			);
		}

		if(get_key == 1)
		{
			int i = 0;
			macaddr_printf(getkey.ik_macaddr);
			printf("  %4d   ", getkey.ik_keyix+1);
			cipher_type(getkey.ik_type);
			printf("    %02X%02X", *(getkey.ik_keydata),
				*(getkey.ik_keydata+1));
			for(i=2;i<getkey.ik_keylen && i < 48;i++)
			{
				if( i%2 == 0)
					printf("-");
				printf("%02X", *(getkey.ik_keydata+i));
			}
			printf("\n");
		}
		else if(cipher == 1)
		{
			macaddr_printf(getkey.ik_macaddr);
			printf("  %4d   ", keyix);
			cipher_type(getkey.ik_type);
			printf("\n");
		}
#if UMAC_SUPPORT_CFG80211
		if (IS_CFG80211_ENABLED(&athkey)) {
			destroy_cfg80211_socket(&athkey);
		} else
#endif /* UMAC_SUPPORT_CFG80211 */
		{
			close(s);
		}
		err = 0;
		goto exit;
	}
	err = -1;

exit:
	/* Zero-out local key variables */
	memset(&setkey, 0, sizeof(struct ieee80211_key));
	memset(&getkey, 0, sizeof(struct ieee80211req_key));
	return err;
}
