set(classes
  vtknvindex_irregular_volume_mapper
  vtknvindex_irregular_volume_representation
  vtknvindex_performance_values
  vtknvindex_representation
  vtknvindex_volumemapper)

set(private_classes
  vtknvindex_affinity
  vtknvindex_clock_pulse_generator
  vtknvindex_cluster_properties
  vtknvindex_colormap_utility
  vtknvindex_config_settings
  vtknvindex_forwarding_logger
  vtknvindex_host_properties
  vtknvindex_instance
  vtknvindex_irregular_volume_importer
  vtknvindex_opengl_app_buffer
  vtknvindex_opengl_canvas
  vtknvindex_receiving_logger
  vtknvindex_regular_volume_properties
  # This duplicates the generated kernel sources below. Should this file be
  # removed?
  #vtknvindex_rtc_kernel_params
  vtknvindex_scene
  vtknvindex_sparse_volume_importer
  vtknvindex_volume_compute)

set(sources)
set(private_headers
  vtknvindex_rtc_kernel_params.h
  vtknvindex_utilities.h)
set(kernel_sources
  kernel_irregular_depth_enhancement.cu
  kernel_irregular_edge_enhancement.cu
  kernel_irregular_isosurface.cu
  kernel_depth_enhancement.cu
  kernel_edge_enhancement.cu
  kernel_gradient.cu
  kernel_isosurface.cu
  kernel_plane_surface_mapping.cu)
set(kernel_string_sources)
foreach (kernel_source IN LISTS kernel_sources)
  get_filename_component(kernel_name "${kernel_source}" NAME_WE)
  string(TOUPPER "${kernel_name}" variable_name)
  vtk_encode_string(
    INPUT "${kernel_source}"
    NAME  "${variable_name}_STRING"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND private_headers
    "${header}")
  list(APPEND sources
    "${source}")
endforeach ()

# Set include and library paths for NVIDIA IndeX
set(NVINDEX_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../include"
  CACHE PATH "Semi-colon separated paths to NVIDIA IndeX headers")
mark_as_advanced(NVINDEX_INCLUDE_DIRS)

if (NOT NVINDEX_INCLUDE_DIRS)
  message(FATAL_ERROR
    "NVIDIA IndeX include path is required to build the plugin")
endif ()

vtk_module_add_module(pvNVIDIAIndeX::IndeXRepresentations
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers})
vtk_module_include(pvNVIDIAIndeX::IndeXRepresentations
  PUBLIC
    $<BUILD_INTERFACE:${NVINDEX_INCLUDE_DIRS}>)

if (NVINDEX_SKIP_BUILD_RPATH)
  vtk_module_set_properties(pvNVIDIAIndeX::IndeXRepresentations
    SKIP_BUILD_RPATH TRUE
    INSTALL_RPATH_USE_LINK_PATH FALSE)
endif ()

vtk_module_definitions(pvNVIDIAIndeX::IndeXRepresentations
  PRIVATE
    USE_SPARSE_VOLUME)

if (NVINDEX_INTERNAL_BUILD)
  vtk_module_definitions(pvNVIDIAIndeX::IndeXRepresentations
    PRIVATE
      NVINDEX_INTERNAL_BUILD)
endif ()

paraview_add_server_manager_xmls(
  XMLS  nvindex.xml)
