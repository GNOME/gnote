
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = noteoftheday.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = libnoteoftheday.la
addins_DATA = $(desktop_files)

libnoteoftheday_la_SOURCES = \
	noteoftheday.hpp \
	noteoftheday.cpp \
	noteofthedayapplicationaddin.hpp \
	noteofthedayapplicationaddin.cpp \
	noteofthedaypreferences.hpp \
	noteofthedaypreferences.cpp \
	noteofthedaypreferencesfactory.hpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
