'use strict';
'require view';
'require fs';
'require ui';

return view.extend({
	load: function() {
		return Promise.all([
			L.resolveDefault(fs.stat('/sbin/logread'), null),
			L.resolveDefault(fs.stat('/usr/sbin/logread'), null)
		]).then(function(stat) {
			var logger = stat[0] ? stat[0].path : stat[1] ? stat[1].path : null;

			return fs.exec_direct(logger, [ '-e', '^' ]).catch(function(err) {
				ui.addNotification(null, E('p', {}, _('Unable to load log data: ' + err.message)));
				return '';
			});
		});
	},

	render: function(logdata) {
		var loglines = logdata.trim().split(/\n/);
		
		var logtext = loglines.join('\n');
		// 创建下载按钮
		var downloadBtn = E('button', {
			'class': 'btn cbi-button-positive',
			'click': function(ev) {
				// 生成更友好的日期时间戳
				var now = new Date();
				var timestamp = now.getFullYear() + '-' + 
					String(now.getMonth() + 1).padStart(2, '0') + '-' + 
					String(now.getDate()).padStart(2, '0') + '_' + 
					String(now.getHours()).padStart(2, '0') + '-' + 
					String(now.getMinutes()).padStart(2, '0') + '-' + 
					String(now.getSeconds()).padStart(2, '0');
				
				// 创建Blob对象
				var blob = new Blob([logtext], { type: 'text/plain' });
				
				// 创建下载链接
				var url = URL.createObjectURL(blob);
				var a = document.createElement('a');
				a.href = url;
				a.download = 'system-log-' + timestamp + '.txt';
				
				// 触发下载
				document.body.appendChild(a);
				a.click();
				document.body.removeChild(a);
				
				// 清理URL
				setTimeout(function() {
					URL.revokeObjectURL(url);
				}, 100);
				
				ev.preventDefault();
				return false;
			}
		}, [_('Download Log')]);
		
		return E([], [
			// 添加按钮容器
			E('div', { 'class': 'cbi-page-actions' }, [
				downloadBtn
			]),
			E('div', { 'id': 'content_syslog' }, [
				E('textarea', {
					'id': 'syslog',
					'style': 'font-size:12px',
					'readonly': 'readonly',
					'wrap': 'off',
					'rows': loglines.length + 1
				}, [ loglines.join('\n') ])
			])
		]);
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});
