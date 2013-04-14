
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = stickynoteimport.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = stickynoteimport.la
addins_DATA = $(desktop_files)


stickynoteimport_la_SOURCES = stickynoteimportnoteaddin.hpp stickynoteimportnoteaddin.cpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
