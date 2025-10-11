include $(TOPDIR)/rules.mk

PKG_NAME:=slkmac
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_IPK_FILE := $(wildcard $(CURDIR)/ipk/$(PKG_NAME)_*.ipk)

ifneq ($(PKG_IPK_FILE),)
  PKG_IPK_BASENAME := $(notdir $(PKG_IPK_FILE))
else
  $(error No IPK file found in ipk/ directory!)
endif

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	CATEGORY:=Custom Packages
	TITLE:=slkmac tool
	DEPENDS:=
endef

define Package/$(PKG_NAME)/description
	Pre-compiled slkmac tool packaged as IPK.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(PKG_IPK_FILE) $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	tar -xzOf $(PKG_BUILD_DIR)/$(PKG_IPK_BASENAME) ./data.tar.gz | tar -C $(1) -xzf -
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
