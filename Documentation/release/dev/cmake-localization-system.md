## CMake localization system

A translation system has been added to ParaView,
it create translations files from source code,
UI files and XML files.

It is controlled by two new CMake options.

* `PARAVIEW_BUILD_TRANSLATIONS`: Turn on to
  generate translation files on build.

* `PARAVIEW_TRANSLATIONS_DIRECTORY`: Path where
  the translation files will be generated on
  build.
