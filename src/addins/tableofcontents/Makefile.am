
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = tableofcontents.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = libtableofcontents.la
addins_DATA = $(desktop_files)


libtableofcontents_la_SOURCES = \
	tableofcontents.hpp          \
	tableofcontentsnoteaddin.hpp \
	tableofcontentsnoteaddin.cpp \
	tableofcontentsmenuitem.hpp  \
	tableofcontentsmenuitem.cpp  \
	tableofcontentsutils.hpp     \
	tableofcontentsutils.cpp     \
	$(NULL)

EXTRA_DIST     = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
