include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_SOURCE_NAME:=bluetopia
PKG_NAME:=bluetopia-mini
# Bluetopia for QCC710 - IPQ95xx
ifneq (, $(findstring ipq95xx, $(CONFIG_TARGET_BOARD)))
PKG_VERSION:=4.2.1.c1_26
PKG_DIST_VERSION:=4.2.1.4
PKG_MD5SUM:=98c0c72192b2964e2e8687c2fe8ae96a
else
# Common to all platforms except QCC710
PKG_VERSION:=4.2.1.c1_26
PKG_DIST_VERSION:=4.2.1.c1
PKG_MD5SUM:=ad1a200e30c51254bf1f63b948134037
endif

PKG_RELEASE:=1
PKG_FOLDER=BLUETOPIA

PKG_SOURCE:=$(PKG_SOURCE_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://qcawebsrvr.qualcomm.com/cnss_win/santaclara/dev01/$(PKG_FOLDER)/
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_DIST_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_DIST_VERSION)

# Patch directory differs based on the package version
PATCH_DIR:=./patches-$(PKG_VERSION)

# Bluetopia Root Paths
export BLUETOPIA_SOURCE_PATH:=$(PKG_BUILD_DIR)/bt_host/Build
export BLUETOPIAPM_SOURCE_PATH:=$(PKG_BUILD_DIR)/bt_host_pm/Build
export BLUETOPIA_ROOT_PATH:=$(PKG_DIST_DIR)/Bluetopia_Dist
export BTPM_DIST_PATH:=$(PKG_DIST_DIR)/BluetopiaPM_Dist

# Bluetopia Core Include and Library Path
BLUETOPIA_INCLUDE_PATH:=$(BLUETOPIA_ROOT_PATH)/include
export BLUETOPIA_LIB_PATH:=$(BLUETOPIA_ROOT_PATH)/lib

# Bluetopia Debug Include and Library Path
BLUETOPIA_DEBUG_INCLUDE_PATH:=$(BLUETOPIA_ROOT_PATH)/debug/include
BLUETOPIA_DEBUG_LIB_PATH:=$(BLUETOPIA_ROOT_PATH)/debug/lib

# Bluetopia Profiles Root Path
export BLUETOPIA_PROFILE_ROOT_PATH:=$(BLUETOPIA_ROOT_PATH)/profiles

# Bluetopia VSER Include and Library Path
BLUETOPIA_VSER_INCLUDE_PATH:=$(BLUETOPIA_ROOT_PATH)/VSER/include
BLUETOPIA_VSER_LIB_PATH:=$(BLUETOPIA_ROOT_PATH)/VSER/lib

# Bluetopia VNET Include and Library Path
BLUETOPIA_VNET_INCLUDE_PATH:=$(BLUETOPIA_ROOT_PATH)/VNET/include
BLUETOPIA_VNET_LIB_PATH:=$(BLUETOPIA_ROOT_PATH)/VNET/lib

# Bluetopia SBC Include and Library Path
BLUETOPIA_SBC_INCLUDE_PATH:=$(BLUETOPIA_ROOT_PATH)/SBC/include
BLUETOPIA_SBC_LIB_PATH:=$(BLUETOPIA_ROOT_PATH)/SBC/lib

# Bluetopia Simple XML Parser Path
export BLUETOPIA_XML_PATH:=$(BLUETOPIA_ROOT_PATH)/XML/SS1SXMLP

# Bluetopia Distribution Path
export BLUETOPIA_PATH:=$(BLUETOPIA_ROOT_PATH)


include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=QTI
  CATEGORY:=QTI software
  TITLE:=$(1) - QTI mini-Bluetopia
  DEPENDS:=@TARGET_ipq807x||TARGET_ipq50xx||TARGET_ipq95xx||TARGET_ipq53xx||TARGET_ipq60xx||TARGET_ipq +libpthread +libopenssl
endef

define Package/$(PKG_NAME)/description
  This package contains mini-Bluetopia PM and mini-Bluetopia binaries
endef

SOC=DK07
ifeq ($(CONFIG_TARGET_ipq),y)
    soc_name:=$(SUBTARGET)
else
    soc_name:=$(CONFIG_TARGET_BOARD)
endif
ifneq (, $(findstring $(soc_name), "ipq50xx" "ipq50xx_64"))
    SOC=Maple
    TARGET_CFLAGS+=-DPOWER_RESET
# For QCC710 - IPQ95xx platform
else ifneq (, $(findstring ipq95xx, $(CONFIG_TARGET_BOARD)))
    Controller=Ziggy
    SOC=""
endif

#TARGET_CFLAGS += -Werror -Wall
TARGET_CFLAGS += -Wall

MAKE_FLAGS="CC=$$(TARGET_CC)" \
	   "GLOBLDFLAGS=$$(TARGET_LDFLAGS)" \
	   "GLOBCFLAGS=$$(TARGET_CFLAGS)" \
	   "GLOBLDLIBS= -lssl -lcrypto"

MAKE_KWFLAG_BTPSKRNL="CC=$$(TARGET_CC)" \
           "GLOBLDFLAGS=$$(TARGET_LDFLAGS)" \
           "GLOBCFLAGS=$$(TARGET_CFLAGS) -DBTPS_KERNEL_USE_C_RUN_TIME_IMPLEMENTATION=0"

INCLDIRS = -I$(BLUETOPIA_INCLUDE_PATH)        \
	   -I$(BLUETOPIA_DEBUG_INCLUDE_PATH)  \
	   -I$(BLUETOPIA_VSER_INCLUDE_PATH)   \
	   -I$(BLUETOPIA_VNET_INCLUDE_PATH)   \
	   -I$(BLUETOPIA_SBC_INCLUDE_PATH)

LIBDIRS = -L$(BLUETOPIA_LIB_PATH)          \
	  -L$(BLUETOPIA_DEBUG_LIB_PATH)    \
	  -L$(BLUETOPIA_VSER_LIB_PATH)     \
	  -L$(BLUETOPIA_VNET_LIB_PATH)     \
	  -L$(BLUETOPIA_SBC_LIB_PATH)

PM_MAKE_FLAGS="CC=$$(TARGET_CC)" \
	   "GLOBLDFLAGS=$$(TARGET_LDFLAGS)" \
	   "GLOBCFLAGS=$$(TARGET_CFLAGS)" \
	   "BLUETOPIA_INCLUDE_PATH=$$(BLUETOPIA_ROOT_PATH)/include" \
	   "GLOBLDLIBS= -lssl -lcrypto"

# BT Host files common to all platforms
COMMON_BT_HOST_FILES:= $(PKG_BUILD_DIR)/bt_host/Build/Linux/btpscert;-f;BTPSCERT.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/btpsfile;-f;BTPSFILE.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/btpsvend;-f;BTPSVEND.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/btpssec;-f;BTPSSEC.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/debug/SS1BTDBG;-f;SS1BTDBG.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/hcidrv_lin;-f;HCIDRV.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/ss1btps;-f;SS1BTPS.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/VSER/SS1SER;-f;SS1SER.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/VSER/SS1VSER;-f;SS1VSER.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/VNET/SS1VNET;-f;SS1VNET.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/XML/SS1SXMLP;-f;SS1SXMLP.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/profiles/GATT;-f;SS1BTGAT.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/profiles_gatt/GAPS;-f;SS1BTGAP.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/profiles_gatt/DIS;-f;SS1BTDIS.mak

# BT Host files applicable to Maple and DK07 only
MAPLE_DK_BT_HOST_FILES:= $(PKG_BUILD_DIR)/bt_host/Build/Linux/ss1btle;-f;SS1BTPS.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/profiles/GATT;-f;SS1BTGAT_LE.mak

# BT Host files applicable to DK07 only
DK_BT_HOST_FILES:= $(PKG_BUILD_DIR)/bt_host/Build/Linux/profiles_gatt/sample/LinuxSPPLE;-f;LinuxSPPLE.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/sample/LinuxSPP;-f;LinuxSPP.mak

# BT Host PM files applicable to Maple and DK07 only
MAPLE_DK_BT_HOST_PM_FILES:= $(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmcert;-f;BTPMCERT.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmdbg;-f;BTPMDBG.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmerr;-f;BTPMERR.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmset;-f;BTPMSET.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmipc/client;-f;BTPMIPC.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmipc/server;-f;BTPMIPC.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmmodc/client;-f;BTPMMODC.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmmodc/server;-f;BTPMMODC.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmscom/client;-f;BTPMSCOM.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmscom/server;-f;BTPMSCOM.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmsppm/client;-f;BTPMSPPM.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmsppm/server;-f;BTPMSPPM.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmgatm/client;-f;BTPMGATM.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmgatm/server;-f;BTPMGATM.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmdevm/client;-f;BTPMDEVM.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/btpmdevm/server;-f;BTPMDEVM.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/ss1btpml/client;-f;SS1BTPML.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/ss1btpml/server;-f;SS1BTPML.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/ss1btpm/client;-f;SS1BTPM.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/ss1btpm/server;-f;SS1BTPM.mak

# BT Host PM files applicable to Maple only
MAPLE_BT_HOST_PM_FILES:= $(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/sample/LinuxSPPLE_PM;-f;LinuxSPPLE_PM.mak \
	$(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/sample/LinuxSPPM;-f;LinuxSPPM.mak

# BT files applicable to QCC710 only
QCC710_BT_FILES:= $(PKG_BUILD_DIR)/bt_host/Build/Ports/Controllers/Ziggy/fwupdate;-f;zgyupdate.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/profiles_gatt/sample/LinuxSPPLE;-f;LinuxSPPLE.mak \
	$(PKG_BUILD_DIR)/bt_host/Build/Linux/profiles_gatt/OTS;-f;SS1BTOTS.mak

# For Controllers other than QCC710,
# copy the platform specific files into
# $(PKG_BUILD_DIR)/bt_host/Build/Linux/
define Build/Prepare
	$(TAR) xjvf $(TOPDIR)/dl/$(PKG_SOURCE) -C $(PKG_BUILD_DIR)
	$(Build/Patch)
ifneq ($(Controller), Ziggy)
	$(CP) $(PKG_BUILD_DIR)/bt_host/Build/Ports/Platform/$(SOC)/* $(PKG_BUILD_DIR)/bt_host/Build/Linux/
endif
ifeq ($(SOC), DK07)
	$(CP) $(PKG_BUILD_DIR)/bt_host/Build/Ports/Common/btpssec/openssl/* $(PKG_BUILD_DIR)/bt_host/Build/Linux/btpssec/
endif
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/bt_host/Build/Linux/btpskrnl  -f BTPSKRNL.mak all $(MAKE_KWFLAG_BTPSKRNL)
	$(foreach common_bt_host_file, $(COMMON_BT_HOST_FILES), ($(MAKE) -C $(subst ;, ,$(common_bt_host_file)) all $(MAKE_FLAGS));)

ifeq ($(Controller), Ziggy)
	$(foreach qcc710_bt_file, $(QCC710_BT_FILES), ($(MAKE) -C $(subst ;, ,$(qcc710_bt_file)) $(MAKE_FLAGS));)
	echo 'creating mini-Bluetopia Dist area'
	/bin/sh $(PKG_BUILD_DIR)/bt_host/Build/MakeFull_win.sh /d $(BLUETOPIA_ROOT_PATH) /s $(BLUETOPIA_SOURCE_PATH)
	echo 'done creating mini-Bluetopia Dist area'
else
	$(foreach mp_dk_bt_host_file, $(MAPLE_DK_BT_HOST_FILES), ($(MAKE) -C $(subst ;, ,$(mp_dk_bt_host_file)) all $(MAKE_FLAGS));)
endif

ifeq ($(SOC), DK07)
	$(foreach dk_bt_host_file, $(DK_BT_HOST_FILES), ($(MAKE) -C $(subst ;, ,$(dk_bt_host_file)) all $(MAKE_FLAGS));)
endif

	echo 'creating mini-Bluetopia Dist area'
	/bin/sh $(PKG_BUILD_DIR)/bt_host/Build/MakeFull_win.sh /d $(BLUETOPIA_ROOT_PATH) /s $(BLUETOPIA_SOURCE_PATH)
	echo 'done creating mini-Bluetopia Dist area'

ifneq ($(Controller), Ziggy)
	$(foreach mp_dk_bt_host_pm_file, $(MAPLE_DK_BT_HOST_PM_FILES), ($(MAKE) -C $(subst ;, ,$(mp_dk_bt_host_pm_file)) all $(PM_MAKE_FLAGS));)
endif

ifeq ($(SOC), Maple)
	$(foreach mp_bt_host_pm_file, $(MAPLE_BT_HOST_PM_FILES), ($(MAKE) -C $(subst ;, ,$(mp_bt_host_pm_file)) all $(PM_MAKE_FLAGS));)
endif

ifneq ($(Controller), Ziggy)
	echo 'creating mini-BluetopiaPM Dist area with default interval 20sec'
	/bin/sh $(PKG_BUILD_DIR)/bt_host_pm/Build/MakeFull.sh /d $(BTPM_DIST_PATH) /s $(BLUETOPIAPM_SOURCE_PATH)
	echo 'done creating mini-Bluetopia Dist area'
endif
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin

ifeq ($(SOC), DK07)
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/etc/init.d $(1)/etc/profile.d
	$(INSTALL_BIN) ./files/PS_KEY_CSR8811.txt $(1)/usr/bin
	$(INSTALL_BIN) ./files/bluetopia.init $(1)/etc/init.d/bluetopia
	$(INSTALL_BIN) ./files/bt_env.sh $(1)/etc/profile.d/bt_env.sh
	$(INSTALL_BIN) \
	    $(PKG_BUILD_DIR)/bt_host/Build/Linux/profiles_gatt/sample/LinuxSPPLE/LinuxSPPLE $(1)/usr/bin
	$(INSTALL_BIN) \
	    $(PKG_BUILD_DIR)/bt_host/Build/Linux/sample/LinuxSPP/LinuxSPP  $(1)/usr/bin
endif

ifeq ($(Controller), Ziggy)
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) ./files/bt_qcc710_start.sh $(1)/sbin/bt_qcc710_start
	$(INSTALL_BIN) \
	    $(PKG_BUILD_DIR)/bt_host/Build/Ports/Controllers/Ziggy/fwupdate/zgyupdate $(1)/usr/bin/
	$(INSTALL_BIN) \
	    $(PKG_BUILD_DIR)/bt_host/Build/Linux/profiles_gatt/sample/LinuxSPPLE/LinuxSPPLE $(1)/usr/bin
endif

ifeq ($(SOC), Maple)
	$(INSTALL_BIN) \
	    $(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/ss1btpm/server/SS1BTPM  $(1)/usr/bin
	$(INSTALL_BIN) \
	    $(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/sample/LinuxSPPLE_PM/LinuxSPPLE_PM  $(1)/usr/bin
	$(INSTALL_BIN) \
	    $(PKG_BUILD_DIR)/bt_host_pm/Build/Linux/sample/LinuxSPPM/LinuxSPPM  $(1)/usr/bin
endif
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
