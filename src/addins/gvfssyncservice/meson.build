desktop_file = 'gvfssyncservice.desktop'

configured_desktop_file = configure_file(
  input: desktop_file + '.in.in',
  output: desktop_file + '.in',
  configuration: addin_conf,
)

i18n.merge_file(
  desktop_file,
  type: 'desktop',
  input: configured_desktop_file,
  output: desktop_file,
  po_dir: '../../../po',
  install: true,
  install_dir: addins_install_dir,
)

shared_library(
  'gvfssyncservice',
  'gvfssyncserviceaddin.cpp',
  dependencies: dependencies,
  include_directories: [root_include_dir, src_include_dir],
  link_with: libgnote_shared_lib,
  install: true,
  install_dir: addins_install_dir,
)

