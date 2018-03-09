#/******************************************************************************
# * Copyright 2018 NVIDIA Corporation. All rights reserved.
# *****************************************************************************/

# Set include and library paths for NVIDIA IndeX
SET(NVINDEX_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include/" CACHE FILEPATH "Semi-colon separated paths to NVIDIA IndeX headers")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src/)

IF(NOT NVINDEX_INCLUDE_DIRS)
    MESSAGE(SEND_ERROR "NVIDIA IndeX include path is required to build the plugin")
    RETURN()
ENDIF(NOT NVINDEX_INCLUDE_DIRS)

# Find and use ParaView
IF (ParaView_SOURCE_DIR)
    INCLUDE_DIRECTORIES( ${PARAVIEW_INCLUDE_DIRS} ${VTK_INCLUDE_DIRS} ${NVINDEX_INCLUDE_DIRS})
ELSE()
    FIND_PACKAGE(ParaView REQUIRED)
    INCLUDE(${PARAVIEW_USE_FILE})
ENDIF()

pv_process_modules()

set(kernel_sources
  src/kernel_depth_enhancement.cu
  src/kernel_edge_enhancement.cu
  src/kernel_isoraycast.cu
  src/kernel_isosurface.cu
  src/kernel_single_scattering.cu)
set(kernel_string_sources)
foreach (kernel_source IN LISTS kernel_sources)
  get_filename_component(kernel_name "${kernel_source}" NAME_WE)
  string(TOUPPER "${kernel_name}" variable_name)
  vtk_encode_string(
    INPUT "${kernel_source}"
    NAME  "${variable_name}_STRING"
    SOURCE_OUTPUT source)
  list(APPEND kernel_string_sources
    "${source}")
endforeach ()

ADD_PARAVIEW_PLUGIN(pvNVIDIAIndeX "2.1"
    EULA "EULA.txt"
    SERVER_MANAGER_XML src/nvindex.xml
    SERVER_MANAGER_SOURCES
        src/vtknvindex_irregular_volume_mapper.cxx
        src/vtknvindex_irregular_volume_representation.cxx
        src/vtknvindex_representation.cxx
        src/vtknvindex_volumemapper.cxx
    SOURCES
        src/vtknvindex_affinity.cxx
        src/vtknvindex_application.cxx
        src/vtknvindex_clock_pulse_generator.cxx
        src/vtknvindex_cluster_properties.cxx
        src/vtknvindex_colormap_utility.cxx
        src/vtknvindex_config_settings.cxx
        src/vtknvindex_forwarding_logger.cxx
        src/vtknvindex_host_properties.cxx
        src/vtknvindex_irregular_volume_importer.cxx
        src/vtknvindex_opengl_app_buffer.cxx
        src/vtknvindex_opengl_canvas.cxx
        src/vtknvindex_performance_values.cxx
        src/vtknvindex_receiving_logger.cxx
        src/vtknvindex_regular_volume_properties.cxx
        src/vtknvindex_scene.cxx
        src/vtknvindex_volume_importer.cxx
        ${kernel_string_sources}
)

ExternalData_Expand_Arguments(pvNVIDIAIndeX_UserGuide user_guide "DATA{nvidia_index_paraview_plugin.pdf}")
ExternalData_Add_Target(pvNVIDIAIndeX_UserGuide)
add_dependencies(pvNVIDIAIndeX pvNVIDIAIndeX_UserGuide)

install(
  FILES         ${user_guide}
  DESTINATION   "${VTK_INSTALL_DOC_DIR}/pvNVIDIAIndeX"
  COMPONENT     Documentation)

include(vtkOpenGL)
vtk_opengl_link(pvNVIDIAIndeX)

IF(UNIX)
SET_TARGET_PROPERTIES(pvNVIDIAIndeX PROPERTIES LINK_FLAGS "-Wl,--as-needed")
ENDIF()

IF(NVINDEX_SKIP_BUILD_RPATH)
    SET_TARGET_PROPERTIES(pvNVIDIAIndeX PROPERTIES
        SKIP_BUILD_RPATH TRUE
        INSTALL_RPATH_USE_LINK_PATH FALSE)
ENDIF(NVINDEX_SKIP_BUILD_RPATH)

IF(NVINDEX_INTERNAL_BUILD)
ADD_DEFINITIONS(-DNVINDEX_INTERNAL_BUILD)
ENDIF(NVINDEX_INTERNAL_BUILD)
