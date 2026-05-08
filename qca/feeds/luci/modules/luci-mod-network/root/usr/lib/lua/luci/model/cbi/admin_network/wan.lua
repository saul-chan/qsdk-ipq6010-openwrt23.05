local sys   = require "luci.sys"
local dsp = require "luci.dispatcher"
local m, s

local boardname=string.gsub(luci.sys.exec("ubus call system board | grep model | awk -F ' ' '{print$NF}'"),"%s+","")

bold_on  = [[<strong>]]
bold_off = [[</strong>]]

m = Map("network", translate("Network Configuration"))
m:chain("luci")

s = m:section(NamedSection, "wan", "interface", translate("WAN Configuration"))
s.anonymous = true
s.addremove = false
s:tab("basic", translate("Basic Configuration"))
s:tab("advanced", translate("Advanced Configuration"))

local function set_status(self)
	st.template = "admin_network/iface_status"
	st.network  = self
	st.value    = nil
end

st = s:taboption("basic", DummyValue, "__statuswan", translate("Status"))
st.on_init = set_status("wan")

--static
protocol = s:taboption("basic", ListValue, "proto", translate("Protocol"))
protocol.default = "dhcp"
protocol:value("pppoe", translate("PPPoE"))
protocol:value("dhcp", translate("DHCP address"))
protocol:value("static", translate("Static address"))
protocol:value("aslan", translate("Bridge to LAN"))
protocol:value("disable", translate("Disable"))

ipaddr = s:taboption("basic", Value, "ipaddr", translate("IP Address"))
ipaddr.datatype = "ip4addr"
ipaddr.default = "192.168.1.100"
ipaddr:depends({proto="static"})

netmask = s:taboption("basic", Value, "netmask",translate("Netmask"))
netmask.datatype = "ip4addr"
netmask.default = "255.255.255.0"
netmask:value("255.255.255.0")
netmask:value("255.255.0.0")
netmask:value("255.0.0.0")
netmask:depends({proto="static"})

gateway= s:taboption("basic", Value, "gateway", translate("Gateway"))
gateway.datatype = "ip4addr"
gateway:depends({proto="static"})

dns= s:taboption("basic", DynamicList, "dns", translate("DNS"), translate("Note: Click the <em>+</em> Button to confirm adding the DNS before <em>SAVE & APPLY</em>."))
dns.datatype = "ip4addr"
dns.cast     = "string"
dns:depends({proto="static"})


-----------------------PPPoE--------------------------

username = s:taboption("basic", Value, "username",translate("Username"))
username:depends({proto="pppoe"})

password = s:taboption("basic", Value, "password",translate("Password"))
password:depends({proto="pppoe"})
password.password = true

-----------------------advanced--------------------------

ifname = s:taboption("advanced", ListValue, "name", translate("Interface"))
if boardname and string.find(boardname,"AP%-CP01%-C5") then
--ifname:value("eth0", translate("WAN(eth0)"))
ifname:value("eth1", translate("LAN1(eth1)"))
ifname:value("eth2", translate("LAN2(eth2)"))
ifname:value("eth3", translate("LAN3(eth3)"))
ifname:value("eth4", translate("WAN-T1(eth4)"))
ifname.default = "eth4"
elseif boardname and string.find(boardname,"AP%-CP01%-C3") then
ifname:value("eth0", translate("WAN(eth0)"))
ifname:value("eth1", translate("LAN1(eth1)"))
ifname:value("eth2", translate("LAN2(eth2)"))
ifname:value("eth3", translate("LAN3(eth3)"))
elseif boardname and string.find(boardname,"AP%-CP01%-C2") then
ifname:value("eth0", translate("LAN1(eth0)"))
ifname:value("eth1", translate("LAN2(eth1)"))
ifname:value("eth2", translate("LAN3(eth2)"))
ifname:value("eth3", translate("LAN4(eth3)"))
ifname:value("eth4", translate("WAN(eth4)"))
ifname.default = "eth4"
else
ifname:value("eth0", translate("WAN(eth0)"))
ifname:value("eth1", translate("LAN1(eth1)"))
ifname:value("eth2", translate("LAN2(eth2)"))
ifname:value("eth3", translate("LAN3(eth3)"))
ifname:value("eth4", translate("SFP(eth4)"))
ifname.default = "eth0"
end

metric = s:taboption("advanced", Value, "metric", translate("Metric"))
metric.default = "10"

mtu = s:taboption("advanced", Value, "mtu", translate("Override MTU"))
mtu.placeholder = "1500"
mtu.datatype    = "max(9200)"

-- negotiation = s:taboption("advanced", ListValue, "negotiation", translate("Auto-negotiation"))
-- negotiation:value("1", translate("OFF"))
-- negotiation:value("0", translate("ON"))
-- negotiation.default = "1"

-- speed = s:taboption("advanced", ListValue, "speed", translate("Speed"))
-- speed:value("10", translate("10 Mbps"))
-- speed:value("100", translate("100 Mbps"))
-- speed:value("1000", translate("1000 Mbps"))
-- speed.default = "1000"
-- speed:depends({negotiation="0"})

-- duplex = s:taboption("advanced", ListValue, "duplex", translate("Duplex"))
-- duplex:value("half", translate("Half"))
-- duplex:value("full", translate("Full"))
-- duplex.default = "full"
-- duplex:depends({negotiation="0"})

dv = s:taboption("advanced", DummyValue, "_separate")
dv.titleref = dsp.build_url("admin", "network", "network")
dv.rawhtml  = true
dv.title = bold_on .. translate("Show more") .. bold_off
dv.description = translate("Follow this link" .. "<br />" ..
	"You will find more configuration")

function m.on_commit(map)
    io.popen("/etc/init.d/netwan &")
end

return m
