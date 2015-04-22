#!/bin/sh

if [[ $EUID -ne 0 ]]; then
	echo "I need root privilege!"
	exit 1
fi

APP_DIR=`dirname $0`
echo "Current APP_DIR: $APP_DIR"
set -v

install -d /usr/sbin
install -d /etc/git-auto-pull
install -d /usr/share/git-auto-pull
install -d /var/log/git-auto-pull

install -m 755 $APP_DIR/git-auto-pull /usr/sbin/git-auto-pull
install -m 644 $APP_DIR/config.json /etc/git-auto-pull/config.json
install -m 644 $APP_DIR/git.zjuqsc.com.crt /usr/share/git-auto-pull/git.zjuqsc.com.crt
install -m 644 $APP_DIR/github.com.crt /usr/share/git-auto-pull/github.com.crt
install -m 755 $APP_DIR/install.sh /usr/share/git-auto-pull/install.sh
install -m 755 $APP_DIR/git-auto-pull.initd /usr/share/git-auto-pull/git-auto-pull.initd
install -m 755 $APP_DIR/git-auto-pull.service /usr/share/git-auto-pull/git-auto-pull.service

set +v

echo
echo "Installation almost finished, Please see /etc/git-auto-pull/config.json"
echo " for configuration."
echo
echo "Run /usr/sbin/git-auto-pull as root to start the daemon, or you can"
echo " use git-auto-pull.initd (for sysv) or git-auto-pull.service (for "
echo " systemd) instead. "
echo 


