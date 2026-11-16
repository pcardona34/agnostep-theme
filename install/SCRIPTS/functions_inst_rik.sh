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
### We try to make and install
### an alternate theme for Window Manager
### and GNUstep system
### it is a fork of Sandro Giulano Rik theme
#####################################################

####################################################
### Functions for inst_rik
####################################################


#############################################
### Fetching
#############################################

function rik_setting
{
STR="Session minimal settings"
subtitulo

#cd $_PWD

### Setup GWorkspace...
#cat RESOURCES/MINSET/org.gnustep.GWorkspace.template | sed s/patrick/${USER}/g >> RESOURCES/MINSET/org.gnustep.GWorkspace.plist
#mv RESOURCES/MINSET/org.gnustep.GWorkspace.plist $HOME/GNUstep/Defaults/

### Removing themed plist of TimeMon and AClock
#for PLIST in org.gap.AClock TimeMon
#do
#	rm --force $HOME/GNUstep/Defaults/${PLIST}.plist
#done

### Defaults
#for DEF in RESOURCES/MINSET/*.plist
#do
#    cp $DEF $HOME/GNUstep/Defaults/
#done

#cp --force RESOURCES/MODERNSET/NSGlobalDomain.plist $HOME/GNUstep/Defaults/

defaults write NSGlobalDomain GSTheme Rik

#if [ ! SCRIPTS/env_modern.sh ];then
#    if [ ! -f $HOME/GNUstep/Defaults/WindowMaker ];then
#        cp RESOURCES/MINSET/WindowMaker $HOME/GNUstep/Defaults/WindowMaker
#    fi
#    setstyle RESOURCES/STYLES/Tradition.style
#
#    ### Retrieving default Clip icon and WMWindowAttributes
#    if [ -f $HOME/GNUstep/Library/Icons/clip.tiff ];then
#	    rm $HOME/GNUstep/Library/Icons/clip.tiff
#	    cat $HOME/GNUstep/Defaults/WMWindowAttributes | sed s#~/GNUstep/Library#/usr/share/WindowMaker# > TEMP && mv TEMP $HOME/GNUstep/Defaults/WMWindowAttributes
#    fi

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
cd $_PWD/RESOURCES/RIKSET || exit 1
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


function install_rik
{
clear
pushd .
cd ../build || exit 1

APPNAME="Rik.theme"
CHECK=NO
HUB="https://github.com"
OWNER="pcardona34" # We use a fork of pkgdemon's.
REPO="rik.theme"
CONFIG_ARGS=""
INSTALL_ARGS=""

STR="$APPNAME"
subtitulo

printf "Fetching...\n"
if [ -d $REPO ];then
	cd $REPO
else
	git clone ${HUB}/${OWNER}/${REPO}
    cd ${REPO}
fi
git pull
sleep 2
clear

ok "${REPO}: Fetched"

_build_Theme

ok "All was done for ${APPNAME}"
popd
}
