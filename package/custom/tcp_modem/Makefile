include $(TOPDIR)/rules.mk

PKG_NAME:=tcp_modem
PKG_VERSION:=1.0
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/tcp_modem
  CATEGORY:=Custom Packages
  TITLE:=tcp_modem sdk
  DEPENDS:=+libpthread 
endef

define Package/tcp_modem/description
	If you can't figure out what this program does, you're probably  
	brain-dead and need immediate medical attention.
endef
define Package/tcp_modem/extra_provides
	echo 'libstdc++.so.6'; \
	echo 'libm.so.6'; \
    echo 'libc.so.6';
endef
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/tcp_modem/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tcp_modem $(1)/usr/sbin/
endef


$(eval $(call BuildPackage,tcp_modem))
