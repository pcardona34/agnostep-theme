#!/bin/bash

####################################################
### AGNoStep  agnostpe-theme
### (c) 2026 Patrick Cardona
###
### Thanks for the GNUstep Developers Community
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

####################################################
### Install a theme for the AGNoStep Desktop
####################################################

####################################################
### You must be superuser to install
sudo -v
if [ $? -ne 0 ];then
	exit 1
fi

clear

SPIN='\-/|'
STOP=0 # Set to 0 to avoid stops; to 1 to make stops for debugging purpose
SLEEP=4
#set -v
GWDEF="org.gnustep.GWorkspace"
DEFDIR=RESOURCES/DEFAULTS
RPI=1 # By default, we assume the hw is not a RPI one. If not, it will be detected.
TEMPFILE=$(mktemp /tmp/agno-XXXXX)
trap "rm -f $TEMPFILE" EXIT

LG=${LANG:0:2}

####################################################
### Include functions

. SCRIPTS/log.sh
. SCRIPTS/colors.sh
. SCRIPTS/spinner.sh
. SCRIPTS/stop.sh
. SCRIPTS/functions_inst_themes.sh
. SCRIPTS/functions_misc_themes.sh

####################################################
### DO NOT RUN under an existent X session
####################################################

if [ -n "$DISPLAY" ];then
	alert "You should not install within an XTERM!"
	info "Logout the Graphical session, open a tty console and try again."
	exit 1
fi

### PATH

which -s gnustep-config
if [ $? -ne 0 ];then
	alert "Your GNUstep System seems not correctly set. Aborting!"
	info "You should log out and back in to update your environment. When it will be done, try again the 'Settings' stage."
	MSG="Seconds before logout: "
	DELAY=8
	timer
	exec SCRIPTS/lo.sh
	exit 1
fi

export APP_DIR=`gnustep-config --variable=GNUSTEP_LOCAL_APPS`
export GNUSTEP_SYSTEM_TOOLS=`gnustep-config --variable=GNUSTEP_SYSTEM_TOOLS`

### End of functions
####################################################

####################################################
### Install the theme
####################################################

. SCRIPTS/splash.sh

STR="Checking your Desktop env..."
titulo

check_deps || exit 1
check_apps || exit 1

clear

STR="Setting a theme..."
titulo

menu_theme_choice

cd "$PWD"

stop

##################################################

MESSAGE="Theme was set."
echo "$MESSAGE" >> $LOG
date >> $LOG
cd $_PWD
