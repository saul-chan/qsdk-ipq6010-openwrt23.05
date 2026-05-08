'use strict';
'require form';
'require poll';
'require uci';
'require view';
'require network';
'require dom';
'require firewall';
'require ui';
'require rpc';
'require fs';

var callSystemBoard = rpc.declare({
	object: 'system',
	method: 'board'
});

function count_changes(ifname) {
    return 0;
}

function write_keepalive(section_id, value) {
	var f_opt = this.map.lookupOption('_keepalive_failure', section_id),
	    i_opt = this.map.lookupOption('_keepalive_interval', section_id),
	    f = (f_opt != null) ? +f_opt[0].formvalue(section_id) : null,
	    i = (i_opt != null) ? +i_opt[0].formvalue(section_id) : null;

	if (f == null || f == '' || isNaN(f))
		f = 0;

	if (i == null || i == '' || isNaN(i) || i < 1)
		i = 1;

	if (f > 0)
		uci.set('network', section_id, 'keepalive', '%d %d'.format(f, i));
	else
		uci.unset('network', section_id, 'keepalive');
}

function render_status(node, ifc, with_device) {
    var desc = null, c = [];

    if (ifc.isDynamic())
        desc = _('Virtual dynamic interface');
    else if (ifc.isAlias())
        desc = _('Alias Interface');
    else if (!uci.get('network', ifc.getName()))
        return L.itemlist(node, [
            null, E('em', _('Interface is marked for deletion'))
        ]);

    var i18n = ifc.getI18n();
    if (i18n)
        desc = desc ? '%s (%s)'.format(desc, i18n) : i18n;
    var changecount = with_device ? 0 : count_changes(ifc.getName()),
        ipaddrs = changecount ? [] : ifc.getIPAddrs(),
        ip6addrs = changecount ? [] : ifc.getIP6Addrs(),
        errors = ifc.getErrors(),
        maindev = ifc.getL3Device() || ifc.getDevice(),
        macaddr = maindev ? maindev.getMAC() : null;
	
	if ((!ipaddrs || ipaddrs.length === 0) && maindev && maindev.dev && maindev.dev.ipaddrs) {
        ipaddrs = maindev.dev.ipaddrs;
        ip6addrs = maindev.dev.ip6addrs || [];
    }
    return L.itemlist(node, [
        _('Protocol'), with_device ? null : (desc || '?'),
        _('Device'),   with_device ? (maindev ? maindev.getShortName() : E('em', _('Not present'))) : null,
        _('Uptime'),   (!changecount && ifc.isUp()) ? '%t'.format(ifc.getUptime()) : null,
        _('MAC'),      (!changecount && !ifc.isDynamic() && !ifc.isAlias() && macaddr) ? macaddr : null,
        _('RX'),       (!changecount && !ifc.isDynamic() && !ifc.isAlias() && maindev) ? '%.2mB (%d %s)'.format(maindev.getRXBytes(), maindev.getRXPackets(), _('Pkts.')) : null,
        _('TX'),       (!changecount && !ifc.isDynamic() && !ifc.isAlias() && maindev) ? '%.2mB (%d %s)'.format(maindev.getTXBytes(), maindev.getTXPackets(), _('Pkts.')) : null,
        _('IPv4'),     ipaddrs[0],
        _('IPv4'),     ipaddrs[1],
        _('IPv4'),     ipaddrs[2],
        _('IPv4'),     ipaddrs[3],
        _('IPv4'),     ipaddrs[4],
        null, changecount ? E('a', {
            href: '#',
            click: L.bind(ui.changes.displayChanges, ui.changes)
        }, _('Interface has %d pending changes').format(changecount)) : null
    ]);
}

function render_modal_status(node, ifc) {
    var dev = ifc ? (ifc.getDevice() || ifc.getL3Device()) : null;

    dom.content(node, [
        E('img', {
            'src': L.resource('icons/%s%s.png').format(dev ? dev.getType() : 'ethernet', (dev && dev.isUp()) ? '' : '_disabled'),
            'title': dev ? dev.getTypeI18n() : _('Not present'),
			'style':'margin: 0 0.2rem 0 0; vertical-align: middle;'
        }),
        ifc ? render_status(E('span', { 'style': 'vertical-align: middle;' }), ifc, true) : E('em', { 'style': 'vertical-align: middle;' }, _('Interface not present or not connected yet.'))
    ]);

    return node;
}

return view.extend({
    load: function() {
        return Promise.all([
            uci.load('network'),
            network.getNetworks(),
            L.resolveDefault(callSystemBoard(), {})
        ]);
    },

    render: function(data) {
        var networks = data[1];
        var boardinfo = data[2] || {};
        var m, s, o;
        var self = this;
        
        m = new form.Map('openvpn', _('OpenVPN Client'));
        
        s = m.section(form.NamedSection, 'sample_client', 'openvpn');
        s.anonymous = true;
        s.addremove = false;

        s.networks = networks;
        s.section_id = 'sample_client';
		
		s.tab("basic", _("Basic Configuration"));
        s.tab("advanced", _("Advanced Configuration"));

        o = s.taboption('basic', form.DummyValue, '_stat', _('Status'));
        o.modalonly = false;
        o.cfgvalue = L.bind(function(section_id) {
            var net = this.findInterface();
			
            if (!net) {
                return E('div', { 'class': 'ifacebox' }, [
                    E('div', { 
                        'class': 'ifacebox-head center',
                        'style': 'background-color:#EEEEEE'
                    }, E('strong', _('Interface'))),
                    E('div', { 'class': 'ifacebox-body left' }, [
                        E('em', _('Not present'))
                    ])
                ]);
            }
            
            return render_modal_status(E('div', {
                'id': '%s-status'.format(section_id),
                'class': 'ifacebadge large',
				'style': 'border: 1px solid #dee2e6; min-width: 18rem; padding-left: 6px; background: #fff; align-items: center;'
            }), net);
        }, s);
        o.write = function() {};
        
        o = s.taboption('basic', form.Flag, 'enabled', _('Enable'));
        //o.default = o.enabled;
        
        o = s.taboption('basic', form.ListValue, 'dev', _('dev'), _('tun/tap device'));
		o.value("tun", _("tun"));
		o.value("tap", _("tap"));
		o.default='tun';
		
		o = s.taboption('basic', form.ListValue, 'proto', _('proto'), _('Use protocol'));
		o.value("udp", _("udp"));
		o.value("tcp-client", _("tcp"));
		o.default='udp';
		
        o = s.taboption('basic', form.Value, 'remote', _('remote'), _('Remote host name or IP address'));
		o.placeholder='my_server_1 1194'
	    //o.datatype = 'host port';
        
		o = s.taboption('basic', form.FileUpload, 'ca', _('ca'), _('Certificate authority'));
		o.root_directory = '/etc/openvpn';
		o.path = 'ca.crt';  // 指定固定文件名
		o.enable_remove = true;
		o.enable_upload = true;
		o.datatype = 'file';
		
		o = s.taboption('basic', form.FileUpload, 'cert', _('cert'), _('Local certificate'));
		o.root_directory = '/etc/openvpn';
		o.path = 'client.crt';  // 指定固定文件名
		o.enable_remove = true;
		o.enable_upload = true;
		o.datatype = 'file';
		
		o = s.taboption('basic', form.FileUpload, 'key', _('key'), _('Local private key'));
		o.root_directory = '/etc/openvpn';
		o.path = 'client.key';  // 指定固定文件名
		o.enable_remove = true;
		o.enable_upload = true;
		o.datatype = 'file';
		
		// o = s.taboption('basic', form.FileUpload, 'tls_auth', _('tls-auth'), _('Additional authentication over TLS'));
		// o.root_directory = '/etc/openvpn';
		// o.enable_remove = true;
		// o.enable_upload = true;
		// o.datatype = 'file';
		// o.path = 'ta.key';  // 指定固定文件名
		
		o = s.taboption('basic', form.FileUpload, 'tls_auth_file', _('tls_auth'), _('Additional authentication over TLS'));
		o.root_directory = '/etc/openvpn';
		o.path = 'ta.key';  // 固定文件名
		o.enable_remove = true;
		o.enable_upload = true;
		o.optional = true;
		o.datatype = 'file';

		// 关键：重写 write 方法
		o.write = function(section_id, value) {
			console.log('tls-auth write called with value:', value);
			
			if (value && value !== '' && value !== 'remove') {
				// 1. 设置 tls_auth 配置（OpenVPN 使用）
				uci.set('openvpn', section_id, 'tls_auth', value + ' 1');
				uci.set('openvpn', section_id, 'tls_auth_file', value);
				
				return value;
			}
		};
		
		o.remove = function(section_id) {
			uci.unset('openvpn', section_id, 'tls_auth');
			uci.unset('openvpn', section_id, 'tls_auth_file');
		};

		o = s.taboption('advanced', form.ListValue, 'verb', _('verb'), _('Set output verbosity'));
		o.value('0');
		o.value('1');
		o.value('2');
		o.value('3');
		o.value('4');
		o.value('5');
		o.value('6');
		o.value('7');
		o.value('8');
		o.value('9');
		o.value('10');
		o.value('11');
		o.default='3';
		
		o = s.taboption('advanced', form.Value, 'auth', _('auth'), _('HMAC authentication for packets'));
		o.default='SHA1';

		o = s.taboption('advanced', form.Value, 'cipher', _('cipher'), _('Encryption cipher for packets'));
		o.value('AES-128-CBC');
		o.value('AES-128-CFB');
		o.value('AES-128-CFB1');
		o.value('AES-128-CFB8');
		o.value('AES-128-GCM');
		o.value('AES-128-OFB');
		o.value('AES-192-CBC');
		o.value('AES-192-CFB');
		o.value('AES-192-CFB1');
		o.value('AES-192-CFB8');
		o.value('AES-192-GCM');
		o.value('AES-192-OFB');
		o.value('AES-256-CBC');
		o.value('AES-256-CFB');
		o.value('AES-256-CFB1');
		o.value('AES-256-CFB8');
		o.value('AES-256-GCM');
		o.value('AES-256-OFB');
		o.value('BF-CBC');
		o.value('BF-CFB');
		o.value('BF-OFB');
		o.value('CAST5-CBC');
		o.value('CAST5-CFB');
		o.value('CAST5-OFB');
		o.value('DES-CBC');
		o.value('DES-CFB');
		o.value('DES-CFB1');
		o.value('DES-CFB8');
		o.value('DES-EDE-CBC');
		o.value('DES-EDE-CFB');
		o.value('DES-EDE-OFB');
		o.value('DES-EDE3-CBC');
		o.value('DES-EDE3-CFB');
		o.value('DES-EDE3-CFB1');
		o.value('DES-EDE3-CFB8');
		o.value('DES-EDE3-OFB');
		o.value('DES-OFB');
		o.value('DESX-CBC');
		o.value('RC2-40-CBC');
		o.value('RC2-64-CBC');
		o.value('RC2-CBC');
		o.value('RC2-CFB');
		o.value('RC2-OFB');
		o.default='AES-128-CBC';
		
		o = s.taboption('advanced', form.ListValue, 'compress', _('compress'), _('Security recommendation: It is recommended to not enable compression and set this parameter to `no`'));
		o.value('', 'no');
		o.value('lzo');
		o.value('lz4');
		
		o = s.taboption('advanced', form.ListValue, 'remote_cert_tls', _('remote_cert_tls'), _('Require explicit key usage on certificate'));
		o.value('', _('disable'));
		o.value('client');
		o.value('server');
		
		o = s.taboption('advanced', form.Flag, 'nobind', _('nobind'), _('Do not bind to local address and port'));
		
		o = s.taboption('advanced', form.Flag, 'client', _('client'), _('Configure client mode'));
		o.rmempty=false;
		o.default='1';
		
		o = s.taboption('advanced', form.FileUpload, 'auth_user_pass', _('auth_user_pass'), _('Authenticate using username/password'));
		o.root_directory = '/etc/openvpn';
		o.path = 'userpass.txt';  // 指定固定文件名
		o.enable_remove = true;
		o.enable_upload = true;
		o.datatype = 'file';
		o.depends('client', '1');
		
		o = s.taboption('advanced', form.DummyValue, '_separate');
        o.titleref = L.url("admin", "vpn", "openvpn")
        o.rawhtml = true;
        o.title = '<b>' + _("Show more") + '</b>';
        o.cfgvalue = function() {
			return '<div style="line-height: normal">' + _("Follow this link<br />You will find more OpenVPN configuration") + '<div>'
        };
		
        s.findInterface = function() {
            if (!this.networks || this.networks.length === 0) {
                return null;
            }
            
            for (var i = 0; i < this.networks.length; i++) {
                var ifc = this.networks[i];
                if (ifc) {
                    var name = ifc.getName();
                    if (name && (name === 'openvpn' || name.toLowerCase().includes('openvpn'))) {
                        return ifc;
                    }
                }
            }
            return null;
        };

        return m.render().then(L.bind(function(m, nodes) {
            var section_id = 'sample_client';
            
            poll.add(L.bind(function() {
                return network.getNetworks().then(L.bind(function(networks) {
                    this.networks = networks;
                    
                    var currentInterface = this.findInterface();
                    
                    if (currentInterface) {
                        var statusNode = nodes.querySelector('#%s-status'.format(section_id));
                        
                        if (statusNode) {
                            render_modal_status(statusNode, currentInterface);
                        }
                    } else {
                        var statusNode = nodes.querySelector('#%s-status'.format(section_id));
                        
                        if (statusNode) {
                            dom.content(statusNode, [
                                E('img', {
                                    'src': L.resource('icons/ethernet_disabled.png'),
                                    'title': _('Not present'),
                                    'style': 'vertical-align: middle; margin: 0 0.2rem 0 0;'
                                }),
                                E('em', { 
                                    'style': 'vertical-align: middle;'
                                }, _('Interface not present or not connected yet.'))
                            ]);
                        }
                    }
                    
                    return network.flushCache();
                }, this));
            }, s), 5);

            return nodes;
        }, this, m));
    }
});