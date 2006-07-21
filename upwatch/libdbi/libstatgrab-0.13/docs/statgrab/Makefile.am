# Makefile for libstatgrab/docs/statgrab
# http://www.i-scream.org/libstatgrab/
# $Id: Makefile.am,v 1.2 2005/11/11 23:37:20 tdb Exp $

if MANPAGES
if STATGRAB
man_MANS = statgrab.1 statgrab-make-mrtg-config.1 \
	   statgrab-make-mrtg-index.1
endif
endif

EXTRA_DIST = $(man_MANS)

%.1: %.xml
	docbook2man -N --symlinks $<

