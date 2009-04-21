

SUBDIRS = addins


INCLUDES = -I$(top_srcdir)
AM_CPPFLAGS=@LIBGTKMM_CFLAGS@ @LIBGLIBMM_CFLAGS@ \
	@LIBXMLPP_CFLAGS@ @GCONF_CFLAGS@ @GTK_CFLAGS@ \
	@GTKSPELL_CFLAGS@ \
	-DGNOTE_LOCALEDIR=\"@GNOTE_LOCALEDIR@\" \
	-DDATADIR=\"$(datadir)\" -DLIBDIR=\"$(libdir)\"

AM_LDFLAGS=-Wl,--export-dynamic

GNOTE_LIBS = @LIBGLIBMM_LIBS@ @LIBGTKMM_LIBS@ @LIBXMLPP_LIBS@ \
	@GCONF_LIBS@ @BOOST_FILESYSTEM_LIBS@ @BOOST_REGEX_LIBS@ \
	@GTKSPELL_LIBS@ @GTK_LIBS@ libgnote.a \
	-luuid -L$(top_builddir)/libtomboy -ltomboy

noinst_LIBRARIES = libgnote.a
bin_PROGRAMS = gnote
check_PROGRAMS = trietest stringtest notetest dttest
TESTS = trietest stringtest notetest dttest


trietest_SOURCES = test/trietest.cpp \
	sharp/string.cpp debug.cpp
trietest_LDADD =  @BOOST_REGEX_LIBS@ @LIBGLIBMM_LIBS@

dttest_SOURCES = test/dttest.cpp \
	sharp/datetime.cpp debug.cpp
dttest_LDADD = @LIBGLIBMM_LIBS@ @BOOST_REGEX_LIBS@

stringtest_SOURCES = test/stringtest.cpp \
	sharp/string.cpp debug.cpp
stringtest_LDADD =  @BOOST_REGEX_LIBS@ @LIBGLIBMM_LIBS@

notetest_SOURCES = test/notetest.cpp
notetest_LDADD =  $(GNOTE_LIBS)

gnote_SOURCES = main.cpp

libgnote_a_SOURCES = \
	base/singleton.hpp \
	sharp/addinstreemodel.hpp sharp/addinstreemodel.cpp \
	sharp/datetime.hpp sharp/datetime.cpp \
	sharp/dynamicmodule.hpp sharp/dynamicmodule.cpp \
	sharp/directory.hpp sharp/directory.cpp \
	sharp/exception.hpp sharp/exception.cpp \
	sharp/files.hpp sharp/files.cpp \
	sharp/map.hpp \
	sharp/modulefactory.hpp \
	sharp/modulemanager.hpp sharp/modulemanager.cpp \
	sharp/propertyeditor.hpp sharp/propertyeditor.cpp \
	sharp/string.hpp sharp/string.cpp \
	sharp/uri.hpp sharp/uri.cpp \
	sharp/uuid.hpp \
	sharp/xmlconvert.hpp sharp/xmlconvert.cpp \
	sharp/xmlwriter.hpp sharp/xmlwriter.cpp \
	gnote.cpp gnote.hpp \
	abstractaddin.hpp abstractaddin.cpp \
	actionmanager.hpp actionmanager.cpp \
	addinmanager.hpp addinmanager.cpp \
	addinpreferencefactory.hpp addinpreferencefactory.cpp \
	applicationaddin.hpp \
	contrast.hpp contrast.cpp \
	debug.hpp debug.cpp \
	keybinder.hpp \
	noteaddin.hpp noteaddin.cpp \
	notebuffer.hpp notebuffer.cpp \
	noteeditor.hpp noteeditor.cpp \
	notemanager.hpp notemanager.cpp \
	notetag.hpp notetag.cpp \
	note.hpp note.cpp \
	notewindow.hpp notewindow.cpp \
	preferences.hpp preferences.cpp \
	preferencesdialog.hpp preferencesdialog.cpp \
	preferencetabaddin.hpp \
	prefskeybinder.hpp prefskeybinder.cpp \
	recenttreeview.hpp \
	recentchanges.hpp recentchanges.cpp \
	search.hpp search.cpp \
	tag.hpp tag.cpp \
	tagmanager.hpp tagmanager.cpp \
	tray.hpp tray.cpp \
	trie.hpp triehit.hpp \
	undo.hpp undo.cpp \
	utils.hpp utils.cpp \
	watchers.hpp watchers.cpp \
	xkeybinder.hpp xkeybinder.cpp \
	notebooks/createnotebookdialog.hpp notebooks/createnotebookdialog.cpp \
	notebooks/notebook.hpp notebooks/notebook.cpp \
	notebooks/notebookapplicationaddin.hpp notebooks/notebookapplicationaddin.cpp \
	notebooks/notebookmanager.hpp notebooks/notebookmanager.cpp \
	notebooks/notebookmenuitem.hpp notebooks/notebookmenuitem.cpp \
	notebooks/notebooknewnotemenuitem.hpp notebooks/notebooknewnotemenuitem.cpp \
	notebooks/notebooknoteaddin.hpp notebooks/notebooknoteaddin.cpp \
	notebooks/notebookstreeview.hpp notebooks/notebookstreeview.cpp \
	$(NULL)

gnote_LDADD = $(GNOTE_LIBS)

