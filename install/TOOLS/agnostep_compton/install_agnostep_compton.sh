#!/bin/bash

####################################################
### A G N o S t e p  -  Desktop - by Patrick Cardona
### pcardona34 @ Github
###
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

################################
### Install the Compton conf file
### herited from B. Deconinck
################################

################################
### VARS

HOME_DIR_CONF=$HOME/.config

if ! [ -d $HOME_DIR_CONF ];then
	mkdir -p "${HOME_DIR_CONF}"
fi

cp -u compton.conf $HOME_DIR_CONF/

printf "\nCompton has been set.\n"
