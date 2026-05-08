-- Copyright 2008 Steven Barth <steven@midlink.org>
-- Copyright 2010-2012 Jo-Philipp Wich <jow@openwrt.org>
-- Licensed to the public under the Apache License 2.0.

local sys = require "luci.sys"
local utl = require "luci.util"
local dsp = require "luci.dispatcher"
local nxo = require "nixio"

local ft = require "luci.tools.firewall"
local fw = require "luci.model.firewall".init()
local nw = require "luci.model.network"
local m, s, o, k, v

m = Map("ipsec_rule",
	translate("IPsec - Policies"))

m.redirect = dsp.build_url("admin/vpn/ipsec/rules")

nw.init(m.uci)


	local name = m:get(arg[1], "name") or m:get(arg[1], "_name")
	if not name or #name == 0 then
		name = translate("(Unnamed Rule)")
	end

	m.title = "%s - %s : %s" %{ translate("<br>IPSec"), translate("Edit Policy"), name }


	s = m:section(NamedSection, arg[1], "rule", "")
	s.anonymous = true
	s.addremove = false

	ft.opt_name(s, Value, translate("Name"))

	o = s:option(ListValue, "family", translate("Restrict to address family"))
	o.rmempty = true
	o:value("", translate("IPv4 and IPv6"))
	o:value("ipv4", translate("IPv4 only"))

	o = s:option(ListValue, "proto", translate("Protocol"))
	o:value("all", translate("Any"))
	o.default = "all"

	o = s:option(ListValue, "src", translate("Source zone"))
	o.default = "lan"
	for k, v in ipairs(fw:get_zones()) do
		o:value(v:name(),v:name())
	end

	o = s:option(Value, "src_ip", translate("Source address"))
	o.datatype = "neg(ipaddr)"
	o.placeholder = translate("any")

	luci.sys.net.ipv4_hints(function(ip, name)
		o:value(ip, "%s (%s)" %{ ip, name })
	end)

	o = s:option(ListValue, "dest", translate("Destination zone"))
	o:value("*", translate("Any zone"))
	o.default = "wan"
	for k, v in ipairs(fw:get_zones()) do
		o:value(v:name(),v:name())
	end

	o = s:option(Value, "dest_ip", translate("Destination address"))
	o.datatype = "neg(ipaddr)"
	o.placeholder = translate("any")

	luci.sys.net.ipv4_hints(function(ip, name)
		o:value(ip, "%s (%s)" %{ ip, name })
	end)

	o = s:option(ListValue, "target", translate("Action"))
	o.default = "ACCEPT"
	o:value("DROP", translate("drop"))
	o:value("ACCEPT", translate("accept"))
	o:value("REJECT", translate("reject"))
	o:value("NOTRACK", translate("don't track"))

return m
