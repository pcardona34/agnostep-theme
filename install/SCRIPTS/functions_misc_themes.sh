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
### Functions for Misc Themes
################################

function update_info()
{
cd $_PWD/RESOURCES/INFOS
for INFO in Info-gnustep.plist_*
do
	printf "Updating ${INFO#Info-gnustep.plist_}...\n"
	TARGET="${LOCAL_INSTALL_DIR}/${INFO#Info-gnustep.plist_}.app/Resources/Info-gnustep.plist"
	if [ -d ${LOCAL_INSTALL_DIR}/${INFO#Info-gnustep.plist_}.app/Resources ]; then
		sudo cp --remove-destination ${INFO} ${TARGET} && ok "Done"
	else
		warning "$TARGET was not found for $INFO"
		#This should not prevent the 3rd step to accomplish...
	fi
done
}

##########################################
### Customize Clip icon
##########################################
function customize_clip
{
CLIP_ICON=$_PWD/RESOURCES/ICONS/clip_c5c.tiff
DESTINATION_ICON=$HOME/GNUstep/Library/Icons/clip.tiff
if [ ! -d $DESTINATION ];then
	mkdir -p $DESTINATION_ICON
fi
cp $CLIP_ICON $DESTINATION_ICON
cat $HOME/GNUstep/Defaults/WMWindowAttributes | sed s#/usr/share/WindowMaker#~/GNUstep/Library# > TEMP && mv TEMP $HOME/GNUstep/Defaults/WMWindowAttributes
}
