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

require_plugin(ClientGraphView)
require_plugin(ClientGraphViewFrame)
require_plugin(ClientRecordView)
require_plugin(ClientTableView)
require_plugin(ClientTreeView)
require_plugin(Infovis)
require_plugin(SQLDatabaseGraphSourcePanel)
require_plugin(SQLDatabaseTableSourcePanel)
require_plugin(TableToGraphPanel)

allow_plugin(Array)
allow_plugin(ClientGeoView)
allow_plugin(ClientGeoView2D)
allow_plugin(ClientGraphViewFrame)
allow_plugin(ClientTreeAreaView)
allow_plugin(CommonToolbar)
allow_plugin(GraphLayoutFilterPanel)
allow_plugin(SplitTableFieldPanel)
allow_plugin(StatisticsToolbar)
allow_plugin(TableToSparseArrayPanel)
allow_plugin(ThresholdTablePanel)

overview_build_client()

