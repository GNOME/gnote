

SUBDIRS = addins

INCLUDES = -I$(top_srcdir)
AM_CPPFLAGS=@LIBGTKMM_CFLAGS@ @LIBGLIBMM_CFLAGS@ \
	@GTK_CFLAGS@ \
	@GTKSPELL_CFLAGS@ @LIBXML_CFLAGS@ \
	@LIBXSLT_CFLAGS@ \
	@UUID_CFLAGS@ \
	@LIBSECRET_CFLAGS@ \
	-DGNOTE_LOCALEDIR=\"@GNOTE_LOCALEDIR@\" \
	-DDATADIR=\"$(datadir)\" -DLIBDIR=\"$(libdir)\"

AM_LDFLAGS=-export-dynamic

LIBGNOTE_LIBS = $(top_builddir)/libtomboy/libtomboy.la \
	@LIBGLIBMM_LIBS@ @LIBGTKMM_LIBS@ \
	@LIBXSLT_LIBS@ \
	@GTKSPELL_LIBS@ @GTK_LIBS@ \
	@UUID_LIBS@ \
	@LIBSECRET_LIBS@
GNOTE_LIBS = libgnote.la $(LIBGNOTE_LIBS)

lib_LTLIBRARIES = libgnote.la
bin_PROGRAMS = gnote
check_PROGRAMS = trietest stringtest notetest dttest uritest filestest \
	fileinfotest xmlreadertest
TESTS = trietest stringtest notetest dttest uritest filestest \
	fileinfotest xmlreadertest


trietest_SOURCES = test/trietest.cpp
trietest_LDADD = libgnote.la @LIBGLIBMM_LIBS@

dttest_SOURCES = test/dttest.cpp
dttest_LDADD = libgnote.la @LIBGLIBMM_LIBS@

stringtest_SOURCES = test/stringtest.cpp
stringtest_LDADD = libgnote.la @LIBGLIBMM_LIBS@

filestest_SOURCES = test/filestest.cpp
filestest_LDADD = libgnote.la @LIBGLIBMM_LIBS@ -lgiomm-2.4

fileinfotest_SOURCES = test/fileinfotest.cpp
fileinfotest_LDADD = libgnote.la @LIBGLIBMM_LIBS@ -lgiomm-2.4

uritest_SOURCES = test/uritest.cpp
uritest_LDADD = libgnote.la @LIBGLIBMM_LIBS@

xmlreadertest_SOURCES = test/xmlreadertest.cpp
xmlreadertest_LDADD = libgnote.la @LIBXML_LIBS@

notetest_SOURCES = test/notetest.cpp
notetest_LDADD =  $(GNOTE_LIBS) -lX11


SUBDIRS += dbus
DBUS_SOURCES=remotecontrolproxy.hpp remotecontrolproxy.cpp \
	dbus/remotecontrol.hpp dbus/remotecontrol.cpp \
	dbus/remotecontrolclient.hpp dbus/remotecontrolclient.cpp \
	dbus/iremotecontrol.hpp \
	dbus/remotecontrol-client-glue.cpp \
	dbus/remotecontrol-glue.cpp \
	$(NULL)

libgnote_la_LDFLAGS = -release @LIBGNOTE_RELEASE@ -version-info @LIBGNOTE_VERSION_INFO@
libgnote_la_LIBADD = $(LIBGNOTE_LIBS)
libgnote_la_SOURCES = \
	base/singleton.hpp \
	base/macros.hpp \
	base/inifile.hpp base/inifile.cpp \
	gnome_keyring/keyringexception.hpp \
	gnome_keyring/ring.hpp gnome_keyring/ring.cpp \
	sharp/datetime.hpp sharp/datetime.cpp \
	sharp/dynamicmodule.hpp sharp/dynamicmodule.cpp \
	sharp/directory.hpp sharp/directory.cpp \
	sharp/exception.hpp sharp/exception.cpp \
	sharp/fileinfo.hpp sharp/fileinfo.cpp \
	sharp/files.hpp sharp/files.cpp \
	sharp/map.hpp \
	sharp/modulefactory.hpp \
	sharp/modulemanager.hpp sharp/modulemanager.cpp \
	sharp/process.hpp sharp/process.cpp \
	sharp/propertyeditor.hpp sharp/propertyeditor.cpp \
	sharp/streamreader.hpp sharp/streamreader.cpp \
	sharp/streamwriter.hpp sharp/streamwriter.cpp \
	sharp/string.hpp sharp/string.cpp \
	sharp/timespan.hpp sharp/timespan.cpp \
	sharp/uri.hpp sharp/uri.cpp \
	sharp/uuid.hpp \
	sharp/xml.hpp sharp/xml.cpp \
	sharp/xmlconvert.hpp sharp/xmlconvert.cpp \
	sharp/xmlreader.hpp sharp/xmlreader.cpp \
	sharp/xmlresolver.hpp \
	sharp/xmlwriter.hpp sharp/xmlwriter.cpp \
	sharp/xsltargumentlist.hpp sharp/xsltargumentlist.cpp \
	sharp/xsltransform.hpp sharp/xsltransform.cpp \
	abstractaddin.hpp abstractaddin.cpp \
	addinmanager.hpp addinmanager.cpp \
	addinpreferencefactory.hpp addinpreferencefactory.cpp \
	applicationaddin.hpp \
	applicationaddin.cpp \
	contrast.hpp contrast.cpp \
	debug.hpp debug.cpp \
	iactionmanager.hpp iactionmanager.cpp \
	iconmanager.hpp iconmanager.cpp \
	ignote.hpp ignote.cpp \
	itagmanager.hpp itagmanager.cpp \
	importaddin.hpp importaddin.cpp \
	mainwindow.hpp mainwindow.cpp \
	noteaddin.hpp noteaddin.cpp \
	notebuffer.hpp notebuffer.cpp \
	noteeditor.hpp noteeditor.cpp \
	notemanager.hpp notemanager.cpp \
	noterenamedialog.hpp noterenamedialog.cpp \
	notetag.hpp notetag.cpp \
	note.hpp note.cpp \
	notewindow.hpp notewindow.cpp \
	preferences.hpp preferences.cpp \
	preferencetabaddin.hpp \
	recenttreeview.hpp \
	search.hpp search.cpp \
	tag.hpp tag.cpp \
	trie.hpp triehit.hpp \
	undo.hpp undo.cpp \
	utils.hpp utils.cpp \
	watchers.hpp watchers.cpp \
	notebooks/createnotebookdialog.hpp notebooks/createnotebookdialog.cpp \
	notebooks/notebook.hpp notebooks/notebook.cpp \
	notebooks/notebookapplicationaddin.hpp notebooks/notebookapplicationaddin.cpp \
	notebooks/notebookmanager.hpp notebooks/notebookmanager.cpp \
	notebooks/notebookmenuitem.hpp notebooks/notebookmenuitem.cpp \
	notebooks/notebooknewnotemenuitem.hpp notebooks/notebooknewnotemenuitem.cpp \
	notebooks/notebooknoteaddin.hpp notebooks/notebooknoteaddin.cpp \
	synchronization/filesystemsyncserver.hpp synchronization/filesystemsyncserver.cpp \
	synchronization/fusesyncserviceaddin.hpp synchronization/fusesyncserviceaddin.cpp \
	synchronization/isyncmanager.hpp synchronization/isyncmanager.cpp \
	synchronization/syncui.hpp synchronization/syncui.cpp \
        synchronization/syncutils.hpp synchronization/syncutils.cpp \
	synchronization/syncserviceaddin.hpp synchronization/syncserviceaddin.cpp \
	$(NULL)


gnote_SOURCES = \
	sharp/addinstreemodel.hpp sharp/addinstreemodel.cpp \
	actionmanager.hpp actionmanager.cpp \
	gnote.cpp gnote.hpp \
	keybinder.hpp \
	preferencesdialog.hpp preferencesdialog.cpp \
	prefskeybinder.hpp prefskeybinder.cpp \
	recentchanges.hpp recentchanges.cpp \
	searchnoteswidget.hpp searchnoteswidget.cpp \
	tagmanager.hpp tagmanager.cpp \
	tray.hpp tray.cpp \
	xkeybinder.hpp xkeybinder.cpp \
	notebooks/notebookstreeview.hpp notebooks/notebookstreeview.cpp \
	synchronization/gnotesyncclient.hpp synchronization/gnotesyncclient.cpp \
	synchronization/silentui.hpp synchronization/silentui.cpp \
	synchronization/syncdialog.hpp synchronization/syncdialog.cpp \
	synchronization/syncmanager.hpp synchronization/syncmanager.cpp \
	$(DBUS_SOURCES)   \
	main.cpp \
	$(NULL)

gnote_LDADD = \
	$(GNOTE_LIBS) \
	-lX11 \
	$(NULL)
