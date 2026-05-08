include $(TOPDIR)/rules.mk

PKG_NAME:=stunClient
PKG_VERSION:=1.0
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/stunClient
  CATEGORY:=Examples
  TITLE:=slk stunClient
  DEPENDS:=
endef

define Package/stunClient/description
	If you can't figure out what this program does, you're probably  
	brain-dead and need immediate medical attention.
endef

define Package/stunClient/extra_provides
	echo 'libstdc++.so.6'; \
	echo 'libm.so.6'; \
    echo 'libc.so.6'; \
	echo 'libpthread.so.0';
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/stunClient/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/stunClient $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/stun-client.init $(1)/etc/init.d/stunClient
	$(INSTALL_DIR) $(1)/etc/hotplug.d/cwmp/
	$(INSTALL_BIN) ./files/01-stun_client.hotplug $(1)/etc/hotplug.d/cwmp/01-stun_client
endef


$(eval $(call BuildPackage,stunClient))
