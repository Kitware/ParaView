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

find_package(GRAPHVIZ QUIET)

if(GRAPHVIZ_FOUND)
  message(STATUS "NodeEditor Plugin: GraphViz found, auto layout enabled")

  target_include_directories(NodeEditor
    PRIVATE
      ${GRAPHVIZ_INCLUDE_DIR}
  )

  target_link_libraries(NodeEditor
    PRIVATE
      ${GRAPHVIZ_CDT_LIBRARY}
      ${GRAPHVIZ_GVC_LIBRARY}
      ${GRAPHVIZ_CGRAPH_LIBRARY}
      ${GRAPHVIZ_PATHPLAN_LIBRARY}
  )

  target_compile_definitions(NodeEditor
    PRIVATE
      NodeEditor_ENABLE_GRAPHVIZ
  )

else()
  message(STATUS "NodeEditor Plugin: Graphviz not found, auto layout disabled")
endif()

target_link_libraries(NodeEditor
  PRIVATE
    ParaView::RemotingViews
    ParaView::pqApplicationComponents
)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
