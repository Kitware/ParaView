INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(MOC_SRCS 
  ClientBarChartView.h
  ChartSetupDialog.h
  ClientChartDisplay.h
  ClientChartView.h
  ClientChartViewActiveOptions.h
  ClientChartViewOptions.h
  ClientLineChartView.h
  ClientStackedChartView.h
  ClientStatisticalBoxChartView.h
  )

SET(UI_FORMS
  ChartSetupDialog.ui
  ClientChartDisplay.ui
  ClientChartView.ui
  ClientChartViewOptions.ui
  )

QT4_WRAP_UI(
  UI_BUILT_SOURCES
  ${UI_FORMS}
  )

SET(SOURCES 
  ClientBarChartView.cxx 
  ChartSetupDialog.cxx
  ClientChartDisplay.cxx 
  ClientChartView.cxx 
  ClientChartViewActiveOptions.cxx 
  ClientChartViewOptions.cxx 
  ClientLineChartView.cxx 
  ClientStackedChartView.cxx 
  ClientStatisticalBoxChartView.cxx 
  )

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${OverViewCore_BINARY_DIR}
  ${OverViewCore_SOURCE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${ParaView_BINARY_DIR}/VTK/GUISupport/Qt
  ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
  ${QtWidgets_BINARY_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${VTK_INCLUDE_DIR}
  ${pqComponents_BINARY_DIR}
  ${pqComponents_SOURCE_DIR}
  ${pqCore_BINARY_DIR}
  ${pqCore_SOURCE_DIR}
  ${vtkQtChart_BINARY_DIR}
  ${vtkQtChart_SOURCE_DIR}
  )

ADD_PARAVIEW_VIEW_MODULE(
  STACKED_IFACES STACKED_IFACE_SRCS
  VIEW_TYPE ClientStackedChartView
  VIEW_NAME "Stacked Chart"
  VIEW_XML_GROUP views
  DISPLAY_XML ClientChartDisplay
  DISPLAY_PANEL ClientChartDisplay
  )

ADD_PARAVIEW_VIEW_MODULE(
  BAR_IFACES BAR_IFACE_SRCS
  VIEW_TYPE ClientBarChartView
  VIEW_NAME "Bar Chart"
  VIEW_XML_GROUP views
  DISPLAY_XML ClientChartDisplay
  DISPLAY_PANEL ClientChartDisplay
  )

ADD_PARAVIEW_VIEW_MODULE(
  LINE_IFACES LINE_IFACE_SRCS
  VIEW_TYPE ClientLineChartView
  VIEW_NAME "Line Chart"
  VIEW_XML_GROUP views
  DISPLAY_XML ClientChartDisplay
  DISPLAY_PANEL ClientChartDisplay
  )

ADD_PARAVIEW_VIEW_MODULE(
  BOX_IFACES BOX_IFACE_SRCS
  VIEW_TYPE ClientStatisticalBoxChartView
  VIEW_NAME "Box Plot"
  VIEW_XML_GROUP views
  DISPLAY_XML ClientChartDisplay
  DISPLAY_PANEL ClientChartDisplay
  )

ADD_PARAVIEW_VIEW_OPTIONS(
  STACKED_OPTIONS_IFACE STACKED_OPTIONS_IFACE_SRCS
  VIEW_TYPE ClientStackedChartView
  ACTIVE_VIEW_OPTIONS ClientChartViewActiveOptions)

ADD_PARAVIEW_VIEW_OPTIONS(
  BAR_OPTIONS_IFACE BAR_OPTIONS_IFACE_SRCS
  VIEW_TYPE ClientBarChartView
  ACTIVE_VIEW_OPTIONS ClientChartViewActiveOptions)

ADD_PARAVIEW_VIEW_OPTIONS(
  LINE_OPTIONS_IFACE LINE_OPTIONS_IFACE_SRCS
  VIEW_TYPE ClientLineChartView
  ACTIVE_VIEW_OPTIONS ClientChartViewActiveOptions)

ADD_PARAVIEW_VIEW_OPTIONS(
  BOX_OPTIONS_IFACE BOX_OPTIONS_IFACE_SRCS
  VIEW_TYPE ClientStatisticalBoxChartView
  ACTIVE_VIEW_OPTIONS ClientChartViewActiveOptions)

ADD_PARAVIEW_PLUGIN(
  ClientChartViewPlugin "1.0"
  GUI_INTERFACES ${STACKED_IFACES} ${STACKED_OPTIONS_IFACE} ${BAR_IFACES} ${BAR_OPTIONS_IFACE} ${LINE_IFACES} ${LINE_OPTIONS_IFACE} ${BOX_IFACES} ${BOX_OPTIONS_IFACE}
  SOURCES ${SOURCES} ${MOC_SRCS} ${UI_BUILT_SOURCES} ${STACKED_IFACE_SRCS} ${STACKED_OPTIONS_IFACE_SRCS} ${BAR_IFACE_SRCS} ${BAR_OPTIONS_IFACE_SRCS} ${LINE_IFACE_SRCS} ${LINE_OPTIONS_IFACE_SRCS} ${BOX_IFACE_SRCS} ${BOX_OPTIONS_IFACE_SRCS}
  SERVER_MANAGER_XML ClientChartView.xml
  )

TARGET_LINK_LIBRARIES(
  ClientChartViewPlugin
  OverViewCore
  QVTK
  pqComponents
  vtkQtChart
  )

