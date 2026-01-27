#!/bin/bash

####################################################
### A G N o S t e p  -  Theme - by Patrick Cardona
### pcardona34 @ Github
###
### Thanks for the GNUstep Developers Community
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

################################
### Log
### Managing Agnostep_Theme Log
################################

LOG=/var/log/agnostep-theme.log
if [ ! -f $LOG ];then
	sudo touch $LOG
	sudo chown $USER:$USER $LOG
fi

date >> $LOG
