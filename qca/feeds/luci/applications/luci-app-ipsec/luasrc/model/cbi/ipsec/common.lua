-- Copyright 2019 Xingwang Liao <kuoruan@gmail.com>
-- Licensed to the public under the MIT License.

local uci = require "luci.model.uci".cursor()
local util = require "luci.util"
local fs = require "nixio.fs"
local sys = require "luci.sys"

local m, s, o

m = Map("ipsec", "%s - %s" % { translate("<br>IPSec"), translate("Common Configuration") },
"<p>%s</p>" % {
	translate("Configure ipsec control parameters, such as enabling debugging.")})

s = m:section(TypedSection, "ipsec")
s.addremove = false
s.anonymous = true


o = s:option(Flag, "debug", translate("Debug"), translate("More debug logs are displayed."))

o = s:option(Flag, "allow_aggressive", translate("Allow Aggressive"), translate("Whether remote aggressive negotiation is allowed."))
o.default="0"

o = s:option(Flag, "rtinstall_enabled", translate("Install Routes"), translate("Installs routes into routing table 220 by default."))
o.default="1"

o = s:option(DynamicList, "ignore_routing_tables", translate("Ignore Routing Tables"))
o = s:option(DynamicList, "interface", translate("Interface"))

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
	fork_exec("/etc/init.d/ipsec reload")
	fork_exec("/etc/init.d/ipsec restart")
	--fork_exec("/etc/init.d/ipsec restart")
end

return m
