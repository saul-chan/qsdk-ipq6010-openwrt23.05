include $(TOPDIR)/rules.mk

PKG_NAME:=slkscripts
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	CATEGORY:=Custom Packages
	TITLE:=slkscripts
	DEPENDS:=
endef

define Package/$(PKG_NAME)/description
	Pre-compiled slk scripts packaged as shell.
endef

define Build/Compile
endef

define Package/slkscripts/install
	$(INSTALL_DIR) $(1)/etc/slkapp
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/slkapp/* $(1)/etc/slkapp/
	$(INSTALL_BIN) ./files/slkapp_init $(1)/etc/init.d/slkapp_init
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
