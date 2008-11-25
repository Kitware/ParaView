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

paraview_require_plugin(ClientAttributeView)
paraview_require_plugin(ClientGraphView)
paraview_require_plugin(ClientRecordView)
paraview_require_plugin(ClientTableView)
paraview_require_plugin(Infovis)
paraview_require_plugin(SQLDatabaseGraphSourcePanel)
paraview_require_plugin(SQLDatabaseTableSourcePanel)
paraview_require_plugin(TableToGraphPanel)

paraview_allow_plugin(Array)
paraview_allow_plugin(ChartViewFrame)
paraview_allow_plugin(ClientChartView)
paraview_allow_plugin(ClientGeoView)
paraview_allow_plugin(ClientGeoView2D)
paraview_allow_plugin(ClientHierarchyView)
paraview_allow_plugin(CommonToolbar)
paraview_allow_plugin(GraphLayoutFilterPanel)
paraview_allow_plugin(SplitTableFieldPanel)
paraview_allow_plugin(StatisticsToolbar)
paraview_allow_plugin(TableToSparseArrayPanel)
paraview_allow_plugin(ThresholdTablePanel)

overview_build_client()

