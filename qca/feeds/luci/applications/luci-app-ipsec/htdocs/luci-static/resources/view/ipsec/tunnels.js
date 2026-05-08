'use strict';
'require view';
'require ui';
'require rpc';
'require uci';
'require form';
'require tools.widgets as widgets';

return view.extend({

	load: function() {
		return Promise.all([
			uci.load('ipsec')
		]);
	},
	
	render: function(data) {
		return this.renderTunnels(data);
	},

	renderTunnels: function(data) {
		var m, s, o;

		m = new form.Map('ipsec', _('IPSec') + ' - ' + _('Tunnels'));

		s = m.section(form.GridSection, 'tunnel');
		s.addremove = true;
		s.anonymous = false;
		s.sortable  = true;
		
		s.sectiontitle = function(section_id) {
			return uci.get('ipsec', section_id, '.name') || _('Unnamed tunnel');
		};
		
		/* s.handleAdd = function(ev) {
			var config_name = this.uciconfig || this.map.config,
			    section_id = uci.add(config_name, this.sectiontype); // 修复：分号而不是逗号

			this.addedSection = section_id;
			this.renderMoreOptionsModal(section_id);
		}; */
	
		o = s.option(form.DummyValue, 'local_subnet', _('Local Subnet'));
		o.modalonly = false;
		
		o = s.option(form.DummyValue, 'remote_subnet', _('Remote Subnet'));
		o.modalonly = false;
		
		o = s.option(form.DummyValue, 'keyexchange', _('Key Exchange'));
		o.modalonly = false;
		
		o = s.option(form.ListValue, 'mode', _('Mode'));
		o.value('add', _('Loads a connection without starting it'));
		o.value('start',_('Loads a connection & brings it up immediately'));
		o.value('route',_('Loads a connection when traffic is detected'));
		o.value('ignore',_('Ignores the connection'));
		o.default = "start"
		o.modalonly = true;
		
		o = s.option(form.Value, 'local_subnet', _('Local Subnet'), _('This terminal subnet, Format: 192.168.2.0/24.'));
		o.placeholder = _('192.168.2.0/24');
		o.modalonly = true;
		
		o = s.option(form.Value, 'remote_subnet', _('Remote Subnet'), _('A remote subnet, Format: 192.168.3.0/24.'));
		o.placeholder = _('192.168.3.0/24');
		o.modalonly = true;
		
		o = s.option(form.ListValue, 'aggressive_mode', _('Exchange Mode'), _('Whether to use IKEv1 Aggressive or Main Mode (the default)'));
		o.value("yes", _("Aggressive Mode"));
		o.value("", _("Main Mode"));
		o.modalonly = true;
		
		o = s.option(form.ListValue, 'keyexchange', _('Key Exchange'), _('Method of key exchange; which protocol should be used to initialize the connection.'));
		o.value("ikev1",_("ikev1"))
		o.value("ikev2",_("ikev2"))
		o.value("ike",_("ike"))
		o.modalonly = true;
		
		o = s.option(form.Value, 'keyingtries', _('Keying Tries'), _('How many attempts (a positive integer or %forever) should be made to negotiate a connection, or a replacement.')); // 修复：描述可能需要调整
		o.placeholder = '3';
		o.modalonly = true;
		
		o = s.option(form.ListValue, 'crypto_proposal', _('ESP Cryptographic Proposal'), _('Choose the cryptographic proposal to be used in negotiations.'));
		o.value('', _('None'));
		uci.sections('ipsec', 'crypto_proposal').forEach(function(section) {
			if (section['.name']) {
				o.value(section['.name'], section['.name']);
			}
		});
		o.modalonly = true;
		
		o = s.option(form.Value, 'lifetime', _('ESP Lifetime'), _('Time to schedule IPSec SA rekeying.(s/m/h)'));
		o.placeholder = '3h';
		o.modalonly = true;
		
		o = s.option(form.ListValue, 'dpdaction', _('DPD Action'), _('Action to perform for this CHILD_SA on DPD timeout.'));
		o.value('none',_('none'))
		o.value('clear',_('clear'))
		o.value('hold',_('hold'))
		o.value('restart',_('restart'))
		o.default = "none"
		o.modalonly = true;
		
		o = s.option(form.Value, 'dpddelay', _('DPD Delay'), _('Interval to check the liveness of a peer actively.(s)'));
		o.placeholder = "30s";
		o.modalonly = true;
		
		return m.render();
	}
});