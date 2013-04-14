
include $(builddir)/../addins.mk

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = exporttohtml.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

INCLUDES = @LIBXSLT_CFLAGS@

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = exporttohtml.la
addins_DATA = $(desktop_files)

xsldir = @datadir@/gnote
xsl_DATA = exporttohtml.xsl

exporttohtml_la_LIBADD = @LIBXSLT_LIBS@

exporttohtml_la_SOURCES = exporttohtmlnoteaddin.hpp exporttohtmlnoteaddin.cpp \
	exporttohtmldialog.hpp exporttohtmldialog.cpp \
	notenameresolver.hpp \
	$(NULL)

EXTRA_DIST = exporttohtml.xsl \
	$(desktop_in_files) \
	$(NULL)
DISTCLEANFILES = $(desktop_files)
