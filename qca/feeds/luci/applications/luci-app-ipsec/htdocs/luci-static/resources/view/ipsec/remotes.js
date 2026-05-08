'use strict';
'require view';
'require ui';
'require rpc';
'require uci';
'require form';
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

	load: function() {
		return Promise.all([
			uci.load('ipsec')
		]);
	},
	
	render: function(data) {
		return this.renderRemotes(data);
	},

	renderRemotes: function(data) {
		var m, s, o;

		m = new form.Map('ipsec', _('IPSec') + ' - ' + _('Remote Configuration'));

		s = m.section(form.GridSection, 'remote');
		s.addremove = true;
		s.anonymous = false;
		s.sortable  = true;
		
		s.sectiontitle = function(section_id) {
			return uci.get('ipsec', section_id, '.name') || _('Unnamed remote');
		};
		
		// s.handleAdd = function(ev) {
			// var config_name = this.uciconfig || this.map.config,
			    // section_id = uci.add(config_name, this.sectiontype); // 修复：分号而不是逗号

			// this.addedSection = section_id;
			// this.renderMoreOptionsModal(section_id);
		// };
		
		o = s.option(form.Flag, 'enabled', _('Enable'));
		o.modalonly = false;
		o.default = o.enabled;
		o.editable = true;
	
		o = s.option(form.DummyValue, 'gateway', _('Gateway'));
		o.modalonly = false;
		
		o = s.option(form.DummyValue, 'pre_shared_key', _('PSK'));
		o.modalonly = false;
		
		o = s.option(form.DummyValue, 'local_identifier', _('Local ID'));
		o.modalonly = false;
		
		o = s.option(form.DummyValue, 'remote_identifier', _('Remote ID')); // 修复：应该是Remote ID
		o.modalonly = false;
		
		o = s.option(form.DummyValue, 'authentication_method', _('Authentication Method'));
		o.modalonly = false;
		
		o = s.option(form.Value, 'gateway', _('Gateway'), _('IP address or domain name of the remote server. If the value is %any, it indicates that the remote connection is not initiated.'));
		o.placeholder = _('Remote ipsec server');
		o.modalonly = true;
		
		o = s.option(form.Value, 'pre_shared_key', _('PSK'), _('Pre shared key.'));
		o.placeholder = _('Your psk password');
		o.modalonly = true;
		
		o = s.option(form.Value, 'local_identifier', _('Local ID'), _('Specify your local ID.'));
		o.placeholder = _('Your local id');
		o.modalonly = true;
		
		o = s.option(form.Value, 'remote_identifier', _('Remote ID'), _('Specify your local ID.')); // 修复：描述可能需要调整
		o.placeholder = _('Remote ID');
		o.modalonly = true;
		
		o = s.option(form.ListValue, 'authentication_method', _('Authentication Method'));
		o.value("psk", "PSK"); // 修复：使用点而不是冒号，并添加显示文本
		o.modalonly = true;
		
		o = s.option(form.ListValue, 'crypto_proposal', _('IKE Cryptographic Proposal'), _('Choose the cryptographic proposal to be used in negotiations.'));
		o.value('', _('None'));
		uci.sections('ipsec', 'crypto_proposal').forEach(function(section) {
			if (section['.name']) {
				o.value(section['.name'], section['.name']);
			}
		});
		o.modalonly = true;
		
		o = s.option(form.Value, 'ikelifetime', _('IKE Lifetime'), _('Time to schedule IKE SA rekeying.(s/m/h)'));
		o.placeholder = "3h";
		o.modalonly = true;
		
		// 隧道选项
		o = s.option(form.DynamicList, 'tunnel', _('Tunnel'));
		o.value('', _('None'));
		uci.sections('ipsec', 'tunnel').forEach(function(section) {
			if (section['.name']) {
				o.value(section['.name'], section['.name']);
			}
		});
		o.modalonly = true;
		/* o.write = function(section_id, value) {
			// 首先移除现有的 tunnel list 条目（如果存在）
			var current_tunnels = uci.get('ipsec', section_id, 'tunnel');
			if (Array.isArray(current_tunnels)) {
				// 如果是数组，清空所有现有的 tunnel 值
				uci.delete('ipsec', section_id, 'tunnel');
			} else if (current_tunnels) {
				// 如果是单个值，也删除
				uci.delete('ipsec', section_id, 'tunnel');
			}
			
			// 如果值不为空，添加为 list
			if (value && value !== '') {
				uci.add_list('ipsec', section_id, 'tunnel', value);
			}
		}; */
		
		return m.render();
	}
});