#!/bin/bash

####################################################
### A G N o S t e p  -  Updater - by Patrick Cardona
### pcardona34 @ Github
###
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

################################
### Install the Updater Tool
################################

################################
### VARS

AUTO=$HOME/GNUstep/Library/WindowMaker/autostart
ICON_DEST=/usr/local/share/icons
BIN_DEST=/usr/local/bin
DEPS="suckless-tools"
DUNSTRC=$HOME/.config/dunst
TEMPFILE=$(mktemp /tmp/agno-XXXXX)
trap "rm $TEMPFILE" EXIT

sudo apt -y install ${DEPS}

if [ ! -d $BIN_DEST ];then
	sudo mkdir -p $BIN_DEST
fi

for SCRIPT in Updater Upgrade
do
	sudo cp -u ${SCRIPT} /usr/local/bin/
	sudo chmod u+s /usr/local/bin/${SCRIPT}
done

###  Copy of dunstrc
if ! [ -d $DUNSTRC ];then
	mkdir -p $DUNSTRC
fi
cp _dunstrc $DUNSTRC/dunstrc

if [ ! -d $ICON_DEST ];then
	mkdir -p $ICON_DEST
fi
sudo cp -u bell.tif $ICON_DEST/

### Crontab
## Root
sudo crontab -u root -l > $TEMPFILE
grep "apt --update" $TEMPFILE
if [ $? -ne 0 ];then
	sudo crontab -u root root_crontab.txt
fi
## USER
crontab -u $USER -l > $TEMPFILE
grep "Updater" $TEMPFILE
if [ $? -ne 0 ];then
	crontab crontab.txt
fi

### Misc appearence of XTerm
### This maybe already set by agnostep-desktop
XRCONF=$HOME/.Xresources
if [ -f $XRCONF ];then
	grep -e "XTerm" $XRCONF &>/dev/null
	if [ $? -eq 0 ];then
		MSG="Xresources already set for XTerm."
		if [ -n "$(LC_ALL=C type -t info)" ] && [ "$(LC_ALL=C type -t info)" == "function" ];then
			info "$MSG"
		else
			printf "${MSG}\n"
		fi
	else
		printf "We must update Xresources...\n"
		cat Xresources >> $XRCONF
		ok "Done"
	fi
else
	cp Xresources $XRCONF
	MSG="Xresources was set for XTerm."
	if [ -n "$(LC_ALL=C type -t info)" ] && [ "$(LC_ALL=C type -t info)" == "function" ];then
		info "$MSG"
	else
		printf "${MSG}\n"
	fi

fi

if [ -n "$DISPLAY" ];then
	# We try to update now the Xressources
	# If it fails, it will be done in the next X session
	xrdb -merge $HOME/.Xresources
fi

printf "\nUpdater monitor has been set.\n"
