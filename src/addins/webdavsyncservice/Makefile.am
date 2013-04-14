
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = webdavsyncservice.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = webdavsyncservice.la
addins_DATA = $(desktop_files)

webdavsyncservice_la_SOURCES = webdavsyncserviceaddin.hpp webdavsyncserviceaddin.cpp

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
