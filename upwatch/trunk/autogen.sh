#!/bin/sh
set -x

rm -f config.cache 
aclocal 
autoheader
autoconf
automake --copy --add-missing
./configure --enable-debug

