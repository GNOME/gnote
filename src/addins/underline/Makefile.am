
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = underline.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = underline.la
addins_DATA = $(desktop_files)


underline_la_SOURCES = underlinenoteaddin.hpp underlinenoteaddin.cpp \
	underlinetag.hpp \
	underlinemenuitem.hpp underlinemenuitem.cpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
