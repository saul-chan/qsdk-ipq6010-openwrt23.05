'use strict';
'require view';
'require rpc';
'require uci';
'require form';
'require fs';
'require network';
'require firewall as fwmodel';
'require tools.firewall as fwtool';
'require tools.widgets as widgets';

return view.extend({
	callHostHints: rpc.declare({
		object: 'luci-rpc',
		method: 'getHostHints',
		expect: { '': {} }
	}),

	callConntrackHelpers: rpc.declare({
		object: 'luci',
		method: 'getConntrackHelpers',
		expect: { result: [] }
	}),
	
	// 添加RPC调用来执行命令
	callSystemExec: rpc.declare({
		object: 'luci',
		method: 'exec',
		params: [ 'command' ],
		expect: { '': '' }
	}),
	
	load: function() {
		return Promise.all([
			this.callHostHints(),
			this.callConntrackHelpers(),
			uci.load('firewallon')
		]);
	},

	render: function(data) {
		if (fwtool.checkLegacySNAT())
			return fwtool.renderMigration();
		else
			return this.renderRules(data);
	},

	renderRules: function(data) {
		var hosts = data[0],
		    ctHelpers = data[1],
		    m, s, o;
			
		m = new form.Map('firewallon', _('Common Configuration'));
		
		s = m.section(form.TypedSection, 'defaults');
		s.anonymous = true;
		s.addremove = false;

		o = s.option(form.ListValue, 'forward', _('Firewall'));
		o.value('REJECT', _('Enable'));
		o.value('ACCEPT', _('Disable'));
		o.write = function(section_id, value) {
			uci.set('firewallon', section_id, 'forward', value);
			uci.set('firewallon', '@zone[1]', 'forward', value);
			uci.set('firewallon', '@zone[1]', 'input', value);
			/* return uci.save().then(function() {
				// 直接返回 fs.exec 的 Promise
				return fs.exec('/etc/init.d/firewall_init', ['restart']);
			}.bind(this)); */
		};
	
		o = s.option(form.DummyValue, '_separate');
        o.titleref = L.url("admin", "network", "firewall")
        o.rawhtml = true;
        o.title = '<b>' + _("Show more") + '</b>';
        o.cfgvalue = function() {
			return '<div style="line-height: normal">' + _("Follow this link<br />You will find more firewall configuration") + '<div>'
        };
		
		return m.render();
	}
});
