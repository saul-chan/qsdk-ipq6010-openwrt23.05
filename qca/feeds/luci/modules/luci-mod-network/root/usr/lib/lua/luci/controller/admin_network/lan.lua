--[[
LuCI - Lua Configuration Interface

Copyright (c) 2013 Qualcomm Atheros, Inc.

All Rights Reserved.
Qualcomm Atheros Confidential and Proprietary.

]]--

module("luci.controller.admin_network.lan", package.seeall)

function index()
	-- if not nixio.fs.access("/etc/config/t1") then
		-- return
	-- end

	local page

	-- page = entry({"admin", "network", "lan"}, cbi("admin_network/lan"), _("LAN Settings"), 11)
	-- page.dependent = true
	
	page = entry({"admin", "network", "lan_dhcp"}, cbi("admin_network/lan_dhcp"), _("DHCP Settings"), 12)
	page.dependent = true

end
