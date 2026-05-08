'use strict';
'require form';
'require poll';
'require uci';
'require view';
'require network';
'require dom';
'require fs';
'require ui';

function showAutoCloseNotification(message, duration) {
    var notification = ui.addNotification(null, E('p', message));
    
    // 设置自动关闭
    if (duration && duration > 0) {
        setTimeout(function() {
            if (notification && notification.parentNode) {
                notification.parentNode.removeChild(notification);
            }
        }, duration);
    }
    
    // 允许点击任何地方关闭通知
    document.addEventListener('click', function closeNotification() {
        if (notification && notification.parentNode) {
            notification.parentNode.removeChild(notification);
        }
        // 移除事件监听器，避免重复
        document.removeEventListener('click', closeNotification);
    }, { once: true });
    
    return notification;
}

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

// 改进的Unicode解码函数 - 处理UTF-8编码
function decodeUnicodeSSID(str) {
    if (!str) return str;
    
    // 处理 \xXX 格式的UTF-8编码
    var decoded = str.replace(/\\x([0-9A-Fa-f]{2})/g, function(match, hex) {
        return String.fromCharCode(parseInt(hex, 16));
    });
    
    // 尝试解码UTF-8
    try {
        // 将字符串转换为字节数组
        var bytes = [];
        for (var i = 0; i < decoded.length; i++) {
            bytes.push(decoded.charCodeAt(i));
        }
        
        // 使用TextDecoder解码UTF-8
        if (typeof TextDecoder !== 'undefined') {
            var decoder = new TextDecoder('utf-8');
            var byteArray = new Uint8Array(bytes);
            return decoder.decode(byteArray);
        }
    } catch (e) {
       // console.warn('UTF-8 decoding failed:', e);
    }
    
    return decoded;
}

// 替代方法：使用unescape解码
function decodeSSIDAlternative(str) {
    if (!str) return str;
    
    // 替换 \x 为 %
    var encoded = str.replace(/\\x/g, '%');
    
    try {
        // 使用decodeURIComponent或unescape
        return decodeURIComponent(encoded);
    } catch (e) {
        try {
            // 回退到unescape
            return unescape(encoded);
        } catch (e2) {
            //console.warn('All decoding methods failed:', e2);
            return str;
        }
    }
}

// WiFi扫描函数
function scanWifiNetworks(device) {
    var ifname = (device === 'wifi0') ? 'ath0' : 'ath1';
    
    return fs.exec_direct('/usr/sbin/iw', [ifname, 'scan']).then(function(output) {
        var ssids = [];
        var lines = output.split('\n');
        
        // 解析输出，提取SSID
        for (var i = 0; i < lines.length; i++) {
            var line = lines[i].trim();
            
            // 查找SSID行
            if (line.startsWith('SSID:')) {
                // 提取SSID值
                var ssid = line.substring(5).trim();
                if (ssid) {
                    // 调试：记录原始SSID
                    if (ssid.includes('\\x')) {
                        //console.log('Raw encoded SSID:', ssid);
                    }
                    
                    // 尝试第一种解码方法
                    var decoded1 = decodeUnicodeSSID(ssid);
                    
                    // 尝试第二种解码方法
                    var decoded2 = decodeSSIDAlternative(ssid);
                    
                    // 使用解码后的结果（优先使用第二种方法）
                    var finalSSID = decoded2;
                    
                    // 检查解码是否有效
                    if (decoded2 !== ssid && decoded2 !== decoded1) {
                       // console.log('Decoded successfully:', ssid, '->', decoded2);
                    } else if (decoded1 !== ssid) {
                        //console.log('Decoded with method 1:', ssid, '->', decoded1);
                        finalSSID = decoded1;
                    }
                    
                    // 去重
                    if (ssids.indexOf(finalSSID) === -1) {
                        ssids.push(finalSSID);
                    }
                }
            }
        }
        
       // console.log('Scanned SSIDs from', device + ':', ssids.length, 'networks');
        // 输出几个中文SSID示例
        ssids.forEach(function(ssid, idx) {
            if (ssid.includes('\\x') || /[\u4e00-\u9fa5]/.test(ssid)) {
               // console.log('Chinese SSID [' + idx + ']:', ssid);
            }
        });
        return ssids;
    }).catch(function(err) {
       // console.error('WiFi scan error:', err);
        return Promise.reject(new Error(_('Failed to scan WiFi networks: ') + (err.message || 'Unknown error')));
    });
}

return view.extend({
    load: function() {
        return Promise.all([
            uci.load('wificonfig'),
            uci.load('network'),
            network.getNetworks()
        ]);
    },

    render: function(data) {
        var networks = data[2];
        var wifiIfname = 'sta0';
		var m, s, t, o;
		var self = this;
        
        // 创建WiFi配置表单
        m = new form.Map('wificonfig', _('General Setup'));
        
        // 基本配置部分
        s = m.section(form.NamedSection, wifiIfname, 'wifi-iface');
        s.anonymous = true;
        s.addremove = false;
		
		s.networks = networks;
		s.section_id = 'sta0';
        
		o = s.option( form.DummyValue, '_stat', _('Status'));
        o.modalonly = false;
        o.cfgvalue = L.bind(function(section_id) {
            var net = this.findInterface();
			//console.log(net);
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
		
		s.findInterface = function() {
            if (!this.networks || this.networks.length === 0) {
                return null;
            }
            
            for (var i = 0; i < this.networks.length; i++) {
                var ifc = this.networks[i];
                if (ifc) {
                    var name = ifc.getName();
                    if (name && (name === 'wific' || name.toLowerCase().includes('wific'))) {
						//console.log(name);
                        return ifc;
                    }
                }
            }
            return null;
        };
		
        // 启用开关
        o = s.option(form.Flag, 'en', _('Enable'));
        o.rmempty = false;
        o.default = '1';
        
        // WiFi接口选择
        o = s.option(form.ListValue, 'device', _('WiFi Interface'));
        o.default = uci.get('wificonfig', wifiIfname, 'device') || 'wifi1';
        o.depends('en', '1');
        o.value('wifi1', _('2.4G Client'));
        o.value('wifi0', _('5.8G Client'));
        
        // SSID选择 - 使用ListValue
        o = s.option(form.ListValue, 'ssid', _('SSID'));
        o.validator = 'maxlength="32"';
        o.placeholder = _('Select or enter WiFi name');
        o.depends('en', '1');
        // 添加一个空选项
        o.value('', _('-- Please choose --'));
		// 覆盖 load 方法以确保包含已保存的值
		o.load = function(section_id) {
			// 先调用父类的 load 方法
			var value = form.ListValue.prototype.load.call(this, section_id);
			
			// 获取已保存的 SSID 值
			var savedSsid = uci.get('wificonfig', section_id, 'ssid');
			//console.log('Loading SSID option, saved value:', savedSsid);
			
			// 如果已有保存的值，确保它在选项中
			if (savedSsid && savedSsid.trim()) {
				var found = false;
				for (var key in this.valuechoices) {
					if (key === savedSsid) {
						found = true;
						break;
					}
				}
				
				// 如果选项中没有保存的值，添加它
				if (!found) {
					this.value(savedSsid, savedSsid);
					//console.log('Added saved SSID to options:', savedSsid);
				}
			}
			
			return value;
		};
		o.cfgvalue = function(section_id) {
			var value = form.ListValue.prototype.cfgvalue.call(this, section_id);
			//console.log('cfgvalue for SSID:', value);
			
			// 如果没有值，从UCI获取
			if (!value || value === '') {
				value = uci.get('wificonfig', section_id, 'ssid') || '';
				//console.log('Fallback to UCI value:', value);
			}
			
			return value;
		};
        
        // 扫描按钮
        o = s.option(form.DummyValue, '_scan', _('  '));
        o.modalonly = false;
		o.depends('en', '1');
        
        // 自定义渲染扫描按钮
        o.cfgvalue = function() {
            var button = E('button', {
                'class': 'btn cbi-button cbi-button-button',
				'style': 'margin-left: 0px !important',
                'click': function(ev) {
                    ev.preventDefault();
                    ev.stopPropagation();
                    
                    // 获取当前选择的设备
                    var deviceSelect = document.getElementById('widget.cbid.wificonfig.sta0.device');
                    var device = 'wifi1';
                    
                    if (deviceSelect) {
                        device = deviceSelect.value || 'wifi1';
                       // console.log('Scanning device:', device);
                    } else {
                        device = uci.get('wificonfig', wifiIfname, 'device') || 'wifi1';
                    }
                    
                    // 显示扫描中提示
                    ui.showModal(_('Scanning WIFI SSID on %s interface...').format(device === 'wifi0' ? '5.8G' : '2.4G'), [
                        E('p', { 'class': 'spinning' }, _('This window will close automatically. Please wait.'))
                    ]);
                    
                    scanWifiNetworks(device).then(function(ssids) {
                        ui.hideModal();
                        
                        //console.log('Scan results for', device + ':', ssids);
                        
                        // 确保ssids是数组
                        if (!Array.isArray(ssids)) {
                           // console.error('ssids is not an array:', ssids);
                            ssids = [];
                        }
                        
                        if (ssids.length > 0) {
                            // 获取SSID选择框
                            var ssidSelect = document.getElementById('widget.cbid.wificonfig.sta0.ssid');
                            if (ssidSelect) {
                                // 清除现有的选项（除了第一个空选项）
                                while (ssidSelect.options.length > 1) {
                                    ssidSelect.remove(1);
                                }
                                
                               // console.log('Cleared existing options, now adding new ones for', device);
                                
                                // 添加新的扫描结果
                                var addedCount = 0;
                                for (var i = 0; i < ssids.length; i++) {
                                    var ssid = ssids[i];
                                    // 确保ssid是字符串且不为空
                                    if (ssid && typeof ssid === 'string' && ssid.trim()) {
                                        var option = document.createElement('option');
                                        option.value = ssid;
                                        option.textContent = ssid;
                                        ssidSelect.appendChild(option);
                                        addedCount++;
                                        
                                        // 调试中文显示
                                        if (ssid.includes('\\x') || /[\u4e00-\u9fa5]/.test(ssid)) {
                                         //   console.log('Adding Chinese SSID:', ssid);
                                        }
                                    }
                                }
                                
                               // console.log('Added', addedCount, 'new networks for', device);
                                
                                // 如果有网络，选择第一个网络
                                if (addedCount > 0 && ssidSelect.options.length > 1) {
                                    ssidSelect.selectedIndex = 1; // 选择第一个非空选项
                                    // 触发change事件以更新表单
                                    ssidSelect.dispatchEvent(new Event('change', { bubbles: true }));
                                }
                                showAutoCloseNotification(
                                    _('Scan completed! Found %d %s WIFI SSID.')
                                    .format(ssids.length, device === 'wifi0' ? '5.8G' : '2.4G'),
                                    5000 // 5秒后自动关闭
                                );
                                /* ui.addNotification(null, E('p', 
                                    _('Scan completed! Found %d %s networks.')
                                    .format(ssids.length, device === 'wifi0' ? '5.8G' : '2.4G')
                                )); */
                            } else {
                               // console.error('SSID select element not found!');
								showAutoCloseNotification(
                                    _('Scan completed! Found %d WIFI SSID.')
                                    .format(ssids.length),
                                    3000
                                );
                                /* ui.addNotification(null, E('p', 
                                    _('Scan completed! Found %d networks.')
                                    .format(ssids.length)
                                )); */
                            }
                        } else {
							showAutoCloseNotification(
                                _('No WIFI SSID found on %s interface.')
                                .format(device === 'wifi0' ? '5.8G' : '2.4G'),
                                3000
                            );
                            /* ui.addNotification(null, E('p', 
                                _('No WiFi networks found on %s interface')
                                .format(device === 'wifi0' ? '5.8G' : '2.4G')
                            )); */
                        }
                    }).catch(function(err) {
                        ui.hideModal();
                        var errorMsg = err ? (err.message || err.toString()) : _('Unknown error');
                       // console.error('Scan error details:', err);
						showAutoCloseNotification(
                            _('Scan failed on %s interface: %s')
                            .format(device === 'wifi0' ? '5.8G' : '2.4G', errorMsg),
                            5000
                        );
                        /* ui.addNotification(null, E('p', 
                            _('Scan failed on %s interface: %s')
                            .format(device === 'wifi0' ? '5.8G' : '2.4G', errorMsg)
                        )); */
                    });
                }
            }, _('Scan'));
            
            return E('div', { 'style': 'margin-top: 5px;' }, button);
        };
        
        // 加密方式
        o = s.option(form.ListValue, 'security', _('Security'));
        o.depends('en', '1');
        o.value('none', _('No Encryption'));
        o.value('WPA2', _('WPA2'));
        o.value('WPA/WPA2-PSK', _('WPA/WPA2-PSK'));
        
        // 密码
        o = s.option(form.Value, 'key', _('Key'));
        o.datatype = 'wpakey';
        o.placeholder = _('Password');
        o.password = true;
        o.depends('security', 'WPA2');
        o.depends('security', 'WPA/WPA2-PSK');
        
        // WDS选项
        o = s.option(form.Flag, 'wds', _('WDS'));
        o.default = '0';
        o.rmempty = false;
        o.depends('en', '1');
		
		t = m.section(form.NamedSection, 'wific', 'interface');
        t.anonymous = false;
        t.addremove = false;
		t.tab('advanced', _('Network Configuration'));
		t.tab('rssi_threshold', _('RSSI Threshold'));
		t.tab('2.4G_channle', _('2.4G Channel Scanning'));
		t.tab('5.8G_channle', _('5.8G Channel Scanning'));
		t.tab('mac_match', _('MAC Match'));
	
		o = t.taboption('advanced', form.ListValue, 'proto', _('Protocol'), _('LAN port of bridge should share the same network segment with upstream equipment'));
		o.value('dhcp', _ ('DHCP address'));
		o.value('static', _ ('Static address'));
		o.value('bridgelan', _ ('Bridge to LAN'));
	
		o = t.taboption('advanced', form.Value, 'ipaddr', _('IP Address'));
		o.datatype = 'ip4addr';
		o.depends('proto', 'static');
		
		o = t.taboption('advanced', form.Value, 'netmask', _('Netmask'));
		o.datatype = 'ip4addr';
		o.default = "255.255.255.0";
        o.value("255.255.255.0");
        o.value("255.255.0.0");
        o.value("255.0.0.0");
		o.depends('proto', 'static');
        
		o = t.taboption('advanced', form.Value, 'gateway', _('Gateway'));
		o.datatype = 'ip4addr';
		o.depends('proto', 'static');
		o.depends('proto', 'bridgelan');
		
		o = t.taboption('advanced', form.DynamicList, 'dns', _('DNS'));
		o.datatype = 'ip4addr';
		o.cast = 'string';
		o.depends('proto', 'static');
		o.depends('proto', 'bridgelan');
		
		o = t.taboption('advanced', form.Value, 'metric', _('Metric'));
		o.default = '20';
		o.depends('proto', 'static');
		o.depends('proto', 'dhcp');
		
		o = t.taboption('rssi_threshold', form.Flag, 'en', _('Enable'));
		o.rmempty = false;
		
		o = t.taboption('rssi_threshold', form.Value, 'rssi_threshold', _('RSSI Threshold'));
		o.default = '-85';
		
		o = t.taboption('rssi_threshold', form.Value, 'Threshold_difference', _('Threshold Difference'), _('WiFi switching occurs when the threshold difference exceeds the set value'));
		o.default = '10';
		
		o = t.taboption('2.4G_channle', form.MultiValue, 'channel_2', _('Channel'));
		o.multiple = true;
		o.value('2412', _('1 (2412MHz)'));
		o.value('2417', _('2 (2417MHz)'));
		o.value('2422', _('3 (2422MHz)'));
		o.value('2427', _('4 (2427MHz)'));
		o.value('2432', _('5 (2432MHz)'));
		o.value('2437', _('6 (2437MHz)'));
		o.value('2442', _('7 (2442MHz)'));
		o.value('2447', _('8 (2447MHz)'));
		o.value('2452', _('9 (2452MHz)'));
		o.value('2457', _('10 (2457MHz)'));
		o.value('2462', _('11 (2462MHz)'));
		o.value('2472', _('12 (2472MHz)'));
		o.value('2477', _('13 (2477MHz)'));
		o.value('2484', _('14 (2484MHz)'));
		
		o = t.taboption('5.8G_channle', form.ListValue, 'country', _('Country'));
		o.value('CN', _('China'));
		o.value('JP', _('Japan'));
		o.value('KR', _('Republic of Korea'));
		o.value('US', _('America'));
		
		o = t.taboption('5.8G_channle', form.MultiValue, 'channel_5', _('Channel'));
		o.multiple = true;
		o.value('5200', _('40 (2412MHz)'));
		o.value('5220', _('44 (5220MHz)'));
		o.value('5240', _('48 (5240MHz)'));
		o.value('5260', _('52 (5260MHz)'));
		o.value('5280', _('56 (5280MHz)'));
		o.value('5300', _('60 (5300MHz)'));
		o.value('5320', _('64 (5320MHz)'));
		o.value('5745', _('149 (5745MHz)'));
		o.value('5765', _('153 (5765MHz)'));
		o.value('5785', _('157 (5785MHz)'));
		o.value('5805', _('161 (5805MHz)'));
		o.value('5825', _('165 (5825MHz)'));
		o.depends('country', 'CN');
		
		o = t.taboption('5.8G_channle', form.MultiValue, 'channel_55', _('Channel'));
		o.multiple = true;
		o.value('5200', _('40 (2412MHz)'));
		o.value('5220', _('44 (5220MHz)'));
		o.value('5240', _('48 (5240MHz)'));
		o.value('5260', _('52 (5260MHz)'));
		o.value('5280', _('56 (5280MHz)'));
		o.value('5300', _('60 (5300MHz)'));
		o.value('5320', _('64 (5320MHz)'));
		o.value('5500', _('100 (5500MHz)'));
		o.value('5520', _('104 (5520MHz)'));
		o.value('5540', _('108 (5540MHz)'));
		o.value('5660', _('112 (5660MHz)'));
		o.value('5580', _('116 (5580MHz)'));
		o.value('5600', _('120 (5600MHz)'));
		o.value('5620', _('124 (5620MHz)'));
		o.value('5640', _('128 (5640MHz)'));
		o.value('5680', _('136 (5680MHz)'));
		o.value('5700', _('140 (5700MHz)'));
		o.value('5720', _('144 (5720MHz)'));
		o.value('5745', _('149 (5745MHz)'));
		o.value('5765', _('153 (5765MHz)'));
		o.value('5785', _('157 (5785MHz)'));
		o.value('5805', _('161 (5805MHz)'));
		o.value('5825', _('165 (5825MHz)'));
		o.depends('country', 'JP');
		o.depends('country', 'US');
		o.depends('country', 'KR');
		
		o = t.taboption('mac_match', form.TextValue, 'MAC', _('MAC Address'));
		
        // 渲染表单
        return m.render().then(L.bind(function(m, nodes) {
            var section_id = 'sta0';
            
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