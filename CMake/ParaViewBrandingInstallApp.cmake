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
macro(cleanup_bundle app app_root libdir pluginsdir)
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

  # HACK: this refers to "paraview" directly. Not a good idea.
  set (python_packages_dir "${libdir}/site-packages/paraview")
  if (EXISTS "${python_packages_dir}")
    file(INSTALL DESTINATION ${app_root}/Contents/Python
         TYPE DIRECTORY FILES "${python_packages_dir}"
         USE_SOURCE_PERMISSIONS)
  endif()

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
