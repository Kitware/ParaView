# This section builds the OverView core library and is specific to the ParaView/OverView build process ...
PROJECT(OverView)
ADD_SUBDIRECTORY(Core)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/OverViewConfig.cmake.in" "${ParaView_BINARY_DIR}/OverViewConfig.cmake" @ONLY)

# This section builds an "OverView-branded" OverView client, and is an example of how to create a custom
# OverView branding ...
INCLUDE("${ParaView_BINARY_DIR}/OverViewConfig.cmake")

SET(OVERVIEW_BRANDED_APPLICATION_TITLE "OverView") 
SET(OVERVIEW_BRANDED_SPLASH_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/SplashScreen.png")
SET(OVERVIEW_BRANDED_SPLASH_TEXT_COLOR "black")
SET(OVERVIEW_BRANDED_VERSION_MAJOR "0")
SET(OVERVIEW_BRANDED_VERSION_MINOR "5")
SET(OVERVIEW_BRANDED_VERSION_PATCH "0")
SET(OVERVIEW_BRANDED_VERSION_TYPE "alpha")

IF(APPLE)
  SET(OVERVIEW_BRANDED_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/overview.icns") 
#  SET(OVERVIEW_BRANDED_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/volume.png") 
ENDIF(APPLE)

IF(WIN32)
  SET(OVERVIEW_BRANDED_APPLICATION_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Icon.ico")
ENDIF(WIN32)

require_plugin(ClientGraphView)
require_plugin(ClientTableView)
require_plugin(Infovis)
require_plugin(TableToGraphPanel)

allow_plugin(Array)
allow_plugin(ChartViewFrame)
allow_plugin(ClientAttributeView)
allow_plugin(ClientChartView)
allow_plugin(ClientGeoView)
allow_plugin(ClientGeoView2D)
allow_plugin(ClientHierarchyView)
allow_plugin(ClientRecordView)
allow_plugin(CommonToolbar)
allow_plugin(GraphLayoutFilterPanel)
allow_plugin(SQLDatabaseGraphSourcePanel)
allow_plugin(SQLDatabaseTableSourcePanel)
allow_plugin(SplitTableFieldPanel)
allow_plugin(StatisticsToolbar)
allow_plugin(TableToSparseArrayPanel)
allow_plugin(ThresholdTablePanel)

build_overview()

