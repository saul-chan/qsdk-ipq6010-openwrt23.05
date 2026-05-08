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
netpro:value("udpserver", "UDP Server")
netpro:value("udpclient", "UDP Client")
netpro:value("modbustcp", "Modbus TCP")

tranpro = s:option(ListValue, "tranpro", translate("Transport Proto"))
tranpro.default = "raw"
tranpro:value("raw", translate("Raw data"))
--tranpro:value("telnet", translate("Telnet (RFC2217)"))
tranpro:depends("netpro", "udpclient")
tranpro:depends("netpro", "udpserver")
tranpro:depends("netpro", "tcpserver")

localport = s:option(Value, "localport", translate ("Local Port1"), translate ("Connect to this port to receive GNSS raw data."))
localport.default = "3030"
localport.datatype="port"
localport:depends("netpro", "tcpserver")
localport:depends("netpro", "udpserver")
localport:depends("netpro", "udpclient")
localport:depends("netpro", "modbustcp")

lport = s:option(ListValue, "lport", translate ("Local Port2"), translate ("Connect to this port to receive GNSS parsed data."))
lport:value("1024", translate("1024"))
lport:depends("netpro", "tcpserver")
lport:depends("netpro", "udpserver")
lport:depends("netpro", "udpclient")
lport:depends("netpro", "modbustcp")

maxconn = s:option(ListValue, "maxconn", translate ("Maximum number"))
maxconn.default = "6"
maxconn:value("1", translate("1"))
maxconn:value("2", translate("2"))
maxconn:value("3", translate("3"))
maxconn:value("4", translate("4"))
maxconn:value("5", translate("5"))
maxconn:value("6", translate("6"))
maxconn:depends("netpro", "udpserver")
maxconn:depends("netpro", "tcpserver")
maxconn:depends("netpro", "modbustcp")
maxconn:depends("netpro", "udpclient")

timeout = s:option(Value, "timeout", translate ("Time Out(s)"))
timeout.default = "300"
timeout:depends("netpro", "udpserver")
timeout:depends("netpro", "tcpserver")
timeout:depends("netpro", "udpclient")
timeout:depends("netpro", "modbustcp")

serverip = s:option(Value, "serverip", translate("Server IP Address"))
serverip.datatype = "ip4addr"
serverip.default = "192.168.0.100"
serverip:depends("netpro", "tcpclient")
serverip:depends("netpro", "udpclient")

serverport = s:option(Value, "serverport", translate ("Server Port"))
serverport.datatype="port"
serverport.default = "10000"
serverport:depends("netpro", "tcpclient")
serverport:depends("netpro", "udpclient")

hb = s:option(Flag, "hb", translate("Heart-Beat"))
hb.rmempty = false
hb:depends("netpro", "tcpclient")

hbp = s:option(Value, "hbp", translate("Heart-Beat Content"))
hbp.default = "FF012345678"
hbp:depends("hb", "1")

hex = s:option(Flag, "hex", translate("Hexadecimal"))
hex:depends("hb", "1")

hbpt = s:option(Value, "hbpt", translate("Heart-Beat Interval(s)"))
hbpt.datatype = "and(uinteger, min(1))"
hbpt.default = "30"
hbpt:depends("hb", "1")

s2 = m:section(TypedSection, "ttyM1", translate("GNSS Settings"))
s2.anonymous = true
s2.addremove = false

s2:tab("Serial", translate("Serial Settings"))
s2:tab("RTK", translate("RTK Settings"))


baud = s2:taboption("Serial", ListValue, "baud", translate("Baud Rate"))
baud.default = "9600"
baud:value("300", translate("300"))
baud:value("600", translate("600"))
baud:value("1200", translate("1200"))
baud:value("2400", translate("2400"))
baud:value("4800", translate("4800"))
baud:value("9600", translate("9600"))
baud:value("19200", translate("19200"))
baud:value("38400", translate("38400"))
baud:value("57600", translate("57600"))
baud:value("115200", translate("115200"))
baud:value("230400", translate("230400"))
baud:value("460800", translate("460800"))
baud:value("921600", translate("921600"))

data = s2:taboption("Serial", ListValue, "data", translate("Data bits"))
data.default = "8DATABITS"
data:value("7DATABITS", translate("7"))
data:value("8DATABITS", translate("8"))

stop = s2:taboption("Serial", ListValue, "stop", translate("Stop bits"))
stop.default = "1STOPBIT"
stop:value("1STOPBIT", translate("1"))
stop:value("2STOPBITS", translate("2"))

parity = s2:taboption("Serial", ListValue, "parity", translate("Parity"))
parity.default = "NONE"
parity:value("NONE", translate("None"))
parity:value("ODD", translate("Odd"))
parity:value("EVEN", translate("Even"))

enable = s2:taboption("RTK",Flag, "NTRIP_enable", translate("Enable"))
enable.rmempty  = false
enable_ssl = s2:taboption("RTK",Flag, "SSL_enable", translate("Enable SSL/TLS"))
enable_ssl.rmempty  = false
NTRIP_IP = s2:taboption("RTK", Value, "NTRIP_IP", translate("address") ,translate("RTK server IP address"))
NTRIP_PORT = s2:taboption("RTK", Value, "NTRIP_PORT", translate("Port") ,translate("RTK server Port"))
NTRIP_USERNAME = s2:taboption("RTK", Value, "NTRIP_USERNAME", translate("Username") ,translate("Username"))
NTRIP_PASSWORD = s2:taboption("RTK", Value, "NTRIP_PASSWORD", translate("Password") ,translate("Password"))
NTRIP_MOUNT_POINT = s2:taboption("RTK", Value, "NTRIP_MOUNT_POINT", translate("Mount point") ,translate("Mount point"))


local apply = luci.http.formvalue("cbi.apply")
if apply then
    io.popen("sleep 1s;/etc/setusb 1 &")
    io.popen("sleep 1s;/etc/init.d/slkgpsinit restart &")
    io.popen("sleep 1s;/etc/init.d/slkgps_rtkinit restart &")
end

return m,m2
