include $(TOPDIR)/rules.mk

PKG_NAME:=qti-license-pfm
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/qti-license-pfm
  TITLE:=QTI Linux PFM license Init and Upgrade script
  SECTION:=QCA
  CATEGORY:=QTI software
  DEPENDS:=@TARGET_ipq95xx||TARGET_ipq53xx
endef

define Package/qti-license-pfm/description
  This package contains a license pfm init and upgrade which can be used
  internally by QTI to update the pfm license files
endef

Build/Compile=true

define Package/qti-license-pfm/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) ./files/license-pfm-upgrade.sh $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/license-pfm.init $(1)/etc/init.d/license-pfm
	$(INSTALL_DIR) $(1)/lib/upgrade/keep.d
	$(INSTALL_DATA) ./files/license-pfm.upgrade $(1)/lib/upgrade/keep.d/license-pfm
endef

$(eval $(call BuildPackage,qti-license-pfm))
