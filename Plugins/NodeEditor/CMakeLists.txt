list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

set(interfaces)
set(sources)

list(APPEND sources
  pqNodeEditorAnnotationItem.h
  pqNodeEditorAnnotationItem.cxx
  pqNodeEditorUtils.h
  pqNodeEditorUtils.cxx
  pqNodeEditorNode.h
  pqNodeEditorNode.cxx
  pqNodeEditorNSource.h
  pqNodeEditorNSource.cxx
  pqNodeEditorNRepresentation.h
  pqNodeEditorNRepresentation.cxx
  pqNodeEditorNView.h
  pqNodeEditorNView.cxx
  pqNodeEditorPort.h
  pqNodeEditorPort.cxx
  pqNodeEditorEdge.h
  pqNodeEditorEdge.cxx
  pqNodeEditorWidget.cxx
  pqNodeEditorWidget.h
  pqNodeEditorView.cxx
  pqNodeEditorView.h
  pqNodeEditorScene.cxx
  pqNodeEditorScene.h
  pqNodeEditorLabel.cxx
  pqNodeEditorLabel.h
)

paraview_plugin_add_dock_window(
  CLASS_NAME pqNodeEditorWidget
  DOCK_AREA Right
  INTERFACES dock_interfaces
  SOURCES dock_sources)
list(APPEND interfaces
  ${dock_interfaces})
list(APPEND sources
  ${dock_sources})

paraview_add_plugin(NodeEditor
  REQUIRED_ON_CLIENT
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

option(NodeEditor_ENABLE_AUTOLAYOUT "Enable auto layout for the node editor. Needs Graphviz." OFF)
mark_as_advanced(NodeEditor_ENABLE_AUTOLAYOUT)
if(NodeEditor_ENABLE_AUTOLAYOUT)
  find_package(Graphviz REQUIRED)
  target_link_libraries(NodeEditor
    PRIVATE
      Graphviz::cgraph
      Graphviz::gvc)
endif()

set_property(SOURCE pqNodeEditorScene.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS "NodeEditor_ENABLE_GRAPHVIZ=$<BOOL:${NodeEditor_ENABLE_AUTOLAYOUT}>")
target_link_libraries(NodeEditor
  PRIVATE
    ParaView::RemotingViews
    ParaView::pqApplicationComponents
)
target_compile_definitions(NodeEditor PRIVATE QT_NO_KEYWORDS)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
