
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = notedirectorywatcher.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = libnotedirectorywatcher.la
addins_DATA = $(desktop_files)


libnotedirectorywatcher_la_SOURCES = \
	notedirectorywatcherapplicationaddin.hpp notedirectorywatcherapplicationaddin.cpp \
	notedirectorywatcherpreferences.hpp notedirectorywatcherpreferences.cpp \
	notedirectorywatcherpreferencesfactory.hpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
