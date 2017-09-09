# Linking to Paraview: needed to build a paraview plugin
if (NOT ParaView_BINARY_DIR)
  find_package(ParaView REQUIRED)
    include(${PARAVIEW_USE_FILE})
else()
  include_directories(${VTK_INCLUDE_DIRS})
endif(NOT ParaView_BINARY_DIR)


# Building GIO
add_definitions(-DGENERICIO_NO_MPI)
add_definitions(-DGENERICIO_NO_COMPRESSION)   # Compression not supported yet
add_library(GenericIO STATIC "${CMAKE_CURRENT_SOURCE_DIR}/GIO/GenericIO.cxx")
set_property(TARGET GenericIO PROPERTY POSITION_INDEPENDENT_CODE ON)


# Sources
set(SRC_LIST vtkGenIOReader.h vtkGenIOReader.cpp)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/GIO)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utils)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tools)


# Enable or disable log output
option(GIO_DEBUG_LOG "Enable GenericIO ParaView Plugin Logging" OFF)
if (GIO_DEBUG_LOG)
  add_definitions(-DOUTPUT_GIO_LOGS)
endif()


# Create the plugin
add_paraview_plugin(GenericIOReader "1.0"
  SERVER_MANAGER_XML vtkGenIOReader.xml
  SERVER_MANAGER_SOURCES vtkGenIOReader.cpp
  SOURCES ${SRC_LIST}
  REQUIRED_ON_SERVER
)

target_link_libraries(GenericIOReader LINK_PUBLIC GenericIO)

if (BUILD_TESTING AND PARAVIEW_BUILD_QT_GUI)
  add_subdirectory(Testing)
endif()
