#!/bin/sh

file_conf=$1

. /$file_conf


##########
nul=/dev/null
# mmc=/dev/mmcblk0p25
exe=slktool

### 生成passwd密码
change_password()
{
	# 以下是密钥明文admin
	admin_key='$1$iiH1xDO6$ZpdF8PENe8U3rbHCb/n3K1'
	# 定义要修改的文件路径
	file_path="/etc/shadow"

	if test -z $other_key ; then
		if [ "$1" == "admin" ];then
			echo "Default admin_key: $admin_key"
			
		else
			# if test -f /tmp/key; then
				# admin_key=`cat /tmp/key`
				# echo "Root_Key==: $admin_key"	
			# else
				# echo "重新输入root密码:"
				# admin_key=`openssl passwd -1`
				# echo "Output: $admin_key"
				# echo $admin_key > /tmp/key
				
			# fi
			`echo admin:$1|chpasswd`
		fi
	else
			admin_key=$other_key	
			echo "Outher key: $admin_key"
	fi
	
	### 获取用户名
	name=`cat $file_path |awk -F ":" 'NR==1 {print $1}'`
	end=':19756:0:99999:7:::'
	sed -i '1c'"$name:$admin_key$end" ${file_path}	
			
	# cat ${file_path}
}


frist_init()
{
#echo "##### Backup bd_info #####"
	out_bin=/tmp/bd_info.bin
	dd if=$mmc of=$out_bin count=1 bs=256	# bs=256=0x00FF
#echo "##### Erase & Restore #####"
	dd if=/dev/zero of=$mmc count=8 bs=256
	dd if=$out_bin of=$mmc
}

write_default_id()
{
	#tim=$(date "+%y%m%d_%W_%H%M%S")
	###年+月+日+周+时
	tim=$(date "+%y%m%d-%W%H")
	tim2=$(date "+%y%m%d")
	ids=`openssl rand -hex 4`
	sns=`openssl rand -hex 4`
	###出厂固定参数设定
	[ -n "$id_num" ] && { $exe $mmc id "$id_num$ids" > $nul; }
	[ -n "$pn_unm" ] && { $exe $mmc pn "$pn_unm$sns" > $nul; }
	[ -n "$sn_un" ] && { $exe $mmc sn "$sn_un" > $nul; }
	[ -n "$tim" ] && { $exe $mmc date "$tim" > $nul; }
	[ -n "$hard_ver" ] && { $exe $mmc hard "$hard_ver" > $nul; }
	[ -n "$soft_ver" ] && { $exe $mmc soft "$soft_ver" > $nul; }
	# $exe $mmc lock  > $nul
	# $exe $mmc custom  > $nul
}

write_data()
{	
	echo "##### Write bin start #####"

	mac_id=$(cat /sys/class/net/eth0/address|awk -F \
	":" '{print $5""$6 }'| tr a-z A-Z)

	### 系统配置
	[ -n "$model" ] && { $exe $mmc model "$model" > $nul; }
	[ -n "$there" ] && { $exe $mmc there "$there" > $nul; }
	[ -n "$password" ] && { $exe $mmc password "$password" > $nul; }
	[ -n "$ssh_en" ] && { $exe $mmc ssh "$ssh_en" > $nul; }
	[ -n "$country" ] && { $exe $mmc country "$country" > $nul; }
	[ -n "$language" ] && { $exe $mmc language "$language" > $nul; }
	[ -n "$hostname" ] && { $exe $mmc hostname "$hostname" > $nul; }
	[ -n "$position" ] && { $exe $mmc position "$position" > $nul; }

	### 网络配置network
	[ -n "$lan_ip" ] && { $exe $mmc lan_ip  "$lan_ip" > $nul; }
	[ -n "$lan_mask" ] && { $exe $mmc lan_mask "$lan_mask" > $nul	; }


	### WIFI配置
	[ -n "$wifi0_en" ] && { $exe $mmc wifi0_en "$wifi0_en" > $nul; }
	[ -n "$wifi0_ssid" ] && { $exe $mmc wifi0_name "$wifi0_ssid-$mac_id" > $nul	; }
	[ -n "$wifi0_key" ] && { $exe $mmc wifi0_key "$wifi0_key" > $nul; }
	[ -n "$wifi0_channel" ] && { $exe $mmc wifi0_channel "$wifi0_channel" > $nul; }
	[ -n "$wifi1_en" ] && { $exe $mmc wifi1_en "$wifi1_en" > $nul; }
	[ -n "$wifi1_ssid" ] && { $exe $mmc wifi1_name "$wifi1_ssid-$mac_id" > $nul	; }
	[ -n "$wifi1_key" ] && { $exe $mmc wifi1_key "$wifi1_key" > $nul; }
	[ -n "$wifi1_channel" ] && { $exe $mmc wifi1_channel "$wifi1_channel" > $nul; }
	

	### 5G配置
	# [ -n "$id_num" ] && { $exe $mmc nr5g_en "$nr5g_en" > $nul
	# [ -n "$id_num" ] && { $exe $mmc nr5g_en2 "$nr5g_en2" > $nul
	# [ -n "$id_num" ] && { $exe $mmc apn cmiot "$apn" > $nul
	# [ -n "$id_num" ] && { $exe $mmc apn2 cmiot "$an2" > $nul
	# [ -n "$id_num" ] && { $exe $mmc nr5g_user "$nr5g_user" > $nul
	# [ -n "$id_num" ] && { $exe $mmc nr5g_user2 "$nr5g_user2" > $nul
	# [ -n "$id_num" ] && { $exe $mmc nr5g_key "$nr5g_key" > $nul
	# [ -n "$id_num" ] && { $exe $mmc nr5g_key2 "$nr5g_key2" > $nul
	

	### 未使用
	[ -n "$undefined1" ] && { $exe $mmc undefined1 "$undefined1" > $nul; }
	[ -n "$undefined2" ] && { $exe $mmc undefined2 "$undefined2" > $nul; }
	[ -n "$undefined3" ] && { $exe $mmc undefined3 "$undefined3" > $nul; }
	[ -n "$undefined4" ] && { $exe $mmc undefined4 "$undefined4" > $nul; }
	[ -n "$undefined5" ] && { $exe $mmc undefined5 "$undefined5" > $nul; }
	[ -n "$undefined6" ] && { $exe $mmc undefined6 "$undefined6" > $nul; }
	[ -n "$undefined7" ] && { $exe $mmc undefined7 "$undefined7" > $nul; }
	[ -n "$undefined8" ] && { $exe $mmc undefined8 "$undefined8" > $nul; }

	#echo "##### Write bin end #####"
}

read_data()
{	
	network_list="lan_ip lan_mask"

	wifi0_list="wifi0_en wifi0_name wifi0_key wifi0_channel country "

	wifi1_list="wifi1_en wifi1_name wifi1_key wifi1_channel"

	nr5g_list="nr5g_en apn nr5g_user nr5g_key"

	nr5g2_list="nr5g_en2 apn2 nr5g_user2 nr5g_key2"

	system_list="model there password ssh language hostname\
	id sn pn position hard soft"

	undef_list="undefined1 undefined2 undefined3 undefined4 \
	undefined5 undefined6 undefined7 undefined8"

### 系统配置
	for i in $system_list
		do
			str=`$exe $mmc "$i" | grep ': '`
			value=`echo $str | awk -F ": " '{print $2}'`
			name=`echo $str | awk -F ": " '{print $1}'`	
			length=$(expr length "$value") ###获取vlaue长度

			if [ "$length" -ne "0" ] || [ "$i" == "model" ]; then
				echo "$name=$value" 
				
				if echo "$name" | grep -q "MODEL"; then
					if [ -z $value ];then
						value=$(cat /mnt/slkdata.conf | grep model | awk -F '"' '{print$2}')
					fi
					uci -q set system.@system[0].model="$value"
					
				elif echo "$name" | grep -q "SSH_EN"; then
					if echo "$value" | grep -q "on"; then
						uci -q set dropbear.@dropbear[0]=dropbear
						uci -q set dropbear.@dropbear[0].Port='22'
						uci -q set dropbear.@dropbear[0].PasswordAuth='on'
						uci -q set dropbear.@dropbear[0].RootPasswordAuth='on'
						uci commit dropbear
						#/etc/init.d/dropbear restart
					else
						uci -q del dropbear.@dropbear[0]
						uci commit dropbear
						#/etc/init.d/dropbear disable
					fi
						
				
				# elif echo "$name" | grep -q "PASSWORD"; then
					# #uci -q set network.lan.mask=$value
					# echo "Set password = $value"
					# change_password $value
						
				elif echo "$name" | grep -q "LANGUAGE"; then
					uci -q set luci.main.lang="$value"
					
				elif echo "$name" | grep -q "LUCI_THERE"; then
					uci -q set luci.main.mediaurlbase="/luci-static/$value"
				
				elif echo "$name" | grep -q "HOSTNAME"; then
					uci -q set system.@system[0].hostname="$value"
				
				elif echo "$name" | grep -q "POSITION"; then
					uci -q set system.@system[0].position="$value"
				
				elif echo "$name" | grep -q "ID"; then
					uci -q set system.@system[0].id="$value"
					
				elif echo "$name" | grep -q "SN"; then
					uci -q set system.@system[0].sn="$value"
				
				elif echo "$name" | grep -q "PN"; then
					uci -q set system.@system[0].pn="$value"
					
				elif echo "$name" | grep -q "HARD_VERSION"; then
					uci -q set system.@system[0].H_ver="$value"
					
				elif echo "$name" | grep -q "SOFT_VERSION"; then
					uci -q set system.@system[0].F_ver="$value"
					
				fi
				
			fi
		done


### 网络配置network.lan
	for i in $network_list
		do
			str=`$exe $mmc "$i" | grep ': '`
			value=`echo $str | awk -F ": " '{print $2}'`
			name=`echo $str | awk -F ": " '{print $1}'`	
			length=$(expr length "$value") ###获取vlaue长度
			
			if [ "$length" -ne "0" ]; then
				echo "$name=$value" 
				
				if echo "$name" | grep -q "LAN_IP"; then
					uci -q set network.lan.ipaddr="$value"
					
				elif echo "$name" | grep -q "LAN_MASK"; then
					uci -q set network.lan.netmask="$value"
					
				fi
				
			fi
		done
		
### WIFI配置wireless.wifi
	for i in $wifi0_list $wifi1_list 
		do
			str=`$exe $mmc "$i" | grep ': '`
			value=`echo $str | awk -F ": " '{print $2}'`
			name=`echo $str | awk -F ": " '{print $1}'`	
			length=$(expr length "$value") ###获取vlaue长度
			
			if [ "$length" -ne "0" ]; then
			
				echo "$name=$value"
	
				if echo "$name" | grep -q "COUNTRY"; then
					uci -q set wireless.wifi0.country="$value"
					uci -q set wireless.wifi1.country="$value"
					
			#####直接写入/etc/config/wificonfig	
				elif echo "$name" | grep -q "WIFI0_EN"; then
					uci -q set wificonfig.wlan0.en="$value"
					
				elif echo "$name" | grep -q "WIFI0_CHANNEL"; then
					uci -q set wificonfig.wlan0.channel="$value"
					
				elif echo "$name" | grep -q "WIFI0_NAME"; then
					uci -q set wificonfig.wlan0.ssid="$value"
					
				elif echo "$name" | grep -q "WIFI0_KEY"; then
					uci -q set wificonfig.wlan0.key="$value"
					
				elif echo "$name" | grep -q "WIFI1_EN"; then
					uci -q set wireless.wifi1.disabled="$value"
					
				elif echo "$name" | grep -q "WIFI1_CHANNEL"; then
					uci -q set wificonfig.wlan1.channel="$value"
					
				elif echo "$name" | grep -q "WIFI1_NAME"; then
					uci -q set wificonfig.wlan1.ssid="$value"
					
				elif echo "$name" | grep -q "WIFI1_KEY"; then
					uci -q set wificonfig.wlan1.key="$value"
					
				fi



			#####直接写入/etc/config/wireless
				# if echo "$name" | grep -q "COUNTRY"; then
					# uci -q set wireless.wifi0.country=$value
					# uci -q set wireless.wifi1.country=$value
					
				# elif echo "$name" | grep -q "WIFI0_EN"; then
					# uci -q set wireless.wifi0.disabled=$value
					
				# elif echo "$name" | grep -q "WIFI0_CHANNEL"; then
					# uci -q set wireless.wifi0.channel=$value
					
				# elif echo "$name" | grep -q "WIFI0_NAME"; then
					# uci -q set wireless.@wifi-iface[0].ssid=$value
					
				# elif echo "$name" | grep -q "WIFI0_KEY"; then
					# uci -q set wireless.@wifi-iface[0].key=$value
					
				# elif echo "$name" | grep -q "WIFI1_EN"; then
					# uci -q set wireless.wifi1.disabled=$value
					
				# elif echo "$name" | grep -q "WIFI1_CHANNEL"; then
					# uci -q set wireless.wifi1.channel=$value
					
				# elif echo "$name" | grep -q "WIFI1_NAME"; then
					# uci -q set wireless.@wifi-iface[1].ssid=$value
					
				# elif echo "$name" | grep -q "WIFI1_KEY"; then
					# uci -q set wireless.@wifi-iface[1].key=$value
					
				# fi
			fi
		done
		
### 5G拔号配置
	# for i in $nr5g_list $nr5g2_list
		# do
			# str=`$exe $mmc "$i" | grep ': '`
			# value=`echo $str | awk -F ": " '{print $2}'`
			# name=`echo $str | awk -F ": " '{print $1}'`	
			# length=$(expr length "$value") ###获取vlaue长度
			
			# if [ "$length" -ne "0" ]; then
				# echo "$name=$value" 
				
				# ###NR5G_2 配置
				# if echo "$name" | grep -q "NR5G_EN2"; then
					# uci -q set modem.SIM.enable2=$value
					
				# elif echo "$name" | grep -q "APN2"; then
					# uci -q set modem.SIM.apn2=$value
				
				# elif echo "$name" | grep -q "NR5G_USER2"; then
					# uci -q set modem.SIM.username2=$value
					
				# elif echo "$name" | grep -q "NR5G_KEY2"; then
					# uci -q set modem.SIM.password2=$value
					
				# ###NR5G配置 /etc/config/modem	
				# elif echo "$name" | grep -q "NR5G_EN"; then
					# uci -q set modem.SIM.enable=$value
					
				# elif echo "$name" | grep -q "APN"; then
					# uci -q set modem.SIM.apn=$value
				
				# elif echo "$name" | grep -q "NR5G_USER"; then
					# uci -q set modem.SIM.username=$value
					
				# elif echo "$name" | grep -q "NR5G_KEY"; then
					# uci -q set modem.SIM.password=$value
				
				# fi
				
			# fi
		# done


### 未定义配置
	for i in $undef_list
		do
			str=`$exe $mmc "$i" | grep ': '`
			value=`echo $str | awk -F ": " '{print $2}'`
			name=`echo $str | awk -F ": " '{print $1}'`	
			length=$(expr length "$value") ###获取vlaue长度
			
			if [ "$length" -ne "0" ]; then
				echo "$name=$value" 
				
				if echo "$name" | grep -q "UNDEFINED1"; then
					echo ">>>$name=$value" 
					
				elif echo "$name" | grep -q "UNDEFINED2"; then
					echo ">>>$name=$value" 
					
				elif echo "$name" | grep -q "UNDEFINED3"; then
					echo ">>>$name=$value" 
				
				elif echo "$name" | grep -q "UNDEFINED4"; then
					echo ">>>$name=$value" 
				
				elif echo "$name" | grep -q "UNDEFINED5"; then
					echo ">>>$name=$value" 
				
				elif echo "$name" | grep -q "UNDEFINED6"; then
					echo ">>>$name=$value" 
				
				elif echo "$name" | grep -q "UNDEFINED7"; then
					echo ">>>$name=$value" 
				
				elif echo "$name" | grep -q "UNDEFINED8"; then
					echo ">>>$name=$value" 
				
				fi
				
			fi
		done
		
		
	#####保存所有配置	
	uci commit	
}


########### RUN ###############
	
##### 初始化  #####
	frist_init
	
##### 写出厂序列号  #####	
	write_default_id
	
##### 写定制出厂参数  #####	
	write_data

##### 读定制出厂参数  #####	
	read_data #> /tmp/bd_info

#echo "##### Read bin #####"
#	hexdump -C $mmc
