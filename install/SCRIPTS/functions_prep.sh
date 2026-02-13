#!/bin/bash

####################################################
### A G N o S t e p  -  Theme - by Patrick Cardona
### pcardona34 @ Github
###
### Thanks for the GNUstep Developers Community
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

################################
### Functions for install_theme.sh
################################

#############################################
### SPEC Raspberry Pi
### Is the current hardware a Raspberry Pi?
### If yes, it has a specific Device Tree
### By default, no RPI:
### means: RPI=1

function is_hw_rpi
{
### Arm architecture 64 bit?
ARCH=`uname -m`
printf "Architecture: ${ARCH}\n"

if [ "$ARCH" == "aarch64" ];then
	if [ -f /sys/firmware/devicetree/base/model ];then
		MODEL=`head --bytes=-1 /proc/device-tree/model`
		echo "$MODEL" | grep -e "Raspberry Pi" &>/dev/null
		if [ $? -eq 0 ];then
			printf "Your model is a RPI: $MODEL\n"
			RPI=0
		else
			printf "Not a Raspberry Pi.\n"
		fi
	else
		printf "Model: no Device Tree Model info\n"
	fi
fi
}
