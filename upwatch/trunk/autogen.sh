#!/bin/sh
set -x

rm -f config.cache 
mkdir -p conf

rm -rf libopts libopts-*
gunzip -c `autoopts-config libsrc` | tar -xf -
mv -f libopts-*.*.* libopts
cp -fp libopts/libopts.m4 conf/.

#gettextize --force --copy
libtoolize --force --copy
aclocal  -I conf
autoheader
autoconf
automake --copy --add-missing
./configure --with-autoopts-config=false --enable-debug $*

