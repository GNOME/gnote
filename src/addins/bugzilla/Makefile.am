
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = bugzilla.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = bugzilla.la
addins_DATA = $(desktop_files)

icondir = @datadir@/gnote/icons/hicolor/16x16/apps
icon_DATA = bug.png

bugzilla_la_SOURCES = bugzillanoteaddin.hpp bugzillanoteaddin.cpp \
	insertbugaction.hpp insertbugaction.cpp \
	bugzillalink.hpp bugzillalink.cpp \
	bugzillapreferencesfactory.hpp \
	bugzillapreferences.hpp bugzillapreferences.cpp \
	$(NULL)

EXTRA_DIST = bug.png \
	$(desktop_in_files) \
	$(NULL)
DISTCLEANFILES = $(desktop_files)
