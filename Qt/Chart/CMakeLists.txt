PROJECT(QtChart)

# when you remove this, remove the moc workaround for include directories
INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
)

INCLUDE_DIRECTORIES(${QT_QTDESIGNER_INCLUDE_DIR})

IF(NOT BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DQT_STATICPLUGIN)
ENDIF(NOT BUILD_SHARED_LIBS)

SET(QtChart_SRCS
  pqChartPlugin.cxx
  pqChartPlugin.h
  pqColorMapWidgetPlugin.h
  pqHistogramWidgetPlugin.h
  pqLineChartWidgetPlugin.h


  pqChartAxis.cxx
  pqChartAxis.h
  pqChartCoordinate.cxx
  pqChartCoordinate.h
  pqChartLabel.cxx
  pqChartLabel.h
  pqChartLegend.cxx
  pqChartLegend.h
  pqChartMouseBox.cxx
  pqChartMouseBox.h
  pqChartValue.cxx
  pqChartValue.h
  pqChartZoomHistory.cxx
  pqChartZoomHistory.h
  pqChartZoomPan.cxx
  pqChartZoomPan.h
  pqChartZoomPanAlt.cxx
  pqChartZoomPanAlt.h
  pqColorMapColorChanger.cxx
  pqColorMapColorChanger.h
  pqColorMapWidget.cxx
  pqColorMapWidget.h
  pqHistogramChart.cxx
  pqHistogramChart.h
  pqHistogramColor.cxx
  pqHistogramColor.h
  pqHistogramListModel.cxx
  pqHistogramListModel.h
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
  pqLineChartPlot.cxx
  pqLineChartPlot.h
  pqLineChartPlotOptions.cxx
  pqLineChartPlotOptions.h
  pqLineChartWidget.cxx
  pqLineChartWidget.h
  pqMarkerPen.cxx
  pqMarkerPen.h
  pqPixelTransferFunction.cxx
  pqPixelTransferFunction.h
  pqPointMarker.cxx
  pqPointMarker.h
  pqSimpleLineChartPlot.cxx
  pqSimpleLineChartPlot.h
  QtChartExport.h
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
SET(MOC_INCLUDE_DIRS ${MOC_INCLUDE_DIRS} ${QT_QTDESIGNER_INCLUDE_DIR})
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(QtChart_MOC_SRCS
  pqChartPlugin.h
  pqColorMapWidgetPlugin.h
  pqHistogramWidgetPlugin.h
  pqLineChartWidgetPlugin.h


  pqChartAxis.h
  pqChartLabel.h
  pqChartLegend.h
  pqChartZoomPan.h
  pqChartZoomPanAlt.h
  pqColorMapColorChanger.h
  pqColorMapWidget.h
  pqHistogramChart.h
  pqHistogramListModel.h
  pqHistogramModel.h
  pqHistogramSelectionModel.h
  pqHistogramWidget.h
  pqLineChart.h
  pqLineChartModel.h
  pqLineChartPlot.h
  pqLineChartPlotOptions.h
  pqLineChartWidget.h
)

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

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


ADD_EXECUTABLE(ChartTest
  ChartTest.cxx
)

TARGET_LINK_LIBRARIES(ChartTest
  QtChart
  ${QT_LIBRARIES}
)

INSTALL(TARGETS QtChart
  RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
  LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
  ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)


