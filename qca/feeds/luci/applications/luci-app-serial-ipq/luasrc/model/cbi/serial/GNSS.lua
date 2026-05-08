local sys   = require "luci.sys"
local m, s, m2, s2, o

m = Map("nport", translate("Configuration"))
m:chain("luci")

s = m:section(TypedSection, "ttyM1", translate("Network Settings"))
s.anonymous = true
s.addremove = false


enable = s:option(Flag, "enable", translate("Enable"))
enable.rmempty  = false

netpro = s:option(ListValue, "netpro", translate("Network Proto"))
netpro.default = "tcpserver"
netpro:value("tcpserver", "TCP Server")
netpro:value("tcpclient", "TCP Client")
netpro:value("udpclient", "UDP Client")

localport = s:option(Value, "localport", translate ("Local Port"))
localport.default = "3030"
localport.datatype="port"
localport:depends("netpro", "tcpserver")

maxconn = s:option(ListValue, "maxconn", translate ("Maximum number"))
maxconn.default = "6"
maxconn:value("1", translate("1"))
maxconn:value("2", translate("2"))
maxconn:value("3", translate("3"))
maxconn:value("4", translate("4"))
maxconn:value("5", translate("5"))
maxconn:value("6", translate("6"))
maxconn:depends("netpro", "tcpserver")

timeout = s:option(Value, "timeout", translate ("Time Out(s)"))
timeout.default = "300"
timeout:depends("netpro", "tcpserver")

serverip = s:option(Value, "serverip", translate("Server IP Address"))
serverip.datatype = "ip4addr"
serverip.default = "192.168.2.101"
serverip:depends("netpro", "tcpclient")
serverip:depends("netpro", "udpclient")

serverport = s:option(Value, "serverport", translate ("Server Port"))
serverport.datatype="port"
serverport.default = "9577"
serverport:depends("netpro", "tcpclient")
serverport:depends("netpro", "udpclient")

DeviceID = s:option(Value, "DeviceID", translate ("Device ID"))
DeviceID.default = "85B62508254D09"
DeviceID:depends("netpro", "tcpclient")
DeviceID:depends("netpro", "udpclient")

send_interval = s:option(Value, "send_interval", translate ("Heart-Beat Interval(s)"))
send_interval.datatype = "and(uinteger, min(1))"
send_interval.default = "1"
send_interval:depends("netpro", "tcpclient")
send_interval:depends("netpro", "udpclient")


local apply = luci.http.formvalue("cbi.apply")
if apply then
    io.popen("sleep 1s;/etc/setgps 1 &")
end

return m,m2
