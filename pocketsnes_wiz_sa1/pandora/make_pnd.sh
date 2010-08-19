#!/bin/sh

rm -Rf build

mkdir -p build/pocketsnes_hack

cp ../pocketsnes PXML.xml snes.png run.sh menu.defaults build/pocketsnes_hack

#pnd_make.sh -p pocketsnes_hack_0.1.pnd -d build/pocketsnes_hack -x build/pocketsnes_hack/PXML.xml
mksquashfs build/pocketsnes_hack build/pocket.squash
cat build/pocket.squash PXML.xml snes.png > pocketsnes.pnd

