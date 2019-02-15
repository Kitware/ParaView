#/******************************************************************************
# * Copyright 2018 NVIDIA Corporation. All rights reserved.
# *****************************************************************************/

vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/src/vtk.module"
  REQUEST_MODULES   pvNVIDIAIndeX::IndeXRepresentations
  PROVIDES_MODULES  index_modules
  REQUIRES_MODULES  required_modules
  HIDE_MODULES_FROM_CACHE ON)

if (required_modules)
  foreach (required_module IN LISTS required_modules)
    if (NOT TARGET "${required_module}")
      message(FATAL_ERROR
        "Failed to find the required module ${required_module}.")
    endif ()
  endforeach ()
endif ()

vtk_module_build(
  MODULES             ${index_modules}
  PACKAGE             pvNVIDIAIndeX
  INSTALL_HEADERS     OFF
  LIBRARY_NAME_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1")

paraview_add_plugin(pvNVIDIAIndeX
  VERSION "2.2.1"
  EULA "EULA.txt"
  MODULES pvNVIDIAIndeX::IndeXRepresentations
  XML_DOCUMENTATION OFF)

if (UNIX)
  set_target_properties(pvNVIDIAIndeX PROPERTIES LINK_FLAGS "-Wl,--as-needed")
endif ()

if (NVINDEX_SKIP_BUILD_RPATH)
  set_target_properties(pvNVIDIAIndeX
    PROPERTIES
      SKIP_BUILD_RPATH TRUE
      INSTALL_RPATH_USE_LINK_PATH FALSE)
endif ()

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
