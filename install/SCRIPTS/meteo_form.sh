#!/bin/bash

####################################################
### A G N o S t e p   Theme  -  by Patrick Cardona
### pcardona34 @ Github
###
### Thanks for the GNUstep Developers Community
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

################################
### Meteo Form
################################

FICHTEMP=$(mktemp /tmp/agno-XXXXX)
trap "rm --force $FICHTEMP" EXIT
METEO_CONF=$HOME/.config/agnostep/meteo.conf
if [ -f ${METEO_CONF} ];then
	. ${METEO_CONF}
	COUNTRY=$(echo $STATION | awk -F, '{print $3}')
	STATE=$(echo $STATION | awk -F, '{print $2}')
	CITY=$(echo $STATION | awk -F, '{print $1}')
else
	COUNTRY=""
	STATE=""
	CITY=""
fi

STATION=""

LG=${LANG:0:2}

case "$LG" in
"fr")
LABEL1="PAYS"
LABEL2="DÉPARTEMENT"
LABEL3="COMMUNE";;
"en"|*)
LABEL1="COUNTRY"
LABEL2="STATE"
LABEL3="CITY";;
esac

dialog                            \
--separate-widget $':'            \
--title "Weather Settings"        \
--form ""                         \
0 0 0                             \
"${LABEL3}:"  1 1 "$CITY"     1 13 30 0  \
"${LABEL2}:"  2 1 "$STATE"    2 13 30 0  \
"${LABEL1}:"  3 1 "$COUNTRY"  3 13 30 0  \
2>$FICHTEMP

if [ $? -eq 0 ];then
	while read LIGNE;do
		STATION="${STATION},$LIGNE"
	done < $FICHTEMP
fi

echo -e "STATION=\"${STATION#,}\"" > ${METEO_CONF}
