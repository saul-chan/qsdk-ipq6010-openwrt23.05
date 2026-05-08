include $(TOPDIR)/rules.mk

PKG_NAME:=fibo_update
PKG_RELEASE:=1.4.0
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)


include $(INCLUDE_DIR)/package.mk

define Package/fibo_update
	CATEGORY:=Custom Packages
	SUBMENU:=fibocom
	TITLE:=fibo_update -- prints a snarky message
endef

define Package/fibo_update/description
	It's my first package demo.
endef

define Build/Prepare
	echo "Here is Package/Prepare"
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/fibo_update/install
	echo "Here is Package/install"
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/upgrade_tool $(1)/bin/
endef

$(eval $(call BuildPackage,fibo_update))
