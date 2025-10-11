-- Copyright 2016-2017 Dan Luedtke <mail@danrl.com>
-- Licensed to the public under the Apache License 2.0.

module("luci.controller.wifi", package.seeall)

function index()
	if (nixio.fs.access("/etc/config/wireless"))then  
		page=entry({"admin", "network" ,"wifi_ap"}, cbi("admin_network/wifi_ap"), _("WIFI Setting"), 8)
		page=entry({"admin", "network" ,"wifi_sta"}, cbi("admin_network/wifi_sta"), _("WIFI Client"), 9)
	end
end
