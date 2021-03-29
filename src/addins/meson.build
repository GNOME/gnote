addins_install_dir = get_option('libdir') / meson.project_name() / 'addins' / meson.project_version()

addin_conf = configuration_data()
addin_conf.set('libgnote_release', gnote_release)
addin_conf.set('libgnote_version_info', libgnote_version_info)

subdir('backlinks')
subdir('bugzilla')

