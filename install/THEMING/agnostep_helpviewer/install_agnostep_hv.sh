#!/bin/bash

####################################################
### A G N o S t e p  -  Theme - by Patrick Cardona
### pcardona34 @ Github
###
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

####################################
### Theming HelpViewer for AGNOSTEP
### CAUTION!!!
### Use only in case you do NOT use
### the localized fork provided by
### Agnostep since rel. 3.x
####################################

################################
### VARS
THERE=`pwd`
LOCAL_APPS=$(gnustep-config --variable=GNUSTEP_LOCAL_APPS)
TARGET=${LOCAL_APPS}/HelpViewer.app/Resources
if [ ! -d $TARGET ];then
	printf "The target $TARGET was not found."
	exit 1
fi

for ICO in *.tiff *.tif
do
	sudo cp $ICO $TARGET/
done
