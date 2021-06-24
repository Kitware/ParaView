list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

set(interfaces)
set(sources)

list(APPEND sources
  pqNodeEditorUtils.h
  pqNodeEditorUtils.cxx
  pqNodeEditorNode.h
  pqNodeEditorNode.cxx
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
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

find_package(Graphviz QUIET)
if (Graphviz_FOUND)
  target_link_libraries(NodeEditor
    PRIVATE
      Graphviz::cgraph
      Graphviz::gvc)
endif ()

set_property(SOURCE pqNodeEditorScene.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS "NodeEditor_ENABLE_GRAPHVIZ=$<BOOL:${Graphviz_FOUND}>")
target_link_libraries(NodeEditor
  PRIVATE
    ParaView::RemotingViews
    ParaView::pqApplicationComponents
)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
