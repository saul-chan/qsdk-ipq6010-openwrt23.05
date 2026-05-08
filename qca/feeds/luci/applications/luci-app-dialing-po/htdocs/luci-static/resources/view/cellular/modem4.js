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
var config_name='SIM4'
var callSystemBoard = rpc.declare({
	object: 'system',
	method: 'board'
});

function count_changes(ifname) {
    return 0;
}
function callModemInfo() {
    try {
        uci.load('system');
        var modem_name = uci.get('system', '@system[0]', config_name + '_name');
        var modem_ver = uci.get('system', '@system[0]', config_name + '_version');
        
        uci.load('modem');
		var modem_type = uci.get('modem', config_name, 'net');
		//console.log("UCI获取结果:", modem_name, modem_ver, modem_type);
        return {
            modem_name: modem_name || 'unknown',
            modem_version: modem_ver || 'unknown',
            modem_type: modem_type || 'unknown'
        };
        
    } catch (e) {
       // console.error("获取modem信息出错:", e);
        return {
            modem_name: 'error',
            modem_version: 'error',
            modem_type: 'error'
        };
    }
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

function addBandOptions(sec, modem_name, modem_version) {
    var o;
    
    // 1. 650模块的判断
    if (modem_name && modem_name.match(/650/i)) {
        // 检查版本号倒数第二位是否为17
        var versionPart = "";
        if (modem_version) {
            var parts = modem_version.split('.');
            if (parts.length >= 2) {
                versionPart = parts[parts.length - 2];
            }
        }
        
        if (versionPart === '17') {
            // 版本17的频段
            o = sec.taboption('advanced', form.ListValue, 'band_wcdma', _('Lock 3G Band List'));
            o.value('0', _('Automatically'));
            o.value('1', _('BAND 1'));
            o.value('5', _('BAND 5'));
            o.value('8', _('BAND 8'));
            o.depends('smode', '1');
            o.depends('smode', '3');
            o.depends('smode', '7');
            
            o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
            o.value('0', _('Automatically'));
            o.value('101', _('BAND 1'));
            o.value('103', _('BAND 3'));
            o.value('105', _('BAND 5'));
            o.value('108', _('BAND 8'));
            o.value('134', _('BAND 34'));
            o.value('138', _('BAND 38'));
            o.value('139', _('BAND 39'));
            o.value('140', _('BAND 40'));
            o.value('141', _('BAND 41'));
            o.depends('smode', '2');
            o.depends('smode', '3');
            o.depends('smode', '6');
            o.depends('smode', '7');
            
            o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
            o.value('0', _('Automatically'));
            o.value('501', _('N1'));
            o.value('505', _('N5'));
            o.value('508', _('N8'));
            o.value('5028', _('N28'));
            o.value('5041', _('N41'));
            o.value('5078', _('N78'));
            o.value('5079', _('N79'));
            o.depends('smode', '4');
            o.depends('smode', '6');
            o.depends('smode', '7');
        } else {
            // 其他版本的频段
            o = sec.taboption('advanced', form.ListValue, 'band_wcdma', _('Lock 3G Band List'));
            o.value('0', _('Automatically'));
            o.value('1', _('BAND 1'));
            o.value('2', _('BAND 2'));
            o.value('5', _('BAND 5'));
            o.value('8', _('BAND 8'));
            o.depends('smode', '1');
            o.depends('smode', '3');
            o.depends('smode', '7');
            
            o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
            o.value('0', _('Automatically'));
            o.value('101', _('BAND 1'));
            o.value('102', _('BAND 2'));
            o.value('103', _('BAND 3'));
            o.value('105', _('BAND 5'));
            o.value('107', _('BAND 7'));
            o.value('108', _('BAND 8'));
            o.value('134', _('BAND 34'));
            o.value('138', _('BAND 38'));
            o.value('139', _('BAND 39'));
            o.value('140', _('BAND 40'));
            o.value('141', _('BAND 41'));
            o.depends('smode', '2');
            o.depends('smode', '3');
            o.depends('smode', '6');
            o.depends('smode', '7');
            
            o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
            o.value('0', _('Automatically'));
            o.value('501', _('N1'));
            o.value('5028', _('N28'));
            o.value('5041', _('N41'));
            o.value('5078', _('N78'));
            o.value('5079', _('N79'));
            o.depends('smode', '4');
            o.depends('smode', '6');
            o.depends('smode', '7');
        }
        
        // 公共选项
        o = sec.taboption('advanced', form.Value, 'arfcn', _('Cell Frequency(ARFCN)'));
        o.depends('smode', '2');
        o.depends('smode', '4');
        o.placeholder = _('[range: 0~16777215]');
        
        o = sec.taboption('advanced', form.Value, 'pci', _('Cell Physical ID(PCI)'));
        o.depends('smode', '2');
        o.placeholder = _('[range: 0~503]');
        
        o = sec.taboption('advanced', form.Value, 'psc', _('Cell Physical ID(PCI)'));
        o.depends('smode', '4');
        o.placeholder = _('[range: 0~1007]');
    }
    
	// 2. 160模块
	else if (modem_name && modem_name.match(/160/i)) {
		// 160-CN的频段选项...
		if (modem_name.match(/160-CN/i)) {
			// 检查版本号倒数第三位是否为04
			var versionPart = '';
			if (modem_version) {
				var parts = modem_version.split('.');
				if (parts.length >= 3) {
					versionPart = parts[parts.length - 3];
				}
			}
			
			if (versionPart === '04') {
				// 版本04的频段...
				o = sec.taboption('advanced', form.ListValue, 'band_wcdma', _('Lock 3G Band List'));
				o.value('0', _('Automatically'));
				o.value('1', _('BAND 1'));
				o.value('8', _('BAND 8'));
				o.depends('smode', '1');
				o.depends('smode', '3');
				o.depends('smode', '7');
				
				o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
				o.value('0', _('Automatically'));
				o.value('101', _('BAND 1'));
				o.value('103', _('BAND 3'));
				o.value('105', _('BAND 5'));
				o.value('108', _('BAND 8'));
				o.value('134', _('BAND 34'));
				o.value('138', _('BAND 38'));
				o.value('139', _('BAND 39'));
				o.value('140', _('BAND 40'));
				o.value('141', _('BAND 41'));
				o.depends('smode', '2');
				o.depends('smode', '3');
				o.depends('smode', '6');
				o.depends('smode', '7');
				
				o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
				o.value('0', _('Automatically'));
				o.value('501', _('N1'));
				o.value('5028', _('N28'));
				o.value('5041', _('N41'));
				o.value('5078', _('N78'));
				o.value('5079', _('N79'));
				o.depends('smode', '4');
				o.depends('smode', '6');
				o.depends('smode', '7');
			} else {
				// 其他版本的频段...
				o = sec.taboption('advanced', form.ListValue, 'band_wcdma', _('Lock 3G Band List'));
				o.value('0', _('Automatically'));
				o.value('1', _('BAND 1'));
				o.value('5', _('BAND 5'));
				o.value('8', _('BAND 8'));
				o.depends('smode', '1');
				o.depends('smode', '3');
				o.depends('smode', '7');
				
				o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
				o.value('0', _('Automatically'));
				o.value('101', _('BAND 1'));
				o.value('103', _('BAND 3'));
				o.value('105', _('BAND 5'));
				o.value('108', _('BAND 8'));
				o.value('134', _('BAND 34'));
				o.value('138', _('BAND 38'));
				o.value('139', _('BAND 39'));
				o.value('140', _('BAND 40'));
				o.value('141', _('BAND 41'));
				o.depends('smode', '2');
				o.depends('smode', '3');
				o.depends('smode', '6');
				o.depends('smode', '7');
				
				o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
				o.value('0', _('Automatically'));
				o.value('501', _('N1'));
				o.value('505', _('N5'));
				o.value('508', _('N8'));
				o.value('5028', _('N28'));
				o.value('5041', _('N41'));
				o.value('5077', _('N77'));
				o.value('5078', _('N78'));
				o.value('5079', _('N79'));
				o.depends('smode', '4');
				o.depends('smode', '6');
				o.depends('smode', '7');
			}
		}
		// 160-EAU的频段选项...
		else if (modem_name.match(/160-EAU/i)) {
			o = sec.taboption('advanced', form.ListValue, 'band_wcdma', _('Lock 3G Band List'));
			o.value('0', _('Automatically'));
			o.value('1', _('BAND 1'));
			o.value('5', _('BAND 5'));
			o.value('8', _('BAND 8'));
			o.depends('smode', '1');
			o.depends('smode', '3');
			o.depends('smode', '7');
			
			o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
			o.value('0', _('Automatically'));
			o.value('101', _('BAND 1'));
			o.value('103', _('BAND 3'));
			o.value('105', _('BAND 5'));
			o.value('107', _('BAND 7'));
			o.value('108', _('BAND 8'));
			o.value('120', _('BAND 20'));
			o.value('128', _('BAND 28'));
			o.value('132', _('BAND 32'));
			o.value('138', _('BAND 38'));
			o.value('140', _('BAND 40'));
			o.value('141', _('BAND 41'));
			o.value('142', _('BAND 42'));
			o.value('143', _('BAND 43'));
			o.value('146', _('BAND 46'));
			o.depends('smode', '2');
			o.depends('smode', '3');
			o.depends('smode', '6');
			o.depends('smode', '7');
			
			o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
			o.value('0', _('Automatically'));
			o.value('501', _('N1'));
			o.value('503', _('N3'));
			o.value('505', _('N5'));
			o.value('507', _('N7'));
			o.value('508', _('N8'));
			o.value('5020', _('N20'));
			o.value('5028', _('N28'));
			o.value('5041', _('N41'));
			o.value('5075', _('N75'));
			o.value('5076', _('N76'));
			o.value('5077', _('N77'));
			o.value('5078', _('N78'));
			o.depends('smode', '4');
			o.depends('smode', '6');
			o.depends('smode', '7');
		}
		// 160-NA的频段选项...
		else if (modem_name.match(/160-NA/i)) {
			
			o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
			o.value('0', _('Automatically'));
			o.value('102', _('BAND 2'));
			o.value('104', _('BAND 4'));
			o.value('105', _('BAND 5'));
			o.value('112', _('BAND 12'));
			o.value('113', _('BAND 13'));
			o.value('114', _('BAND 14'));
			o.value('128', _('BAND 29'));
			o.value('130', _('BAND 30'));
			o.value('141', _('BAND 41'));
			o.value('146', _('BAND 46'));
			o.value('148', _('BAND 48'));
			o.value('166', _('BAND 66'));
			o.value('171', _('BAND 77'));
			o.depends('smode', '2');
			o.depends('smode', '3');
			o.depends('smode', '6');
			o.depends('smode', '7');
			
			o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
			o.value('0', _('Automatically'));
			o.value('502', _('N2'));
			o.value('505', _('N5'));
			o.value('5012', _('N12'));
			o.value('5014', _('N14'));
			o.value('5025', _('N25'));
			o.value('5030', _('N30'));
			o.value('5041', _('N41'));
			o.value('5048', _('N48'));
			o.value('5066', _('N66'));
			o.value('5070', _('N70'));
			o.value('5071', _('N71'));
			o.value('5077', _('N77'));
			o.depends('smode', '4');
			o.depends('smode', '6');
			o.depends('smode', '7');
		}
		// 160-JK的频段选项...
		else if (modem_name.match(/160-JK/i)) {
			
			o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
			o.value('0', _('Automatically'));
			o.value('101', _('BAND 1'));
			o.value('103', _('BAND 3'));
			o.value('105', _('BAND 5'));
			o.value('107', _('BAND 7'));
			o.value('108', _('BAND 8'));
			o.value('118', _('BAND 18'));
			o.value('119', _('BAND 19'));
			o.value('126', _('BAND 26'));
			o.value('128', _('BAND 28'));
			o.value('139', _('BAND 39'));
			o.value('141', _('BAND 41'));
			o.value('142', _('BAND 42'));
			o.depends('smode', '2');
			o.depends('smode', '3');
			o.depends('smode', '6');
			o.depends('smode', '7');
			
			o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
			o.value('0', _('Automatically'));
			o.value('501', _('N1'));
			o.value('503', _('N3'));
			o.value('507', _('N7'));
			o.value('508', _('N8'));
			o.value('5028', _('N28'));
			o.value('5041', _('N41'));
			o.value('5077', _('N77'));
			o.value('5078', _('N78'));
			o.value('5079', _('N79'));
			o.depends('smode', '4');
			o.depends('smode', '6');
			o.depends('smode', '7');
		}
		
		// 公共选项
		o = sec.taboption('advanced', form.Value, 'arfcn', _('Cell Frequency(ARFCN)'));
		o.depends('smode', '2');
		o.depends('smode', '4');
		o.placeholder = _('[range: 0~4294967295]');
			
		o = sec.taboption('advanced', form.Value, 'pci', _('Cell Physical ID(PCI)'));
		o.depends('smode', '2');
		o.placeholder = _('[range: 0~503]');
			
		o = sec.taboption('advanced', form.Value, 'psc', _('Cell Physical ID(PCI)'), _('To lock a cell, specify the frequency band at the same time'));
		o.depends('smode', '4');
		o.placeholder = _('[range: 0~1007]');
			
		o = sec.taboption('advanced', form.ListValue, 'scs', _('NR Sub Carrier Space(SCS)'));
		o.value('0', _('15kHz(FDD)'));
		o.value('1', _('30kHz(TDD)'));
		o.default = '1';
		o.depends('smode', '4');
    }
    
    // 5. 668模块系列
    else if (modem_name && modem_name.match(/668/i)) {
        if (modem_name.match(/668-CN/i)) {
            o = sec.taboption('advanced', form.ListValue, 'band_gsm', _('Lock GSM Band'));
            o.default = '0';
            o.value('0', _('Automatically'));
            o.value('900', _('900 MHz Band'));
            o.value('1800', _('1800 MHz Band'));
            o.depends('smode', '1');
            o.depends('smode', '4');
            o.depends('smode', '6');
            o.depends('smode', '7');
            
            o = sec.taboption('advanced', form.ListValue, 'band_umts', _('Lock UMTS Band'));
            o.default = '0';
            o.value('0', _('Automatically'));
            o.value('1', _('BAND_UMTS_I'));
            o.value('8', _('BAND_UMTS_VIII'));
            o.depends('smode', '2');
            o.depends('smode', '4');
            o.depends('smode', '5');
            o.depends('smode', '7');
			
			o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock LTE Band'));
			o.default = '0';
			o.value('0', _('Automatically'));
			o.value('101', _('BAND 1'));
			o.value('103', _('BAND 3'));
			o.value('105', _('BAND 5'));
			o.value('108', _('BAND 8'));
			o.value('134', _('BAND 34'));
			o.value('138', _('BAND 38'));
			o.value('139', _('BAND 39'));
			o.value('140', _('BAND 40'));
			o.value('141', _('BAND 41'));
			o.depends('smode', '3');
			o.depends('smode', '5');
			o.depends('smode', '6');
			o.depends('smode', '7');
			
			o = sec.taboption('advanced', form.ListValue, 'band_scdma', _('Lock TD-SCDMA Band'));
            o.default = '0';
            o.value('0', _('Automatically'));
            o.value('201', _('BAND_UMTS_TDD_A'));
            o.value('206', _('BAND_UMTS_TDD_F'));
            o.depends('smode', '8');
			
			o = sec.taboption('advanced', form.ListValue, 'band_cdma', _('Lock CDMA or EVDO Band'));
            o.default = '0';
            o.value('0', _('Automatically'));
            o.value('300', _('BAND_BC0'));
            o.depends('smode', '11');
            o.depends('smode', '12');
            o.depends('smode', '13');
			
        } else if (modem_name.match(/668-LA/i)) {
            o = sec.taboption('advanced', form.ListValue, 'band_gsm', _('Lock GSM Band'));
            o.default = '0';
            o.value('0', _('Automatically'));
            o.value('850', _('850 MHz Band'));
            o.value('900', _('900 MHz Band'));
            o.value('1800', _('1800 MHz Band'));
            o.value('1900', _('1900 MHz Band'));
            o.depends('smode', '1');
            o.depends('smode', '4');
            o.depends('smode', '6');
            o.depends('smode', '7');
            
            o = sec.taboption('advanced', form.ListValue, 'band_umts', _('Lock UMTS Band'));
            o.default = '0';
            o.value('0', _('Automatically'));
            o.value('1', _('BAND_UMTS_I'));
            o.value('2', _('BAND_UMTS_II'));
            o.value('3', _('BAND_UMTS_III'));
            o.value('4', _('BAND_UMTS_IV'));
            o.value('5', _('BAND_UMTS_V'));
            o.value('8', _('BAND_UMTS_VIII'));
            o.depends('smode', '2');
            o.depends('smode', '4');
            o.depends('smode', '5');
            o.depends('smode', '7');
			
			o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock LTE Band'));
			o.default = '0';
			o.value('0', _('Automatically'));
			o.value('101', _('BAND 1'));
			o.value('102', _('BAND 2'));
			o.value('103', _('BAND 3'));
			o.value('104', _('BAND 4'));
			o.value('105', _('BAND 5'));
			o.value('107', _('BAND 7'));
			o.value('108', _('BAND 8'));
			o.value('112', _('BAND 12'));
			o.value('117', _('BAND 17'));
			o.value('128', _('BAND 28'));
			o.value('138', _('BAND 38'));
			o.value('140', _('BAND 40'));
			o.value('166', _('BAND 66'));
			o.depends('smode', '3');
			o.depends('smode', '5');
			o.depends('smode', '6');
			o.depends('smode', '7');
        } else if (modem_name.match(/668-EU/i)) {
            o = sec.taboption('advanced', form.ListValue, 'band_gsm', _('Lock GSM Band'));
            o.default = '0';
            o.value('0', _('Automatically'));
            o.value('850', _('850 MHz Band'));
            o.value('900', _('900 MHz Band'));
            o.value('1800', _('1800 MHz Band'));
            o.depends('smode', '1');
            o.depends('smode', '4');
            o.depends('smode', '6');
            o.depends('smode', '7');
            
            o = sec.taboption('advanced', form.ListValue, 'band_umts', _('Lock UMTS Band'));
            o.default = '0';
            o.value('0', _('Automatically'));
            o.value('1', _('BAND_UMTS_I'));
            o.value('5', _('BAND_UMTS_V'));
            o.value('8', _('BAND_UMTS_VIII'));
            o.depends('smode', '2');
            o.depends('smode', '4');
            o.depends('smode', '5');
            o.depends('smode', '7');
			
			o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock LTE Band'));
			o.default = '0';
			o.value('0', _('Automatically'));
			o.value('101', _('BAND 1'));
			o.value('103', _('BAND 3'));
			o.value('105', _('BAND 5'));
			o.value('107', _('BAND 7'));
			o.value('108', _('BAND 8'));
			o.value('120', _('BAND 20'));
			o.depends('smode', '3');
			o.depends('smode', '5');
			o.depends('smode', '6');
			o.depends('smode', '7');
        } else if (modem_name.match(/668-EAU/i)) {
            o = sec.taboption('advanced', form.ListValue, 'band_gsm', _('Lock GSM Band'));
            o.default = '0';
            o.value('0', _('Automatically'));
            o.value('850', _('850 MHz Band'));
            o.value('900', _('900 MHz Band'));
            o.value('1800', _('1800 MHz Band'));
            o.depends('smode', '1');
            o.depends('smode', '4');
            o.depends('smode', '6');
            o.depends('smode', '7');
            
            o = sec.taboption('advanced', form.ListValue, 'band_umts', _('Lock UMTS Band'));
            o.default = '0';
            o.value('0', _('Automatically'));
            o.value('1', _('BAND_UMTS_I'));
            o.value('5', _('BAND_UMTS_V'));
            o.value('8', _('BAND_UMTS_VIII'));
            o.depends('smode', '2');
            o.depends('smode', '4');
            o.depends('smode', '5');
            o.depends('smode', '7');
			
			o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock LTE Band'));
			o.default = '0';
			o.value('0', _('Automatically'));
			o.value('101', _('BAND 1'));
			o.value('103', _('BAND 3'));
			o.value('105', _('BAND 5'));
			o.value('107', _('BAND 7'));
			o.value('108', _('BAND 8'));
			o.value('120', _('BAND 20'));
			o.value('128', _('BAND 28'));
			o.value('138', _('BAND 38'));
			o.value('140', _('BAND 40'));
			o.value('141', _('BAND 41'));
			o.depends('smode', '3');
			o.depends('smode', '5');
			o.depends('smode', '6');
			o.depends('smode', '7');
        } else if (modem_name.match(/668-JP/i)) {
            o = sec.taboption('advanced', form.ListValue, 'band_umts', _('Lock UMTS Band'));
            o.default = '0';
            o.value('0', _('Automatically'));
            o.value('1', _('BAND_UMTS_I'));
            o.value('6', _('BAND_UMTS_VI'));
            o.value('8', _('BAND_UMTS_VIII'));
            o.value('19', _('BAND_UMTS_XIX'));
            o.depends('smode', '2');
            o.depends('smode', '4');
            o.depends('smode', '5');
            o.depends('smode', '7');
			
			o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock LTE Band'));
			o.default = '0';
			o.value('0', _('Automatically'));
			o.value('101', _('BAND 1'));
			o.value('103', _('BAND 3'));
			o.value('108', _('BAND 8'));
			o.value('111', _('BAND 11'));
			o.value('118', _('BAND 18'));
			o.value('119', _('BAND 19'));
			o.value('121', _('BAND 21'));
			o.value('126', _('BAND 26'));
			o.value('128', _('BAND 28'));
			o.value('141', _('BAND 41'));
			o.depends('smode', '3');
			o.depends('smode', '5');
			o.depends('smode', '6');
			o.depends('smode', '7');
        }
        
        // 公共选项
        if (modem_name.match(/668$/i)) {
            o = sec.taboption('advanced', form.Value, 'arfcn', _('Cell Frequency(ARFCN)'));
            o.depends('smode', '3');
            o.placeholder = _('[range: 0~4294967295]');
            
            o = sec.taboption('advanced', form.Value, 'pci', _('Cell Physical ID(PCI)'));
            o.depends('smode', '3');
            o.placeholder = _('[range: 0~503]');
        }
    }
    
    // 6. 其他模块的判断
    else if (modem_name && modem_name.match(/8200/i)) {
        o = sec.taboption('advanced', form.ListValue, 'band_wcdma', _('Lock 3G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('BAND 1'));
		o.value('3', _('BAND 3'));
		o.value('5', _('BAND 5'));
		o.value('6', _('BAND 6'));
		o.value('8', _('BAND 8'));
		o.value('9', _('BAND 9'));
		o.value('19', _('BAND 19'));
		o.depends('smode', '1');
		o.depends('smode', '3');
		o.depends('smode', '7');
				
		o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('BAND 1'));
		o.value('3', _('BAND 3'));
		o.value('5', _('BAND 5'));
		o.value('7', _('BAND 7'));
		o.value('8', _('BAND 8'));
		o.value('18', _('BAND 18'));
		o.value('19', _('BAND 19'));
		o.value('26', _('BAND 26'));
		o.value('28', _('BAND 28'));
		o.value('34', _('BAND 34'));
		o.value('38', _('BAND 38'));
		o.value('39', _('BAND 39'));
		o.value('40', _('BAND 40'));
		o.value('41', _('BAND 41'));
		o.value('42', _('BAND 42'));
		o.value('43', _('BAND 43'));
		o.value('48', _('BAND 48'));
		o.depends('smode', '2');
		o.depends('smode', '3');
		o.depends('smode', '6');
		o.depends('smode', '7');
				
		o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G(SA) Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('N1'));
		o.value('3', _('N3'));
		o.value('5', _('N5'));
		o.value('7', _('N7'));
		o.value('8', _('N8'));
		o.value('28', _('N28'));
		o.value('38', _('N38'));
		o.value('40', _('N40'));
		o.value('41', _('N41'));
		o.value('48', _('N48'));
		o.value('77', _('N77'));
		o.value('78', _('N78'));
		o.value('79', _('N79'));
		o.depends('smode', '4');
		o.depends('smode', '6');
		o.depends('smode', '7');
		
		o = sec.taboption('advanced', form.ListValue, 'band_nsa', _('Lock 5G(NSA) Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('N1'));
		o.value('3', _('N3'));
		o.value('5', _('N5'));
		o.value('7', _('N7'));
		o.value('8', _('N8'));
		o.value('28', _('N28'));
		o.value('38', _('N38'));
		o.value('40', _('N40'));
		o.value('41', _('N41'));
		o.value('48', _('N48'));
		o.value('77', _('N77'));
		o.value('78', _('N78'));
		o.value('79', _('N79'));
		o.depends('smode', '4');
		o.depends('smode', '6');
		o.depends('smode', '7');
    } else if (modem_name && modem_name.match(/500U-CN/i)) {
        o = sec.taboption('advanced', form.ListValue, 'band_wcdma', _('Lock 3G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('BAND 1'));
		o.value('2', _('BAND 2'));
		o.value('5', _('BAND 5'));
		o.value('8', _('BAND 8'));
		o.depends('smode', '1');
		o.depends('smode', '3');
		o.depends('smode', '7');
				
		o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('BAND 1'));
		o.value('2', _('BAND 2'));
		o.value('3', _('BAND 3'));
		o.value('5', _('BAND 5'));
		o.value('7', _('BAND 7'));
		o.value('8', _('BAND 8'));
		o.value('20', _('BAND 20'));
		o.value('28', _('BAND 28'));
		o.value('34', _('BAND 34'));
		o.value('38', _('BAND 38'));
		o.value('39', _('BAND 39'));
		o.value('40', _('BAND 40'));
		o.value('41', _('BAND 41'));
		o.depends('smode', '2');
		o.depends('smode', '3');
		o.depends('smode', '6');
		o.depends('smode', '7');
				
		o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('N1'));
		o.value('28', _('N28'));
		o.value('41', _('N41'));
		o.value('77', _('N77'));
		o.value('78', _('N78'));
		o.value('79', _('N79'));
		o.depends('smode', '4');
		o.depends('smode', '6');
		o.depends('smode', '7');
    } else if (modem_name && modem_name.match(/500Q-GL/i)) {
        o = sec.taboption('advanced', form.ListValue, 'band_wcdma', _('Lock 3G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('BAND 1'));
		o.value('2', _('BAND 2'));
		o.value('3', _('BAND 3'));
		o.value('4', _('BAND 4'));
		o.value('5', _('BAND 5'));
		o.value('6', _('BAND 6'));
		o.value('8', _('BAND 8'));
		o.value('19', _('BAND 19'));
		o.depends('smode', '1');
		o.depends('smode', '3');
		o.depends('smode', '7');
				
		o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('BAND 1'));
		o.value('2', _('BAND 2'));
		o.value('3', _('BAND 3'));
		o.value('4', _('BAND 4'));
		o.value('5', _('BAND 5'));
		o.value('7', _('BAND 7'));
		o.value('8', _('BAND 8'));
		o.value('12', _('BAND 12'));
		o.value('13', _('BAND 13'));
		o.value('14', _('BAND 14'));
		o.value('17', _('BAND 17'));
		o.value('18', _('BAND 18'));
		o.value('19', _('BAND 19'));
		o.value('20', _('BAND 20'));
		o.value('25', _('BAND 25'));
		o.value('26', _('BAND 26'));
		o.value('28', _('BAND 28'));
		o.value('29', _('BAND 29'));
		o.value('30', _('BAND 30'));
		o.value('32', _('BAND 32'));
		o.value('34', _('BAND 34'));
		o.value('38', _('BAND 38'));
		o.value('39', _('BAND 39'));
		o.value('40', _('BAND 40'));
		o.value('41', _('BAND 41'));
		o.value('42', _('BAND 42'));
		o.value('43', _('BAND 43'));
		o.value('46', _('BAND 46'));
		o.value('48', _('BAND 48'));
		o.value('66', _('BAND 66'));
		o.value('71', _('BAND 71'));
		o.depends('smode', '2');
		o.depends('smode', '3');
		o.depends('smode', '6');
		o.depends('smode', '7');
				
		o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('N1'));
		o.value('2', _('N2'));
		o.value('3', _('N3'));
		o.value('5', _('N5'));
		o.value('7', _('N7'));
		o.value('8', _('N8'));
		o.value('12', _('N12'));
		o.value('20', _('N20'));
		o.value('25', _('N25'));
		o.value('28', _('N28'));
		o.value('38', _('N38'));
		o.value('40', _('N40'));
		o.value('41', _('N41'));
		o.value('48', _('N48'));
		o.value('66', _('N66'));
		o.value('71', _('N71'));
		o.value('77', _('N77'));
		o.value('78', _('N78'));
		o.value('79', _('N79'));
		o.depends('smode', '4');
		o.depends('smode', '6');
		o.depends('smode', '7');
    } else if (modem_name && modem_name.match(/520N/i)) {
        if (modem_name.match(/520N-EU/i) || modem_name.match(/520N-EB/i)) {
            o = sec.taboption('advanced', form.ListValue, 'band_wcdma', _('Lock 3G Band List'));
			o.value('0', _('Automatically'));
			o.value('1', _('BAND 1'));
			o.value('5', _('BAND 5'));
			o.value('8', _('BAND 8'));
			o.depends('smode', '1');
			o.depends('smode', '3');
			o.depends('smode', '7');
					
			o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
			o.value('0', _('Automatically'));
			o.value('1', _('BAND 1'));
			o.value('3', _('BAND 3'));
			o.value('5', _('BAND 5'));
			o.value('7', _('BAND 7'));
			o.value('8', _('BAND 8'));
			o.value('20', _('BAND 20'));
			o.value('28', _('BAND 28'));
			o.value('32', _('BAND 32'));
			o.value('38', _('BAND 38'));
			o.value('40', _('BAND 40'));
			o.value('41', _('BAND 41'));
			o.value('42', _('BAND 42'));
			o.value('43', _('BAND 43'));
			o.depends('smode', '2');
			o.depends('smode', '3');
			o.depends('smode', '6');
			o.depends('smode', '7');
					
			o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
			o.value('0', _('Automatically'));
			o.value('1', _('N1'));
			o.value('3', _('N3'));
			o.value('5', _('N5'));
			o.value('7', _('N7'));
			o.value('8', _('N8'));
			o.value('20', _('N20'));
			o.value('28', _('N28'));
			o.value('38', _('N38'));
			o.value('40', _('N40'));
			o.value('41', _('N41'));
			o.value('75', _('N75'));
			o.value('76', _('N76'));
			o.value('77', _('N77'));
			o.value('78', _('N78'));
			o.depends('smode', '4');
			o.depends('smode', '6');
			o.depends('smode', '7');
        } else if (modem_name.match(/520N-GL/i)) {
            o = sec.taboption('advanced', form.ListValue, 'band_wcdma', _('Lock 3G Band List'));
			o.value('0', _('Automatically'));
			o.value('1', _('BAND 1'));
			o.value('2', _('BAND 2'));
			o.value('4', _('BAND 4'));
			o.value('5', _('BAND 5'));
			o.value('8', _('BAND 8'));
			o.value('19', _('BAND 19'));
			o.depends('smode', '1');
			o.depends('smode', '3');
			o.depends('smode', '7');
					
			o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
			o.value('0', _('Automatically'));
			o.value('1', _('BAND 1'));
			o.value('2', _('BAND 2'));
			o.value('3', _('BAND 3'));
			o.value('4', _('BAND 4'));
			o.value('5', _('BAND 5'));
			o.value('7', _('BAND 7'));
			o.value('8', _('BAND 8'));
			o.value('12', _('BAND 12'));
			o.value('13', _('BAND 13'));
			o.value('14', _('BAND 14'));
			o.value('17', _('BAND 17'));
			o.value('18', _('BAND 18'));
			o.value('19', _('BAND 19'));
			o.value('20', _('BAND 20'));
			o.value('25', _('BAND 25'));
			o.value('26', _('BAND 26'));
			o.value('28', _('BAND 28'));
			o.value('29', _('BAND 29'));
			o.value('30', _('BAND 30'));
			o.value('32', _('BAND 32'));
			o.value('34', _('BAND 34'));
			o.value('38', _('BAND 38'));
			o.value('39', _('BAND 39'));
			o.value('40', _('BAND 40'));
			o.value('41', _('BAND 41'));
			o.value('42', _('BAND 42'));
			o.value('43', _('BAND 43'));
			o.value('46', _('BAND 46'));
			o.value('48', _('BAND 48'));
			o.value('66', _('BAND 66'));
			o.value('71', _('BAND 71'));
			o.depends('smode', '2');
			o.depends('smode', '3');
			o.depends('smode', '6');
			o.depends('smode', '7');
					
			o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
			o.value('0', _('Automatically'));
			o.value('1', _('N1'));
			o.value('2', _('N2'));
			o.value('3', _('N3'));
			o.value('5', _('N5'));
			o.value('7', _('N7'));
			o.value('8', _('N8'));
			o.value('12', _('N12'));
			o.value('13', _('N13'));
			o.value('14', _('N14'));
			o.value('18', _('N18'));
			o.value('20', _('N20'));
			o.value('25', _('N25'));
			o.value('26', _('N26'));
			o.value('28', _('N28'));
			o.value('29', _('N29'));
			o.value('30', _('N30'));
			o.value('38', _('N38'));
			o.value('40', _('N40'));
			o.value('41', _('N41'));
			o.value('48', _('N48'));
			o.value('66', _('N66'));
			o.value('70', _('N70'));
			o.value('71', _('N71'));
			o.value('75', _('N75'));
			o.value('76', _('N76'));
			o.value('77', _('N77'));
			o.value('78', _('N78'));
			o.value('79', _('N79'));
			o.depends('smode', '4');
			o.depends('smode', '6');
			o.depends('smode', '7');
        } else if (modem_name.match(/520N-CN/i)) {
            o = sec.taboption('advanced', form.ListValue, 'band_wcdma', _('Lock 3G Band List'));
			o.value('0', _('Automatically'));
			o.value('1', _('BAND 1'));
			o.value('8', _('BAND 8'));
			o.depends('smode', '1');
			o.depends('smode', '3');
			o.depends('smode', '7');
					
			o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
			o.value('0', _('Automatically'));
			o.value('1', _('BAND 1'));
			o.value('3', _('BAND 3'));
			o.value('5', _('BAND 5'));
			o.value('8', _('BAND 8'));
			o.value('34', _('BAND 34'));
			o.value('38', _('BAND 38'));
			o.value('39', _('BAND 39'));
			o.value('40', _('BAND 40'));
			o.value('41', _('BAND 41'));
			o.depends('smode', '2');
			o.depends('smode', '3');
			o.depends('smode', '6');
			o.depends('smode', '7');
					
			o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
			o.value('0', _('Automatically'));
			o.value('1', _('N1'));
			o.value('3', _('N3'));
			o.value('5', _('N5'));
			o.value('8', _('N8'));
			o.value('28', _('N28'));
			o.value('41', _('N41'));
			o.value('78', _('N78'));
			o.value('79', _('N79'));
			o.depends('smode', '4');
			o.depends('smode', '6');
			o.depends('smode', '7');
		}
		// 公共选项
		o = sec.taboption('advanced', form.Value, 'arfcn', _('Cell Frequency(ARFCN)'));
		o.depends('smode', '2');
		o.depends('smode', '4');
		//o.placeholder = _('[range: 0~4294967295]');
			
		o = sec.taboption('advanced', form.Value, 'pci', _('Cell Physical ID(PCI)'));
		o.depends('smode', '2');
		//o.placeholder = _('[range: 0~503]');
			
		o = sec.taboption('advanced', form.Value, 'psc', _('Cell Physical ID(PCI)'), _('To lock a cell, specify the frequency band at the same time'));
		o.depends('smode', '4');
		//o.placeholder = _('[range: 0~1007]');
			
		o = sec.taboption('advanced', form.ListValue, 'scs', _('NR Sub Carrier Space(SCS)'));
		o.value('15', _('15kHz(FDD)'));
		o.value('30', _('30kHz(TDD)'));
		o.value('120', _('120kHz(TDD)'));
		o.value('240', _('240kHz(TDD)'));
		o.default = '15';
		o.depends('smode', '4');
    } else if (modem_name && modem_name.match(/5700M-CN/i)) {
        o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('BAND 1'));
		o.value('3', _('BAND 3'));
		o.value('5', _('BAND 5'));
		o.value('8', _('BAND 8'));
		o.value('34', _('BAND 34'));
		o.value('38', _('BAND 38'));
		o.value('39', _('BAND 39'));
		o.value('40', _('BAND 40'));
		o.value('41', _('BAND 41'));
		o.depends('smode', '2');
		o.depends('smode', '3');
		o.depends('smode', '6');
		o.depends('smode', '7');
				
		o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('N1'));
		o.value('3', _('N3'));
		o.value('5', _('N5'));
		o.value('8', _('N8'));
		o.value('28', _('N28'));
		o.value('41', _('N41'));
		o.value('78', _('N78'));
		o.value('79', _('N79'));
		o.value('80', _('N80(SUL)'));
		o.value('81', _('N81(SUL)'));
		o.value('83', _('N83(SUL)'));
		o.value('84', _('N84(SUL)'));
		o.depends('smode', '4');
		o.depends('smode', '6');
		o.depends('smode', '7');
		
		o = sec.taboption('advanced', form.Value, 'arfcn', _('Cell Frequency(ARFCN)'));
		o.depends('smode', '2');
		o.depends('smode', '4');
		o.placeholder = _('[range: 0~4294967295]');
			
		o = sec.taboption('advanced', form.Value, 'pci', _('Cell Physical ID(PCI)'));
		o.depends('smode', '2');
		o.placeholder = _('[range: 0~503]');
			
		o = sec.taboption('advanced', form.Value, 'psc', _('Cell Physical ID(PCI)'), _('To lock a cell, specify the frequency band at the same time'));
		o.depends('smode', '4');
		o.placeholder = _('[range: 0~1007]');
			
		o = sec.taboption('advanced', form.ListValue, 'scs', _('NR Sub Carrier Space(SCS)'));
		o.value('0', _('15kHz(FDD)'));
		o.value('1', _('30kHz(TDD)'));
		o.value('2', _('60kHz(TDD)'));
		o.value('3', _('120kHz(TDD)'));
		o.value('4', _('240kHz(TDD)'));
		o.default = '1';
		o.depends('smode', '4');
    } else if (modem_name && (modem_name.match(/5711_CN/i) || modem_name.match(/5710_CN/i))) {
        o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('BAND 1'));
		o.value('3', _('BAND 3'));
		o.value('5', _('BAND 5'));
		o.value('8', _('BAND 8'));
		o.value('34', _('BAND 34'));
		o.value('38', _('BAND 38'));
		o.value('39', _('BAND 39'));
		o.value('40', _('BAND 40'));
		o.value('41', _('BAND 41'));
		o.value('59', _('BAND 59'));
		o.value('62', _('BAND 62'));
		o.depends('smode', '2');
		o.depends('smode', '3');
		o.depends('smode', '6');
		o.depends('smode', '7');
				
		o = sec.taboption('advanced', form.ListValue, 'band_nr', _('Lock 5G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('N1'));
		o.value('3', _('N3'));
		o.value('5', _('N5'));
		o.value('8', _('N8'));
		o.value('28', _('N28'));
		o.value('41', _('N41'));
		o.value('78', _('N78'));
		o.value('79', _('N79'));
		o.depends('smode', '4');
		o.depends('smode', '6');
		o.depends('smode', '7');
		
		o = sec.taboption('advanced', form.Value, 'arfcn', _('Cell Frequency(ARFCN)'));
		o.depends('smode', '2');
		o.depends('smode', '4');
		o.placeholder = _('[range: 0~4294967295]');
			
		o = sec.taboption('advanced', form.Value, 'pci', _('Cell Physical ID(PCI)'));
		o.depends('smode', '2');
		o.placeholder = _('[range: 0~503]');
			
		o = sec.taboption('advanced', form.Value, 'psc', _('Cell Physical ID(PCI)'), _('To lock a cell, specify the frequency band at the same time'));
		o.depends('smode', '4');
		o.placeholder = _('[range: 0~1007]');
			
		o = sec.taboption('advanced', form.ListValue, 'scs', _('NR Sub Carrier Space(SCS)'));
		o.value('0', _('15kHz(FDD)'));
		o.value('1', _('30kHz(TDD)'));
		o.value('2', _('60kHz(TDD)'));
		o.value('3', _('120kHz(TDD)'));
		o.value('4', _('240kHz(TDD)'));
		o.default = '1';
		o.depends('smode', '4');
    }
    
    // 7. EC25模块（根据版本判断）
    else if (modem_version && modem_version.match(/EC25AF/i)) {
        o = sec.taboption('advanced', form.ListValue, 'band_wcdma', _('Lock 3G Band List'));
		o.value('0', _('Automatically'));
		o.value('10', _('BAND 1'));
		o.value('80', _('BAND 8'));
		o.depends('smode', '2');
				
		o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
		o.value('0', _('Automatically'));
		o.value('2', _('BAND 2'));
		o.value('8', _('BAND 4'));
		o.value('10', _('BAND 5'));
		o.value('800', _('BAND 12'));
		o.value('1000', _('BAND 13'));
		o.value('2000', _('BAND 14'));
		o.value('20000000000000000', _('BAND 66'));
		o.value('400000000000000000', _('BAND 71'));
		o.depends('smode', '3');
    } else if (modem_version && modem_version.match(/EC25EUX/i)) {
		o = sec.taboption('advanced', form.ListValue, 'band_gsm', _('Lock 2G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('EGSM900'));
		o.value('2', _('DCS1800'));
		o.depends('smode', '1');
		
        o = sec.taboption('advanced', form.ListValue, 'band_wcdma', _('Lock 3G Band List'));
		o.value('0', _('Automatically'));
		o.value('10', _('BAND 1'));
		o.value('80', _('BAND 8'));
		o.depends('smode', '2');
				
		o = sec.taboption('advanced', form.ListValue, 'band_lte', _('Lock 4G Band List'));
		o.value('0', _('Automatically'));
		o.value('1', _('BAND 1'));
		o.value('4', _('BAND 3'));
		o.value('40', _('BAND 7'));
		o.value('80', _('BAND 8'));
		o.value('8000', _('BAND 20'));
		o.value('8000000', _('BAND 28'));
		o.value('2000000000', _('BAND 38'));
		o.value('8000000000', _('BAND 40'));
		o.value('10000000000', _('BAND 41'));
		o.depends('smode', '3');
    }
}

return view.extend({
    load: function() {
        return Promise.all([
            uci.load('network'),
            network.getNetworks(),
            L.resolveDefault(callSystemBoard(), {}),
            L.resolveDefault(callModemInfo(), {})
        ]);
    },

    render: function(data) {
        var networks = data[1];
        var boardinfo = data[2] || {};
        var modeminfo = callModemInfo() || {};
		var boardName = boardinfo.board_name || boardinfo.model || '';
        var modem_name = modeminfo.modem_name || '';
        var modem_ver = modeminfo.modem_version || '';
        var modem_type = modeminfo.modem_type || '';
        var m, s, s1, s2, s3, o;
        var self = this;
        
        //console.log('DEBUG: Board info:', boardName);
        
        m = new form.Map('modem', modem_type + _(' Network'));
		
        //通用配置
        s = m.section(form.NamedSection, config_name, 'ndis', _(config_name + ' Settings'));
        s.anonymous = true;
        s.addremove = false;

        s.networks = networks;
        s.section_id = config_name;
		
		s.tab('general', _('General Settings'));
		s.tab('advanced', _('Advanced Settings'));
		s.tab('physical', _('Physical Settings'));
		
        o = s.taboption('general', form.DummyValue, '_stat', _('Status'));
        o.modalonly = false;
        o.cfgvalue = L.bind(function(section_id) {
            var net = this.findInterface();
			
			var currentProto = uci.get('network', section_id, 'proto') || 'dhcp';
            
            // 如果协议是'aslan'，不显示接口状态，显示桥接信息
            // if (currentProto === 'aslan') {
                // return E('div', { 
                    // 'class': 'ifacebox',
                    // 'style': 'border: 0px solid #dee2e6; min-width: 18rem; padding-left: 6px; background: #fff;'
                // }, [
                    // E('div', { 
                        // 'class': 'ifacebox-head center',
                        // 'style': 'background-color:#fff'
                    // }, E('strong', _('-')))
                // ]);
            // }
			
            // if (!net) {
                // return E('div', { 
                    // 'class': 'ifacebox',
                    // 'style': 'border: 0px solid #dee2e6; min-width: 18rem; padding-left: 6px; background: #fff;'
                // }, [
                    // E('div', { 
                        // 'class': 'ifacebox-head center',
                        // 'style': 'background-color:#fff'
                    // }, E('strong', _('-')))
                // ]);
            // }
            
            return render_modal_status(E('div', {
                'id': '%s-status'.format(section_id),
                'class': 'ifacebadge large',
				'style': 'border: 1px solid #dee2e6; min-width: 18rem; padding-left: 6px; background: #fff;'
            }), net);
        }, s);
        o.write = function() {};
        
        o = s.taboption('general', form.Flag, 'enable', _('Enable'));
        o.rmempty = false;
		
		//基本配置
		o = s.taboption('general', form.Value, 'apn', _('APN'));
		
		o = s.taboption('general', form.Value, 'username', _('Username'));
		
		o = s.taboption('general', form.Value, 'password', _('Password'));
		o.password = true;
		
		o = s.taboption('general', form.ListValue, 'auth_type', _('Auth Type'));
		o.value('0',_('none'));
        o.value('1',_('pap'));
        o.value('2',_('chap'));
		if (modem_name) {
			if (modem_name.match(/mv31w/i)) {
				o.value('3', _('mschapv2'));
			} 
			else if (
				modem_name.match(/650/i) || 
				modem_name.match(/160/i) || 
				modem_name.match(/668/i) ||
				modem_name.match(/500U/i) ||
				modem_name.match(/500Q/i) ||
				modem_name.match(/520N/i) ||
				modem_name.match(/7600/i) ||
				modem_name.match(/EC25/i)
			) {
				o.value('3', _('pap/chap'));
			}
		}
		
		if (modem_name)
		{
			if (
				modem_name.match(/5700M/i) ||
				modem_name.match(/5711/i) ||
				modem_name.match(/5710/i)
			) {
				o = s.taboption('general', form.Value, 'plmn', _('PLMN'));
			}
		}
		
		o = s.taboption('general', form.Value, 'pincode', _('PIN Code'));
		
        s.findInterface = function() {
            if (!this.networks || this.networks.length === 0) {
                return null;
            }
            
            for (var i = 0; i < this.networks.length; i++) {
                var ifc = this.networks[i];
                if (ifc) {
                    var name = ifc.getName();
                    if (name && (name === config_name || name.toLowerCase().includes(config_name))) {
                        return ifc;
                    }
                }
            }
            return null;
        };
		
		//高级配置
		o = s.taboption('advanced', form.ListValue, 'smode', _('Service Mode'));
		if (modem_name) {
			if (modem_name.match(/160-JK/i)) {
				o.value('2', _('4G (LTE) Only'));
				o.value('4', _('5G (NR) Only'));
				o.value('6', _('4/5G (LTE/NR)'));
				o.default = '6';
			}
			else
			{
				o.value('0', _('Automatically'));
				o.default = '0';
				if (modem_name.match(/7600/i)) {
					o.value('5', _('2G (GSM) Only'));
					o.value('1', _('3G (WCDMA) Only'));
					o.value('2', _('4G (LTE) Only'));
					o.value('8', _('2/3G (GSM/WCDMA)'));
					o.value('9', _('2/4G (GSM/LTE)'));
					o.value('3', _('3/4G (WCDMA/LTE)'));
					o.value('10', _('2/3/4G (GSM/WCDMA/LTE)'));
				}
				else if (
					modem_name.match(/668-LA/i) ||
					modem_name.match(/668-EU/i) ||
					modem_name.match(/668-EAU/i)
				) {
					o.value('1', _('2G (GSM) Only'));
					o.value('2', _('3G (UMTS) Only'));
					o.value('3', _('4G (LTE) Only'));
					o.value('4', _('2/3G (GSM/UMTS)'));
					o.value('6', _('2/4G (GSM/LTE)'));
					o.value('5', _('3/4G (UMTS/LTE)'));
					o.value('7', _('2/3/4G (GSM/UMTS/LTE)'));
				}
				else if (
					modem_name.match(/668-JP/i) ||
					modem_name.match(/668-AM/i)
				) {
					o.value('2', _('3G (UMTS) Only'));
					o.value('3', _('4G (LTE) Only'));
					o.value('5', _('3/4G (UMTS/LTE)'));
				}
				else if (modem_name.match(/668-CN/i)) {
					o.value('1', _('2G (GSM) Only'));
					o.value('2', _('3G (UMTS) Only'));
					o.value('3', _('4G (LTE) Only'));
					o.value('4', _('2/3G (GSM/UMTS)'));
					o.value('6', _('2/4G (GSM/LTE)'));
					o.value('5', _('3/4G (UMTS/LTE)'));
					o.value('7', _('2/3/4G (GSM/UMTS/LTE)'));
					o.value('8', _('TD-SCDMA Only'));
					o.value('11', _('CDMA Only'));
					o.value('13', _('EVDO Only'));
					o.value('12', _('CDMA/EVDO'));
				}
				else if (modem_name.match(/EC25AF/i)) {
					o.value('2', _('3G (WCDMA) Only'));
					o.value('3', _('4G (LTE) Only'));
				}
				else if (modem_name.match(/EC25EUX/i)) {
					o.value('1', _('2G (GSM) Only'));
					o.value('2', _('3G (WCDMA) Only'));
					o.value('3', _('4G (LTE) Only'));
				}
				else if (
					modem_name.match(/5711_CN/i) ||
					modem_name.match(/5710_CN/i)
				) {
					o.value('2', _('4G (LTE) Only'));
					o.value('4', _('5G (NR) Only'));
					o.value('6', _('4/5G (LTE/NR)'));
				}
				else {
					o.value('1', _('3G (WCDMA) Only'));
					o.value('2', _('4G (LTE) Only'));
					o.value('4', _('5G (NR) Only'));
					o.value('3', _('3/4G (WCDMA/LTE)'));
					o.value('6', _('4/5G (LTE/NR)'));
					o.value('7', _('3/4/5G (WCDMA/LTE/NR)'));
				}
			}
		}
		addBandOptions(s, modem_name, modem_ver);
		
		o = s.taboption('advanced', form.ListValue, 'ipv4v6', _('IP Type'));
		o.value('IP',_('IPV4'));
		// o.value('IPV6',_('IPV6'));
        o.value('IPV4V6',_('IPV4V6'));
		
		//GPS使能
		if ( ( modem_name && ( modem_name.match(/160/i) || modem_name.match(/668-EU/i) ) ) && ( boardName && ( boardName.match(/ap-cp01-c1/i) || boardName.match(/ap-cp01-c3/i) ) ) )
		{
			o = s.taboption('advanced', form.Flag, 'gps_enable', _('Enable GPS'))
		}
		
		//NAT使能
		if ( modem_name && modem_name.match(/668/i) )
		{
			o = s.taboption('advanced', form.Flag, 'natmode', _('Shared Connection(NAT)'))
		}
		
		o = s.taboption('advanced', form.Flag, 'force_dial', _('Force Dial'));
        o.rmempty = false;
		
		o = s.taboption('physical', form.Value, 'metric', _('Metric'));
        o.default = '10';
		
		o = s.taboption('physical', form.Value, 'mtu', _('Override MTU'));
        o.datatype = 'max(9200)';
        o.default = '1500';
		
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
            
            var section_id = 'SIM';
            
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
            fs.exec_direct('/etc/init.d/modeminit', ['restart']).then(function(output) {
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