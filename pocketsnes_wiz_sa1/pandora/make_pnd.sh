#!/bin/sh

rm -Rf build

mkdir -p build/pocketsnes_hack
mkdir -p build/pocketsnes_hack/options
echo -n "/media" > build/pocketsnes_hack/options/romdir.opt

cp ../pocketsnes build/pocketsnes_hack
cp PXML.xml build/pocketsnes_hack

pnd_make.sh -p pocketsnes_hack_0.1.pnd -d build/pocketsnes_hack -x build/pocketsnes_hack/PXML.xml

