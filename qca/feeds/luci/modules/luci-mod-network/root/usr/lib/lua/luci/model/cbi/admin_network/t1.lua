--[[
LuCI - Lua Configuration Interface

Copyright (c) 2013 Qualcomm Atheros, Inc.

All Rights Reserved.
Qualcomm Atheros Confidential and Proprietary.

]]--

local uci = require "luci.model.uci"


m = Map("t1", translate("T1 Settings"),
	translate("Configuration of T1 workmode "))

s = m:section(TypedSection, "t1", translate("Workmode Settings"))
s.anonymous = true

o = s:option(ListValue, "speed", translate("Speed"))
o:value("1000", translate("1000M"))
o:value("100", translate("100M"))
o.default = "1000"

o = s:option(ListValue, "mode", translate("Mode"))
o:value("master", translate("Master"))
o:value("slave", translate("Slave"))
o.default = "slave"

return m