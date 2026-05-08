module("luci.controller.modem", package.seeall)

function index()
	-- if nixio.fs.access("/etc/config/modem") then
		-- local get_sim=luci.sys.exec("uci -q get modem.@default[0]")
		-- if string.len(get_sim) ~= 0 then
			-- local types=luci.sys.exec("uci -q get modem.@default[0].net")
			-- if string.find(types,"4G") then
				-- page=entry({"admin", "network", "modem"}, cbi("admin_network/modem"), _("4G Modem"), 1)
			-- else
				-- page=entry({"admin", "network", "modem"}, cbi("admin_network/modem"), _("5G Modem"), 1)
			-- end
		-- end
		-- local get_sim1=luci.sys.exec("uci -q get modem.SIM1")
		-- if string.len(get_sim1) ~= 0 then
			-- page=entry({"admin", "network", "modem1"}, cbi("admin_network/modem1"), _("SIM1 Setting"), 1)
		-- end
		-- local get_sim2=luci.sys.exec("uci -q get modem.SIM2")
		-- if string.len(get_sim2) ~= 0 then
			-- page=entry({"admin", "network", "modem2"}, cbi("admin_network/modem2"), _("SIM2 Setting"), 2)
		-- end
		-- local get_sim2=luci.sys.exec("uci -q get modem.SIM3")
		-- if string.len(get_sim2) ~= 0 then
			-- page=entry({"admin", "network", "modem3"}, cbi("admin_network/modem3"), _("SIM3 Setting"), 3)
		-- end
		-- local get_sim2=luci.sys.exec("uci -q get modem.SIM4")
		-- if string.len(get_sim2) ~= 0 then
			-- page=entry({"admin", "network", "modem4"}, cbi("admin_network/modem4"), _("SIM4 Setting"), 4)
		-- end
	-- end
end
