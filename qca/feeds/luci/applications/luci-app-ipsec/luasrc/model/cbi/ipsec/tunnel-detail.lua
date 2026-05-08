-- Copyright 2019 Xingwang Liao <kuoruan@gmail.com>
-- Licensed to the public under the MIT License.

local dsp = require "luci.dispatcher"
local uci = require "luci.model.uci".cursor()

local m, s, o
local proposal_table = { }

local sid = arg[1]

m = Map("ipsec", "%s - %s : %s" % { translate("<br>IPSec"), translate("Edit IPSec Tunnel"), sid })
m.redirect = dsp.build_url("admin/vpn/ipsec/tunnels")

if m.uci:get("ipsec", sid) ~= "tunnel" then
	luci.http.redirect(m.redirect)
	return
end

s = m:section(NamedSection, sid, "tunnel")
s.anonymous = true
s.addremove = false

o = s:option(ListValue, "mode", translate("Mode"))
o:value("add", translate("Loads a connection without starting it"))
o:value("start",translate("Loads a connection & brings it up immediately"))
o:value("route",translate("Loads a connection when traffic is detected"))
o:value("ignore",translate("Ignores the connection"))
o.default = "start"

o = s:option(Value, "local_subnet", translate("Local Subnet"), translate("This terminal subnet, Format: 192.168.2.0/24."))

o = s:option(Value, "remote_subnet", translate("Remote Subnet"), translate("A remote subnet, Format: 192.168.3.0/24."))

o = s:option(ListValue, "keyexchange", translate("Key Exchange"), translate("Method of key exchange; which protocol should be used to initialize the connection."))
o:value("ikev1",translate("ikev1"))
o:value("ikev2",translate("ikev2"))
o:value("ike",translate("ike"))

o = s:option(Value, "keyingtries", translate("Keying Tries"), translate("How many attempts (a positive integer or %forever) should be made to negotiate a connection, or a replacement."))
o.placeholder="3"

uci:foreach("ipsec", "crypto_proposal", function(s)
	if s['.name'] then
		proposal_table[s[".name"]] = s['.name']
	end
end)

o = s:option(DynamicList, "crypto_proposal", translate("ESP Cryptographic Proposal"), translate("Choose the cryptographic proposal to be used in negotiations."))
o:value("", translate("None"))
for k, v in pairs(proposal_table) do
	o:value(k, v)
end

o = s:option(Value, "lifetime", translate("ESP Lifetime"), translate("Maximum lifetime before CHILD_SA gets closed.(s/m/h)"))
o.placeholder="3h"

o = s:option(ListValue, "dpdaction", translate("DPD Action"), translate("Action to perform for this CHILD_SA on DPD timeout."))
o:value("none",translate("none"))
o:value("clear",translate("clear"))
o:value("hold",translate("hold"))
o:value("restart",translate("restart"))
o.default = "none"

o = s:option(Value, "dpddelay", translate("DPD Delay"), translate("Interval to check the liveness of a peer actively.(s)"))
o.default = "30s"

return m
