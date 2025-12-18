#!/bin/bash

####################################################
### A G N o S t e p  -  Desktop - by Patrick Cardona
### pcardona34 @ Github
###
### Thanks for the GNUstep Developers Community
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

################################
### Functions for Misc Folders
################################

###############################################
function icon_folder
{
FOLDER="$1"
if ! [ -d $HOME/${FOLDER} ];then
	mkdir -p $HOME/${FOLDER}
fi
cd RESOURCES/ICONS && cp -u dir_${FOLDER}.tiff $HOME/${FOLDER}/.dir.tiff
cd $_PWD
}

function l18n_folder
{
LG=${LANG:0:2}
case $LG in
	"fr")
	if [ -d $HOME/Books ];then
		mv $HOME/Books $HOME/Livres
	fi
	if [ -d $HOME/Favorites ];then
		mv $HOME/Favorites $HOME/Favoris
	fi
	if [ -d $HOME/Samples ];then
		mv $HOME/Samples $HOME/Exemples
	fi
	printf "Les dossiers ont été francisés autant que faire se peut.\n";;
	"en"|*)
	printf "The Folders names were not changed: default is English.\n";;
esac
}
