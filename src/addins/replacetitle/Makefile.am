
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = replacetitle.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = replacetitle.la
addins_DATA = $(desktop_files)

replacetitle_la_SOURCES = \
	replacetitlenoteaddin.cpp \
	replacetitlenoteaddin.hpp


EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
