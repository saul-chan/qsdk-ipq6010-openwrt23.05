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
	
	// 添加RPC调用来执行命令
	callSystemExec: rpc.declare({
		object: 'luci',
		method: 'exec',
		params: [ 'command' ],
		expect: { '': '' }
	}),
	
	load: function() {
		return Promise.all([
			uci.load('ipsec')
		]);
	},

	render: function(data) {
		return this.renderCommon(data);
	},

	renderCommon: function(data) {
		var m, s, o;
			
		m = new form.Map('ipsec', _('IPSec') + ' - ' + _('Common Configuration'));
		
		s = m.section(form.TypedSection, 'ipsec');
		s.anonymous = true;
		s.addremove = false;

		o = s.option(form.Flag, 'debug', _('Debug'), _('More debug logs are displayed.'));
		
		o = s.option(form.Flag, 'allow_aggressive', _('Allow Aggressive'), _('Whether remote aggressive negotiation is allowed.'));
		
		o = s.option(form.Flag, 'rtinstall_enabled', _('Install Routes'), _('Installs routes into routing table 220 by default.'));
		
		o = s.option(widgets.NetworkSelect, 'interface', 
            _('Interfaces'),
            _('Network interfaces to use for IPSec tunnel'));
		
		o = s.option(form.DynamicList, 'ignore_routing_tables', _('Ignore Routing Tables'));
		o.datatype = 'uinteger';
		o.value('220', _('Routing table %d').format('220'));
		
		return m.render();
	}
});
