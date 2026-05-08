-- Copyright 2019 Xingwang Liao <kuoruan@gmail.com>
-- Licensed to the public under the MIT License.

local dsp = require "luci.dispatcher"

local m, s, o


m = Map("ipsec", "%s - %s" % { translate("<br>IPSec"), translate("Remote Configuration") },
"<p>%s</p>" % {
	translate("Configurations for common remote access, supporting multiple remote peer.")})

s = m:section(TypedSection, "remote")
--s.anonymous = true
s.addremove = true
--s.sortable = true
s.template = "cbi/tblsection"
s.extedit = luci.dispatcher.build_url(
	"admin", "vpn", "ipsec", "remotes", "%s"
)
--s.extedit = dsp.build_url("admin/vpn/ipsec/remotes/%s")
-- function s.create(...)
	-- local sid = TypedSection.create(...)
	-- if sid then
		-- m.uci:save("ipsec")
		-- luci.http.redirect(s.extedit % sid)
		-- return
	-- end
-- end

function s.create(self, name)
	AbstractSection.create(self, name)
	luci.http.redirect( self.extedit:format(name) )
end

o = s:option(Flag, "enabled", translate("Enable"))
o.cfgvalue = function (...)
	return Value.cfgvalue(...) or "0"
end

o = s:option(DummyValue, "gateway", translate("Gateway"))
o.cfgvalue = function (...)
	return Value.cfgvalue(...) or translate("None")
end

o = s:option(DummyValue, "pre_shared_key", translate("PSK"))
o.cfgvalue = function (...)
	return Value.cfgvalue(...) or translate("None")
end

o = s:option(DummyValue, "local_identifier", translate("Local ID"))
o.cfgvalue = function (...)
	return Value.cfgvalue(...) or translate("None")
end

o = s:option(DummyValue, "remote_identifier", translate("Remote ID"))
o.cfgvalue = function (...)
	return Value.cfgvalue(...) or translate("None")
end

o = s:option(DummyValue, "authentication_method", translate("Authentication Method"))
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
