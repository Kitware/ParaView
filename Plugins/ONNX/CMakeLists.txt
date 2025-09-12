set(interfaces )
set(sources
    pqONNXPropertyGroupWidget.cxx
    pqONNXPropertyGroupWidget.h
    pqDoublePropertyMultiWidgets.cxx
    pqDoublePropertyMultiWidgets.h
  )

paraview_plugin_add_property_widget(
  KIND GROUP_WIDGET
  TYPE "onnx_property_widget"
  CLASS_NAME pqONNXPropertyGroupWidget
  INTERFACES property_interfaces
  SOURCES property_sources)
list(APPEND interfaces
  ${property_interfaces})
list(APPEND sources
  ${property_sources})

paraview_add_plugin(
  ONNXPlugin
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
  MODULES
    ONNXPlugin::ONNXParameters
  MODULE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Parameters/vtk.module"
  SERVER_MANAGER_XML "ONNXPlugin.xml")

target_link_libraries(ONNXPlugin
  PUBLIC
    ParaView::RemotingServerManager
    ParaView::pqComponents
)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
