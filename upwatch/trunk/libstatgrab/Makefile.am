# Makefile for libstatgrab
# http://www.i-scream.org/libstatgrab/
# $Id: Makefile.am,v 1.2 2004/05/30 19:56:28 raarts Exp $

AUTOMAKE_OPTIONS = gnu

SUBDIRS = src docs examples

ChangeLog:
	cvs2cl --stdout --no-wrap -r -t -w -S -g "-d :pserver:anonymous@cvs.i-scream.org:/cvs/i-scream" -g "-q" > ChangeLog

EXTRA_DIST = libstatgrab.pc.in PLATFORMS COPYING COPYING.LGPL

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libstatgrab.pc
