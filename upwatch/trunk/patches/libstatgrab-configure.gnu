#!/bin/sh
#
# wrapper for configure, so we can call it with proper defaults

echo ./configure $* --disable-manpages --disable-examples
./configure "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9" "${10}" \
   "${11}" "${12}" "${13}" "${14}" "${15}" "${16}" "${17}" "${18}" "${19}" "${20}" \
   --disable-manpages --disable-examples

