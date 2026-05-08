'use strict';
'require view';
'require form';

return view.extend({
	render: function() {
		var m, s, o;

		m = new form.Map('uhttpd', _('HTTP(S) Access'), _('uHTTPd offers <abbr title="Hypertext Transfer Protocol">HTTP</abbr> or <abbr title="Hypertext Transfer Protocol Secure">HTTPS</abbr> network access.'));

		s = m.section(form.NamedSection, 'main', 'uhttpd', _('Settings'));
		s.addremove = false;

		o = s.option(form.Flag, 'redirect_https', _('Redirect to HTTPS'), _('Enable automatic redirection of <abbr title="Hypertext Transfer Protocol">HTTP</abbr> requests to <abbr title="Hypertext Transfer Protocol Secure">HTTPS</abbr> port.'));
		o.rmempty = false;
		
		o = s.option(form.Flag, 'rfc1918_filter', _('RTC1918 Filter'), _('Enable RTC1918 Filter.'));
		o.rmempty = false;

		return m.render();
	}
});
