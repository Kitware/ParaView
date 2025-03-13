set(PARAVIEW_ENABLE_EXAMPLES ON CACHE BOOL "")

set(PARAVIEW_ENABLE_ADIOS2 ON CACHE BOOL "")
set(PARAVIEW_ENABLE_FFMPEG ON CACHE BOOL "")
set(PARAVIEW_ENABLE_FIDES ON CACHE BOOL "")
# set(PARAVIEW_ENABLE_GDAL ON CACHE BOOL "")
set(PARAVIEW_ENABLE_OPENTURNS ON CACHE BOOL "")
# set(PARAVIEW_ENABLE_PDAL ON CACHE BOOL "")
set(PARAVIEW_ENABLE_VISITBRIDGE ON CACHE BOOL "")
set(PARAVIEW_ENABLE_XDMF3 ON CACHE BOOL "")

set(PARAVIEW_PLUGINS_DEFAULT ON CACHE BOOL "")

set(PARAVIEW_PLUGIN_ENABLE_pvNVIDIAIndeX ON CACHE BOOL "")

set(PARAVIEW_PLUGIN_ENABLE_CDIReader ON CACHE BOOL "")

set(PARAVIEW_PLUGIN_ENABLE_PythonQtPlugin ON CACHE BOOL "")

set(PARAVIEW_BUILD_TRANSLATIONS ON CACHE BOOL "")
set(PARAVIEW_TRANSLATIONS_DIRECTORY "$ENV{CI_PROJECT_DIR}/translations" CACHE BOOL "")

set(PARAVIEW_SSH_SERVERS_TESTING ON CACHE BOOL "")

# External package settings.
# set(PARAVIEW_BUILD_WITH_EXTERNAL ON CACHE BOOL "")
# set(VTK_MODULE_USE_EXTERNAL_ParaView_vtkcatalyst OFF CACHE BOOL "")
# set(VTK_MODULE_USE_EXTERNAL_VTK_libharu OFF CACHE BOOL "")
# set(VTK_MODULE_USE_EXTERNAL_VTK_cgns OFF CACHE BOOL "")
# set(VTK_MODULE_USE_EXTERNAL_VTK_gl2ps OFF CACHE BOOL "")

set(PARAVIEW_XRInterface_OpenVR_Support ON CACHE BOOL "")
set(VTK_MODULE_ENABLE_VTK_RenderingOpenVR YES CACHE STRING "")

# https://gitlab.kitware.com/paraview/paraview/-/issues/22909
set(PARAVIEW_XRInterface_OpenXR_Support OFF CACHE BOOL "")
set(VTK_MODULE_ENABLE_VTK_RenderingOpenXR NO CACHE STRING "")

# enable optimizations for debug build because new imaging testing framework is slow otherwise
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora35.cmake")
