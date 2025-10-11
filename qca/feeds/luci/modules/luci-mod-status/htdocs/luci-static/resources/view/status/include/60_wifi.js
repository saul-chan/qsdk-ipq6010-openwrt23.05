    'use strict';
    'require baseclass';
    'require dom';
    'require network';
    'require uci';
    'require fs';
    'require rpc';
	'require ui';
	function frequencyToChannel(frequencyGHz) {
		if (frequencyGHz >= 2.412 && frequencyGHz <= 2.472) {
			// 2.4GHz Band (Channels 1-14)
			return Math.round((frequencyGHz - 2.412) / 0.005 + 1);
		} else if (frequencyGHz >= 5.170 && frequencyGHz <= 5.925) {
			// 5GHz Band (Channels 36-165)
			return Math.round((frequencyGHz - 5.0) / 0.005);
		} else if (frequencyGHz >= 5.925 && frequencyGHz <= 7.125) {
			// 6GHz Band (Wi-Fi 6E, Channels 1-233)
			return Math.round((frequencyGHz - 5.930) / 0.005 + 1);
		} else {
			return null; // 不在标准 Wi-Fi 频段
		}
	}
	function parseIwconfig(output) {
    const interfaces = [];
    // 按空行分割不同接口的信息
    const interfaceBlocks = output.split('\n\n').filter(block => block.trim() !== '');
    
    interfaceBlocks.forEach(block => {
        const lines = block.split('\n').filter(line => line.trim() !== '');
        if (lines.length === 0) return;
        
        const interfaceInfo = {};
        // 第一行包含接口名称和协议
        const firstLine = lines[0];
        const interfaceMatch = firstLine.match(/^(\S+)\s+IEEE\s+(\S+)/);
        if (interfaceMatch) {
            interfaceInfo.dev = interfaceMatch[1].trim();
			if(interfaceInfo.dev=="ath0" || interfaceInfo.dev=="ath01")
				interfaceInfo.devicename = "wifi0";
			else
				interfaceInfo.devicename = "wifi1";
            interfaceInfo.type = interfaceMatch[2].trim();
        }
        
        // 解析其他信息
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i].trim();
            
            // 解析 ESSID
            const essidMatch = line.match(/ESSID:"([^"]*)"/);
            if (essidMatch) {
                interfaceInfo.essid = essidMatch[1];
            }
            
            // 解析模式
            const modeMatch = line.match(/Mode:(\S+)/);
            if (modeMatch) {
                interfaceInfo.mode = modeMatch[1];
            }
            
            // 解析频率
            const freqMatch = line.match(/Frequency:([0-9.]+) GHz/);
            if (freqMatch) {
                interfaceInfo.frequency = parseFloat(freqMatch[1]);
				const chanMatch = frequencyToChannel(interfaceInfo.frequency);
				if (chanMatch) {
					interfaceInfo.channel = chanMatch;
				}
            }
            
            // 解析 MAC 地址
            const macMatch = line.match(/Access Point:\s+([0-9A-Fa-f:]{17})/);
            if (macMatch) {
                interfaceInfo.mac = macMatch[1];
            }
            
            // 解析比特率
            const bitRateMatch = line.match(/Bit Rate:([0-9.]+) ([GM]b\/s)/);
            if (bitRateMatch) {
                interfaceInfo.bitRate = {
                    value: parseFloat(bitRateMatch[1]),
                    unit: bitRateMatch[2]
                };
            }
            
            // 解析发射功率
            const txPowerMatch = line.match(/Tx-Power:(-?\d+) dBm/);
            if (txPowerMatch) {
                interfaceInfo.txPower = parseInt(txPowerMatch[1]);
            }
            
            // 解析加密信息
            const encryptionMatch = line.match(/Encryption key:([^ ]+)/);
            if (encryptionMatch) {
                interfaceInfo.encryptionKey = encryptionMatch[1].trim();
            }
            
            // 解析安全模式
            const securityMatch = line.match(/Security mode:(\S+)/);
            if (securityMatch) {
                interfaceInfo.securityMode = securityMatch[1];
            }
            
            // 解析电源管理
            const powerMgmtMatch = line.match(/Power Management:(\S+)/);
            if (powerMgmtMatch) {
                interfaceInfo.powerManagement = powerMgmtMatch[1];
            }
            
            // 解析链路质量
            const qualityMatch = line.match(/Link Quality=(\d+\/\d+)/);
            if (qualityMatch) {
                interfaceInfo.linkQuality = qualityMatch[1];
				const parts = interfaceInfo.linkQuality.split('/');
				interfaceInfo.quality = parseInt(parseInt(parts[0], 10) / parseInt(parts[1], 10) * 100);
            }
            
            // 解析信号和噪声水平
            const signalMatch = line.match(/Signal level=(-?\d+) dBm/);
            if (signalMatch) {
                interfaceInfo.signalLevel = parseInt(signalMatch[1]);
            }
            
            const noiseMatch = line.match(/Noise level=(-?\d+) dBm/);
            if (noiseMatch) {
                interfaceInfo.noiseLevel = parseInt(noiseMatch[1]);
            }
            
            // 解析统计信息
            const invalidNwidMatch = line.match(/Rx invalid nwid:(\d+)/);
            if (invalidNwidMatch) {
                interfaceInfo.rxInvalidNwid = parseInt(invalidNwidMatch[1]);
            }
            
            const invalidCryptMatch = line.match(/Rx invalid crypt:(\d+)/);
            if (invalidCryptMatch) {
                interfaceInfo.rxInvalidCrypt = parseInt(invalidCryptMatch[1]);
            }
            
            const invalidFragMatch = line.match(/Rx invalid frag:(\d+)/);
            if (invalidFragMatch) {
                interfaceInfo.rxInvalidFrag = parseInt(invalidFragMatch[1]);
            }
            
            const excessiveRetriesMatch = line.match(/Tx excessive retries:(\d+)/);
            if (excessiveRetriesMatch) {
                interfaceInfo.txExcessiveRetries = parseInt(excessiveRetriesMatch[1]);
            }
            
            const invalidMiscMatch = line.match(/Invalid misc:(\d+)/);
            if (invalidMiscMatch) {
                interfaceInfo.invalidMisc = parseInt(invalidMiscMatch[1]);
            }
            
            const missedBeaconMatch = line.match(/Missed beacon:(\d+)/);
            if (missedBeaconMatch) {
                interfaceInfo.missedBeacon = parseInt(missedBeaconMatch[1]);
            }
			
        }
        
        interfaces.push(interfaceInfo);
    });
    
    return interfaces;
}
    return baseclass.extend({
        title: _('Wireless'),
        WPSTranslateTbl: {
            Disabled: _('Disabled'),
            Active: _('Active'),
            'Timed-out': _('Timed-out'),
            Overlap: _('Overlap'),
            Unknown: _('Unknown')
        },
        callSessionAccess: rpc.declare({
            object: 'session',
            method: 'access',
            params: ['scope', 'object', 'function'],
            expect: {
                'access': false
            }
        }),
        wifirate: function(rt) {
            var s = '%.1f\xa0%s, %d\xa0%s'.format(rt.rate / 1000, _('Mbit/s'), rt.mhz, _('MHz'))
              , ht = rt.ht
              , vht = rt.vht
              , mhz = rt.mhz
              , nss = rt.nss
              , mcs = rt.mcs
              , sgi = rt.short_gi
              , he = rt.he
              , he_gi = rt.he_gi
              , he_dcm = rt.he_dcm;
            if (ht || vht) {
                if (vht)
                    s += ', VHT-MCS\xa0%d'.format(mcs);
                if (nss)
                    s += ', VHT-NSS\xa0%d'.format(nss);
                if (ht)
                    s += ', MCS\xa0%s'.format(mcs);
                if (sgi)
                    s += ', ' + _('Short GI').replace(/ /g, '\xa0');
            }
            if (he) {
                s += ', HE-MCS\xa0%d'.format(mcs);
                if (nss)
                    s += ', HE-NSS\xa0%d'.format(nss);
                if (he_gi)
                    s += ', HE-GI\xa0%d'.format(he_gi);
                if (he_dcm)
                    s += ', HE-DCM\xa0%d'.format(he_dcm);
            }
            return s;
        },
        handleDelClient: function(wifinet, mac, ev, cmd) {
            var exec = cmd || 'disconnect';
            dom.parent(ev.currentTarget, '.tr').style.opacity = 0.5;
            ev.currentTarget.classList.add('spinning');
            ev.currentTarget.disabled = true;
            ev.currentTarget.blur();
            wifinet.disconnectClient(mac, true, 5, 60000);
            if (exec == 'addlist') {
                wifinet.maclist.push(mac);
                uci.set('wireless', wifinet.sid, 'maclist', wifinet.maclist);
                return uci.save().then(L.bind(L.ui.changes.init, L.ui.changes)).then(L.bind(L.ui.changes.displayChanges, L.ui.changes));
            }
        },
        handleGetWPSStatus: function(wifinet) {
            return rpc.declare({
                object: 'hostapd.%s'.format(wifinet),
                method: 'wps_status',
            })()
        },
        handleCallWPS: function(wifinet, ev) {
            ev.currentTarget.classList.add('spinning');
            ev.currentTarget.disabled = true;
            ev.currentTarget.blur();
            return rpc.declare({
                object: 'hostapd.%s'.format(wifinet),
                method: 'wps_start',
            })();
        },
        handleCancelWPS: function(wifinet, ev) {
            ev.currentTarget.classList.add('spinning');
            ev.currentTarget.disabled = true;
            ev.currentTarget.blur();
            return rpc.declare({
                object: 'hostapd.%s'.format(wifinet),
                method: 'wps_cancel',
            })();
        },
        renderbox: function(radio, networks, wifinet) {
			//console.log(wifinet);
            var chan = null
              , freq = null
              , rate = null
              , rate_unit = null
              , type = null
              , encryption = null
              , badges = [];
            for (var i = 0; i < networks.length; i++) {
				for(var j=0;j< wifinet.length;j++){
				
                var wifi = wifinet[j]
				  , net = networks[i];
				if(net.getActiveSSID()==wifi.essid)
				{
                var is_assoc = ((net.getBSSID() != '00:00:00:00:00:00' || wifi.mac != '00:00:00:00:00:00' ) && ( net.getChannel() || wifi.channel ) && !net.isDisabled())
                  , quality = (net.getSignalPercent() != 0 ) ? net.getSignalPercent() : wifi.quality;
				 //console.log("wifi",wifi);
				 //console.log("wifi.quality",wifi.quality);
				//console.log("quality",quality);
                var icon;
                if (net.isDisabled())
                    icon = L.resource('icons/signal-none.png');
                else if (quality <= 0)
                    icon = L.resource('icons/signal-0.png');
                else if (quality < 25)
                    icon = L.resource('icons/signal-0-25.png');
                else if (quality < 50)
                    icon = L.resource('icons/signal-25-50.png');
                else if (quality < 75)
                    icon = L.resource('icons/signal-50-75.png');
                else
                    icon = L.resource('icons/signal-75-100.png');
                var WPS_button = null;
                if (net.isWPSEnabled) {
                    if (net.wps_status == 'Active') {
                        WPS_button = E('button', {
                            'class': 'cbi-button cbi-button-remove',
                            'click': L.bind(this.handleCancelWPS, this, net.getIfname()),
                        }, [_('Stop WPS')])
                    } else {
                        WPS_button = E('button', {
                            'class': 'cbi-button cbi-button-apply',
                            'click': L.bind(this.handleCallWPS, this, net.getIfname()),
                        }, [_('Start WPS')])
                    }
                }
				encryption = (encryption != null) ? encryption : (net.getActiveEncryption() != "-") ? net.getActiveEncryption() : uci.get('wireless', net.sid, 'encryption')
				//console.log(net.getActiveEncryption());
                var badge = renderBadge(icon, '%s: %d dBm / %s: %d%%'.format(_('Signal'), net.getSignal(), _('Quality'), quality), _('SSID'), net.getActiveSSID() || '?', _('Mode'), net.getActiveMode(), _('BSSID'), is_assoc ? (net.getActiveBSSID() || wifi.mac || '-') : null, _('Encryption'), is_assoc ? encryption : null, _('Associations'), is_assoc ? (net.assoclist.length || '-') : null, null, is_assoc ? null : E('em', net.isDisabled() ? _('Wireless is disabled') : _('Wireless is not associated')), _('WPS status'), this.WPSTranslateTbl[net.wps_status], '', WPS_button);
                badges.push(badge);
                chan = (chan != null) ? chan : net.getChannel() ? net.getChannel() : wifi.channel;
                freq = (freq != null) ? freq : net.getFrequency() ? net.getFrequency() : wifi.frequency;
                rate = (rate != null) ? rate : (wifi.bitRate != null) ? wifi.bitRate.value : '';
                rate_unit = (rate_unit != null) ? rate_unit : (wifi.bitRate != null) ? wifi.bitRate.unit : '';
				type = (type != null) ? type : radio.getI18n().replace(/^Generic | Wireless Controller .+$/g, '')
				if (type == "unknown")
					type = wifi.type;
				}
			}
            }
			//console.log(radio.getI18n());
            
			
            return E('div', {
                class: 'ifacebox'
            }, [E('div', {
                class: 'ifacebox-head center ' + (radio.isUp() ? 'active' : '')
            }, E('strong', radio.getName())), E('div', {
                class: 'ifacebox-body left'
            }, [L.itemlist(E('span'), [_('Type'), type , _('Channel'), chan ? '%d (%.3f %s)'.format(chan, freq, _('GHz')) : '-', _('Bitrate'), rate ? '%f %s'.format(rate, rate_unit) : '-', ]), E('div', {}, badges)])]);
        },
        isWPSEnabled: {},
        load: function() {
			//console.log('[DEBUG] Starting load function');
            return Promise.all([network.getWifiDevices()
			, network.getWifiNetworks()
			, network.getHostHints()
			, this.callSessionAccess('access-group', 'luci-mod-status-index-wifi', 'read')
			, this.callSessionAccess('access-group', 'luci-mod-status-index-wifi', 'write')
			, uci.load('wireless')
			, fs.exec_direct('/usr/sbin/iwconfig')
            ]).then(L.bind(function(data) {
                var tasks = []
                  , radios_networks_hints = data[1]
                  , hasWPS = L.hasSystemFeature('hostapd', 'wps')
				  , wifiConfigs = data[6];
                for (var i = 0; i < radios_networks_hints.length; i++) {
                    tasks.push(L.resolveDefault(radios_networks_hints[i].getAssocList(), []).then(L.bind(function(net, list) {
                        net.assoclist = list.sort(function(a, b) {
                            return a.mac > b.mac
                        });
                    }, this, radios_networks_hints[i])));
                    if (hasWPS && uci.get('wireless', radios_networks_hints[i].sid, 'wps_pushbutton') == '1') {
                        radios_networks_hints[i].isWPSEnabled = true;
                        tasks.push(L.resolveDefault(this.handleGetWPSStatus(radios_networks_hints[i].getIfname()), null).then(L.bind(function(net, data) {
                            net.wps_status = data ? data.pbc_status : _('No Data');
                        }, this, radios_networks_hints[i])));
                    }
                }
				//console.log(data[6]);
				//console.log(data[0]);
				//console.log(data[1]);
				//console.log(data[2]);
				//console.log(parseIwconfig(data[6]));
				
                return Promise.all(tasks).then(function() {
                    return data;
                });
            }, this));
        },
        render: function(data) {
            var seen = {}
              , radios = data[0]
              , networks = data[1]
              , hosthints = data[2]
              , hasReadPermission = data[3]
              , hasWritePermission = data[4]
			  , wifinet = parseIwconfig(data[6]);
            var table = E('div', {
                'class': 'network-status-table'
            });
			var wifinetMap = {};
				for (var j = 0; j < wifinet.length; j++) {
					wifinetMap[wifinet[j].dev] = wifinet[j];
					//console.log("wifinet", wifinet[j])
				}
			//console.log("wifinetMap", wifinetMap)	
            for (var i = 0; i < radios.sort(function(a, b) {
                a.getName() > b.getName()
            }).length; i++){
				//console.log(wifinetMap[radios[i].getName()]);
                table.appendChild(this.renderbox(radios[i]
				, networks.filter(function(net) {
					//console.log(net)
                    return net.getWifiDeviceName() == radios[i].getName()
				})
				, wifinet.filter(function(dev) {
					//console.log("dev",dev.devicename);
                    return dev.devicename == radios[i].getName()
				})
			))};
            if (!table.lastElementChild)
                return null;
            var assoclist = E('table', {
                'class': 'table assoclist'
            }, [E('tr', {
                'class': 'tr table-titles'
            }, [E('th', {
                'class': 'th nowrap'
            }, _('Network')), E('th', {
                'class': 'th hide-xs'
            }, _('MAC address')), E('th', {
                'class': 'th'
            }, _('Host')), E('th', {
                'class': 'th'
            }, '%s / %s'.format(_('Signal'), _('Noise'))), E('th', {
                'class': 'th'
            }, '%s / %s'.format(_('RX Rate'), _('TX Rate')))])]);
            var rows = [];
            for (var i = 0; i < networks.length; i++) {
                var macfilter = uci.get('wireless', networks[i].sid, 'macfilter')
                  , maclist = {};
                if (macfilter != null && macfilter != 'disable') {
                    networks[i].maclist = L.toArray(uci.get('wireless', networks[i].sid, 'maclist'));
                    for (var j = 0; j < networks[i].maclist.length; j++) {
                        var mac = networks[i].maclist[j].toUpperCase();
                        maclist[mac] = true;
                    }
                }
                for (var k = 0; k < networks[i].assoclist.length; k++) {
                    var bss = networks[i].assoclist[k]
                      , name = hosthints.getHostnameByMACAddr(bss.mac)
                      , ipv4 = hosthints.getIPAddrByMACAddr(bss.mac)
                      , ipv6 = hosthints.getIP6AddrByMACAddr(bss.mac);
                    var icon;
                    var q = Math.min((bss.signal + 110) / 70 * 100, 100);
                    if (q == 0)
                        icon = L.resource('icons/signal-0.png');
                    else if (q < 25)
                        icon = L.resource('icons/signal-0-25.png');
                    else if (q < 50)
                        icon = L.resource('icons/signal-25-50.png');
                    else if (q < 75)
                        icon = L.resource('icons/signal-50-75.png');
                    else
                        icon = L.resource('icons/signal-75-100.png');
                    var sig_title, sig_value;
                    if (bss.noise) {
                        sig_value = '%d/%d\xa0%s'.format(bss.signal, bss.noise, _('dBm'));
                        sig_title = '%s: %d %s / %s: %d %s / %s %d'.format(_('Signal'), bss.signal, _('dBm'), _('Noise'), bss.noise, _('dBm'), _('SNR'), bss.signal - bss.noise);
                    } else {
                        sig_value = '%d\xa0%s'.format(bss.signal, _('dBm'));
                        sig_title = '%s: %d %s'.format(_('Signal'), bss.signal, _('dBm'));
                    }
                    var hint;
                    if (name && ipv4 && ipv6)
                        hint = '%s <span class="hide-xs">(%s, %s)</span>'.format(name, ipv4, ipv6);
                    else if (name && (ipv4 || ipv6))
                        hint = '%s <span class="hide-xs">(%s)</span>'.format(name, ipv4 || ipv6);
                    else
                        hint = name || ipv4 || ipv6 || '?';
                    var row = [E('span', {
                        'class': 'ifacebadge',
                        'title': networks[i].getI18n(),
                        'data-ifname': networks[i].getIfname(),
                        'data-ssid': networks[i].getActiveSSID()
                    }, [E('img', {
                        'src': L.resource('icons/wifi.png')
                    }), E('span', {}, [' ', networks[i].getShortName(), E('small', {}, [' (', networks[i].getIfname(), ')'])])]), bss.mac, hint, E('span', {
                        'class': 'ifacebadge',
                        'title': sig_title,
                        'data-signal': bss.signal,
                        'data-noise': bss.noise
                    }, [E('img', {
                        'src': icon
                    }), E('span', {}, [' ', sig_value])]), E('span', {}, [E('span', this.wifirate(bss.rx)), E('br'), E('span', this.wifirate(bss.tx))])];
                    if (networks[i].isClientDisconnectSupported() && hasWritePermission) {
                        if (assoclist.firstElementChild.childNodes.length < 6)
                            assoclist.firstElementChild.appendChild(E('th', {
                                'class': 'th cbi-section-actions'
                            }));
                        if (macfilter != null && macfilter != 'disable' && !maclist[bss.mac]) {
                            row.push(new L.ui.ComboButton('button',{
                                'addlist': macfilter == 'allow' ? _('Add to Whitelist') : _('Add to Blacklist'),
                                'disconnect': _('Disconnect')
                            },{
                                'click': L.bind(this.handleDelClient, this, networks[i], bss.mac),
                                'sort': ['disconnect', 'addlist'],
                                'classes': {
                                    'addlist': 'btn cbi-button cbi-button-remove',
                                    'disconnect': 'btn cbi-button cbi-button-remove'
                                }
                            }).render())
                        } else {
                            row.push(E('button', {
                                'class': 'cbi-button cbi-button-remove',
                                'click': L.bind(this.handleDelClient, this, networks[i], bss.mac)
                            }, [_('Disconnect')]));
                        }
                    } else {
                        row.push('-');
                    }
                    rows.push(row);
                }
            }
            cbi_update_table(assoclist, rows, E('em', _('No information available')));
            return E([table, hasReadPermission ? E('h3', _('Associated Stations')) : E([]), hasReadPermission ? assoclist : E([])]);
        }
    });
