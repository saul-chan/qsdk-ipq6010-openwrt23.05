-- Copyright 2019 Xingwang Liao <kuoruan@gmail.com>
-- Licensed to the public under the MIT License.

local http = require "luci.http"
local uci = require "luci.model.uci".cursor()
local sys = require "luci.sys"

module("luci.controller.ipsec", package.seeall)

function index()
	-- if not nixio.fs.access("/etc/config/ipsec") then
		-- return
	-- end

	-- entry({"admin", "vpn", "ipsec"},
		-- firstchild(), _("IPSec VPN"),3).dependent = false

	-- entry({"admin", "vpn", "ipsec", "common"},
		-- cbi("ipsec/common"), _("Common"), 5)

	-- entry({"admin", "vpn", "ipsec", "remotes"},
		-- arcombine(cbi("ipsec/remotes"), cbi("ipsec/remote-detail")),
		-- _("Remotes"), 1).leaf = true
	
	-- entry({"admin", "vpn", "ipsec", "tunnels"},
		-- arcombine(cbi("ipsec/tunnels"), cbi("ipsec/tunnel-detail")),
		-- _("Tunnels"), 2).leaf = true

	-- entry({"admin", "vpn", "ipsec", "cryptographic"},
		-- arcombine(cbi("ipsec/cryptographics"), cbi("ipsec/cryptographic-detail")),
		-- _("Cryptographic Proposal"), 3).leaf = true

	-- entry({"admin", "vpn", "ipsec", "rules"},
		-- arcombine(cbi("ipsec/rules"), cbi("ipsec/rule-details")),
		-- _("Policies"), 4).leaf = true
	
	entry({"admin", "status", "ipsec"}, template("ipsec"), _("IPSec Status"), 94)
end


-- function action_status()
	-- local running = false

	-- local client = uci:get("frpc", "main", "client_file")
	-- if client and client ~= "" then
		-- local file_name = client:match(".*/([^/]+)$") or ""
		-- if file_name ~= "" then
			-- running = sys.call("pidof %s >/dev/null" % file_name) == 0
		-- end
	-- end

	-- http.prepare_content("application/json")
	-- http.write_json({
		-- running = running
	-- })
-- end
