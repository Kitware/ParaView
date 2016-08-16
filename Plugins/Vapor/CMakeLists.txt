PROJECT(VDFReader)

# Plugin that uses the Vapor libraries to read VDF files into ParaView

cmake_minimum_required(VERSION 3.3)

IF (ParaView_SOURCE_DIR)
  #building within the ParaView source tree
  INCLUDE_DIRECTORIES(${VTK_INCLUDE_DIRS})
ELSE ()
  #building outside of the source tree
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF ()

#find where all vapor header files live (especially vaporinternal/common.h)
#include the parts of vapor that we need to compile the reader
#both vapor/ and vaporinternal/ are underneath this
FIND_PATH (VAPOR_HEADERS
  vaporinternal/common.h
  PATHS /ThirdPartyLibraries/VAPOR/source/vapor-1.5.2/vapor/include
  DOC "NCAR Vapor source directory, where include/vaporinternal/common.h can be found"
  )
INCLUDE_DIRECTORIES(${VAPOR_HEADERS})

#link to the parts of vapor lib that we need to run/compile the reader
FIND_LIBRARY(VAPOR_VDF_LIB vdf DOC "NCAR Vapor Data Format library")
FIND_LIBRARY(VAPOR_COMMON_LIB common DOC "NCAR Vapor common libary")

#find expat and netcdf libs that vapor library itself uses
FIND_PACKAGE(EXPAT REQUIRED)
INCLUDE_DIRECTORIES(${EXPAT_INCLUDE_DIRS})
FIND_LIBRARY(VAPOR_NETCDF_LIB netcdf DOC "NetCDF library required for VAPOR plugin")
FIND_PATH (VAPOR_NETCDF_INCLUDE_DIRS
  netcdf.h
  DOC "NetCDF include directory for VAPOR plugin"
  )
INCLUDE_DIRECTORIES(${VAPOR_NETCDF_INCLUDE_DIRS})

#TODO: GUI XML should not be necessary. Why are hints in server xml not working for the plugin?
ADD_PARAVIEW_PLUGIN(VDFReaderPlugin "1.0"
  SERVER_MANAGER_XML VDFReader.xml
  SERVER_MANAGER_SOURCES vtkVDFReader.cxx
  REQUIRED_ON_SERVER
)
target_link_libraries(VDFReaderPlugin
  LINK_PRIVATE
  ${VAPOR_VDF_LIB}
  ${VAPOR_COMMON_LIB}
  ${EXPAT_LIBRARIES}
  ${VAPOR_NETCDF_LIB}
  )
