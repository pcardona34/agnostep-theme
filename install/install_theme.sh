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
### Install the Theme AGNOSTEP
### And set the User settings: defaults, .xsession...
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
STOP=1 # Set to 0 to avoid stops; to 1 to make stops
#set -v
MSG_STOP="Stop: type <Enter> to continue."
LOG=$HOME/AGNOSTEP_THEME.log
GWDEF="org.gnustep.GWorkspace"
DEFDIR=RESOURCES/DEFAULTS
DEPS="laptop-detect"
RPI=1 # By default, we assume the hw is not a RPI one. If not, it will be detected.

####################################################
### Include functions

. SCRIPTS/colors.sh
. SCRIPTS/spinner.sh
. SCRIPTS/functions_prep.sh
. SCRIPTS/functions_misc_folders.sh
. SCRIPTS/functions_inst_themes.sh
. SCRIPTS/functions_misc_themes.sh
. RELEASE

stop

echo $PATH | grep -e "/System/Tools" &>/dev/null
if [ $? -ne 0 ];then
	export PATH=/System/Tools:$PATH
fi
LOCAL_INSTALL_DIR=$(gnustep-config --variable=GNUSTEP_LOCAL_APPS)

stop

### End of functions
####################################################

STR="Installing AGNOSTEP theme ${THEME_RELEASE} - ${THEME_STATUS}"
titulo

####################################################
### Dependencies
STR="Dependencies"
subtitulo
DEPS="laptop-detect"
sudo apt -y install "$DEPS"
ok "Done"
sleep 2;clear

####################################################
### FreeDesktop User filesystem

STR="Customizing the User Home folder"
subtitulo

### Set or restore standard $HOME filesystem
### Do NOT use xdg-user-dirs-update!!!
for FOLD in Books Desktop Documents Downloads Favorites GNUstep Images Mailboxes Music Samples Videos
do
	if ! [ -d $HOME/$FOLD ];then
		mkdir -p $HOME/$FOLD
	fi
done

cd $_PWD

################################
### Miscellaneous for Home Folders
################################

################################
### ENV

STR="Miscellaneous for Home Folders"
subtitulo
###########################################
### L18N for some folders (out of the scope of XDG rules)

STR="L18N Folders"
subtitulo

l18n_folder

stop
cd $_PWD

###############################################
LG=${LANG:0:2}
case "$LG" in
"fr")
	FOLDERS="Livres Favoris GNUstep Mailboxes Exemples SOURCES";;
"en"|*)
	FOLDERS="Books Favorites GNUstep Mailboxes Samples SOURCES";;
esac

for FOLD in ${FOLDERS}
do
	printf "Setting Folder ${FOLD}\n"
	icon_folder $FOLD
	ok "Done"
done
ok "Done"
stop
cd $_PWD

###################################################
### Autostart, Xinitrc / Xsession, meteo config...
STR="Flavour, Autostart, Xsession..."
subtitulo

. SCRIPTS/user_conf_gen.sh || exit 1

cd $_PWD
set_flavour
write_xinitrc
write_autostart
write_meteo_conf

stop

cd $_PWD
ok "Done"

stop

#################################################

###################################################
### User WindowMaker profile
STR="User's WindowMaker profile"
subtitulo

### We set standard first
if [ -d $HOME/GNUstep/Library/WindowMaker ];then
	printf "Already set.\n"
else
	cd
	mkdir -p $HOME/GNUstep/Library/WindowMaker
fi
cd $_PWD
ok "Done"

stop

#################################################
### Wallpaper
STR="Wallpaper"
subtitulo

. $HOME/.config/agnostep/flavour.conf
case $FLAVOUR in
"conky")
	WP="fond_agnostep_cubes.png";;
"c5c")
	WP="fond_agnostep_waves.png";;
esac

WP_FOLDER=/usr/share/wallpapers
if [ ! -d $WP_FOLDER ];then
	sudo mkdir -p $WP_FOLDER
fi

cd RESOURCES/WALLPAPERS || exit 1
sudo cp --remove-destination ${WP} ${WP_FOLDER}/fond_agnostep.png
cd $_PWD
ok "Done"

stop

###########################################
### Installing the main AGNOSTEP theme
STR="Main AGNOSTEP Theme"
titulo

printf "Window Maker Theme...\n"
install_wm_theme
customize_clip
cd $_PWD

printf "GNUstep Theme..."
install_gs_theme
cd $_PWD

### Some Apps known to not comply with Theme: workaround
### We need to update Info-gnustep.plist for these apps
### Adding 'CFBundleIdentifier' property in the Dictionary
### Updating too Help path

update_info

cd $_PWD

stop

###

###########################################
### Setting the Defaults...
STR="GNUstep Defaults Setting"
subtitulo

################################
### Prep defaults...
################################

HOME_GNUSTEP_DEF=$HOME/GNUstep/Defaults
if [ ! -d $HOME_GNUSTEP_DEF ];then
	mkdir -p $HOME_GNUSTEP_DEF
fi

cd $DEFDIR || exit 1

is_hw_rpi
if [ $RPI -eq 0 ];then
	GWD=${GWDEF}.TEMPLATE.RPI
else
	GWD=${GWDEF}.TEMPLATE
fi

cat ${GWD} | sed -e s/patrick/$USER/g > ${GWDEF}.plist

cd $_PWD
if [ ! -f $HOME_GNUSTEP_DEF/WindowMaker ];then
	cd RESOURCES/DEFAULTS && cp WindowMaker $HOME_GNUSTEP_DEF/
fi
if [ ! -f $HOME_GNUSTEP_DEF/WMState ];then
        cd RESOURCES/DEFAULTS && cp WMState $HOME_GNUSTEP_DEF/
fi
cd $_PWD

################################
### Set the defaults
### for a AGNoStep Desktop
################################

############################################
### Applying a theme for WMaker:
printf "Applying a theme for WMaker...\n"
#### Syntax: setstyle THEME-PACK
#### (in our case: THEME-PACK is 'AGNOSTEP')

. $HOME/.config/agnostep/flavour.conf
if [ "$FLAVOUR" == "c5c" ];then
	WMSTYLE=$HOME/GNUstep/Library/WindowMaker/Themes/AGNOSTEP-C5C.themed
else
	WMSTYLE=$HOME/GNUstep/Library/WindowMaker/Themes/AGNOSTEP.themed
fi

if [ ! -d $WMSTYLE ];then
	alert "$WMSTYLE was not found!\nThis is a major issue."
	exit 1
fi

setstyle --no-cursors --no-fonts $WMSTYLE
ok "Done"

############################################
#### Applying Defaults for GNUstep
############################################

DEST=$HOME/GNUstep/Defaults
if ! [ -d $DEST ];then
	alert "$DEST was not found!"
	exit 1
fi

cd RESOURCES/DEFAULTS || exit 1

for PLIST in "Addresses" "NSGlobalDomain" "org.gap.InnerSpace" "org.gap.Terminal" "org.gnustep.GNUMail" "org.gnustep.GWorkspace" "org.poroussel.SimpleAgenda" "org.gap.AClock" "TimeMon"
do
	if [ -f ${PLIST}.plist ];then
		printf "Setting Defaults for ${PLIST}\n"
		if [ -f $DEST/${PLIST}.plist ] && [ "${PLIST}" == "org.gnustep.GNUMail" ];then
			info "We preserve GNUMail settings."
			continue;
		else
			cp --force ${PLIST}.plist $DEST/
			if [ "$PLIST" == "org.gnustep.GWorkspace" ];then
				rm -f ${PLIST}.plist
			fi
		fi
		ok "Done"
	else
		warning "The File ${PLIST}.plist was not found."
	fi
done

### Misc GWorkspace settings
. $HOME/.config/agnostep/flavour.conf
case $FLAVOUR in
"conky")
        DOCKPOS=0;;
"c5c")
        DOCKPOS=1;;
esac
defaults write org.gnustep.GWorkspace dockposition $DOCKPOS

cd $_PWD

stop

###########################################
### Installing Tools
STR="User Tools"
subtitulo

cd RESOURCES/SCRIPTS || exit 1
for TOOL in agnostep
do
	sudo cp -u $TOOL /usr/local/bin/
done
cd $_PWD
stop

cd SCRIPTS || exit 1
for TOOL in colors.sh
do
	sudo cp -u $TOOL /usr/local/bin/
done
cd $_PWD
ok "Done"

stop
###########################################

#################################################
### Installing Tools and confs... Updater
TITLE="Updater tool"
echo "$TITLE" >>$LOG
title "$TITLE"

cd TOOLS/agnostep_updater || exit 1
. ./install_agnostep_updater.sh
cd $_PWD
ok "Done"

stop
#################################################
### Installing Tools and confs... Setup_Printer
TITLE="Setup_Printer"
echo "$TITLE" >>$LOG
title "$TITLE"

cd TOOLS || exit 1
sudo cp Setup_Printer /usr/local/bin/
cd $_PWD
ok "Done"

stop
##################################################
### Installing Tools and confs... Shooting CLI
TITLE="Screenshot Shooting CLI"
echo "$TITLE" >>$LOG
title "$TITLE"

cd TOOLS || exit 1
sudo cp Shooting /usr/local/bin/
cd $_PWD
ok "Done"

stop

####################################################
### Installing Tools and confs... Conky
. $HOME/.config/agnostep/flavour.conf
if [ "$FLAVOUR" == "conky" ];then
	STR="Conky Monitoring Board"
	titulo
	cd TOOLS/agnostep_conky || exit 1
	. ./prepare_agnostep_conky.sh || exit 1
	. ./install_agnostep_conky.sh || exit 1

	cd $_PWD
	### We must complete conky symbols: icon battery
	ICOBAT=RESOURCES/ICONS/battery.png
	if [ ! -d /usr/local/share/icons/conky ];then
        	sudo mkdir -p /usr/local/share/icons/conky
	fi
	sudo cp ${ICOBAT} /usr/local/share/icons/conky/
	cd $_PWD
	ok "Done"

	##################################################
	### Installing Tools and confs... Compton
	STR="Compton Compositing"
	subtitulo
	cd TOOLS/agnostep_compton || exit 1
	. ./install_agnostep_compton.sh
	cd $_PWD
	ok "Done"
fi
stop

###########################################
### Installing Tools and confs... BirthNotify
STR="BirthNotify tool"
subtitulo

cd TOOLS/agnostep_birthday || exit 1
. ./install_birthday.sh
cd $_PWD
ok "Done"

stop

###########################################
### Theme for FocusWriter
STR="Theme for FocusWriter"
subtitulo

cd TOOLS/agnostep_fw || exit 1
. ./install_agnostep_fw.sh
cd $_PWD
ok "Done"
###########################################

####################################################
### Installing Tools and confs... Conky
. $HOME/.config/agnostep/flavour.conf
if [ "$FLAVOUR" == "c5c" ];then
	cd TOOLS/dockapps
	./install_dockapps.sh
	cd $_PWD
fi

###########################################
### Loading
sudo cp -u RESOURCES/SCRIPTS/loading.sh /usr/local/bin/

stop
###########################################




##################################################
MESSAGE="A G N o S t e p   Theme  was set."

info "$MESSAGE"

cd $_PWD
