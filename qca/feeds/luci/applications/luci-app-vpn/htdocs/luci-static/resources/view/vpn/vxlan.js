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
        null, changecount ? E('a', {
            href: '#',
            click: L.bind(ui.changes.displayChanges, ui.changes)
        }, _('Interface has %d pending changes').format(changecount)) : null
    ]);
}

function render_modal_status(node, ifc) {
    var dev = ifc ? (ifc.getDevice() || ifc.getL3Device()) : null;
	
	var l3dev = (dev && dev.isUp())? dev : ifc.getL3Device();

    dom.content(node, [
        E('img', {
            'src': L.resource('icons/%s%s.png').format(dev ? dev.getType() : 'ethernet', (dev && (dev.isUp() || l3dev.isUp())) ? '' : '_disabled'),
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
        var m, s, t, o;
        var self = this;
        
        m = new form.Map('network', _('VXLAN'));
        
        s = m.section(form.NamedSection, 'vxlan100', 'interface');
        s.anonymous = false;
        s.addremove = false;

        s.networks = networks;
        s.section_id = 'vxlan100';

        o = s.option( form.DummyValue, '_stat', _('Status'));
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
        
        o = s.option( form.Flag, 'auto', _('Enable'));
        o.default = o.enabled;
		o.rmempty = false;
        
        o = s.option( form.Value, 'peeraddr', _('Remote IPv4 address'), _('The IPv4 address or the fully-qualified domain name of the remote end.'));
		o.optional = false;
		o.datatype = 'or(hostname,ip4addr("nomask"))';

		o = s.option( form.Value, 'ipaddr', _('Local IPv4 address'), _('The local IPv4 address over which the tunnel is created (optional).'));
		o.optional = true;
		o.datatype = 'ip4addr("nomask")';
		
		o = s.option( form.Value, 'port', _('Destination port'));
		o.optional = true;
		o.placeholder = 4789;
		o.datatype = 'port';

		o = s.option( form.Value, 'vid', _('VXLAN network identifier'), _('ID used to uniquely identify the VXLAN'));
		o.optional = true;
		o.datatype = 'range(1, 16777216)';
        
        s.findInterface = function() {
            if (!this.networks || this.networks.length === 0) {
                return null;
            }
            
            for (var i = 0; i < this.networks.length; i++) {
                var ifc = this.networks[i];
                if (ifc) {
                    var name = ifc.getName();
                    if (name && (name === 'vxlan100' || name.toLowerCase().includes('vxlan100'))) {
                        return ifc;
                    }
                }
            }
            return null;
        };

        return m.render().then(L.bind(function(m, nodes) {
            var section_id = 'gre';
            
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