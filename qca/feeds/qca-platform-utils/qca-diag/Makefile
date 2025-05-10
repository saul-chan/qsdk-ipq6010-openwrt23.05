include $(TOPDIR)/rules.mk

PKG_NAME:=qca-diag
PKG_VERSION:=g2e41e7f
PKG_RELEASE:=1
PKG_DIST_VERSION=1.0
PKG_FIXUP:=autoreconf

LOCAL_SRC:=$(TOPDIR)/qca/src/diag

include $(INCLUDE_DIR)/local-development.mk
include $(INCLUDE_DIR)/package.mk

define Package/qca-diag
  TITLE:=qca-diag
  DEPENDS:=+libpthread +qca-qmi-framework
  PKG_BUILD_DEPENDS:=common-headers
endef

define Package/qca-diag/Description
  This package contains a diag framework for the user space
endef

TARGET_CFLAGS += -Werror -Wno-address-of-packed-member \
	-I$(STAGING_DIR)/usr/include/ \
	-I$(PKG_BUILD_DIR)/include \
	-I$(PKG_BUILD_DIR)/src \
	-DDIAG_DISABLE_TIME_SERVICES

TARGET_LDFLAGS += \
	-L$(STAGING_DIR)/usr/lib

define Package/qca-diag/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/.libs/libdiag.so.1 $(1)/usr/lib/
	$(LN) ./libdiag.so.1 $(1)/usr/lib/libdiag.so
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/PktRspTest/PktRspTest $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/callback_sample/diag_callback_sample $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dci_sample/diag_dci_sample $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/klog/diag_klog $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mdlog/diag_mdlog $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/socket_log/diag_socket_log $(1)/usr/sbin/
	$(LN) ./diag_socket_log $(1)/usr/sbin/diag_socket_app
	$(LN) ./diag_mdlog $(1)/usr/sbin/diag_flash_logger
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/test/test_diag $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/test/debug-diag $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uart_log/diag_uart_log $(1)/usr/sbin/
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/qca-diag $(STAGING_DIR)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/include/* $(STAGING_DIR)/usr/include/qca-diag/
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/
	$(INSTALL_DIR) $(STAGING_DIR)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/.libs/libdiag.so.1 $(STAGING_DIR)/usr/lib/
	$(LN) $(STAGING_DIR)/usr/lib/libdiag.so.1 $(STAGING_DIR)/usr/lib/libdiag.so
endef

$(eval $(call BuildPackage,qca-diag))
