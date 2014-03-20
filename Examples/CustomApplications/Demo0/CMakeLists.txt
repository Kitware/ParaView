cmake_minimum_required(VERSION 2.8.8)
project(Demo0)

find_package(ParaView REQUIRED)
include(${PARAVIEW_USE_FILE})
if (NOT PARAVIEW_BUILD_QT_GUI)
  message(FATAL_ERROR
    "Demo0 example requires PARAVIEW_BUILD_QT_GUI to be enabled. "
    "Please rebuild ParaView (or point to a different build of ParaView) "
    "with PARAVIEW_BUILD_QT_GUI set to TRUE")
endif()

add_executable(DemoApp0 DemoApp0.cxx)
target_link_libraries(DemoApp0 pqCore)
if(PARAVIEW_QT_VERSION VERSION_GREATER "4")
  set_target_properties(DemoApp0 PROPERTIES
    COMPILE_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
endif()
