-- Copyright 2012 Christian Gagneraud <chris@techworks.ie>
-- Licensed to the public under the Apache License 2.0.

local nw = require "luci.model.network"


m = Map("wificonfig",translate("Wireless Network"),translate("Configure WiFi 2.4G"))

nw.init(m.uci)

s = m:section(NamedSection,"wlan1","wifi-iface")
s.anonymous = true
s.addremove = false

s:tab("basic", translate("Basic Configuration"))
s:tab("advanced", translate("Advanced Configuration"))
s:tab("macfilter", translate("MAC-Filter"))

en=s:taboption("basic", Flag, "en", translate("Enabled"))

ssid=s:taboption("basic", Value, "ssid", translate("WiFi Name"))
ssid.default="WiFi_2G"

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
htmode:value("HT40","HT40")

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

m2 = Map("wificonfig"," ",translate("Configure WiFi 5.8G"))

s2 = m2:section(NamedSection,"wlan0","wifi-iface")
s2.anonymous = true
s2.addremove = false

s2:tab("basic", translate("Basic Configuration"))
s2:tab("advanced", translate("Advanced Configuration"))
s2:tab("macfilter", translate("MAC-Filter"))

en=s2:taboption("basic", Flag, "en", translate("Enabled"))

ssid=s2:taboption("basic", Value, "ssid", translate("WiFi Name"))
ssid.default="WiFi_5G"

security = s2:taboption("basic", ListValue, "security", translate("Security"))
security:value("none", translate("No Encryption"))
--security:value("WPA", translate("WPA"))
security:value("WPA2", translate("WPA2"))
security:value("WPA/WPA2PSK", translate("WPA/WPA2 PSK"))

if string.find(luci.sys.exec("slktool /dev/mmcblk0p25 undefined1"),"huaruan") then
	key=s2:taboption("basic", Value, "key", translate("WiFi Key"), translate("Password strength needs to meet the requirements.</br>Including uppercase and lowercase letters, numbers, special characters (~!@#$%^&*.,;); the minimum is not less than 8 characters."))
	key:depends({security="WPA"})
	key:depends({security="WPA2"})
	key:depends({security="WPA/WPA2PSK"})
	key.password=true
	--key.rmempty=false
	key.datatype = "hrkey"
else
	key=s2:taboption("basic", Value, "key", translate("WiFi Key"), translate("The key must contain at least 8 characters"))
	key:depends({security="WPA"})
	key:depends({security="WPA2"})
	key:depends({security="WPA/WPA2PSK"})
	key.password=true
	key.datatype = "wpakey"
end

hidden=s2:taboption("advanced", Flag, "hidden", translate("Hide ESSID"))

wds=s2:taboption("advanced", Flag, "wds", translate("WDS Enabled"))

channel=s2:taboption("advanced", ListValue, "channel", translate("Channel"))
channel.default="auto"
channel:value("auto",translate("auto"))
--channel:value("36","36 (5180MHz)")
channel:value("40","40 (5200MHz)")
channel:value("44","44 (5220MHz)")
channel:value("48","48 (5240MHz)")
channel:value("52","52 (5260MHz)")
channel:value("56","56 (5280MHz)")
channel:value("60","60 (5300MHz)")
channel:value("64","64 (5320MHz)")
channel:value("149","149 (5745MHz)")
channel:value("153","153 (5765MHz)")
channel:value("157","157 (5785MHz)")
channel:value("161","161 (5805MHz)")
--channel:value("165","165 (5825MHz)")
 
-- channel:value("100","100 (5500MHz)")
-- channel:value("104","104 (5520MHz)")
-- channel:value("108","108 (5540MHz)")
-- channel:value("112","112 (5660MHz)")
-- channel:value("116","116 (5580MHz)")
-- channel:value("120","120 (5600MHz)")
-- channel:value("124","124 (5620MHz)")
-- channel:value("128","128 (5640MHz)")
-- channel:value("136","136 (5680MHz)")
-- channel:value("140","140 (5700MHz)")
-- channel:value("144","144 (5720MHz)")

htmode=s2:taboption("advanced", ListValue, "htmode", translate("Width"))
htmode.default=htmoded
htmode:value("HT20","HT20")
htmode:value("HT40","HT40")
htmode:value("HT80","HT80")

wpa_group_rekey=s2:taboption("advanced", Value, "wpa_group_rekey", translate("WPA Group Rekey"))
wpa_group_rekey.default="0"

wpa_pair_rekey=s2:taboption("advanced", Value, "wpa_pair_rekey", translate("WPA Pair Rekey"))
wpa_pair_rekey.default="0"

wpa_master_rekey=s2:taboption("advanced", Value, "wpa_master_rekey", translate("WPA Master Rekey"))
wpa_master_rekey.default="0"

-- txpower=s2:taboption("advanced", ListValue, "txpower", translate("Transmit Power"))
-- txpower:value("15",translate("Low"))
-- txpower:value("21",translate("Medium"))
-- txpower:value("27",translate("High"))

-- kick_en=s2:taboption("advanced", Flag, "enbale_kick", translate("Enabled Kick Client"))

-- rssi=s2:taboption("advanced", Value, "rssi", translate("Kick Client Rssi Low"), translate("The client is kicked out when the signal value is lower than the specified value."))
-- rssi.default="-75"
-- rssi:depends({enbale_kick="1"})

mp = s2:taboption("macfilter", ListValue, "macfilter", translate("MAC-Address Filter"))
mp:value("", translate("disable"))
mp:value("allow", translate("Allow listed only"))
mp:value("deny", translate("Allow all except listed"))

ml = s2:taboption("macfilter", DynamicList, "maclist", translate("MAC-List"))
ml.datatype = "macaddr"
ml:depends({macfilter="allow"})
ml:depends({macfilter="deny"})
luci.ip.neighbors({ family = 4 }, function(n)
	if n.mac and n.dest then
		ml:value(n.mac, "%s (%s)" %{  n.mac, n.dest:string() })
	end
end)

function m2.on_after_commit(map)
	local key1 = luci.http.formvalue("cbid.wificonfig.wlan1.key")
	local key0 = luci.http.formvalue("cbid.wificonfig.wlan0.key")
	if not key1 or key1 == nil or key1 == "" then
		luci.sys.exec("uci set wificonfig.wlan1.security=none")
		
	end
	if not key0 or key0 == nil or key0 == "" then
		luci.sys.exec("uci set wificonfig.wlan0.security=none")
	end
	luci.sys.exec("uci commit wificonfig")
	io.popen("/etc/init.d/wifi_init restart")
end

return m,m2
