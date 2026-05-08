#!/bin/sh
while [ 1 ]
do
# 方法1: 通过设备路径查找
	for netdev in /sys/class/net/*; do
		dev_name=$(basename "$netdev")
		
		# 获取USB设备路径
		usb_device=$(readlink "$netdev/device" 2>/dev/null | xargs basename 2>/dev/null)
		
		if [ -n "$usb_device" ]; then
			# 查找父设备（实际的4G模块）
			parent_path=$(dirname $(readlink "/sys/bus/usb/devices/$usb_device" 2>/dev/null) 2>/dev/null)
			parent_device=$(basename "$parent_path" 2>/dev/null)
			
			if [ -n "$parent_device" ] && [ -f "/sys/bus/usb/devices/$parent_device/serial" ]; then
				serial=$(cat "/sys/bus/usb/devices/$parent_device/serial" 2>/dev/null)
				echo "USB=$parent_device ifname=$dev_name serialnumber=$serial"
				section=$(uci -P /var/state -q show cellular | grep "$parent_device" | awk -F '.' '{print $2}')
				[ -n "$section" ] && uci -P /var/state set cellular.$section.ifname=$dev_name #  >> /tmp/log/usbmapping
			fi
		fi
	done
sleep 2s
#echo '' >  /tmp/log/usbmapping
done
