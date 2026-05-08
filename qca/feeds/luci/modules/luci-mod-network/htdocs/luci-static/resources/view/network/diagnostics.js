'use strict';
'require view';
'require dom';
'require fs';
'require ui';
'require uci';
'require network';

return view.extend({
	handleCommand: function(exec, args) {
		var buttons = document.querySelectorAll('.diag-action > .cbi-button');

		for (var i = 0; i < buttons.length; i++)
			buttons[i].setAttribute('disabled', 'true');

		return fs.exec(exec, args).then(function(res) {
			var out = document.querySelector('textarea');

			dom.content(out, [ res.stdout || '', res.stderr || '' ]);
		}).catch(function(err) {
			ui.addNotification(null, E('p', [ err ]))
		}).finally(function() {
			for (var i = 0; i < buttons.length; i++)
				buttons[i].removeAttribute('disabled');
		});
	},

	handlePing: function(ev, cmd) {
		var exec = cmd || 'ping',
		    inputEl = document.getElementById('ping-input'),
		    addr = inputEl ? inputEl.value : '',
		    args = (exec == 'ping') ? [ '-4', '-c', '5', '-W', '1', addr ] : [ '-6', '-c', '5', addr ];

		return this.handleCommand(exec, args);
	},

	handleTraceroute: function(ev, cmd) {
		var exec = cmd || 'traceroute',
		    inputEl = document.getElementById('route-input'),
		    addr = inputEl ? inputEl.value : '',
		    args = (exec == 'traceroute') ? [ '-4', '-q', '1', '-w', '1', '-n', '-m', String(L.env.rpctimeout || 20), addr ] : [ '-q', '1', '-w', '2', '-n', addr ];

		return this.handleCommand(exec, args);
	},

	handleNslookup: function(ev, cmd) {
		var inputEl = document.getElementById('dns-input'),
		    addr = inputEl ? inputEl.value : '';

		return this.handleCommand('nslookup', [ addr ]);
	},

	handleArpScan: function(ev, cmd) {
		var addr = ev.currentTarget.parentNode.previousSibling.value;

		return this.handleCommand('arp-scan', [ '-l', '-I', addr ]);
	},

	load: function() {
		return Promise.all([
			L.resolveDefault(fs.stat('/bin/ping6'), {}),
			L.resolveDefault(fs.stat('/usr/bin/ping6'), {}),
			L.resolveDefault(fs.stat('/bin/traceroute6'), {}),
			L.resolveDefault(fs.stat('/usr/bin/traceroute6'), {}),
			L.resolveDefault(fs.stat('/usr/bin/arp-scan'), {}),
			network.getDevices(),
			uci.load('luci')
		]);
	},

	render: function(res) {
		var has_ping6 = res[0].path || res[1].path,
		    has_traceroute6 = res[2].path || res[3].path,
		    has_arpscan = res[4].path,
		    devices = res[5],
			dns_host = uci.get('luci', 'diag', 'dns') || 'openwrt.org',
			ping_host = uci.get('luci', 'diag', 'ping') || 'openwrt.org',
			route_host = uci.get('luci', 'diag', 'route') || 'openwrt.org',
			mediaurlbase = uci.get('luci', 'main', 'mediaurlbase') || '/luci-static/slk',
			lang = uci.get('luci', 'main', 'lang') || 'en';
		
		// 获取预设域名列表（包含显示名称和实际值）
		var getPresetDomains = function() {
			var domains = [];
			
			if (mediaurlbase.includes('slk')) {
				// SLK 主题
				domains.push({ label: 'Seriallink', value: 'www.seriallink.cn' });
				
				if (lang === 'zh-cn' || lang === 'zh-CN' || lang.indexOf('zh') === 0) {
					domains.push({ label: 'Baidu', value: 'www.baidu.com' });
				} else {
					domains.push({ label: 'Google', value: 'www.google.cn' });
				}
				domains.push({ label: '8.8.8.8', value: '8.8.8.8' });
			} else {
				// 其他主题
				if (lang === 'zh-cn' || lang === 'zh-CN' || lang.indexOf('zh') === 0) {
					domains.push({ label: 'Baidu', value: 'www.baidu.com' });
				} else {
					domains.push({ label: 'Google', value: 'www.google.cn' });
				}
				domains.push({ label: '8.8.8.8', value: '8.8.8.8' });
			}
			
			return domains;
		};
		
		var presetDomains = getPresetDomains();
		
		// 创建支持预设选项的输入框
		var createValueWithPresets = function(id, initialValue, presets) {
			var container = E('div', { 'style': 'display:flex; gap:5px; align-items:center' });
			
			// 创建文本输入框
			var input = E('input', {
				'id': id,
				'type': 'text',
				'class': 'cbi-input-text',
				'style': 'flex:2; max-width: 150px; min-width: 130px;',
				'value': initialValue,
				'placeholder': _('Enter hostname or IP address...')
			});
			
			// 创建预设选项下拉框（显示友好名称）
			var select = E('select', {
				'class': 'cbi-input-select',
				'style': 'flex:1; max-width: 110px; min-width: 90px;'
			});
			
			// 添加预设选项（显示友好名称，存储实际值）
			presets.forEach(function(preset) {
				var option = E('option', { 'value': preset.value }, [ preset.label ]);
				select.appendChild(option);
			});
			
			// 设置当前输入框的值对应的选中项
			var matchingPreset = presets.find(function(p) { return p.value === initialValue; });
			if (matchingPreset) {
				select.value = matchingPreset.value;
			}
			
			// 监听下拉框变化
			select.addEventListener('change', function(ev) {
				if (this.value) {
					input.value = this.value;
					ev.preventDefault();
				}
			});
			
			// 监听输入框变化，同步下拉框的选中状态
			input.addEventListener('input', function() {
				var matching = presets.find(function(p) { return p.value === this.value; }.bind(this));
				if (matching) {
					select.value = matching.value;
				} else {
					select.selectedIndex = -1;
				}
			}.bind(this));
			
			container.appendChild(input);
			container.appendChild(select);
			
			return container;
		};
		
		var table = E('table', { 'class': 'table' }, [
				E('tr', { 'class': 'tr', 'style': 'background-color: #fff !important;' }, [
					E('td', { 'class': 'td left', 'style': 'overflow:initial; display: inline-flex; flex-direction: row; border-bottom: 0px !important;' }, [
						createValueWithPresets('ping-input', ping_host, presetDomains),
						E('span', { 'class': 'diag-action' }, [
							has_ping6 ? new ui.ComboButton('ping', {
								'ping': '%s %s'.format(_('IPv4'), _('Ping')),
								'ping6': '%s %s'.format(_('IPv6'), _('Ping')),
							}, {
								'click': ui.createHandlerFn(this, 'handlePing'),
								'classes': {
									'ping': 'btn cbi-button cbi-button-action',
									'ping6': 'btn cbi-button cbi-button-action'
								}
							}).render() : E('button', {
								'class': 'cbi-button cbi-button-action',
								'click': ui.createHandlerFn(this, 'handlePing')
							}, [ _('Ping') ])
						])
					]),

					E('td', { 'class': 'td left', 'style': 'overflow:initial; display: inline-flex; flex-direction: row; border-bottom: 0px !important;' }, [
						createValueWithPresets('route-input', route_host, presetDomains),
						E('span', { 'class': 'diag-action' }, [
							has_traceroute6 ? new ui.ComboButton('traceroute', {
								'traceroute': '%s %s'.format(_('IPv4'), _('Traceroute')),
								'traceroute6': '%s %s'.format(_('IPv6'), _('Traceroute')),
							}, {
								'click': ui.createHandlerFn(this, 'handleTraceroute'),
								'classes': {
									'traceroute': 'btn cbi-button cbi-button-action',
									'traceroute6': 'btn cbi-button cbi-button-action'
								}
							}).render() : E('button', {
								'class': 'cbi-button cbi-button-action',
								'click': ui.createHandlerFn(this, 'handleTraceroute')
							}, [ _('Traceroute') ])
						])
					]),

					E('td', { 'class': 'td left', 'style': 'overflow:initial; display: inline-flex; flex-direction: row; border-bottom: 0px !important;' }, [
						createValueWithPresets('dns-input', dns_host, presetDomains),
						E('span', { 'class': 'diag-action' }, [
							E('button', {
								'class': 'cbi-button cbi-button-action',
								'click': ui.createHandlerFn(this, 'handleNslookup')
							}, [ _('Nslookup') ])
						])
					]),

					has_arpscan ? E('td', { 'class': 'td left' }, [
						E('select', {
							'style': 'margin:5px 0',
							'class': 'cbi-input-select'
						}, devices.map(function(device) {
							if (!device.isUp())
								return E([]);

							return E('option', { 'value': device.getName() }, [ device.getI18n() ]);
						})),
						E('span', { 'class': 'diag-action' }, [
							E('button', {
								'class': 'cbi-button cbi-button-action',
								'click': ui.createHandlerFn(this, 'handleArpScan')
							}, [ _('Arp-scan') ])
						])
					]) : E([]),
				])
			]);

		var view = E('div', { 'class': 'cbi-map'}, [
			E('h2', {}, [ _('Diagnostics') ]),
			E('div', { 'class': 'cbi-map-descr'}, _('Execution of various network commands to check the connection and name resolution to other systems.')),
			table,
			E('div', {'class': 'cbi-section'}, [
				E('div', { 'id' : 'command-output'},
					E('textarea', {
						'id': 'widget.command-output',
						'style': 'width: 100%; font-family:monospace; white-space:pre',
						'readonly': true,
						'wrap': 'off',
						'rows': '20'
					})
				)
			])
		]);

		return view;
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});