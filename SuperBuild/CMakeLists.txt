
#-----------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.3)
#-----------------------------------------------------------------------------

# Important: change architecture *before* any enable_language() or project()
# calls so that it's set properly to detect 64-bit-ness...
#
if(APPLE)
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET OR "${CMAKE_OSX_DEPLOYMENT_TARGET}" STREQUAL "")
    set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "force build for 64-bit Leopard" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.5" CACHE STRING "force build for 64-bit Leopard" FORCE)
    set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.5.sdk" CACHE PATH "force build for 64-bit Leopard" FORCE)
  endif()

  set(pv_apple_compiler_args
    -DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}
    -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
    -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT})

  if(NOT "${CMAKE_OSX_SYSROOT}" STREQUAL "")
    if(NOT EXISTS "${CMAKE_OSX_SYSROOT}")
      message(FATAL_ERROR "error: CMAKE_OSX_SYSROOT='${CMAKE_OSX_SYSROOT}' does not exist")
    endif()
  endif()
endif()

project(ParaViewSuperBuild)

set(ParaViewSuperBuild_CMAKE_SOURCE_DIR ${ParaViewSuperBuild_SOURCE_DIR})
set(ParaViewSuperBuild_CMAKE_BINARY_DIR ${ParaViewSuperBuild_BINARY_DIR})

#------------------------------------------------------------------------------
# Update CMake module path
#------------------------------------------------------------------------------

set(CMAKE_MODULE_PATH
  ${ParaViewSuperBuild_CMAKE_SOURCE_DIR}
  ${ParaViewSuperBuild_CMAKE_BINARY_DIR}
  ${CMAKE_MODULE_PATH}
  )

#-----------------------------------------------------------------------------
# Enable and setup External project global properties
#-----------------------------------------------------------------------------

INCLUDE(ExternalProject)

#-----------------------------------------------------------------------------
# Git protocole option
#-----------------------------------------------------------------------------

option(GIT_USE_GIT_PROTOCOL "If behind a firewall turn this off to use http instead." ON)

set(git_protocol "git")
if(NOT Slicer_USE_GIT_PROTOCOL)
  set(git_protocol "http")
endif()

#-----------------------------------------------------------------------------
# Qt - Let's check if a valid version of Qt is available
#-----------------------------------------------------------------------------

FIND_PACKAGE(Qt4)
if(QT_FOUND)
  #IF("${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}" VERSION_LESS "${minimum_required_qt_version}")
  #  message(FATAL_ERROR "error: Slicer requires Qt >= ${minimum_required_qt_version} -- you cannot use Qt ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}.")
  #ENDIF()
else()
  #message(FATAL_ERROR "error: Qt4 was not found on your system. Please set QT_QMAKE_EXECUTABLE")
endif()

# ----------------------------------------------------------------------------
# generic compiler args (compiler and cflags) to pass to subprojects
#-----------------------------------------------------------------------------

set(pv_compiler_args
  -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
  -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
  -DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG}
  -DCMAKE_CXX_FLAGS_MINSIZEREL:STRING=${CMAKE_CXX_FLAGS_MINSIZEREL}
  -DCMAKE_CXX_FLAGS_RELEASE:STRING=${CMAKE_CXX_FLAGS_RELEASE}
  -DCMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
  -DCMAKE_C_FLAGS_DEBUG:STRING=${CMAKE_C_FLAGS_DEBUG}
  -DCMAKE_C_FLAGS_MINSIZEREL:STRING=${CMAKE_C_FLAGS_MINSIZEREL}
  -DCMAKE_C_FLAGS_RELEASE:STRING=${CMAKE_C_FLAGS_RELEASE}
  -DCMAKE_C_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_C_FLAGS_RELWITHDEBINFO}
  -DCMAKE_EXE_LINKER_FLAGS:STRING=${CMAKE_EXE_LINKER_FLAGS}
  -DCMAKE_SHARED_LINKER_FLAGS:STRING=${CMAKE_SHARED_LINKER_FLAGS}
)

set(pv_tpl_cxx_flags ${CMAKE_CXX_FLAGS})
set(pv_tpl_c_flags ${CMAKE_C_FLAGS})
set(pv_cxx_flags ${CMAKE_CXX_FLAGS})
set(pv_c_flags ${CMAKE_C_FLAGS})

if(WIN32)
  list(APPEND pv_compiler_args -DCMAKE_BUILD_TYPE:STRING=${CMAKE_CFG_INTDIR})
else()
  list(APPEND pv_compiler_args -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE})
endif()

if(APPLE)
  list(APPEND pv_compiler_args ${pv_apple_compiler_args})
endif()

# We aren't interested in seeing third party compiler warnings.
set(pv_tpl_compiler_args ${pv_compiler_args})
string(REPLACE "/W3" "/W0" pv_tpl_cxx_flags "${pv_tpl_cxx_flags}")
string(REPLACE "/W4" "/W0" pv_tpl_cxx_flags "${pv_tpl_cxx_flags}")
string(REPLACE "/W3" "/W0" pv_tpl_c_flags "${pv_tpl_c_flags}")
string(REPLACE "/W4" "/W0" pv_tpl_c_flags "${pv_tpl_c_flags}")

if(WIN32)
  set(_LINK_LIBRARY_SUFFIX .lib)
elseif(APPLE)
  set(_LINK_LIBRARY_SUFFIX .dylib)
else()
  set(_LINK_LIBRARY_SUFFIX .so)
endif()

# Attempt to detect if we have a Fortran Compiler available.
include(CheckFortran)

#------------------------------------------------------------------------------
# Establish Target Dependencies based on Selected Options
#------------------------------------------------------------------------------

include(PackageVersions)

set(python_dependencies)
set(CLAPACK_dependencies)
set(NUMPY_dependencies)
set(Manta_dependencies)
set(OpenMPI_dependencies)
set(ParaView_dependencies)


set(MPI_ARGS)

if(NOT APPLE)

  if(WIN32)
    include(External_OpenMPI)
    list(APPEND ParaView_dependencies OpenMPI)
  else()
    include(External_MPICH2)
    list(APPEND ParaView_dependencies MPICH2)
  endif()

  set(MPI_ARGS
    -DPARAVIEW_USE_MPI:BOOL=ON
    -DMPIEXEC:FILEPATH=${MPIEXEC}
    -DMPI_INCLUDE_PATH:PATH=${MPI_INCLUDE_PATH}
    -DMPI_LIBRARY:FILEPATH=${MPI_LIBRARY}
    -DMPI_EXTRA_LIBRARY:FILEPATH=${MPI_EXTRA_LIBRARY}
    -DPARAVIEW_INSTALL_MPIEXEC:BOOL=ON)

  if(NOT WIN32)
    set(MPI_COMPILER_ARGS
       -DCMAKE_C_COMPILER:FILEPTH=${MPI_C_COMPILER}
       -DCMAKE_CXX_COMPILER:FILEPATH=${MPI_CXX_COMPILER})
    list(APPEND MPI_ARGS ${MPI_COMPILER_ARGS})
  endif()

  include(External_Python)
  list(APPEND ParaView_depenencies Python)

  set(NUMPY_dependencies python)

  if(CMAKE_Fortran_COMPILER)
    include(External_LAPACK)
    list(APPEND NUMPY_dependencies LAPACK)
    list(APPEND ParaView_dependencies LAPACK)
  else()
    include(External_CLAPACK)
    list(APPEND NUMPY_dependencies CLAPACK)
    list(APPEND ParaView_dependencies CLAPACK)
  endif()

  include(External_NUMPY)

  if(NOT WIN32)
    option(PARAVIEW_BUILD_PLUGIN_VisTrailsPlugin "Build Depenednecies for VisTrails" FALSE)
    mark_as_advanced(PARAVIEW_BUILD_PLUGIN_VisTrailsPlugin)
    if(PARAVIEW_BUILD_PLUGIN_VisTrailsPlugin)
      # Build SIP and PyQt for vistrails plugin.
      include(External_SIP)
      list(APPEND PyQt_dependencies SIP)
      include(External_PyQt)
      list(APPEND ParaView_dependencies PyQt)

      set(VISTRAILS_ARGS
        -DPARAVIEW_BUILD_PLUGIN_VisTrailsPlugin:BOOL=ON)
    endif()
  endif()
else()

  find_package(PythonInterp)
  find_package(PythonLibs)

  if(APPLE AND "${CMAKE_OSX_DEPLOYMENT_TARGET}" STREQUAL "10.5")
    set(PYTHON_EXECUTABLE "/System/Library/Frameworks/Python.framework/Versions/2.5/bin/python" CACHE FILEPATH "" FORCE)
    set(PYTHON_LIBRARY "/System/Library/Frameworks/Python.framework/Versions/2.5/Python" CACHE PATH "" FORCE)
    set(PYTHON_INCLUDE_DIR "/System/Library/Frameworks/Python.framework/Versions/2.5/include/python2.5" CACHE FILEPATH "" FORCE)
  endif()

  find_package(MPI)
  set(MPI_ARGS
    -DPARAVIEW_USE_MPI:BOOL=ON
    -DMPIEXEC:FILEPATH=${MPIEXEC}
    -DMPI_INCLUDE_PATH:PATH=${MPI_INCLUDE_PATH}
    -DMPI_LIBRARY:FILEPATH=${MPI_LIBRARY}
    -DMPI_EXTRA_LIBRARY:FILEPATH=${MPI_EXTRA_LIBRARY}
    -DPARAVIEW_INSTALL_MPIEXEC:BOOL=OFF)

  include(External_NUMPY)
endif()

set(MANTA_ARGS)
# Manta doesn't compile on Windows x64 currently
if(NOT (WIN32 AND "${CMAKE_SIZEOF_VOID_P}" EQUAL 8))
  include(External_Manta)
  set(MANTA_ARGS
    -DPARAVIEW_BUILD_PLUGIN_Manta:BOOL=ON
    -DMANTA_BUILD:PATH=${Manta_binary}
    -DMANTA_SOURCE:PATH=${Manta_source})
  list(APPEND ParaView_dependencies Manta)
endif()

include(External_Boost)
list(APPEND ParaView_dependencies Boost)

include(External_zlib)
list(APPEND ParaView_dependencies zlib)
list(APPEND png_dependencies zlib)

include(External_png)
list(APPEND ParaView_dependencies png)

if(NOT WIN32 AND NOT APPLE)
  #include(External_Silo)
  #list(APPEND ParaView_dependencies Silo)

  include(External_FFMPEG)
  set(FFMPEG_ARGS
    -DVTK_USE_FFMPEG_ENCODER:BOOL=ON
    -DFFMPEG_INCLUDE_DIR:PATH=${FFMPEG_INCLUDE_DIR}
    -DFFMPEG_avcodec_LIBRARY:FILEPATH=${FFMPEG_avcodec_LIBRARY}
    -DFFMPEG_avformat_LIBRARY:FILEPATH=${FFMPEG_avformat_LIBRARY}
    -DFFMPEG_avutil_LIBRARY:FILEPATH=${FFMPEG_avutil_LIBRARY}
    -DFFMPEG_swscale_LIBRARY:FILEPATH=${FFMPEG_swscale_LIBRARY})
  list(APPEND ParaView_dependencies FFMPEG)
endif()

option(ParaView_BUILD_DOCUMENTATION "Build ParaView Documentation" FALSE)
set(DOC_ARGS)
if(ParaView_BUILD_DOCUMENTATION)
  set(DOC_ARGS
    -DBUILD_DOCUMENTATION:BOOL=ON
    -DPARAVIEW_GENERATE_PROXY_DOCUMENTATION:BOOL=ON
    -DGENERATE_FILTERS_DOCUMENTATION:BOOL=ON
    -DDOCUMENTATION_HTML_HELP:BOOL=ON
  )
endif()

#------------------------------------------------------------------------------
# Configure and build ParaView
#------------------------------------------------------------------------------

set(proj ParaView)
ExternalProject_Add(${proj}
  DEPENDS ${ParaView_dependencies}
  DOWNLOAD_COMMAND ""
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/ParaView-build
  CMAKE_CACHE_ARGS
    ${pv_compiler_args}
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DBUILD_TESTING:BOOL=OFF
    -DPARAVIEW_DISABLE_VTK_TESTING:BOOL=ON
    -DPARAVIEW_TESTING_WITH_PYTHON:BOOL=OFF
    # Boost
    -DVTK_USE_BOOST:BOOL=ON
    -DBoost_INCLUDE_DIR:PATH=${Boost_INCLUDE_DIR}
    # Python
    -DPARAVIEW_ENABLE_PYTHON:BOOL=ON
    -DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
    -DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIR}
    -DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}
    # Qt
    -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
    # MPI
    ${MPI_ARGS}
    # Manta
    ${MANTA_ARGS}
    # FFMPEG
    ${FFMPEG_ARGS}
    # EyeDome
    -DPARAVIEW_BUILD_PLUGIN_EyeDomeLighting:BOOL=ON
    # VisIt
    -DPARAVIEW_USE_VISITBRIDGE:BOOL=ON
    #-DVISIT_BUILD_READER_CGNS:BOOL=ON
    #-DCGNS_INCLUDE_DIR:PATH=
    #-DCGNS_LIBRARY:FILEPATH=
    #-DVISIT_BUILD_READER_Silo:BOOL=ON
    #-DSILO_INCLUDE_DIR:PATH=
    #-DSILO_LIBRARY:FILEPATH=
    # Documentation
    # VisTrails
    ${VISTRAILS_ARGS}
    ${DOC_ARGS}
  INSTALL_COMMAND ""
  )
