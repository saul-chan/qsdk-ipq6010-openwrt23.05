-- Copyright 2022 Michael Liu
-- Licensed to the public under the Apache License 2.0.

module("luci.controller.admin.easycwmp", package.seeall)

function index()
	if (nixio.fs.access("/etc/config/easycwmp"))then
        	entry({"admin", "services", "easycwmp"}, cbi("admin_system/easycwmp"), _("CWMP"), 10)
	end
end
