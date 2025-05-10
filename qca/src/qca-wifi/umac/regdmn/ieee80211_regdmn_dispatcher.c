/*
 * Copyright (c) 2017-2018, 2020 Qualcomm Innovation Center, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 */

#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_reg_ucfg_api.h>
#include <wlan_reg_services_api.h>
#include <wlan_reg_channel_api.h>
#include <reg_services_public_struct.h>

int ieee80211_regdmn_program_cc(struct wlan_objmgr_pdev *pdev,
		struct cc_regdmn_s *rd)
{
	return ucfg_reg_program_cc(pdev, rd);
}

QDF_STATUS ieee80211_regdmn_get_chip_mode(struct wlan_objmgr_pdev *pdev,
		uint64_t *chip_mode)
{
	return wlan_reg_get_chip_mode(pdev, chip_mode);
}

QDF_STATUS ieee80211_regdmn_get_phybitmap(struct wlan_objmgr_pdev *pdev,
                                          uint16_t *phybitmap)
{
        return wlan_reg_get_phybitmap(pdev, phybitmap);
}

QDF_STATUS ieee80211_regdmn_get_freq_range(struct wlan_objmgr_pdev *pdev,
		qdf_freq_t *low_2g,
		qdf_freq_t *high_2g,
		qdf_freq_t *low_5g,
		qdf_freq_t *high_5g)
{
	return wlan_reg_get_freq_range(pdev, low_2g, high_2g, low_5g, high_5g);
}

QDF_STATUS ieee80211_regdmn_get_current_chan_list(struct wlan_objmgr_pdev *pdev,
		struct regulatory_channel *chan_list)
{
	return ucfg_reg_get_current_chan_list(pdev, chan_list);
}

QDF_STATUS ieee80211_regdmn_get_current_cc(struct wlan_objmgr_pdev *pdev,
		struct cc_regdmn_s *rd)
{
	return ucfg_reg_get_current_cc(pdev, rd);
}

enum channel_state ieee80211_regdmn_get_5g_bonded_channel_state_for_freq(
                struct wlan_objmgr_pdev *pdev, qdf_freq_t freq,
                struct ch_params *ch_params)
{
	return wlan_reg_get_5g_bonded_channel_state_for_pwrmode(pdev, freq,
								ch_params,
								REG_CURRENT_PWR_MODE);
}

enum channel_state ieee80211_regdmn_get_2g_bonded_channel_state_for_freq(
                struct wlan_objmgr_pdev *pdev, qdf_freq_t freq,
                qdf_freq_t sec_ch_freq, enum phy_ch_width bw)
{
        return wlan_reg_get_2g_bonded_channel_state_for_freq(pdev, freq, sec_ch_freq, bw);
}

void ieee80211_regdmn_set_channel_params_for_freq(struct wlan_objmgr_pdev *pdev,
                                                  qdf_freq_t freq,
                                                  qdf_freq_t sec_ch_2g_freq,
                                                  struct ch_params *ch_params,
                                                  enum supported_6g_pwr_types in_6g_pwr_mode)
{
    return wlan_reg_set_channel_params_for_pwrmode(pdev, freq, sec_ch_2g_freq, ch_params, in_6g_pwr_mode);
}

#ifdef WLAN_FEATURE_11BE
void ieee80211_regdmn_reg_fill_channel_list(struct wlan_objmgr_pdev *pdev,
                                            qdf_freq_t freq,
                                            qdf_freq_t sec_ch_2g_freq,
                                            enum phy_ch_width ch_width,
                                            qdf_freq_t band_center_320,
                                            struct reg_channel_list *chan_list,
                                            enum supported_6g_pwr_types in_6g_pwr_mode,
                                            bool treat_nol_chan_as_disabled)
{
	return wlan_reg_fill_channel_list_for_pwrmode(pdev,
						      freq, sec_ch_2g_freq,
						      ch_width, band_center_320,
						      chan_list,
						      in_6g_pwr_mode,
						      treat_nol_chan_as_disabled);
}
#endif

void ieee80211_regdmn_get_channel_params(struct wlan_objmgr_pdev *pdev,
                                         qdf_freq_t freq,
                                         qdf_freq_t sec_ch_2g_freq,
                                         struct ch_params *ch_params,
                                         enum supported_6g_pwr_types in_6g_pwr_mode)
{
    return wlan_reg_get_channel_params(pdev, freq, sec_ch_2g_freq, ch_params, in_6g_pwr_mode);
}

int ieee80211_regdmn_get_dfs_region(struct wlan_objmgr_pdev *pdev,
		enum dfs_reg *dfs_region)
{
	return wlan_reg_get_dfs_region(pdev, dfs_region);
}
