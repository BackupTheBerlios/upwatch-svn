# Makefile for libstatgrab/docs/saidar
# http://www.i-scream.org/libstatgrab/
# $Id: Makefile.am,v 1.2 2005/11/11 23:37:17 tdb Exp $

if MANPAGES
if SAIDAR
man_MANS = saidar.1
endif
endif

EXTRA_DIST = $(man_MANS)

%.1: %.xml
	docbook2man -N --symlinks $<

