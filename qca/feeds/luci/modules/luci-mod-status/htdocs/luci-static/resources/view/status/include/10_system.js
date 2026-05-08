    'use strict';
    'require baseclass';
    'require fs';
    'require rpc';
    'require uci';
	var callLuciVersion = rpc.declare({
        object: 'luci',
        method: 'getVersion'
    });
    var callSystemBoard = rpc.declare({
        object: 'system',
        method: 'board'
    });
    var callSystemInfo = rpc.declare({
        object: 'system',
        method: 'info'
    });
    function getModel() {
        try {
            uci.load('system');
            var model = uci.get('system', '@system[0]', 'model');
            return model || '';
        } catch (e) {
            console.error('Failed to get model:', e);
            return '';
        }
    }
    function getF_ver() {
        try {
            uci.load('system');
            var f_ver = uci.get('system', '@system[0]', 'F_ver');
            return f_ver || '';
        } catch (e) {
            console.error('Failed to get F_ver:', e);
            return '';
        }
    }
    function getH_ver() {
        try {
            uci.load('system');
            var h_ver = uci.get('system', '@system[0]', 'H_ver');
            return h_ver || '';
        } catch (e) {
            console.error('Failed to get H_ver:', e);
            return '';
        }
    }
	function getCellModel() {
		try {
			uci.load('system');
			var simModel = uci.get('system', '@system[0]', 'module_name');
			var sim1Model = uci.get('system', '@system[0]', 'SIM1_name');
			var sim2Model = uci.get('system', '@system[0]', 'SIM2_name');
			var sim3Model = uci.get('system', '@system[0]', 'SIM3_name');
			var sim4Model = uci.get('system', '@system[0]', 'SIM4_name');
			
			uci.load('luci');
			var theme = uci.get('luci', 'main', 'mediaurlbase');
			if (simModel && theme && theme.includes('slk')) {
				simModel = 'SLK' + simModel;
			}
			// 对两个SIM卡型号分别处理
			if (sim1Model && theme && theme.includes('slk')) {
				sim1Model = 'SLK' + sim1Model;
			}
			if (sim2Model && theme && theme.includes('slk')) {
				sim2Model = 'SLK' + sim2Model;
			}
			if (sim3Model && theme && theme.includes('slk')) {
				sim3Model = 'SLK' + sim3Model;
			}
			if (sim4Model && theme && theme.includes('slk')) {
				sim4Model = 'SLK' + sim4Model;
			}
			
			// 返回格式：SIM1型号 / SIM2型号
			var result = '';
			if (simModel) result += simModel;
			if (sim1Model) result += 'CELL1: ' + sim1Model;
			if (sim1Model && sim2Model) result += ' / CELL2: ';
			if (sim2Model) result += sim2Model;
			if (sim2Model && sim3Model) result += ' / CELL3: ';
			if (sim3Model) result += sim3Model;
			if (sim3Model && sim4Model) result += ' / CELL4: ';
			if (sim4Model) result += sim4Model;
			return result || '';
		} catch (e) {
			console.error('Failed to get model:', e);
			return '';
		}
	}
	function getCell_ver() {
		try {
			uci.load('system');
			var simVersion = uci.get('system', '@system[0]', 'module_version');
			var sim1Version = uci.get('system', '@system[0]', 'SIM1_version');
			var sim2Version = uci.get('system', '@system[0]', 'SIM2_version');
			var sim3Version = uci.get('system', '@system[0]', 'SIM3_version');
			var sim4Version = uci.get('system', '@system[0]', 'SIM4_version');
			
			// 返回格式：SIM1版本 / SIM2版本
			var result = '';
			if (simVersion) result += simVersion;
			if (sim1Version) result +=  'CELL1: ' + sim1Version;
			if (sim1Version && sim2Version) result += ' / CELL2: ';
			if (sim2Version) result += sim2Version;
			if (sim2Version && sim3Version) result += ' / CELL3: ';
			if (sim3Version) result += sim3Version;
			if (sim3Version && sim4Version) result += ' / CELL4: ';
			if (sim4Version) result += sim4Version;
			return result || '';
		} catch (e) {
			console.error('Failed to get version:', e);
			return '';
		}
	}
	function getAllSimTemps() {
		return Promise.all([
			fs.exec_direct('/sbin/uci', ['-P', '/var/state', '-q', 'get', 'cellular.SIM.TEMP']),
			fs.exec_direct('/sbin/uci', ['-P', '/var/state', '-q', 'get', 'cellular.SIM1.TEMP']),
			fs.exec_direct('/sbin/uci', ['-P', '/var/state', '-q', 'get', 'cellular.SIM2.TEMP']),
			fs.exec_direct('/sbin/uci', ['-P', '/var/state', '-q', 'get', 'cellular.SIM3.TEMP']),
			fs.exec_direct('/sbin/uci', ['-P', '/var/state', '-q', 'get', 'cellular.SIM4.TEMP'])
		]).then(function(temps) {
			var simTemps = [];
			
			// 处理每个温度值，提取数字部分
			temps.forEach(function(temp, index) {
				if (temp && temp.trim()) {
					var match = temp.trim().match(/^\+TEMP:\s*(\d+)\s*$/);
					if (match && match[1]) {
						simTemps.push({
							name: index === 0 ? 'CELL' : 'CELL' + index,
							value: match[1] + '°C'
						});
					}
				}
			});
			
			return simTemps;
		}).catch(function(error) {
			console.error('读取SIM卡温度失败:', error);
			return [];
		});
	}
	function extractTemperature(data) {
		const match = data.match(/sensor temperature:\s*(\d+)/);
		return match ? parseInt(match[1], 10) : null;
	}
	function preprocessCpuTemperature(content) {
		if (!content) return 'N/A';
		
		var temp_str = String(content).trim();
		if (!temp_str.match(/^\d+$/)) return 'N/A';
		
		var temp_num = parseInt(temp_str, 10);
		
		// 专门处理CPU温度（通常都是毫摄氏度）
		if (temp_num > 1000) {
			return (temp_num / 1000).toFixed(1); // 转换为摄氏度，保留1位小数
		} else if (temp_num >= 20 && temp_num <= 120) {
			return temp_num.toString(); // 已经是摄氏度
		} else {
			return 'N/A'; // 数值不合理
		}
	}
	// 构建温度显示的函数
	function buildTemperatureDisplay(cpu, wifi0, wifi1, simTemps) {
		var parts = [];
		
		// CPU温度
		if (cpu && cpu !== 'N/A') {
			parts.push('CPU: (' + cpu + '°C)');
		}
		
		// WiFi温度
		var wifiParts = [];
		if (wifi0) {
			wifiParts.push('WIFI-2.4G: (' + wifi0 + '°C)');
		}
		if (wifi1) {
			wifiParts.push('WIFI-5.8G: (' + wifi1 + '°C)');
		}
		
		if (wifiParts.length > 0) {
			parts.push(wifiParts.join(' / '));
		}
		
		// SIM卡温度 - 根据数组数量动态添加
		if (simTemps && simTemps.length > 0) {
			var cellParts = [];
			simTemps.forEach(function(simTemp) {
				cellParts.push(simTemp.name + ': (' + simTemp.value + ')');
			});
			parts.push(cellParts.join(' / '));
		}
		
		return parts.length > 0 ? parts.join('   ') : 'N/A';
	}
    return baseclass.extend({
        title: _('System'),
        load: function() {
            return Promise.all([
			L.resolveDefault(callSystemBoard(), {}),
			L.resolveDefault(callSystemInfo(), {}),
			getF_ver(),
			getH_ver(),
			fs.exec_direct('/usr/sbin/slktool', ['/dev/mmcblk0p25', 'sn']),
			fs.trimmed('/sys/class/net/wifi0/thermal/temp'),
			fs.trimmed('/sys/class/net/wifi1/thermal/temp'),
			fs.trimmed('/sys/class/thermal/thermal_zone0/temp'),
			getAllSimTemps()
			]);
        },
        render: function(data) {
            var boardinfo = data[0]
              , systeminfo = data[1];
			var wifi0_temp = data[5];
			var wifi1_temp = data[6];
			var cpu_temp = preprocessCpuTemperature(data[7]);
			var sn = data[4];
			sn = sn.substring(sn.indexOf(':') + 1).trim();
			var simTemps = data[8]; 
            var datestr = null;
            if (systeminfo.localtime) {
                var date = new Date(systeminfo.localtime * 1000);
                datestr = '%04d-%02d-%02d %02d:%02d:%02d'.format(date.getUTCFullYear(), date.getUTCMonth() + 1, date.getUTCDate(), date.getUTCHours(), date.getUTCMinutes(), date.getUTCSeconds());
            }
			uci.load('luci');
			var theme = uci.get('luci', 'main', 'mediaurlbase');
			//console.log(theme);
            var fields = [
			_('Hostname'), boardinfo.hostname,
			...(theme.includes('zx') ? [] : [_('Model'), getModel()]),
			_('Firmware Version'), getF_ver(),
			_('Hardware Version'), getH_ver(),
			_('Serial Number'), sn ? sn : '',
			_('Cellular Model'), getCellModel(),
			_('Cellular Version'), getCell_ver(),
			_('Local Time'), datestr,
			_('Uptime'), systeminfo.uptime ? '%t'.format(systeminfo.uptime) : null,
			_('Load Average'), Array.isArray(systeminfo.load) ? '%.2f, %.2f, %.2f'.format(systeminfo.load[0] / 65535.0, systeminfo.load[1] / 65535.0, systeminfo.load[2] / 65535.0) : null,
			_('Temperature'), buildTemperatureDisplay(cpu_temp, wifi0_temp, wifi1_temp, simTemps)
			
			];
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
