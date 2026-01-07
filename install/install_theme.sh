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
### accordingly.
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
LOG=$HOME/AGNOSTEP_THEME.log
GWDEF="org.gnustep.GWorkspace"
DEFDIR=RESOURCES/DEFAULTS
RPI=1 # By default, we assume the hw is not a RPI one. If not, it will be detected.
TEMPFILE=$(mktemp /tmp/agno-XXXXX)
trap "rm -f $TEMPFILE" EXIT

####################################################
### Include functions

. SCRIPTS/colors.sh
. SCRIPTS/spinner.sh
. SCRIPTS/functions_prep.sh
. SCRIPTS/functions_misc_folders.sh
. SCRIPTS/functions_inst_themes.sh
. SCRIPTS/functions_misc_themes.sh
. RELEASE

####################################################
### DO NOT RUN under an existent X session
####################################################

if [ -n "$DISPLAY" ];then
	alert "You should not install this theme within an XTERM!"
	info "Logout the Graphical session, open a tty console and try again."
	exit 1
fi

### PATH

whereis gnustep-config
if [ $? -eq 0 ];then
	LOCAL_INSTALL_DIR=$(gnustep-config --variable=GNUSTEP_LOCAL_APPS)
	GNUSTEP_SYSTEM_TOOLS=$(gnustep-config --variable=GNUSTEP_SYSTEM_TOOLS)
	echo $PATH | grep -e "${GNUSTEP_SYSTEM_TOOLS}" &>/dev/null
	if [ $? -ne 0 ];then
		export PATH=${GNUSTEP_SYSTEM_TOOLS}:$PATH
	fi
else
	alert "Your GNUstep System seems not correctly set. Aborting!"
fi

### End of functions
####################################################

STR="Installing AGNOSTEP theme ${THEME_RELEASE} - ${THEME_STATUS}"
titulo

####################################################
### Dependencies
STR="Dependencies"
subtitulo
DEPS="laptop-detect picom dunst dialog"
sudo apt -y install ${DEPS}
ok "Done"
sleep 2;clear

####################################################
### FreeDesktop User filesystem

STR="Customizing the User Home folder"
subtitulo

### Set or restore standard $HOME filesystem
### Do NOT use xdg-user-dirs-update!!!

LG=${LANG:0:2}
case "$LG" in
"fr")
	# Some folders are auto-translated by GNUstep, others none: we only care about the others;
	# The same is for the folders icons...
	FOLDERS="Livres Desktop Documents Downloads Favoris GNUstep Aide Images Mailboxes Music Exemples SOURCES Videos";;
"en"|*)
	FOLDERS="Books Desktop Documents Downloads Favorites GNUstep Help Images Mailboxes Music Samples SOURCES Videos";;
esac

### We manage the case of existing folders in English or in French
l18n_folder

stop

for FOLD in ${FOLDERS}
do
	printf "Setting Folder ${FOLD}\n"
	if [ ! -d ${FOLD} ];then
		mkdir -p $HOME/${FOLD}
	fi
	icon_folder $FOLD
	ok "Done"
done
sleep 2
stop

cd $_PWD
###############################################

STR="Compositor"
cd TOOLS/agnostep_picom
. install_agnostep_picom.sh || exit 1
install_picom
cd $_PWD
stop

###################################################
### Autostart, Xinitrc / Xsession, meteo config...
STR="Flavour, Autostart, Xsession..."
subtitulo

. SCRIPTS/functions_goodies.sh || exit 1
. SCRIPTS/user_conf_gen.sh || exit 1

cd $_PWD
set_flavour

stop

write_xinitrc
cp $HOME/.xinitrc $HOME/.xsession

stop

write_autostart

stop

write_meteo_conf

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

. $HOME/.config/agnostep/flavour.conf || exit 1
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

#printf "Window Maker Theme...\n"
#install_wm_theme
#stop

printf "GNUstep Theme..."
install_gs_theme

cd $_PWD
stop

### Clip
. $HOME/.config/agnostep/flavour.conf || exit 1
if [ "$FLAVOUR" == "c5c" ];then
	customize_clip
fi
cd $_PWD
stop

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

cat ${GWD} | sed -e s/patrick/$USER/g | sed -e s#/Local/Applications#${LOCAL_INSTALL_DIR}#g > ${GWDEF}.plist

cd $_PWD
#if [ ! -f $HOME_GNUSTEP_DEF/WindowMaker ];then
#	cd RESOURCES/DEFAULTS && cp WindowMaker $HOME_GNUSTEP_DEF/
#fi



. $HOME/.config/agnostep/flavour.conf || exit 1
case $FLAVOUR in
"conky")
	WMSTATE="WMState_conky";;
"c5c"|*)
	if [ -d $HOME/GNUstep/Library/WindowMaker/CachedPixmaps ];then
		cd $HOME/GNUstep/Library/WindowMaker/CachedPixmaps
		rm -f *.xpm
		cp $_PWD/RESOURCES/THEMES/c5c_cachedpixmaps.tar.gz ./
		gunzip --force c5c_cachedpixmaps.tar.gz
		tar -xf c5c_cachedpixmaps.tar && rm c5c_cachedpixmaps.tar
		cd $_PWD
	fi
	WMSTATE="WMState_c5c";;
esac

cd RESOURCES/DEFAULTS
if [ ! -f $WMSTATE ];then
	alert "The file $WMSTATE was not found. This is a major issue."
	exit 1
else
	cp $WMSTATE $HOME_GNUSTEP_DEF/WMState
	### Setting to the user env
	cat $HOME_GNUSTEP_DEF/WMState | sed -e s#/System/Tools#${GNUSTEP_SYSTEM_TOOLS}#g > $TEMPFILE
	cat $TEMPFILE > $HOME_GNUSTEP_DEF/WMState
fi
cd $_PWD
stop

################################
### Set the defaults
### for a AGNoStep Desktop
################################

############################################
### Applying a style for WMaker:
printf "Applying a Style to WMaker...\n"
#### Syntax: setstyle style.style

STYLE=RESOURCES/THEMES/wmaker.style
if [ ! -f $STYLE ];then
	alert "$STYLE was not found!\nThis is a major issue."
	exit 1
fi

setstyle --no-cursors --no-fonts $STYLE
ok "Done"
stop

############################################
#### Applying Defaults for GNUstep
############################################

DEST=$HOME/GNUstep/Defaults
if ! [ -d $DEST ];then
	alert "$DEST was not found!"
	exit 1
fi

stop

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
		stop
	else
		warning "The File ${PLIST}.plist was not found."
		stop
	fi
done

### Misc GWorkspace settings
. $HOME/.config/agnostep/flavour.conf || exit 1
case $FLAVOUR in
"conky")
        DOCKPOS=0
	HIDE=0;;
"c5c")
        DOCKPOS=1
	HIDE=1;;
esac
defaults write org.gnustep.GWorkspace hidedock $HIDE
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

####################################################
### Installing Tools and confs... Conky
. $HOME/.config/agnostep/flavour.conf || exit 1
if [ "$FLAVOUR" == "conky" ];then
	STR="Conky Monitoring Board"
	titulo
	cd TOOLS/agnostep_conky || exit 1
	. ./prepare_agnostep_conky.sh || exit 1
	. ./install_agnostep_conky.sh || exit 1

stop

	cd $_PWD
	### We must complete conky symbols: icon battery
	ICOBAT=RESOURCES/ICONS/battery.png
	if [ ! -d /usr/local/share/icons/conky ];then
        	sudo mkdir -p /usr/local/share/icons/conky
	fi
	sudo cp ${ICOBAT} /usr/local/share/icons/conky/
	cd $_PWD
	ok "Done"
fi
stop

####################################################
### Installing Tools and confs... Dockapps
### for C5C flavour
. $HOME/.config/agnostep/flavour.conf || exit 1
if [ "$FLAVOUR" == "c5c" ];then
	cd TOOLS/dockapps
	./install_dockapps.sh
	cd $_PWD
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
stop

###########################################

###########################################
### Loading notification
sudo cp -u RESOURCES/SCRIPTS/loading.sh /usr/local/bin/

stop
###########################################




##################################################
MESSAGE="A G N o S t e p   Theme  was set."

info "$MESSAGE"

cd $_PWD
