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
### Select and install the Goodies
### like Weather, Birthday, Updater...
################################

function about_goodies
{
dialog --title "About AGNOSTEP Goodies" \
--ok-label "Close" --msgbox "
What are the Goodies provided by AGNOSTEP theme?

There are helpfull tools available for the two flavours
of the theme: classic or conky.

- Birthday: it checks wether you have a birthday or
a feast to celebrate today.
- Updater: it checks wether new Debian packages are 
available.
- Weather: it gets the weather health from your next
Weather Station." 16 60

}

### Testing
about_goodies;clear
