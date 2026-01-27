#!/bin/bash

####################################################
### A G N o S t e p  -  Theme - by Patrick Cardona
### pcardona34 @ Github
###
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

################################
### Install the theme for FocusWriter
################################

################################
### VARS
THERE=`pwd`

whereis focuswriter
if [ $? -eq 0 ];then
	THEMES_DIR=$HOME/.local/share/GottCode/FocusWriter
	CONF_DIR=$HOME/.config/GottCode
	WP_DIR=/usr/share/wallpapers

	for DIR in "$THEMES_DIR" "$CONF_DIR"
	do
		if [ ! -d $DIR ];then
			mkdir -p $DIR
		fi
	done

	sudo cp fond_agnostep_fw.png $WP_DIR/
	cp FocusWriter_Themes.tar.gz $THEMES_DIR/
	cd $THEMES_DIR
	gunzip --force FocusWriter_Themes.tar.gz
	tar -xf FocusWriter_Themes.tar
	rm FocusWriter_Themes.tar
	cd "$THERE"
	cp --remove-destination FocusWriter.conf $CONF_DIR/
else
	alert "FocusWriter not found. This theme will not be installed."
fi
cd "$THERE"
