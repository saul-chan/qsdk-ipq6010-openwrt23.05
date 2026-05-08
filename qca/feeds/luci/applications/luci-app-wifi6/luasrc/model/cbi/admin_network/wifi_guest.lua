-- Copyright 2012 Christian Gagneraud <chris@techworks.ie>
-- Licensed to the public under the Apache License 2.0.

local nw = require "luci.model.network"


m = Map("wificonfig",translate("Wireless Guest"))

nw.init(m.uci)

s = m:section(NamedSection,"guest0","wifi-iface")
s.anonymous = true
s.addremove = false

s:tab("basic", translate("Basic Configuration"))
s:tab("advanced", translate("Advanced Configuration"))
s:tab("macfilter", translate("MAC-Filter"))

en=s:taboption("basic", Flag, "en", translate("Enabled"))

device=s:taboption("basic", ListValue, "device", translate("WiFi Interface"))
device:value("wifi1",translate("2.4G"))
device:value("wifi0",translate("5.8G"))
device.default='wifi0'

ssid=s:taboption("basic", Value, "ssid", translate("WiFi Name"))
ssid.default="WiFi_Guest"

security = s:taboption("basic", ListValue, "security", translate("Security"))
security:value("none", translate("No Encryption"))
--security:value("WPA", translate("WPA"))
security:value("WPA2", translate("WPA2"))
security:value("WPA/WPA2PSK", translate("WPA/WPA2 PSK"))
if string.find(luci.sys.exec("slktool /dev/mmcblk0p25 undefined1"),"huaruan") then
	key=s:taboption("basic", Value, "key", translate("WiFi Key"), translate("Password strength needs to meet the requirements.</br>Including uppercase and lowercase letters, numbers, special characters (~!@#$%^&*.,;); the minimum is not less than 8 characters."))
	key:depends({security="WPA"})
	key:depends({security="WPA2"})
	key:depends({security="WPA/WPA2PSK"})
	key.password=true
	--key.rmempty=false
	key.datatype = "hrkey"
else
	key=s:taboption("basic", Value, "key", translate("WiFi Key"), translate("The key must contain at least 8 characters"))
	key:depends({security="WPA"})
	key:depends({security="WPA2"})
	key:depends({security="WPA/WPA2PSK"})
	key.password=true	
	key.datatype = "wpakey"
end

hidden=s:taboption("advanced", Flag, "hidden", translate("Hide ESSID"))

wds=s:taboption("advanced", Flag, "wds", translate("WDS Enabled"))

------------------------channel-------------------------
channel=s:taboption("advanced", ListValue, "channel", translate("Channel"))
channel.default="auto"
channel:value("auto",translate("auto"))
channel:value("1","1 (2412MHz)")
channel:value("2","2 (2417MHz)")
channel:value("3","3 (2422MHz)")
channel:value("4","4 (2427MHz)")
channel:value("5","5 (2432MHz)")
channel:value("6","6 (2437MHz)")
channel:value("7","7 (2442MHz)")
channel:value("8","8 (2447MHz)")
channel:value("9","9 (2452MHz)")
channel:value("10","10 (2457MHz)")
channel:value("11","11 (2462MHz)")
--channel:value("12","12 (2472MHz)")
--channel:value("13","13 (2477MHz)")
--channel:value("14","14 (2484MHz)")

htmode=s:taboption("advanced", ListValue, "htmode", translate("Width"))
htmode.default=htmoded
htmode:value("HT20","HT20")
--htmode:value("HT40","HT40")

wpa_group_rekey=s:taboption("advanced", Value, "wpa_group_rekey", translate("WPA Group Rekey"))
wpa_group_rekey.default="0"

wpa_pair_rekey=s:taboption("advanced", Value, "wpa_pair_rekey", translate("WPA Pair Rekey"))
wpa_pair_rekey.default="0"

wpa_master_rekey=s:taboption("advanced", Value, "wpa_master_rekey", translate("WPA Master Rekey"))
wpa_master_rekey.default="0"

mp = s:taboption("macfilter", ListValue, "macfilter", translate("MAC-Address Filter"))
mp:value("", translate("disable"))
mp:value("allow", translate("Allow listed only"))
mp:value("deny", translate("Allow all except listed"))

ml = s:taboption("macfilter", DynamicList, "maclist", translate("MAC-List"))
ml.datatype = "macaddr"
ml:depends({macfilter="allow"})
ml:depends({macfilter="deny"})
luci.ip.neighbors({ family = 4 }, function(n)
	if n.mac and n.dest then
		ml:value(n.mac, "%s (%s)" %{  n.mac, n.dest:string() })
	end
end)

-- txpower=s:taboption("advanced", ListValue, "txpower", translate("Transmit Power"))
-- txpower:value("15",translate("Low"))
-- txpower:value("21",translate("Medium"))
-- txpower:value("27",translate("High"))

-- kick_en=s:taboption("advanced", Flag, "enbale_kick", translate("Enabled Kick Client"))

-- rssi=s:taboption("advanced", Value, "rssi", translate("Kick Client Rssi Low"), translate("The client is kicked out when the signal value is lower than the specified value."))
-- rssi.default="-75"
-- rssi:depends({enbale_kick="1"})




s2 = m:section(NamedSection, "guest", "interface")
s2.anonymous = true
s2.addremove = false

-- proto = s2:taboption("advanced",ListValue, "proto", translate("Protocol"), translate("LAN port of bridge should share the same network segment with upstream equipment"))
-- proto:value("dhcp", translate("DHCP address"))
-- proto:value("static", translate("Static address"))
-- proto:value("bridgelan", translate("Bridge Lan"))

ipaddr = s2:option(Value, "ipaddr", translate("IP Address"))
ipaddr.datatype = "ip4addr"
ipaddr.default = "192.168.100.1"

netmask = s2:option(Value, "netmask",translate("Netmask"))
netmask.datatype = "ip4addr"
netmask.default = "255.255.255.0"
netmask:value("255.255.255.0")
netmask:value("255.255.0.0")
netmask:value("255.0.0.0")

-- gateway= s2:taboption("advanced",Value, "gateway", translate("Gateway"))
-- gateway.datatype = "ip4addr"
-- gateway:depends({proto="static"})
-- gateway:depends({proto="bridgelan"})

-- dns= s2:taboption("advanced",DynamicList, "dns", translate("DNS"), translate("Note: Click the <em>+</em> Button to confirm adding the DNS before <em>SAVE & APPLY</em>."))
-- dns.datatype = "ip4addr"
-- dns.cast     = "string"
-- dns:depends({proto="static"})
-- dns:depends({proto="bridgelan"})

-- metric = s2:taboption("advanced",Value, "metric", translate("Metric"))
-- metric.default = "20"
-- metric:depends({proto="static"})
-- metric:depends({proto="dhcp"})

-- function m2.on_after_commit(map)
	-- local key1 = luci.http.formvalue("cbid.wificonfig.wlan1.key")
	-- local key0 = luci.http.formvalue("cbid.wificonfig.wlan0.key")
	-- if not key1 or key1 == nil or key1 == "" then
		-- luci.sys.exec("uci set wificonfig.wlan1.security=none")
		
	-- end
	-- if not key0 or key0 == nil or key0 == "" then
		-- luci.sys.exec("uci set wificonfig.wlan0.security=none")
	-- end
	-- luci.sys.exec("uci commit wificonfig")
	-- io.popen("/etc/init.d/wifi_init restart")
-- end

return m,m2
