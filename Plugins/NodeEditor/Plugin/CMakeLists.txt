find_package(Qt5 REQUIRED COMPONENTS Widgets)

set(interfaces)
set(sources
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

# START_FIND_GRAPHVIZ
find_path(GRAPHVIZ_INCLUDE_DIR
  NAMES
    graphviz/cgraph.h
  HINTS
    ${_GRAPHVIZ_INCLUDE_DIR}
    )
mark_as_advanced(GRAPHVIZ_INCLUDE_DIR)

find_library(GRAPHVIZ_CDT_LIBRARY
  NAMES
    cdt
  HINTS
    ${_GRAPHVIZ_LIBRARY_DIR}
    )
mark_as_advanced(GRAPHVIZ_CDT_LIBRARY)

find_library(GRAPHVIZ_GVC_LIBRARY
  NAMES
    gvc
  HINTS
    ${_GRAPHVIZ_LIBRARY_DIR}
    )
mark_as_advanced(GRAPHVIZ_GVC_LIBRARY)

find_library(GRAPHVIZ_CGRAPH_LIBRARY
  NAMES
    cgraph
  HINTS
    ${_GRAPHVIZ_LIBRARY_DIR}
    )
mark_as_advanced(GRAPHVIZ_CGRAPH_LIBRARY)

find_library(GRAPHVIZ_PATHPLAN_LIBRARY
  NAMES
    pathplan
  HINTS
    ${_GRAPHVIZ_LIBRARY_DIR}
    )
mark_as_advanced(GRAPHVIZ_PATHPLAN_LIBRARY)

if(GRAPHVIZ_INCLUDE_DIR
    AND GRAPHVIZ_CDT_LIBRARY
    AND GRAPHVIZ_GVC_LIBRARY
    AND GRAPHVIZ_CGRAPH_LIBRARY
    AND GRAPHVIZ_PATHPLAN_LIBRARY
    )
  set(NodeEditor_ENABLE_GRAPHVIZ "Enable GraphViz Support" ON INTERNAL)
  message(STATUS "NodeEditor Plugin: GraphViz found")
else()
  set(NodeEditor_ENABLE_GRAPHVIZ "Enable GraphViz Support" OFF INTERNAL)
  message(STATUS "NodeEditor Plugin: GraphViz NOT FOUND!")
endif()

target_compile_definitions(NodeEditor
  PUBLIC
    NodeEditor_ENABLE_GRAPHVIZ
)

target_include_directories(NodeEditor
  PUBLIC
    ${GRAPHVIZ_INCLUDE_DIR}
)

target_link_libraries(NodeEditor
  PRIVATE
    ${GRAPHVIZ_CDT_LIBRARY}
    ${GRAPHVIZ_GVC_LIBRARY}
    ${GRAPHVIZ_CGRAPH_LIBRARY}
    ${GRAPHVIZ_PATHPLAN_LIBRARY}
    ParaView::RemotingViews
    ParaView::pqApplicationComponents
    Qt5::Widgets
)
