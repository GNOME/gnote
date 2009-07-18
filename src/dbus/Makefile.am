

remotecontrol-glue.hpp: gnote-introspect.xml
	dbusxx-xml2cpp $^ --adaptor=$@

BUILT_SOURCES = remotecontrol-glue.hpp
CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

