#------------------------------------------------------------------------------
# Add extra library containing custom code for the client.
QT4_WRAP_CPP(MOC_BUILT_SOURCES
  ParaViewMainWindow.h)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ParaViewMainWindow.ui
)

source_group("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

set (QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

set(ParaView_SOURCE_FILES
  ParaViewMainWindow.cxx
  ParaViewMainWindow.h
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES})

#------------------------------------------------------------------------------
# Add install rules for required system runtimes such as MSVCRxx.dll
set (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP ON)
include(InstallRequiredSystemLibraries)
if (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
  install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
    DESTINATION ${PV_INSTALL_BIN_DIR}
    PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ
    COMPONENT Runtime)

  # Install the runtimes to the lib dir as well since python modules are
  # installed in that directory and the manifest files need to present there as
  # well.
  install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
    DESTINATION ${PV_INSTALL_LIB_DIR}
    PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ
    COMPONENT Runtime)
endif (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)

#------------------------------------------------------------------------------
# Build the client
build_paraview_client(paraview
    APPLICATION_NAME "ParaView"
    TITLE "ParaView ${PARAVIEW_VERSION_FULL} ${PARAVIEW_BUILD_ARCHITECTURE}-bit"
    ORGANIZATION  "ParaView"
    VERSION_MAJOR ${PARAVIEW_VERSION_MAJOR} 
    VERSION_MINOR ${PARAVIEW_VERSION_MINOR}
    VERSION_PATCH ${PARAVIEW_VERSION_PATCH}
    SPLASH_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/PVSplashScreen.png"
    PVMAIN_WINDOW ParaViewMainWindow
    PVMAIN_WINDOW_INCLUDE ParaViewMainWindow.h
    BUNDLE_ICON   "${CMAKE_CURRENT_SOURCE_DIR}/MacIcon.icns"
    APPLICATION_ICON  "${CMAKE_CURRENT_SOURCE_DIR}/WinIcon.ico"
    GUI_CONFIGURATION_XMLS
      ${CMAKE_CURRENT_SOURCE_DIR}/ParaViewSources.xml
      ${CMAKE_CURRENT_SOURCE_DIR}/ParaViewFilters.xml
      ${CMAKE_CURRENT_SOURCE_DIR}/ParaViewReaders.xml
      ${CMAKE_CURRENT_SOURCE_DIR}/ParaViewWriters.xml
      ${PARAVIEW_EXTRA_GUI_RESOURCES} # Add any compiled in GUI configuration xmls
    COMPRESSED_HELP_FILE "${ParaView_BINARY_DIR}/Documentation/paraview.qch"
    SOURCES ${ParaView_SOURCE_FILES}
    INSTALL_BIN_DIR "${PV_INSTALL_BIN_DIR}"
    INSTALL_LIB_DIR "${PV_INSTALL_LIB_DIR}")

# link enabled plugins if not building in shared library mode
if(NOT BUILD_SHARED_LIBS)
  target_link_libraries(paraview ${PARAVIEW_PLUGINLIST})
endif(NOT BUILD_SHARED_LIBS)

# This ensure that the paraview.qch file is generated.
add_dependencies(${paraview_client_real_exe_name}
  ParaViewOnlineHelp)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif (BUILD_TESTING)


#########################################################################
# Add install rules for installing certain dependencies.

if (PARAVIEW_INSTALL_THIRD_PARTY_LIBRARIES AND NOT APPLE)

  if(WIN32)
   set(pv_runtime_dest ${PV_INSTALL_BIN_DIR})
  else()
   set(pv_runtime_dest ${PV_INSTALL_LIB_DIR})
  endif()

  # Now install the plugins for sqldrivers. For use by the help system
  install(DIRECTORY ${QT_PLUGINS_DIR}/sqldrivers 
          DESTINATION ${pv_runtime_dest}
          COMPONENT Runtime)

  install(DIRECTORY ${QT_PLUGINS_DIR}/imageformats 
          DESTINATION ${pv_runtime_dest}
          COMPONENT Runtime)

  if(PARAVIEW_ENABLE_PYTHON)
    option(PARAVIEW_INSTALL_PYTHON "Install core python modules in ParaView." OFF)
    mark_as_advanced(PARAVIEW_INSTALL_PYTHON)

    if(PARAVIEW_INSTALL_PYTHON)
      if(WIN32)
        execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(standard_lib=1)"
                        OUTPUT_VARIABLE PYTHON_LIBS_DIR
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
        set(PYTHON_LIBS_DEST ${pv_runtime_dest})
      else(WIN32)
        get_filename_component(PYTHON_LIBS_DIR "${PYTHON_LIBRARY}" PATH)
        file(TO_CMAKE_PATH ${PYTHON_LIBS_DIR} PYTHON_LIBS_DIR)

        # install pyconfig.h
        string(REGEX MATCH "python[0-9].[0-9]" py_ver_dir ${PYTHON_INCLUDE_DIR})
        install(FILES ${PYTHON_INCLUDE_DIR}/pyconfig.h
                DESTINATION ${PV_INSTALL_LIB_DIR}/include/${py_ver_dir}
                PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ
                COMPONENT Runtime)
        set(PYTHON_LIBS_DIR ${PYTHON_LIBS_DIR}/${py_ver_dir})
        set(PYTHON_LIBS_DEST ${pv_runtime_dest}/lib)
        
        file(GLOB python_modules "${ParaView_BINARY_DIR}/bin/*Python.so")

        install(FILES ${python_modules}
          DESTINATION ${pv_runtime_dest}
          COMPONENT Runtime)

      endif(WIN32)
      
      file(TO_CMAKE_PATH ${PYTHON_LIBS_DIR} PYTHON_LIBS_DIR)
      install(DIRECTORY ${PYTHON_LIBS_DIR}
              DESTINATION ${PYTHON_LIBS_DEST}
              USE_SOURCE_PERMISSIONS
              COMPONENT Runtime
              REGEX "lib2to3/" EXCLUDE
              REGEX "lib[-]old/" EXCLUDE
              REGEX "plat[-].*" EXCLUDE
              REGEX "wsgiref*" EXCLUDE
              REGEX "distutils/" EXCLUDE)
    endif(PARAVIEW_INSTALL_PYTHON)
  endif(PARAVIEW_ENABLE_PYTHON)

  set(DIRS) # directories to search for prerequisites

  if(PARAVIEW_BUILD_QT_GUI)
    list(APPEND DIRS ${QT_BINARY_DIR} ${QT_LIBRARY_DIR})
  endif(PARAVIEW_BUILD_QT_GUI)

  if(VTK_USE_FFMPEG_ENCODER)
    get_filename_component(FFMPEG_DIR "${FFMPEG_avcodec_LIBRARY}" PATH)
    list(APPEND DIRS ${FFMPEG_DIR})
  endif(VTK_USE_FFMPEG_ENCODER)

  if(PARAVIEW_ENABLE_PYTHON)
    get_filename_component(PYTHON_BIN_DIR "${PYTHON_EXECUTABLE}" PATH)
    get_filename_component(PYTHON_LIB_DIR "${PYTHON_LIBRARY}" PATH)
    list(APPEND DIRS ${PYTHON_BIN_DIR} ${PYTHON_LIB_DIR})
  endif(PARAVIEW_ENABLE_PYTHON)

  if(PARAVIEW_BUILD_PLUGIN_Manta)
    if(WIN32)
      list(APPEND DIRS ${MANTA_BUILD}/lib/${CMAKE_CFG_INTDIR})
    else()
      list(APPEND DIRS ${MANTA_BUILD}/lib)
    endif()
  endif(PARAVIEW_BUILD_PLUGIN_Manta)

  if(PARAVIEW_USE_MPI)
    get_filename_component(MPI_LIB_DIR "${MPI_LIBRARY}" PATH)
    get_filename_component(MPI_BIN_DIR "${MPIEXEC}" PATH)
    list(APPEND DIRS ${MPI_LIB_DIR} ${MPI_BIN_DIR})

    option(PARAVIEW_INSTALL_MPIEXEC "Include the MPI executable in an installation." OFF)
    mark_as_advanced(PARAVIEW_INSTALL_MPIEXEC)
    if(PARAVIEW_INSTALL_MPIEXEC AND MPIEXEC)
      GET_FILENAME_COMPONENT(mpi_bin_dir ${MPIEXEC} PATH)
      install(DIRECTORY ${mpi_bin_dir}/
        DESTINATION ${pv_runtime_dest}
        USE_SOURCE_PERMISSIONS
        COMPONENT Runtime
        PATTERN "mpicc*" EXCLUDE
        PATTERN "mpicxx*" EXCLUDE
        PATTERN "mpic++*" EXCLUDE
        PATTERN "python*" EXCLUDE)
    endif(PARAVIEW_INSTALL_MPIEXEC AND MPIEXEC)
  endif(PARAVIEW_USE_MPI)

  set(_dir "${PV_INSTALL_LIB_DIR}")
  if(WIN32)
    set(_dir "bin")
  endif(WIN32)

  if(NOT PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS)
    set(PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${DIRS} CACHE STRING
      "Directories for FixupBundle to search for dependencies" FORCE)
  endif()
  mark_as_advanced(PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS)
  
  set(search_path ${PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS})
  list(APPEND search_path ${DIRS})
  list(REMOVE_DUPLICATES search_path)

  set(APPS "\${CMAKE_INSTALL_PREFIX}/${_dir}/paraview${CMAKE_EXECUTABLE_SUFFIX}")  # paths to executables

  if(WIN32)
    foreach(pluginname ${PARAVIEW_PLUGINLIST})
      list(APPEND plugin_libs "${EXECUTABLE_OUTPUT_PATH}/Release/${pluginname}.dll")
    endforeach(pluginname ${PARAVIEW_PLUGINLIST})
  else()
    foreach(pluginname ${PARAVIEW_PLUGINLIST})
      list(APPEND plugin_libs "${EXECUTABLE_OUTPUT_PATH}/lib${pluginname}.so")
    endforeach(pluginname ${PARAVIEW_PLUGINLIST})
  endif()

  set(PARAVIEW_INSTALL_APPS ${APPS} CACHE INTERNAL "" FORCE)
  set(PARAVIEW_INSTALL_PLUGIN_LIBS ${plugin_libs} CACHE INTERNAL "" FORCE)
  set(PARAVIEW_INSTALL_LIBRARY_SEARCH_PATH ${search_path} CACHE INTERNAL "" FORCE)

endif(PARAVIEW_INSTALL_THIRD_PARTY_LIBRARIES AND NOT APPLE)

#------------------------------------------------------------------------
# Import site specific installation rules file, if any.
set (PARAVIEW_EXTRA_INSTALL_RULES_FILE "" CACHE FILEPATH 
  "Path to site-specific install rules for ParaView.")
mark_as_advanced(PARAVIEW_EXTRA_INSTALL_RULES_FILE)


foreach(rule_file ${PARAVIEW_EXTRA_INSTALL_RULES_FILE})
  if (EXISTS "${rule_file}")
    include("${rule_file}")
  endif (EXISTS "${rule_file}")
endforeach(rule_file ${PARAVIEW_EXTRA_INSTALL_RULES_FILE})


#------------------------------------------------------------------------
# Install the .plugins configuration file.
install(FILES "${EXECUTABLE_OUTPUT_PATH}/.plugins"
        DESTINATION ${PV_INSTALL_PLUGIN_DIR}
        COMPONENT BrandedRuntime)

# Call this last so the install script runs after the above targets are installed:
#
#if(APPLE)
#
#  option(PARAVIEW_BUILD_AS_APPLICATION_BUNDLE "Build ParaView as a Mac Application Bundle" ON)
#  if(PARAVIEW_BUILD_AS_APPLICATION_BUNDLE)
#    set (ParaView_BUNDLE_LOCATION "" CACHE STRING
#      "Subdirectory in which to place ParaView app inside a bundle.")

#    mark_as_advanced(ParaView_BUNDLE_LOCATION)

#    set(distributed_plugins ${PARAVIEW_PLUGINLIST})
#    set(PV_EXE_NAME "paraview")
#    set(OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT
#      "${CMAKE_CURRENT_BINARY_DIR}/ParaView_OSX_MakeStandAloneBundle.cmake")
#    set(OSX_MAKE_STANDALONE_BUNDLE_BASH_SCRIPT
#      "${CMAKE_CURRENT_BINARY_DIR}/ParaView_OSX_MakeStandAloneBundle.sh")
    
#    set (QT_SQLLITE_PLUGINS_DIR ${QT_PLUGINS_DIR}/sqldrivers)

#    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CompleteBundle.cmake.in"
#      "${OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT}" @ONLY IMMEDIATE)
#    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CreateBundle.sh.in"
#      "${OSX_MAKE_STANDALONE_BUNDLE_BASH_SCRIPT}" @ONLY IMMEDIATE)

#    install(SCRIPT "${OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT}" COMPONENT Bundle)
#  endif(PARAVIEW_BUILD_AS_APPLICATION_BUNDLE)
#endif(APPLE)

# Call FixupBundle in a subdirectory
add_subdirectory(LastConfigureStep)

set(_package_name "ParaView")
if(PARAVIEW_INSTALL_DEVELOPMENT)
  set(_package_name "ParaView-Development")
endif(PARAVIEW_INSTALL_DEVELOPMENT)

set(_paraview_version_patch_full "${PARAVIEW_VERSION_PATCH}")
if(NOT "${PARAVIEW_VERSION_PATCH_EXTRA}" STREQUAL "")
  set(_paraview_version_patch_full 
    "${PARAVIEW_VERSION_PATCH}-${PARAVIEW_VERSION_PATCH_EXTRA}"
  )
endif(NOT "${PARAVIEW_VERSION_PATCH_EXTRA}" STREQUAL "")

#------------------------------------------------------------------------
# Install ParaView client
build_paraview_client_cpack_config_init(
    PACKAGE_NAME ${_package_name}
    ORGANIZATION "Kitware Inc."
    VERSION_MAJOR "${PARAVIEW_VERSION_MAJOR}"
    VERSION_MINOR "${PARAVIEW_VERSION_MINOR}"
    VERSION_PATCH "${_paraview_version_patch_full}"
    DESCRIPTION "ParaView is a scientific visualization tool"
    LICENSE_FILE "${ParaView_SOURCE_DIR}/License_v1.2.txt"
    DESCRIPTION_FILE "${ParaView_SOURCE_DIR}/License_v1.2.txt"
    PACKAGE_EXECUTABLES "paraview;ParaView"
)

# Set NSIS specific CPACK vars for install icons, strings etc..
if(WIN32 AND NOT UNIX)
    # There is a bug in NSI that does not handle full unix paths properly. Make
    # sure there is at least one set of four (4) backlasshes.
    set(CPACK_PACKAGE_ICON "${ParaView_SOURCE_DIR}/Applications/ParaView\\\\pvIcon.png")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\paraview.exe")
    set(CPACK_NSIS_DISPLAY_NAME "${_package_name}-${PARAVIEW_VERSION_FULL} a cross-platform, open-source visualization system")
    set(CPACK_NSIS_PACKAGE_NAME "${_package_name}-${PARAVIEW_VERSION_FULL}")
    set(CPACK_NSIS_HELP_LINK "http://www.paraview.org")
    set(CPACK_NSIS_URL_INFO_ABOUT "http://www.kitware.com")
    set(CPACK_NSIS_CONTACT "webmaster@paraview.org")
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_MUI_ICON "${ParaView_SOURCE_DIR}/Applications/ParaView\\\\WinIcon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${ParaView_SOURCE_DIR}/Applications/ParaView\\\\WinIcon.ico")
    
    get_filename_component(users_guide_file ${PARAVIEW_USERS_GUIDE} NAME)
    set(CPACK_NSIS_MENU_LINKS ${PV_INSTALL_DOC_DIR}/${users_guide_file} "ParaView User's Guide")
  
endif(WIN32 AND NOT UNIX)

# This ensure that the only component that is used by cpack when building the
# drag-n-drop on Mac is the "Bundle" component.
set (CPACK_INSTALL_CMAKE_PROJECTS_DRAGNDROP
  "${CMAKE_CURRENT_BINARY_DIR}" "ParaView Mac Bundle" "Bundle" "/")

build_paraview_client_cpack_config()
#########################################################################
