#!/bin/sh
##################################################################
#                                                                #
#  This file provides a cross-compilation from Linux to Windows  #
#                                                                #
##################################################################

QT4_VERSION="4.8.3"   # Version Qt4

##################################################################
# Detect project version
##################################################################

PROJECT=`cat CMakeLists.txt | grep "project(" | sed 's/.*(//' | sed 's/).*//g'`
VERSION=`cat CMakeLists.txt | grep "set(PACKAGE_VERSION" | sed 's/.* "//' | sed 's/\".*//g'`

echo Version: ${PROJECT} - ${VERSION};

QT4_FILENAME="qt-everywhere-opensource-src-${QT4_VERSION}"
PRJ_FILENAME="${PROJECT}-${VERSION}"

SOURCE_DIR=`pwd`

cd ~

mkdir "${PRJ_FILENAME}"
cd "${PRJ_FILENAME}"

BUILD_DIR=`pwd`

##################################################################
# Install Qt
##################################################################

cd "${BUILD_DIR}"
#rm -rf "${QT4_FILENAME}"
#wget -c "http://releases.qt-project.org/qt4/source/${QT4_FILENAME}.tar.gz"
#tar -xvzf "${QT4_FILENAME}.tar.gz"
cd "${QT4_FILENAME}"

./configure -prefix /usr/i686-w64-mingw32/usr \
  -opensource -release -confirm-license \
  -xplatform win32-g++ -device-option CROSS_COMPILE=i686-w64-mingw32-
#  -webkit -qt-sql-sqlite -qt-zlib -qt-libpng -qt-libjpeg \
#  -dont-process -no-qt3support -no-multimedia -no-audio-backend -no-phonon \
#  -nomake examples -nomake demos -nomake tools -nomake translations -nomake docs \
#  -no-opengl -no-qt3support -no-declarative -no-multimedia -no-audio-backend -no-phonon \
#  -no-dbus -no-script -no-scripttools  \
#  -no-opengl -no-qt3support -no-declarative -no-multimedia -no-audio-backend -no-phonon \
#  -no-exceptions -no-stl -no-scripttools -no-openssl -no-script -no-openssl \
#  -I /usr/i686-w64-mingw32/include \
#  -L /usr/i686-w64-mingw32/lib \

#make

#sudo make install

#cd "${BUILD_DIR}"
#cmake -DCMAKE_TOOLCHAIN_FILE=../3rdparty/mingw/i686-pc-mingw32.cmake ..
