
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = statistics.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)


addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = libstatistics.la
addins_DATA = $(desktop_files)

libstatistics_la_SOURCES = \
	statisticsapplicationaddin.hpp statisticsapplicationaddin.cpp \
	statisticswidget.hpp statisticswidget.cpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
