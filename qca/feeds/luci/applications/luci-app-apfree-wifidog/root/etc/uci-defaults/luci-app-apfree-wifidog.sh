#!/bin/sh
[ -f "/etc/config/hostnames" ] || {
	echo 'config hostname' > /etc/config/hostnames
}
sn=$(slktool /dev/mmcblk0p25 sn | awk -F ': ' '{print$2}')
mac=$(slkmac eth0 | tr -d ':')
format_mac=$(echo "$mac" | sed 's/\(..\)/\1-/g;s/-$//')
sn_mac="${sn:0:4}${mac}"

uci set wifidogx.common.device_id=$sn
uci set wifidogx.common.location_id=$sn
uci set wifidogx.common.ap_device_id=$sn_mac
uci set wifidogx.common.ap_mac_address=$format_mac
uci set wifidogx.apfree.gateway_id=$sn
uci commit

exit 0
