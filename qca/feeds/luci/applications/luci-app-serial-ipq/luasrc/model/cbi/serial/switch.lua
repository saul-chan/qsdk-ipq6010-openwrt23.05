local sys   = require "luci.sys"
local m, s,o

m = Map("switch", translate("Switch Configuration"))
m:chain("luci")

s = m:section(TypedSection, "signal", translate("Configuration"))
s.anonymous = true
s.addremove = false

serialproto = s:option(ListValue, "serialproto", translate("Transport Protocol"))
serialproto:value("0", translate("Modbus RTU"))

localport = s:option(Value, "localport", translate ("Local Port"))
localport.datatype="port"

-- iosignal = s:option(ListValue, "iosignal", translate ("Power Input"))
-- iosignal:value("0",translate("Passive Input"))
-- iosignal:value("1",translate("Active Input"))

local apply = luci.http.formvalue("cbi.apply")
if apply then
    io.popen("/etc/init.d/switch start &")
end

return m