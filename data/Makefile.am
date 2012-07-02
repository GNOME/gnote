
SUBDIRS = icons

@INTLTOOL_DESKTOP_RULE@

desktop_in_files = gnote.desktop.in
desktop_files    = $(desktop_in_files:.desktop.in=.desktop)

desktopdir   = $(datadir)/applications
desktop_DATA = $(desktop_files)


#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_in_files = tomboy-addins.pc.in
#pkgconfig_DATA = $(pkgconfig_in_files:.pc.in=.pc)


gsettings_SCHEMAS = org.gnome.gnote.gschema.xml
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@


dbusservicedir       = $(DBUS_SERVICE_DIR)
dbusservice_in_files = org.gnome.Gnote.service.in
dbusservice_DATA     = $(dbusservice_in_files:.service.in=.service)

$(dbusservice_DATA): $(dbusservice_in_files) Makefile
	$(AM_V_GEN)sed -e "s|\@bindir\@|$(bindir)|g"	\
	    -e "s|\@wrapper\@|gnote|g"		\
	    < $< > $@

@INTLTOOL_SERVER_RULE@


man_MANS = gnote.1

gnotedatadir = @datadir@/gnote
gnotedata_DATA = UIManagerLayout.xml

noinst_DATA = 			


EXTRA_DIST = 					\
	UIManagerLayout.xml                     \
	$(desktop_in_files)     		\
	$(pkgconfig_in_files)			\
	org.gnome.gnote.gschema.xml.in		\
	$(man_MANS)				\
	$(noinst_DATA)				\
	$(dbusservice_in_files)

DISTCLEANFILES = 				\
	$(desktop_files)       			\
	$(pkgconfig_DATA)			\
	$(dbusservice_DATA)			\
	$(gsettings_SCHEMAS)
