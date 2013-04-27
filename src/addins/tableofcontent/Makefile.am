
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = tableofcontent.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = tableofcontent.la
addins_DATA = $(desktop_files)


tableofcontent_la_SOURCES = \
	tableofcontent.hpp \
	tableofcontentnoteaddin.hpp \
	tableofcontentnoteaddin.cpp \
	tableofcontentmenuitem.hpp  \
	tableofcontentmenuitem.cpp  \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
