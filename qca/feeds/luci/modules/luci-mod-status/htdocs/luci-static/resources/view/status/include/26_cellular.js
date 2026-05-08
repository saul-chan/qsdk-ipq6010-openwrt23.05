'use strict';
'require baseclass';
'require fs';
'require rpc';
'require uci';

// 合并初始化函数，只读取一次文件
function initAllCellularData() {
    return fs.read('/var/state/cellular').then(function(content) {
        var data = {
            SIM: {},
            SIM1: {}, 
            SIM2: {},
            SIM3: {},
            SIM4: {}
        };
        
        if (content) {
            var lines = content.split('\n');
            lines.forEach(function(line) {
                // 匹配所有SIM卡数据
                var match = line.match(/^cellular\.(SIM|SIM1|SIM2|SIM3|SIM4)\.([^=]+)='([^']*)'$/);
                if (match) {
                    var simType = match[1];
                    var field = match[2];
                    var value = match[3].trim();
                    data[simType][field] = value;
                }
            });
        }
        return data;
    }).catch(function(error) {
        console.error('初始化cellular数据失败:', error);
        return { SIM: {}, SIM1: {}, SIM2: {}, SIM3: {}, SIM4: {} };
    });
}

function getField(cellularData, fieldName) {
    return cellularData[fieldName] || '';
}

function getCleanValue(cellularData, fieldName) {
    var value = cellularData[fieldName] || '';
    var match = value.match(/^\+[A-Za-z0-9]+:\s*(.*)$/);
    if (match && match[1]) {
        var cleaned = match[1].trim() || '';
        return cleaned || '';
    }
    else
        return '';
}

function getSignalQuality(cellularData, sinr, rsrp, rsrq, rssi) {
    var sval = parseInt(getCleanValue(cellularData, 'SVAL') || '0');
    var signalIcon = getSignalIcon(sval);
    
    var parts = [];
    
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
    
    if (parts.length === 0) {
        return _('No signal data available');
    }
    
    var signalText = parts.join(' / ');
    
    return E('div', { class: 'cellular-signal-with-icon', style: 'display: flex; flex-direction: row; align-items: flex-start;' }, [
        E('img', { 
            src: signalIcon, 
            class: 'cellular-signal-icon',
            title: _('Signal strength: ') + sval + '/5'
        }),
        E('span', { class: 'cellular-signal-text' }, '&#160' + signalText)
    ]);
}

function getSignalIcon(sval) {
    if (sval <= 0) {
        return L.resource('icons/signal-none.png');
    } else if (sval <= 2) {
        return L.resource('icons/signal-0-25.png');
    } else if (sval === 3) {
        return L.resource('icons/signal-25-50.png');
    } else if (sval === 4) {
        return L.resource('icons/signal-50-75.png');
    } else {
        return L.resource('icons/signal-75-100.png');
    }
}

function getOperator(cellularData, cops, imsi) {
    try {
        if (cops && cops !== '') {
            var operator = parseOperatorFromCops(cops);
            if (operator && operator !== _('Unknown Operator') && operator !== ('????')) {
                return operator;
            }
        }
        
        if (imsi && imsi !== '') {
            var operator = parseOperatorFromImsi(imsi);
            if (operator && operator !== _('Unknown Operator')) {
                return operator;
            }
        }
        
        return getFallbackOperator(cops, imsi);
    } catch (e) {
        console.error('Error getting operator:', e);
        return _('Unknown Operator');
    }
}

function parseOperatorFromCops(cops) {
    var operatorPatterns = [
        /(?:COPS:\s*)?\d+,\d+,([^,]+)/,
        /([A-Z]{3}-[A-Z]+)/,
        /(CHN-[A-Z]+)/,
        /(China[A-Z\s]+)/i
    ];
    
    for (var i = 0; i < operatorPatterns.length; i++) {
        var match = cops.match(operatorPatterns[i]);
        if (match && match[1]) {
            var rawName = match[1].trim();
            return mapChineseOperatorName(rawName);
        }
    }
    
    return null;
}

function parseOperatorFromImsi(imsi) {
    if (!imsi || imsi.length < 5) return null;
    
    var mnc = imsi.substring(0, 5);
    
    var operatorMap = {
        '46000': _('China Mobile'),
        '46002': _('China Mobile'),
        '46004': _('China Mobile'),
        '46007': _('China Mobile'),
        '46008': _('China Mobile'),
        '46009': _('China Mobile'),
        '46001': _('China Unicom'),
        '46006': _('China Unicom'),
        '46003': _('China Telecom'),
        '46005': _('China Telecom'),
        '46011': _('China Telecom'),
        '46020': _('China Tietong')
    };
    
    return operatorMap[mnc] || null;
}

function mapChineseOperatorName(rawName) {
    var nameMap = {
        'CHN-UNICOM': _('China Unicom'),
        'UNICOM': _('China Unicom'),
        'China Unicom': _('China Unicom'),
        'CHINA UNICOM': _('China Unicom'),
        'CHN-CT': _('China Telecom'),
        'CHN-TELECOM': _('China Telecom'),
        'China Telecom': _('China Telecom'),
        'CT': _('China Telecom'),
        'CHINA TELECOM': _('China Telecom'),
        'CHN-CMCC': _('China Mobile'),
        'CHN-MOBILE': _('China Mobile'),
        'China Mobile': _('China Mobile'),
        'CMCC': _('China Mobile'),
        'CM': _('China Mobile'),
        'CHINA MOBILE': _('China Mobile')
    };
    
    return nameMap[rawName] || rawName;
}

function getFallbackOperator(cops, imsi) {
    if (cops && cops !== '') {
        var operatorName = extractRawOperatorName(cops);
        if (operatorName) {
            return operatorName;
        }
    }
    
    if (imsi && imsi.length >= 5) {
        return 'IMSI:' + imsi.substring(0, 5);
    }
    
    return _('');
}

function extractRawOperatorName(cops) {
    var patterns = [
        /(?:COPS:\s*)?\d+,\d+,"([^"]+)"/,
        /(?:COPS:\s*)?\d+,\d+,([^,]+)/,
        /"([^"]+)"/,
        /([A-Z]{2,}[\w\s\-]+)/i
    ];
    
    for (var i = 0; i < patterns.length; i++) {
        var match = cops.match(patterns[i]);
        if (match && match[1]) {
            var name = match[1].trim();
            if (name.length > 2 && name.length < 50) {
                return name;
            }
        }
    }
    
    return null;
}

function getRegistrationStatus(cellularData) {
    var statusMap = {
        '0': _('Not registered'),
        '1': _('Registered'),
        '2': _('Searching'),
        '3': _('Denied'),
        '4': _('Unknown'),
        '5': _('Roaming'),
        '8': _('Emergency')
    };
    
    var networks = [
        { key: 'C5GREG', name: '5G' },
        { key: 'CEREG', name: '4G' },
        { key: 'CGREG', name: '3G/2G' },
    ];
    
    var foundStatuses = [];
    
    networks.forEach(function(net) {
        var value = getCleanValue(cellularData, net.key);
        if (value && value !== '') {
            var stat = extractStatValue(value);
            if (stat !== null) {
                foundStatuses.push({
                    network: net.name,
                    stat: stat,
                    statusText: statusMap[stat] || _('Unknown')
                });
            }
        }
    });
    
    if (foundStatuses.length === 0) {
        return _('No registration data');
    }
    
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
    
    return _('Not registered');
}

function getNetworkMode(cellularData, cpsiValue) {
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
    
    var foundNetworks = [];
    for (var key in networkMap) {
        if (cpsiString.toUpperCase().includes(key.toUpperCase())) {
            foundNetworks.push(networkMap[key]);
        }
    }
    
    if (foundNetworks.length === 0) {
        return cpsiString;
    }
    
    var bandMatch = cpsiString.match(/Band\s+([^,]+)/i);
    var bandInfo = bandMatch ? ' Band ' + bandMatch[1] : '';
    
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

// 检查SIM卡数据是否有效
function hasValidData(cellularData) {
    if (!cellularData) return false;
    
    // 检查是否有关键字段存在且不为空
    var criticalFields = ['CPIN', 'COPS', 'IMSI', 'CCID'];
    for (var i = 0; i < criticalFields.length; i++) {
        var value = cellularData[criticalFields[i]];
        if (value && value !== '' && value !== 'NULL' && value !== 'null') {
            return true;
        }
    }
    return false;
}

return baseclass.extend({
    title: _('Cellular Status'),
    load: function() {
        // 合并所有操作，只读取一次文件
        return Promise.all([
            L.resolveDefault(uci.load('modem')),
            initAllCellularData()
        ]);
    },
    render: function(data) {
        var self = this;
        
        // 检查modem类型
        var modemType = uci.get('modem', '@default[0]', '.type');
		//console.log(modemType)
        if(modemType != 'default') {
			modemType = uci.get('modem', 'SIM1', '.type');
			if(modemType != 'ndis') {
				return null;
			}
        }
        
        // 从合并的数据中获取
        var allData = data[1] || {};
        var simData = allData.SIM || {};
        var sim1Data = allData.SIM1 || {};
        var sim2Data = allData.SIM2 || {};
        var sim3Data = allData.SIM3 || {};
        var sim4Data = allData.SIM4 || {};
        
        // 检测有效的SIM卡
        var hasSIM = hasValidData(simData);
        var hasSIM1 = hasValidData(sim1Data);
        var hasSIM2 = hasValidData(sim2Data);
        var hasSIM3 = hasValidData(sim3Data);
        var hasSIM4 = hasValidData(sim4Data);
        
        /* console.log('SIM卡检测结果:', {
            SIM: hasSIM,
            SIM1: hasSIM1, 
            SIM2: hasSIM2,
            SIM3: hasSIM3,
            SIM4: hasSIM4
        }); */
        
        // 根据SIM卡数量决定显示方式
        if (hasSIM && !hasSIM1 && !hasSIM2 && !hasSIM3 && !hasSIM4) {
            return this.renderSingleTable(simData, 'SIM');
        } else {
            return this.renderFourTables(sim1Data, sim2Data, sim3Data, sim4Data);
        }
    },
    
    // 渲染单个表格
    renderSingleTable: function(simData, simName) {
        var fields = [
            _('SIM Card'), getSIMStatus(getCleanValue(simData, 'CPIN')),
            _('Operator'), getOperator(simData, getCleanValue(simData, 'COPS'), getCleanValue(simData, 'IMSI')),
            _('Registration Status'), getRegistrationStatus(simData),
            _('Network Modes'), getNetworkMode(simData, getCleanValue(simData, 'CPSI')),
            _('Frequency Band'), getCleanValue(simData, 'BAND'),
            _('ARFCN'), getCleanValue(simData, 'ARFCN'),
            _('PCI'), getCleanValue(simData, 'PCI'),
            _('eNB'), getCleanValue(simData, 'eNB'),
            _('Cell ID'), getCleanValue(simData, 'cellID'),
            _('Signal Quality'), getSignalQuality(simData, getCleanValue(simData, 'SINR'), getCleanValue(simData, 'RSRP'), getCleanValue(simData, 'RSRQ'), getCleanValue(simData, 'RSSI')), 
            _('Card Slot'), getCleanValue(simData, 'SLOT') ? "SIM" + getCleanValue(simData, 'SLOT') : '',
            _('IMEI'), getCleanValue(simData, 'IMEI'), 
            _('IMSI'), getCleanValue(simData, 'IMSI'),
            _('ICCID'), getCleanValue(simData, 'CCID')
        ];
        
        var table = E('table', {
            'class': 'table',
            'style': 'width: 100%;'
        });
        
        for (var i = 0; i < fields.length; i += 2) {
            table.appendChild(E('tr', {
                'class': 'tr'
            }, [E('td', {
                'class': 'td left',
                'width': '45%'
            }, [fields[i]]), E('td', {
                'class': 'td left',
                'width': '50%'
            }, [(fields[i + 1] != null) ? fields[i + 1] : '?'])]));
        }
        
        return table;
    },
    
    // 渲染四个表格：SIM1和SIM2并排，SIM3和SIM4并排
    renderFourTables: function(sim1Data, sim2Data, sim3Data, sim4Data) {
        // 创建主容器
        var mainContainer = E('div', {
            'class': 'four-sim-container',
            'style': 'display: flex; flex-direction: column; gap: 20px;'
        });

        // 第一行：SIM1和SIM2并排
        var firstRow = E('div', {
            'class': 'sim-row',
            'style': 'display: flex; gap: 20px; flex-wrap: wrap;'
        });

        // 第二行：SIM3和SIM4并排
        var secondRow = E('div', {
            'class': 'sim-row',
            'style': 'display: flex; gap: 20px; flex-wrap: wrap;'
        });

        // 为每个SIM卡创建表格
        if (hasValidData(sim1Data)) {
            firstRow.appendChild(this.createSimTable(sim1Data, _('SIM Card 1')));
        }
        
        if (hasValidData(sim2Data)) {
            firstRow.appendChild(this.createSimTable(sim2Data, _('SIM Card 2')));
        }
        
        if (hasValidData(sim3Data)) {
            secondRow.appendChild(this.createSimTable(sim3Data, _('SIM Card 3')));
        }
        
        if (hasValidData(sim4Data)) {
            secondRow.appendChild(this.createSimTable(sim4Data, _('SIM Card 4')));
        }

        // 只有当有内容时才添加行
        if (firstRow.childNodes.length > 0) {
            mainContainer.appendChild(firstRow);
        }
        
        if (secondRow.childNodes.length > 0) {
            mainContainer.appendChild(secondRow);
        }

        // 如果没有有效的SIM卡数据，显示警告
        if (mainContainer.childNodes.length === 0) {
            return E('div', { class: 'alert-message warning' }, 
                _('No valid SIM card data available'));
        }

        return mainContainer;
    },

    // 创建单个SIM卡表格的辅助方法
    createSimTable: function(simData, title) {
        var fields = [
            title, getSIMStatus(getCleanValue(simData, 'CPIN')),
            _('Operator'), getOperator(simData, getCleanValue(simData, 'COPS'), getCleanValue(simData, 'IMSI')),
            _('Registration Status'), getRegistrationStatus(simData),
            _('Network Modes'), getNetworkMode(simData, getCleanValue(simData, 'CPSI')),
            _('Frequency Band'), getCleanValue(simData, 'BAND'),
            _('ARFCN'), getCleanValue(simData, 'ARFCN'),
            _('PCI'), getCleanValue(simData, 'PCI'),
            _('eNB'), getCleanValue(simData, 'eNB'),
            _('Cell ID'), getCleanValue(simData, 'cellID'),
            _('Signal Quality'), getSignalQuality(simData, getCleanValue(simData, 'SINR'), getCleanValue(simData, 'RSRP'), getCleanValue(simData, 'RSRQ'), getCleanValue(simData, 'RSSI')), 
            _('IMEI'), getCleanValue(simData, 'IMEI'), 
            _('IMSI'), getCleanValue(simData, 'IMSI'),
            _('ICCID'), getCleanValue(simData, 'CCID')
        ];
        
        var table = E('table', {
            'class': 'table',
            'style': 'flex: 1; min-width: 300px;'
        });
        
        for (var i = 0; i < fields.length; i += 2) {
            table.appendChild(E('tr', {
                'class': 'tr'
            }, [E('td', {
                'class': 'td left',
                'width': '45%'
            }, [fields[i]]), E('td', {
                'class': 'td left',
                'width': '50%'
            }, [(fields[i + 1] != null) ? fields[i + 1] : '?'])]));
        }
        
        return table;
    }
});