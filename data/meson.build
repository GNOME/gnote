install_man('gnote.1')

desktop_file = 'org.gnome.Gnote.desktop'
gnote_desktop_file = i18n.merge_file(
  type: 'desktop',
  input: desktop_file + '.in',
  output: desktop_file,
  po_dir: '../po',
  install: true,
  install_dir: get_option('datadir') / 'applications',
)

desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test('validate-desktop', desktop_file_validate, args: [gnote_desktop_file])
endif

install_data('org.gnome.Gnote.search-provider.ini', install_dir: get_option('datadir') / 'gnome-shell' / 'search-providers')

gnote_gschema = files('org.gnome.gnote.gschema.xml')
install_data(gnote_gschema, install_dir: get_option('datadir') / 'glib-2.0/schemas')
gschema_dtd = gio_dep.get_variable(pkgconfig: 'schemasdir') / 'gschema.dtd'
xmllint = find_program('xmllint', required: false)
if xmllint.found()
  test('validate-gschema', xmllint, args: ['--noout', '--dtdvalid', gschema_dtd, gnote_gschema])
endif

dbus_service_file = 'org.gnome.Gnote.service'
dbus_service_conf = configuration_data()
dbus_service_conf.set('bindir', get_option('prefix') / get_option('bindir'))
dbus_service_conf.set('wrapper', 'gnote')
configure_file(
  input: dbus_service_file + '.in',
  output: dbus_service_file,
  configuration: dbus_service_conf,
  install_dir: get_option('datadir') / 'dbus-1/services',
)

gnote_appdata = 'org.gnome.Gnote.appdata.xml'
appdata_file = i18n.merge_file(
  input: gnote_appdata + '.in',
  output: gnote_appdata,
  po_dir: '../po',
  install: true,
  install_dir: get_option('datadir') / 'metainfo'
)

appstreamcli = find_program('appstreamcli', required: false)
if appstreamcli.found()
  test('validate-appdata', appstreamcli, args: ['validate', '--no-net', '--explain', appdata_file])
endif

install_data('shortcuts-gnote.ui', install_dir: get_option('datadir') / meson.project_name())

subdir('icons')

