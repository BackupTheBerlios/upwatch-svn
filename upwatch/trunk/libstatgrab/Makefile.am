# Makefile for libstatgrab
# http://www.i-scream.org/
# $Id: Makefile.am,v 1.1 2003/12/04 18:59:16 raarts Exp $

AUTOMAKE_OPTIONS = gnu

SUBDIRS = src docs examples

ChangeLog:
	cvs2cl --stdout --no-wrap -r -t -w -S -g "-d :pserver:anonymous@cvs.i-scream.org:/cvs/i-scream" -g "-q" > ChangeLog

EXTRA_DIST = libstatgrab.pc.in PLATFORMS

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libstatgrab.pc
