
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = todo.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)


addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = todo.la
addins_DATA = $(desktop_files)

todo_la_SOURCES = \
	todonoteaddin.hpp todonoteaddin.cpp \
	$(NULL)

EXTRA_DIST = $(desktop_in_files)
DISTCLEANFILES = $(desktop_files)
