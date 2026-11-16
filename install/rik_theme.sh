#!/bin/bash

####################################################
### A G N o S t e p  -  Desktop - by Patrick Cardona
### pcardona34 @ Github
###
### Thanks for the GNUstep Developers Community
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

####################################################
### Uninstall AGNOSTEP Theme if present
### This set Rik Theme...
####################################################

sudo -v
if [ $? -ne 0 ];then
	exit 1
fi

clear

#_PWD=`pwd`
SPIN='\-/|'
STOP=0 # Set to 0 to avoid stops; to 1 to make stops
SLEEP=2
#set -v
#MSG_STOP="Stop: type <Enter> to continue."
LG=${LANG:0:2}

case "$LG" in
"fr")
	SAMPLE_FOLDER="Exemples"
	FAVORITE_FOLDER="Favoris"
	BOOKS_FOLDER="Livres";;
"en"|*)
	SAMPLE_FOLDER="Samples"
	FAVORITE_FOLDER="Favorites"
	BOOKS_FOLDER="Books";;
esac

####################################################
### Include functions

. SCRIPTS/log.sh
. SCRIPTS/colors.sh
. SCRIPTS/spinner.sh
. SCRIPTS/stop.sh
. SCRIPTS/check_app.sh
. SCRIPTS/std_build.sh
. SCRIPTS/functions_inst_themes.sh

### End of functions
####################################################

####################################################
### DO NOT RUN under an existent X session
####################################################

if [ -n "$DISPLAY" ];then
        alert "You should not uninstall this theme within an XTERM!"
        info "Logout the Graphical session, open a tty console and try again."
        exit 1
fi


#cd $_PWD

clear

reset_theme

info "The Rik theme will apply.\n"
sleep ${SLEEP:-2}
. SCRIPTS/inst_rik.sh

#######################################

if [ -z "$SINGLE_THEME" ];then

    MESSAGE="Rik Theme has been set."

    info "$MESSAGE"

    LEVEL_STATUS=$(systemctl get-default)
    if [ "${LEVEL_STATUS}" == "graphical.target" ];then
        echo "Graphical Target is already set."
    else
        warning "If you are not using a Display Manager, You need to logout and login again to apply the changes..."
        MSG="Seconds before logout: "
        DELAY=6
        timer
        exec SCRIPTS/lo.sh
    fi
fi
