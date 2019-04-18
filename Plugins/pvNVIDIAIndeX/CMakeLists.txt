#/******************************************************************************
# * Copyright 2018 NVIDIA Corporation. All rights reserved.
# *****************************************************************************/

paraview_add_plugin(pvNVIDIAIndeX
  VERSION "2.3-beta"
  EULA "EULA.txt"
  MODULES pvNVIDIAIndeX::IndeXRepresentations
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/vtk.module"
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
