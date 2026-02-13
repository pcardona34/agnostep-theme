#!/bin/bash

####################################################
### A G N o S t e p   Theme  -  by Patrick Cardona
### pcardona34 @ Github
###
### Thanks for the GNUstep Developers Community
### This is Free and Open Source software.
### Read License in the root directory.
####################################################

####################################################
### Functions for meteo queries and conf
####################################################

LAT=""
LONG=""

function read_meteo_conf
{
METEO_CONF=$HOME/.config/agnostep/meteo.conf
if [ -f ${METEO_CONF} ];then
	STATION=$(cat ${METEO_CONF} | grep . | awk -F= '{print $2}')
	LAT=$(echo $STATION | awk -F: '{print $1}')
	LONG=$(echo $STATION | awk -F: '{print $2}')
else
	LAT=""
	LONG=""
fi
}


function write_meteo_conf
{
read_meteo_conf

FICHTEMP=$(mktemp /tmp/agno-XXXXX)
trap "rm --force $FICHTEMP" EXIT
LG=${LANG:0:2}

case "$LG" in
"fr"|"en"|*)
LABEL1="Latitude"
LABEL2="Longitude";;
esac

STATION=""

dialog --no-shadow                \
 --separate-widget $':'           \
 --title "Weather Settings"       \
 --form "
Use a decimal dot: "              \
0 0 0                             \
"${LABEL1}:"  1 1 "$LAT"     1 13 30 0  \
"${LABEL2}:"  2 1 "$LONG"    2 13 30 0  2>$FICHTEMP

if [ $? -eq 0 ];then
	while read LIGNE;do
		STATION="${STATION}:$LIGNE"
	done < $FICHTEMP
fi

if [ -n "$STATION" ];then
	echo -e "STATION=${STATION#:}" > ${METEO_CONF}
fi
}

################################
### Fetch weather infos
### Dep: jq
################################

function fetch_meteo
{
read_meteo_conf
if [ -z "$LAT" ] || [ -z "$LONG" ];then
	exit 1
fi

JSON=$(mktemp /tmp/agno-XXXXX-meteo.json)
API="api.open-meteo.com"
PARAMS="latitude=${LAT}&longitude=${LONG}&current=temperature_2m,wind_speed_10m,snowfall,rain"

wget --quiet -O $JSON "https://${API}/v1/forecast?${PARAMS}"

### QUERIES ###

HEAT=$(cat $JSON | jq '.["current"]' | jq '.["temperature_2m"]')
WIND=$(cat $JSON | jq '.["current"]' | jq '.["wind_speed_10m"]')
SNOW=$(cat $JSON | jq '.["current"]' | jq '.["snowfall"]')
RAIN=$(cat $JSON | jq '.["current"]' | jq '.["rain"]')
}
