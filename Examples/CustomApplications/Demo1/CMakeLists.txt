
cmake_minimum_required(VERSION 2.6)

project(Demo1)

FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

INCLUDE(${ParaView_CMAKE_DIR}/ParaViewBranding.cmake)
INCLUDE(${ParaView_CMAKE_DIR}/ParaViewPlugins.cmake)

SET (SOURCE_FILES 
  DemoApp1.cxx
  myMainWindow.cxx
  myMainWindow.h
)

QT4_WRAP_CPP(MOC_SRCS
  myMainWindow.h
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  myMainWindow.ui
)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

ADD_EXECUTABLE(DemoApp1 ${SOURCE_FILES} ${MOC_SRCS} ${UI_BUILT_SOURCES})
TARGET_LINK_LIBRARIES(DemoApp1 pqCore pqComponents pqApplicationComponents
${QT_LIBRARIES} )
