PROJECT(pqChart)

INCLUDE_DIRECTORIES(
  ${QtComponents_SOURCE_DIR}
)

SET(pqChart_SRCS
  pqAbstractPlot.cxx
  pqAbstractPlot.h
  pqChartAxis.cxx
  pqChartAxis.h
  pqChartCoordinate.cxx
  pqChartCoordinate.h
  pqChartExport.h
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
  pqHistogramChart.cxx
  pqHistogramChart.h
  pqHistogramColor.cxx
  pqHistogramColor.h
  pqHistogramSelection.cxx
  pqHistogramSelection.h
  pqHistogramWidget.cxx
  pqHistogramWidget.h
  pqLineChart.cxx
  pqLineChart.h
  pqLineChartWidget.cxx
  pqLineChartWidget.h
  pqLineErrorPlot.cxx
  pqLineErrorPlot.h
  pqLinePlot.cxx
  pqLinePlot.h
  pqMarkerPen.cxx
  pqMarkerPen.h
  pqScatterPlot.cxx
  pqScatterPlot.h
)


QT4_WRAP_CPP(pqChart_MOC_SRCS
  pqChartAxis.h
  pqChartLabel.h
  pqChartLegend.h
  pqChartZoomPan.h
  pqHistogramChart.h
  pqHistogramWidget.h
  pqLineChart.h
  pqLineChartWidget.h
)


SET(UI_RESOURCES
  pqChart.qrc
)

QT4_ADD_RESOURCES(pqChart_QRC_SRCS
  ${UI_RESOURCES}
)


SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
)

SOURCE_GROUP("Generated" FILES
  ${pqChart_MOC_SRCS}
  ${pqChart_QRC_SRCS}
)


ADD_LIBRARY(pqChart
  ${pqChart_SRCS}
  ${pqChart_MOC_SRCS}
  ${pqChart_QRC_SRCS}
)

TARGET_LINK_LIBRARIES(pqChart
  QtComponents
  ${QT_LIBRARIES}
)


# -----------------------------------------------------------------------------
# plugin
# -----------------------------------------------------------------------------

SET(BUILD_PARAQ_CHART_PLUGIN ON)

# don't build plugin on systems that require dependents to be shared (PIC actually)
IF(NOT BUILD_SHARED_LIBS)
  IF(CMAKE_SYSTEM MATCHES "HP-UX.*")
    SET(BUILD_PARAQ_CHART_PLUGIN OFF)
  ENDIF(CMAKE_SYSTEM MATCHES "HP-UX.*")
ENDIF(NOT BUILD_SHARED_LIBS)


SET(pqChartPlugin_SRCS
  pqChartPlugin.cxx
  pqChartPlugin.h
  pqHistogramWidgetPlugin.h
  pqLineChartWidgetPlugin.h
)

QT4_WRAP_CPP(pqChartPlugin_MOC_SRCS
  pqChartPlugin.h
  pqHistogramWidgetPlugin.h
  pqLineChartWidgetPlugin.h
)

ADD_DEFINITIONS(-DQT_PLUGIN)

ADD_LIBRARY(pqChartPlugin
  SHARED
  ${pqChartPlugin_SRCS}
  ${pqChartPlugin_MOC_SRCS}
)

TARGET_LINK_LIBRARIES(pqChartPlugin 
  pqChart
  ${QT_LIBRARIES}
)
