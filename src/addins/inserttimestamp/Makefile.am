
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = inserttimestamp.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = inserttimestamp.la
addins_DATA = $(desktop_files)


inserttimestamp_la_SOURCES = inserttimestampnoteaddin.hpp \
	inserttimestampnoteaddin.cpp \
	inserttimestamppreferences.hpp inserttimestamppreferences.cpp \
	inserttimestamppreferencesfactory.hpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
