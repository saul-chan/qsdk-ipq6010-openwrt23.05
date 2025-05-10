include $(TOPDIR)/rules.mk

PKG_NAME:=tenureprobe
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/tenureprobe
  TITLE:=QTI Linux tenureprobe script
  DEPENDS:=@TARGET_ipq53xx
  SECTION:=QTI
  CATEGORY:=QTI software
endef

define Package/tenureprobe/description
  This package contains a tenureprobe script which can be used
  internally by QTI to collect the memnoc latency.
endef

Build/Compile=true

define Package/tenureprobe/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) ./files/tenureprobe.sh $(1)/usr/sbin
endef

$(eval $(call BuildPackage,tenureprobe))
