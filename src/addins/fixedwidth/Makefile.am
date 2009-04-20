


INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
AM_CPPFLAGS=@LIBGTKMM_CFLAGS@ @LIBGLIBMM_CFLAGS@ \
	@GCONF_CFLAGS@ \
	-DDATADIR=\"$(datadir)\" -DLIBDIR=\"$(libdir)\"

AM_LDFLAGS = -avoid-version -module -Wl,--export-dynamic

addinsdir = @libdir@/@PACKAGE_NAME@/addins/@VERSION@
addins_LTLIBRARIES = fixedwidth.la


fixedwidth_la_SOURCES = fixedwidthnoteaddin.hpp fixedwidthnoteaddin.cpp \
	fixedwidthtag.hpp \
	fixedwidthmenuitem.hpp fixedwidthmenuitem.cpp \
	$(NULL)
