
app_data_in_files = gnote.appdata.xml.in
app_data_files = $(app_data_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

appdatadir = $(datadir)/appdata
appdata_DATA = $(app_data_files)


EXTRA_DIST = $(app_data_in_files)

DISTCLEANFILES = $(app_data_files)
