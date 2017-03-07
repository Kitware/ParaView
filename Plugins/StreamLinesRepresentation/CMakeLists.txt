if (NOT ParaView_BINARY_DIR)
  cmake_minimum_required(VERSION 3.3)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

include_directories(
  ${VTK_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

include(vtkOpenGL)
add_definitions(-DVTKGL2)

set(SRCS)

if(PARAVIEW_BUILD_QT_GUI)
  pv_find_package_qt(qt_targets REQUIRED QUIET
    QT4_COMPONENTS QtGui
    QT5_COMPONENTS Widgets)

  pv_qt_wrap_cpp(MOC_SRCS
    pqStreamLinesAnimationManager.h
    )

  add_paraview_auto_start(
    IFACES IFACE_SRCS
    CLASS_NAME pqStreamLinesAnimationManager
    STARTUP onStartup
    SHUTDOWN onShutdown
    )
 list(APPEND SRCS "pqStreamLinesAnimationManager.cxx")
endif()

encode_files_as_strings(ENCODED_STRING_FILES
  shaders/vtkStreamLines_fs.glsl
  shaders/vtkStreamLines_gs.glsl
  shaders/vtkStreamLines_vs.glsl
  shaders/vtkStreamLinesBlending_fs.glsl
  shaders/vtkStreamLinesCopy_fs.glsl
  )

add_paraview_plugin(
  StreamLinesRepresentation "0.1"
  DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc"
  SERVER_MANAGER_XML StreamLinesRepresentation.xml
  SERVER_MANAGER_SOURCES
    vtkStreamLinesRepresentation.cxx
    vtkStreamLinesMapper.cxx
  GUI_INTERFACES ${IFACES}
  SOURCES
    ${ENCODED_STRING_FILES}
    ${SRCS} ${MOC_SRCS} ${RCS_SRCS} ${IFACE_SRCS}
  )

include(vtkOpenGL)
vtk_opengl_link(StreamLinesRepresentation)

if(PARAVIEW_BUILD_QT_GUI)
  target_link_libraries(StreamLinesRepresentation LINK_PRIVATE pqApplicationComponents)

  if (BUILD_TESTING)
    add_subdirectory(Testing)
  endif()
endif()
