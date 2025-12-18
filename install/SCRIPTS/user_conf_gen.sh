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
### User configuration files generator
####################################################

####################################################
### VARS
LG=${LANG:0:2}
STR="..."
XINITRC=$HOME/.xinitrc
XSESSION=$HOME/.xsession
AUTOSTART=$HOME/GNUstep/Library/WindowMaker/autostart
METEO_CONF=$HOME/.config/agnostep/meteo.conf
FLAVOUR_CONF=$HOME/.config/agnostep/flavour.conf

####################################################
### Functions include
. SCRIPTS/colors.sh

### STRINGS L18N
STRINGS="${0%.sh}"
L18N="RESOURCES/L18N/${STRINGS}.${LG}"

if [ -f $L18N ];then
	. $L18N
else
 	. "RESOURCES/L18N/${STRINGS}.en"
fi

####################################################
### internal functions
####################################################

####################################################
### writing xinitrc
function write_xinitrc
{
STR="Xinitrc - Xsession";subtitulo

cat << "HEAD_OF_XINIT" > $XINITRC

#!/bin/sh

PID_XSESSION=$$

HEAD_OF_XINIT


cat << BODY_OF_XINIT >> $XINITRC

### Window Manager
/usr/bin/wmaker --no-dock $WMCLIP --static &

BODY_OF_XINIT


cat << "END_OF_XINIT" >> $XINITRC
sleep 1
/System/Tools/openapp SimpleAgenda &

### GWorkspace within a DBus session
sleep 4
exec dbus-launch --sh-syntax --exit-with-session /System/Tools/openapp GWorkspace

### This is a secure way in any case Dbus fails to kill the session:

kill $PID_XSESSION

END_OF_XINIT



cp -f $XINITRC $XSESSION
}
###################################################
### Writing autostart
function write_autostart
{
STR="Autostart";subtitulo

cat << "HEAD_OF_AUTOSTART" > $AUTOSTART
#!/bin/bash
xset m 20/10 4

### This is to prevent DMPS and blanking issues on RPI machine
xset -dpms
xset s off

### gdnc
/Local/Tools/gdomap -L GDNCServer || /Local/Tools/gdnc &
### compton: the -b arg means as a daemon like &
pgrep compton || compton -b
### Notifications
systemctl --user import-environment DISPLAY
pgrep dunst || dunst &
sleep 2 && /usr/local/bin/loading.sh &

HEAD_OF_AUTOSTART


### FOOT:

cat << FOOT_OF_AUTOSTART >> $AUTOSTART
### conky only with Conky Flavour
${CONKY}

### Updater
sleep 10 && /usr/local/bin/Updater -d &

### Birthday Notifier only with Conky Flavour
${BIRTHDAY}
FOOT_OF_AUTOSTART


}
####################################################

####################################################
### Writing the meteo conf
function write_meteo_conf
{
STR="$METEO_TITLE";subtitulo

if [ -f $METEO_CONF ];then
	printf "${INFO_FILE_EXISTS}\n"
	read -p "(1) ${EDIT_FILE} - (2) ${ERASE_FILE} - ${ENTER}: " REP
	case "$REP" in
		1)
		nano $METEO_CONF;;
		2|*)
		rm -f $METEO_CONF
		read -p "${COUNTRY_TAG} - ${ENTER}: " COUNTRY
		read -p "${STATE_TAG}:  - ${ENTER}: " STATE
		read -p "${CITY_TAG}:  - ${ENTER}: " CITY
		echo -e "STATION=\"${CITY},${STATE},${COUNTRY}\"" > $METEO_CONF;;
	esac
fi

info "$INFO_METEO_END"

}
####################################################


####################################################
### What is the flavour?
####################################################

function set_flavour
{
clear
STR="A G N o S t e p  -  ${FLAVOUR_TITLE}";titulo

info "${FLAVOUR_INFO}: \n\t(1) Conky: ${FLAVOUR_ONE}. \n\t(2) C5C: ${FLAVOUR_TWO}. ${INFO_BOTH}."

read -p "${CHOICE_PROMPT}: 1, 2. ${ENTER} : " CHOICE

case "$CHOICE" in
	"2") echo "${YOUR_CHOICE}: AGNoStep Classic"
		FLAVOUR="c5c"
		WMCLIP=""
		CONKY=""
		BIRTHDAY="";;
	"1"|*) echo "${YOUR_CHOICE}: AGNoStep and Conky."
		FLAVOUR="conky"
		WMCLIP="--no-clip"
		CONKY="pgrep conky || sleep 8 && conky -c ~/.config/agnostep/conky.conf &"
		BIRTHDAY="#sleep 10 && /usr/local/bin/BirthNotify &";;
esac

echo -e "FLAVOUR=\"${FLAVOUR}\"" > ${FLAVOUR_CONF}
}

### To be called in install_theme.sh:
# set_flavour
# write_xinitrc
# write_autostart
# write_meteo
