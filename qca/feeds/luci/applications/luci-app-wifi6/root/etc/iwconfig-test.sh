#!/bin/sh

ath=$(uci -q get wireless.sta0.ifname)
echo "#####测试接口$ath#########"
count=1
rssi_threshold="-60"
iwcnf="/tmp/iwconfig"
while true
do
	iwconfig $ath > $iwcnf
	addr=$(ifconfig $ath | grep "inet addr:" | awk '{print $2}')
	essid=$(cat $iwcnf | grep Acc |awk '{print $6}')
	#rssi=$(cat $iwcnf | grep Signal |awk '{print $4}')
	rssi=$(cat $iwcnf | grep "Signal level" | awk '{print $4}' | cut -d '=' -f 2 | cut -d ' ' -f 1)
	if [ $(echo "$rssi < $rssi_threshold" | bc) -eq 1 ]; then
		#echo "IP $addr,  RSSI $rssi,  ESSID=$essid  $count"
		echo -e "\033[31m""IP $addr,  RSSI $rssi,  ESSID=$essid  <$count>""\033[0m"
	else
		echo "IP $addr,  RSSI $rssi,  ESSID=$essid  $count"
	fi
	sleep 1
	let count++	#计数器
done


