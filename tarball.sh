#!/bin/sh

PROJECT=`cat CMakeLists.txt | grep "project(" | sed 's/.*(//' | sed 's/).*//g'`
VERSION=`cat CMakeLists.txt | grep "set(PACKAGE_VERSION" | sed 's/.* "//' | sed 's/\".*//g'`

echo Version: ${PROJECT} - ${VERSION};

rm -rf tarball
mkdir tarball
cd tarball

mkdir ${PROJECT}-${VERSION}
cd ${PROJECT}-${VERSION}

cp -f ../../AUTHORS          .
cp -f ../../INSTALL          .
cp -f ../../LICENSE          .
cp -f ../../README           .
cp -f ../../ChangeLog        .
cp -f ../../CMakeLists.txt   . 
cp -f ../../fb2edit.pro      .
cp -f ../../windows.sh       .
cp -rf ../../3rdparty        .
cp -rf ../../desktop         .
cp -rf ../../source          .

cd ..

tar -cvjf ${PROJECT}-${VERSION}.tar.bz2 ${PROJECT}-${VERSION}
cp ${PROJECT}-${VERSION}.tar.bz2 ${PROJECT}_${VERSION}.orig.tar.bz2
