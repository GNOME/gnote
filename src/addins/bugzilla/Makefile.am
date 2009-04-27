
include $(builddir)/../addins.mk

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = bugzilla.la

icondir = @datadir@/gnote/icons/hicolor/16x16/apps
icon_DATA = bug.png

bugzilla_la_SOURCES = bugzillanoteaddin.hpp bugzillanoteaddin.cpp \
	insertbugaction.hpp insertbugaction.cpp \
	bugzillalink.hpp bugzillalink.cpp \
	bugzillapreferencesfactory.hpp \
	bugzillapreferences.hpp bugzillapreferences.cpp \
	$(srcdir)/../../sharp/fileinfo.cpp \
	$(NULL)
