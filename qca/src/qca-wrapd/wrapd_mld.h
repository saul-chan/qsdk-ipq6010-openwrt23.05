/*
 * Copyright (c) 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef _WRAPD_MLD_H
#define _WRAPD_MLD_H

struct mld_global {
    struct mld_node *mld_list;
    uint8_t mld_count;
};

enum mld_node_type {
    APVAP_MLD_NODE,
    MPSTA_MLD_NODE,
    PSTA_MLD_NODE,
};

struct mld_node {
    uint8_t mld_id;
    u8 mld_addr[IEEE80211_ADDR_LEN];
    void *link_ptrs[MAX_RADIO_CNT];
    uint16_t link_count;
    enum mld_node_type node_type;
    struct mld_node *next;
};

int8_t wrapd_init_mld();
void wrapd_add_ap_mld_node(struct ctrl_conn *hapd, struct cmd_args *args);
void wrapd_add_mpsta_mld_node(struct wrap_wifi_iface *aptr, struct cmd_args *args, struct wrapd_global *wptr);
void wrapd_add_psta_mld_node(struct wrap_wifi_iface *aptr, struct proxy_sta *psta, const u8* mac);
void wrapd_remove_mld_node(void *node);
u8 *wrapd_get_mld_addr(void *node);
void
wrapd_create_psta_conf_file(struct wrap_wifi_iface *aptr, struct proxy_sta *psta, void *mldnode);
#endif /* _WRAPD_MLD_H */
