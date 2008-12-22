PROJECT(QtChart)

# Any build configured headers should be added to this 
# variable so that they also get installed.
SET (dynamicHeaders)

INCLUDE_DIRECTORIES(${QT_QTDESIGNER_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(QtChart_SRCS
  pqChartArea.cxx
  pqChartArea.h
  pqChartAxis.cxx
  pqChartAxis.h
  pqChartAxisLayer.cxx
  pqChartAxisLayer.h
  pqChartAxisModel.cxx
  pqChartAxisModel.h
  pqChartAxisOptions.cxx
  pqChartAxisOptions.h
  pqChartContentsSpace.cxx
  pqChartContentsSpace.h
  pqChartCoordinate.cxx
  pqChartCoordinate.h
  pqChartGridLayer.cxx
  pqChartGridLayer.h
  pqChartInteractor.cxx
  pqChartInteractor.h
  pqChartInteractorSetup.cxx
  pqChartInteractorSetup.h
  pqChartLayer.cxx
  pqChartLayer.h
  pqChartLegend.cxx
  pqChartLegend.h
  pqChartLegendModel.cxx
  pqChartLegendModel.h
  pqChartMouseBox.cxx
  pqChartMouseBox.h
  pqChartMouseFunction.cxx
  pqChartMouseFunction.h
  pqChartMousePan.cxx
  pqChartMousePan.h
  pqChartMouseSelection.cxx
  pqChartMouseSelection.h
  pqChartMouseZoom.cxx
  pqChartMouseZoom.h
  pqChartPixelScale.cxx
  pqChartPixelScale.h
  pqChartPlugin.cxx
  pqChartPlugin.h
  pqChartSeriesColorManager.cxx
  pqChartSeriesColorManager.h
  pqChartSeriesOptionsGenerator.cxx
  pqChartSeriesOptionsGenerator.h
  pqChartTitle.cxx
  pqChartTitle.h
  pqChartValue.cxx
  pqChartValue.h
  pqChartWidget.cxx
  pqChartWidget.h
  pqChartWidgetPlugin.h
  pqChartZoomHistory.cxx
  pqChartZoomHistory.h
  pqColorMapColorChanger.cxx
  pqColorMapColorChanger.h
  pqColorMapModel.cxx
  pqColorMapModel.h
  pqColorMapWidget.cxx
  pqColorMapWidget.h
  pqColorMapWidgetPlugin.h
  pqHistogramChart.cxx
  pqHistogramChart.h
  pqHistogramChartOptions.cxx
  pqHistogramChartOptions.h
  pqHistogramColor.cxx
  pqHistogramColor.h
  pqHistogramModel.cxx
  pqHistogramModel.h
  pqHistogramSelection.cxx
  pqHistogramSelection.h
  pqHistogramSelectionModel.cxx
  pqHistogramSelectionModel.h
  pqHistogramWidget.cxx
  pqHistogramWidget.h
  pqLineChart.cxx
  pqLineChart.h
  pqLineChartModel.cxx
  pqLineChartModel.h
  pqLineChartOptions.cxx
  pqLineChartOptions.h
  pqLineChartSeries.cxx
  pqLineChartSeries.h
  pqLineChartSeriesOptions.cxx
  pqLineChartSeriesOptions.h
  pqLineChartWidget.cxx
  pqLineChartWidget.h
  pqPointMarker.cxx
  pqPointMarker.h
  pqSimpleHistogramModel.cxx
  pqSimpleHistogramModel.h
  pqSimpleLineChartSeries.cxx
  pqSimpleLineChartSeries.h
  QtChartExport.h
)

QT4_WRAP_CPP(QtChart_MOC_SRCS
  pqChartArea.h
  pqChartAxis.h
  pqChartAxisModel.h
  pqChartAxisOptions.h
  pqChartContentsSpace.h
  pqChartInteractor.h
  pqChartLegend.h
  pqChartLegendModel.h
  pqChartLayer.h
  pqChartMouseFunction.h
  pqChartMouseSelection.h
  pqChartPlugin.h
  pqChartTitle.h
  pqChartWidget.h
  pqChartWidgetPlugin.h
  pqColorMapColorChanger.h
  pqColorMapModel.h
  pqColorMapWidget.h
  pqColorMapWidgetPlugin.h
  pqHistogramChart.h
  pqHistogramChartOptions.h
  pqHistogramModel.h
  pqHistogramSelectionModel.h
  pqLineChart.h
  pqLineChartModel.h
  pqLineChartOptions.h
  pqLineChartSeries.h
  pqLineChartSeriesOptions.h
  pqSimpleHistogramModel.h
)

SET(UI_RESOURCES
  pqChart.qrc
)

QT4_ADD_RESOURCES(QtChart_QRC_SRCS
  ${UI_RESOURCES}
)

SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
)

SOURCE_GROUP("Generated" FILES
  ${QtChart_MOC_SRCS}
  ${QtChart_QRC_SRCS}
)

ADD_LIBRARY(QtChart
  ${QtChart_SRCS}
  ${QtChart_MOC_SRCS}
  ${QtChart_QRC_SRCS}
)

TARGET_LINK_LIBRARIES(QtChart
  ${QT_LIBRARIES}
)

SET(QTCHART_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/QtChartExport.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/QtChartExport.h")
SET(dynamicHeaders 
    "${dynamicHeaders};${CMAKE_CURRENT_BINARY_DIR}/QtChartExport.h")

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)

INSTALL(TARGETS QtChart
  RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
  LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
  ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)

IF (NOT PV_INSTALL_NO_DEVELOPMENT)
  # Headers
  GLOB_INSTALL_DEVELOPMENT(
      "${CMAKE_CURRENT_SOURCE_DIR}"
      "${PV_INSTALL_INCLUDE_DIR}"
      "*.h;*.hxx;*.txx")
  INSTALL(
      FILES ${dynamicHeaders}
      DESTINATION "${PV_INSTALL_INCLUDE_DIR}"
      COMPONENT Development)
  # QT resources
  GLOB_INSTALL_DEVELOPMENT( 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PV_INSTALL_INCLUDE_DIR}/Qt/Chart/Resources"
    "*.ui;*.qrc;*.ico;*.png;*.svg;*.rc;*.xpm")
ENDIF (NOT PV_INSTALL_NO_DEVELOPMENT)
