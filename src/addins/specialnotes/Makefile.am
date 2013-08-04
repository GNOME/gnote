
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = specialnotes.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = specialnotes.la
addins_DATA = $(desktop_files)

specialnotes_la_SOURCES = \
	specialnotesapplicationaddin.hpp \
	specialnotesapplicationaddin.cpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
