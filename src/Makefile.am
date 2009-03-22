

INCLUDE = -I$(top_srcdir)
AM_CPPFLAGS=@LIBGTKMM_CFLAGS@ @LIBGLIBMM_CFLAGS@ \
	@LIBXMLPP_CFLAGS@ @GCONF_CFLAGS@ \
	-DDATADIR=\"$(datadir)\"

bin_PROGRAMS = gnote

gnote_SOURCES = gnote.cpp gnote.hpp \
	actionmanager.hpp actionmanager.cpp \
	utils.hpp utils.cpp \
	tray.hpp tray.cpp \
	debug.hpp debug.cpp \
	preferences.hpp preferences.cpp \
	preferencesdialog.hpp preferencesdialog.cpp \
	sharp/exception.hpp sharp/exception.cpp \
	sharp/propertyeditor.hpp sharp/propertyeditor.cpp \
	sharp/directory.hpp sharp/directory.cpp \
	sharp/datetime.hpp sharp/datetime.cpp \
	sharp/string.hpp sharp/string.cpp \
	sharp/xmlconvert.hpp sharp/xmlconvert.cpp \
	sharp/xmlwriter.hpp sharp/xmlwriter.cpp \
	notebuffer.hpp notebuffer.cpp \
	noteeditor.hpp noteeditor.cpp \
	notemanager.hpp notemanager.cpp \
	notetag.hpp notetag.cpp \
	note.hpp note.cpp \
	notwindow.hpp notewindow.cpp \
	$(NULL)

gnote_LDADD =  @LIBGLIBMM_LIBS@ @LIBGTKMM_LIBS@ @LIBXMLPP_LIBS@ \
	@GCONF_LIBS@ @BOOST_FILESYSTEM_LIBS@ @BOOST_REGEX_LIBS@ \
	-luuid -L$(top_builddir)/libtomboy -ltomboy