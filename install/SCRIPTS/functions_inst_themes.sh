#!/bin/bash

####################################################
### AGNoStep  -  Theme
### (c) 2026 Patrick Cardona
###
### Thanks for the GNUstep Developers Community
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

################################
### Functions for Install Themes
################################

#############################################
### Dependencies on a debian system
#############################################
function check_deps
{
STR="Dependencies";titulo

### Checking if GNUstep, namely base, is already set
STR="Checking GNUstep";subtitulo
LOCAL_LIBS=$(gnustep-config --variable=GNUSTEP_LOCAL_LIBRARIES)
GSBASE=${LOCAL_LIBS}/libgnustep-base.so

if [ ! -f ${GSBASE} ];then
    alert "GNUstep libs-base was not found! You must install a GNUstep system first."
    exit 1
else
    ok "GNUstep was found"
    sleep 2;clear
fi
}

function check_apps
{
### GNUstep apps needed
APP_DIR=$(gnustep-config --variable=GNUSTEP_LOCAL_APPS)
STR="Checking installed apps";titulo

APPS="GWorkspace SystemPreferences Terminal"
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
}


function reset_theme
{
STR="Session minimal settings"
subtitulo

### Setup GWorkspace...
#cat RESOURCES/MINSET/org.gnustep.GWorkspace.template | sed s/patrick/${USER}/g >> RESOURCES/MINSET/org.gnustep.GWorkspace.plist
#mv RESOURCES/MINSET/org.gnustep.GWorkspace.plist $HOME/GNUstep/Defaults/

### Removing themed plist of TimeMon and AClock
for PLIST in org.gap.AClock TimeMon
do
	rm --force $HOME/GNUstep/Defaults/${PLIST}.plist
done

### Defaults
#for DEF in RESOURCES/MINSET/*.plist
#do
#    cp $DEF $HOME/GNUstep/Defaults/
#done

defaults write NSGlobalDomain GSTheme GNUstep
#if [ ! -f $HOME/GNUstep/Defaults/WindowMaker ];then
#    cp RESOURCES/MINSET/WindowMaker $HOME/GNUstep/Defaults/WindowMaker
#fi
setstyle RESOURCES/STYLES/Tradition.style

### Retrieving default Clip icon and WMWindowAttributes
if [ -f $HOME/GNUstep/Library/Icons/clip.tiff ];then
	rm $HOME/GNUstep/Library/Icons/clip.tiff
	cat $HOME/GNUstep/Defaults/WMWindowAttributes | sed s#~/GNUstep/Library#/usr/share/WindowMaker# > TEMP && mv TEMP $HOME/GNUstep/Defaults/WMWindowAttributes
fi

### Cached Pixmaps
if [ ! -d $HOME/GNUstep/Library/WindowMaker ];then
    mkdir -p $HOME/GNUstep/Library/WindowMaker
else
    if [ -d $HOME/GNUstep/Library/WindowMaker/CachedPixmaps ];then
        pushd .
	    cd $HOME/GNUstep/Library/WindowMaker
	    rm -r CachedPixmaps
        popd
    fi
fi

pushd .
cd RESOURCES/MINSET || exit 1
cp -r CachedPixmaps $HOME/GNUstep/Library/WindowMaker/
popd

### Reinitialising themed folders
LG=${LANG:0:2}
case "$LG" in
fr)
    FOLDERS="Bookshelf Exemples Favoris GNUstep Livres SOURCES Mailboxes";;
en)
	FOLDERS="Bookshelf Samples Favorites GNUstep Books SOURCES Mailboxes";;
esac
for FOLD in ${FOLDERS}
do
	if [ -d $HOME/$FOLD ];then
		rm -f $HOME/$FOLD/.dir.tiff
	fi
done
}

### install AGNOSTEP theme
###############################################
function install_gs_theme
{
#title "AGNOSTEP Theme for GNUstep"
printf "Installing the theme for GNUstep...\n"
GS_USER_LIB=$(gnustep-config --variable=GNUSTEP_USER_LIBRARY)
if ! [ -d ${GS_USER_LIB}/Themes ];then
	mkdir -p ${GS_USER_LIB}/Themes
fi
pushd .
cd RESOURCES/THEMES || exit 1
cp -a AGNOSTEP.theme ${GS_USER_LIB}/Themes/
popd
ok "Done"
}

########################################
function menu_theme_choice
{
dialog --backtitle "AGNoStep Setup" --title "Theme Choice" \
--radiolist "

Choose your theme" 12 66 3 \
"AGNOSTEP" "Default recommended theme" on \
"Rik" "Another modern theme" off \
"GNUstep" "The classic Next like" off 2>> $TEMPFILE

if [ $? -eq 0 ];then
for i in `cat $TEMPFILE`
do
    case "$i" in
        "AGNOSTEP")
    	info "AGNOSTEP theme will apply.\n"
        sleep ${SLEEP:-2}
        . agnostep_theme.sh;;
        "Rik")
    	info "Rik theme will apply.\n"
        sleep ${SLEEP:-2}
        . rik_theme.sh;;
        "GNUstep")
    	info "Default GNUstep theme will apply.\n"
        sleep ${SLEEP:-2}
       	. gnustep_theme.sh;;
    esac
done
fi
}



###############################################
### !!! The funtion: install_wm_theme is obsolete
### DO NOT use it
###############################################
function install_wm_theme
{
title "AGNOSTEP Theme for WindowMaker"
printf "Installing the theme...\n"
cd RESOURCES/THEMES || exit 1

if [ ! -d $HOME/GNUstep/Library/WindowMaker/Themes ];then
	mkdir -p $HOME/GNUstep/Library/WindowMaker/Themes
fi

cp WMAGNOSTEP-C5C.tar.gz $HOME/GNUstep/Library/WindowMaker/Themes/
cd $HOME/GNUstep/Library/WindowMaker/Themes || exit 1
gunzip --force WMAGNOSTEP-C5C.tar.gz && tar -xf WMAGNOSTEP-C5C.tar
rm WMAGNOSTEP-C5C.tar
cd $_PWD
ok "Done"
}
