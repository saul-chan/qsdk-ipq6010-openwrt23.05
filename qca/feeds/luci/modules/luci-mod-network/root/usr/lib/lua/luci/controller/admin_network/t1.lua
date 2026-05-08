--[[
LuCI - Lua Configuration Interface

Copyright (c) 2013 Qualcomm Atheros, Inc.

All Rights Reserved.
Qualcomm Atheros Confidential and Proprietary.

]]--

module("luci.controller.admin_network.t1", package.seeall)

function index()
	if not nixio.fs.access("/etc/config/t1") then
		return
	end

	local page

	page = entry({"admin", "network", "t1"}, cbi("admin_network/t1"), _("T1 Network"), 13)
	page.dependent = true

end
