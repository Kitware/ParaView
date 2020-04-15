set(sources
  pqSaveStateAndScreenshotActions.cxx
  pqSaveStateAndScreenshotReaction.cxx
)

set(interfaces)

paraview_plugin_add_toolbar(
  CLASS_NAME pqSaveStateAndScreenshotActions
  INTERFACES toolbar_interfaces
  SOURCES toolbar_sources
)

list(APPEND interfaces ${toolbar_interfaces})
list(APPEND sources ${toolbar_sources})

paraview_add_plugin(SaveStateAndScreenshot
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
)

target_link_libraries(SaveStateAndScreenshot
  PRIVATE
    ParaView::pqApplicationComponents
    ParaView::RemotingViews
)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
