/*
 * Copyright (c) 2011,2017-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 *  Copyright (c) 2008 Atheros Communications Inc.
 * All Rights Reserved.
 */

#ifndef _NET80211_IEEE80211_REGDMN_H
#define _NET80211_IEEE80211_REGDMN_H

#include <ieee80211_var.h>
#ifdef CONFIG_AFC_SUPPORT
#include <wlan_reg_afc.h>
#endif
#include <reg_services_public_struct.h>
#ifdef QCA_R2P_UPDATE_ENABLED
#include <ol_if_athvar.h>

typedef int (*ol_ath_ucfg_rate2power_set_cb)(struct ol_ath_softc_net80211 *scn, ath_power_table_t *ptr);
#endif

int ieee80211_set_country_code(struct ieee80211com *ic, char* isoName, u_int16_t cc, enum ieee80211_clist_cmd cmd);
void ieee80211_update_spectrumrequirement(struct ieee80211vap *vap, bool *thread_started);
#ifdef WLAN_REG_PARTIAL_OFFLOAD
void ieee80211_set_regclassids(struct ieee80211com *ic, const u_int8_t *regclassids, u_int nregclass);
#endif

struct ieee80211_mode_info {
    bool is_auto;
    enum ieee80211_phymode combined_mode;
    enum phy_ch_width ch_width;
    enum reg_phymode r_phymode;
};

#define FULL_BW 20
#define HALF_BW 10
#define QRTR_BW  5
#define FREQ_OFFSET_TENMHZ 10

#define OPCLASS_TBL_MAX 9

#define BW_WITHIN(min, bw, max) ((min) <= (bw) && (bw) <= (max))
/* Get N bits from the index position */
#define REG_GET_BITS(_val,_index,_num_bits) (((_val) >> (_index)) & \
    ((1 << (_num_bits)) - 1))
/* Set a value to a variable, N bits from the index position */
#define REG_SET_BITS(_var,_index,_num_bits,_val)                    \
    do {                                                            \
    (_var) &= ~(((1 << (_num_bits)) - 1) << (_index));              \
    (_var) |= (((_val) & ((1 << (_num_bits)) - 1)) << (_index));    \
    } while (0)

#define HW_OP_CLASS  hwcap->opclasses[(*total_n_sup_opclass)]
#define AP_CAP       reg_ap_cap[idx]
#define MAP_OP_CHAN  op_chan[(*total_n_sup_opclass)]

/* Offset between two HT20 channels is 20MHz */
#define CHAN_HT40_OFFSET 20

#define MAX_CHANNELS_PER_OPERATING_CLASS  24

#ifdef WLAN_FEATURE_11BE
#define MAX_NUM_20MHZ_SUBCHANS 16
#else
#define MAX_NUM_20MHZ_SUBCHANS 8
#endif

#ifdef WLAN_FEATURE_11BE
#define IEEE80211_IS_CHAN_MODE_160(chan) \
    IEEE80211_IS_CHAN_11AC_VHT160(chan) || \
    IEEE80211_IS_CHAN_11AXA_HE160(chan) || \
    IEEE80211_IS_CHAN_11BEA_EHT160(chan)
#else
#define IEEE80211_IS_CHAN_MODE_160(chan) \
    IEEE80211_IS_CHAN_11AC_VHT160(chan) || \
    IEEE80211_IS_CHAN_11AXA_HE160(chan)
#endif

typedef enum {
	IEEE80211_MIN_2G_CHANNEL = 1,
	IEEE80211_MAX_2G_CHANNEL = 14,
	IEEE80211_MIN_5G_CHANNEL = 36,
	IEEE80211_MAX_5G_CHANNEL = 169,
} IEEE80211_MIN_MAX_CHANNELS;

/* Supported STA Bands*/
typedef enum {
	IEEE80211_2G_BAND,
	IEEE80211_5G_BAND,
	IEEE80211_6G_BAND,
	IEEE80211_INVALID_BAND,
} IEEE80211_STA_BAND;

int regdmn_get_current_chan_txpower(struct wlan_objmgr_pdev *pdev);

void regdmn_get_curr_chan_and_opclass(struct wlan_objmgr_vdev *vdev,
                                      uint8_t *chan_num,
                                      uint8_t *opclass);

void regdmn_get_supp_opclass_list(struct wlan_objmgr_pdev *pdev,
                                  uint8_t *opclass_list,
                                  uint8_t *num_supp_op_class,
                                  bool global_tbl_lookup);

void ieee80211_set_chan_flags(struct ieee80211_ath_channel *chan,
			      enum channel_state state1,
			      enum channel_state state2,
			      qdf_freq_t freq);

u_int64_t ieee80211_mode_to_chanflags(enum ieee80211_phymode mode);
/* Get sta band capabilities from supporting opclass */
uint8_t regdmn_get_band_cap_from_op_class(uint8_t no_of_opclass,
                                          const  uint8_t *opclass);

/**
 * regdmn_convert_chanflags_to_chanwidth() - Convert channel flags to Channel width.
 * @chan - Channel structure.
 * Return - Channel width.
 */
uint32_t regdmn_convert_chanflags_to_chanwidth(struct ieee80211_ath_channel *chan);

uint16_t regdmn_get_min_6ghz_chan_freq(void);

uint16_t regdmn_get_max_6ghz_chan_freq(void);

uint16_t regdmn_get_min_5ghz_chan_freq(void);

uint16_t regdmn_get_max_5ghz_chan_freq(void);

void regdmn_get_channel_list_from_op_class(
        uint8_t reg_class,
        struct ieee80211_node *ni);
uint8_t regdmn_get_opclass (uint8_t *country_iso, struct ieee80211_ath_channel *channel);

uint8_t regdmn_get_map_opclass(struct wlan_objmgr_pdev *pdev,
                               struct wlan_radio_basic_capabilities *hwcap,
                               struct wlan_op_chan *op_chan,
                               struct wlan_op_class *op_class,
                               bool global_tbl_lookup,
                               bool dfs_required);

/** ieee80211_send_tpc_power_cmd() - Send EIRP-PSD/EIRP power-levels
 *  to the FW through the WMI_SET_TPC_POWER_CMDID.
 *  @vap - Pointer to vap.
 */
void ieee80211_send_tpc_power_cmd(struct ieee80211vap *vap);

#ifdef CONFIG_AFC_SUPPORT
/** ieee80211_send_afc_cmd() - Send AFC response received indication to
 *  the FW.
 *  @ic - Pointer to ic.
 *  @serv_resp_format - AFC Server response format
 *  @cmd_type - Type of AFC command send to FW
 */
QDF_STATUS
ieee80211_send_afc_cmd(struct ieee80211com *ic,
                       enum reg_afc_serv_resp_format serv_resp_format,
                       enum reg_afc_cmd_type cmd_type);
#endif

/**
 * ieee80211_get_start_index_for_sta_bw() - Find the TPE index from which
 * the power values should be read for the STA's operating BW.
 *
 * Example: Consider AP operating in HT160 MHz and STA connected to it in HT40 MHz
 * with the following power values in the TPE IE (P indicates primary channel):
 * |-----|---|---|---|---|---|---|-------|--|
 * |Chan#| 1 | 5 | 9 | 13| 17| 21| 25(P) |29|
 * | ----|---|---|---|---|---|---|-------|--|
 * |PSD  | 5 | 6 | 4 |18 |20 | -1| 7     |9 |
 * | ----|---|---|---|---|---|---|-------|--|
 *
 * The power values that the STA must read are 7 and 9 corresponding to channels
 * 25 and 29. Hence we need to find the index of the TPE IE from which
 * the STA must read/parse the TPE values and this API computes that using
 * the bonded channel intersection of AP and STA's BW.
 * The index is dependent on the AP-STA operating BW.
 *
 * @ap_phy_chwidth: AP's operating channel width in MHz
 * @sta_width: STA's operating channel width in MHz
 * @primary_freq: Channel primary Frequency
 * @cen320_freq: 320 BW Center Frequency
 *
 * Return: The channel index in the TPE IE from which the STA read the power
 * values.
 */
int8_t
ieee80211_get_start_index_for_sta_bw(uint16_t ap_width,
                                     uint16_t sta_width,
                                     qdf_freq_t primary_freq,
                                     qdf_freq_t cen320_freq);

/** ieee80211_fill_reg_tpc_obj() - Fill the reg_tpc_obj object that is sent to
 *  the FW through the WMI_SET_TPC_POWER_CMDID.
 *  @vap - Pointer to vap.
 */
QDF_STATUS ieee80211_fill_reg_tpc_obj(struct ieee80211vap *vap);

QDF_STATUS ieee80211_set_6G_opclass_triplets(struct ieee80211com *ic,
                                             uint16_t value);

/**
 * ieee80211_get_channel_list() - Fill channel list with primary channel
 * parameters and all supported flags.
 * @ic: Pointer to ieee80211_com.
 * @chan_list: Channel list to be filled.
 * @chan_info: Channel info
 * @nchans: Numbers of channels filled.
 * @flag_160: flag indicating the API to fill the center frequencies of 160MHz.
 *
 */
void ieee80211_get_channel_list(
        struct ieee80211com *ic,
        struct ieee80211_ath_channel *chan_list,
        struct ieee80211_channel_info *chan_info,
        int *nchans,
        bool flag_160);

#ifdef CONFIG_AFC_SUPPORT
/**
 * ieee80211_get_afc_channel_info()- Fill AFC channel information, i.e. power
 * types supported and power values to all the power types supported.
 * @ic: Pointer to ieee80211_com.
 * @chan_info: Channel info.
 * @primary_freq: Primary frequency to check for supported power types.
 * @pwr_type: Power type to be checked for given frequency.
 */
QDF_STATUS ieee80211_get_afc_channel_info(
                   struct ieee80211com *ic,
                   struct ieee80211_channel_params *chan_info,
                   qdf_freq_t primary_freq,
                   enum reg_6g_ap_type pwr_type);
#endif

/**
 * ieee80211_get_radio_channel_list() - Fill channel list with primary channel
 * parameters and all supported flags.
 * @ic: Pointer to ieee80211_com.
 * @chan_list: Channel list to be filled.
 * @chan_info: Channel info
 * @nchans: Numbers of channels filled.
 *
 */
void ieee80211_get_radio_channel_list(
        struct ieee80211com *ic,
        struct ieee80211_ath_channel *chan_list,
        struct ieee80211_channel_params *chan_info,
        int *nchans);

/**
 * ieee80211_is_80p80mhz_supported() - Find if 80p80mhz mode is supported
 * by the frequency and the radio.
 * @ic: Pointer to ieee80211com.
 * @primary_freq: Primary frequency to check for 80p80 supported.
 * @sec_cfreq: Output secondary center frequency.
 * @sec_cieee: Output secondary center IEEE channel.
 * @include_nol_chan: Flag to indicate whether NOL channels are considered.
 * @in_6g_pwr_mode: Input 6G power mode which indicates the 6G channel list that
 * is being used.
 *
 * Return true if there exists another 80MHz non contiguous channel for the
 * given frequency, else false.
 */
bool ieee80211_is_80p80mhz_supported(struct ieee80211com *ic,
                                     qdf_freq_t primary_freq,
                                     qdf_freq_t *sec_cfreq,
                                     uint8_t *sec_cieee,
                                     bool include_nol_chan,
                                     enum supported_6g_pwr_types in_6g_pwr_mode);

uint16_t ieee80211_get_max5gbw(struct ieee80211com *ic);

/**
 * ieee80211_get_default_psd_power - Fetch default PSD power values from the
 * regulatory component.
 * @vap - Pointer to vap.
 * @client -  6G client type.
 * @psd_pwr - Pointer to psd power.
 */
void ieee80211_get_default_psd_power(struct ieee80211vap *vap,
                                     enum reg_6g_client_type client_type,
                                     uint8_t *psd_pwr);

/**
 * ieee80211_get_chwidth_val() - Find the value in MHz for a given channel width
 * @ch_width: Channel width enum.
 */
int ieee80211_get_chwidth_val(enum phy_ch_width ch_width);

enum channel_state
regdmn_reg_get_bonded_channel_state_for_freq(struct wlan_objmgr_pdev *pdev,
                                             qdf_freq_t freq,
                                             qdf_freq_t sec_ch_freq_2g,
                                             struct ch_params *ch_params);

#ifdef CONFIG_AFC_SUPPORT
/**
 * ieee80211_allow_power_mode_change() - Check if the power mode change can be
 * allowed based on vaps and their states.
 * @ic: Pointer to structure struct ieee80211com
 *
 * Return: True, if power mode change can be allowed. False otherwise.
 */
bool
ieee80211_allow_power_mode_change(struct ieee80211com *ic);

/**
 * mlme_check_curchan_and_set_ap_pwr_type() - Checks if input channel 'chan'
 * is present in afc channel list. It changes the power mode if the channel is
 * present and based on the current ap mode.
 */
QDF_STATUS
mlme_check_curchan_and_set_ap_pwr_type(struct ieee80211com *ic,
                                       struct ieee80211_ath_channel *chan);
/**
 * ucfg_check_curchan_and_set_ap_pwr_type() - Checks if input channel 'chan'
 * is present in afc channel list. It changes the power mode if the channel is
 * present, based on the current ap mode, by sending the power values in TPC
 * command and updating the beacon. If the frequency in not present in the afc
 * channel ist then only the current channel list is changed. The BSS operation
 * on current channel is unaffected. It relies on a channel change command from
 * the user to properly update the current channel power. Note that this API
 * should be used only in channel change sequence.
 */
QDF_STATUS
ucfg_check_curchan_and_set_ap_pwr_type(struct ieee80211com *ic,
                                       struct ieee80211_ath_channel *chan,
                                       enum reg_6g_ap_type new_ap_pwr_type);

#else
static inline QDF_STATUS
mlme_check_curchan_and_set_ap_pwr_type(struct ieee80211com *ic,
                                       struct ieee80211_ath_channel *chan)
{
	return QDF_STATUS_E_FAILURE;
}

static inline bool
ieee80211_allow_power_mode_change(struct ieee80211com *ic)
{
	return false;
}

static inline QDF_STATUS
ucfg_check_curchan_and_set_ap_pwr_type(struct ieee80211com *ic,
                                       struct ieee80211_ath_channel *chan,
                                       enum reg_6g_ap_type new_ap_pwr_type)
{
	return QDF_STATUS_E_FAILURE;
}
#endif

/** ieee80211_get_num_pwr_levels() - Find the number of subchannels in a channel
 *  based on the input channel frequency and channel width.
 *  @pdev - Pointer to pdev.
 *  @freq - Channel frequency in MHz.
 *  @ch_width - Channel width.
 *  @is_psd - Boolean to indicate if PSD is supported or not.
 */
uint8_t ieee80211_get_num_pwr_levels(struct wlan_objmgr_pdev *pdev,
                                     qdf_freq_t freq,
                                     enum ieee80211_cwm_width ch_width,
				     bool is_psd);

/**
 * ieee80211_get_phymode_for_ch_flags() - Get phymode for the given channel flag
 * @ch_flags: Channel flag
 *
 * Return: phymode
 */
enum ieee80211_phymode ieee80211_get_phymode_for_ch_flags(uint64_t ch_flags);

#ifdef WLAN_FEATURE_11BE
/**
 * ieee80211_get_phymode_for_channel_flags() - Get phymode for the given channel
 * flag
 * @ch_flags: Channel flag
 *
 * Return: phymode
 */
enum ieee80211_phymode ieee80211_get_phymode_for_channel_flags(
        uint64_t ch_flags);
#endif /* WLAN_FEATURE_11BE */

#if defined(CONFIG_AFC_SUPPORT) && defined(CONFIG_REG_6G_PWRMODE)
/**
 * ieee80211_apply_best_6g_ap_pwr_mode() - Get the best power mode for the 6G
 * frequency and apply it as the 6G power type for the radio.
 *
 * @ic: Pointer to the ic.
 * @freq: input 6G frequency.
 * @cen320: 320MHZ band center frequency in MHZ
 * @restart_vaps: bool indicating whether or not to restart vaps immediately.
 * @phymode: Phymode
 * @in_punc_pattern: Input puncture pattern
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
ieee80211_apply_best_6g_ap_pwr_mode(struct ieee80211com *ic, qdf_freq_t freq,
                                    qdf_freq_t cen320,
                                    bool restart_vaps,
                                    enum ieee80211_phymode phymode,
                                    uint16_t in_punc_pattern);
#else
static inline QDF_STATUS
ieee80211_apply_best_6g_ap_pwr_mode(struct ieee80211com *ic, qdf_freq_t freq,
                                    qdf_freq_t cen320,
                                    bool restart_vaps,
                                    enum ieee80211_phymode phymode,
                                    uint16_t in_punc_pattern)
{
    return QDF_STATUS_SUCCESS;
}
#endif /* CONFIG_AFC_SUPPORT */

/**
 * ieee80211_quick_apply_best_6g_ap_pwr_mode() - Get the best power mode for the
 * 6G frequency and apply it as the 6G power type for the radio. Use the
 * minimalistic regulatory quick APIs optimized for special vap.
 *
 * @ic: Pointer to the ic.
 * @freq: input 6G frequency.
 * @cen320: 320MHZ band center frequency in MHz.
 * @send_tpc: bool indicating whether to send tpc or not. If send_tpc is false,
 * tpc command should be sent as part vdev restart.
 * @phymode: Phymode.
 * @in_punc_pattern: Input puncture pattern.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
ieee80211_quick_apply_best_6g_ap_pwr_mode(struct ieee80211com *ic,
                                          qdf_freq_t freq,
                                          qdf_freq_t cen320,
                                          bool send_tpc,
                                          enum ieee80211_phymode phymode,
                                          uint16_t in_puncture_bitmap);

/**
 * regdmn_get_opclass_for_hwmode() - Fill opclass info for the current hwmode
 * @pdev: Pointer to struct wlan_objmgr_pdev
 * @apcap: Pointer to wlan_radio_basic_capabilities where opclass info is filled
 * @global_tbl_lookup: bool to indicate global tbl lookup
 *
 * Return - None
 */
void regdmn_get_opclass_for_hwmode(struct wlan_objmgr_pdev *pdev,
                                   wlan_radio_basic_capabilities *apcap,
                                   bool global_tbl_lookup);

/**
 * ieee80211_cfg80211_update_channel_list: Wrapper to populate update channelist in wiphy
 * @data: pointer to data
 *
 * Return - None
 */
void ieee80211_cfg80211_update_channel_list(void *data);

/**
 * ieee80211_reg_get_num_ap_vaps: Reguatory API to get number of AP vaps in a
 * radio.
 * @ic: pointer to ic
 *
 * Return - Number of AP vaps.
 */
int ieee80211_reg_get_num_ap_vaps(struct ieee80211com *ic);

/**
 * ieee80211_reg_is_pdev_ap_no_sta: Regulatory API to determine if a radio
 * consists of only AP vaps. This API ignores the presence of a Monitor vap.
 * @ic: pointer to ic
 *
 * Return - True, if radio has only AP vaps. False, if there is a STA vap.
 */
bool ieee80211_reg_is_pdev_ap_no_sta(struct ieee80211com *ic);

/**
 * ieee80211_reg_is_repeater_pdev: Regulatory API to determine if a radio
 * consists of a repeater i.e. an AP vap exist along with a STA vap.
 * @ic: pointer to ic
 *
 * Return - True, if there is a repeater. False, if not.
 */
bool ieee80211_reg_is_repeater_pdev(struct ieee80211com *ic);
#ifdef CONFIG_BAND_6GHZ
/**
 * ieee80211_reg_get_eirp_arr_for_6g() - Fill 6G EIRP values for a frequency
 * based on the AP and client type. If the input channel is punctured, the
 * effective bandwidth EIRP is found if the puncture pattern is valid/invalid.
 * @vap: Pointer to vap
 * @ap_pwr_type: AP power type
 * @is_client_pwr_needed: Whether client chan list lookup is needed
 * @client_type: Client type
 * @max_eirp_arr: Pointer to max_eirp_arr
 * @chan: Pointer to struct ieee80211_ath_channel
 * @is_twice_power: Boolean to indicate EIRP Power in 0.5dBM
 *
 * Return - QDF_STATUS
 */
QDF_STATUS
ieee80211_reg_get_eirp_arr_for_6g(struct ieee80211vap *vap,
                                  enum reg_6g_ap_type ap_pwr_type,
                                  bool is_client_pwr_needed,
                                  enum reg_6g_client_type client_type,
                                  int8_t *max_eirp_arr,
                                  struct ieee80211_ath_channel *chan,
                                  bool is_twice_power);

/**
 * ieee80211_reg_get_eirp_arr_for_6g_compap() - Fill 6G EIRP values for a frequency
 * based on the Composite AP and client type.
 * @vap: Pointer to vap
 * @ap_pwr_type: AP power type
 * @is_client_pwr_needed: Whether client chan list lookup is needed
 * @client_type: Client type
 * @freq: Frequency
 * @max_eirp_arr: Pointer to max_eirp_arr
 * @chan: Pointer to struct ieee80211_ath_channel
 *
 * Return - QDF_STATUS
 */
QDF_STATUS ieee80211_reg_get_eirp_arr_for_6g_compap(struct ieee80211vap *vap,
                                                    enum reg_6g_ap_type ap_pwr_type,
                                                    bool is_client_pwr_needed,
                                                    enum reg_6g_client_type client_type,
                                                    qdf_freq_t freq,
                                                    int8_t *max_eirp_arr,
                                                    struct ieee80211_ath_channel *chan);

/**
* ieee80211_get_psd_from_eirp() - Get PSD from the input EIRP values
* by finding the maximum of all PSDs calculated from every bandwidth's EIRP
*
* @param vap            pointer to vap
* @max_reg_psd_pwr      pointer to max_reg_psd_pwr
* @max_eirp_arr         pointer to max_eirp_arr
* @is_twice_eirp_power  Boolean to indicate if EIRP power is in 0.5dbm steps
*
* @return: Void
*/
void
ieee80211_get_psd_from_eirp(struct ieee80211vap *vap,
                            int16_t *max_reg_psd_pwr,
                            int8_t *max_eirp_arr,
                            bool is_twice_eirp_power);
#endif

#ifdef CONFIG_AFC_SUPPORT
/** ieee80211_get_dev_deployment_type() - Get device deployment type for pdev
 *
 *  @pdev: pdev pointer
 *  @reg_afc_dev_type: pointer to deployment type
 *
 *  Return: 0 for success; -EINVAL otherwise
 */
int
ieee80211_get_dev_deployment_type(
        struct wlan_objmgr_pdev *pdev,
        enum reg_afc_dev_deploy_type *reg_afc_dev_type);
#endif

/**
 * regdmn_enable_disable_opclass_chans: Disable or enable the channels present in
 * the input 20MHz operating class
 * @pdev: pointer to pdev
 * @is_disable: Boolean to disable or enable channels
 * @ieee_chan_list: Pointer to ieee_chan_list
 * @chan_list_size: Size of the ieee_chan_list
 *
 * Return - int
 */
int regdmn_enable_disable_opclass_chans(struct ieee80211com *ic,
                                        bool is_disable,
                                        uint8_t opclass,
                                        uint8_t *ieee_chan_list,
                                        uint8_t chan_list_size);

#ifdef QCA_R2P_UPDATE_ENABLED
/**
 * ieee80211_read_set_home_chan_r2p_table() - Read the home chan r2p table file
 * and send it to target through WMI
 * @data: Pointer to ic strucure
 *
 * Return: void
 */
void ieee80211_read_set_home_chan_r2p_table(void *data);

/**
 * ieee80211_send_homechan_r2p_table: Parse the home channel r2p table binary
 * file contents and send it to the target through WMI
 * @scn: scn handle
 * @buff: Input buff with home chan r2p table
 * @buff_size: Size of the input buffer
 * @rate2power_set_cb: Call back to send the r2p table to FW
 *
 * Return - QDF_STATUS
 */
QDF_STATUS ieee80211_send_homechan_r2p_table(struct ol_ath_softc_net80211 *scn,
                                             char *buff, u_int32_t buff_size,
                                             ol_ath_ucfg_rate2power_set_cb rate2power_set_cb);
#endif

/**
 * ieee80211_reg_get_radar_punc_bitmap() - If the input chan is radar infected,
 * puncture the NOL infected channels in the input chan and return the
 * puncture bitmap.
 *
 * @ic: Pointer to struct ieee80211com
 * @chan: Pointer to  struct ieee80211_ath_channel
 * Return - Puncture bitmap of NOL infected channels
 */
#ifdef WLAN_FEATURE_11BE
uint16_t ieee80211_reg_get_radar_punc_bitmap(struct ieee80211com *ic,
                                             struct ieee80211_ath_channel *chan);

/**
 * ieee80211_intersect_puncture_bitmap_for_40() - Given a channel with a certain
 * puncturing pattern and bandwidth, find the puncturing pattern for a reduced
 * bandwidth of 40MHz. Eventhough, standard does not allow puncturing pattern
 * for 40MHz, it is considered valid for cases where the sub-channel next to the
 * primary channel is punctured.
 *
 * @chan: Pointer to channel.
 * @input_phymode: Phymode of the channel.
 * @target_bw: Target bandwidth for which the input channel's puncturing pattern
 * has to be reduced.
 *
 * Return : Punturing pattern of the reduced bandwidth of 40MHz.
 */
uint16_t
ieee80211_intersect_puncture_bitmap_for_40(struct ieee80211_ath_channel *chan,
                                           enum ieee80211_phymode input_phymode);

/**
 * ieee80211_get_punc_pattern_for_bw() - Given a channel with a certain
 * puncturing pattern and bandwidth, find the puncturing pattern for a reduced
 * bandwidth.
 *
 * @chan: Pointer to channel.
 * @input_phymode: Phymode of the channel.
 * @target_bw: Target bandwidth for which the input channel's puncturing pattern
 * has to be reduced.
 *
 * Return : Punturing pattern of the reduced bandwidth.
 */
uint16_t
ieee80211_get_punc_pattern_for_bw(struct ieee80211_ath_channel *chan,
                                  enum ieee80211_phymode input_phymode,
                                  uint16_t target_bw);
#else
static inline uint16_t
ieee80211_reg_get_radar_punc_bitmap(struct ieee80211com *ic,
                                    struct ieee80211_ath_channel *chan)
{
    return 0;
}

static inline uint16_t
ieee80211_intersect_puncture_bitmap_for_40(struct ieee80211_ath_channel *chan,
                                           enum ieee80211_phymode input_phymode)
{
    return 0;
}

static inline uint16_t
ieee80211_get_punc_pattern_for_bw(struct ieee80211_ath_channel *chan,
                                  enum ieee80211_phymode input_phymode,
                                  uint16_t target_bw)
{
    return 0;
}
#endif

/**
 * ieee80211_display_channel() - Display channel list for the specific band.
 *
 * @ic: Pointer to struct ieee80211com
 * Return - void
 */
void ieee80211_display_channel(struct ieee80211com *ic);

QDF_STATUS
reg_get_max_chwidth_for_cur_hwmode(struct wlan_objmgr_pdev *pdev,
                                   enum phy_ch_width *max_chwidth);


/* ieee80211_is_chwidth_supported_by_reg: Get the maximum supported
 *              regulatory bandwidth in the current regulatory domain
 *              and compare it against the input channel width
 *              (in_chwidth) parameter. If the maximum supported
 *              regulatory bandwidth is greater than or equal to the
 *              given input channel width (in_chwidth) then return
 *              true. This signifies the input channel width (in_chwidth)
 *              is supported in the current regulatory domain.
 *              Else return false. False signifes the input channel width
 *              is not supported in the current regulatory domain.
 *
 * @ic        : pointer to ic
 * @in_chwidth: input channel width to verify if this is supported
 *              in the current regulatory domain
 * Returns    : true if the input channel width (in_chwidth)
 *              is supported in the current regulatory domain else false.
 */
uint8_t ieee80211_is_chwidth_supported_by_reg(struct ieee80211com *ic,
                                             enum phy_ch_width in_chwidth);

#endif /* _NET80211_IEEE80211_REGDMN_H */
