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
        _('IPv6'),     ip6addrs[0],
        _('IPv6'),     ip6addrs[1],
        _('IPv6'),     ip6addrs[2],
        _('IPv6'),     ip6addrs[3],
        _('IPv6'),     ip6addrs[4],
        _('IPv6'),     ip6addrs[5],
        _('IPv6'),     ip6addrs[6],
        _('IPv6'),     ip6addrs[7],
        _('IPv6'),     ip6addrs[8],
        _('IPv6'),     ip6addrs[9],
        _('IPv6-PD'),  changecount ? null : ifc.getIP6Prefix(),
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
			'style':'margin: 0 0.2rem 0 0;'
        }),
        ifc ? render_status(E('span'), ifc, true) : E('em', _('Interface not present or not connected yet.'))
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
        
        // console.log('DEBUG: Board info:', boardinfo);
        
        m = new form.Map('network', _('Network Configuration'));
        
        s = m.section(form.NamedSection, 'wan', 'interface', _('WAN Configuration'));
        s.anonymous = true;
        s.addremove = false;

        s.networks = networks;
        s.section_id = 'wan';

        s.tab("basic", _("Basic Configuration"));
        s.tab("advanced", _("Advanced Configuration"));
        
        o = s.taboption('basic', form.DummyValue, '_stat', _('Status'));
        o.modalonly = false;
        o.cfgvalue = L.bind(function(section_id) {
            var net = this.findInterface();
			
			var currentProto = uci.get('network', section_id, 'proto') || 'dhcp';
            
            // 如果协议是"aslan"，不显示接口状态，显示桥接信息
            if (currentProto === 'aslan') {
                return E('div', { 
                    'class': 'ifacebox',
                    'style': 'border: 0px solid #dee2e6; min-width: 18rem; padding-left: 6px; background: #fff;'
                }, [
                    E('div', { 
                        'class': 'ifacebox-head center',
                        'style': 'background-color:#fff'
                    }, E('strong', _('-')))
                ]);
            }
			
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
				'style': 'border: 1px solid #dee2e6; min-width: 18rem; padding-left: 6px; background: #fff;'
            }), net);
        }, s);
        o.write = function() {};
        
        o = s.taboption("basic", form.ListValue, 'proto', _('Protocol'));
        o.default = "dhcp";
        o.value("pppoe", _("PPPoE"));
        o.value("dhcp", _("DHCP address"));
        o.value("static", _("Static address"));
        o.value("aslan", _("Bridge to LAN"));
        o.value("disable", _("Disable"));
        
        o = s.taboption("basic", form.Value, 'ipaddr', _('IP Address'));
        o.datatype = 'ip4addr';
        o.default = "192.168.1.100";
        o.depends('proto', 'static');
        
        o = s.taboption("basic", form.Value, 'netmask', _('Netmask'));
        o.datatype = 'ip4addr';
        o.default = "255.255.255.0";
        o.value("255.255.255.0")
        o.value("255.255.0.0")
        o.value("255.0.0.0")
        o.depends('proto', 'static');
        
        o = s.taboption("basic", form.Value, 'gateway', _('Gateway'));
        o.datatype = 'ip4addr';
        o.depends('proto', 'static');
        
        o = s.taboption("basic", form.DynamicList, 'dns', _('DNS Servers'));
        o.datatype = 'ip4addr';
        o.depends('proto', 'static');
        
        o = s.taboption("basic", form.Value, 'username', _('Username'));
        o.datatype = 'string';
        o.depends('proto', 'pppoe');
        
        o = s.taboption("basic", form.Value, 'password', _('Password'));
        o.datatype = 'string';
        o.password = true;
        o.depends('proto', 'pppoe');
        
        o = s.taboption("advanced", form.ListValue, 'name', _('Interface'));
        
        // 根据板子型号添加接口选项
        var boardName = boardinfo.board_name || '';
        // console.log('DEBUG: Board name:', boardName);
        
        if (boardName && boardName.match(/ap-cp01-c5/i)) {
            // console.log('DEBUG: Using AP-CP01-C5 interface configuration');
            o.value("eth1", _("LAN1(eth1)"));
            o.value("eth2", _("LAN2(eth2)"));
            o.value("eth3", _("LAN3(eth3)"));
            o.value("eth4", _("WAN-T1(eth4)"));
            o.default = "eth4";
        } else if (boardName && boardName.match(/ap-cp01-c3/i)) {
            // console.log('DEBUG: Using AP-CP01-C3 interface configuration');
            o.value("eth0", _("WAN(eth0)"));
            o.value("eth1", _("LAN1(eth1)"));
            o.value("eth2", _("LAN2(eth2)"));
            o.value("eth3", _("LAN3(eth3)"));
            o.default = "eth0";
        } else if (boardName && boardName.match(/ap-cp01-c2/i)) {
            // console.log('DEBUG: Using AP-CP01-C2 interface configuration');
            o.value("eth0", _("LAN1(eth0)"));
            o.value("eth1", _("LAN2(eth1)"));
            o.value("eth2", _("LAN3(eth2)"));
            o.value("eth3", _("LAN4(eth3)"));
            o.value("eth4", _("WAN(eth4)"));
            o.default = "eth4";
        } else {
            // console.log('DEBUG: Using default interface configuration');
            o.value("eth0", _("WAN(eth0)"));
            o.value("eth1", _("LAN1(eth1)"));
            o.value("eth2", _("LAN2(eth2)"));
            o.value("eth3", _("LAN3(eth3)"));
            o.value("eth4", _("SFP(eth4)"));
            o.default = "eth0";
        }
		
		o = s.taboption("advanced", form.Value, 'metric', _('Metric'));
        o.default = '10';
		
		o = s.taboption("advanced", form.Value, 'mtu', _('Override MTU'));
        o.placeholder = '1500'
		o.datatype    = 'max(9200)'
		
		o = s.taboption("advanced", form.DummyValue, '_separate');
        o.titleref = L.url("admin", "network", "network")
        o.rawhtml = true;
        o.title = '<b>' + _("Show more") + '</b>';
        o.cfgvalue = function() {
			return '<div style="line-height: normal">' + _("Follow this link<br />You will find more configuration") + '<div>'
        };

        s.findInterface = function() {
            if (!this.networks || this.networks.length === 0) {
                return null;
            }
            
            for (var i = 0; i < this.networks.length; i++) {
                var ifc = this.networks[i];
                if (ifc) {
                    var name = ifc.getName();
                    if (name && (name === 'wan' || name.toLowerCase().includes('wan'))) {
                        return ifc;
                    }
                }
            }
            return null;
        };

        return m.render().then(L.bind(function(m, nodes) {
            // console.log('DEBUG: m.render completed, nodes:', nodes);
            
            // 方法1：直接重写form.Map的save和apply方法
            // console.log('DEBUG: Overriding form.Map save/apply methods');
            
            var originalSave = m.save;
            var originalApply = m.apply;
            
            if (originalSave && typeof originalSave === 'function') {
                m.save = function(ev) {
                    // console.log('DEBUG: Custom save method called');
                    return originalSave.call(this, ev).then(function() {
                        // console.log('DEBUG: Original save completed, now restarting netwan');
                        // 延迟执行，确保配置已保存
                        setTimeout(function() {
                            self.restartNetwan(false);
                        }, 500);
                        return Promise.resolve();
                    });
                };
            }
            
            if (originalApply && typeof originalApply === 'function') {
                m.apply = function(ev) {
                    // console.log('DEBUG: Custom apply method called');
                    return originalApply.call(this, ev).then(function() {
                        // console.log('DEBUG: Original apply completed, now restarting netwan');
                        // 延迟执行，确保配置已应用
                        setTimeout(function() {
                            self.restartNetwan(true);
                        }, 500);
                        return Promise.resolve();
                    });
                };
            }
            
            // 方法2：设置MutationObserver监听按钮（作为备用）
            this.setupMutationObserver();
            
            var section_id = 'wan';
            
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
                                    'title': _('Not present')
                                }),
                                E('em', _('Interface not present or not connected yet.'))
                            ]);
                        }
                    }
                    
                    return network.flushCache();
                }, this));
            }, s), 5);

            return nodes;
        }, this, m));
    },
    
    // 设置MutationObserver监听按钮
    setupMutationObserver: function() {
        // console.log('DEBUG: Setting up MutationObserver');
        
        var self = this;
        var observer = new MutationObserver(function(mutations) {
            mutations.forEach(function(mutation) {
                if (mutation.addedNodes.length) {
                    // console.log('DEBUG: DOM changed, added nodes:', mutation.addedNodes.length);
                    
                    for (var i = 0; i < mutation.addedNodes.length; i++) {
                        var node = mutation.addedNodes[i];
                        if (node.nodeType === 1) {
                            var buttons = node.querySelectorAll('button, input[type="submit"]');
                            // console.log('DEBUG: Found', buttons.length, 'buttons in new node');
                            
                            for (var j = 0; j < buttons.length; j++) {
                                var button = buttons[j];
                                var btnText = (button.value || button.textContent || '').toLowerCase().trim();
                                // console.log('DEBUG: Button text:', btnText);
                                
                                // 识别保存和应用按钮
                                if (btnText.includes('保存') || btnText.includes('save')) {
                                    // console.log('DEBUG: Found save/apply button via MutationObserver');
                                    self.setupButtonHandler(button);
                                }
                            }
                        }
                    }
                }
            });
        });
        
        observer.observe(document.body, {
            childList: true,
            subtree: true
        });
        
        this.observer = observer;
        // console.log('DEBUG: MutationObserver started');
    },
    
    // 设置按钮处理函数
    setupButtonHandler: function(button) {
        var btnText = (button.value || button.textContent || '').toLowerCase().trim();
        var isApply = btnText.includes('apply') || btnText.includes('应用');
        
        // console.log('DEBUG: Setting up handler for button:', btnText, 'isApply:', isApply);
        
        // 避免重复设置
        if (button.dataset.netwanHandler) {
            // console.log('DEBUG: Handler already set for this button');
            return;
        }
        
        button.dataset.netwanHandler = 'true';
        var self = this;
        
        // 保存原始onclick
        var originalOnClick = button.onclick;
        
        button.onclick = function(ev) {
            // console.log('DEBUG: Button handler called for', btnText);
            
            // 先执行原始点击
            var result;
            if (originalOnClick) {
                result = originalOnClick.call(this, ev);
            }
            
            // 延迟执行netwan重启
            setTimeout(function() {
                // console.log('DEBUG: Executing netwan restart after button click');
                self.restartNetwan(isApply).then(function() {
                    ui.addNotification(null, E('p', 
                        isApply ? 
                            _('Configuration applied and netwan service restarted successfully') :
                            _('Configuration saved and netwan service restarted successfully')
                    ));
                }).catch(function(err) {
                    // console.error('DEBUG: Failed to restart netwan:', err);
                    ui.addNotification(null, E('p', 
                        _('Configuration saved but error restarting netwan: %s').format(err.message)
                    ));
                });
            }, 1000); // 等待1秒确保保存完成
            
            return result;
        };
        
        // console.log('DEBUG: Button handler setup complete');
    },
    
    // 重启netwan服务
    restartNetwan: function(isApply) {
        // console.log('DEBUG: restartNetwan called, isApply:', isApply);
        
        return new Promise(function(resolve, reject) {
            // 显示加载提示
            /* ui.showModal(_('Applying configuration...'), [
                E('p', { 'class': 'spinning' }, 
                    isApply ? 
                        _('Applying configuration and restarting network service...') :
                        _('Saving configuration and restarting network service...')
                )
            ]); */
            
            // 使用fs.exec_direct执行命令
            fs.exec_direct('/etc/init.d/netwan', ['restart']).then(function(output) {
                // console.log('DEBUG: exec_direct result:', output);
                
                // 关闭加载提示
                setTimeout(function() {
                    ui.hideModal();
                }, 500);
                
                resolve(output);
                
            }).catch(function(err) {
                // console.error('DEBUG: exec_direct error:', err);
                
                // 关闭加载提示
                setTimeout(function() {
                    ui.hideModal();
                }, 500);
                
                reject(err);
            });
        });
    },
    
    // 清理函数
    onDestroy: function() {
        if (this.observer) {
            this.observer.disconnect();
            // console.log('DEBUG: MutationObserver disconnected');
        }
    }
});