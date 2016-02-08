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
)

if(BUILD_TESTING AND PARAVIEW_BUILD_QT_GUI)
  include(ParaViewTestingMacros)
  add_client_tests("pv"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/TestForceTimePlugin.xml"
    LOAD_PLUGIN "ForceTime")
endif()
