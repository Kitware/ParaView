

# Call this last so the install script runs after the above targets are installed:
#
if(PARAVIEW_INSTALL_THIRD_PARTY_LIBRARIES AND NOT APPLE)

  set(_dir "${PV_INSTALL_LIB_DIR}")
  if(WIN32)
    set(_dir "bin")
  endif(WIN32)

  set(APPS "\${CMAKE_INSTALL_PREFIX}/${_dir}/paraview${CMAKE_EXECUTABLE_SUFFIX}")  # paths to executables

  install(CODE "
include(\"${ParaView_CMAKE_DIR}/BundleUtilities.cmake\")
fixup_bundle(\"${APPS}\"   \"${PARAVIEW_INSTALL_PLUGIN_LIBS}\"   \"${PARAVIEW_INSTALL_LIBRARY_SEARCH_PATH}\")
" COMPONENT BrandedRuntime)

endif(PARAVIEW_INSTALL_THIRD_PARTY_LIBRARIES AND NOT APPLE)

if(APPLE)
  option(PARAVIEW_BUILD_AS_APPLICATION_BUNDLE "Build ParaView as a Mac Application Bundle" ON)
  if(PARAVIEW_BUILD_AS_APPLICATION_BUNDLE)
    set (ParaView_BUNDLE_LOCATION "" CACHE STRING
      "Subdirectory in which to place ParaView app inside a bundle.")

    mark_as_advanced(ParaView_BUNDLE_LOCATION)

    set(distributed_plugins ${PARAVIEW_PLUGINLIST})
    set(PV_EXE_NAME "paraview")
    set(OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT
      "${CMAKE_CURRENT_BINARY_DIR}/ParaView_OSX_MakeStandAloneBundle.cmake")
    set(OSX_MAKE_STANDALONE_BUNDLE_BASH_SCRIPT
      "${CMAKE_CURRENT_BINARY_DIR}/ParaView_OSX_MakeStandAloneBundle.sh")
    
    set (QT_SQLLITE_PLUGINS_DIR ${QT_PLUGINS_DIR}/sqldrivers)

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CompleteBundle.cmake.in"
      "${OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT}" @ONLY IMMEDIATE)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CreateBundle.sh.in"
      "${OSX_MAKE_STANDALONE_BUNDLE_BASH_SCRIPT}" @ONLY IMMEDIATE)

    install(SCRIPT "${OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT}" COMPONENT Bundle)
  endif(PARAVIEW_BUILD_AS_APPLICATION_BUNDLE)
endif(APPLE)
