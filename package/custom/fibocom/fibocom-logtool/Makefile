include $(TOPDIR)/rules.mk

PKG_NAME:=fibocom-logtool
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)


include $(INCLUDE_DIR)/package.mk

define Package/fibocom-logtool
	CATEGORY:=Custom Packages
	SUBMENU:=fibocom
	TITLE:=diaggrab_fibolog sdk
	DEPENDS:=+libpthread +libstdcpp +zlib
endef

define Package/fibocom-logtool/description
	It's my first package demo.
endef

define Build/Prepare
	echo "Here is Package/Prepare"
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/fibocom-logtool/install
	echo "Here is Package/install"
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/logtool $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,fibocom-logtool))
