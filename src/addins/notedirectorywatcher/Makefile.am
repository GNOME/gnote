
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = notedirectorywatcher.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = notedirectorywatcher.la
addins_DATA = $(desktop_files)


notedirectorywatcher_la_SOURCES = notedirectorywatcherapplicationaddin.hpp notedirectorywatcherapplicationaddin.cpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
