# Linking to Paraview: needed to build a paraview plugin
if(NOT ParaView_BINARY_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
else()
  include_directories(${VTK_INCLUDE_DIRS})
endif(NOT ParaView_BINARY_DIR)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# This plugin depends on a customized snapshot of GenericIO:
add_definitions(-DLANL_GENERICIO_NO_MPI)
add_definitions(-DLANL_GENERICIO_NO_COMPRESSION)   # Compression not supported yet
add_definitions(-D__STDC_CONSTANT_MACROS) # Required by GenericIO target
add_library(LANL_GenericIO STATIC "LANL/GIO/GenericIO.cxx")
set_property(TARGET LANL_GenericIO PROPERTY POSITION_INDEPENDENT_CODE ON)

# Sources
set(SRC_LIST vtkGenIOReader.h vtkGenIOReader.cxx)

# Enable or disable log output
option(GIO_DEBUG_LOG "Enable GenericIO ParaView Plugin Logging" OFF)
if(GIO_DEBUG_LOG)
  add_definitions(-DOUTPUT_GIO_LOGS)
endif()
mark_as_advanced(GIO_DEBUG_LOG)


# Create the plugin
add_paraview_plugin(GenericIOReader "1.0"
  SERVER_MANAGER_XML vtkGenIOReader.xml
  SERVER_MANAGER_SOURCES vtkGenIOReader.cxx
  SERVER_SOURCES ${SRC_LIST}
  REQUIRED_ON_SERVER
)

target_link_libraries(GenericIOReader LINK_PUBLIC LANL_GenericIO)

if(BUILD_TESTING AND PARAVIEW_BUILD_QT_GUI)
  add_subdirectory(Testing)
endif()
