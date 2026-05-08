-- Copyright 2019 Xingwang Liao <kuoruan@gmail.com>
-- Licensed to the public under the MIT License.

local dsp = require "luci.dispatcher"

local m, s, o

m = Map("ipsec", "%s - %s" % { translate("<br>IPSec"), translate("Tunnels") },
"<p>%s</p>" % {
	translate("Configure parameters related to the local and peer tunnels. A remote can be configured with multiple tunnels.")})

s = m:section(TypedSection, "tunnel")
--s.anonymous = true
s.addremove = true
--s.sortable = true
s.template = "cbi/tblsection"
s.extedit = luci.dispatcher.build_url(
	"admin", "vpn", "ipsec", "tunnels", "%s"
)
function s.create(self, name)
	AbstractSection.create(self, name)
	luci.http.redirect( self.extedit:format(name) )
end

--o = s:option(Flag, "disabled", translate("Disabled"))

o = s:option(DummyValue, "local_subnet", translate("Local Subnet"))
o.cfgvalue = function (...)
	return Value.cfgvalue(...) or translate("None")
end

o = s:option(DummyValue, "remote_subnet", translate("Remote Subnet"))
o.cfgvalue = function (...)
	return Value.cfgvalue(...) or translate("None")
end

o = s:option(DummyValue, "keyexchange", translate("Key Exchange"))
o.cfgvalue = function (...)
	return Value.cfgvalue(...) or translate("None")
end

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
	fork_exec("/etc/init.d/ipsec restart")
end

return m
