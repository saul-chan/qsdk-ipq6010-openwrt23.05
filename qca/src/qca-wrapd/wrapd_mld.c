/*
 * Copyright (c) 2021-2023 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <linux/types.h>
#include <net/if_arp.h>
#include <sys/utsname.h>
#include "includes.h"
#include "utils/common.h"
#include "ieee802_11_defs.h"
#include "linux_wext.h"
#include "netlink.h"
#include "priv_netlink.h"

#include "wrapd_api.h"
#include "qwrap_structure.h"
#include "wrapd_mld.h"

#if WLAN_11BE_MLO_ENABLE

struct mld_global *mldg;

int8_t wrapd_init_mld()
{
    mldg = os_malloc(sizeof(struct mld_global));
    if(mldg== NULL) {
        wrapd_printf("ERROR: node alloc failed\n");
        return -1;
    }
    os_memset(mldg, 0, sizeof(struct mld_global));
    return 0;
}

struct mld_node *wrapd_find_mld_node(const u8 *mac)
{
    struct mld_node *node = NULL;
    for(node = mldg->mld_list ; node ; node = node->next)
    {
        if (os_memcmp(node->mld_addr, mac, IEEE80211_ADDR_LEN) == 0) {
            return node;
        }
    }
    return NULL;
}

int8_t wrapd_add_mld_node(struct mld_node **mld_node, const u8 *addr)
{
    struct mld_node *node = NULL;

    if (addr != NULL) {
        node = wrapd_find_mld_node(addr);
        if (node) {
            wrapd_printf("mld node already exists with this mac (%02x:%02x:%02x:%02x:%02x:%02x)",
                         addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
            *mld_node = node;
            return -1;
        }
        node = os_malloc(sizeof(struct mld_node));
        if(node== NULL) {
            wrapd_printf("ERROR: node alloc failed\n");
            *mld_node = NULL;
            return -1;
        }
        os_memset(node, 0, sizeof(struct mld_node));
        node->next = mldg->mld_list;
        mldg->mld_list = node;
        *mld_node = node;
        return 0;
    }
    *mld_node = NULL;
    return -1;
}

void wrapd_add_ap_mld_node(struct ctrl_conn *hapd, struct cmd_args *args)
{
    struct mld_node *node = NULL;
    int8_t retv = 0;

    retv = wrapd_add_mld_node(&node, (const u8*)args->mld_mac);
    if (node) {
        if (retv == 0) {
            node->mld_id = args->mld_id;
            os_memcpy(node->mld_addr, args->mld_mac, IEEE80211_ADDR_LEN);
            node->node_type = APVAP_MLD_NODE;
            hapd->primary_link = 1;
        }
        node->link_ptrs[node->link_count++] = (void *) hapd;
        hapd->mld_node = (void *)node;
    }
    return;
}

void wrapd_add_mpsta_mld_node(struct wrap_wifi_iface *aptr, struct cmd_args *args, struct wrapd_global *wptr)
{
    struct mld_node *node = NULL;
    int8_t retv = 0;

    retv = wrapd_add_mld_node(&node, (const u8*)args->mld_mac);
    if (node) {
        if (retv == 0) {
            node->mld_id = args->mld_id;
            os_memcpy(node->mld_addr, args->mld_mac, IEEE80211_ADDR_LEN);
            node->node_type = MPSTA_MLD_NODE;
            aptr->primary_link = 1;
            wptr->mld_enabled = 1;
        }
        node->link_ptrs[node->link_count++] = (void *) aptr;
        aptr->mld_node = (void *)node;
    }
    return;
}

void
wrapd_create_psta_conf_file(struct wrap_wifi_iface *aptr, struct proxy_sta *psta, void *mldnode)
{
    FILE *fptr1, *fptr2;
    char str[14] = "sta_mld_addr=";
    char read[100];
    int i=0;
    char *ptr = NULL;
    struct mld_node *mld_node = (struct mld_node *) mldnode;

    fptr1 = fopen(aptr->wpa_conf_file, "r");
    if (fptr1 == NULL)
    {
        wrapd_printf("Cannot open file %s \n", aptr->wpa_conf_file);
        return;
    }

    fptr2 = fopen(psta->wpa_conf_file, "w+");
    if (fptr2 == NULL)
    {
        wrapd_printf("Cannot open file %s \n", psta->wpa_conf_file);
        fclose(fptr1);
        return;
    }
    while (!feof(fptr1)) {
	fscanf(fptr1, "%[^\n]\n", read);
	if (strncmp(read, str, (sizeof(str) -1)) == 0) {
            ptr = &read[0];
            ptr = ptr + (sizeof(str) -1);
            for (i = 0; i < IEEE80211_ADDR_LEN; i++) {
                snprintf(ptr, 3, "%02x", mld_node->mld_addr[i]);
                ptr = ptr + 2;
                if (i != (IEEE80211_ADDR_LEN -1)) {
                    *ptr = ':';
                    ptr++;
                }
            }
        }
        fprintf(fptr2, "%s\n", read);
    }
    fprintf(fptr2, "preferred_assoc_link=%d\n", aptr->assoc_link);
    fclose(fptr1);
    fclose(fptr2);
    return;
}

void wrapd_add_psta_mld_node(struct wrap_wifi_iface *aptr, struct proxy_sta *psta, const u8* mac)
{
    struct mld_node *node = NULL;
    int8_t retv = 0;
    struct mld_node *mld_node = aptr->mld_node;
    u8 mld_mac[IEEE80211_ADDR_LEN];

    os_memcpy(mld_mac, mac, IEEE80211_ADDR_LEN);

    if ((mld_mac[1] == mld_node->mld_addr[1]) && (mld_mac[2] == mld_node->mld_addr[2])) {
        if ((mld_mac[1] != 0x0A) && (mld_mac[2] != 0x0B)) {
            mld_mac[1] = 0x0A;
            mld_mac[2] = 0x0B;
        } else {
            mld_mac[1] = 0x1A;
            mld_mac[2] = 0x1B;
        }
    } else {
        mld_mac[0] = mld_node->mld_addr[0];
        mld_mac[1] = mld_node->mld_addr[1];
        mld_mac[2] = mld_node->mld_addr[2];
    }

    retv = wrapd_add_mld_node(&node, mld_mac);
    if (node) {
        if (retv == 0) {
            // generate mld mac for PSTA on wrapd using below logic
            os_memcpy(node->mld_addr, mld_mac, IEEE80211_ADDR_LEN);
            node->node_type = PSTA_MLD_NODE;
            psta->primary_link = 1;
        }
        node->link_ptrs[node->link_count++] = (void *) psta;
        psta->mld_node = (void *)node;
    }
    return;
}

void delink_mld_node(struct mld_node *node)
{
    struct mld_node *prev;
    /* Remove interface from the global list of interfaces */
    prev = mldg->mld_list;
    if (prev == node) {
        mldg->mld_list = node->next;
    }	 else {
        while (prev && prev->next != node)
            prev = prev->next;
        if (prev == NULL)
            return;
        prev->next = node->next;
    }
    return;
}

void wrapd_remove_mld_node(void *node)
{
    struct mld_node *mld_node = (struct mld_node *)node;

    if (mld_node != NULL) {
        delink_mld_node(mld_node);
        os_free(mld_node);
    }
    return;
}

u8 *wrapd_get_mld_addr(void *node)
{
    struct mld_node *mld_node = (struct mld_node *)node;

    if (mld_node)
        return mld_node->mld_addr;
    return NULL;
}
#else
void wrapd_remove_mld_node(void *node)
{
    return;
}
u8 *wrapd_get_mld_addr(void *node)
{
    return NULL;
}
void wrapd_add_psta_mld_node(struct wrap_wifi_iface *aptr, struct proxy_sta *psta, const u8* mac)
{
    return;
}
void
wrapd_create_psta_conf_file(struct wrap_wifi_iface *aptr, struct proxy_sta *psta, void *mldnode)
{
    return;
}
void wrapd_add_ap_mld_node(struct ctrl_conn *hapd, struct cmd_args *args)
{
    return;
}
void wrapd_add_mpsta_mld_node(struct wrap_wifi_iface *aptr, struct cmd_args *args, struct wrapd_global *wptr)
{
    return;
}
int8_t wrapd_init_mld()
{
    return 0;
}
#endif
