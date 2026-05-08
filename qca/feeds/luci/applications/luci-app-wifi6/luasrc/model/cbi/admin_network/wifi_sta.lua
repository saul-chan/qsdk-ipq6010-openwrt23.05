-- Copyright 2012 Christian Gagneraud <chris@techworks.ie>
-- Licensed to the public under the Apache License 2.0.

local wific="sta0"

local nw = require "luci.model.network"
m = Map("wificonfig",translate("General Setup"))

s = m:section(NamedSection,wific,"wifi-iface")
s.anonymous = true

-----------------------Status--------------------------

local function set_status(self)
	st.template = "admin_network/iface_status"
	st.network  = self
	st.value    = nil
end


local proto  = luci.sys.exec("uci get network.@internets[0].proto")
st = s:option(DummyValue, "__statuswifi", translate("Status"))
st.on_init = set_status("wific")


-----------------------WIFI Client--------------------------
enable = s:option(Flag, "en", translate("Enable"))
enable.rmempty = false

local wifiinfo=string.gsub(luci.sys.exec("uci get wificonfig."..wific..".device"),"%s+","")
infos = s:option(ListValue, "device", translate("WiFi Interface"))
infos.default=wifiinfo--string.gsub(luci.sys.exec("uci set wireless."..wnet.sid..".device"),"%s+","")
infos:depends({en="1"})
infos:value("wifi1",translate("2.4G Client"))
infos:value("wifi0",translate("5.8G Client"))

-- scan_btn = s:option(Button, "", translate("Scan"))
-- scan_btn.inputtitle = translate("Scan")
-- scan_btn:depends({en="1"})

-- function scan_btn.write()
-- one = m:formvalue("cbid.wificonfig.%s.device" % wific)
-- if one == "wifi0" then
	-- one="wifi0"
-- else
	-- one="wifi1"
-- end

-- local iw = luci.sys.wifi.getiwinfo(one)
-- for k, v in ipairs(iw.scanlist or { }) do

	-- if v.ssid then
		-- ssid:value(v.ssid, "%s" %{ v.ssid })
	-- end
-- end

-- end

-- ssid = s:option(Value, "ssid", translate("SSID"), translate("Click the scan button and select the wifi name to be connected"))
-- ssid.validator = 'maxlength="32"'
-- ssid.placeholder = translate("Enter or click search")--translate("SSID is required")
-- ssid:depends({en="1"})
-- ssid.default= translate("Select wifi ssid")

scan_btn = s:option(Button, "", translate("Scan"))
scan_btn.inputtitle = translate("Scan")
scan_btn:depends({en="1"})

-- 辅助函数：将 \xXX 格式的 Unicode 转成可读字符串
local function unicode_to_utf8(str)
    -- 匹配 \xXX 格式的编码
    return str:gsub("\\x(%x%x)", function(hex)
        return string.char(tonumber(hex, 16))  -- 转换为字符
    end)
end

function scan_btn.write()
    local device = m:formvalue("cbid.wificonfig.%s.device" % wific)
    device = (device == "wifi0") and "ath0" or "ath1"  -- 根据实际情况调整

    -- 执行 iw scan 并提取 SSID（包括 Unicode 编码的）
    local cmd = string.format("iw dev %s scan | grep -E '^[[:blank:]]+SSID: ' | sed 's/^[[:space:]]*SSID: //'", device)
    local scan_output = luci.sys.exec(cmd)

    -- 清空现有 SSID 选项
    ssid:value("", translate("Select wifi ssid"))

    -- 逐行解析并转换 Unicode SSID
    for line in scan_output:gmatch("[^\r\n]+") do
        if line ~= "" then
            local ssid_name = line
            -- 检查是否是 \xXX 编码的中文 SSID
            if ssid_name:match("\\x%x%x") then
                ssid_name = unicode_to_utf8(ssid_name)  -- 转换 Unicode 到 UTF-8
            end
            ssid:value(ssid_name, ssid_name)  -- 添加到下拉菜单
        end
    end
end

ssid = s:option(Value, "ssid", translate("SSID"), translate("Click the scan button and select the wifi name to be connected"))
ssid.validator = 'maxlength="32"'
ssid.placeholder = translate("Enter or click search")
ssid:depends({en="1"})
ssid.default = translate("Select wifi ssid")

enc = s:option(ListValue, "security", translate("Security"))
enc:value("none", translate("No Encryption"))
--enc:value("WPA", translate("WPA"))
enc:value("WPA2", translate("WPA2"))
enc:value("WPA/WPA2-PSK", translate("WPA/WPA2-PSK"))
enc:depends({en="1"})

key = s:option(Value, "key", translate("Key"))
key.datatype = "wpakey"
key.placeholder = translate("Key")
key.password = true
key:depends({security="WPA"})
key:depends({security="WPA2"})
key:depends({security="WPA/WPA2-PSK"})
key.errmsg = translate("Key can't be empty")

wds = s:option(Flag, "wds", translate("WDS"))
wds.default = "0"
wds.rmempty = false
wds.submit = true
wds:depends({en="1"})


function fork_exec(command)
	local pid = nixio.fork()
	if pid > 0 then
		return
	elseif pid == 0 then
		-- change to root dir
		nixio.chdir("/")

		-- patch stdin, out, err to /dev/null
		local null = nixio.open("/dev/null", "w+")
		if null then
			nixio.dup(null, nixio.stderr)
			nixio.dup(null, nixio.stdout)
			nixio.dup(null, nixio.stdin)
			if null:fileno() > 2 then
				null:close()
			end
		end

		-- replace with target command
		nixio.exec("/bin/sh", "-c", command)
	end
end

local sys   = require "luci.sys"
local s, m2 ,s2

m2 = Map("wificonfig", translate("Common Settings"))
m2:chain("luci")

s2 = m2:section(NamedSection, "wific", "interface")
s2.anonymous = true
s2.addremove = false
s2:tab("advanced", translate("Advanced Configuration"))
--s2:tab("2.4G_channle", translate("2.4G channel settings"))
--s2:tab("5.8G_channle", translate("5.8G channel settings"))
--s2:tab("MAC_match", translate("MAC match"))
-----------------------WIFI Client--------------------------
--rssi_enable = s2:taboption("advanced",Flag, "en", translate("RSSI_Enable"))
--rssi_enable.rmempty = false

--rssi_threshold = s2:taboption("advanced",Value, "rssi_threshold", translate("rssi_threshold"))
--rssi_threshold.default = "-85"

--Threshold_difference = s2:taboption("advanced",Value, "Threshold_difference", translate("Threshold_difference"),translate("WiFi switching occurs when the threshold difference exceeds the set value"))
--Threshold_difference.default = "10"

proto = s2:taboption("advanced",ListValue, "proto", translate("Protocol"), translate("LAN port of bridge should share the same network segment with upstream equipment"))
proto:value("dhcp", translate("DHCP address"))
proto:value("static", translate("Static address"))
proto:value("bridgelan", translate("Bridge Lan"))

ipaddr = s2:taboption("advanced",Value, "ipaddr", translate("IP Address"))
ipaddr.datatype = "ip4addr"
ipaddr.default = "192.168.1.100"
ipaddr:depends({proto="static"})

netmask = s2:taboption("advanced",Value, "netmask",translate("Netmask"))
netmask.datatype = "ip4addr"
netmask.default = "255.255.255.0"
netmask:value("255.255.255.0")
netmask:value("255.255.0.0")
netmask:value("255.0.0.0")
netmask:depends({proto="static"})

gateway= s2:taboption("advanced",Value, "gateway", translate("Gateway"))
gateway.datatype = "ip4addr"
gateway:depends({proto="static"})
gateway:depends({proto="bridgelan"})

dns= s2:taboption("advanced",DynamicList, "dns", translate("DNS"), translate("Note: Click the <em>+</em> Button to confirm adding the DNS before <em>SAVE & APPLY</em>."))
dns.datatype = "ip4addr"
dns.cast     = "string"
dns:depends({proto="static"})
dns:depends({proto="bridgelan"})

metric = s2:taboption("advanced",Value, "metric", translate("Metric"))
metric.default = "20"
metric:depends({proto="static"})
metric:depends({proto="dhcp"})



--查询模块型号--
-- function country_code(name)
	-- country_code=string.gsub(luci.sys.exec("uci -q get wificonfig.wific.country | grep "..name.." | wc -l"),"%s+","")
	-- return country_code
-- end


-- channel= s2:taboption("2.4G_channle",MultiValue, "channel_2", translate("channel selection"))
-- channel.rmempty = true
-- channel:value("2412","1 (2412MHz)")
-- channel:value("2417","2 (2417MHz)")
-- channel:value("2422","3 (2422MHz)")
-- channel:value("2427","4 (2427MHz)")
-- channel:value("2432","5 (2432MHz)")
-- channel:value("2437","6 (2437MHz)")
-- channel:value("2442","7 (2442MHz)")
-- channel:value("2447","8 (2447MHz)")
-- channel:value("2452","9 (2452MHz)")
-- channel:value("2457","10 (2457MHz)")
-- channel:value("2462","11 (2462MHz)")
-- channel:value("2472","12 (2472MHz)")
-- channel:value("2477","13 (2477MHz)")
-- channel:value("2484","14 (2484MHz)")




-- country = s2:taboption("5.8G_channle", ListValue, "country", translate("Country"))
-- country.rmempty = false
-- country:value("CN", translate("China"))
-- country:value("JP", translate("Japan"))
-- country:value("KR", translate("Republic of Korea"))
-- country:value("US", translate("America"))

-- channel_5 = s2:taboption("5.8G_channle",MultiValue, "channel_5", translate("channel selection"))
-- channel_5.rmempty = true
-- channel_5:value("5180","36 (5180MHz)")
-- channel_5:value("5200","40 (5200MHz)")
-- channel_5:value("5220","44 (5220MHz)")
-- channel_5:value("5240","48 (5240MHz)")
-- channel_5:value("5260","52 (5260MHz)")
-- channel_5:value("5280","56 (5280MHz)")
-- channel_5:value("5300","60 (5300MHz)")
-- channel_5:value("5320","64 (5320MHz)")
-- channel_5:value("5745","149 (5745MHz)")
-- channel_5:value("5765","153 (5765MHz)")
-- channel_5:value("5785","157 (5785MHz)")
-- channel_5:value("5805","161 (5805MHz)")
-- channel_5:value("5825","165 (5825MHz)") 
-- channel_5:depends("country","CN")


-- channel_55= s2:taboption("5.8G_channle",MultiValue, "channel_55", translate("channel selection"))
-- channel_55.rmempty = true
-- channel_55:value("5180","36 (5180MHz)")
-- channel_55:value("5200","40 (5200MHz)")
-- channel_55:value("5220","44 (5220MHz)")
-- channel_55:value("5240","48 (5240MHz)")
-- channel_55:value("5260","52 (5260MHz)")
-- channel_55:value("5280","56 (5280MHz)")
-- channel_55:value("5300","60 (5300MHz)")
-- channel_55:value("5320","64 (5320MHz)")
-- channel_55:value("5500","100 (5500MHz)")
-- channel_55:value("5520","104 (5520MHz)")
-- channel_55:value("5540","108 (5540MHz)")
-- channel_55:value("5660","112 (5660MHz)")
-- channel_55:value("5580","116 (5580MHz)")
-- channel_55:value("5600","120 (5600MHz)")
-- channel_55:value("5620","124 (5620MHz)")
-- channel_55:value("5640","128 (5640MHz)")
-- channel_55:value("5680","136 (5680MHz)")
-- channel_55:value("5700","140 (5700MHz)")
-- channel_55:value("5720","144 (5720MHz)")
-- channel_55:value("5745","149 (5745MHz)")
-- channel_55:value("5765","153 (5765MHz)")
-- channel_55:value("5785","157 (5785MHz)")
-- channel_55:value("5805","161 (5805MHz)")
-- channel_55:value("5825","165 (5825MHz)")  
-- channel_55:depends("country","JP")
-- channel_55:depends("country","US")
-- channel_55:depends("country","KR")

-- MAC=s2:taboption("MAC_match",TextValue, "MAC", translate("MAC_match"))
-- MAC.template = "cbi/wifi_tvalue"


return m,m2
