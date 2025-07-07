libgnote_sources = [
  'base/monitor.cpp',
  'gnome_keyring/ring.cpp',
  'sharp/datetime.cpp',
  'sharp/dynamicmodule.cpp',
  'sharp/directory.cpp',
  'sharp/exception.cpp',
  'sharp/fileinfo.cpp',
  'sharp/files.cpp',
  'sharp/modulemanager.cpp',
  'sharp/propertyeditor.cpp',
  'sharp/streamreader.cpp',
  'sharp/streamwriter.cpp',
  'sharp/string.cpp',
  'sharp/timespan.cpp',
  'sharp/uri.cpp',
  'sharp/xml.cpp',
  'sharp/xmlconvert.cpp',
  'sharp/xmlreader.cpp',
  'sharp/xmlwriter.cpp',
  'sharp/xsltargumentlist.cpp',
  'sharp/xsltransform.cpp',
  'abstractaddin.cpp',
  'addininfo.cpp',
  'addinmanager.cpp',
  'addinpreferencefactory.cpp',
  'applicationaddin.cpp',
  'debug.cpp',
  'iactionmanager.cpp',
  'iconmanager.cpp',
  'ignote.cpp',
  'itagmanager.cpp',
  'importaddin.cpp',
  'mainwindow.cpp',
  'mainwindowaction.cpp',
  'mainwindowembeds.cpp',
  'noteaddin.cpp',
  'notebase.cpp',
  'notebuffer.cpp',
  'noteeditor.cpp',
  'notemanager.cpp',
  'notemanagerbase.cpp',
  'noterenamedialog.cpp',
  'notetag.cpp',
  'note.cpp',
  'notewindow.cpp',
  'popoverwidgets.cpp',
  'preferences.cpp',
  'search.cpp',
  'tag.cpp',
  'tagmanager.cpp',
  'undo.cpp',
  'utils.cpp',
  'watchers.cpp',
  'notebooks/createnotebookdialog.cpp',
  'notebooks/notebook.cpp',
  'notebooks/notebookapplicationaddin.cpp',
  'notebooks/notebookmanager.cpp',
  'notebooks/notebooknamepopover.cpp',
  'notebooks/notebooknoteaddin.cpp',
  'notebooks/specialnotebooks.cpp',
  'synchronization/filesystemsyncserver.cpp',
  'synchronization/gvfssyncservice.cpp',
  'synchronization/gvfstransfer.cpp',
  'synchronization/isyncmanager.cpp',
  'synchronization/syncui.cpp',
  'synchronization/syncutils.cpp',
  'synchronization/syncserviceaddin.cpp',
]

gnote_sources = [
  'sharp/pluginsmodel.cpp',
  'actionmanager.cpp',
  'gnote.cpp',
  'preferencesdialog.cpp',
  'recentchanges.cpp',
  'searchnoteswidget.cpp',
  'notebooks/notebooksview.cpp',
  'synchronization/gnotesyncclient.cpp',
  'synchronization/silentui.cpp',
  'synchronization/syncdialog.cpp',
  'synchronization/syncmanager.cpp',
  'main.cpp',
]
dbus_sources = [
  'remotecontrolproxy.cpp',
  'dbus/remotecontrol.cpp',
  'dbus/remotecontrol-glue.cpp',
  'dbus/searchprovider.cpp',
]

threads_support = dependency('threads')
compiler_flags = [
]

libgnote_shared_lib = shared_library(
  'gnote-@0@'.format(gnote_release),
  [ libgnote_sources, dbus_sources ],
  version: libgnote_version_info,
  dependencies: dependencies,
  include_directories: root_include_dir,
  install: true,
  cpp_args: compiler_flags,
)

gnote = executable(
  'gnote',
  gnote_sources,
  dependencies: [ dependencies, threads_support ],
  include_directories: root_include_dir,
  link_with: libgnote_shared_lib,
  install: true,
  cpp_args: compiler_flags,
)

src_include_dir = include_directories('.')

subdir('plugins')
subdir('dbus')
if unit_test_pp.found()
  subdir('test')
endif

