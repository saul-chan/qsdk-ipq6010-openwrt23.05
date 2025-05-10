#!/bin/sh
#
# Copyright (c) 2021 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

# QCC710 v1.0 reset for BT bringup
qcc710_reset() {
	if [[ -e /proc/device-tree/soc/pinctrl@1000000/QCC710_pins/QCC710_reset/pins ]]; then
		reset_gpio_pin=$(cat /proc/device-tree/soc/pinctrl@1000000/QCC710_pins/QCC710_reset/pins | sed s/"gpio"//)
	elif [[ -e /proc/device-tree/soc@0/pinctrl@1000000/QCC710_pins/QCC710_reset/pins ]]; then
		reset_gpio_pin=$(cat /proc/device-tree/soc@0/pinctrl@1000000/QCC710_pins/QCC710_reset/pins | sed s/"gpio"//)
	else
		return
	fi
	gpio_base=$(cat /sys/class/gpio/gpiochip*/base | head -n1)
	gpio_reset=$(( gpio_base + reset_gpio_pin ))
	if [[ ! -e /sys/class/gpio/gpio$gpio_reset ]]; then
                [ -z ${SLEEP} ] && echo -e "Enter sleep value for reset. Options:\n10 \n1" && read -p "Enter : "  SLEEP
                [ -z ${SLEEP} ] && SLEEP=10
		echo $gpio_reset > /sys/class/gpio/export
		echo out > /sys/class/gpio/gpio$gpio_reset/direction
		echo "Performing QCC710 reset ...." > /dev/console
		{ echo 1 > /sys/class/gpio/gpio$gpio_reset/value ; \
			sleep $SLEEP; \
			echo 0 > /sys/class/gpio/gpio$gpio_reset/value; \
			echo "QCC710 reset complete ...." > /dev/console; }
		sleep 1
	fi
}

while [ -n "$1" ]; do
	case "$1" in
		-h|--help) HELP=1; break;;
		-b|--baudrate) BAUDRATE="$2";shift;;
		-s|--sleep) SLEEP="$2";shift;;
		-*)
			echo "Invalid option: $1"
			ERROR=1;
			break
		;;
		*)break;;
	esac
	shift
done

[ -n "$HELP" -o -n "$ERROR" ] && {
	        cat <<EOF
Usage: $0 [-h] [-b baudrate] [-s sleep]

bt_qcc710_start options:
	-h	print this help
	-b      baudrate
	-s	sleep

Example:
bt_qcc710_start -b <baudrate> -s <sleep value>

version 1 : ./sbin/bt_qcc710_start -b 2000000 -s 10
version 2 : ./sbin/bt_qcc710_start -b 115200 -s 1
EOF
	# If we requested the help flag, then exit normally.
	# Else, it's probably an error so report it as such.
	[ -n "$HELP" ] && exit 0
	exit 1
}

qcc710_reset
BLUETOPIA_PID=$(ps | grep LinuxSPPLE | grep -v grep | awk '{print $1}')
if [ -n "$BLUETOPIA_PID" ]; then
	kill -s SIGTERM $BLUETOPIA_PID
	echo "Stopped LinuxSPPLE process $BLUETOPIA_PID"
fi
[ -z ${BAUDRATE} ] && echo -e "Enter baudrate for stack bringup. Options:\n2000000\n115200" && read -p "Enter : "  BAUDRATE
[ -z ${BAUDRATE} ] && BAUDRATE=2000000
./usr/bin/LinuxSPPLE 1 /dev/ttyMSM1 $BAUDRATE
