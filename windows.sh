#!/bin/sh

QT_WIN32_PREFIX="~/.wine/drive_c/Qt/4.8.4"

rm -rf build-msw
mkdir build-msw
cd build-msw

cmake \
    -D CMAKE_TOOLCHAIN_FILE=../3rdparty/cmake/mingw32-toolchain.cmake \
    -D QT_WIN32_PREFIX=${QT_WIN32_PREFIX} \
    ..

make

