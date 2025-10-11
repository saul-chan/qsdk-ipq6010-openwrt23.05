    'use strict';
    'require baseclass';
    'require fs';
    'require rpc';
    'require uci';
	
	var cellularData = {};
	function initCellularData() {
		return fs.read('/var/state/cellular').then(function(content) {
			if (content) {
				var lines = content.split('\n');
				lines.forEach(function(line) {
					//console.log(line);
					var match = line.match(/^cellular\.SIM\.([^=]+)='([^']*)'$/);
					//console.log(match);
					if (match) {
						cellularData[match[1]] = match[2].trim();
					}
				});
			}
			return cellularData;
		}).catch(function(error) {
			console.error('初始化cellular数据失败:', error);
			return {};
		});
	}
	function getField(fieldName) {
		return cellularData[fieldName] || '';
	}
	function getCleanValue(fieldName) {
		var value = cellularData[fieldName] || '';
		
		// 清理值（移除+XXX:前缀）
		var match = value.match(/^\+[A-Za-z]+:\s*(.*)$/);
		if (match && match[1]) {
			var cleaned = match[1].trim() || '';
			//console.log(fieldName + ": " + cleaned);
			return cleaned || '';
		}
		else
			return '';
	}
	function getSignalQuality(sinr, rsrp, rsrq, rssi) {
		var sval = parseInt(getCleanValue('SVAL') || '0');
		var signalIcon = getSignalIcon(sval);
		
		var parts = [];
		
		// 只添加非空的值
		if (rsrq && rsrq !== '') {
			parts.push('RSRQ: ' + rsrq + ' dB');
		}
		
		if (rsrp && rsrp !== '') {
			parts.push('RSRP: ' + rsrp + ' dBm');
		}
		
		if (sinr && sinr !== '') {
			parts.push('SINR: ' + sinr + ' dB');
		}
		
		if (rssi && rssi !== '') {
			parts.push('RSSI: ' + rssi + ' dBm');
		}
		
		// 如果所有值都为空，返回默认提示
		if (parts.length === 0) {
			return _('No signal data available');
		}
		
		var signalText = parts.join(' / ');
		
		// 创建包含图标和文本的HTML元素
		return E('div', { class: 'cellular-signal-with-icon', style: 'display: flex; flex-direction: row; align-items: flex-start;' }, [
			E('img', { 
				src: signalIcon, 
				class: 'cellular-signal-icon',
				title: _('Signal strength: ') + sval + '/5'
			}),
			E('span', { class: 'cellular-signal-text' }, '&#160' + signalText)
		]);
	}
	// 根据SVAL值获取信号图标
	function getSignalIcon(sval) {
		// 根据SVAL值返回对应的信号图标
		if (sval <= 0) {
			return L.resource('icons/signal-none.png');
		} else if (sval <= 2) {
			return L.resource('icons/signal-0-25.png');  // 1格信号
		} else if (sval === 3) {
			return L.resource('icons/signal-25-50.png'); // 2格信号
		} else if (sval === 4) {
			return L.resource('icons/signal-50-75.png'); // 3格信号
		} else {
			return L.resource('icons/signal-75-100.png'); // 4格信号
		}
	}
	function getOperator(cops, imsi) {
    try {
        //console.log('COPS:', cops, 'IMSI:', imsi);
        
        // 优先从COPS获取
        if (cops && cops !== '') {
            var operator = parseOperatorFromCops(cops);
            if (operator && operator !== _('Unknown Operator')) {
                return operator;
            }
        }
        
        // 其次从IMSI获取
        if (imsi && imsi !== '') {
            var operator = parseOperatorFromImsi(imsi);
            if (operator && operator !== _('Unknown Operator')) {
                return operator;
            }
        }
        
        // 如果都不属于三大运营商，尝试显示原始运营商名称或IMSI编码
        return getFallbackOperator(cops, imsi);
    } catch (e) {
        console.error('Error getting operator:', e);
        return _('Unknown Operator');
    }
}

function parseOperatorFromCops(cops) {
    // 支持多种格式:
    // +COPS: 0,0,CHN-UNICOM,7
    // 0,0,CHN-UNICOM,7
    // CHN-UNICOM
    var operatorPatterns = [
        /(?:COPS:\s*)?\d+,\d+,([^,]+)/,  // 标准COPS格式
        /([A-Z]{3}-[A-Z]+)/,             // 直接匹配运营商代码
        /(CHN-[A-Z]+)/,                  // 匹配CHN-开头
        /(China[A-Z\s]+)/i               // 匹配英文名称
    ];
    
    for (var i = 0; i < operatorPatterns.length; i++) {
        var match = cops.match(operatorPatterns[i]);
        if (match && match[1]) {
            var rawName = match[1].trim();
            // 只映射三大运营商，其他保持原样
            return mapChineseOperatorName(rawName);
        }
    }
    
    return null;
}

function parseOperatorFromImsi(imsi) {
    if (!imsi || imsi.length < 5) return null;
    
    var mnc = imsi.substring(0, 5); // MCC + MNC
    
    var operatorMap = {
        // 中国移动
        '46000': _('China Mobile'),
        '46002': _('China Mobile'),
        '46004': _('China Mobile'),
        '46007': _('China Mobile'),
        '46008': _('China Mobile'),
        '46009': _('China Mobile'),
        // 中国联通
        '46001': _('China Unicom'),
        '46006': _('China Unicom'),
        // 中国电信
        '46003': _('China Telecom'),
        '46005': _('China Telecom'),
        '46011': _('China Telecom'),
        // 其他中国运营商仍然映射
        '46020': _('China Tietong')
    };
    
    return operatorMap[mnc] || null;
}

function mapChineseOperatorName(rawName) {
    // 只映射中国的三大运营商，其他保持原样
    var nameMap = {
        // 中国联通
        'CHN-UNICOM': _('China Unicom'),
        'UNICOM': _('China Unicom'),
        'China Unicom': _('China Unicom'),
        // 中国电信
        'CHN-CT': _('China Telecom'),
        'CHN-TELECOM': _('China Telecom'),
        'China Telecom': _('China Telecom'),
        'CT': _('China Telecom'),
        // 中国移动
        'CHN-CMCC': _('China Mobile'),
        'CHN-MOBILE': _('China Mobile'),
        'China Mobile': _('China Mobile'),
        'CMCC': _('China Mobile'),
        'CM': _('China Mobile')
    };
    
    return nameMap[rawName] || rawName; // 不属于三大运营商就返回原始名称
}

function getFallbackOperator(cops, imsi) {
    // 优先尝试从COPS提取原始运营商名称
    if (cops && cops !== '') {
        var operatorName = extractRawOperatorName(cops);
        if (operatorName) {
            return operatorName;
        }
    }
    
    // 如果COPS提取失败，显示IMSI的前几位
    if (imsi && imsi.length >= 5) {
        return 'IMSI:' + imsi.substring(0, 5); // 显示MCC+MNC
    }
    
    return _('');
}

function extractRawOperatorName(cops) {
    // 尝试从各种COPS格式中提取运营商名称
    var patterns = [
        /(?:COPS:\s*)?\d+,\d+,"([^"]+)"/,  // 带引号的格式：0,0,"AT&T",7
        /(?:COPS:\s*)?\d+,\d+,([^,]+)/,    // 不带引号的格式：0,0,CHN-UNICOM,7
        /"([^"]+)"/,                        // 直接匹配引号内的内容
        /([A-Z]{2,}[\w\s\-]+)/i             // 匹配英文运营商名称
    ];
    
    for (var i = 0; i < patterns.length; i++) {
        var match = cops.match(patterns[i]);
        if (match && match[1]) {
            var name = match[1].trim();
            // 过滤掉过长的无意义字符串
            if (name.length > 2 && name.length < 50) {
                return name;
            }
        }
    }
    
    return null;
}
	function getRegistrationStatus() {
		var statusMap = {
			'0': _('Not registered'),
			'1': _('Registered'),
			'2': _('Searching'),
			'3': _('Denied'),
			'4': _('Unknown'),
			'5': _('Roaming'), // 漫游也视为已注册
			'8': _('Emergency')
		};
		
		// 检查各网络注册状态
		var networks = [
			{ key: 'CEREG', name: '4G' },
			{ key: 'CGREG', name: '2G/3G' },
			{ key: 'C5GREG', name: '5G' }
		];
		
		var foundStatuses = [];
		var allStats = [];
		
		networks.forEach(function(net) {
			var value = getCleanValue(net.key);
			if (value && value !== '') {
				var stat = extractStatValue(value);
				if (stat !== null) {
					foundStatuses.push({
						network: net.name,
						stat: stat,
						statusText: statusMap[stat] || _('Unknown')
					});
					allStats.push(net.name + ':' + stat);
				}
			}
		});
		
		if (foundStatuses.length === 0) {
			return _('No registration data');
		}
		
		// 判断整体状态
		var overallStatus = calculateOverallStatus(foundStatuses);
		var statSummary = '(' + foundStatuses.map(n => n.network).join('/') + ': ' + 
						 Array.from(new Set(foundStatuses.map(n => n.stat))).join(',') + ')';
		
		return overallStatus + ' ' + statSummary;
	}

	function extractStatValue(regString) {
		var match = regString.match(/(\d),(\d)/);
		return match ? match[2] : null;
	}

	function calculateOverallStatus(statuses) {
		// 优先级：已注册/漫游 > 紧急呼叫 > 搜索中 > 未注册/拒绝/未知
		var hasRegistered = statuses.some(s => s.stat === '1' || s.stat === '5');
		var hasEmergency = statuses.some(s => s.stat === '8');
		var hasSearching = statuses.some(s => s.stat === '2');
		
		if (hasRegistered) {
			return _('Registered');
		}
		if (hasEmergency) {
			return _('Emergency');
		}
		if (hasSearching) {
			return _('Searching');
		}
		
		// 如果所有网络状态一致，显示该状态，否则显示主要问题状态
		var uniqueStats = Array.from(new Set(statuses.map(s => s.stat)));
		if (uniqueStats.length === 1) {
			var stat = uniqueStats[0];
			var statusMap = {
				'0': _('Not registered'),
				'3': _('Denied'),
				'4': _('Unknown')
			};
			return statusMap[stat] || _('Unknown');
		}
		
		return _('Not registered'); // 默认显示未注册
	}
	function getNetworkMode(cpsiValue) {
		if (!cpsiValue || cpsiValue === '') {
			return _('');
		}
		
		return parseDetailedNetworkMode(cpsiValue);
	}

	function parseDetailedNetworkMode(cpsiString) {
		if (cpsiString.toUpperCase() === 'NO SERVICE') {
			return _('No service');
		}
		
		var networkMap = {
			'NR5G_SA': '5G SA',
			'NR5G_NSA': '5G NSA',
			'NR5G': '5G',
			'NR_SA': '5G SA',
			'NR_NSA': '5G NSA',
			'NR': '5G',
			'LTE': '4G LTE',
			'WCDMA': '3G WCDMA',
			'UMTS': '3G UMTS', 
			'HSPA': '3G HSPA',
			'HSPA+': '3G HSPA+',
			'TD-SCDMA': '3G TD-SCDMA',
			'GSM': '2G GSM',
			'EDGE': '2G EDGE',
			'GPRS': '2G GPRS',
			'CDMA': '2G CDMA',
			'EVDO': '3G EVDO'
		};
		
		// 查找匹配的网络类型
		var foundNetworks = [];
		for (var key in networkMap) {
			if (cpsiString.toUpperCase().includes(key.toUpperCase())) {
				foundNetworks.push(networkMap[key]);
			}
		}
		
		if (foundNetworks.length === 0) {
			return cpsiString; // 返回原始信息
		}
		
		// 提取频段信息
		var bandMatch = cpsiString.match(/Band\s+([^,]+)/i);
		var bandInfo = bandMatch ? ' Band ' + bandMatch[1] : '';
		
		// 优先显示最高级的网络技术
		var priorityOrder = ['5G', '4G LTE', '3G HSPA+', '3G HSPA', '3G WCDMA', '3G UMTS', '2G'];
		for (var i = 0; i < priorityOrder.length; i++) {
			if (foundNetworks.includes(priorityOrder[i])) {
				return priorityOrder[i] + bandInfo;
			}
		}
		
		return foundNetworks[0] + bandInfo;
	}
	function getSIMStatus(cpin) {
		if (cpin === 'READY') {
			return _('SIM READY');
		}
		else if (cpin === 'SIM card not inserted') {
			return _('No card inserted');
		}
		else if (cpin === 'SIM IS BUSY') {
			return _('SIM is busy');
		}
		else if (cpin === 'SIM PIN') {
			return _('SIM PIN');
		}
		
		return _(cpin);
	}
    return baseclass.extend({
        title: _('Cellular Status'),
		load: function() {
			// 预加载UCI配置
			return Promise.all([
			L.resolveDefault(uci.load('modem')),
			initCellularData()
			]);
		},
        render: function(data) {
			var self = this;
        
        // 检查modem类型
        var modemType = uci.get('modem', '@default[0]', '.type');
        console.log(modemType);
        
        if(modemType != 'default') {
            return null;
        }
			
			var fields = [];
			fields = [_('SIM Card'), getSIMStatus(getCleanValue('CPIN')),
			_('Operator'), getOperator(getCleanValue('COPS'), getCleanValue('IMSI')),
			_('Registration Status'), getRegistrationStatus(),
			_('Network Modes'), getNetworkMode(getCleanValue('CPSI')),
			_('Frequency Band'), getCleanValue('BAND'),
			_('ARFCN'), getCleanValue('ARFCN'),
			_('PCI'), getCleanValue('PCI'),
			_('eNB'), getCleanValue('eNB'),
			_('Cell ID'), getCleanValue('cellID'),
			_('Signal Quality'), getSignalQuality(getCleanValue('SINR'), getCleanValue('RSRP'), getCleanValue('RSRQ'), getCleanValue('RSSI')), 
			_('Card Slot'), getCleanValue('SLOT') ? "SIM" + getCleanValue('SLOT') : '',
			_('IMEI'), getCleanValue('IMEI'), 
			_('IMSI'), getCleanValue('IMSI'),
			_('ICCID'), getCleanValue('CCID')];
            var table = E('table', {
                'class': 'table'
            });
            for (var i = 0; i < fields.length; i += 2) {
                table.appendChild(E('tr', {
                    'class': 'tr'
                }, [E('td', {
                    'class': 'td left',
                    'width': '45%'
                }, [fields[i]]), E('td', {
                    'class': 'td left',
					'width' : '50%'
                }, [(fields[i + 1] != null) ? fields[i + 1] : '?'])]));
            }
            return table;
			
        }
    });
