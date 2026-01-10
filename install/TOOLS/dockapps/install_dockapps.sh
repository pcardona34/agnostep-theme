#!/bin/bash

### VARS
LOG="$HOME/DOCKAPPS.log"
if [ -f $LOG ];then
	rm $LOG
fi
APT_OPT="-y --no-show-upgraded --no-upgrade --show-progress --color"
SPIN='/-\|'

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
sudo apt $APT_OPT install wmnd
ok "Done";sleep 2;clear

################################################################"
### wmtext: used for new Updater notification
### Call:
# wmtext -i 3600 -b 'forest green' -a 'New' crimson white updater4wmtext

STR="Uptime, Date, Heat, Updater, Birthdays...: WMText";subtitulo

cd wmtext || exit 1

if [ -f Makefile ] && [ -f wmtext ];then
	make clean &>/dev/null
fi

printf "Building...\n"
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

####################################################
### Mountapp

STR="WMudmount";subtitulo

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

################################################

info "DockApps: all was done."
