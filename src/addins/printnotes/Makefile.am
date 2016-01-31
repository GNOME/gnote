
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = printnotes.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = libprintnotes.la
addins_DATA = $(desktop_files)


libprintnotes_la_SOURCES = printnotesnoteaddin.hpp printnotesnoteaddin.cpp\
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
