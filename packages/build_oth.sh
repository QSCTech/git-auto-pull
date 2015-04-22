#!/bin/sh
#
# Copyright (C) Senorsen (Zhang Sen) <sen@senorsen.com>
#
# This software is distributed under the terms of GPLv3 or later,
# See `LICENSE' for more information. 
#

set -x

BINFILE=$4
VERSION=$1
RELEASE=$2
ARCH=$3
WORKDIR="git-auto-pull-${VERSION}_${RELEASE}_${ARCH}"
WORKPATH="packages/oth/$WORKDIR"
CURRENT_PATH=`pwd`
mkdir -p packages/oth
rm -rf $WORKPATH
mkdir -p $WORKPATH

install -m 755 $BINFILE $WORKPATH/git-auto-pull
install -m 644 miscellaneous/github.com.crt $WORKPATH/github.com.crt
install -m 644 miscellaneous/git.zjuqsc.com.crt $WORKPATH/git.zjuqsc.com.crt
install -m 755 miscellaneous/git-auto-pull.initd $WORKPATH/git-auto-pull.initd
install -m 755 miscellaneous/git-auto-pull.service $WORKPATH/git-auto-pull.service
install -m 644 miscellaneous/config.json $WORKPATH/config.json
install -m 644 miscellaneous/qsc.public.key $WORKPATH/qsc.public.key
install -m 755 miscellaneous/install.sh $WORKPATH/install.sh

cd packages/oth/
tar czvf "${WORKDIR}.tar.gz" $WORKDIR
cd ../..

rm -rf $WORKPATH

