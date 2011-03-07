INCLUDE(${ParaView_CMAKE_DIR}/ParaViewPlugins.cmake)

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
paraview_build_optional_plugin(NetDMFReader "NetDMF file format reader" NetDMFReader OFF)
paraview_build_optional_plugin(Prism "Prism" PrismPlugins ON)
paraview_build_optional_plugin(PointSprite "Point Sprites" PointSprite ON)
IF(PARAVIEW_ENABLE_PYTHON)
  paraview_build_optional_plugin(pvblot "pvblot" pvblot ON)
ENDIF(PARAVIEW_ENABLE_PYTHON)
paraview_build_optional_plugin(SierraPlotTools "SierraPlotTools" SierraPlotTools ON)
paraview_build_optional_plugin(SLACTools "SLACTools" SLACTools ON)
paraview_build_optional_plugin(StreamingView "Streaming View" StreamingView ON)
paraview_build_optional_plugin( SurfaceLIC "SurfaceLIC" SurfaceLIC ON )
paraview_build_optional_plugin(VaporPlugin "Plugin to read NCAR VDF files" Vapor OFF)
paraview_build_optional_plugin(VRPNPlugin "Virtual Reality Private Network (VRPN) tracker" VRPN OFF)
paraview_build_optional_plugin(VRUIPlugin "Virtual Reality User Interface (VRUI) tracker" VRUI OFF)
paraview_build_optional_plugin(H5PartReader "Reader for *.h5part files" H5PartReader ON)
paraview_build_optional_plugin(AdiosReader "Reader for *.bp files based on Adios" AdiosReader OFF)
IF(PARAVIEW_BUILD_QT_GUI)
  paraview_build_optional_plugin(CoProcessingScriptGenerator "Plugin for creating python coprocessing scripts" CoProcessingScriptGenerator OFF)
  
  # We do not include VisTrails source in our repo for Licensing reasons. If the directory is manually copied into Plugins, build it.
  IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/VisTrails/CMakeLists.txt)
    paraview_build_optional_plugin(VisTrailsPlugin "VisTrailsPlugin" VisTrails ON)
  ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/VisTrails/CMakeLists.txt)
ENDIF(PARAVIEW_BUILD_QT_GUI)
paraview_build_optional_plugin(AnalyzeNIfTIReaderWriter "Reader/Writer for Analyze and NIfTI." AnalyzeNIfTIReaderWriter ON)
paraview_build_optional_plugin(ForceTime "Override time requests" ForceTime OFF)

#------------------------------------------------------------------------
# Build any plugins located outside of ParaView.
SET (PARAVIEW_EXTERNAL_PLUGINS_FILE "" CACHE FILEPATH
  "Path to a cmake file containing calls to build additional plugins external to ParaView.")

MARK_AS_ADVANCED(PARAVIEW_EXTERNAL_PLUGINS_FILE)

IF (EXISTS "${PARAVIEW_EXTERNAL_PLUGINS_FILE}")
  INCLUDE("${PARAVIEW_EXTERNAL_PLUGINS_FILE}")
ENDIF (EXISTS "${PARAVIEW_EXTERNAL_PLUGINS_FILE}")
