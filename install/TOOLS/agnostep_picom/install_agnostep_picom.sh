#!/bin/bash

####################################################
### A G N o S t e p  -  Theme - by Patrick Cardona
### pcardona34 @ Github
###
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

################################
### Install the Picom conf file
################################

################################
### VARS

function install_picom
{
STR="Picom conf..."
printf "$STR\n"

HOME_DIR_CONF=$HOME/.config

if ! [ -d $HOME_DIR_CONF ];then
	mkdir -p "${HOME_DIR_CONF}"
fi

cp -u picom.conf $HOME_DIR_CONF/
ok  "Done"
}
