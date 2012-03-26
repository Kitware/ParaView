
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

  if(APPLE)
    set(paraview_osx_flags "-arch x86_64 -mmacosx-version-min=@CMAKE_OSX_DEPLOYMENT_TARGET@ -isysroot @CMAKE_OSX_SYSROOT@")
  endif()
endif()

project(ParaViewSuperBuild)

set(ParaViewSuperBuild_CMAKE_SOURCE_DIR ${ParaViewSuperBuild_SOURCE_DIR})
set(ParaViewSuperBuild_CMAKE_BINARY_DIR ${ParaViewSuperBuild_BINARY_DIR}/CMake)

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
set(ENABLE_YT OFF CACHE BOOL "Install yt for astrophysics analysis")

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


#if(WIN32 OR APPLE)
  set(pv_cxx_flags ${CMAKE_CXX_FLAGS})
  set(pv_c_flags ${CMAKE_C_FLAGS})
#elseif()
#  set(pv_cxx_flags "${CMAKE_CXX_FLAGS} -L${ParaViewSuperBuild_BINARY_DIR}/lib -I${ParaViewSuperBuild_BINARY_DIR}/include")
#  set(pv_c_flags "${CMAKE_C_FLAGS} -L${ParaViewSuperBuild_BINARY_DIR}/lib -I${ParaViewSuperBuild_BINARY_DIR}/include")
#endif()

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
string(REPLACE " /W3" "" pv_tpl_cxx_flags "${pv_tpl_cxx_flags}")
string(REPLACE " /W4" "" pv_tpl_cxx_flags "${pv_tpl_cxx_flags}")
string(REPLACE " /W3" "" pv_tpl_c_flags "${pv_tpl_c_flags}")
string(REPLACE " /W4" "" pv_tpl_c_flags "${pv_tpl_c_flags}")

if(WIN32)
  set(pv_tpl_c_flags "/w ${pv_tpl_c_flags}")
  set(pv_tpl_cxx_flags "/w ${pv_tpl_cxx_flags}")
else()
  set(pv_tpl_c_flags "-w ${pv_tpl_c_flags}")
  set(pv_tpl_cxx_flags "-w ${pv_tpl_cxx_flags}")
endif()

if(WIN32)
  set(_LINK_LIBRARY_SUFFIX .lib)
elseif(APPLE)
  set(_LINK_LIBRARY_SUFFIX .dylib)
else()
  set(_LINK_LIBRARY_SUFFIX .so)
endif()

# Attempt to detect if we have a Fortran Compiler available.
include(CheckFortran)

if(CMAKE_Fortran_COMPILER)
  enable_language(Fortran)
endif()

#------------------------------------------------------------------------------
configure_file(${ParaViewSuperBuild_SOURCE_DIR}/CTestCustom.ctest.in
  ${ParaViewSuperBuild_BINARY_DIR}/CTestCustom.ctest @ONLY)

#------------------------------------------------------------------------------
# Configure Common environment files
#------------------------------------------------------------------------------

configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/paraview_common_environment.cmake.in
  ${ParaViewSuperBuild_CMAKE_BINARY_DIR}/paraview_common_environment.cmake @ONLY)

configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/paraview_configure_step.cmake.in
  ${ParaViewSuperBuild_CMAKE_BINARY_DIR}/paraview_configure_step.cmake @ONLY)

configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/paraview_make_step.cmake.in
  ${ParaViewSuperBuild_CMAKE_BINARY_DIR}/paraview_make_step.cmake @ONLY)

configure_file(${ParaViewSuperBuild_CMAKE_SOURCE_DIR}/paraview_install_step.cmake.in
  ${ParaViewSuperBuild_CMAKE_BINARY_DIR}/paraview_install_step.cmake @ONLY)

#------------------------------------------------------------------------------
# Establish Target Dependencies based on Selected Options
#------------------------------------------------------------------------------

include(PackageVersions)

set(python_dependencies)
set(CLAPACK_dependencies)
set(NUMPY_dependencies)
set(MPL_dependencies)
set(Cython_dependencies)
set(h5py_dependencies)
set(Manta_dependencies)
set(OpenMPI_dependencies)
set(ParaView_dependencies)

set(PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS)
set(PARAVIEW_EXTERNAL_PLUGIN_DIRS)
set(MPI_ARGS)

# Build a common zlib used by several dependencies
include(External_zlib)
set(ZLIB_ARGS
   -DVTK_USE_SYSTEM_ZLIB:BOOL=ON
   -DZLIB_INCLUDE_DIR:PATH=${ZLIB_INCLUDE_DIR}
   -DZLIB_LIBRARY:FILEPATH=${ZLIB_LIBRARY})
list(APPEND ParaView_dependencies zlib)
list(APPEND png_dependencies zlib)
list(APPEND python_dependencies zlib)

# Build a common szip used by several dependencies
include(External_szip)
#set(SZIP_ARGS
#   -DVTK_USE_SYSTEM_ZLIB:BOOL=ON
#   -DZLIB_INCLUDE_DIR:PATH=${ZLIB_INCLUDE_DIR}
#   -DZLIB_LIBRARY:FILEPATH=${ZLIB_LIBRARY})

if(WIN32 OR APPLE)
  # Build a common png used by several dependencies
  include(External_png)
  # can't get paraview to build SMExtractDocumentation with externally built png
  # until then ParaView will still use the system png.
  set(PNG_ARGS
    -DVTK_USE_SYSTEM_PNG:BOOL=ON
    -DPNG_PNG_INCLUDE_DIR:PATH=${PNG_INCLUDE_DIR}
    -DPNG_LIBRARY:FILEPATH=${PNG_LIBRARY})
  list(APPEND ParaView_dependencies png)
endif()

if(WIN32)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${png_install}/bin)
else()
  get_filename_component(PNG_DIR "${PNG_LIBRARY}" PATH)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${PNG_DIR})
endif()

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
    -DMPI_C_COMPILER:FILEPATH=${MPICC}
    -DMPI_CXX_COMPILER:FILEPATH=${MPICXX}
    -DVTK_MPIRUN_EXE:FILEPATH=${MPIEXEC}
    -DMPI_INCLUDE_PATH:PATH=${MPI_INCLUDE_PATH}
    -DMPI_LIBRARY:FILEPATH=${MPI_LIBRARY}
    -DMPI_EXTRA_LIBRARY:FILEPATH=${MPI_EXTRA_LIBRARY}
    -DPARAVIEW_INSTALL_MPIEXEC:BOOL=ON)

  include(External_Python)
  list(APPEND ParaView_depenencies Python)

  set(NUMPY_dependencies python)

  if(CMAKE_Fortran_COMPILER AND NOT WIN32)
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

  find_package(PythonInterp)
  find_package(PythonLibs)

  if(APPLE AND "${CMAKE_OSX_DEPLOYMENT_TARGET}" STREQUAL "10.5")
    set(PYTHON_EXECUTABLE "/System/Library/Frameworks/Python.framework/Versions/2.5/bin/python" CACHE FILEPATH "" FORCE)
    set(PYTHON_LIBRARY "/System/Library/Frameworks/Python.framework/Versions/2.5/Python" CACHE PATH "" FORCE)
    set(PYTHON_INCLUDE_DIR "/System/Library/Frameworks/Python.framework/Versions/2.5/include/python2.5" CACHE FILEPATH "" FORCE)
  endif()

  #find_package(MPI)
  #set(MPI_ARGS
  #  -DPARAVIEW_USE_MPI:BOOL=OFF
  #  -DMPI_DIR:PATH=${MPI_INSTALL}
  #  -DMPIEXEC:FILEPATH=${MPIEXEC}
  #  -DVTK_MPIRUN_EXE:FILEPATH=${MPIEXEC}
  #  -DMPI_INCLUDE_PATH:PATH=${MPI_INCLUDE_PATH}
  #  -DMPI_LIBRARY:FILEPATH=${MPI_LIBRARY}
  #  -DMPI_EXTRA_LIBRARY:FILEPATH=${MPI_EXTRA_LIBRARY}
  #  -DPARAVIEW_INSTALL_MPIEXEC:BOOL=ON)

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

if(WIN32)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${zlib_install}/bin)
else()
  get_filename_component(ZLIB_DIR "${ZLIB_LIBRARY}" PATH)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${ZLIB_DIR})
endif()

# Build HDF5
list(APPEND HDF5_dependencies zlib szip)
list(APPEND ParaView_dependencies HDF5)
include(External_HDF5)
set(HDF5_ARGS
  -DVTK_USE_SYSTEM_HDF5:BOOL=ON
  -DHDF5_DIR:PATH=${HDF5_install}
  -DHDF5_FOUND:BOOL=ON
  -DHDF5_INCLUDE_DIRS:PATH=${HDF5_INCLUDE_DIR}
  -DHDF5_INCLUDE_DIR:PATH=${HDF5_INCLUDE_DIR}
  -DHDF5_LIBRARIES:FILEPATH=${HDF5_LIBRARY};${HDF5_HL_LIBRARY}
  -DHDF5_LIBRARY:FILEPATH=${HDF5_LIBRARY}
  -DHDF5_HL_LIBRARY:FILEPATH=${HDF5_HL_LIBRARY}
  -DHDF5_hdf5_LIBRARY:FILEPATH=${HDF5_LIBRARY}
  -DHDF5_hdf5_LIBRARY_RELEASE:FILEPATH=${HDF5_LIBRARY}
  -DHDF5_hdf5_hl_LIBRARY:FILEPATH=${HDF5_HL_LIBRARY}
  -DHDF5_hdf5_hl_LIBRARY_RELEASE:FILEPATH=${HDF5_HL_LIBRARY})

if(WIN32)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${HDF5_install}/bin)
else()
  get_filename_component(HDF5_DIR "${HDF5_LIBRARY}" PATH)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${HDF5_DIR})
endif()

if(WIN32 OR APPLE)
  set(freetype_dependencies zlib)
  list(APPEND ParaView_dependencies freetype)
  include(External_freetype)
  set(freetype_ARGS
    -DVKT_USE_SYSTEM_FREETYPE:BOOL=ON
    -DFREETYPE_INCLUDE_DIR_freetype2:PATH=${FT_INCLUDE_DIR}/freetype
    -DFREETYPE_INCLUDE_DIR_ft2build:PATH=${FT_INCLUDE_DIR}
    -DFREETYPE_LIBRARY:FILEPATH=${FT_LIBRARY}
    )

  if(WIN32)
    list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${freetype_install}/bin)
  else()
    get_filename_component(FT_DIR "${FT_LIBRARY}" PATH)
    list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${FT_DIR})
  endif()
endif()

if(ENABLE_YT)
  set(MPL_dependencies NUMPY zlib)
  if(WIN32 OR APPLE)
    list(APPEND MPL_dependencies freetype png)
  endif()
  include(External_MPL)

  set(Cython_dependencies NUMPY)
  include(External_Cython)

  set(h5py_dependencies NUMPY HDF5 Cython)
  include(External_h5py)

  set(yt_dependencies NUMPY HDF5 MPL h5py Cython)
  if(NOT APPLE)
    set(distribute_dependencies python)
    include(External_distribute)
    list(APPEND yt_dependencies distribute)
  endif()

  include(External_yt)
  list(APPEND ParaView_dependencies yt)
endif()

# Build Silo
option(PARAVIEW_BUILD_Silo "Build Silo library" ON)
if(PARAVIEW_BUILD_Silo)
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
endif()

# Build CGNS
option(PARAVIEW_BUILD_CGNS "Build CGNS library" ON)
if(PARAVIEW_BUILD_CGNS)
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

# Build VRPN
list(APPEND ParaView_dependencies VRPN)
include(External_VRPN)
set(VRPN_ARGS
  -DPARAVIEW_BUILD_PLUGIN_VRPNPlugin:BOOL=ON
  -DVRPN_LIBRARY:FILEPATH=${VRPN_LIBRARY}
  -DVRPN_INCLUDE_DIR:PATH=${VRPN_INCLUDE_DIR})

if(WIN32)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${VRPN_install}/lib)
else()
  get_filename_component(VRPN_DIR "${VRPN_LIBRARY}" PATH)
  list(APPEND PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS ${VRPN_DIR})
endif()

# Build AcuSolveReaderPlugin
if(NOT APPLE)
  option(PARAVIEW_BUILD_PLUGIN_AcuSolveReader "Build AcuSolveReader Plugin" FALSE)
  set(AcuSolve_ARGS)
  if(PARAVIEW_BUILD_PLUGIN_AcuSolveReader)
    list(APPEND ParaView_dependencies AcuSolveReaderPlugin)
    include(External_AcuSolveReaderPlugin)
    set(AcuSolve_ARGS
      -DPARAVIEW_BUILD_PLUGIN_AcuSolveReader:BOOL=ON
    )
    list(APPEND PARAVIEW_EXTERNAL_PLUGIN_DIRS ${AcuSolveReaderPlugin_source})
  else()
    set(AcuSolve_ARGS
      -DPARAVIEW_BUILD_PLUGIN_AcuSolveReader:BOOL=OFF
    )
  endif()
endif()

if(NOT APPLE AND CMAKE_Fortran_COMPILER)
  option(PARAVIEW_BUILD_PLUGIN_Nektar "Build Nektar Plugin" FALSE)
  if(PARAVIEW_BUILD_PLUGIN_Nektar)
    set(Nektar_ARGS
      -DPARAVIEW_BUILD_PLUGIN_Nektar:BOOL=ON
    )
  else()
    set(Nektar_ARGS
      -DPARAVIEW_BUILD_PLUGIN_Nektar:BOOL=OFF
    )
  endif()
endif()

# Build VisTrailsPlugin
option(PARAVIEW_BUILD_PLUGIN_VisTrails "Build VisTrails Plugin" FALSE)
set(VisTrails_ARGS)
if(PARAVIEW_BUILD_PLUGIN_VisTrails)
  include(External_VisTrails)
  list(APPEND ParaView_dependencies VisTrails)
  set(VisTrails_ARGS
    -DPARAVIEW_BUILD_PLUGIN_VisTrails:BOOL=ON
  )
  list(APPEND PARAVIEW_EXTERNAL_PLUGIN_DIRS ${VisTrails_source})
  # Build SIP and PyQt for vistrails plugin.
  #set(SIP_dependencies python)
  #include(External_SIP)
  #list(APPEND PyQt_dependencies SIP)
  #set(PyQt_dependencies python SIP)
  #include(External_PyQt)
  #list(APPEND ParaView_dependencies PyQt)

else()
  set(VisTrails_ARGS
    -DPARAVIEW_BUILD_PLUGIN_VisTrails:BOOL=OFF
  )
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

set(PARAVIEW_EXTRA_INSTALL_RULES)

# Install libbz2 until we start building it ourselves.
if(UNIX AND NOT APPLE)
  list(APPEND PARAVIEW_EXTRA_INSTALL_RULES ${CMAKE_CURRENT_SOURCE_DIR}/install_bz2.cmake)
endif()

# Install FORTRAN runtime
if(CMAKE_Fortran_COMPILER AND NOT WIN32)
  get_filename_component(fortran_compiler ${CMAKE_Fortran_COMPILER} NAME_WE)
  if("${fortran_compiler}" STREQUAL "gfortran")
    set(FORTRAN_INSTALL_RULE_FILE
      ${CMAKE_CURRENT_SOURCE_DIR}/install_gfortran.cmake)
  elseif("${fortran_compiler}" STREQUAL "g77")
    set(FORTRAN_INSTALL_RULE_FILE
      ${CMAKE_CURRENT_SOURCE_DIR}/install_g2c.cmake)
  elseif("${fortran_compiler}" STREQUAL "ifort")
    #set(FORTRAN_INSTALL_RULE_FILE
    #  ${CMAKE_CURRENT_SOURCE_DIR}/install_g2c.cmake)
  endif()
  list(APPEND PARAVIEW_EXTRA_INSTALL_RULES ${FORTRAN_INSTALL_RULE_FILE})
endif()

list(REMOVE_DUPLICATES PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS)

#------------------------------------------------------------------------------
# Configure and build ParaView
#------------------------------------------------------------------------------

ExternalProject_Add(ParaView
  DEPENDS ${ParaView_dependencies}
  DOWNLOAD_COMMAND ""
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/ParaView-build
  CMAKE_CACHE_ARGS
    ${pv_compiler_args}
    -DPARAVIEW_EXTRA_INSTALL_RULES_FILE:FILEPATH=${PARAVIEW_EXTRA_INSTALL_RULES}
    -DCMAKE_CXX_FLAGS:STRING=${pv_cxx_flags}
    -DCMAKE_C_FLAGS:STRING=${pv_c_flags}
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DBUILD_TESTING:BOOL=OFF
    -DPARAVIEW_DISABLE_VTK_TESTING:BOOL=ON
    -DPARAVIEW_TESTING_WITH_PYTHON:BOOL=OFF
    -DPARAVIEW_INSTALL_THIRD_PARTY_LIBRARIES:BOOL=ON
    -DPARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS:STRING=${PARAVIEW_FIXUP_BUNDLE_SEARCH_PATHS}
    # BLAS/LAPACK
    -DBLAS_blas_LIBRARY:FILEPATH=${BLAS_LIBRARY}
    -DLAPACK_lapack_LIBRARY:FILEPATH=${LAPACK_LIBRARY}
    -DF2C_LIBRARY:FILEPATH=${F2C_LIBRARY}
    -DPARAVIEW_EXTERNAL_PLUGIN_DIRS:STRING=${PARAVIEW_EXTERNAL_PLUGIN_DIRS}
    # Boost
    -DVTK_USE_BOOST:BOOL=ON
    -DBoost_INCLUDE_DIR:PATH=${Boost_INCLUDE_DIR}
    -DBOOST_LIBRARYDIR:PATH=${BOOST_LIBRARYDIR}
    -DBOOST_ROOT:PATH=${CMAKE_CURRENT_BINARY_DIR}
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
    ${VisTrails_ARGS}
    # VRPN Plugin
    ${VRPN_ARGS}
    # zlib
    ${ZLIB_ARGS}
    -DPARAVIEW_BUILD_PLUGIN_CoProcessingScriptGenerator:BOOL=ON
    -DPARAVIEW_BUILD_PLUGIN_AMR:BOOL=OFF
    ${AcuSolve_ARGS}
    ${Nektar_ARGS}
    # Documentation
    ${DOC_ARGS}
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ParaViewSuperBuild_SOURCE_DIR}/.plugins <BINARY_DIR>/bin/
  )

#-----------------------------------------------------------------------------
# Configure testing support.
include(CTest)
if(BUILD_TESTING)
  enable_testing()

  set(Testing_Temporary_dir ${ParaViewSuperBuild_BINARY_DIR}/Testing/Temporary)
  make_directory(${Testing_Temporary_dir})
  
  ExternalProject_Get_Property(ParaView binary_dir)
  set(ParaView_BINARY_DIR "${binary_dir}")
  
  include(${ParaViewSuperBuild_SOURCE_DIR}/../CMake/ParaViewDetermineVersion.cmake)
  
  find_package(Git)
  
  determine_version(
    ${ParaViewSuperBuild_SOURCE_DIR}/..
    ${GIT_EXECUTABLE}
    ${ParaViewSuperBuild_SOURCE_DIR}/../CMake/.paraview.version
    "PARAVIEW")

  # Copied from Applications\ParaView\CMakeLists.txt
  set(_paraview_version_patch_full "${PARAVIEW_VERSION_PATCH}")
  if(NOT "${PARAVIEW_VERSION_PATCH_EXTRA}" STREQUAL "")
    set(_paraview_version_patch_full 
      "${PARAVIEW_VERSION_PATCH}-${PARAVIEW_VERSION_PATCH_EXTRA}"
    )
  endif(NOT "${PARAVIEW_VERSION_PATCH_EXTRA}" STREQUAL "")

  set(PARAVIEW_CPACK_VERSION
    ${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}.${_paraview_version_patch_full}
  )

  # Copied from ParaViewBrandingCPack.cmake - Should put in own file/function
  IF (CMAKE_SYSTEM_PROCESSOR MATCHES "unknown")
    SET (CMAKE_SYSTEM_PROCESSOR "x86")
  ENDIF (CMAKE_SYSTEM_PROCESSOR MATCHES "unknown")
  IF(NOT DEFINED CPACK_SYSTEM_NAME)
    SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
  ENDIF(NOT DEFINED CPACK_SYSTEM_NAME)
  IF(${CPACK_SYSTEM_NAME} MATCHES Windows)
    IF(CMAKE_CL_64)
      SET(CPACK_SYSTEM_NAME Win64-${CMAKE_SYSTEM_PROCESSOR})
    ELSE(CMAKE_CL_64)
      SET(CPACK_SYSTEM_NAME Win32-${CMAKE_SYSTEM_PROCESSOR})
    ENDIF(CMAKE_CL_64)
  ENDIF(${CPACK_SYSTEM_NAME} MATCHES Windows)

  if(${CPACK_SYSTEM_NAME} MATCHES Darwin AND CMAKE_OSX_ARCHITECTURES)
    list(LENGTH CMAKE_OSX_ARCHITECTURES _length)
    IF(_length GREATER 1)
      SET(CPACK_SYSTEM_NAME Darwin-Universal)
    ELSE(_length GREATER 1)
      SET(CPACK_SYSTEM_NAME Darwin-${CMAKE_OSX_ARCHITECTURES})
    ENDIF(_length GREATER 1)
  endif(${CPACK_SYSTEM_NAME} MATCHES Darwin AND CMAKE_OSX_ARCHITECTURES)

  set(CPACK_PACKAGE_FILE_NAME ParaView-${PARAVIEW_CPACK_VERSION}-${CPACK_SYSTEM_NAME})
  
  find_path(PARAVIEW_DATA_ROOT ParaViewData.readme
    ${ParaView_SOURCE_DIR}/ParaViewData
    ${ParaView_SOURCE_DIR}/../ParaViewData
    $ENV{PARAVIEW_DATA_ROOT}
  )

  add_subdirectory(Testing)
endif()
