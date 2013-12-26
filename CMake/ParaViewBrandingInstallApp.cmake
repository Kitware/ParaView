# This is a helper module that can be used to package an APP for MacOSX. It is
# not designed to bring in all *external* dependencies. It's meant to package
# 'what is  built'.
if (NOT APPLE)
  return()
endif()

# cleanup_bundle is called to fillup the .app file with libraries and plugins
# built. This does not package any external libraries, only those that are
# at the specified locations. This will create a workable bundle that works on
# the machine where its built (since it relies on other shared frameworks and
# libraries) e.g. python
macro(cleanup_bundle app app_root libdir pluginsdir datadir)
  # take all libs from ${ARGN} and put it in the Libraries dir.
  file(GLOB_RECURSE dylibs ${libdir}/*.dylib)
  file(GLOB_RECURSE solibs ${libdir}/*.so)

  file(INSTALL ${dylibs} ${solibs}
       DESTINATION ${app_root}/Contents/Libraries
       USE_SOURCE_PERMISSIONS)

  file(GLOB_RECURSE plugins ${pluginsdir}/*)
  file(INSTALL ${plugins}
       DESTINATION ${app_root}/Contents/Plugins
       USE_SOURCE_PERMISSIONS)

  file(GLOB pyfiles ${libdir}/*.py)
  file(GLOB pycfiles ${libdir}/*.pyc)
  if (pycfiles OR pyfiles)
    file(INSTALL ${pyfiles} ${pycfiles}
         DESTINATION ${app_root}/Contents/Python
         USE_SOURCE_PERMISSIONS)
  endif()

  # Handle any python package/module
  file(GLOB pyModules ${libdir}/site-packages/*)
  foreach(pyModule IN LISTS pyModules)
    if (EXISTS "${pyModule}/")
      file(INSTALL "${pyModule}"
           DESTINATION ${app_root}/Contents/Python
           USE_SOURCE_PERMISSIONS)
    endif()
  endforeach()

  # Package web server content
  file(GLOB webFiles ${datadir}/www/*)
  foreach(webFile IN LISTS webFiles)
    if (EXISTS "${webFile}/")
      file(INSTALL "${webFile}"
           DESTINATION ${app_root}/Contents/www
           USE_SOURCE_PERMISSIONS)
    endif()
  endforeach()

  # package other executables such as pvserver.
  get_filename_component(bin_dir "${app_root}" PATH)
  file(GLOB executables "${bin_dir}/*")
  foreach(exe IN LISTS executables)
    if (EXISTS "${exe}" AND NOT IS_DIRECTORY "${exe}")
      file(INSTALL "${exe}"
           DESTINATION "${app_root}/Contents/bin"
           USE_SOURCE_PERMISSIONS)
    endif()
  endforeach()
endmacro()

# When doing unix-style installs, we purge the "app" bundle, instead install the
# executable as a command line executable.
function(convert_bundle_to_executable app app_root bin_dir)
  # copy the executable in the app bundle.
  file(INSTALL ${app}
       DESTINATION ${bin_dir}
       USE_SOURCE_PERMISSIONS)

  # now delete the app bundle.
  file(REMOVE_RECURSE ${app_root})
endfunction()
