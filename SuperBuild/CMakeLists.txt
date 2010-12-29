
#-----------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.3)
#-----------------------------------------------------------------------------

project(ParaViewSuperBuild)

set(ParaViewSuperBuild_CMAKE_SOURCE_DIR ${ParaViewSuperBuild_SOURCE_DIR})
set(ParaViewSuperBuild_CMAKE_BINARY_DIR ${ParaViewSuperBuild_BINARY_DIR})

#-----------------------------------------------------------------------------
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

# ---------------------------------------------------------------------------
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
set(NUMPY_dependencies python)
set(Manta_dependencies)
set(OpenMPI_dependencies)

set(ParaView_dependencies python CLAPACK NUMPY OpenMPI Manta)

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

include(PackageVersions)

if(NOT APPLE)
  include(External_Python)

  if(CMAKE_Fortran_COMPILER)
    include(External_LAPACK)
    list(APPEND NUMPY_dependencies LAPACK)
  else()
    include(External_CLAPACK)
    list(APPEND NUMPY_dependencies CLAPACK)
  endif()

endif()
include(External_NUMPY)
include(External_OpenMPI)
include(External_Manta)


#-----------------------------------------------------------------------------
# Configure and build ParaView
#------------------------------------------------------------------------------

#set(proj ParaView)
#ExternalProject_Add(${proj}
#  DEPENDS ${ParaView_dependencies}
#  DOWNLOAD_COMMAND ""
#  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..
#  BINARY_DIR ParaView-build
#  CMAKE_GENERATOR ${gen}
#  CMAKE_ARGS
#    ${slicer_superbuild_boolean_args}
#    -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
#    -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
#    -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
#    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    # Python
#    -DSlicer_USE_SYSTEM_PYTHON:BOOL=OFF
#    -DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
#    -DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE}
#    -DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}
    # Qt
#    -DQT_QMAKE_EXECUTABLE:PATH=${QT_QMAKE_EXECUTABLE}
#  INSTALL_COMMAND ""
#  )
