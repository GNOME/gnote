
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = fixedwidth.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = libfixedwidth.la
addins_DATA = $(desktop_files)


libfixedwidth_la_SOURCES = fixedwidthnoteaddin.hpp fixedwidthnoteaddin.cpp \
	fixedwidthtag.hpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
