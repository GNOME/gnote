desktop_file = 'bugzilla.desktop'

configured_desktop_file = configure_file(
  input: desktop_file + '.in.in',
  output: desktop_file + '.in',
  configuration: addin_conf,
)


custom_target(
  desktop_file,
  input: configured_desktop_file,
  output: desktop_file,
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: addins_install_dir,
)

install_data('bug.png', install_dir: get_option('datadir') / meson.project_name()/ 'icons/hicolor/16x16/apps')

shared_library(
  'bugzilla',
  [
    'bugzillanoteaddin.cpp',
    'insertbugaction.cpp',
    'bugzillalink.cpp',
    'bugzillapreferences.cpp',
  ],
  dependencies: dependencies,
  include_directories: [root_include_dir, src_include_dir],
  link_with: libgnote_shared_lib,
  install: true,
  install_dir: addins_install_dir,
  cpp_args: compiler_flags,
)

