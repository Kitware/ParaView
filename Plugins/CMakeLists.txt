INCLUDE(${ParaView_CMAKE_DIR}/ParaViewPlugins.cmake)

# We setup the standard include directories that would be needed by all plugins
# anyways.
INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${PARAVIEW_GUI_INCLUDE_DIRS})
IF (PARAVIEW_BUILD_QT_GUI)
  INCLUDE(${QT_USE_FILE})
  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIR}
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS})
ENDIF()

MACRO(paraview_build_optional_plugin name comment subdirectory default)
  OPTION(PARAVIEW_BUILD_PLUGIN_${name} "Build ${comment}" ${default})
  MARK_AS_ADVANCED(PARAVIEW_BUILD_PLUGIN_${name})
  IF(PARAVIEW_BUILD_PLUGIN_${name})
    MESSAGE(STATUS "Plugin: ${comment} enabled")
    get_filename_component(builddirectory ${subdirectory} NAME_WE)
    ADD_SUBDIRECTORY("${subdirectory}" "${CMAKE_CURRENT_BINARY_DIR}/${builddirectory}")
  ELSE(PARAVIEW_BUILD_PLUGIN_${name})
    MESSAGE(STATUS "Plugin: ${comment} disabled")
  ENDIF(PARAVIEW_BUILD_PLUGIN_${name})
ENDMACRO(paraview_build_optional_plugin)

paraview_build_optional_plugin(EyeDomeLighting "EyeDomeLighting" EyeDomeLighting OFF)
paraview_build_optional_plugin(Manta "Manta ray traced rendering" Manta OFF)
paraview_build_optional_plugin(Moments "Filters for Flux and Circulation Fields" Moments ON)
paraview_build_optional_plugin(Prism "Prism" PrismPlugins ON)
paraview_build_optional_plugin(PacMan "Testing plugin" TestPlugin  ON)

# The PointSprite plugin does not build in static mode therefore we make sure it is disable
if(BUILD_SHARED_LIBS)
  paraview_build_optional_plugin(PointSprite "Point Sprites" PointSprite ON)
else(BUILD_SHARED_LIBS)
  set(PARAVIEW_BUILD_PLUGIN_PointSprite OFF CACHE BOOL "Must be disable in static build" FORCE)
endif(BUILD_SHARED_LIBS)

IF(PARAVIEW_ENABLE_PYTHON)
  paraview_build_optional_plugin(pvblot "pvblot" pvblot ON)
ENDIF(PARAVIEW_ENABLE_PYTHON)
paraview_build_optional_plugin(SierraPlotTools "SierraPlotTools" SierraPlotTools ON)
paraview_build_optional_plugin(SLACTools "SLACTools" SLACTools ON)
paraview_build_optional_plugin(StreamingView "Streaming View" StreamingView ON)
paraview_build_optional_plugin( SurfaceLIC "SurfaceLIC" SurfaceLIC ON )
paraview_build_optional_plugin(VaporPlugin "Plugin to read NCAR VDF files" Vapor OFF)
paraview_build_optional_plugin(H5PartReader "Reader for *.h5part files" H5PartReader ON)
paraview_build_optional_plugin(AdiosReader "Reader for *.bp files based on Adios" AdiosReader OFF)
IF(PARAVIEW_BUILD_QT_GUI)
  IF(PARAVIEW_ENABLE_PYTHON)
    paraview_build_optional_plugin(CoProcessingScriptGenerator
      "Plugin for creating python coprocessing scripts" CoProcessingScriptGenerator ON)
  ENDIF(PARAVIEW_ENABLE_PYTHON)
  paraview_build_optional_plugin(VRPlugin "Virtual Reality Devices and Interactor styles" VR OFF)

  # We do not include VisTrails source in our repo for Licensing reasons. If the directory is manually copied into Plugins, build it.
  IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/VisTrails/CMakeLists.txt)
    paraview_build_optional_plugin(VisTrailsPlugin "VisTrailsPlugin" VisTrails ON)
  ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/VisTrails/CMakeLists.txt)
ENDIF(PARAVIEW_BUILD_QT_GUI)
paraview_build_optional_plugin(AnalyzeNIfTIReaderWriter "Reader/Writer for Analyze and NIfTI." AnalyzeNIfTIReaderWriter ON)
paraview_build_optional_plugin(ForceTime "Override time requests" ForceTime OFF)
paraview_build_optional_plugin(SQToolkit "SciberQuest SciViz Toolkit" SQToolkit OFF)
if(PARAVIEW_USE_MPI AND NOT WIN32)
  paraview_build_optional_plugin(Nektar "Nektar Reader Plugin" Nektar OFF)
endif()

#------------------------------------------------------------------------
# Build any plugins located outside of ParaView. This mechanism makes it
# possible to add a directory containing additions plugins to ParaView.
SET (PARAVIEW_EXTERNAL_PLUGINS_FILE "" CACHE FILEPATH
  "Path to a cmake file containing calls to build additional plugins external to ParaView.")

MARK_AS_ADVANCED(PARAVIEW_EXTERNAL_PLUGINS_FILE)

IF (EXISTS "${PARAVIEW_EXTERNAL_PLUGINS_FILE}")
  INCLUDE("${PARAVIEW_EXTERNAL_PLUGINS_FILE}")
ENDIF (EXISTS "${PARAVIEW_EXTERNAL_PLUGINS_FILE}")

#------------------------------------------------------------------------
# Added additional external plugins to ParaView's build process.
# Note that all external plugins added in such a way are enabled by default.
SET (PARAVIEW_EXTERNAL_PLUGIN_DIRS "" CACHE STRING
  "';' separated paths to additional plugin directories to be included in build process.")
MARK_AS_ADVANCED(PARAVIEW_EXTERNAL_PLUGIN_DIRS)
  
FOREACH (external_plugin_dir ${PARAVIEW_EXTERNAL_PLUGIN_DIRS})
  set (plugin_name)
  get_filename_component(plugin_name ${external_plugin_dir} NAME)
  paraview_build_optional_plugin(${plugin_name}
    "${plugin_name} (External Plugin)" ${external_plugin_dir} ON)
ENDFOREACH () 

write_plugins_file()
write_static_plugins_init_file()
