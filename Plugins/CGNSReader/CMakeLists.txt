set(PLUGIN_CGNS_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
set(PLUGIN_CGNS_CMAKE_PATH ${PLUGIN_CGNS_DIR}/CMake)

#include the CGNS cmake directory on the cmake search path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PLUGIN_CGNS_CMAKE_PATH})

#
find_package(CGNS REQUIRED)
if (NOT CGNS_FOUND)
  message(FATAL_ERROR "Unable to locate CGNS library.")
endif()

#
include_directories(${CGNS_INCLUDE_DIR})

set_source_files_properties(
vtkCGNSReaderInternal
cgio_helpers

WRAP_EXCLUDE
)

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
include_directories(${VTK_INCLUDE_DIRS})
if (PARAVIEW_BUILD_QT_GUI)
  include(${QT_USE_FILE})
endif()

# -----------------------------------------------------------------------------
# Disable some warnings + Add c++11
# -----------------------------------------------------------------------------
if (WIN32)
  if (MSVC_VERSION GREATER 1400)
    # -------------------------------------------------------------------------
    # Disable deprecation warnings for standard C and STL functions in VS2005
    # and newer
    # -------------------------------------------------------------------------
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
  endif ()
elseif (UNIX)
  if (CMAKE_COMPILER_IS_GNUCXX)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG(-Wno-unused-result Wno-unused-result)
    if (Wno-unused-result)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-result")
    endif()
  endif()
endif()

set(SOURCES
cgio_helpers.cxx
cgio_helpers.h
vtkCGNSReaderInternal.cxx
vtkCGNSReaderInternal.h
vtkCGNSReader.cxx
vtkCGNSReader.h
)

# ---------------------------------------------------------------------------
# Combined plugin
# -----------------------------------------------------------------------------
if (PARAVIEW_BUILD_QT_GUI)
  add_paraview_plugin(CGNSReader "1.0"
    REQUIRED_ON_SERVER
    REQUIRED_ON_CLIENT
    SERVER_MANAGER_XML CGNSReaderSM.xml
    SERVER_MANAGER_SOURCES ${SOURCES}
    GUI_INTERFACES ${IFACES}
    GUI_SOURCES ${MOC_SRCS} ${UI_SRCS} ${IFACE_SRCS}
    )
else()
  add_paraview_plugin(CGNSReader "1.0"
    REQUIRED_ON_SERVER
    REQUIRED_ON_CLIENT
    SERVER_MANAGER_XML CGNSReaderSM.xml
    SERVER_MANAGER_SOURCES ${SOURCES})
endif()

set(CGNS_LINK_TO_HDF5 ON CACHE BOOL "use -lhdf5 for linking cgns to hdf5" )
mark_as_advanced(CGNS_LINK_TO_HDF5)

if (CGNS_LINK_TO_HDF5)
  target_link_libraries(CGNSReader LINK_PRIVATE ${CGNS_LIBRARIES} -lhdf5)
else()
  target_link_libraries(CGNSReader LINK_PRIVATE ${CGNS_LIBRARIES})
endif()

# Add testing if necessary
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
