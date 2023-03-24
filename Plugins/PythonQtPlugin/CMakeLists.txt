find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" QUIET REQUIRED COMPONENTS Widgets)

list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package("Python3" QUIET REQUIRED COMPONENTS Interpreter)

find_package(PythonQt QUIET REQUIRED)

set(decorator_file "${CMAKE_CURRENT_BINARY_DIR}/pqPluginDecorators.h")

set(sources
  pqPythonEventFilter.h
  pqPythonQtMethodHelpers.h
  pqPythonQtPlugin.cxx
  pqPythonQtPlugin.h
  pqPythonQtWrapperFactory.cxx
  pqPythonQtWrapperFactory.h
  "${decorator_file}")
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
    VTK::WrappingPythonCore
    ParaView::pqApplicationComponents
    ParaView::pqPython
    PythonQt::PythonQt)

if(TARGET PythonQt::PythonQt_QtAll)
  target_link_libraries(PythonQtPlugin
    PRIVATE
      PythonQt::PythonQt_QtAll)
endif()

target_compile_definitions(PythonQtPlugin PRIVATE
  WITH_QtAll=$<TARGET_EXISTS:PythonQt::PythonQt_QtAll>)

target_include_directories(PythonQtPlugin
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(
  OUTPUT  "${decorator_file}"
  COMMAND "$<TARGET_FILE:Python3::Interpreter>"
          "${CMAKE_CURRENT_SOURCE_DIR}/WrapPythonQt.py"
          -i "${CMAKE_CURRENT_SOURCE_DIR}/wrapped_methods.txt"
          -o "${decorator_file}"
          --class-prefixes vtk pq
          --qt-class-prefixes pq
          --module-name paraview
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/wrapped_methods.txt"
          "${CMAKE_CURRENT_SOURCE_DIR}/WrapPythonQt.py")

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
