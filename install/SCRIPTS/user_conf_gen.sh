#!/bin/bash

####################################################
### A G N o S t e p  -  Theme - by Patrick Cardona
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
if [ ! -d $HOME/GNUstep/Library/WindowMaker ];then
	mkdir -p $HOME/GNUstep/Library/WindowMaker
fi
FLAVOUR_CONF=$HOME/.config/agnostep/flavour.conf
MENUS_CONF=$HOME/.config/agnostep/menus.conf
if [ ! -d $HOME/.config/agnostep ];then
	mkdir -p $HOME/.config/agnostep
fi
FICHTEMP=$(mktemp /tmp/agno-XXXXX)
trap "rm -f $FICHTEMP" EXIT
CHOICE=""
GNUSTEP_SYSTEM_TOOLS=`gnustep-config --variable=GNUSTEP_SYSTEM_TOOLS`

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
#!/bin/bash

PID_XSESSION=$$

# Compositor
/usr/bin/picom --config $HOME/.config/picom.conf &

if [ -f $HOME/.Xresources ];then
	xrdb -merge $HOME/.Xresources
fi

HEAD_OF_XINIT


cat << BODY_OF_XINIT >> $XINITRC

### Window Manager
/usr/bin/wmaker $WMDOCK $WMCLIP --static &

### Agenda
${AGENDA}

### GWorkspace within a DBus session
sleep 4
exec dbus-launch --sh-syntax --exit-with-session ${GNUSTEP_SYSTEM_TOOLS}/openapp GWorkspace

BODY_OF_XINIT

cat << "END_OF_XINIT" >> $XINITRC

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
#/Local/Tools/gdomap -L GDNCServer || /Local/Tools/gdnc &
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
${UPDATER}

### Birthday Notifier only with Conky Flavour
${BIRTHDAY}

FOOT_OF_AUTOSTART

chmod +x $AUTOSTART
}
####################################################

####################################################
### What is the flavour?
####################################################

function menu_fr
{
# boîte de menu
dialog --backtitle "Variante du thème" --title "Choix de la variante" \
--menu "
Le thème AGNOSTEP se décline en deux variantes.

Choisissez une des variantes proposées:" 18 66 3 \
"Conky" "Installer le panneau d'infos système de Conky" \
"Classic" "Installer la variante classique de Window Maker" 2>> $FICHTEMP
# traitement de la réponse
if [ $? = 0 ]
then
for i in `cat $FICHTEMP`
do
case $i in
"Conky") echo "Vous avez choisi: Conky";CHOICE="CONKY" ;;
"Classic") echo "Vous avez choisi: Classic";CHOICE="CLASSIC";;
esac
done
fi
}

function menu_en
{
# boîte de menu
dialog --backtitle "Flavour of the Theme" --title "Flavour Choice" \
--menu "
The AGNOSTEP Theme provides two flavours.

Select one of these flavours:" 18 66 2 \
"Conky" "Install the Conky Sysinfo Panel" \
"Classic" "Install the Classic flavour of Window Maker" 2>> $FICHTEMP
# traitement de la réponse
if [ $? = 0 ]
then
for i in `cat $FICHTEMP`
do
case $i in
"Conky") echo "You chose: Conky";CHOICE="CONKY" ;;
"Classic") echo "You chose: Classic";CHOICE="CLASSIC";;
esac
done
fi
}

function set_flavour
{
clear
#STR="A G N o S t e p  -  ${FLAVOUR_TITLE}";titulo
#sleep 2

LG=${LANG:0:2}
case $LG in
"fr") menu_fr;;
"en"|*) menu_en;;
esac

case "$CHOICE" in
	"CLASSIC"|"MACSTYLE")
		FLAVOUR="c5c"
		WMCLIP=""
		WMDOCK=""
		AGENDA=""
		CONKY=""
		BIRTHDAY=""
		UPDATER="";;
	"CONKY"|*) echo "${YOUR_CHOICE}: AGNoStep and Conky."
		FLAVOUR="conky"
		WMCLIP="--no-clip"
		WMDOCK="--no-dock"
		AGENDA="sleep 2;${GNUSTEP_SYSTEM_TOOLS}/openapp SimpleAgenda &"
		CONKY="pgrep conky || sleep 8 && conky -c ~/.config/agnostep/conky.conf &"
		BIRTHDAY="sleep 10 && /usr/local/bin/BirthNotify &"
		UPDATER="sleep 10 && /usr/local/bin/Updater -d &";;
esac

echo -e "FLAVOUR=\"${FLAVOUR}\"" > ${FLAVOUR_CONF}
}

function set_menus
{
# boîte de menu
dialog --backtitle "Menus of the Theme" --title "Menus Style Choice" \
--menu "
The AGNOSTEP Theme provides two menus styles.

Select one of these styles:" 18 66 2 \
"NextStep" "Vertical menus boxes" \
"Mac" "Macintosh Horizontal menus" 2>> $FICHTEMP
# traitement de la réponse
if [ $? = 0 ]
then
for i in `cat $FICHTEMP`
do
case $i in
"NextStep") echo "You chose: NextStep style"
	defaults write NSGlobalDomain NSMenuInterfaceStyle "NSNextStepInterfaceStyle";;
"Mac") echo "You chose: Mac style"
	defaults write NSGlobalDomain NSMenuInterfaceStyle "NSMacintoshInterfaceStyle"
	cd RESOURCES/MACSET
	cp WMState WMWindowAttributes $HOME/GNUstep/Defaults/
	cd $_PWD;;
esac
done
fi
}

### To be called in install_theme.sh:
# set_flavour
# write_xinitrc
# write_autostart
# write_meteo
# set_menus
