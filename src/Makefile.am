

AM_CPPFLAGS=@LIBGTKMM_CFLAGS@ @LIBGLIBMM_CFLAGS@ \
	@LIBXMLPP_CFLAGS@ \
	-DDATADIR=\"$(datadir)\"

bin_PROGRAMS = gnote

gnote_SOURCES = gnote.cpp gnote.hpp \
	actionmanager.hpp actionmanager.cpp \
	utils.hpp utils.cpp \
	tray.hpp tray.cpp \
	debug.hpp debug.cpp \
	notemanager.hpp

gnote_LDADD =  @LIBGLIBMM_LIBS@ @LIBGTKMM_LIBS@ @LIBXMLPP_LIBS@