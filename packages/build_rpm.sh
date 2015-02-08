#!/bin/sh

BINFILE=$3
VERSION=$1
ARCH=$2
FULLDIR="git-auto-pull-$VERSION-1.$ARCH"
SEMIDIR="git-auto-pull-$VERSION"
mkdir -p packages/rpm
rm -rf packages/rpm/$SEMIDIR
mkdir -p packages/rpm/$SEMIDIR
cp miscellaneous/qsc.repo miscellaneous/qsc.public.key miscellaneous/config.json miscellaneous/git-auto-pull.initd miscellaneous/git-auto-pull.service packages/rpm/$SEMIDIR
cp $BINFILE packages/rpm/$SEMIDIR/git-auto-pull.bin
cp $BINFILE git-auto-pull.bin
cd packages/rpm
tar zcvf ${SEMIDIR}.tar.gz $SEMIDIR
mkdir -p ~/rpmbuild/SOURCES
cp ${SEMIDIR}.tar.gz ~/rpmbuild/SOURCES
rm ${SEMDIR}.tar.gz
cd ../../
./packages/rpm/build_spec.sh $VERSION
mkdir -p ~/rpmbuild/SPECS
cp git-auto-pull.spec ~/rpmbuild/SPECS
rm git-auto-pull.bin
fakeroot rpmbuild -ba git-auto-pull.spec --target $ARCH
rm git-auto-pull.spec
rm -rf packages/rpm/$SEMIDIR
cp ~/rpmbuild/RPMS/$ARCH/${FULLDIR}.rpm packages/rpm

