cmake_minimum_required(VERSION 3.3)
project(Demo1)

find_package(ParaView REQUIRED)
include(${PARAVIEW_USE_FILE})
if (NOT PARAVIEW_BUILD_QT_GUI)
  message(FATAL_ERROR
    "Demo1 example requires PARAVIEW_BUILD_QT_GUI to be enabled. "
    "Please rebuild ParaView (or point to a different build of ParaView) "
    "with PARAVIEW_BUILD_QT_GUI set to TRUE")
endif()

# Set a consistent MACOSX_RPATH default across all CMake versions.
# When CMake 2.8.12 is required, change this default to 1.
# When CMake 3.0.0 is required, remove this block (see CMP0042).
if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

SET (SOURCE_FILES
  DemoApp1.cxx
  myMainWindow.cxx
  myMainWindow.h
)

pv_qt_wrap_cpp(MOC_SRCS myMainWindow.h)
pv_qt_wrap_ui(UI_BUILT_SOURCES myMainWindow.ui)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

add_executable(DemoApp1 ${SOURCE_FILES} ${MOC_SRCS} ${UI_BUILT_SOURCES})
target_link_libraries(DemoApp1 pqApplicationComponents)
if(PARAVIEW_QT_VERSION VERSION_GREATER "4")
  set_target_properties(DemoApp1 PROPERTIES
    COMPILE_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
endif()
