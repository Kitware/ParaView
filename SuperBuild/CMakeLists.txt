
#-----------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.3)
#-----------------------------------------------------------------------------

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
  message(FATAL_ERROR "error: Qt4 was not found on your system. Please set QT_QMAKE_EXECUTABLE")
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

# We aren't interested in seeing third party compiler warnings.
set(pv_tpl_compiler_args ${pv_compiler_args})
string(REPLACE "/W3" "/W0" pv_tpl_cxx_flags "${pv_tpl_cxx_flags}")
string(REPLACE "/W4" "/W0" pv_tpl_cxx_flags "${pv_tpl_cxx_flags}")
string(REPLACE "/W3" "/W0" pv_tpl_c_flags "${pv_tpl_c_flags}")
string(REPLACE "/W4" "/W0" pv_tpl_c_flags "${pv_tpl_c_flags}")

# Attempt to detect if we have a Fortran Compiler available.
include(CheckFortran)

#------------------------------------------------------------------------------
# Establish Target Dependencies based on Selected Options
#------------------------------------------------------------------------------

set(python_dependencies)
set(CLAPACK_dependencies)
set(NUMPY_dependencies)
set(Manta_dependencies)
set(OpenMPI_dependencies)

set(ParaView_dependencies)

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

include(PackageVersions)

if(NOT APPLE)
  include(External_OpenMPI)
  list(APPEND ParaView_dependencies OpenMPI)

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
else()

  #find_package(LAPACK)
  find_package(PythonInterp)
  find_package(PythonLibs)
  find_package(MPI)

  include(External_NUMPY)
endif()






include(External_Manta)
set(MANTA_ARGS
 -DPARAVIEW_BUILD_PLUGIN_Manta:BOOL=ON
 -DMANTA_BUILD:PATH=${Manta_binary}
 -DMANTA_SOURCE:PATH=${Manta_source})
list(APPEND ParaView_dependencies Manta)

if(NOT WIN32)
  #include(External_Silo)
  #list(APPEND ParaView_dependencies Silo)

  #include(External_FFMPEG)
  #list(APPEND ParaView_dependencies FFMPEG)
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
   -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
   -DBUILD_TESTING:BOOL=OFF
   -DPARAVIEW_DISABLE_VTK_TESTING:BOOL=ON
   -DPARAVIEW_TESTING_WITH_PYTHON:BOOL=OFF
   -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
   -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
   -DCMAKE_BUILD_TYPE:STRING=${CMAKE_CFG_INTDIR}
   # Python
   -DPARAVIEW_ENABLE_PYTHON:BOOL=ON
   -DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
   -DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIR}
   -DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}
   # Qt
   -DQT_QMAKE_EXECUTABLE:PATH=${QT_QMAKE_EXECUTABLE}
   # MPI
   -DPARAVIEW_USE_MPI:BOOL=ON
   -DMPIEXEC:FILEPATH=${MPIEXEC}
   -DMPI_INCLUDE_PATH:PATH=${MPI_INCLUDE_PATH}
   -DMPI_LIBRARY:FILEPATH=${MPI_LIBRARY}
   -DMPI_EXTRA_LIBRARY:FILEPATH=${MPI_EXTRA_LIBRARY}
   # Manta
   ${MANTA_ARGS}
   # EyeDome
   -DPARAVIEW_BUILD_PLUGIN_EyeDome:BOOL=ON
  INSTALL_COMMAND ""
  )
