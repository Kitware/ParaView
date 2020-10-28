#/******************************************************************************
# * Copyright 2020 NVIDIA Corporation. All rights reserved.
# *****************************************************************************/

paraview_add_plugin(pvNVIDIAIndeX
  VERSION "5.9"
  EULA "EULA.txt"
  MODULES pvNVIDIAIndeX::IndeXRepresentations
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/vtk.module"
  XML_DOCUMENTATION OFF)

# install kernels
install(
  DIRECTORY   "kernel_programs/"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/paraview${paraview_version_suffix}/kernels_nvidia_index"
  COMPONENT   runtime)

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
