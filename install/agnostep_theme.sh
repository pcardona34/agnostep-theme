#!/bin/bash

####################################################
### AGNoStep  Theme
### (c) 2026 Patrick Cardona
###
### Thanks for the GNUstep Developers Community
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

####################################################
### Install AGNOSTEP Theme
####################################################

sudo -v
if [ $? -ne 0 ];then
	exit 1
fi

clear

SPIN='\-/|'
STOP=0 # Set to 0 to avoid stops; to 1 to make stops for debugging purpose
SLEEP=2
#set -v
GWDEF="org.gnustep.GWorkspace"
DEFDIR=RESOURCES/AGNOSET
#if [ -f SCRIPTS/env_modern.sh ];then
#    DEFDIR=RESOURCES/MODERNSET
#else
#    DEFDIR=RESOURCES/DEFAULTS
#fi
RPI=1 # By default, we assume the hw is not a RPI one. If not, it will be detected.
TEMPFILE=$(mktemp /tmp/agno-XXXXX)
trap "rm -f $TEMPFILE" EXIT

####################################################
### Include functions

. SCRIPTS/log.sh
. SCRIPTS/colors.sh
. SCRIPTS/spinner.sh
. SCRIPTS/stop.sh
. SCRIPTS/functions_prep.sh
. SCRIPTS/functions_misc_folders.sh
. SCRIPTS/functions_inst_themes.sh
. SCRIPTS/functions_misc_themes.sh
#. ../Documentation/RELEASE_THEME.txt

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
echo $PATH | grep -e "${GNUSTEP_SYSTEM_TOOLS}" &>/dev/null
if [ $? -ne 0 ];then
	export PATH=${GNUSTEP_SYSTEM_TOOLS}:$PATH
fi

### End of functions
####################################################

reset_theme

STR="Installing AGNOSTEP theme"
titulo

###################################################

####################################################
### Customizing User Home Folders

STR="Customizing the User Home folder contents"
subtitulo

LG=${LANG:0:2}
case "$LG" in
"fr")
	# Some folders are auto-translated by GNUstep, so we keep their English name.
	FOLDERS="Livres Desktop Documents Downloads Favoris GNUstep Images Mailboxes Music Exemples SOURCES Videos";;
"en"|*)
	FOLDERS="Books Desktop Documents Downloads Favorites GNUstep Images Mailboxes Music Samples SOURCES Videos";;
esac

### Bookshelf on RPI's
is_hw_rpi
if [ $RPI -eq 0 ];then
	FOLDERS="$FOLDERS Bookshelf"
fi

### We manage the case of existing folders in English or in French
l18n_folder

stop

STR="Customizing the Icons of User Home folder contents"
subtitulo

pushd .
for FOLD in ${FOLDERS}
do
	printf "Setting Icon for the Folder ${FOLD}\n"
	if [ -d $HOME/${FOLD} ];then
        # Only non default themed icons are provided there...
    	icon_folder $FOLD
	    ok "Done"
    fi
done
sleep $SLEEP
stop

popd

###########################################
### Installing the main AGNOSTEP theme
STR="Main AGNOSTEP Theme"
titulo

#printf "GNUstep Theme..."
pushd .
install_gs_theme
popd

sleep $SLEEP

stop

### Clip
if [ ! -f $_PWD/SCRIPTS/env_modern.sh ];then
    printf "Clip Icon...\n"
    pushd .
    customize_clip
    ok "Done";sleep $SLEEP 

    popd
    stop
fi

### Some Apps known to not comply with Theme: workaround
### We need to update Info-gnustep.plist for these apps
### Adding 'CFBundleIdentifier' property in the Dictionary
### Updating too Help path

pushd .
update_info
popd

sleep ${SLEEP:-2}
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

pushd .
cd $DEFDIR || exit 1

GWD=${GWDEF}.TEMPLATE

cat ${GWD} | sed -e s/patrick/$USER/g > ${GWDEF}.plist
cp --force --verbose ${GWDEF}.plist ${HOME_GNUSTEP_DEF}
rm ${GWDEF}.plist
cp --force --verbose NSGlobalDomain.plist ${HOME_GNUSTEP_DEF}
popd

if [ ! -f $HOME_GNUSTEP_DEF/WindowMaker ];then
    pushd .
	cd ${DEFDIR} || exit 1
    cp WindowMaker $HOME_GNUSTEP_DEF/
	popd
fi

cp --force ${DEFDIR}/WMWindowAttributes $HOME/GNUstep/Defaults/

### Cached Pixmaps
if [ -d $HOME/GNUstep/Library/WindowMaker/CachedPixmaps ];then
    pushd .
	cd $HOME/GNUstep/Library/WindowMaker
	rm -fR CachedPixmaps
    popd
    fi

pushd .
cd ${DEFDIR} || exit 1
cp -r CachedPixmaps $HOME/GNUstep/Library/WindowMaker/
popd

pushd .
WMSTATE="WMState"
cd RESOURCES/MINSET
if [ ! -f $WMSTATE ];then
	    alert "The template $WMSTATE was not found. This is a major issue."
	    exit 1
    else
	    cp --force $WMSTATE $HOME_GNUSTEP_DEF/WMState
	    ### Setting to the user env
	    cat $HOME_GNUSTEP_DEF/WMState | sed -e s#/System/Tools#${GNUSTEP_SYSTEM_TOOLS}#g > $TEMPFILE
	    cat $TEMPFILE > $HOME_GNUSTEP_DEF/WMState
    fi
popd

ok "Done"
sleep $SLEEP
stop


################################
### Set the defaults
### for AGNoStep Desktop
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

pushd .
cd ${DEFDIR} || exit 1

for PLIST in "Addresses" "org.gap.InnerSpace" "org.gap.Terminal" "org.gnustep.GNUMail" "org.poroussel.SimpleAgenda" "org.gap.AClock" "TimeMon"
do
	if [ -f ${PLIST}.plist ];then
		printf "Setting Defaults for ${PLIST}\n"
		if [ -f $DEST/${PLIST}.plist ] && [ "${PLIST}" == "org.gnustep.GNUMail" ];then
			info "We preserve GNUMail settings."
			continue;
		else
			cp --force ${PLIST}.plist $DEST/
		fi
		ok "Done"
		stop
	else
		warning "The File ${PLIST}.plist was not found."
		stop
	fi
done
popd

STR="GWorkspace: Dock position and visibility"
subtitulo

DOCKPOS=1
HIDE=1
printf "Hidden.\n"

defaults write org.gnustep.GWorkspace hidedock $HIDE
defaults write org.gnustep.GWorkspace dockposition $DOCKPOS

ok "Done"
sleep $SLEEP
stop

STR="Recycler: is Docked?"
subtitulo

ISDOCKED=1
printf "Recycler will be docked.\n"
defaults write org.gnustep.GWorkspace.Recycler docked $ISDOCKED

ok "Done"

###########################################
### Theme for FocusWriter

which -s focuswriter
if [ $? -eq 0 ];then
    if [ -d ${APP_DIR}/Writer.app ];then
        STR="Theme for FocusWriter"
        subtitulo
        pushd .
        cd THEMING/agnostep_fw || exit 1
        . ./install_agnostep_fw.sh
        popd
        ok "Done"
        sleep ${SLEEP:-2}
        stop
    fi
fi

###########################################

MESSAGE="A G N o S t e p  Theme  was set."

info "$MESSAGE" | tee -a $LOG
date >> $LOG

sleep ${SLEEP:-2}
