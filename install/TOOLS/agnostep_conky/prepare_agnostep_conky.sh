#!/bin/bash

####################################################
### A G N o S t e p  -  Theme - by Patrick Cardona
### pcardona34 @ Github
###
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

################################
### Create the Conky conf file
################################

################################
### VARS

HERE=`pwd`
TRANS=${LANG:0:2}
. ../../SCRIPTS/functions_prep.sh
RPI=1
FOOT=foot.txt

function restore_foot_template
{
if [ -f ${FOOT%.txt}.bak ];then
	mv -f ${FOOT%.txt}.bak ${FOOT}
fi
}

function guess_ifname
{
IFNAME=$(ls -1 /sys/class/net | grep -e "enp")
if [ -z "$IFNAME" ];then
	error "Interface not guessed"
	exit 1
else
	ok "Interface: $IFNAME";sleep 2
	cp $FOOT ${FOOT%.txt}.bak
	sed -i "s/enp/${IFNAME}/g" $FOOT
	#less $FOOT
fi
}

###################################################
function assemble
{
laptop-detect
if [ $? -eq 0 ];then
	cat head.txt battery.txt $FOOT >> conky.conf.$TRANS
else
	cat head.txt $FOOT >> conky.conf.$TRANS
fi
}
###################################################

is_hw_rpi
if [ $RPI -eq 0 ];then
	FOOT=foot.rpi.txt
fi

if [ -n "$TRANS" ];then
	case "$TRANS" in
		"fr")
			cd fr || exit 1
			guess_ifname
			assemble
			restore_foot_template
			mv conky.conf.fr ../conky.conf
			cd ..
			printf "The file 'conky.conf' has been generated\n";;
		"en" | *)
			cd en || exit 1
			guess_ifname
			assemble
			restore_foot_template
			mv conky.conf.en ../conky.conf
			cd ..
			printf "The file 'conky.conf' has been generated\n";;
	esac
else
	printf "The LANG was not guessed. Aborting.\n"
	exit 1
fi
