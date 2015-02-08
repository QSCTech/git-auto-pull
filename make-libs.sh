#!/bin/sh
# Requirement: sudo apt-get install gcc-multilib g++-multilib libtool autoconf automake libssh-dev libssh-dev:i386 # and etc...

mkdir -p tmp

git clone https://github.com/json-c/json-c.git tmp/json-c 2>&1 >/dev/null || true
git clone https://github.com/libgit2/libgit2.git tmp/libgit2 2>&1 >/dev/null || true
git clone git://git.libssh2.org/libssh2.git tmp/libssh2 2>&1 >/dev/null || true
git clone https://github.com/joyent/http-parser.git tmp/http-parser || true
[ -f tmp/libgcrypt-1.6.2.tar.bz2 ] || wget ftp://ftp.gnupg.org/gcrypt/libgcrypt/libgcrypt-1.6.2.tar.bz2 -O tmp/libgcrypt-1.6.2.tar.bz2
[ -d tmp/libgcrypt-1.6.2 ] || [ -d tmp/libgcrypt ] || (cd tmp; rm -rf libgcrypt libgcrypt-1.6.2; tar xavf libgcrypt-1.6.2.tar.bz2; mv libgcrypt-1.6.2 libgcrypt; cd ..)
[ -f tmp/libgpg-error-1.17.tar.bz2 ] || wget ftp://ftp.gnupg.org/gcrypt/libgpg-error/libgpg-error-1.17.tar.bz2 -O tmp/libgpg-error-1.17.tar.bz2
[ -d tmp/libgpg-error-1.17 ] || [ -d tmp/libgpg-error ] || (cd tmp; rm -rf libgpg-error libgpg-error-1.17; tar xavf libgpg-error-1.17.tar.bz2; mv libgpg-error-1.17 libgpg-error; cd ..)
[ -f tmp/zlib-1.2.8.tar.gz ] || wget http://zlib.net/zlib-1.2.8.tar.gz -O tmp/zlib-1.2.8.tar.gz
[ -d tmp/zlib ] || (cd tmp; rm -rf zlib-1.2.8 zlib; tar xavf zlib-1.2.8.tar.gz; mv zlib-1.2.8 zlib; cd ..)
[ -f tmp/openssl-1.0.2.tar.gz ] || wget https://www.openssl.org/source/openssl-1.0.2.tar.gz -O tmp/openssl-1.0.2.tar.gz
[ -d tmp/openssl ] || (cd tmp; rm -rf openssl-1.0.2 openssl; tar xavf openssl-1.0.2.tar.gz; mv openssl-1.0.2 openssl; cd ..)

PWDA=`pwd`
echo $PWDA

do_compile0() {
	PATH00=$PWDA/tmp/$THINGS
	PATH32=$PWDA/tmp/$THINGS-bin32
	PATH64=$PWDA/tmp/$THINGS-bin64
	if [ -d $PATH32 ]; then
		if [ -d $PATH64 ]; then
			return
		fi
	fi
	rm -rf $PATH32
	rm -rf $PATH64
	mkdir -p $PATH32
	mkdir -p $PATH64
	cd $PATH00
	make clean || true
	rm Makefile
	if [ -n "$OPT32" ]; then
		$OPT32 --prefix=$PATH32
	else
		CCASFLAGS="-m32" CPPFLAGS="-m32" CFLAGS="-m32" ./configure --prefix=$PATH32 $PARA32
	fi
	(make || make) && make install || (echo "Make failed. "; return 1;)
	make clean || true
	if [ -n "$OPT64" ]; then
		$OPT64 --prefix=$PATH64
	else
		CCASFLAGS="-m64" CPPFLAGS="-m64" CFLAGS="-m64" ./configure --prefix=$PATH64 $PARA64
	fi
	(make || make) && make install || (echo "Make failed. "; return 1;)
	cd $PWDA
}

PARA32=""
PARA64=""
THINGS="zlib"
do_compile0

OPT32="setarch i386 ./config -m32 -fPIC shared "
OPT64="setarch x86_64 ./config -m64 -fPIC shared "
THINGS="openssl"
do_compile0

OPT32=""
OPT64=""
PARA32=" --enable-static "
PARA64=" --enable-static "
THINGS=libgpg-error
do_compile0
PARA32=" --with-gpg-error-prefix=$PWDA/tmp/$THINGS-bin32 --disable-asm --enable-static "
PARA64=" --with-gpg-error-prefix=$PWDA/tmp/$THINGS-bin64 --disable-asm --enable-static "
THINGS=libgcrypt
do_compile0

THINGS=json-c

do_compile1() {
	PATH32=$PWDA/tmp/$THINGS-bin32
	PATH64=$PWDA/tmp/$THINGS-bin64
	if [ -d $PATH32 ]; then
		if [ -d $PATH64 ]; then
			return
		fi
	fi
	rm -rf $PATH32
	rm -rf $PATH64
	mkdir -p $PATH32
	mkdir -p $PATH64
	cd tmp/$THINGS
	make clean || true
	sh autogen.sh
	CFLAGS=-m32 ./configure --prefix=$PATH32 && make && make install
	make clean || true
	CFLAGS=-m64 ./configure --prefix=$PATH64 && make && make install
	cd $PWDA
}

do_compile1

THINGS=libssh2

do_compile2() {
	PATH32=$PWDA/tmp/$THINGS-bin32
	PATH64=$PWDA/tmp/$THINGS-bin64
	if [ -d $PATH32 ]; then
		if [ -d $PATH64 ]; then
			return
		fi
	fi
	rm -rf $PATH32
	rm -rf $PATH64
	cd tmp/$THINGS
	make clean || true
	./buildconf
	CFLAGS="-m32" ./configure --prefix=$PATH32 --with-libgcrypt-prefix=$PWDA/tmp/libgcrypt-bin32 && make && make install
	make clean || true
	CFLAGS="-m64" ./configure --prefix=$PATH64 --with-libgcrypt-prefix=$PWDA/tmp/libgcrypt-bin64 && make && make install
	cd $PWDA
}

do_compile2

THINGS=http-parser

do_compile3() {
	PATH32=$PWDA/tmp/$THINGS-bin32
	PATH64=$PWDA/tmp/$THINGS-bin64
	if [ -d $PATH32 ]; then
		if [ -d $PATH64 ]; then
			return
		fi
	fi
	rm -rf $PATH32
	rm -rf $PATH64
	mkdir -p $PATH32
	mkdir -p $PATH64
	cp -r $PWDA/tmp/$THINGS/* $PATH32/
	cp -r $PWDA/tmp/$THINGS/* $PATH64/
	cd $PATH32
	make clean || true
	export CFLAGS="-m32"
	export CPPFLAGS="-m32"
	make package
	make library
	cd $PATH64
	make clean || true
	export CFLAGS="-m64"
	export CPPFLAGS="-m64"
	make package
	make library
	cd $PWDA
}

do_compile3

THINGS=libgit2

do_compile4() {
	PATH32=$PWDA/tmp/$THINGS-bin32
	PATH64=$PWDA/tmp/$THINGS-bin64
	if [ -d $PATH32 ]; then
		if [ -d $PATH64 ]; then
			return
		fi
	fi
	rm -rf $PATH32
	rm -rf $PATH64
	mkdir -p $PATH32
	mkdir -p $PATH64
	cd $PWDA
	cp miscellaneous/senmod-ssh2.patch $PWDA/tmp
	cp miscellaneous/sen2.patch $PWDA/tmp
	TMPSTR_PWD="$PWDA/tmp/libssh2-bin32/include/"
	TMPSTR_PWD=$(echo $TMPSTR_PWD | sed 's|\\|\\\\|g' | sed 's|/|\\/|g')
	sed -i "s/SEN_INCLUDE/$TMPSTR_PWD/g" $PWDA/tmp/senmod-ssh2.patch
	TMPSTR_PWD="$PWDA/tmp/libssh2-bin32/lib/libssh2.so"
	TMPSTR_PWD=$(echo $TMPSTR_PWD | sed 's|\\|\\\\|g' | sed 's|/|\\/|g')
	sed -i "s/SEN_LIB/$TMPSTR_PWD/g" $PWDA/tmp/senmod-ssh2.patch
	cd $PWDA/tmp/$THINGS
	git checkout CMakeLists.txt
	patch -f < $PWDA/tmp/senmod-ssh2.patch
	patch -f < $PWDA/tmp/sen2.patch
	cd $PATH32
	cmake $PWDA/tmp/$THINGS -DCMAKE_SHARED_LINKER_FLAGS="-m32" -DCMAKE_C_FLAGS="-m32" -DCMAKE_FIND_ROOT_PATH="$PWDA/tmp/libssh2-bin32/" -DZLIB_LIBRARY="$PWDA/tmp/zlib-bin64/lib/libz.so" -DOPENSSL_SSL_LIBRARY="$PWDA/tmp/openssl-bin32/lib/libssl.so" -DOPENSSL_CRYPTO_LIBRARY="$PWDA/tmp/openssl-bin32/lib/libcrypto.so" -DCMAKE_LIBRARY_PATH="$PWDA/tmp/libssh2-bin32/lib/" -DBUILD_SHARED_LIBS="OFF" -DBUILD_CLAR="OFF" -DTHREADSAFE="OFF" -DCMAKE_INSTALL_PREFIX=$PATH32
	cmake --build .
	rm $PWDA/tmp/senmod-ssh2.patch
	rm $PWDA/tmp/sen2.patch
	cd $PWDA
	cp miscellaneous/senmod-ssh2.patch $PWDA/tmp
	cp miscellaneous/sen2.patch $PWDA/tmp
	TMPSTR_PWD="$PWDA/tmp/libssh2-bin64/include/"
	TMPSTR_PWD=$(echo $TMPSTR_PWD | sed 's|\\|\\\\|g' | sed 's|/|\\/|g')
	sed -i "s/SEN_INCLUDE/$TMPSTR_PWD/g" $PWDA/tmp/senmod-ssh2.patch
	TMPSTR_PWD="$PWDA/tmp/libssh2-bin64/lib/libssh2.so"
	TMPSTR_PWD=$(echo $TMPSTR_PWD | sed 's|\\|\\\\|g' | sed 's|/|\\/|g')
	sed -i "s/SEN_LIB/$TMPSTR_PWD/g" $PWDA/tmp/senmod-ssh2.patch
	cd $PWDA/tmp/$THINGS
	git checkout CMakeLists.txt
	patch -f < $PWDA/tmp/senmod-ssh2.patch
	patch -f < $PWDA/tmp/sen2.patch
	cd $PATH64
	cmake $PWDA/tmp/$THINGS -DCMAKE_SHARED_LINKER_FLAGS="-m64" -DCMAKE_C_FLAGS="-m64" -DCMAKE_FIND_ROOT_PATH="$PWDA/tmp/libssh2-bin64/" -DZLIB_LIBRARY="$PWDA/tmp/zlib-bin64/lib/libz.so" -DOPENSSL_SSL_LIBRARY="$PWDA/tmp/openssl-bin32/lib/libssl.so" -DOPENSSL_CRYPTO_LIBRARY="$PWDA/tmp/openssl-bin32/lib/libcrypto.so" -DCMAKE_LIBRARY_PATH="$PWDA/tmp/libssh2-bin64/lib/" -DBUILD_SHARED_LIBS="OFF" -DBUILD_CLAR="OFF" -DTHREADSAFE="OFF" -DCMAKE_INSTALL_PREFIX=$PATH64
	cmake --build .
	cd $PWDA
}

do_compile4

