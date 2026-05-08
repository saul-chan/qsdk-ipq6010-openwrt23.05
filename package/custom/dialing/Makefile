include $(TOPDIR)/rules.mk

PKG_NAME:=dialing
PKG_VERSION:=3.0
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)

TARGET_CFLAGS += -fdiagnostics-color=always
export GCC_COLORS='error=01;31:warning=01;35:note=01;36:caret=01;32:locus=01:quote=01'
include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/dialing
  CATEGORY:=Custom Packages
  TITLE:=dialing sdk
  DEPENDS:=+libpthread 
endef

define Package/dialing/description
	version 2.0 supports dial 4/5G within 2 modules, or 1 module via 2 different apns.
endef
define Package/dialing/extra_provides
	echo 'libstdc++.so.6'; \
	echo 'libm.so.6'; \
    echo 'libc.so.6';
endef
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/dialing/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dialing $(1)/usr/sbin/
endef


$(eval $(call BuildPackage,dialing))
