-- Copyright 2019 Xingwang Liao <kuoruan@gmail.com>
-- Licensed to the public under the MIT License.

local dsp = require "luci.dispatcher"

local m, s, o

m = Map("ipsec", "%s - %s" % { translate("<br>IPSec"), translate("Cryptographic Proposal") },
"<p>%s</p>" % {
	translate("Set parameters related to communication negotiation, including one-stage and two-stage negotiation parameters.")})

s = m:section(TypedSection, "crypto_proposal")
--s.anonymous = true
s.addremove = true
--s.sortable = true
s.template = "cbi/tblsection"
s.extedit = luci.dispatcher.build_url(
	"admin", "vpn", "ipsec", "cryptographic", "%s"
)
function s.create(self, name)
	AbstractSection.create(self, name)
	luci.http.redirect( self.extedit:format(name) )
end

--o = s:option(Flag, "disabled", translate("Disabled"))

o = s:option(DummyValue, "encryption_algorithm", translate("Encryption Algorithm"))
o.cfgvalue = function (...)
	return Value.cfgvalue(...) or translate("None")
end

o = s:option(DummyValue, "hash_algorithm", translate("Hash Algorithm"))
o.cfgvalue = function (...)
	return Value.cfgvalue(...) or translate("None")
end

o = s:option(DummyValue, "dh_group", translate("DH Group"))
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
