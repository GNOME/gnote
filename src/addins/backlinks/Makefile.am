
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = backlinks.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = backlinks.la
addins_DATA = $(desktop_files)


backlinks_la_SOURCES = backlinksnoteaddin.hpp backlinksnoteaddin.cpp \
	backlinkmenuitem.hpp backlinkmenuitem.cpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
