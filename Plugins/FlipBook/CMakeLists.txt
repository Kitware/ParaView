set(sources
  pqFlipBookReaction.cxx
  pqFlipBookToolbarActions.cxx
)

set(qrc_files
  pqFlipBookResources.qrc
)

set(interfaces)

paraview_plugin_add_toolbar(
  CLASS_NAME pqFlipBookToolbarActions
  INTERFACES toolbar_interfaces
  SOURCES toolbar_sources
)

list(APPEND interfaces ${toolbar_interfaces})
list(APPEND sources ${toolbar_sources})

paraview_add_plugin(FlipBookPlugin
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  UI_RESOURCES ${qrc_files}
  SOURCES ${sources}
)

target_link_libraries(FlipBookPlugin
  PRIVATE
    ParaView::pqApplicationComponents
)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
