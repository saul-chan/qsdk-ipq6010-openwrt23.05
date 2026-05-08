include $(TOPDIR)/rules.mk

PKG_NAME:=slkgpio
PKG_VERSION:=1.0
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/slkgpio
  CATEGORY:=Custom Packages
  TITLE:=Seriallink Modbus gpio
  DEPENDS:=+libpthread 
endef

define Package/slkgpio/description
	If you can't figure out what this program does, you're probably  
	brain-dead and need immediate medical attention.
endef
define Package/slkgpio/extra_provides
	echo 'libstdc++.so.6'; \
	echo 'libm.so.6'; \
    echo 'libc.so.6';
endef
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/slkgpio/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/slkgpio $(1)/usr/sbin/
endef


$(eval $(call BuildPackage,slkgpio))
