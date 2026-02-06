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
### Uninstall AGNOSTEP Theme
### This set GNUstep Default Theme...
####################################################

sudo -v
if [ $? -ne 0 ];then
	exit 1
fi

clear

function stop
{
if [ $STOP -ne 0 ];then
	read -p "$MSG_STOP" R
fi
}

_PWD=`pwd`
SPIN='\-/|'
STOP=0 # Set to 0 to avoid stops; to 1 to make stops
SLEEP=2
#set -v
MSG_STOP="Stop: type <Enter> to continue."
LG=${LANG:0:2}

case "$LG" in
"fr")
	SAMPLE_FOLDER="Exemples"
	HELP_FOLDER="Aide"
	FAVORITE_FOLDER="Favoris"
	BOOKS_FOLDER="Livres"
	APP_DEVEL=Prog
	APP_GAMES=Jeux
	APP_TOOLS=Utilitaires;;
"en"|*)
	SAMPLE_FOLDER="Samples"
	HELP_FOLDER="Help"
	FAVORITE_FOLDER="Favorites"
	BOOKS_FOLDER="Books"
	APP_DEVEL=Dev
	APP_GAMES=Games
	APP_TOOLS=Utilities;;
esac

####################################################
### Include functions

. SCRIPTS/log.sh
. SCRIPTS/colors.sh
. SCRIPTS/spinner.sh

### End of functions
####################################################

####################################################
### DO NOT RUN under an existent X session
####################################################

if [ -n "$DISPLAY" ];then
        alert "You should not uninstall this theme within an XTERM!"
        info "Logout the Graphical session, open a tty console and try again."
        exit 1
fi

####################################################
### Samples
STR="FOLDERS CUSTOMIZED"
echo "$STR" >> $LOG

for FOLD in ${SAMPLE_FOLDER} ${HELP_FOLDER} ${FAVORITE_FOLDER} #${BOOKS_FOLDER}
do
	if [ -d $HOME/$FOLD ];then
		dialog --backtitle "Uninstalling AGNOSTEP Theme" --title "$FOLD" \
		--yesno "
		Do you want to remove this Folder: ${FOLD}?" 12 60
		# traitement de la réponse
		# O est le code retour du bouton Oui
		if [ $? = 0 ];then
        		rm -fR $HOME/$FOLD
		fi
	fi
done

ok "Done"
sleep $SLEEP
clear

stop

function minimal_setting
{
STR="Session minimal settings"
subtitulo

cd $_PWD
cp RESOURCES/MINSET/_xinitrc $HOME/.xinitrc
cp $HOME/.xinitrc $HOME/.xsession
if [ ! -d $HOME/GNUstep/Library/WindowMaker ];then
	mdir -p $HOME/GNUstep/Library/WindowMaker
fi
if [ ! -d $HOME/GNUstep/Defaults ];then
	mdir -p $HOME/GNUstep/Defaults
fi
cp RESOURCES/MINSET/WMState $HOME/GNUstep/Defaults/
cp RESOURCES/MINSET/WMWindowAttributes $HOME/GNUstep/Defaults/

### Setup GWorkspace...
cat RESOURCES/MINSET/org.gnustep.GWorkspace.template | sed s/patrick/${USER}/g >> RESOURCES/MINSET/org.gnustep.GWorkspace.plist
mv RESOURCES/MINSET/org.gnustep.GWorkspace.plist $HOME/GNUstep/Defaults/
rm --force $HOME/GNUstep/Library/WindowMaker/autostart
defaults write NSGlobalDomain GSTheme GNUstep
setstyle RESOURCES/STYLES/Tradition.style

### Removing themed plist of TimeMon and AClock
for PLIST in org.gap.AClock TimeMon
do
	rm --force $HOME/GNUstep/Defaults/${PLIST}.plist
done


### Retrieving default Clip icon and WMWindowAttributes
if [ -f $HOME/GNUstep/Library/Icons/clip.tiff ];then
	rm $HOME/GNUstep/Library/Icons/clip.tiff
#	cat $HOME/GNUstep/Defaults/WMWindowAttributes | sed s#~/GNUstep/Library#/usr/share/WindowMaker# > TEMP && mv TEMP $HOME/GNUstep/Defaults/WMWindowAttributes
fi

### Cached Pixmaps
if [ -d $HOME/GNUstep/Library/WindowMaker/CachedPixmaps ];then
	cd $HOME/GNUstep/Library/WindowMaker
	rm -r CachedPixmaps
	cd $_PWD/RESOURCES/MINSET
	cp -r CachedPixmaps $HOME/GNUstep/Library/WindowMaker/
	cd $_PWD
fi

### Cleaning theming folders
LG=${LANG:0:2}
case "$LG" in
fr)
	FOLDERS="Aide Bookshelf Exemples Favoris GNUstep Livres SOURCES Mailboxes";;
en)
	FOLDERS="Help Bookshelf Samples Favorites GNUstep Books SOURCES Mailboxes";;
esac
for FOLD in ${FOLDERS}
do
	if [ -d $HOME/$FOLD ];then
		rm -f $HOME/$FOLD/.dir.tiff
	fi
done
}

### Cleaning Apps Subfolders
DIR_APP=$(gnustep-config --variable=GNUSTEP_LOCAL_APPS)
for FOLD in $APP_DEVEL $APP_GAMES $APP_TOOLS
do
	cd $DIR_APP
	if [ -f $FOLD/.dir.tiff ];then
		sudo -E rm $FOLD/.dir.tiff
	fi
done

cd $_PWD

clear
info "The default GNUstep theme will apply.\n"
sleep $SLEEP
minimal_setting


#######################################

MESSAGE="A G N O S T E P Theme has been removed."

info "$MESSAGE"

warning "If you are not using a Display Manager, You need to logout and login again to apply the changes..."
MSG="Seconds before logout: "
DELAY=6
timer
exec SCRIPTS/lo.sh

