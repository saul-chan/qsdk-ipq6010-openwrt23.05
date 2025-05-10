
include $(TOPDIR)/rules.mk

PKG_NAME:=qti-softsku-license-loader-libs
PKG_VERSION:=1.0
PKG_RELEASE:=1

LOCAL_SRC:=$(TOPDIR)/qca/src/qca-platform-utils/qti-softsku-license-loader
include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
 PKG_REV:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
 PKG_VERSION:=g$(PKG_REV)
endif

include $(INCLUDE_DIR)/package.mk

define Package/qti-softsku-license-loader-libs
  SECTION:=QCA
  CATEGORY:=QTI software
  TITLE:=qti-softsku-license-loader Library for Qwin agent
  DEPENDS:=@TARGET_ipq95xx||TARGET_ipq53xx
endef

define Package/qti-softsku-license-loader-libs/description
  qti-softsku-license-loader Library for Qwin agent
endef

TARGET_CFLAGS += -fPIC

TARGET_LDFLAGS += \
	-L$(STAGING_DIR)/usr/lib

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/qti-softsku-license-loader-libs
	$(INSTALL_DIR) $(1)/usr/lib/qti-softsku-license-loader-libs/
	$(CP) $(PKG_BUILD_DIR)/softsku-loader.h $(1)/usr/include/qti-softsku-license-loader-libs/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/libsoftsku-loader.so $(1)/usr/lib/qti-softsku-license-loader-libs/
endef

define Package/qti-softsku-license-loader-libs/install
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libsoftsku-loader.so $(1)/lib/
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/softsku_loader-app $(1)/usr/sbin
endef

$(eval $(call BuildPackage,qti-softsku-license-loader-libs))
