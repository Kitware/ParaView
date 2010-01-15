project(FortranAdaptor)
cmake_minimum_required(VERSION 2.6)
include_directories(${CoProcessor_SOURCE_DIR})

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

if(UNIX AND NOT WIN32)
  find_program(CMAKE_UNAME uname /bin /usr/bin /usr/local/bin)
  if(CMAKE_UNAME)
    exec_program(uname ARGS -m OUTPUT_VARIABLE CMAKE_SYSTEM_PROCESSOR)
    set(CMAKE_SYSTEM_PROCESSOR 
      ${CMAKE_SYSTEM_PROCESSOR}
      CACHE
      INTERNAL
      "processor type (i386 and x86_64)")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
      add_definitions(-fPIC)
    endif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  endif(CMAKE_UNAME)
endif(UNIX AND NOT WIN32)

set(FortranAdaptor_SRCS
  FortranAdaptorAPI.cxx
)
add_library(FortranAdaptor ${FortranAdaptor_SRCS})
target_link_libraries(
  FortranAdaptor vtkCoProcessor
)

#option(BUILD_PHASTA_ADAPTOR "Build Phasta Adaptor" OFF)
#if(BUILD_PHASTA_ADAPTOR)
#  add_subdirectory(PhastaAdaptor)
#endif(BUILD_PHASTA_ADAPTOR)

