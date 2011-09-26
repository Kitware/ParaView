
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
    set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.6.sdk" CACHE PATH "force build for 64-bit Leopard" FORCE)
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

include(ExternalProject)

#-----------------------------------------------------------------------------
# Git protocol option
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
CONFIGURE_FILE(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/CTestCustom.ctest.in
  ${ParaViewSuperBuild_CMAKE_BINARY_DIR}/CTestCustom.ctest @ONLY)

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

set(PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS)

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
    -DMPI_DIR:PATH=${MPI_INSTALL}
    -DPARAVIEW_USE_MPI:BOOL=ON
    -DMPIEXEC:FILEPATH=${MPIEXEC}
    -DVTK_MPIRUN_EXE:FILEPATH=${MPIEXEC}
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

  if(CMAKE_Fortran_COMPILER AND NOT WIN32 AND NOT UNIX)
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
    option(PARAVIEW_BUILD_PLUGIN_VisTrailsPlugin "Build Dependencies for VisTrails" FALSE)
    mark_as_advanced(PARAVIEW_BUILD_PLUGIN_VisTrailsPlugin)
    if(PARAVIEW_BUILD_PLUGIN_VisTrailsPlugin)
      # Build SIP and PyQt for vistrails plugin.
      set(SIP_dependencies python)
      include(External_SIP)
      list(APPEND PyQt_dependencies SIP)
      set(PyQt_dependencies python SIP)
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
    -DMPI_DIR:PATH=${MPI_INSTALL}
    -DMPIEXEC:FILEPATH=${MPIEXEC}
    -DVTK_MPIRUN_EXE:FILEPATH=${MPIEXEC}
    -DMPI_INCLUDE_PATH:PATH=${MPI_INCLUDE_PATH}
    -DMPI_LIBRARY:FILEPATH=${MPI_LIBRARY}
    -DMPI_EXTRA_LIBRARY:FILEPATH=${MPI_EXTRA_LIBRARY}
    -DPARAVIEW_INSTALL_MPIEXEC:BOOL=ON)

  include(External_NUMPY)
endif()

set(MANTA_ARGS)
# Manta doesn't compile on Windows x64 currently
#if(NOT (WIN32 AND "${CMAKE_SIZEOF_VOID_P}" EQUAL 8))
if(NOT WIN32)
  include(External_Manta)
  set(MANTA_ARGS
    -DPARAVIEW_BUILD_PLUGIN_Manta:BOOL=ON
    -DMANTA_BUILD:PATH=${Manta_binary}
    -DMANTA_SOURCE:PATH=${Manta_source})
  list(APPEND ParaView_dependencies Manta)
endif()

include(External_Boost)
list(APPEND ParaView_dependencies Boost)

# Build a common zlib used by several dependencies
include(External_zlib)
set(ZLIB_ARGS
   -DVTK_USE_SYSTEM_ZLIB:BOOL=ON
   -DZLIB_INCLUDE_DIR:PATH=${ZLIB_INCLUDE_DIR}
   -DZLIB_LIBRARY:FILEPATH=${ZLIB_LIBRARY})
list(APPEND ParaView_dependencies zlib)
list(APPEND png_dependencies zlib)

if(WIN32)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${zlib_install}/bin)
else()
  get_filename_component(ZLIB_DIR "${ZLIB_LIBRARY}" PATH)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${ZLIB_DIR})
endif()

if(NOT APPLE)
  # Build a common png used by several dependencies
  include(External_png)
  set(PNG_ARGS
    -DVTK_USE_SYSTEM_PNG:BOOL=OFF
    -DPNG_PNG_INCLUDE_DIR:PATH=${PNG_INCLUDE_DIR}
    -DPNG_LIBRARY:FILEPATH=${PNG_LIBRARY})
  list(APPEND ParaView_dependencies png)

  if(WIN32)
    list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${png_install}/bin)
  else()
    get_filename_component(PNG_DIR "${PNG_LIBRARY}" PATH)
    list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${PNG_DIR})
  endif()
endif()

# Build HDF5
list(APPEND HDF5_dependencies zlib)
list(APPEND ParaView_dependencies HDF5)
include(External_HDF5)
set(HDF5_ARGS
  -DVTK_USE_SYSTEM_HDF5:BOOL=ON
  -DHDF5_DIR:PATH=${HDF5_install}
  -DHDF5_FOUND:BOOL=ON
  -DHDF5_INCLUDE_DIRS:PATH=${HDF5_INCLUDE_DIR}
  -DHDF5_INCLUDE_DIR:PATH=${HDF5_INCLUDE_DIR}
  -DHDF5_LIBRARIES:FILEPATH=${HDF5_LIBRARY}
  -DHDF5_LIBRARY:FILEPATH=${HDF5_LIBRARY}
  -DHDF5_hdf5_LIBRARY:FILEPATH=${HDF5_LIBRARY}
  -DHDF5_hdf5_LIBRARY_RELEASE:FILEPATH=${HDF5_LIBRARY})

if(WIN32)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${HDF5_install}/bin)
else()
  get_filename_component(HDF5_DIR "${HDF5_LIBRARY}" PATH)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${HDF5_DIR})
endif()

# Build Silo
list(APPEND Silo_dependencies zlib HDF5)
list(APPEND ParaView_dependencies Silo)
include(External_Silo)
set(Silo_ARGS
  -DVISIT_BUILD_READER_Silo:BOOL=ON
  -DSILO_INCLUDE_DIR:PATH=${SILO_INCLUDE_DIR}
  -DSILO_LIBRARY:FILEPATH=${SILO_LIBRARY})

if(WIN32)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${Silo_install}/bin)
else()
  get_filename_component(SILO_DIR "${SILO_LIBRARY}" PATH)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${SILO_DIR})
endif()

# Build CGNS
list(APPEND CGNS_dependencies zlib HDF5)
list(APPEND ParaView_dependencies CGNS)
include(External_CGNS)
set(CGNS_ARGS
  -DVISIT_BUILD_READER_CGNS:BOOL=ON
  -DCGNS_INCLUDE_DIR:PATH=${CGNS_INCLUDE_DIR}
  -DCGNS_LIBRARY:FILEPATH=${CGNS_LIBRARY})

if(WIN32)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${CGNS_install}/lib)
else()
  get_filename_component(CGNS_DIR "${CGNS_LIBRARY}" PATH)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${CGNS_DIR})
endif()


# Build VRPN
list(APPEND ParaView_dependencies VRPN)
include(External_VRPN)
set(VRPN_ARGS
  -DPARAVIEW_BUILD_PLUGIN_VRPNPlugin:BOOL=ON
  -DVRPN_LIBRARY:FILE_PATH=${VRPN_LIBRARY}
  -DVRPN_INCLUDE_DIR:PATH=${VRPN_INCLUDE_DIR})

if(WIN32)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${VRPN_install}/lib)
else()
  get_filename_component(VRPN_DIR "${VRPN_LIBRARY}" PATH)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${VRPN_DIR})
endif()

if(NOT WIN32)
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
  file(DOWNLOAD ${PARAVIEW_USERS_GUIDE_URL}/${PARAVIEW_USERS_GUIDE_PDF_NO_SPACES} 
      ${CMAKE_CURRENT_BINARY_DIR}/${PARAVIEW_USERS_GUIDE_PDF}  
      EXPECTED_MD5 ${PARAVIEW_USERS_GUIDE_MD5})

  set(DOC_ARGS
    -DBUILD_DOCUMENTATION:BOOL=ON
    -DPARAVIEW_GENERATE_PROXY_DOCUMENTATION:BOOL=ON
    -DGENERATE_FILTERS_DOCUMENTATION:BOOL=ON
    -DDOCUMENTATION_HTML_HELP:BOOL=ON
    -DPARAVIEW_USERS_GUIDE:FILEPATH=${CMAKE_CURRENT_BINARY_DIR}/${PARAVIEW_USERS_GUIDE_PDF}
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
    -DPARAVIEW_EXTRA_INSTALL_RULES_FILE:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/install_gfortran.cmake;${CMAKE_CURRENT_SOURCE_DIR}/install_g2c.cmake
    -DCMAKE_CXX_FLAGS:STRING=${pv_cxx_flags}
    -DCMAKE_C_FLAGS:STRING=${pv_c_flags}
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DBUILD_TESTING:BOOL=OFF
    -DPARAVIEW_DISABLE_VTK_TESTING:BOOL=ON
    -DPARAVIEW_TESTING_WITH_PYTHON:BOOL=OFF
    -DPARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS:STRING=${PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS}
    # Boost
    -DVTK_USE_BOOST:BOOL=ON
    -DBoost_INCLUDE_DIR:PATH=${Boost_INCLUDE_DIR}
    # png
    ${PNG_ARGS}
    # Python
    -DPARAVIEW_ENABLE_PYTHON:BOOL=ON
    -DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
    -DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIR}
    -DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}
    -DPARAVIEW_INSTALL_PYTHON:BOOL=ON
    # Qt
    -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
    # MPI
    ${MPI_ARGS}
    # Manta
    ${MANTA_ARGS}
    # FFMPEG
    ${FFMPEG_ARGS}
    # HDF5
    ${HDF5_ARGS}
    # EyeDome
    -DPARAVIEW_BUILD_PLUGIN_EyeDomeLighting:BOOL=ON
    # VisIt
    -DPARAVIEW_USE_VISITBRIDGE:BOOL=ON
    # VisIt - CGNS
    ${CGNS_ARGS}
    # VisIt - Silo
    ${Silo_ARGS}
    # VisTrails
    ${VISTRAILS_ARGS}
    # VRPN Plugin
    ${VRPN_ARGS}
    # zlib
    ${ZLIB_ARGS}
    # Documentation
    ${DOC_ARGS}
  INSTALL_COMMAND ""
  )
