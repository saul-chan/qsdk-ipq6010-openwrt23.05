--[[
LuCI - Lua Configuration Interface

Copyright (c) 2013 Qualcomm Atheros, Inc.

All Rights Reserved.
Qualcomm Atheros Confidential and Proprietary.

]]--

module("luci.controller.admin_network.wan", package.seeall)

function index()
	-- if not nixio.fs.access("/etc/config/t1") then
		-- return
	-- end

	local page

	-- page = entry({"admin", "network", "wan"}, cbi("admin_network/wan"), _("WAN Settings"), 10)
	-- page.dependent = true

end
