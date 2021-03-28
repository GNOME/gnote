install_man('gnote.1')

desktop_file = 'org.gnome.Gnote.desktop'
gnote_desktop_file = i18n.merge_file(
  desktop_file,
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
gschema_dtd = gio_dep.get_pkgconfig_variable('schemasdir') / 'gschema.dtd'
xmllint = find_program('xmllint', required: false)
if xmllint.found()
  test('validate-gschema', xmllint, args: ['--noout', '--dtdvalid', gschema_dtd, gnote_gschema])
endif

