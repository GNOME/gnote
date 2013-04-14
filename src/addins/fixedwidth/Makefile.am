
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = fixedwidth.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = fixedwidth.la
addins_DATA = $(desktop_files)


fixedwidth_la_SOURCES = fixedwidthnoteaddin.hpp fixedwidthnoteaddin.cpp \
	fixedwidthtag.hpp \
	fixedwidthmenuitem.hpp fixedwidthmenuitem.cpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
