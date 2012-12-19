#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
if(NOT ParaView_SOURCE_DIR)
  find_package(ParaView)
  include("${PARAVIEW_USE_FILE}")
  include (ParaViewPlugins)
else()
  # we're building from within ParaView Source.
endif()

include_directories(${VTK_INCLUDE_DIRS})
add_paraview_plugin(ForceTime "1.0"
  SERVER_MANAGER_SOURCES vtkForceTime.cxx
  SERVER_MANAGER_XML ForceTimeServer.xml
  GUI_RESOURCE_FILES ForceTimeClient.xml
)
