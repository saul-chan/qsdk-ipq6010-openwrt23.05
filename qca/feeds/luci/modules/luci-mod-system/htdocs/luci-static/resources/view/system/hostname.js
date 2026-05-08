'use strict';
'require view';
'require poll';
'require ui';
'require uci';
'require rpc';
'require form';
'require tools.widgets as widgets';

return view.extend({
	load: function() {
		return Promise.all([
			uci.load('system')
		]);
	},

	render: function(rpc_replies) {
		var m, s, o;

		m = new form.Map('system',
			_('Hostname'));

		m.chain('luci');

		s = m.section(form.TypedSection, 'system');
		s.anonymous = true;
		s.addremove = false;

		o = s.option(form.Value, 'hostname', _('Hostname'));
		o.datatype = 'hostname';

		return m.render();
	}
});
