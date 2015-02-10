VERSION=0.4.62

all: static64 static32

static64:
	mkdir -p tmp
	gcc -static main.c -D VERSION=\"$(VERSION)\" -Wl,--as-needed -Wa,--noexecstack -I ./tmp/http-parser-bin64/ -L ./tmp/http-parser-bin64/ -I ./tmp/libssh2-bin64/include -L ./tmp/libgit2-bin64 -L ./tmp/json-c-bin64/lib -I ./tmp/libgcrypt-bin64/include -I ./tmp/libgit2/include -I ./tmp/json-c-bin64/include tmp/libssh2-bin64/lib/libssh2.a tmp/http-parser-bin64/libhttp_parser.a tmp/libgit2-bin64/libgit2.a tmp/json-c-bin64/lib/libjson-c.a -o tmp/git-auto-pull-static_amd64 -L ./tmp/libssh2-bin64/lib -lssh2 -L ./tmp/libgcrypt-bin64/lib -l gcrypt -L ./tmp/libgpg-error-bin64/lib -l gpg-error -I ./tmp/openssl-bin64/include -L ./tmp/zlib-bin64/lib -lz -L ./tmp/openssl-bin64/lib -lssl -lcrypto ./tmp/openssl-bin64/lib/libssl.a ./tmp/openssl-bin64/lib/libcrypto.a -L /usr/lib/z86_64-linux-gnu/ -lrt -ldl -m64

static32:
	mkdir -p tmp
	gcc -static main.c -D VERSION=\"$(VERSION)\" -Wl,--as-needed -Wa,--noexecstack -I ./tmp/http-parser-bin32/ -L ./tmp/http-parser-bin32/ -I ./tmp/libssh2-bin32/include -L ./tmp/libgit2-bin32 -L ./tmp/json-c-bin32/lib -I ./tmp/libgcrypt-bin32/include -I ./tmp/libgit2/include -I ./tmp/json-c-bin32/include tmp/libssh2-bin32/lib/libssh2.a tmp/http-parser-bin32/libhttp_parser.a tmp/libgit2-bin32/libgit2.a tmp/json-c-bin32/lib/libjson-c.a -o tmp/git-auto-pull-static_i386 -L ./tmp/libssh2-bin32/lib -lssh2 -L ./tmp/libgcrypt-bin32/lib -l gcrypt -L ./tmp/libgpg-error-bin32/lib -l gpg-error -I ./tmp/openssl-bin32/include -L ./tmp/zlib-bin32/lib -lz -L ./tmp/openssl-bin32/lib -lssl -lcrypto ./tmp/openssl-bin32/lib/libssl.a ./tmp/openssl-bin32/lib/libcrypto.a -L /usr/lib/i386-linux-gnu/ -lrt -ldl -m32

clean:
	rm -rf tmp/git-auto-pull*

deb: static64 static32
	rm -rf packages/deb/*.deb
	mkdir -p packages/deb/git-auto-pull/DEBIAN
	mkdir -p packages/deb/git-auto-pull/etc/git-auto-pull
	mkdir -p packages/deb/git-auto-pull/var/log/git-auto-pull
	mkdir -p packages/deb/git-auto-pull/usr/sbin
	mkdir -p packages/deb/git-auto-pull/usr/share/git-auto-pull
	cp miscellaneous/qsc.public.key packages/deb/git-auto-pull/usr/share/git-auto-pull/qsc.public.key
	cp miscellaneous/git-auto-pull.initd packages/deb/git-auto-pull/usr/share/git-auto-pull/git-auto-pull
	cp miscellaneous/git-auto-pull.service packages/deb/git-auto-pull/usr/share/git-auto-pull/git-auto-pull.service
	cp miscellaneous/config.json packages/deb/git-auto-pull/etc/git-auto-pull/config.json
	rm -rf packages/deb/git-auto-pull/usr/sbin/git-auto-pull
	cp tmp/git-auto-pull-static_amd64 packages/deb/git-auto-pull/usr/sbin/git-auto-pull
	./packages/deb/build_control.sh packages/deb/git-auto-pull/usr/sbin/git-auto-pull $(VERSION) amd64
	mv control packages/deb/git-auto-pull/DEBIAN/control
	fakeroot dpkg -b packages/deb/git-auto-pull packages/deb/git-auto-pull_$(VERSION)_amd64.deb 
	cp tmp/git-auto-pull-static_i386 packages/deb/git-auto-pull/usr/sbin/git-auto-pull
	./packages/deb/build_control.sh packages/deb/git-auto-pull/usr/sbin/git-auto-pull $(VERSION) i386
	mv control packages/deb/git-auto-pull/DEBIAN/control
	fakeroot dpkg -b packages/deb/git-auto-pull packages/deb/git-auto-pull_$(VERSION)_i386.deb
	rm -rf packages/deb/git-auto-pull/usr/sbin/git-auto-pull

rpm: static64 static32
	rm -rf packages/rpm/*.rpm packages/rpm/*.tar.gz
	./packages/build_rpm.sh $(VERSION) x86_64 tmp/git-auto-pull-static_amd64
	./packages/build_rpm.sh $(VERSION) i386 tmp/git-auto-pull-static_i386
	rm -rf packages/rpm/*.tar.gz

.PHONY: all static shared
