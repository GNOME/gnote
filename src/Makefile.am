


INCLUDE = -I$(top_srcdir)
AM_CPPFLAGS=@LIBGTKMM_CFLAGS@ @LIBGLIBMM_CFLAGS@ \
	@LIBXMLPP_CFLAGS@ @GCONF_CFLAGS@ \
	@GTKSPELL_CFLAGS@ \
	-DDATADIR=\"$(datadir)\"

bin_PROGRAMS = gnote
check_PROGRAMS = trietest
TESTS = trietest


trietest_SOURCES = test/trietest.cpp \
	sharp/string.cpp
trietest_LDADD =  @BOOST_REGEX_LIBS@

gnote_SOURCES = \
	sharp/datetime.hpp sharp/datetime.cpp \
	sharp/directory.hpp sharp/directory.cpp \
	sharp/exception.hpp sharp/exception.cpp \
	sharp/propertyeditor.hpp sharp/propertyeditor.cpp \
	sharp/string.hpp sharp/string.cpp \
	sharp/uri.hpp sharp/uri.cpp \
	sharp/xmlconvert.hpp sharp/xmlconvert.cpp \
	sharp/xmlwriter.hpp sharp/xmlwriter.cpp \
	gnote.cpp gnote.hpp \
	abstractaddin.hpp abstractaddin.cpp \
	actionmanager.hpp actionmanager.cpp \
	addinmanager.hpp addinmanager.cpp \
	applicationaddin.hpp \
	utils.hpp utils.cpp \
	tray.hpp tray.cpp \
	debug.hpp debug.cpp \
	noteaddin.hpp noteaddin.cpp \
	notebuffer.hpp notebuffer.cpp \
	noteeditor.hpp noteeditor.cpp \
	notemanager.hpp notemanager.cpp \
	notetag.hpp notetag.cpp \
	note.hpp note.cpp \
	notewindow.hpp notewindow.cpp \
	preferences.hpp preferences.cpp \
	preferencesdialog.hpp preferencesdialog.cpp \
	recenttreeview.hpp \
	recentchanges.hpp recentchanges.cpp \
	tag.hpp tag.cpp \
	tagmanager.hpp tagmanager.cpp \
	trie.hpp triehit.hpp \
	watchers.hpp watchers.cpp \
	notebooks/createnotebookdialog.hpp notebooks/createnotebookdialog.cpp \
	notebooks/notebook.hpp notebooks/notebook.cpp \
	notebooks/notebookapplicationaddin.hp notebooks/notebookapplicationaddin.cpp \
	notebooks/notebookmanager.hpp notebooks/notebookmanager.cpp \
	notebooks/notebooknewnotemenuitem.hpp notebooks/notebooknewnotemenuitem.cpp \
	$(NULL)

gnote_LDADD =  @LIBGLIBMM_LIBS@ @LIBGTKMM_LIBS@ @LIBXMLPP_LIBS@ \
	@GCONF_LIBS@ @BOOST_FILESYSTEM_LIBS@ @BOOST_REGEX_LIBS@ \
	@GTKSPELL_LIBS@ \
	-luuid -L$(top_builddir)/libtomboy -ltomboy