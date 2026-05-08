module("luci.controller.serial", package.seeall)

function index()
	--local nport = luci.sys.exec("uci -q show nport")
	--if string.len(nport) ~= 0 then
	if not nixio.fs.access("/etc/config/nport") then
			return
		end
		entry({"admin", "serial"},alias("serial","ttys1"), _("Serial Utility"), 55).index = true
		
		local board = luci.sys.exec("uci -q get system.@system[0].board")
		if string.find(board,"R690") then
			--entry({"admin", "serial","ttys0"}, cbi("serial/port0"), _("RS232"), 3)
			local msm1 = luci.sys.exec("uci -q show nport.@ttyM1[0]") 
			-- if string.len(msm1) ~= 0 then
				-- entry({"admin", "serial","ttys1"}, cbi("serial/port1"), _("GNSS"), 2)
			-- end
			if string.len(msm1) ~= 0 then
				entry({"admin", "serial","ttys1"}, cbi("serial/GNSS"), _("GNSS"), 2)  --上传GPS数据到服务器版本
			end			
			
			local msm2 = luci.sys.exec("uci -q show nport.@ttyM2[0]") 
			if string.len(msm2) ~= 0 then
				entry({"admin", "serial","ttys2"}, cbi("serial/port2"), _("RS485_PORT1"), 6)	
			end
			local msm3 = luci.sys.exec("uci -q show nport.@ttyM3[0]") 
			if string.len(msm3) ~= 0 then
				entry({"admin", "serial","ttys3"}, cbi("serial/port3"), _("RS485_PORT2"), 7)	
			end
			local can1 = luci.sys.exec("uci -q show nport.@ttyCAN1[0]") 
			if string.len(can1) ~= 0 then
				entry({"admin", "serial","ttycan1"}, cbi("serial/port_can1"), _("CAN1"), 8)
			end
			local can2 = luci.sys.exec("uci -q show nport.@ttyCAN2[0]") 
			if string.len(can2) ~= 0 then
				entry({"admin", "serial","ttycan2"}, cbi("serial/port_can2"), _("CAN2"), 9)
			end
			if nixio.fs.access("/etc/config/sms") then
				entry({"admin", "serial","sms"}, cbi("serial/sms"), _("IO SMS alarm"), 10)	
			end		
			if nixio.fs.access("/etc/config/switch") then
				--entry({"admin", "serial","switch"}, cbi("serial/switch"), _("IO switch quantity"), 11)	
			end		
		end
		if string.find(board,"R680") then
			entry({"admin", "serial","ttys1"}, cbi("serial/port1"), _("PORT-RS485"), 3)
			entry({"admin", "serial","ttys2"}, cbi("serial/port2"), _("PORT-RS232"), 4)
		end
		if string.find(board,"RT990") then
			entry({"admin", "serial","ttys1"}, cbi("serial/port1"), _("GNSS"), 2)
		end
	--end
end
