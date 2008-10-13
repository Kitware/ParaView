GET_FILENAME_COMPONENT(CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)

SET(OVERVIEW_BRANDED_APPLICATION_TITLE "OverView") 
SET(OVERVIEW_BRANDED_SPLASH_IMAGE "${CURRENT_LIST_DIR}/SplashScreen.png")
SET(OVERVIEW_BRANDED_SPLASH_TEXT_COLOR "black")
SET(OVERVIEW_BRANDED_VERSION_MAJOR ${OVERVIEW_VERSION_MAJOR})
SET(OVERVIEW_BRANDED_VERSION_MINOR ${OVERVIEW_VERSION_MINOR})
SET(OVERVIEW_BRANDED_VERSION_PATCH ${OVERVIEW_VERSION_PATCH})
SET(OVERVIEW_BRANDED_VERSION_TYPE ${OVERVIEW_VERSION_TYPE})

IF(APPLE)
  SET(OVERVIEW_BRANDED_BUNDLE_ICON "${CURRENT_LIST_DIR}/overview.icns") 
#  SET(OVERVIEW_BRANDED_PACKAGE_ICON "${CURRENT_LIST_DIR}/volume.png") 
ENDIF(APPLE)

IF(WIN32)
  SET(OVERVIEW_BRANDED_APPLICATION_ICON "${CURRENT_LIST_DIR}/Icon.ico")
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
allow_plugin(ClientHierarchyView)
allow_plugin(ClientRecordView)
allow_plugin(SplitTableFieldPanel)
allow_plugin(StatisticsToolbar)
allow_plugin(TableToSparseArrayPanel)



