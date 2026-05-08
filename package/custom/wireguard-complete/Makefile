include $(TOPDIR)/rules.mk

PKG_NAME:=wireguard-complete
PKG_VERSION:=1.0
PKG_RELEASE:=1

# 定义两个IPK文件
KMOD_IPK_FILE := $(wildcard $(CURDIR)/ipk/kmod-wireguard_*.ipk)
TOOLS_IPK_FILE := $(wildcard $(CURDIR)/ipk/wireguard-tools_*.ipk)

ifeq ($(KMOD_IPK_FILE),)
	$(error No kmod-wireguard IPK file found!)
endif
ifeq ($(TOOLS_IPK_FILE),)
	$(error No wireguard-tools IPK file found!)
endif

KMOD_IPK_BASENAME := $(notdir $(KMOD_IPK_FILE))
TOOLS_IPK_BASENAME := $(notdir $(TOOLS_IPK_FILE))

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	CATEGORY:=Network
	TITLE:=Complete WireGuard package (kmod + tools)
	DEPENDS:=@LINUX_5_4 +kmod-udptunnel4 +kmod-udptunnel6
endef

define Package/$(PKG_NAME)/description
	Complete WireGuard installation including kernel module and user-space tools.
	This package contains pre-compiled binaries for aarch64_cortex-a53_neon-vfpv4.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(KMOD_IPK_FILE) $(PKG_BUILD_DIR)/
	$(CP) $(TOOLS_IPK_FILE) $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	# 安装kmod-wireguard
	tar -xzOf $(PKG_BUILD_DIR)/$(KMOD_IPK_BASENAME) ./data.tar.gz | tar -C $(1) -xzf -
	# 安装wireguard-tools
	tar -xzOf $(PKG_BUILD_DIR)/$(TOOLS_IPK_BASENAME) ./data.tar.gz | tar -C $(1) -xzf -
endef

$(eval $(call BuildPackage,$(PKG_NAME)))