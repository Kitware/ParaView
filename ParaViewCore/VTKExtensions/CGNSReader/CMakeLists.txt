set(PLUGIN_CGNS_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
set(PLUGIN_CGNS_CMAKE_PATH ${PLUGIN_CGNS_DIR}/CMake)

#include the CGNS cmake directory on the cmake search path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PLUGIN_CGNS_CMAKE_PATH})

#
find_package(CGNS REQUIRED)
if (NOT CGNS_FOUND)
  message(FATAL_ERROR "Unable to locate CGNS library.")
endif()

include_directories(${CGNS_INCLUDE_DIR})

# -----------------------------------------------------------------------------
# Disable some warnings
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

set(Module_SRCS
cgio_helpers.cxx
vtkCGNSReaderInternal.cxx
vtkCGNSReader.cxx
)

set_source_files_properties(
  vtkCGNSReaderInternal
  cgio_helpers
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
    SKIP_HEADER_INSTALL 1
)

vtk_module_library(vtkPVVTKExtensionsCGNSReader ${Module_SRCS})
target_link_libraries(vtkPVVTKExtensionsCGNSReader
                      LINK_PRIVATE
                        ${CGNS_LIBRARIES}
                      )
vtk_mpi_link(vtkPVVTKExtensionsCGNSReader)
