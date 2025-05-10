include $(TOPDIR)/rules.mk

PKG_NAME:=common-headers
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_DIST_VERSION=1.0

LOCAL_SRC:=$(TOPDIR)/qca/src/common_headers

include $(INCLUDE_DIR)/local-development.mk
include $(INCLUDE_DIR)/package.mk

define Package/common-headers
  TITLE:=Common Headers
  SECTION:=QCA
endef

define Package/common-headers/description
  This package contains header files that are used by
  other packages during compilation.
endef

Build/Compile=true

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/inc/* $(STAGING_DIR)/usr/include/
endef

$(eval $(call BuildPackage,common-headers))
