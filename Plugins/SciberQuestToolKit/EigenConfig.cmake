#    ____    _ __           ____               __    ____
#   / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
#  _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
# /___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
# Copyright 2012 SciberQuest Inc.
#
set(Eigen_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/eigen-3.0.3/eigen-eigen-3.0.3
  CACHE FILEPATH
  "Path to Eigen install.")
mark_as_advanced(Eigen_DIR)

if (NOT EXISTS ${Eigen_DIR})
  message(FATAL_ERROR
    "SQTK Set Eigen_DIR to the path to your Eigen install." )
endif ()

include_directories(${Eigen_DIR})

# prevent intel compilers from errouniously using intrinsics packaged
# with gcc
if (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  set(INTEL_COMP_ROOT "" CACHE PATH "Intel compiler root")
  get_filename_component(INTEL_COMP_ROOT ${CMAKE_CXX_COMPILER} PATH)
  mark_as_advanced(INTEL_COMP_ROOT)
  exec_program(${CMAKE_CXX_COMPILER}
    ARGS ${CMAKE_CXX_COMPILER_ARG1} -dumpversion
    OUTPUT_VARIABLE INTEL_COMP_VERSION)
  string(REGEX
    REPLACE "([0-9])\\.([0-9])(\\.[0-9])?" "\\1\\2"
    INTEL_COMP_VERSION ${INTEL_COMP_VERSION})
  set(INTEL_XMMINTRIN)
  mark_as_advanced(INTEL_XMMINTRIN)
  #if (INTEL_COMP_VERSION LESS 120)
    set(_PATH_TOK)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(_PATH_TOK "intel64")
    else()
      set(_PATH_TOK "ia32")
    endif()
    file(GLOB_RECURSE _XMM_INTRINS FOLLOW_SYMLINKS "${INTEL_COMP_ROOT}/../../xmmintrin.h*")
    foreach(_XMM_INTRIN ${_XMM_INTRINS})
      string(REGEX MATCH "${_PATH_TOK}/xmmintrin.h" _X ${_XMM_INTRIN})
      if (_X)
        set(INTEL_XMMINTRIN ${_XMM_INTRIN})
      endif()
    endforeach()
    if (NOT INTEL_XMMINTRIN)
      message(WARNING "The Intel compiler may need EXTRA_INTEL_INCLUDES set to the directory containing xmmintrin.h")
      message(STATUS "CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
      message(STATUS "INTEL_COMP_ROOT=${INTEL_COMP_ROOT}")
      message(STATUS "INTEL_COMP_VERSION=${INTEL_COMP_VERSION}")
    endif()
  #endif ()
  if (INTEL_XMMINTRIN)
    get_filename_component(INTEL_XMMINTRIN_PATH ${INTEL_XMMINTRIN} PATH)
  endif()
  mark_as_advanced(INTEL_XMMINTRIN_PATH)
  set(EXTRA_INTEL_INCLUDES ${INTEL_XMMINTRIN_PATH} CACHE PATH
    "Set this to the path to intel xmm intrinsics header if there are conflicts with gcc's xmm intrinsics.")
  mark_as_advanced(EXTRA_INTEL_INCLUDES)
  if (EXISTS ${EXTRA_INTEL_INCLUDES})
    include_directories(${EXTRA_INTEL_INCLUDES})
  endif ()
endif ()
