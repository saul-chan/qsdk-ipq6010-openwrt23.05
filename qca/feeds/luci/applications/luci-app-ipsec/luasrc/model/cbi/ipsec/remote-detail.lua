-- Copyright 2019 Xingwang Liao <kuoruan@gmail.com>
-- Licensed to the public under the MIT License.

local dsp = require "luci.dispatcher"
local uci = require "luci.model.uci".cursor()

local m, s, o
local proposal_table = { }
local tunnel_table = { }

local sid = arg[1]

m = Map("ipsec", "%s - %s : %s" % { translate("<br>IPSec"), translate("Edit IPSec Remote"), sid})
m.redirect = dsp.build_url("admin/vpn/ipsec/remotes")

if m.uci:get("ipsec", sid) ~= "remote" then
	luci.http.redirect(m.redirect)
	return
end

s = m:section(NamedSection, sid, "remote")
s.anonymous = true
s.addremove = false

o = s:option(Flag, "enabled", translate("Enabled"))
--o.datatype = "host"
o.rmempty = false

o = s:option(Value, "gateway", translate("Gateway"), translate("IP address or domain name of the remote server. If the value is %any, it indicates that the remote connection is not initiated."))
--o.datatype = "host"
o.rmempty = false

o = s:option(Value, "pre_shared_key", translate("PSK"), translate("Pre shared key."))
o.password = true
--o.rmempty = false

o = s:option(Value, "local_identifier", translate("Local ID"), translate("Specify your local ID."))

o = s:option(Value, "remote_identifier", translate("Remote ID"), translate("Specify the remote ID, can be left blank."))

o = s:option(ListValue, "authentication_method", translate("Authentication Method"))
o:value("psk")

o = s:option(ListValue, "aggressive_mode", translate("Exchange Mode"), translate("Whether to use IKEv1 Aggressive or Main Mode (the default)"))
o:value("yes",translate("Aggressive Mode"))
o:value("",translate("Main Mode"))
o.default=""

uci:foreach("ipsec", "crypto_proposal", function(s)
	if s['.name'] then
		proposal_table[s[".name"]] = s['.name']
	end
end)

o = s:option(DynamicList, "crypto_proposal", translate("IKE Cryptographic Proposal"), translate("Choose the cryptographic proposal to be used in negotiations."))
o:value("", translate("None"))
for k, v in pairs(proposal_table) do
	o:value(k, v)
end

o = s:option(Value, "ikelifetime", translate("IKE Lifetime"), translate("Time to schedule CHILD_SA rekeying.(s/m/h)"))
o.placeholder="3h"

uci:foreach("ipsec", "tunnel", function(s)
	if s['.name'] then
		tunnel_table[s[".name"]] = s['.name']
	end
end)

o = s:option(DynamicList, "tunnel", translate("Tunnel"))
o:value("", translate("None"))
for k, v in pairs(tunnel_table) do
	o:value(k, v)
end

return m
