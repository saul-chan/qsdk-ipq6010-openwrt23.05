/*
 * Copyright (c) 2019, 2023 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Copyright (c) 2008-2010, Atheros Communications Inc.
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

#ifndef __ACFG_API_H
#define __ACFG_API_H

#include <qcatools_lib.h>
#include <qdf_types.h>
#ifndef BUILD_X86
#include <ieee80211_external_config.h>
#else
#include <bsd/string.h>
#endif
#include <acfg_api_cmds.h>

extern char ctrl_hapd[ACFG_CTRL_IFACE_LEN];
extern char ctrl_wpasupp[ACFG_CTRL_IFACE_LEN];
/**
 * @brief  Initialize interface for device-less configurations
 *
 * @return
 */
uint32_t
acfg_dl_init(void);

/**
 * @brief Create a VAP on the specified WIFI
 *
 * @param wifi_name (Upto ACFG_MAX_IFNAME)
 * @param vap_name  (Upto ACFG_MAX_IFNAME)
 * @param mode
 * @param vapid
 * @param flags
 * @return
 */
uint32_t
acfg_create_vap(uint8_t            *wifi_name,
        uint8_t            *vap_name,
        acfg_opmode_t         mode,
        int32_t             vapid,
        uint32_t             flags,
        uint8_t              mld_id);

/**
 * @brief Delete a VAP on the specified WIFI
 *
 * @param wifi_name (Upto ACFG_MAX_IFNAME)
 * @param vap_name  (Upto ACFG_MAX_IFNAME)
 *
 * @return
 */
uint32_t
acfg_delete_vap(uint8_t *wifi_name, uint8_t *vap_name);



/**
 * @brief Determine whether a VAP is local or remote
 *
 * @param vap_name  (Upto ACFG_MAX_IFNAME)
 *
 * @return On Success
 *             If vap is offload (remote): A_STATUS_OK
 *             Else: A_STATUS_FAILED
 *         On Error: A_STATUS_EFAULT
 */
uint32_t
acfg_is_offload_vap(uint8_t *vap_name);

/**
 * @brief Add client with overriding params
 *
 * @param vap_name  (Upto ACFG_MAX_IFNAME)
 *        mac
 *        aid
 *        qos
 *        mlo
 *        lrates
 *        htrates
 *        vhtrates
 *        herates
 *        ehtrates
 *        num partner links
 *        link 1 mac
 *        link 1 qos
 *        link 1 link_id
 *        link 1 lrates
 *        link 1 htrates
 *        link 1 vhtrates
 *        link 1 herates
 *        link 1 ehtrates
 *        link 2 mac
 *        link 2 qos
 *        link 2 link_id
 *        link 2 lrates
 *        link 2 htrates
 *        link 2 vhtrates
 *        link 2 herates
 *        link 2 ehtrates
 *
 * @return On Success
 *             If : A_STATUS_OK
 *             Else: A_STATUS_FAILED
 *         On Error: A_STATUS_EFAULT
 */
uint32_t
acfg_add_client(uint8_t *vap_name, uint8_t *mac, uint32_t aid, uint32_t qos, uint32_t mlo,
                acfg_rateset_t lrates, acfg_rateset_t htrates, acfg_rateset_t vhtrates,
                acfg_rateset_t herates, acfg_rateset_t ehtrates, uint32_t num_partner_links,
                uint8_t *mac_1, uint32_t qos_1, uint32_t link_id_1,
                acfg_rateset_t lrates_1, acfg_rateset_t htrates_1, acfg_rateset_t vhtrates_1,
                acfg_rateset_t herates_1, acfg_rateset_t ehtrates_1,
                uint8_t *mac_2, uint32_t qos_2, uint32_t link_id_2,
                acfg_rateset_t lrates_2, acfg_rateset_t htrates_2, acfg_rateset_t vhtrates_2,
                acfg_rateset_t herates_2, acfg_rateset_t ehtrates_2);

/**
 * @brief Authorize client
 *
 * @param vap_name  (Upto ACFG_MAX_IFNAME)
 *        mac
 *
 * @return On Success
 *             If : A_STATUS_OK
 *             Else: A_STATUS_FAILED
 *         On Error: A_STATUS_EFAULT
 */
uint32_t
acfg_forward_client(uint8_t *vap_name, uint8_t *mac);

/**
 * @brief Delete a client
 *
 * @param vap_name  (Upto ACFG_MAX_IFNAME)
 *        mac
 *
 * @return On Success
 *             If : A_STATUS_OK
 *             Else: A_STATUS_FAILED
 *         On Error: A_STATUS_EFAULT
 */
uint32_t
acfg_delete_client(uint8_t *vap_name, uint8_t *mac);

/**
 * @brief Delete the encryption key
 *
 * @param vap_name  (Upto ACFG_MAX_IFNAME)
 *        mac
 *        key index
 *
 * @return On Success
 *             If : A_STATUS_OK
 *             Else: A_STATUS_FAILED
 *         On Error: A_STATUS_EFAULT
 */
uint32_t
acfg_del_key(uint8_t *vap_name, uint8_t *mac, uint16_t keyidx);

/**
 * @brief Set the encryption key
 *
 * @param vap_name  (Upto ACFG_MAX_IFNAME)
 *        mac
 *        cipher
 *        key index
 *        key length
 *        keydata - 64 bytes
 *
 * @return On Success
 *             If : A_STATUS_OK
 *             Else: A_STATUS_FAILED
 *         On Error: A_STATUS_EFAULT
 */
uint32_t
acfg_set_key(uint8_t *vap_name, uint8_t *mac, CIPHER_METH cipher, uint16_t keyidx,
             uint32_t keylen, uint8_t *keydata);

/**
 * @brief Set the SSID for the given VAP
 *
 * @param vap_name (Upto ACFG_MAX_IFNAME)
 * @param ssid
 *
 * @return
 */
uint32_t
acfg_set_ssid(uint8_t *vap_name, acfg_ssid_t *ssid);

/**
 * @brief Get the ssid for the given vap
 *
 * @param vap_name (Upto ACFG_MAX_IFNAME)
 * @param ssid
 */
uint32_t
acfg_get_ssid(uint8_t *vap_name, acfg_ssid_t *ssid);

/**
 * @brief Get the rssi for the given vap
 *
 * @param vap_name (Upto ACFG_MAX_IFNAME)
 * @param rssi
 */
uint32_t
acfg_get_rssi(uint8_t *vap_name, acfg_rssi_t *rssi);


/**
 * @brief Set the Channel (IEEE) number
 *
 * @param wifi_name (Radio)
 * @param chan_num (Upto ACFG_MAX_IEEE_CHAN)
 * @param chan_band (channel band)
 * @chan_num_seg2 (Secondary segment channel number)
 * @puncture_bitmap(puncture bitmap)
 * @post_acs_done_evt (Post ACS done event instead of chan change)
 */
uint32_t
acfg_set_channel(uint8_t *wifi_name, uint8_t chan_num, uint8_t chan_band,
                 uint16_t chan_num_seg2, uint16_t puncture_bitmap,
                 uint8_t post_acs_done_evt);


/**
 * @brief Get scan results
 *
 * @param vap_name (Upto ACFG_MAX_IFNAME)
 * @param list (Pointer to the list of scanned results)
 * @param len (Pointer to store the number of scan entries)
 * @param offset (If there are more results, this should be
 *                used to retrive the next block)
 *
 * @return (A_STATUS_OK if no more scan results or else
 *          A_STATUS_EINPROGRESS for more results pending)
 */
//uint32_t
//acfg_get_scan_list(uint8_t         *vap_name,
//        acfg_scan_list_t  *list,
//        uint32_t        *len,
//        uint8_t          offset);

/**
 * @brief Set the opmode
 *
 * @param wifi_name (Radio)
 * @param opmode
 */
uint32_t
acfg_set_opmode(uint8_t *wifi_name, acfg_opmode_t opmode);

/**
 * @brief Get the opmode
 *
 * @param wifi_name (Radio)
 * @param opmode
 */
uint32_t
acfg_get_opmode(uint8_t *wifi_name, acfg_opmode_t *opmode);

/**
 * @brief Set the frequency
 *
 * @param wifi_name
 * @param freq - Frequency in MHz
 *
 * @return
 */
uint32_t
acfg_set_freq(uint8_t *wifi_name, uint32_t freq);

/**
 * @brief Get RTS threshold. The value returned is in iwparam->value.
 *        Other fields can be ignored.
 *
 * @param vap_name
 * @param rts
 *
 * @return
 */
uint32_t
acfg_get_rts(uint8_t *vap_name, acfg_rts_t *rts);

/**
 *   @brief Set RTS threshold. The value returned is in iwparam->value.
 *          Other fields can be ignored.
 *
 *     @param vap_name
 *     @rts value
 *     @rts flags
 *     @return
 */
uint32_t
acfg_set_rts(uint8_t *vapname, acfg_rts_t *rts);

/**
 *   @brief Set fragmentation threshold.
 *
 *     @param vap_name
 *     @param frag
 *     @return
 */

uint32_t
acfg_set_frag(uint8_t *vapname, acfg_frag_t *frag);

/**
 * @brief Get Fragmentation threshold
 *
 * @param wifi_name
 * @param iwparam
 *
 * @return
 */
uint32_t
acfg_get_frag(uint8_t *vap_name, acfg_frag_t *frag);

uint32_t
acfg_set_txpow(uint8_t *vapname, acfg_txpow_t *txpow, uint32_t fstype);


/**
 * @brief Get default Tx Power in dBm
 *
 * @param wifi_name
 * @param txpow
 */
uint32_t
acfg_get_txpow(uint8_t *wifi_name, acfg_txpow_t *txpow);

/**
 * @brief Get Access Point MAC Address
 *
 * @param vap_name
 * @param mac
 */
uint32_t
acfg_get_ap(uint8_t *vap_name, acfg_macaddr_t * mac);

/**
 * @brief Set Encode Information
 *
 * @param vap_name
 * @param flag
 * @param len
 * @param enc
 */
uint32_t
acfg_set_enc(uint8_t *vap_name, acfg_encode_flags_t flag, char *enc);

/**
 * @brief set Vap vendor param
 *
 * @param vap_name
 * @param param
 * @param data
 * @param len
 * @param type
 * @param reinit
 * @return
 */
uint32_t
acfg_set_vap_vendor_param(uint8_t *vap_name, \
        acfg_vendor_param_vap_t param, uint8_t *data,
        uint32_t len, uint32_t type, acfg_vendor_param_init_flag_t reinit);


/**
 * @brief Set Vap param
 *
 * @param vap_name  -   vap name
 * @param param     -   param id
 * @param val       -   value
 *
 * @return
 */
uint32_t
acfg_set_vap_param(uint8_t *vap_name, \
        acfg_param_vap_t param, uint32_t val);


/**
 * @brief Get Vap param
 *
 * @param vap_name  -   vap name
 * @param param     -   param id
 * @param val       -   pointer to memory for return value
 *
 * @return
 */
uint32_t
acfg_get_vap_param(uint8_t *vap_name, \
        acfg_param_vap_t param, uint32_t *val);

/**
 * @brief Set Radio param
 *
 * @param radio_name    -   radio name
 * @param param         -   param id
 * @param val           -   value
 *
 * @return
 */
uint32_t
acfg_set_radio_param(uint8_t *radio_name, \
        acfg_param_radio_t param, uint32_t val);

/**
 * @brief Get Radio param
 *
 * @param radio_name    -   radio name
 * @param param         -   param id
 * @param val           -   pointer to memory for return value
 *
 * @return
 */
uint32_t
acfg_get_radio_param(uint8_t *radio_name, \
        acfg_param_radio_t param, uint32_t *val);

uint32_t
acfg_set_rate(uint8_t *vap_name, acfg_rate_t *rate);

/**
 * @brief Get default bit rate
 *
 * @param vap_name
 * @param mac
 *
 * @return
 */
uint32_t
acfg_get_rate(uint8_t *vap_name, uint32_t *rate);

/**
 * @brief Enable MU Grouping per tidmask
 *
 * @param vap_name  (Upto ACFG_MAX_IFNAME)
 *        mac
 *        tidmask
 *
 * @return On Success
 *             If : A_STATUS_OK
 *             Else: A_STATUS_FAILED
 *         On Error: A_STATUS_EFAULT
 */
uint32_t
acfg_set_mu_whtlist(uint8_t *vap_name, uint8_t *mac, uint16_t tidmask);

/**
 * @brief Set the phymode
 *
 * @param vap_name
 * @param mode
 * @param link_id
 *
 * @return
 */
uint32_t
acfg_set_phymode(uint8_t *vap_name, uint32_t mode, uint8_t link_id);



/**
 * @brief Set Reg value
 *
 * @param radio_name    -   radio name
 * @param offset        -   offset
 * @param val           -   value
 *
 * @return
 */
uint32_t
acfg_set_reg(uint8_t *radio_name, \
        uint32_t offset, uint32_t val);

/**
 * @brief Get Reg Value
 *
 * @param radio_name    -   radio name
 * @param offset        -   offset
 * @param val           -   pointer to memory for return value
 *
 * @return
 */
uint32_t
acfg_get_reg(uint8_t *radio_name, \
        uint32_t param, uint32_t *val);



uint32_t
acfg_hostapd_set_wpa(uint8_t *argv[]);


/*
 * Functions pertaining to adding/removing/getting
 * mac addresses from the first ACL list.
 */
uint32_t
acfg_acl_addmac(uint8_t *vap_name, uint8_t *addr);

uint32_t
acfg_acl_getmac(uint8_t *vap_name, acfg_macacl_t *maclist);

uint32_t
acfg_acl_delmac(uint8_t *vap_name, uint8_t *addr);

/*
 * Functions pertaining to adding/removing/getting
 * mac addresses from the secondary ACL list.
 */
uint32_t
acfg_acl_addmac_secondary(uint8_t *vap_name, uint8_t *addr);

uint32_t
acfg_acl_getmac_secondary(uint8_t *vap_name, acfg_macacl_t *maclist);

uint32_t
acfg_acl_delmac_secondary(uint8_t *vap_name, uint8_t *addr);

uint32_t
acfg_set_ap(uint8_t *vap_name, uint8_t *addr);

void
acfg_wlan_vap_profile_print (acfg_wlan_profile_vap_params_t *vap_params);

void
acfg_wlan_profile_print (acfg_wlan_profile_t *profile);

/**
 * acfg_init_mld_params() - Initialize MLD params to default values
 *
 * Init vap, radio params in mld to dedault values.
 *
 * Return: Nothing.
 */
void acfg_init_mld_params();

/**
 * acfg_alloc_mlo_profile() - Allocate memory for MLO data structure.
 *
 * It also memsets MLO data to 0.
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_NOMEM on failure.
 */
uint32_t acfg_alloc_mlo_profile();

/**
 * acfg_alloc_mlo_profile() - Free MLO data structure.
 *
 * Frees MLO data structure and sets it to NULL.
 *
 * Return: Nothing
 */
void acfg_free_mlo_profile();

/**
 * acfg_read_mld_file() - Open and read mld bin file
 *
 * This API tries to open mld bin file. If successful,
 * reads it
 *
 * @curr_mld: Output buf to store read mld bin file
 *
 * Return: QDF_STATUS_SUCCESS on success open and read.
 *         QDF_STATUS_E_NOMEM on failure.
 */
uint32_t acfg_read_mld_file(struct mld_profile *curr_mld);

/**
 * acfg_read_mld_file() - Open and write mld bin file
 *
 * This API tries to open/create mld bin file. If successful,
 * writes into it.
 *
 * Return: QDF_STATUS_SUCCESS on successful write.
 *         QDF_STATUS_E_NOMEM on failure.
 */
uint32_t acfg_write_mld_file();

/**
 * acfg_read_mld_file() - Build MLD list
 *
 * This API adds new mld and mld vaps to the mld list
 *
 * @vap_param: New vap param
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_NOMEM on failure if max allowed mld or mld vaps reached.
 */
uint32_t acfg_build_mld_list(acfg_wlan_profile_vap_params_t *vap_param);

/**
 * acfg_get_radio_param_from_name() - Get radio_params from name
 *
 * This helper function gets the radio_params from radio interface name.
 *
 * @mld_list_addr: Pointer to mld_profile
 * @radio_name: Radio interface name.
 *
 * Return: Pointer to acfg_wlan_profile_radio_params_t.
 *         NULL on failure.
 */
acfg_wlan_profile_radio_params_t*
acfg_get_radio_param_from_name(struct mld_profile *mld_list_addr,
                               char *radio_name);

/**
 * acfg_find_mlo_vap_from_name() - Find MLO VAP from name
 *
 * This helper function finds MLO VAP based on interface name.
 *
 * @mld_id: MLD ID to which VAP belongs
 * @vap_name: Interface name of the VAP to be searched.
 *
 * Return: Pointer to acfg_wlan_profile_vap_params_t.
 *         NULL on failure.
 */
acfg_wlan_profile_vap_params_t*
acfg_find_mlo_vap_from_name(uint8_t mld_id, uint8_t *vap_name);

/**
 * acfg_read_mld_mac_list() - Read mld mac address list from config
 *
 * Read MLD mac addresses from config file seperated by commas.
 * Indexes of mld mac in config file should map to mld ids
 * E.g: mld_macs=00:11:22:33:44:55,aa:bb:cc:dd:ee:ff
 * here mld mac for mld id 0 is 00:11:22:33:44:55 and
 * mld mac for mld id 1 is aa:bb:cc:dd:ee:ff
 *
 * @profile: Pointer to profile
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_FAILURE on failure.
 */
uint32_t acfg_read_mld_mac_list(acfg_wlan_profile_t *profile);

/**
 * acfg_read_link_mac_link_id() - Read PHY name from /sys
 *
 * Read mapping from radio name ("wifiX") to phy name (phyX)
 * from sys file in location /sys/class/net/<radio_name>/phy80211/name
 *
 * @wifi_name: Radio interface name
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_FAILURE on failure.
 */
uint32_t acfg_read_phy_name(char *wifi_name);

/**
 * acfg_read_link_mac_link_id() - Read link MAC and link ID for MLD VAPs
 *
 * Reads link MAC from /sys/class/net/<interface_name>/address. Derives
 * link ID from radio interface to which this MLD VAP belongs.
 * E.g. If VAP belongs to wifi0, then link ID of that VAP will be 0.
 * This read params are later used by acfg_prepare_link_id_list()
 *
 * @vap_params: Pointer to VAP params
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_FAILURE on failure.
 */
uint32_t acfg_read_link_mac_link_id(acfg_wlan_profile_vap_params_t *vap_params);

/**
 * acfg_prepare_link_id_list() - Prepare link ID list for MLD.
 *
 * Prepare link ID list for an MLD by iterating through all VAP belongs
 * to that MLD.
 * E.g: If there are 3 links(MLD VAPs) with link id 0, 1 and 2, in an MLD,
 * this API iterates through all MLD VAPs reads link ids and prepares list
 * list will be 0 1 2.
 *
 * @mld_id: MLD ID of an MLD
 * @link_id_list: Buffer to put link IDs
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_FAILURE on failure.
 */
uint32_t acfg_prepare_link_id_list(uint8_t mld_id, unsigned char *link_id_list);

/**
 * acfg_prepare_link_mac_list() - Prepare link MAC list for MLD.
 *
 * Prepare link MAC list for an MLD by iterating through all VAP belongs
 * to that MLD.
 *
 * @mld_id: MLD ID of an MLD
 * @link_mac_list: Buffer to put link MACs
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_FAILURE on failure.
 */
uint32_t acfg_prepare_link_mac_list(uint8_t mld_id, unsigned char *link_mac_list);

/**
 * acfg_prepare_cur_vap_radio_param() - Prepare current vap and radio param
 *
 * Get current radio and vap configuration by reading
 * them from current mld bin file
 *
 * @mld_id: MLD ID of an MLD
 * @vap_name: vap interface name
 * @cur_mld_profile: Current mld profile read from mld bin file
 * @cur_vap_params: Output buf holding current vap param
 * @cur_radio_params: Output buf holding current radio param
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_FAILURE on failure.
 */
uint32_t acfg_prepare_cur_vap_radio_param(uint8_t mld_id, uint8_t *vap_name,
                                          struct mld_profile *cur_mld_profile,
                                          acfg_wlan_profile_vap_params_t *cur_vap_params,
                                          acfg_wlan_profile_radio_params_t *cur_radio_params);
/**
 * acfg_bring_down_up_6GHz_link_mld() - Bring down or up the 6GHz link MLD
 *
 * Bring down/up all mlds which contains 6GHz as one of their links
 *
 * @up_down: 0 for down, 1 for up
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_FAILURE on failure.
 */
void acfg_bring_down_up_6GHz_link_mld(uint8_t up_down);

/**
 * acfg_mld_bring_down_up() - Bring down or up the given mld.
 *
 * Bring down/up the given mld
 *
 * @mld_id: MLD ID of the mld to be brought down/up
 * @up_down: 0 for down, 1 for up
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_FAILURE on failure.
 */
uint32_t acfg_mld_bring_down_up(struct mld_params *mld, uint8_t up_down);

/**
 * acfg_save_mbss_info() - Save mbss info in mld list.
 *
 * Get mbss info from driver and save it in mld list
 * so that application can check if tx vap is down and take action.
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_FAILURE on failure.
 */
void acfg_save_mbss_info();

/**
 * acfg_is_vap_created() - Check if vap is created
 *
 * Check if vap is created by looking at hostap/supplicant
 * interface.
 *
 * @vap_param: vap param to check if it is created or not
 *
 * Return: False if vap is not created.
 *         True otherwise.
 */
bool acfg_is_vap_created(acfg_wlan_profile_vap_params_t *vap_param);

/**
 * acfg_mark_mld_for_restart() - Mark/unmark an mld for restart
 *
 * Mark/unmark mlds belongs to given radio for restart.
 *
 * @radio_name: radio interface name.
 * @set_reset: 1 to set for restart. 0 to reset flag.
 *
 * Return: Nothing.
 */
void acfg_mark_mld_for_restart(char *radio_name, bool set_reset);

/**
 * acfg_set_mlo_radio_profile_chan() - Sets channel related params
 *
 * Sets channel, beacon rate, phymode and MBSS Tx VAP (If 6GHz) for MLD VAP.
 * Also bring up VAPs.
 *
 * @cur_vap_params: Pointer to Current vap params.
 * @vap_params: Pointer to new VAP params.
 * @config_change: Flag indicating first time configuration or
 *                 profile modification
 * @tx_vap_down: Flag indicating tx_vap is going down.
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_FAILURE on failure.
 */
uint32_t acfg_set_mlo_radio_profile_chan(acfg_wlan_profile_vap_params_t *cur_vap_params,
                                         acfg_wlan_profile_vap_params_t *vap_params,
                                         bool config_change, bool *tx_vap_down);

/**
 * acfg_start_mlo_vaps() - Start MLO VAPs
 *
 * Start MLD VAPs
 *
 * @mld_param: Pointer to mld_param
 * @skip_mbss_config: Pointer to bool value. Used to skip MBSS Tx VAP config
                      for STA MLD
 * @update_sec_params: To indicate only security param update required.
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_FAILURE on failure.
 */
uint32_t acfg_start_mlo_vaps(struct mld_params *mld_param,
                             uint8_t *skip_mbss_config,
                             bool update_sec_params);

/**
 * acfg_start_mlo_vaps() - Start MLO VAPs
 *
 * This is helper API to start STA MLD first followed by AP MLD.
 *
 * @mld_param: Pointer to mld_param
 * @skip_mbss_config: Pointer to bool value. Used to skip MBSS Tx VAP config
                      for STA MLD
 *
 * Return: QDF_STATUS_SUCCESS on success.
 *         QDF_STATUS_E_FAILURE on failure.
 */
uint32_t acfg_start_mld();

uint32_t
acfg_set_profile(acfg_wlan_profile_t *new_profile,
        acfg_wlan_profile_t *cur_profile);

uint32_t
acfg_write_file(acfg_wlan_profile_t *new_profile);

uint32_t
acfg_reset_cur_profile(char *radio_name);

uint32_t
acfg_apply_profile(acfg_wlan_profile_t *new_profile);

acfg_wlan_profile_t * acfg_get_profile(char *radio_name);

uint32_t acfg_recover_profile(char *radio_name);

uint32_t acfg_standby_profile(char *radioname, char *vapname, uint8_t trigger);

void
acfg_free_profile(acfg_wlan_profile_t *profile);

uint32_t
acfg_get_current_profile (acfg_wlan_profile_t *profile);

void
acfg_init_profile(acfg_wlan_profile_t *profile);

void
acfg_init_radio_params (acfg_wlan_profile_radio_params_t *unspec_radio_params);

void
acfg_init_vap_params (acfg_wlan_profile_vap_params_t *unspec_vap_params);

void
acfg_mac_str_to_octet(uint8_t *mac_str, uint8_t *mac);

uint32_t
acfg_set_wps_pbc(char *ifname);

uint32_t
acfg_set_wps_pin(char *ifname, int action, char *pin, char *pin_txt,
        char *bssid);

uint32_t
acfg_wps_config(uint8_t *ifname, char *ssid,
        char *auth, char *encr, char *key);

void
acfg_open_dpp_config_file(uint8_t *ifname);

void
acfg_write_dpp_config_file(uint8_t *ifname, char *data);

uint32_t
acfg_dpp_update_sta_vap(uint8_t *ifname);

uint32_t
acfg_dpp_update_vap(uint8_t *ifname);

char *acfg_get_errstr(void);

uint32_t
acfg_set_radio_enable(uint8_t *ifname);

uint32_t
acfg_set_radio_disable(uint8_t *ifname);

uint32_t
acfg_set_country(uint8_t *radio_name, uint16_t country_code);

uint32_t
acfg_get_country(uint8_t *radio_name, uint32_t *country_code);

uint32_t
acfg_set_regdomain(uint8_t *radio_name, uint32_t regdomain);

uint32_t
acfg_get_regdomain(uint8_t *radio_name, uint32_t *regdomain);

uint32_t
acfg_set_tx_antenna(uint8_t *radio_name, uint16_t mask);

uint32_t
acfg_set_rx_antenna(uint8_t *radio_name, uint16_t mask);

uint32_t
acfg_set_shpreamble(uint8_t *radio_name, uint16_t shpreamble);

uint32_t
acfg_get_shpreamble(uint8_t *radio_name, uint32_t *shpreamble);

uint32_t
acfg_set_txpower_limit(uint8_t *radio_name, uint32_t band, uint32_t power);

uint32_t
acfg_set_chainmask(uint8_t *radio_name, uint32_t type, uint16_t mask);

uint32_t
acfg_config_radio(uint8_t *radio_name);

uint32_t
acfg_set_preamble(uint8_t  *vap_name, uint32_t preamble);

uint32_t
acfg_set_slot_time(uint8_t  *vap_name, uint32_t slot);

uint32_t
acfg_set_erp(uint8_t  *vap_name, uint32_t erp);

uint32_t
acfg_add_app_ie(uint8_t  *vap_name, const uint8_t *ie, uint32_t ie_len);

uint32_t
acfg_set_chanswitch(uint8_t  *wifi_name, uint8_t  chan_num, uint8_t  chan_band);

uint32_t
acfg_set_ratemask(uint8_t  *vap_name, uint32_t preamble, uint32_t mask_lower32,
                  uint32_t mask_higher32, uint32_t mask_lower32_2);

uint32_t
acfg_set_shslot(uint8_t *radio_name, uint16_t shslot);

uint32_t
acfg_get_shslot(uint8_t *radio_name, uint32_t *shslot);



uint32_t
acfg_tx99_tool(uint8_t  *wifi_name, acfg_tx99_data_t* tx99_data);

uint32_t
acfg_set_tx_power(uint8_t  *wifi_name, uint32_t mode, float power);

uint32_t
acfg_set_sens_level(uint8_t  *wifi_name, float sens_level);

uint32_t
acfg_gpio_config(uint8_t  *wifi_name, uint32_t gpio_num, uint32_t input, uint32_t pull_type, uint32_t intr_mode, uint32_t mux_config_val, uint32_t drive, uint32_t init_enable);

uint32_t
acfg_gpio_set(uint8_t  *wifi_name, uint32_t gpio_num, uint32_t set);

uint32_t
acfg_send_raw_pkt(uint8_t  *vap_name, uint8_t *pkt_buf, uint32_t len, uint8_t type, uint16_t chan, uint8_t chan_band, uint8_t nss, uint8_t preamble, uint8_t mcs, uint8_t retry, uint8_t power, uint16_t scan_dur, uint8_t tx_beamforming);



uint32_t
acfg_enable_amsdu(uint8_t *radio_name, uint16_t amsdutidmask);

uint32_t
acfg_enable_ampdu(uint8_t *radio_name, uint16_t ampdutidmask);

uint32_t
acfg_set_ctl_table(uint8_t  *wifi_name, uint32_t band, uint32_t len, uint8_t  *ctl_table);

uint32_t
acfg_set_sta_max_pwr_table(uint8_t *wifi_name, acfg_wlan_sta_pwr_t *sta_table);

uint32_t
acfg_set_power_table(uint8_t  *wifi_name, uint32_t freq_band, uint32_t is_ext,
                     uint32_t target_type, uint32_t sub_band, uint32_t end_of_update,
                     uint32_t len, int8_t  *power_table);

uint32_t
acfg_enable_dfs_puncturing(uint8_t *radio_name, uint8_t val);

uint32_t
acfg_get_dfs_puncturing(uint8_t *radio_name, uint8_t *dfs_puncture);

/**
 * @brief set basic & supported rates in beacon,
 * and use lowest basic rate as mgmt mgmt/bcast/mcast rates by default.
 * target_rates: an array of supported rates with bit7 set for basic rates.
 * num_of_rates: number of rates in the array
*/
uint32_t
acfg_set_op_support_rates(uint8_t  *radio_name, uint8_t *target_rates, uint32_t num_of_rates);

/**
 * @brief Set management/action frame retry limit
 *
 * @param
 * @radio_name: Physical radio interface
 * @limit:      Management/action frame retry limit
 *
 * @return
 */
uint32_t
acfg_set_mgmt_retry_limit(uint8_t *radio_name, uint8_t limit);

/**
 * @brief Get management/action frame retry limit
 *
 * @param
 * @radio_name: Physical radio interface
 * @limit:      Management/action frame retry limit
 *
 * @return
 */
uint32_t
acfg_get_mgmt_retry_limit(uint8_t *radio_name, uint8_t *limit);

/**
 * @brief get supported rates of the specified phymode for the radio
 * target_rates: return an array of supported rates with bit7 set for basic rates.
 * phymode : phymode
*/
uint32_t
acfg_get_radio_supported_rates(uint8_t  *radio_name, acfg_rateset_t *target_rates, uint32_t phymode);

/**
 * @brief get supported legacy rates from BEACON IE
 * target_rates: return an array of supported rates with bit7 set for basic rates.
*/
uint32_t
acfg_get_beacon_supported_rates(uint8_t  *vap_name, acfg_rateset_t *target_rates);


uint32_t
acfg_set_cca_threshold(uint8_t  *wifi_name, float cca_threshold);

uint32_t
acfg_get_nf_dbr_dbm_info(uint8_t  *wifi_name);

uint32_t
acfg_get_packet_power_info(uint8_t  *wifi_name, acfg_packet_power_param_t *param);


uint32_t
acfg_handle_wps_event(uint8_t *ifname, enum acfg_event_handler_type event);

uint32_t
acfg_set_bss_color(uint8_t *radio_name, uint32_t bsscolor, uint32_t override);

uint32_t
acfg_get_bss_color(uint8_t *radio_name, uint32_t *bsscolor);

uint32_t
acfg_set_acs_pcaconly(uint8_t *radio_name, uint32_t acs_pcaconly);

uint32_t
acfg_get_acs_pcaconly(uint8_t *radio_name, uint32_t *acs_pcaconly);

enum acfg_buf_type;
/**
 * @brief Send a generic buffer to kernel space
 *
 * @param
 * @radio_name radio interface
 * @buf  buffer to send
 * @len  buffer length
 * @buf_type type of buffer
 * @return
 */
uint32_t
acfg_add_generic_buf(uint8_t  *radio_name, const uint8_t *buf,
                     uint32_t len, enum acfg_buf_type buf_type);

/**
 * @brief Add a user RNR info entry to driver
 *
 * @radio_name radio interface
 * @buf  buffer to send (struct user_rnr_data, uid and struct ieee80211_rnr_nbr_ap_info)
 * @len  buffer length
 * @return
 */
uint32_t
acfg_add_rnr_entry(uint8_t *radio_name, const uint8_t *buf, uint32_t len);

/**
 * @brief Delete a user RNR info entry
 *
 * @radio_name radio interface
 * @buf  buffer to send (struct user_rnr_data, only uid field needed)
 * @len  buffer length
 * @return
 */
uint32_t
acfg_del_rnr_entry(uint8_t *radio_name, const uint8_t *buf, uint32_t len);

/**
 * @brief Dump all user RNR info entries
 *
 * @radio_name radio interface
 * @return
 */
uint32_t
acfg_dump_rnr_entries(uint8_t *radio_name);

#endif
