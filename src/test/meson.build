test_sources = [
  'runner.cpp',
  'testgnote.cpp',
  'testnote.cpp',
  'testnotemanager.cpp',
  'testsyncaddin.cpp',
  'testsyncclient.cpp',
  'testsyncmanager.cpp',
  'testtagmanager.cpp',
  'unit/datetimeutests.cpp',
  'unit/directorytests.cpp',
  'unit/filesutests.cpp',
  'unit/fileinfoutests.cpp',
  'unit/gnotesyncclientutests.cpp',
  'unit/hashtests.cpp',
  'unit/noteutests.cpp',
  'unit/notemanagerutests.cpp',
  'unit/stringutests.cpp',
  'unit/syncmanagerutests.cpp',
  'unit/trieutests.cpp',
  'unit/uriutests.cpp',
  'unit/utiltests.cpp',
  'unit/xmldecodertests.cpp',
  'unit/xmlreaderutests.cpp',
]

extra_testee_sources = [
  '../synchronization/gnotesyncclient.cpp',
  '../synchronization/silentui.cpp',
  '../synchronization/syncmanager.cpp',
]

gnoteunittests = executable(
  'gnoteunittests',
  [test_sources, extra_testee_sources],
  dependencies: [ dependencies, unit_test_pp, threads_support ],
  include_directories: [root_include_dir, src_include_dir],
  link_with: libgnote_shared_lib,
)

test('gnote_unit_tests', gnoteunittests)

