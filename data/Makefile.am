
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
	sed -e "s|\@bindir\@|$(bindir)|g"	\
	    -e "s|\@wrapper\@|gnote|g"		\
	    < $< > $@

if HAVE_PANELAPPLET
appletdir       = $(datadir)/gnome-panel/4.0/applets
applet_in_files = org.gnome.gnote.panel-applet.in
applet_in_in_files = $(applet_in_files).in
applet_DATA     = $(applet_in_files:.panel-applet.in=.panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
	               -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
	               $< > $@
 
@INTLTOOL_SERVER_RULE@
%.panel-applet: %.panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.gnome.panel.applet.GnoteAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.gnome.panel.applet.GnoteAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@
endif

@INTLTOOL_SERVER_RULE@


man_MANS = gnote.1

gnotedatadir = @datadir@/gnote
gnotedata_DATA = UIManagerLayout.xml \
	GNOME_GnoteApplet.xml

noinst_DATA = 			


EXTRA_DIST = 					\
	UIManagerLayout.xml                     \
	GNOME_GnoteApplet.xml                   \
	$(desktop_in_files)     		\
	$(pkgconfig_in_files)			\
	org.gnome.gnote.gschema.xml.in		\
	$(man_MANS)				\
	$(noinst_DATA)				\
	$(service_in_files)                     \
	$(applet_in_in_files)                   \
	$(dbusservice_in_files)

DISTCLEANFILES = 				\
	$(desktop_files)       			\
	$(pkgconfig_DATA)			\
	$(applet_DATA)                          \
	$(applet_DATA).in                       \
	$(service_DATA)                         \
	$(dbusservice_DATA)			\
	$(gsettings_SCHEMAS)
