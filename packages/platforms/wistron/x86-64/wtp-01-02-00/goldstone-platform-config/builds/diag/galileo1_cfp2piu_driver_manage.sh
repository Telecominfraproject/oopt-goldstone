#!/bin/bash

## Project: Galileo 1
## File Name: galileo1_cfp2piu_driver_manage.sh
## Description: To unload and load cfp2piu driver for running DCO-PIU MCU firmware upgrade tool or utility.
## History: 2021/4/1 Created.

stop() {
	echo "Stop tai service, onlp service and cfp2piu driver."
	## Stop tai service
	systemctl stop tai.service
	## Stop onlpd service
	systemctl stop onlpd.service
	## unload cfp2piu driver
	rmmod cfp2piu
	echo "Done."
}

start() {
	echo "Start tai service, onlp service and cfp2piu driver."
	## Load cfp2piu driver
	modprobe cfp2piu
	## Start onlpd service
	systemctl start onlpd.service
	## Start tai service
	systemctl start tai.service
	## Start goldstone management service
	systemctl start gs-mgmt.service
	echo "Done."
}

check() {
	echo "Check tai containers, onlp daemon and cfp2piu driver."
	## Check cfp2piu driver
	lsmod | grep cfp2piu
	## Check onlp daemon
	ps -aux | grep onlpd
	## Check tai container
	kubectl get pods | grep tai
}

main() {
	case "$1" in
		start|stop|check)
			$1
			;;
		*)
			echo "Usage: $0 {start|stop|check}"
			exit 1
			;;
	esac
}

main $*
