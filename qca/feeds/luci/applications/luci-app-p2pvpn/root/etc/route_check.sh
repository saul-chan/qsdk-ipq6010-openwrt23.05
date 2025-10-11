#!/bin/sh /etc/rc.common

. /lib/functions.sh                                                                    
. /lib/netifd/calbrdaddr.sh

START=82
NAME=parpd
PROG=${PROG:-/usr/sbin/$NAME}
CONFNAME=lanroute.conf
CONF_F="/tmp/run/$CONFNAME"
PID_F="/tmp/run/lanroute.pid"
CKPID_F="/tmp/run/routechk.pid"

config_load network
BRADDR=$(config_get lan ipaddr)
BRMASK=$(config_get lan netmask)
BRIFNAME=$(config_get lan ifname)
MASK=$(prefixlen $BRMASK)
eval "$(ipcalc.sh $BRADDR/$MASK)"
BRSUBNET="$NETWORK"
[ -x "$PROG" ] || return
PROXYIP=""
IGNRIP=""

function find_local_ip(){
	local target=$(config_get $1 target)
	[ -n "$target" ] || return 
	local ipaddr=${target%%/*}
	eval "$(ipcalc.sh $ipaddr/$MASK)"
	local subnet="$NETWORK"
	[[ "$subnet" = "$BRSUBNET" ]] && {
		PROXYIP="$PROXYIP $target"
	}
}

config_foreach find_local_ip route

find_p2pvpn_ip(){
	local routes=$(ip route list table 52| awk -F ' ' '{print$1}')
	local ipaddr
	local subnet
	for r in $routes; do
		ipaddr=${r%%/*}
		eval "$(ipcalc.sh $ipaddr/$MASK)"
		subnet="$NETWORK"
		[[ "$subnet" = "$BRSUBNET" ]] && {  
			if [ "$r" = "${r##*/}" ]; then
                		PROXYIP="$PROXYIP $r/32" 
			else
                		PROXYIP="$PROXYIP $r"  
			fi
		}
	done
}

find_p2pvpn_ip


function check_route_change(){
	local prev_proxyip="$PROXYIP"
	while [ 1 ]; do
		PROXYIP=""
		config_foreach find_local_ip route	
		find_p2pvpn_ip
		if [ "$prev_proxyip" != "$PROXYIP" ]; then
			prev_proxyip="$PROXYIP"
			/etc/init.d/lanroute start
		fi
		sleep 10s
	done
		
}

/etc/init.d/lanroute enabled && {
	check_route_change
}
