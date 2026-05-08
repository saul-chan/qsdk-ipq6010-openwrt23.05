-- Author 2022 Michael
local   uci = require("luci.model.uci").cursor()
local   datatypes = require("luci.cbi.datatypes")
local   util = require("luci.util")
local   sys = require("luci.sys")
local   m, s

m = Map("easycwmp", translate("CWMP"))
s = m:section(TypedSection, "local")
s:tab("general", translate("General Settings"))
s:tab("physical", translate("Physical Settings"))
s.anonymous = true
s.addremove = false

o = s:taboption("general", Flag, "enable", translate("Enable"))
o.default = o.disabled

o = s:taboption("physical", Value, "interface", translate("Interface"))
o.template = "cbi/network_ifacelist"
o.widget = "radio"

o = s:taboption("general", Value, "port", translate("Port"))
o.default = 7547
o.datatype = "uinteger"

o = s:taboption("general", Value, "username", translate("Username"))
o.placeholder = "Enter user name"
o.datatype = "string"

o = s:taboption("general", Value, "password", translate("Password"))
o.placeholder = "Enter password "
o.password = true
o.datatype = "string"

o = s:taboption("general", ListValue, "authentication", translate("Authentication"))
o:value("Basic", translate("Basic"))
o:value("Digest", translate("Digest"))
o.default = "Digest"

-- o = s:taboption("general", Value, "provisioning_code", translate("Provisioning code"))
-- o.datatype = "string"

-- o = s:taboption("general", Value, "serial_number", translate("Serialnumber"))
-- o.datatype = "string"

o = s:taboption("general", ListValue, "logging_level", translate("Logging level"))
o:value("0", translate("Critical"))
o:value("1", translate("Warning"))
o:value("2", translate("Notice"))
o:value("3", translate("Info"))
o:value("4", translate("Debug"))
o.default="3"

------------------------------------------------------------------------------------
s = m:section(TypedSection, "local", "STUN " .. translate("Settings"))
s.anonymous = true
s.addremove = false

o = s:option(Flag, "stun_enable", "STUN "..translate("Enable"))
o.default = o.disabled

o = s:option(Value, "stun_address", "STUN "..translate("Server")..translate("Address"))
--o.datatype = "ip4addr"

o = s:option(Value, "stun_port", "STUN "..translate("Server")..translate("Port"))
o.datatype = "uinteger"
o.default = 3478 

o = s:option(Value, "stun_username", "STUN "..translate("Server")..translate("Username"))
o.datatype = "string"

o = s:option(Value, "stun_password", "STUN "..translate("Server")..translate("Password"))
o.password = true
o.datatype = "string"

o = s:option(Value, "stun_maxkeepaliveperiod", "STUN "..translate("Server")..translate("Max Keepalive Period"))
o.datatype = "uinteger"
o.default = 60 

o = s:option(Value, "stun_minkeepaliveperiod", "STUN "..translate("Server")..translate("Min Keepalive Period"))
o.datatype = "uinteger"
o.default = 10 

o = s:option(Value, "stun_keepaliveperiod", "STUN "..translate("Server")..translate("Keepalive Period"))
o.datatype = "uinteger"
o.default = 20 
------------------------------------------------------------------------------------

s = m:section(TypedSection, "acs", "ACS " .. translate("Settings"))
s.anonymous = true                                           
s.addremove = false
o = s:option(Value, "url", "ACS " .. translate("URL"))


o = s:option(Value, "username", translate("Username"))
o.placeholder = "Enter user name"
o.datatype = "string"
o.default = "cwmpguest"

o = s:option(Value, "password", translate("Password"))
o.placeholder = "Enter password "
o.default = "cwmp"
o.password = true
o.datatype = "string"

-- o = s:option(Value, "parameter_key", translate("Parameter key"))
-- o.datatype = "string"

-- o = s:option(Flag, "periodic_enable", translate("Periodic enable"))
-- o.default = true

-- o = s:option(Value, "periodic_interval", translate("Periodic interval"))
-- o.default = 100
-- o.datatype = "uinteger"

local apply = luci.http.formvalue("cbi.apply")
if apply then
    io.popen("/etc/init.d/easycwmpd stop")
    io.popen("/etc/init.d/easycwmpd boot")
end

return m

