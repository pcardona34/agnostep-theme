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
### Display title and messages
### with colors
################################

### VARS
APT_OPT="-y --no-show-upgraded --no-upgrade --show-progress --color"
SPIN='/-\|'

. ../../SCRIPTS/log.sh
. ../../SCRIPTS/colors.sh
. ../../SCRIPTS/spinner.sh

STR="D O C K    A P P S";titulo


####################################
### Dependencies

STR="Dependencies";subtitulo
sudo apt $APT_OPT install libxpm-dev libdockapp-dev libwings-dev libudisks2-dev libsecret-1-dev libgcr-3-dev librsvg2-bin
ok "Done";sleep 2;clear

###################################################################
### wmnd: for Network checking

STR="Network monitoring: wmnd";subtitulo

which -s wmnd
if [ $? -ne 0 ];then
	sudo apt $APT_OPT install wmnd
	ok "Done";sleep 2;clear
else
	ok "Yet installed."
fi

################################################################"
### wmtext: used for new Updater notification
### Call:
# wmtext -i 3600 -b 'forest green' -a 'New' crimson white updater4wmtext
### http://fccode.free.fr/dockapps

STR="Uptime, Date, Heat, Updater, Birthdays...: WMText";subtitulo

which -s wmtext
if [ $? -ne 0 ];then
	HUB=http://fccode.free.fr/dockapps
	ARCH=wmtext-3.tbz2
	if [ ! -d wmtext-3 ];then
		printf "Fetching...\n"
		wget $HUB/$ARCH
		bunzip2 $ARCH
		tar -xf ${ARCH%.tbz2}.tar && rm ${ARCH%.tbz2}.tar
		ok "Done"
	fi

	cd wmtext-3 || exit 1

	### Patch
	printf "A patch must be applied\n"
	PATCH=../wmtext.c.patch
	TARGET=src/wmtext.c
	patch --forward -u $TARGET -i $PATCH
	ok "Done"

	printf "Building...\n"
	if [ -f Makefile ] && [ -f wmtext ];then
		make clean &>/dev/null
	fi

	make &>>$LOG &
	PID=$!
	spinner
	ok "\rDone"

	printf "Installing...\n"
	sudo -E make install &>>$LOG
	ok "Done"

	printf "wmtext Scripts:\n"
	cd ..
	cd wmtext_scripts || exit 1
	for SCRIPT in *4wmtext
	do
	echo "- $SCRIPT"
	sudo -E cp $SCRIPT /usr/local/bin/
	sudo -E chmod 755 /usr/local/bin/$SCRIPT
	done
	cd ..
	ok "Done"
	sleep 2;clear
else
	ok "Yet installed."
fi

####################################################
### Mountapp

STR="WMudmount";subtitulo

which -s wmudmount
if [ $? -ne 0 ];then
	### Fetching
	if [ ! -d wmudmount ];then
		printf "Fetching WMudmount source...\n"
		WMUDMOUNT=https://sourceforge.net/projects/wmudmount/files/wmudmount/wmudmount-3.0.tar.gz
		wget $WMUDMOUNT && gunzip --force wmudmount-3.0.tar.gz && tar -xf wmudmount-3.0.tar && rm wmudmount-3.0.tar
		mv wmudmount-3.0 wmudmount
	fi
	cd wmudmount || exit 1
	### To avoid the config.guess type error:
	### We must update config.guess... and  config.sub

	printf "Updating config.guess and config.sub...\n"
	wget --quiet -O config.guess 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'

	wget --quiet -O config.sub 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'
	ok "Done"

	printf "Configuring...\n"
	./configure &>>$LOG &
	PID=$!
	spinner
	ok "\rDone"

	printf "Building...\n"
	if [ ! -f Makefile ];then
		alert "Aborting..."
		exit 1
	fi
	make &>>$LOG &
	PID=$!
	spinner
	ok "\rDone"

	printf "Installing...\n"
	sudo -E make install &>>$LOG &
	PID=$!
	spinner
	ok "\rDone"
	cd ..
else
	ok "Yet installed."
fi
################################################

STR="Post-install: cleaning...";subtitulo
for DIR in wmtext-3 wmudmount
do
rm -fr $DIR
done
ok "Done"

#info "DockApps: all was done."
sleep 2
