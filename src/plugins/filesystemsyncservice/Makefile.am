
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = filesystemsyncservice.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = libfilesystemsyncservice.la
addins_DATA = $(desktop_files)


libfilesystemsyncservice_la_SOURCES = filesystemsyncserviceaddin.hpp filesystemsyncserviceaddin.cpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
