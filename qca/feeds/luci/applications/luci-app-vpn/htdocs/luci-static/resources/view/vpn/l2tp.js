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
        _('Error'),    errors ? errors[0] : null,
        _('Error'),    errors ? errors[1] : null,
        _('Error'),    errors ? errors[2] : null,
        _('Error'),    errors ? errors[3] : null,
        _('Error'),    errors ? errors[4] : null,
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
        var dev = m, s, o;
        var self = this;
        
        m = new form.Map('network', _('L2TP Client'));
        
        s = m.section(form.NamedSection, 'l2tp', 'interface');
        s.anonymous = false;
        s.addremove = false;

        s.networks = networks;
        s.section_id = 'l2tp';
		
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
        
        o = s.taboption('basic', form.Flag, 'auto', _('Enable'));
		//o.modalonly = true;
		o.default = o.enabled;
        
        o = s.taboption('basic', form.Value, 'server', _('Server Address'));
        o.datatype = 'host';
        
		o = s.taboption('basic', form.Value, 'username', _('Username'));
        
		o = s.taboption('basic', form.Value, 'password', _('Password'));
		o.password = true
		
		o = s.taboption('basic', form.ListValue, 'mode', _('Protocol'));
        o.default = "dhcp";
        o.value("dhcp", _("DHCP address"));
        o.value("static", _("Static address"));
        
        o = s.taboption('basic', form.Value, 'ipaddr', _('IP Address'));
        o.datatype = 'ip4addr';
        //o.default = "192.168.1.100";
        o.depends('mode', 'static');
       
        o = s.taboption('basic', form.Value, 'gateway', _('Gateway'));
        o.datatype = 'ip4addr';
        o.depends('mode', 'static');
		
		o = s.taboption('advanced', form.Value, '_keepalive_failure', _('LCP echo failure threshold'), _('Presume peer to be dead after given amount of LCP echo failures, use 0 to ignore failures'));
		o.placeholder = '0';
		o.datatype    = 'uinteger';
		o.write       = write_keepalive;
		o.remove      = write_keepalive;
		o.cfgvalue = function(section_id) {
			var v = uci.get('network', section_id, 'keepalive');
			if (typeof(v) == 'string' && v != '') {
				var m = v.match(/^(\d+)[ ,]\d+$/);
				return m ? m[1] : v;
			}
		};

		o = s.taboption('advanced', form.Value, '_keepalive_interval', _('LCP echo interval'), _('Send LCP echo requests at the given interval in seconds, only effective in conjunction with failure threshold'));
		o.placeholder = '5';
		o.datatype    = 'min(1)';
		o.write       = write_keepalive;
		o.remove      = write_keepalive;
		o.cfgvalue = function(section_id) {
			var v = uci.get('network', section_id, 'keepalive');
			if (typeof(v) == 'string' && v != '') {
				var m = v.match(/^\d+[ ,](\d+)$/);
				return m ? m[1] : v;
			}
		};
		
		o = s.taboption('advanced', form.Value, 'mtu', _('Override MTU'));
		o.placeholder = '1410';
		o.datatype    = 'max(9200)';
		
		o = s.taboption('advanced', form.Value, 'metric', _('Metric'));
		o.default=9
		
        s.findInterface = function() {
            if (!this.networks || this.networks.length === 0) {
                return null;
            }
            
            for (var i = 0; i < this.networks.length; i++) {
                var ifc = this.networks[i];
                if (ifc) {
                    var name = ifc.getName();
                    if (name && (name === 'l2tp' || name.toLowerCase().includes('l2tp'))) {
                        return ifc;
                    }
                }
            }
            return null;
        };

        return m.render().then(L.bind(function(m, nodes) {
            var section_id = 'l2tp';
            
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