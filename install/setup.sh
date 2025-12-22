#!/bin/bash

####################################################
### A G N o S t e p  Theme - by Patrick Cardona
### pcardona34 @ Github
###
### Thanks for the GNUstep Developers Community
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

####################################################
### Setup the Installation of the Theme AGNOSTEP
####################################################

clear

function stop
{
if [ $STOP -ne 0 ];then
	read -p "$MSG_STOP" R
fi
}

_PWD=`pwd`
SPIN='\-/|'
STOP=0 # Set to 0 to avoid stops; to 1 to make stops for debugging purpose
#set -v
MSG_STOP="Stop: type <Enter> to continue."
LOG=$HOME/AGNOSTEP_SETUP.log

####################################################
### Include functions

. SCRIPTS/colors.sh
. SCRIPTS/spinner.sh
. RELEASE

### End of functions
####################################################

STR="Setup of AGNOSTEP theme ${THEME_RELEASE} - ${THEME_STATUS}"
titulo

####################################################
### Dependencies
STR="Dependencies"
subtitulo
DEPS="dialog"
sudo apt -y install ${DEPS}
ok "Done"
sleep 2;clear

####################################################
### Is a previous Setup there?
####################################################
SETUP=$HOME/.config/agnostep/theme.conf
if [ -f $SETUP ];then
	echo "Already set"
else
	echo "No theme set."
fi
