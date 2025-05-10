#
## Copyright (c) 2014, 2017, 2019-2023 Qualcomm Innovation Center, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Innovation Center, Inc.
#
# 2014 Qualcomm Atheros, Inc..
#
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.
#

LINUX_KMOD_SUFFIX:=ko
QCAWLAN_MODULE_LIST:=
PWD:=$(shell pwd)
# These two functions are used to define options based on WLAN config
if_opt_set=       $(if $(filter $(1)=1,$(QCAWLAN_MAKEOPTS)),$(2),)
if_opt_clear=     $(if $(filter $(1)=0,$(QCAWLAN_MAKEOPTS)),$(2),)

# Use the function below to add driver opts depending on menuconfig values
append_if_notnull=QCAWLAN_MAKEOPTS+=$(if $(call qstrip,$(1)),$(2),$(3))

ifneq ($(BUILD_VARIANT),)
ifneq ($(DRIVER_PATH),)
QCAWLAN_MAKEOPTS:=$(shell cat $(DRIVER_PATH)/os/linux/configs/config.wlan.$(subst -,.,$(BUILD_VARIANT)))
else
QCAWLAN_MAKEOPTS:=$(shell cat $(PWD)/os/linux/configs/config.wlan.$(subst -,.,$(BUILD_VARIANT)))
endif
endif

# If this is wkk profile, add unified profile on top of wkk profile
ifeq ($(BUILD_VARIANT),wkk-profile)
ifneq ($(DRIVER_PATH),)
QCAWLAN_MAKEOPTS+=$(shell cat $(DRIVER_PATH)/os/linux/configs/config.wlan.unified.profile)
else
QCAWLAN_MAKEOPTS+=$(shell cat $(PWD)/os/linux/configs/config.wlan.unified.profile)
endif
endif

# If this is host only profile, add unified profile on top of it
ifeq ($(BUILD_VARIANT),unified-profile-nonss)
ifneq ($(DRIVER_PATH),)
QCAWLAN_MAKEOPTS+=$(shell cat $(DRIVER_PATH)/os/linux/configs/config.wlan.unified.profile)
else
QCAWLAN_MAKEOPTS+=$(shell cat $(PWD)/os/linux/configs/config.wlan.unified.profile)
endif
endif

# If this is lowmem host only profile, add lowmem profile on top of it
ifeq ($(BUILD_VARIANT),lowmem-profile-nonss)
ifneq ($(DRIVER_PATH),)
QCAWLAN_MAKEOPTS+=$(shell cat $(DRIVER_PATH)/os/linux/configs/config.wlan.lowmem.profile)
else
QCAWLAN_MAKEOPTS+=$(shell cat $(PWD)/os/linux/configs/config.wlan.lowmem.profile)
endif
endif

ifeq ($(CONFIG_BUILD_YOCTO),y)
QCAWLAN_MAKEOPTS+=WIFISTATS_TOOL_SUPPORT=1
QCAWLAN_MAKEOPTS+=CONFIG_LITHIUM=1
else # CONFIG_BUILD_YOCTO is false
ifeq ($(CONFIG_WIFI_TARGET_WIFI_3_0),y)
ifneq ($(CONFIG_LOWMEM_FLASH),y)
QCAWLAN_MAKEOPTS+=WIFISTATS_TOOL_SUPPORT=1
else
ifeq ($(CONFIG_DEBUG),y)
QCAWLAN_MAKEOPTS+=WIFISTATS_TOOL_SUPPORT=1
QCAWLAN_MAKEOPTS+=BE_PKTLOG_SUPPORT=1
QCAWLAN_MAKEOPTS+=REMOVE_PKT_LOG=0
else
QCAWLAN_MAKEOPTS+=BE_PKTLOG_SUPPORT=0
QCAWLAN_MAKEOPTS+=REMOVE_PKT_LOG=1
endif
endif
endif
endif # CONFIG_BUILD_YOCTO

QCAWLAN_HEADERS:= \
   cmn_dev/qdf/inc/qdf_types.h \
   cmn_dev/qdf/inc/qdf_status.h \
   cmn_dev/qdf/linux/src/i_qdf_types.h \
   offload/include/a_types.h \
   offload/include/athdefs.h \
   offload/include/ol_txrx_stats.h \
   cmn_dev/dp/inc/cdp_txrx_stats_struct.h \
   offload/wlan/lmac_offload_if/ol_ath_ucfg.h \
   include/ieee80211_defines.h \
   include/_ieee80211.h \
   include/ieee80211.h \
   include/ieee80211_wnm_proto.h \
   include/if_upperproto.h \
   include/compat.h \
   include/wlan_opts.h \
   include/sys/queue.h \
   include/ieee80211_phytype.h \
   include/ext_ioctl_drv_if.h \
   include/ol_if_thermal.h \
   include/qwrap_structure.h \
   include/ieee80211_rrm.h \
   os/linux/src/ah_osdep.h \
   os/linux/include/ieee80211_external.h \
   os/linux/include/ieee80211_ioctl.h \
   os/linux/include/ieee80211_ev.h \
   os/linux/include/ieee80211_wpc.h \
   os/linux/tools/athrs_ctrl.h \
   offload/wlan/lmac_offload_if/if_athioctl.h \
   cmn_dev/spectral/dispatcher/inc/spectral_ioctl.h \
   include/acfg_wsupp_api.h \
   include/acfg_event_types.h \
   include/acfg_event.h \
   include/acfg_api_types.h \
   include/appbr_types.h \
   include/bypass_types.h \
   os/linux/src/ath_papi.h \
   include/ieee80211_parameter_api.h \
   cmn_dev/umac/dfs/dispatcher/inc/wlan_dfs_ioctl.h \
   umac/airtime_fairness/dispatcher/inc/wlan_atf_utils_defs.h \
   os/linux/src/ath_ssid_steering.h \
   os/linux/src/cfg80211_external.h \
   os/linux/tools/qcatools_lib.h \
   os/linux/tools/apstats.h \
   os/linux/tools/qcatools_lib.o \
   cmn_dev/os_if/linux/qca_vendor.h
ifeq ($(ENABLE_WKK_PROFILE),1)
ifeq ($(BUILD_VARIANT),wkk-profile)
QCAWLAN_HEADERS += \
	include/ieee80211_external_config.h
endif
else
QCAWLAN_HEADERS += \
	include/ieee80211_external_config.h
endif
QCAWLAN_HEADERS += \
   component_dev/dp/inc/dp_rate_stats_pub.h \
   cmn_dev/umac/cfr/dispatcher/inc/wlan_cfr_public_structs.h \
   offload/os/linux/include/athtypes_linux.h \
   cmn_dev/dp/inc/cdp_txrx_hist_struct.h \
   component_dev/dp/inc/cdp_txrx_extd_struct.h \
   cmn_dev/fw_hdr/fw/htt_common.h \
   cmn_dev/fw_hdr/fw/htt.h \
   cmn_dev/fw_hdr/fw/htt_stats.h \
   cmn_dev/fw_hdr/fw/htt_deps.h \
   cmn_dev/fw_hdr/fw/wmix.h \
   cmn_dev/fw_hdr/fw/wmi_unified.h \
   cmn_dev/fw_hdr/fw/wmi_services.h \
   cmn_dev/fw_hdr/fw/wmi_unified_vendor.h \
   cmn_dev/fw_hdr/fw/wmi_tlv_helper.h \
   cmn_dev/fw_hdr/fw/wmi_tlv_defs.h \
   cmn_dev/fw_hdr/fw/wlan_defs.h \
   cmn_dev/fw_hdr/fw/wmi.h \
   offload/os/linux/include/osapi_linux.h \
   cmn_dev/fw_hdr/fw/a_osapi.h \
   component_dev/tools/linux/telemetry/stats_lib.h \
   component_dev/telemetry/inc/wlan_stats_define.h \
   cmn_dev/umac/cmn_services/regulatory/inc/wlan_reg_afc.h \
   cmn_dev/umac/wifi_radar/dispatcher/inc/wlan_wifi_radar_public_structs.h

QCAWLAN_SON_HEADERS:= \
   umac/son/dispatcher/inc/wlan_son_ioctl.h \
   umac/son/dispatcher/inc/wlan_son_ald_external.h \
   umac/son/dispatcher/inc/wlan_son_band_steering_api.h

#########################################################
############ WLAN DRIVER BUILD CONFIGURATION ############
#########################################################
# Module list
# This list is filled dynamically based on the WLAN configuration
# It depends on the content of the wlan config file (.profile)
QCAWLAN_MODULE_LIST+=$(strip $(call if_opt_set, WIFI_MEM_MANAGER_SUPPORT, \
	$(PKG_BUILD_DIR)/os/linux/mem/mem_manager.$(LINUX_KMOD_SUFFIX)))

ifneq ($(QCA_SINGLE_WIFI_3_0),1)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/cmn_dev/qdf/qdf.$(LINUX_KMOD_SUFFIX)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/umac/umac.$(LINUX_KMOD_SUFFIX)
ifneq ($(CONFIG_BUILD_YOCTO),y)
ifneq ($(strip ${CONFIG_LINUX_6_1}),y)
QCAWLAN_MODULE_LIST+=$(strip $(call if_opt_set, WLAN_CONFIG_TELEMETRY_AGENT, \
	$(PKG_BUILD_DIR)/telemetry_agent/telemetry_agent.$(LINUX_KMOD_SUFFIX)))
endif

ifeq ($(strip ${CONFIG_KERNEL_IPQ_MEM_PROFILE}),256)
ifeq ($(strip ${ENABLE_QCA5018_HEADER}${ENABLE_QCA8074_HEADER}${ENABLE_QCA9574_HEADER}${ENABLE_6018_HEADER}),1)
QCAWLAN_MODULE_LIST := $(filter-out $(PKG_BUILD_DIR)/telemetry_agent/telemetry_agent.$(LINUX_KMOD_SUFFIX), $(QCAWLAN_MODULE_LIST))
$(info Telemetry Agent Set - Skipped as target unsupported)
endif #256M-MLO supported Builds
endif #256M Build
endif
QCAWLAN_MODULE_LIST+=$(strip $(call if_opt_set, WLAN_SPECTRAL_ENABLE, \
	$(PKG_BUILD_DIR)/cmn_dev/spectral/qca_spectral.$(LINUX_KMOD_SUFFIX)))
ifeq ($(CONFIG_WLAN_IOT_SIM_SUPPORT),y)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/cmn_dev/iot_sim/qca_iot_sim.$(LINUX_KMOD_SUFFIX)
endif
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/qca_ol/qca_ol.$(LINUX_KMOD_SUFFIX)
QCAWLAN_MODULE_LIST+=$(strip $(call if_opt_set, UNIFIED_SMARTANTENNA, \
	$(PKG_BUILD_DIR)/smartantenna/smart_antenna.$(LINUX_KMOD_SUFFIX)))
QCAWLAN_MODULE_LIST+=$(strip $(call if_opt_set, QCA_SUPPORT_RAWMODE_PKT_SIMULATION, \
	$(PKG_BUILD_DIR)/rawsim/rawmode_sim.$(LINUX_KMOD_SUFFIX)))
ifeq ($(CONFIG_WIFI_TARGET_WIFI_3_0),y)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/qca_ol/wifi3.0/wifi_3_0.$(LINUX_KMOD_SUFFIX)
endif
ifeq ($(CONFIG_WIFI_MONITOR_SUPPORT),y)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/component_dev/dp/wifi3.0/monitor/monitor.$(LINUX_KMOD_SUFFIX)
endif
ifeq ($(CONFIG_WIFI_TARGET_WIFI_2_0),y)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/qca_ol/wifi2.0/wifi_2_0.$(LINUX_KMOD_SUFFIX)
endif
ifneq ($(CONFIG_LOWMEM_FLASH),y)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/lmac/ath_pktlog/ath_pktlog.$(LINUX_KMOD_SUFFIX)
else
ifeq ($(CONFIG_DEBUG),y)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/lmac/ath_pktlog/ath_pktlog.$(LINUX_KMOD_SUFFIX)
endif
endif
QCAWLAN_MODULE_LIST+=$(strip $(call if_opt_set, ATH_SW_WOW_SUPPORT, \
	$(PKG_BUILD_DIR)/wow/sw_wow.$(LINUX_KMOD_SUFFIX)))
else
# Single module
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/wifi_3_0_sim0.$(LINUX_KMOD_SUFFIX)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/wifi_3_0_sim1.$(LINUX_KMOD_SUFFIX)
QCAWLAN_MODULE_LIST+=$(PKG_BUILD_DIR)/wifi_3_0_sim2.$(LINUX_KMOD_SUFFIX)
endif
ifeq ($(CONFIG_BUILD_YOCTO),y)
QCAWLAN_MODULE_NAMES:= $(notdir $(basename $(strip $(QCAWLAN_MODULE_LIST))))
endif
#########################################################
################# BUILD/INSTALL RULES ###################
#########################################################
ifneq ($(CONFIG_LOWMEM_FLASH),y)
QCAWLAN_TOOL_LIST:= 80211stats athstats athstatsclr apstats pktlogconf pktlogdump wifitool wlanconfig thermaltool wps_enhc exttool athkey qca_gensock
QCAWLAN_TOOL_LIST+= $(call if_opt_set, ATH_SUPPORT_DFS, radartool)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, WLAN_SPECTRAL_ENABLE, spectraltool)
ifneq ($(CONFIG_BUILD_YOCTO),y)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, QCA_SUPPORT_SSID_STEERING, ssidsteering)
QCAWLAN_TOOL_LIST+= assocdenialnotify
ifneq ($(strip ${CONFIG_LINUX_6_1}),y)
ifneq ($(strip ${CONFIG_LINUX_6_6}),y)
ifneq ($(strip ${CONFIG_KERNEL_IPQ_MEM_PROFILE}),256)
ifeq ($(strip ${ENABLE_QCA9574_HEADER}${ENABLE_QCA5332_HEADER}),1)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, QCA_WLAN_SUPPORT_SCS, scs_tool)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, QCA_WLAN_SUPPORT_SCS_API, scs_api)
endif
endif
endif
endif
endif
QCAWLAN_TOOL_LIST+= $(call if_opt_set, ATH_SUPPORT_TX99, tx99tool)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, DEBUG_TOOLS, dumpregs reg)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, WIFISTATS_TOOL_SUPPORT, wifistats)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, ATH_ACS_DEBUG_SUPPORT, acsdbgtool)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, QCA_CFR_SUPPORT, ../../../component_dev/tools/linux/cfr_test_app)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, WLAN_WIFI_RADAR_ENABLE, ../../../component_dev/tools/linux/wifi_radar_test_app)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, QCA_SUPPORT_RDK_STATS, ../../../component_dev/tools/linux/peerratestats)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, QLD, qldtool)
ifneq ($(strip ${CONFIG_KERNEL_IPQ_MEM_PROFILE}),512)
ifneq ($(strip ${CONFIG_KERNEL_IPQ_MEM_PROFILE}),256)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, QCA_SUPPORT_LITE_MONITOR, lite_mon)
endif
endif
ifneq ($(strip ${CONFIG_KERNEL_IPQ_MEM_PROFILE}),256)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, WLAN_SUPPORT_TELEMETRY, wifitelemetry)
endif
else
ifeq ($(CONFIG_DEBUG),y)
QCAWLAN_TOOL_LIST:= athstats athstatsclr apstats pktlogconf pktlogdump wifitool wlanconfig wps_enhc exttool qca_gensock thermaltool
QCAWLAN_TOOL_LIST+= $(call if_opt_set, ATH_SUPPORT_DFS, radartool)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, QCA_SUPPORT_LITE_MONITOR, lite_mon)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, QCA_SUPPORT_SSID_STEERING, ssidsteering)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, WIFISTATS_TOOL_SUPPORT, wifistats)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, QCA_CFR_SUPPORT, ../../../component_dev/tools/linux/cfr_test_app)
QCAWLAN_TOOL_LIST+= $(call if_opt_set, WLAN_WIFI_RADAR_ENABLE, ../../../component_dev/tools/linux/wifi_radar_test_app)
else
QCAWLAN_TOOL_LIST:= wlanconfig wps_enhc qca_gensock thermaltool
QCAWLAN_TOOL_LIST+= $(call if_opt_set, QCA_SUPPORT_SSID_STEERING, ssidsteering)
endif
endif
