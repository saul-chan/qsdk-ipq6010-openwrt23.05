-- Copyright 2008 Steven Barth <steven@midlink.org>
-- Licensed to the public under the Apache License 2.0.

require("luci.ip")
require("luci.model.uci")

local function set_status(self)
	st.template = "admin_network/iface_status"
	st.network  = self
	st.value    = nil
end

local m = Map("network",
				translate("P2P VPN"), 
				translate("P2P VPN enables encrypted point-to-point connections using the open source WireGuard protocol, which means only devices on your private network can communicate with each other."))

local s = m:section( NamedSection, "p2pvpn", "interface" )
s:tab("general", translate("General Settings"))
s:tab("advanced", translate("Advanced Settings"))
st = s:taboption("general", DummyValue, "__statuswifi", translate("Status"))
st.on_init = set_status("p2pvpn")

enable = s:taboption("general", Flag, "enable", translate("Enable"))
enable.rmempty  = false
severs=s:taboption("general", Value,"server",translate("Server Address"))
username=s:taboption("general", Value,"port",translate("Port"))
password=s:taboption("general", Value,"preauthkey",translate("Preauthkey"))
password.password=true
metric=s:taboption("general", Value,"metric",translate("Metric"),translate("Configure the priority of this network"))
metric.default=7

route=s:taboption("advanced", ListValue,"accept_route",translate("Accept Route"),translate("Allow route creation"))
route:value("accept",translate("ACCEPT"))
route:value("reject",translate("REJECT"))
route.default="accept"
dns=s:taboption("advanced", ListValue,"accept_dns",translate("Accept DNS"),translate("Use DNS on the headscale server"))
dns:value("accept",translate("ACCEPT"))
dns:value("reject",translate("REJECT"))
dns.default="reject"

--route_adv=s:taboption("advanced", DynamicList,"advertise_routes",translate("Advertise Routes"), translate("Host-IP or Network/Netmask. eg. 192.168.2.0/24"))

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

function m.on_after_commit(map)
	fork_exec("/etc/init.d/tailscale_init restart")
end
return m
