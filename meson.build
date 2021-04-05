project('gnote', 'cpp', version: '41.alpha', meson_version: '>=0.50.0')

gnome = import('gnome')
i18n = import('i18n')
python = import('python')

gnote_release = meson.project_version().split('.')[0]
libgnote_version_info = '0.0.0'

glib_version = '>=2.62'
gio_dep = dependency('gio-2.0', version: glib_version)

dependencies = [
  dependency('glibmm-2.4', version: glib_version),
  dependency('giomm-2.4', version: glib_version),
  dependency('gtkmm-3.0', version: '>=3.22.20'),
  dependency('libsecret-1', version: '>=0.8'),
  dependency('libxml-2.0'),
  dependency('libxslt'),
  dependency('uuid'),
]

gspell = dependency('gspell-1', version: '>=1.6.0', required: false)
if gspell.found()
  dependencies = [dependencies, gspell]
  add_project_arguments('-DENABLE_GSPELL=1', language: 'cpp')
endif

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG', language: 'cpp')
endif


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
srcdir = meson.current_source_dir()

meson.add_install_script(python.find_installation('python3').path(),
  join_paths(meson.source_root(), 'post-install.py'))

subdir('data')
subdir('help')
subdir('src')
subdir('po')

