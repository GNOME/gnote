
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = exporttogtg.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = libexporttogtg.la
addins_DATA = $(desktop_files)

libexporttogtg_la_SOURCES = \
	exporttogtgnoteaddin.hpp exporttogtgnoteaddin.cpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
