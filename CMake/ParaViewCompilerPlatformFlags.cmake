set(PARAVIEW_REQUIRED_C_FLAGS)
set(PARAVIEW_REQUIRED_CXX_FLAGS)

# A GCC compiler.
if(CMAKE_COMPILER_IS_GNUCXX)
  if(CYGWIN)
    string(APPEND PARAVIEW_REQUIRED_CXX_FLAGS " -mwin32")
    string(APPEND PARAVIEW_REQUIRED_C_FLAGS " -mwin32")
    link_libraries(-lgdi32)
  endif()
  if(MINGW)
    string(APPEND PARAVIEW_REQUIRED_CXX_FLAGS " -mthreads")
    string(APPEND PARAVIEW_REQUIRED_C_FLAGS " -mthreads")
    string(APPEND PARAVIEW_REQUIRED_EXE_LINKER_FLAGS " -mthreads")
    string(APPEND PARAVIEW_REQUIRED_SHARED_LINKER_FLAGS " -mthreads")
    string(APPEND PARAVIEW_REQUIRED_MODULE_LINKER_FLAGS " -mthreads")
  endif()
else()
  if(CMAKE_ANSI_CFLAGS)
    string(APPEND PARAVIEW_REQUIRED_C_FLAGS " ${CMAKE_ANSI_CFLAGS}")
  endif()
endif()

# figure out whether the compiler might be the Intel compiler
set(_MAY_BE_INTEL_COMPILER FALSE)
if(UNIX)
  if(CMAKE_CXX_COMPILER_ID)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
      set(_MAY_BE_INTEL_COMPILER TRUE)
    endif()
  else()
    if(NOT CMAKE_COMPILER_IS_GNUCXX)
      set(_MAY_BE_INTEL_COMPILER TRUE)
    endif()
  endif()
endif()

#if so, test whether -i_dynamic is needed
if(_MAY_BE_INTEL_COMPILER)
  include(${CMAKE_CURRENT_LIST_DIR}/ParaViewTestNO_ICC_IDYNAMIC_NEEDED.cmake)
  testno_icc_idynamic_needed(NO_ICC_IDYNAMIC_NEEDED ${CMAKE_CURRENT_LIST_DIR})
  if(NO_ICC_IDYNAMIC_NEEDED)
    set(PARAVIEW_REQUIRED_CXX_FLAGS "${PARAVIEW_REQUIRED_CXX_FLAGS}")
  else()
    set(PARAVIEW_REQUIRED_CXX_FLAGS "${PARAVIEW_REQUIRED_CXX_FLAGS} -i_dynamic")
  endif()
endif()

if(MSVC)
# Use the highest warning level for visual c++ compiler.
  set(CMAKE_CXX_WARNING_LEVEL 4)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
endif()

# Disable deprecation warnings for standard C and STL functions in VS2015+
# and later
if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-D_SCL_SECURE_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
endif()

# Enable /MP flag for Visual Studio
if(MSVC)
  set(CMAKE_CXX_MP_FLAG OFF CACHE BOOL "Build with /MP flag enabled")
  set(PROCESSOR_COUNT "$ENV{NUMBER_OF_PROCESSORS}")
  set(CMAKE_CXX_MP_NUM_PROCESSORS ${PROCESSOR_COUNT} CACHE STRING "The maximum number of processes for the /MP flag")
  if (CMAKE_CXX_MP_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP${CMAKE_CXX_MP_NUM_PROCESSORS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP${CMAKE_CXX_MP_NUM_PROCESSORS}")
  endif ()
endif()

# Enable /bigobj for MSVC to allow larger symbol tables
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /bigobj")
endif()

# Use /utf-8 so that MSVC uses utf-8 in source files and object files
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
endif()

#-----------------------------------------------------------------------------
# Add compiler flags ParaView needs to work on this platform.  This must be
# done after the call to CMAKE_EXPORT_BUILD_SETTINGS, but before any
# try-compiles are done.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PARAVIEW_REQUIRED_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PARAVIEW_REQUIRED_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PARAVIEW_REQUIRED_EXE_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${PARAVIEW_REQUIRED_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PARAVIEW_REQUIRED_MODULE_LINKER_FLAGS}")
