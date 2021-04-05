
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = gvfssyncservice.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = libgvfssyncservice.la
addins_DATA = $(desktop_files)


libgvfssyncservice_la_SOURCES = gvfssyncserviceaddin.hpp gvfssyncserviceaddin.cpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
