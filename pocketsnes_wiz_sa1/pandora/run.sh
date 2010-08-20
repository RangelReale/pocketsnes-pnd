#!/bin/bash

FBDEV=/dev/fb1

mkdir -p ./options
if [ ! -f ./options/romdir.opt ] ; then
	echo -n /media > ./options/romdir.opt
fi
if [ ! -f ./options/menu.opt ] ; then
	cp menu.defaults ./options/menu.opt
fi

ofbset -fb $FBDEV -pos 80 0 -size 640 480 -mem 614400 -en 1
fbset -fb $FBDEV -g 320 240 320 960 16

op_runfbapp ./pocketsnes

ofbset -fb $FBDEV -pos 0 0 -size 0 0 -mem 0 -en 0

