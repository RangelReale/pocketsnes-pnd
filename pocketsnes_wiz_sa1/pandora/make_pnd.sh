#!/bin/sh

rm -Rf build

mkdir -p build/pocketsnes

cp ../pocketsnes PXML.xml snes.png preview.png run.sh menu.defaults build/pocketsnes
chmod +x build/pocketsnes build/pocketsnes/pocketsnes build/pocketsnes/run.sh

mksquashfs build/pocketsnes build/pocket.squash
cat build/pocket.squash PXML.xml snes.png > pocketsnes.pnd

