project('gnote', 'cpp', version: '41.alpha')

gnote_release = meson.project_version().split('.')[0]
libgnote_version_info = '0.0.0'

dependencies = [
  dependency('glibmm-2.4', version: '>2.62'),
  dependency('giomm-2.4', version: '>2.62'),
  dependency('gtkmm-3.0', version: '>3.22.20'),
  dependency('libsecret-1', version: '>0.8'),
  dependency('libxml-2.0'),
  dependency('libxslt'),
  dependency('uuid'),
]


conf_data = configuration_data()
conf_data.set('package_name', 'gnote')
conf_data.set('package_version', gnote_release)
conf_data.set('version', meson.project_version())
conf_data.set('libgnote_release', gnote_release)
conf_data.set('libgnote_version_info', libgnote_version_info)
conf_data.set('libdir', get_option('prefix') / get_option('libdir'))
conf_data.set('datadir', get_option('prefix') / get_option('datadir'))
conf_data.set('gnote_localedir', conf_data.get('datadir') / 'locale')

configure_file(
  input: 'config.h.in',
  output: 'config.h',
  configuration: conf_data,
)

root_include_dir = include_directories('.')

subdir('src')
