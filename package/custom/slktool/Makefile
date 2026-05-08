include $(TOPDIR)/rules.mk

PKG_NAME:=slktool
PKG_VERSION:=1.1
PKG_RELEASE:=1

PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/$(PKG_NAME)
	CATEGORY:=Custom Packages
	TITLE:=slktool tool
	DEPENDS:=+libstdcpp
endef

define Package/$(PKG_NAME)/description
	slktool SDK.
endef

# define Build/Prepare
	# mkdir -p $(PKG_BUILD_DIR)
	# $(CP) $(PKG_IPK_FILE) $(PKG_BUILD_DIR)/
# endef

# define Build/Configure
# endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin/
	$(CP) ./src/slktool $(1)/usr/sbin/
	$(INSTALL_BIN) ./src/write_bin.sh $(1)/usr/sbin/write_bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
