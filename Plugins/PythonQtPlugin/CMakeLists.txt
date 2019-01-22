find_package(Qt5 QUIET REQUIRED COMPONENTS Widgets)

list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(PythonQt QUIET REQUIRED)

set(sources
  pqPythonEventFilter.h
  pqPythonQtMethodHelpers.h
  pqPythonQtPlugin.cxx
  pqPythonQtPlugin.h
  pqPythonQtWrapperFactory.cxx
  pqPythonQtWrapperFactory.h)

set(interfaces)
paraview_plugin_add_auto_start(
  CLASS_NAME "pqPythonQtPlugin"
  INTERFACES autostart_interface
  SOURCES autostart_sources)
list(APPEND interfaces
  ${autostart_interface})
list(APPEND sources
  ${autostart_sources})

paraview_add_plugin(PythonQtPlugin
  REQUIRED_ON_CLIENT
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

target_link_libraries(PythonQtPlugin
  PRIVATE
    VTK::CommonCore
    VTK::Python
    VTK::PythonInterpreter
    VTK::WrappingPythonCore)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
