PLUGINS:= manager mcfwdtbl-eswitch
ifeq ($(MCS_MODULE_WLAN),y)
PLUGINS+=  mcfwdtbl-wlan
endif

all: $(PLUGINS)

manager:
	@$(MAKE) -C manager all

mcfwdtbl-wlan:
	@$(MAKE) -C mcfwdtbl-wlan all

mcfwdtbl-eswitch:
	@$(MAKE) -C mcfwdtbl-eswitch all

clean:
	@$(MAKE) -C manager clean
ifeq ($(MCS_MODULE_WLAN),y)
	@$(MAKE) -C mcfwdtbl-wlan clean
endif
	@$(MAKE) -C mcfwdtbl-eswitch clean

.PHONY: manager mcfwdtbl-wlan mcfwdtbl-eswitch
