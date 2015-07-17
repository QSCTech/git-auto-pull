#!/bin/sh
#
# Copyright (C) Senorsen (Zhang Sen) <sen@senorsen.com>
#
# This software is distributed under the terms of GPLv3 or later,
# See `LICENSE' for more information. 
#

BINFILE=$4
VERSION=$1
RELEASE=$2
ARCH=$3
FULLDIR="git-auto-pull-$VERSION-$RELEASE.$ARCH"
SEMIDIR="git-auto-pull-$VERSION"
mkdir -p packages/rpm
rm -rf packages/rpm/$SEMIDIR
mkdir -p packages/rpm/$SEMIDIR
if [ "$5" = "yes" ]; then
  cp miscellaneous/qsc.repo miscellaneous/qsc.public.key packages/rpm/$SEMIDIR
fi
cp miscellaneous/*.crt miscellaneous/config.json miscellaneous/git-auto-pull.initd miscellaneous/git-auto-pull.service packages/rpm/$SEMIDIR
cp $BINFILE packages/rpm/$SEMIDIR/git-auto-pull.bin
strip packages/rpm/$SEMIDIR/git-auto-pull.bin
cp $BINFILE git-auto-pull.bin
cd packages/rpm
tar zcvf ${SEMIDIR}.tar.gz $SEMIDIR
mkdir -p ~/rpmbuild/SOURCES
cp ${SEMIDIR}.tar.gz ~/rpmbuild/SOURCES
rm ${SEMDIR}.tar.gz
cd ../../
./packages/rpm/build_spec.sh $VERSION $RELEASE $5
mkdir -p ~/rpmbuild/SPECS
cp git-auto-pull.spec ~/rpmbuild/SPECS
rm git-auto-pull.bin
fakeroot rpmbuild -ba git-auto-pull.spec --target $ARCH
rm git-auto-pull.spec
rm -rf packages/rpm/$SEMIDIR
cp ~/rpmbuild/RPMS/$ARCH/${FULLDIR}.rpm packages/rpm

