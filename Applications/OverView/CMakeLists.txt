# This section builds the OverView core library and is specific to the ParaView/OverView build process ...
ADD_SUBDIRECTORY(Core)

# This section builds an "OverView-branded" OverView client, and is an example of how to create a custom
# OverView branding ...
PROJECT(OverView)
INCLUDE("${ParaView_BINARY_DIR}/OverViewConfig.cmake")

SET(OVERVIEW_BRANDED_APPLICATION_TITLE "OverView") 
SET(OVERVIEW_BRANDED_SPLASH_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/SplashScreen.png")
SET(OVERVIEW_BRANDED_SPLASH_TEXT_COLOR "black")
SET(OVERVIEW_BRANDED_VERSION_MAJOR "0")
SET(OVERVIEW_BRANDED_VERSION_MINOR "7")
SET(OVERVIEW_BRANDED_VERSION_PATCH "0")
SET(OVERVIEW_BRANDED_VERSION_TYPE "alpha")

IF(APPLE)
  SET(OVERVIEW_BRANDED_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/overview.icns") 
#  SET(OVERVIEW_BRANDED_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/volume.png") 
ENDIF(APPLE)

IF(WIN32)
  SET(OVERVIEW_BRANDED_APPLICATION_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Icon.ico")
ENDIF(WIN32)

#require_paraview_plugin(ClientAttributeView)
require_paraview_plugin(ClientGraphView)
require_paraview_plugin(ClientGraphViewFrame)
require_paraview_plugin(ClientRecordView)
require_paraview_plugin(ClientTableView)
require_paraview_plugin(Infovis)
require_paraview_plugin(SQLDatabaseGraphSourcePanel)
require_paraview_plugin(SQLDatabaseTableSourcePanel)
require_paraview_plugin(TableToGraphPanel)

allow_paraview_plugin(Array)
#allow_paraview_plugin(ChartViewFrame)
#allow_paraview_plugin(ClientChartView)
allow_paraview_plugin(ClientGeoView)
allow_paraview_plugin(ClientGeoView2D)
allow_paraview_plugin(ClientGraphViewFrame)
allow_paraview_plugin(ClientHierarchyView)
allow_paraview_plugin(CommonToolbar)
allow_paraview_plugin(GraphLayoutFilterPanel)
allow_paraview_plugin(SplitTableFieldPanel)
allow_paraview_plugin(StatisticsToolbar)
allow_paraview_plugin(TableToSparseArrayPanel)
allow_paraview_plugin(ThresholdTablePanel)

overview_build_client()

