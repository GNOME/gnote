addins_install_dir = get_option('libdir') / meson.project_name() / 'plugins' / gnote_release

addin_conf = configuration_data()
addin_conf.set('libgnote_release', gnote_release)
addin_conf.set('libgnote_version_info', libgnote_version_info)

msgfmt_plugin_cmd = [
  find_program('msgfmt'),
  '--desktop',
  '--keyword=Name',
  '--keyword=Description',
  '--keyword=Authors',
  '--keyword=Copyright',
  '--template=@INPUT@',
  '-d', join_paths(srcdir, 'po'),
  '--output=@OUTPUT@'
]

subdir('backlinks')
subdir('bugzilla')
subdir('exporttogtg')
subdir('exporttohtml')
subdir('filesystemsyncservice')
subdir('fixedwidth')
subdir('gvfssyncservice')
subdir('inserttimestamp')
subdir('notedirectorywatcher')
subdir('noteoftheday')
subdir('printnotes')
subdir('readonly')
subdir('replacetitle')
subdir('specialnotes')
subdir('statistics')
subdir('tableofcontents')
subdir('todo')
subdir('tomboyimport')
subdir('underline')
subdir('webdavsyncservice')

