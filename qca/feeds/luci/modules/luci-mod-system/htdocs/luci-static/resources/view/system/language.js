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
			uci.load('luci'),
			uci.load('system')
		]);
	},

	render: function(rpc_replies) {
		var m, s, o;

		m = new form.Map('system',
			_('Language'));

		m.chain('luci');

		s = m.section(form.TypedSection, 'system');
		s.anonymous = true;
		s.addremove = false;

		/*
		 * Language & Style
		 */

		o = s.option(form.ListValue, '_lang', _('Language'))
		o.uciconfig = 'luci';
		o.ucisection = 'main';
		o.ucioption = 'lang';
		o.value('auto');

		var l = Object.assign({ en: 'English' }, uci.get('luci', 'languages')),
		    k = Object.keys(l).sort();
		for (var i = 0; i < k.length; i++)
			if (k[i].charAt(0) != '.')
				o.value(k[i], l[k[i]]);

		// o = s.option(form.ListValue, '_mediaurlbase', _('Design'))
		// o.uciconfig = 'luci';
		// o.ucisection = 'main';
		// o.ucioption = 'mediaurlbase';

		// var k = Object.keys(uci.get('luci', 'themes') || {}).sort();
		// for (var i = 0; i < k.length; i++)
			// if (k[i].charAt(0) != '.')
				// o.value(uci.get('luci', 'themes', k[i]), k[i]);

		return m.render();
	}
});
