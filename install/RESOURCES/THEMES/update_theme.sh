#!/bin/bash

THEME=AGNOSTEP
_PWD=`pwd`

cd $HOME/GNUstep/Library/Themes || exit 1

if [ -d ${THEME}.theme ];then
	tar -cvf GSAGNOSTEP.tar ${THEME}.theme
	gzip GSAGNOSTEP.tar
	mv --force GSAGNOSTEP.tar.gz $_PWD/
fi
printf "Theme AGNOSTEP was updated and the archive created.\n"

cd $_PWD
ls -l

