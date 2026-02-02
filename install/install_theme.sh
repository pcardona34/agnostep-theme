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

sudo -v
if [ $? -ne 0 ];then
	exit 1
fi

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
SLEEP=1
#set -v
MSG_STOP="Stop: type <Enter> to continue."
GWDEF="org.gnustep.GWorkspace"
DEFDIR=RESOURCES/DEFAULTS
RPI=1 # By default, we assume the hw is not a RPI one. If not, it will be detected.
TEMPFILE=$(mktemp /tmp/agno-XXXXX)
trap "rm -f $TEMPFILE" EXIT

####################################################
### Include functions

. SCRIPTS/log.sh
. SCRIPTS/colors.sh
. SCRIPTS/spinner.sh
. SCRIPTS/functions_prep.sh
. SCRIPTS/functions_misc_folders.sh
. SCRIPTS/functions_inst_themes.sh
. SCRIPTS/functions_misc_themes.sh
. SCRIPTS/meteo_form.sh
. RELEASE.txt

####################################################
### DO NOT RUN under an existent X session
####################################################

if [ -n "$DISPLAY" ];then
	alert "You should not install this theme within an XTERM!"
	info "Logout the Graphical session, open a tty console and try again."
	exit 1
fi

### PATH

which -s gnustep-config
if [ $? -eq 0 ];then
	export APP_DIR=`gnustep-config --variable=GNUSTEP_LOCAL_APPS`
	export GNUSTEP_SYSTEM_TOOLS=`gnustep-config --variable=GNUSTEP_SYSTEM_TOOLS`
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
sleep $SLEEP ;clear

###################################################
### GNUstep apps needed (1/2)
STR="Checking installed apps (1)";titulo

APPS="GWorkspace SimpleAgenda Terminal Ink GNUMail InnerSpace"
for APP in ${APPS}
do
	echo -e "Looking for: ${APP_DIR}/${APP}.app"
	if [ -d ${APP_DIR}/${APP}.app ];then
		info "$APP has been found."
	else
		alert "$APP was not found. Install it before, please."
		exit 1
	fi
done
ok "Done"
sleep $SLEEP 

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
	FOLDERS="Livres Bookshelf Desktop Documents Downloads Favoris GNUstep Aide Images Mailboxes Music Exemples SOURCES Videos";;
"en"|*)
	FOLDERS="Books Bookshelf Desktop Documents Downloads Favorites GNUstep Help Images Mailboxes Music Samples SOURCES Videos";;
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
sleep $SLEEP
stop

cd $_PWD

###############################################

STR="Customizing Subfolders in Applications folder"
subtitulo

APP_DIR=$(gnustep-config --variable=GNUSTEP_LOCAL_APPS)

LG=${LANG:0:2}
case "$LG" in
"fr")
	FOLDERS="Jeux Prog Utilitaires";;

"en"|*)
	FOLDERS="Dev Games Utilities";;
esac

for FOLD in ${FOLDERS}
do
	printf "Setting Folder ${FOLD}\n"
	cd $APP_DIR || exit 1
	if [ -d ${FOLD} ];then
		app_sub_icon_folder "$FOLD"
		ok "Done"
	fi
done
sleep $SLEEP
stop

cd $_PWD

###############################################

STR="Compositor";subtitulo

cd TOOLS/agnostep_picom
. install_agnostep_picom.sh || exit 1
install_picom
cd $_PWD

stop

###################################################
### Autostart, Xinitrc / Xsession, meteo config...
STR="Flavour, Autostart, Xsession..."
subtitulo
sleep $SLEEP

#. SCRIPTS/functions_goodies.sh || exit 1
. SCRIPTS/user_conf_gen.sh || exit 1

sleep $SLEEP

cd $_PWD
set_flavour

###################################################
### GNUstep apps needed (2/2)
. $HOME/.config/agnostep/flavour.conf || exit 1

function more_apps
{
STR="Checking installed apps (2)";titulo
APPS="AClock TimeMon"
laptop-detect
if [ $? -eq 0 ];then
   	APPS="$APPS batmon"
fi
for APP in ${APPS}
do
	echo -e "Looking for ${APP_DIR}/${APP}.app"
       	if [ -d ${APP_DIR}/${APP}.app ];then
		info "$APP has been found."
       	else
               	alert "$APP was not found. Install it before, please."
               	exit 1
       	fi
done
}

if [ "$FLAVOUR" == "c5c" ];then
	more_apps
	#ok "Done"
	sleep $SLEEP
fi

stop

printf "Xsession...\n"
write_xinitrc
cp RESOURCES/SCRIPTS/xprofile $HOME/.xprofile
cp $HOME/.xinitrc $HOME/.xsession

ok "Done"
sleep $SLEEP 

stop

printf "Autostart...\n"
write_autostart
ok "Done"
sleep $SLEEP 

stop

##################################################
### Meteo settings

write_meteo_conf

cd $_PWD

stop

#################################################

###################################################
### User WindowMaker profile
clear
STR="User's WindowMaker profile"
subtitulo

### We set standard first
if [ -d $HOME/GNUstep/Library/WindowMaker ];then
	printf "GNUstep Folder already set.\n"
else
	cd
	mkdir -p $HOME/GNUstep/Library/WindowMaker
fi
cd $_PWD
ok "Done"
sleep $SLEEP 

stop

###########################################
### Installing the main AGNOSTEP theme
STR="Main AGNOSTEP Theme"
titulo

#printf "GNUstep Theme..."
install_gs_theme

cd $_PWD
sleep $SLEEP 

stop

### Clip
. $HOME/.config/agnostep/flavour.conf || exit 1
if [ "$FLAVOUR" == "c5c" ];then
	printf "Clip Icon...\n"
	customize_clip
	ok "Done";sleep $SLEEP 
fi
cd $_PWD
stop

### Some Apps known to not comply with Theme: workaround
### We need to update Info-gnustep.plist for these apps
### Adding 'CFBundleIdentifier' property in the Dictionary
### Updating too Help path

update_info

cd $_PWD
sleep 3
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
if [ ! -d ${HOME_GNUSTEP_DEF} ];then
	mkdir -p ${HOME_GNUSTEP_DEF}
fi

cd $DEFDIR || exit 1

is_hw_rpi
if [ $RPI -eq 0 ];then
	GWD=${GWDEF}.TEMPLATE.RPI
else
	GWD=${GWDEF}.TEMPLATE
fi

cat ${GWD} | sed -e s/patrick/$USER/g | sed -e s#/Local/Applications#${APP_DIR}#g > ${GWDEF}.plist

cd $_PWD
if [ ! -f $HOME_GNUSTEP_DEF/WindowMaker ];then
	cd RESOURCES/DEFAULTS && cp WindowMaker $HOME_GNUSTEP_DEF/
	cd $_PWD
fi

. $HOME/.config/agnostep/flavour.conf || exit 1
case $FLAVOUR in
"conky")
	WMSTATE="WMState_conky";;
"c5c"|*)
	cp --force $_PWD/RESOURCES/DEFAULTS/WMWindowAttributes $HOME/GNUstep/Defaults/
	if [ -d $HOME/GNUstep/Library/WindowMaker/CachedPixmaps ];then
		cd $HOME/GNUstep/Library/WindowMaker
		rm -fR CachedPixmaps
	fi
	### CachedPixmaps ###
	cd $_PWD/RESOURCES/THEMES
	cp -a CachedPixmaps_c5c $HOME/GNUstep/Library/WindowMaker/
	mv $HOME/GNUstep/Library/WindowMaker/CachedPixmaps_c5c $HOME/GNUstep/Library/WindowMaker/CachedPixmaps
	cd $_PWD
	### Laptop? ###
	laptop-detect
	if [ $? -eq 0 ];then
		WMSTATE="WMState_laptop"
		cp $_PWD/RESOURCES/ICONS/batmon.GNUstep.xpm $HOME/GNUstep/Library/WindowMaker/CachedPixmaps/
	else
		WMSTATE="WMState_c5c"
	fi;;
esac

cd RESOURCES/DEFAULTS
if [ ! -f $WMSTATE ];then
	alert "The file $WMSTATE was not found. This is a major issue."
	exit 1
else
	cp --force $WMSTATE $HOME_GNUSTEP_DEF/WMState
	### Setting to the user env
	cat $HOME_GNUSTEP_DEF/WMState | sed -e s#/System/Tools#${GNUSTEP_SYSTEM_TOOLS}#g > $TEMPFILE
	cat $TEMPFILE > $HOME_GNUSTEP_DEF/WMState
fi
cd $_PWD
ok "Done"
sleep $SLEEP

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
sleep $SLEEP 
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
ok "Done"
sleep $SLEEP 
stop

#################################################
### Wallpaper
STR="Generic Wallpaper"
subtitulo

. $HOME/.config/agnostep/flavour.conf || exit 1
case $FLAVOUR in
"conky")
	WP="fond_agnostep_cubes.png";;
"c5c")
	WP="fond_agnostep_waves.png";;
esac

### The following folder will be searched too by Lightdm
WP_FOLDER=/usr/local/share/wallpapers
if [ ! -d $WP_FOLDER ];then
	sudo mkdir -p $WP_FOLDER
fi

cd RESOURCES/WALLPAPERS || exit 1
sudo cp -f ${WP} ${WP_FOLDER}/fond_agnostep.png

DEFGW=org.gnustep.GWorkspace.plist
cd $HOME/GNUstep/Defaults || exit 1
sed -i -r "s#.*wallpapers.*#<string>$WP_FOLDER/fond_agnostep.png</string>#" $DEFGW

cd $_PWD
ok "Done"
sleep $SLEEP

#################################################
### User Wallpaper
STR="User Wallpaper Collection"
subtitulo

function is_wprotate
{
dialog --no-shadow --erase-on-exit --backtitle "Wallpaper" \
 --title "Rotation ask" \
 --yesno "
Do you want to rotate the wallpaper 
from a collection of pictures?" 12 60

if [ $? -eq 0 ];then
	cd TOOLS/agnostep_wprotate || exit 1
	./install_wprotate.sh
else
	WPRCONF=$HOME/.config/agnostep/wprotate.conf
	if [ -f $WPRCONF ];then
		rm $WPRCONF
	fi
fi
}
is_wprotate
cd $_PWD
sleep $SLEEP
stop

###########################################
### Installing Tools
STR="User Tools"
subtitulo

if [ ! -d /usr/local/bin ];then
	sudo mkdir -p /usr/local/bin
fi

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
sleep $SLEEP 

stop
###########################################

#################################################
### Installing Tools and confs... Updater
STR="Updater tool"
subtitulo

cd TOOLS/agnostep_updater || exit 1
. ./install_agnostep_updater.sh
cd $_PWD
ok "Done"
sleep $SLEEP 

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
	sleep $SLEEP 
fi

stop

####################################################
### Installing Tools and confs... Dockapps
### for C5C flavour
. $HOME/.config/agnostep/flavour.conf || exit 1
if [ "$FLAVOUR" == "c5c" ];then
	cd TOOLS/dockapps
	./install_dockapps.sh
	./check_dockapps.sh
	cd $_PWD
	sleep $SLEEP 
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
sleep $SLEEP 

stop

###########################################
### Theme for FocusWriter
STR="Theme for FocusWriter"
subtitulo

cd TOOLS/agnostep_fw || exit 1
. ./install_agnostep_fw.sh
cd $_PWD
ok "Done"
sleep $SLEEP 
stop

###########################################

###########################################
printf "Loading notification script\n"
sudo cp -u RESOURCES/SCRIPTS/loading.sh /usr/local/bin/
ok "Done"
sleep $SLEEP 

stop
###########################################

set_menus
sleep $SLEEP

##################################################
MESSAGE="A G N o S t e p  Theme  was set."

info "$MESSAGE" | tee -a $LOG
date >> $LOG
cd $_PWD
