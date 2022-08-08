#!/bin/sh

LIST="utopic trusty"

NUMBER=1

PROJECT=`cat CMakeLists.txt | grep "project(" | sed 's/.*(//' | sed 's/).*//g'`
VERSION=`cat CMakeLists.txt | grep "set(PACKAGE_VERSION" | sed 's/.* "//' | sed 's/\".*//g'`

echo 
echo "================= ${PROJECT}-${VERSION} ================="
echo 

mkdir tarball
cd tarball

cd ${PROJECT}-${VERSION}

cp -rf ../../debian .

echo "${PROJECT} (${VERSION}-squeeze${NUMBER}) stable; urgency=low" > debian/changelog
cat ../../debian/changelog | sed '1d'>> debian/changelog
debuild -S -sa

mkdir ../osc_${PROJECT}
mv ../*squeeze* ../osc_${PROJECT}
cp ../${PROJECT}_${VERSION}.orig.tar.bz2 ../osc_${PROJECT}

cat ../../debian/control | sed 's/cdbs/cdbs, libqtwebkit-dev/' > debian/control
for DISTRIB in $LIST;
do
  echo "${PROJECT} (${VERSION}-${DISTRIB}${NUMBER}) ${DISTRIB}; urgency=low" > debian/changelog
  cat ../../debian/changelog | sed '1d'>> debian/changelog
  debuild -S -sa
done
