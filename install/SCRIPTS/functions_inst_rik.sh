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

defaults write NSGlobalDomain GSTheme Rik

### Cached Pixmaps
if [ ! -d $HOME/GNUstep/Library/WindowMaker ];then
   mkdir -p $HOME/GNUstep/Library/WindowMaker
else
    if [ -d $HOME/GNUstep/Library/WindowMaker/CachedPixmaps ];then
        pushd . >/dev/null
        cd $HOME/GNUstep/Library/WindowMaker
        rm -r CachedPixmaps
        popd >/dev/null
    fi
fi

pushd . >/dev/null
cd RESOURCES/RIKSET || exit 1
cp -r CachedPixmaps $HOME/GNUstep/Library/WindowMaker/
popd >/dev/null

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
pushd . >/dev/null
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
popd >/dev/null
}
