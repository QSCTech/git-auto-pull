#
# Copyright (C) Senorsen (Zhang Sen) <sen@senorsen.com>
#
# This software is distributed under the terms of GPLv3 or later,
# See `LICENSE' for more information. 
#

VERSION=0.4.72
RELEASE=3
LIBDIR=tmp
CFLAGS=
CC=gcc

ifeq ($(WITH_REPO),yes)
	DEB_SHARE_FILES +=  miscellaneous/qsc.public.key miscellaneous/qsc.list 
endif

all: static64 static32

debug:
	mkdir -p tmp
	$(CC) -static main.c -D VERSION=\"$(VERSION)\" -g $(CFLAGS) -Wl,--as-needed -Wa,--noexecstack -I $(LIBDIR)/http-parser-bin64/ -L $(LIBDIR)/http-parser-bin64/ -I $(LIBDIR)/libssh2-bin64/include -L $(LIBDIR)/libgit2-bin64 -L $(LIBDIR)/json-c-bin64/lib -I $(LIBDIR)/libgcrypt-bin64/include -I $(LIBDIR)/libgit2/include -I $(LIBDIR)/json-c-bin64/include $(LIBDIR)/libssh2-bin64/lib/libssh2.a $(LIBDIR)/http-parser-bin64/libhttp_parser.a $(LIBDIR)/libgit2-bin64/libgit2.a $(LIBDIR)/json-c-bin64/lib/libjson-c.a -o tmp/git-auto-pull-debug -L $(LIBDIR)/libssh2-bin64/lib -lssh2 -L $(LIBDIR)/libgcrypt-bin64/lib -l gcrypt -L $(LIBDIR)/libgpg-error-bin64/lib -l gpg-error -I $(LIBDIR)/openssl-bin64/include -L $(LIBDIR)/zlib-bin64/lib -lz -L $(LIBDIR)/openssl-bin64/lib -lssl -lcrypto $(LIBDIR)/openssl-bin64/lib/libssl.a $(LIBDIR)/openssl-bin64/lib/libcrypto.a -L /usr/lib/z86_64-linux-gnu/ -lrt -ldl

install: debug
	install -m 755 tmp/git-auto-pull-debug /usr/sbin/git-auto-pull
	echo init.d or systemd script is not installed.

static64:
	mkdir -p tmp
	$(CC) -static main.c -D VERSION=\"$(VERSION)\" $(CFLAGS) -Wl,--as-needed -Wa,--noexecstack -I $(LIBDIR)/http-parser-bin64/ -L $(LIBDIR)/http-parser-bin64/ -I $(LIBDIR)/libssh2-bin64/include -L $(LIBDIR)/libgit2-bin64 -L $(LIBDIR)/json-c-bin64/lib -I $(LIBDIR)/libgcrypt-bin64/include -I $(LIBDIR)/libgit2/include -I $(LIBDIR)/json-c-bin64/include $(LIBDIR)/libssh2-bin64/lib/libssh2.a $(LIBDIR)/http-parser-bin64/libhttp_parser.a $(LIBDIR)/libgit2-bin64/libgit2.a $(LIBDIR)/json-c-bin64/lib/libjson-c.a -o tmp/git-auto-pull-static_amd64 -L $(LIBDIR)/libssh2-bin64/lib -lssh2 -L $(LIBDIR)/libgcrypt-bin64/lib -l gcrypt -L $(LIBDIR)/libgpg-error-bin64/lib -l gpg-error -I $(LIBDIR)/openssl-bin64/include -L $(LIBDIR)/zlib-bin64/lib -lz -L $(LIBDIR)/openssl-bin64/lib -lssl -lcrypto $(LIBDIR)/openssl-bin64/lib/libssl.a $(LIBDIR)/openssl-bin64/lib/libcrypto.a -L /usr/lib/z86_64-linux-gnu/ -lrt -ldl -m64

static32:
	mkdir -p tmp
	$(CC) -static main.c -D VERSION=\"$(VERSION)\" -Wl,--as-needed -Wa,--noexecstack -I $(LIBDIR)/http-parser-bin32/ -L $(LIBDIR)/http-parser-bin32/ -I $(LIBDIR)/libssh2-bin32/include -L $(LIBDIR)/libgit2-bin32 -L $(LIBDIR)/json-c-bin32/lib -I $(LIBDIR)/libgcrypt-bin32/include -I $(LIBDIR)/libgit2/include -I $(LIBDIR)/json-c-bin32/include $(LIBDIR)/libssh2-bin32/lib/libssh2.a $(LIBDIR)/http-parser-bin32/libhttp_parser.a $(LIBDIR)/libgit2-bin32/libgit2.a $(LIBDIR)/json-c-bin32/lib/libjson-c.a -o tmp/git-auto-pull-static_i386 -L $(LIBDIR)/libssh2-bin32/lib -lssh2 -L $(LIBDIR)/libgcrypt-bin32/lib -l gcrypt -L $(LIBDIR)/libgpg-error-bin32/lib -l gpg-error -I $(LIBDIR)/openssl-bin32/include -L $(LIBDIR)/zlib-bin32/lib -lz -L $(LIBDIR)/openssl-bin32/lib -lssl -lcrypto $(LIBDIR)/openssl-bin32/lib/libssl.a $(LIBDIR)/openssl-bin32/lib/libcrypto.a -L /usr/lib/i386-linux-gnu/ -lrt -ldl -m32

clean:
	rm -rf miscellaneous/*.crt
	rm -rf tmp/git-auto-pull*
	rm -rf packages/deb/*.deb packages/deb/git-auto-pull/
	rm -rf packages/rpm/*.rpm

prepack:
	$(MAKE) -C miscellaneous all

pack: deb rpm

deb: prepack static64 static32
	rm -rf packages/deb/*.deb
	mkdir -p packages/deb/git-auto-pull/DEBIAN
	mkdir -p packages/deb/git-auto-pull/etc/git-auto-pull
	mkdir -p packages/deb/git-auto-pull/var/log/git-auto-pull
	mkdir -p packages/deb/git-auto-pull/usr/sbin
	mkdir -p packages/deb/git-auto-pull/usr/share/git-auto-pull
	$(COPY_DEB_REPO_KEY)
	cp miscellaneous/postinst miscellaneous/postrm miscellaneous/copyright miscellaneous/conffiles miscellaneous/changelog packages/deb/git-auto-pull/DEBIAN/
	cp miscellaneous/git-auto-pull.initd packages/deb/git-auto-pull/usr/share/git-auto-pull/git-auto-pull
	cp $(DEB_SHARE_FILES) miscellaneous/*.crt miscellaneous/git-auto-pull.service packages/deb/git-auto-pull/usr/share/git-auto-pull/
	cp miscellaneous/config.json packages/deb/git-auto-pull/etc/git-auto-pull/config.json
	rm -rf packages/deb/git-auto-pull/usr/sbin/git-auto-pull
	cp tmp/git-auto-pull-static_amd64 packages/deb/git-auto-pull/usr/sbin/git-auto-pull
	./packages/deb/build_control.sh packages/deb/git-auto-pull/usr/sbin/git-auto-pull $(VERSION) $(RELEASE) amd64
	mv control packages/deb/git-auto-pull/DEBIAN/control
	fakeroot dpkg -b packages/deb/git-auto-pull packages/deb/git-auto-pull_$(VERSION)-$(RELEASE)_amd64.deb 
	cp tmp/git-auto-pull-static_i386 packages/deb/git-auto-pull/usr/sbin/git-auto-pull
	./packages/deb/build_control.sh packages/deb/git-auto-pull/usr/sbin/git-auto-pull $(VERSION) $(RELEASE) i386
	mv control packages/deb/git-auto-pull/DEBIAN/control
	fakeroot dpkg -b packages/deb/git-auto-pull packages/deb/git-auto-pull_$(VERSION)-$(RELEASE)_i386.deb
	rm -rf packages/deb/git-auto-pull/

rpm: prepack static64 static32
	rm -rf packages/rpm/*.rpm packages/rpm/*.tar.gz
	./packages/build_rpm.sh $(VERSION) $(RELEASE) x86_64 tmp/git-auto-pull-static_amd64 $(WITH_REPO)
	./packages/build_rpm.sh $(VERSION) $(RELEASE) i386 tmp/git-auto-pull-static_i386 $(WITH_REPO)
	rm -rf packages/rpm/*.tar.gz

.PHONY: all static shared
