#!/bin/sh
set -x

rm -f config.cache 
#gettextize --force --copy
libtoolize --force --copy
aclocal 
autoheader
autoconf
automake --copy --add-missing
./configure --enable-debug $*

