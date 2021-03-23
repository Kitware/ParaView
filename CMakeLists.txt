#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================
cmake_minimum_required(VERSION 3.12...3.16 FATAL_ERROR)
project(ParaView)

# Disallow in-source build
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
  message(FATAL_ERROR
    "ParaView requires an out-of-source build. Please create a separate "
    "binary directory and run CMake there.")
endif ()

#------------------------------------------------------------------------------
# Set a default build type if none was specified
get_property(generator_is_multi_config GLOBAL
  PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (NOT CMAKE_BUILD_TYPE AND NOT generator_is_multi_config)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE
    PROPERTY
      STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

# Set up our directory structure for output libraries and binaries
include(GNUInstallDirs)
if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
endif ()
if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
endif ()
if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
endif ()

set(CMAKE_INSTALL_LICENSEDIR ""
  CACHE PATH "License files (DATAROOTDIR/licenses/${CMAKE_PROJECT_NAME}")
mark_as_advanced(CMAKE_INSTALL_LICENSEDIR)
if (NOT CMAKE_INSTALL_LICENSEDIR)
  set(CMAKE_INSTALL_LICENSEDIR
    "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${CMAKE_PROJECT_NAME}")
endif ()

set(paraview_cmake_dir "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
set(vtk_cmake_dir "${CMAKE_CURRENT_SOURCE_DIR}/VTK/CMake")
if (NOT IS_DIRECTORY "${vtk_cmake_dir}")
  message(FATAL_ERROR
    "Failed to find the VTK CMake directory. Did you forget to initialize the "
    "submodule? (git submodule update --init --recursive)")
endif ()
list(APPEND CMAKE_MODULE_PATH
  "${paraview_cmake_dir}"
  "${vtk_cmake_dir}")
include(ParaViewServerManager)
include(ParaViewTesting)
include(ParaViewClient)
include(ParaViewPlugin)
include(vtkModuleWrapClientServer)
include(CTest)
set_property(CACHE BUILD_TESTING
  PROPERTY
    TYPE INTERNAL)

include(vtkCompilerChecks)
include(vtkCompilerPlatformFlags)
include(vtkCompilerExtraFlags)
include(vtkInitializeBuildType)
include(vtkSupportMacros)

include(CMakeDependentOption)
include(ParaViewOptions)
include(ParaViewMigrations)

#------------------------------------------------------------------------------
# Choose static or shared libraries.
set(BUILD_SHARED_LIBS "${PARAVIEW_BUILD_SHARED_LIBS}")
set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

#------------------------------------------------------------------------------
# Protect against a stampede of static links at the same time.
if (NOT PARAVIEW_BUILD_SHARED_LIBS)
  set(CMAKE_JOB_POOL_LINK static_exe_link)
  set_property(GLOBAL APPEND PROPERTY
    JOB_POOLS "${CMAKE_JOB_POOL_LINK}=1")
endif ()

#-----------------------------------------------------------------------------
# Does VTK require support for 64 bit file systems
include(CheckCXXSourceRuns)
file(READ "${vtk_cmake_dir}/vtkRequireLargeFilesSupport.cxx"
  VTK_REQUIRE_LARGE_FILE_SUPPORT_FILE)
check_cxx_source_runs("${VTK_REQUIRE_LARGE_FILE_SUPPORT_FILE}"
  CMAKE_REQUIRE_LARGE_FILE_SUPPORT)
set(VTK_REQUIRE_LARGE_FILE_SUPPORT ${CMAKE_REQUIRE_LARGE_FILE_SUPPORT})

#-----------------------------------------------------------------------------
# Provide compatibility options.
set(VTK_LEGACY_REMOVE ${PARAVIEW_BUILD_LEGACY_REMOVE})
set(VTK_LEGACY_SILENT ${PARAVIEW_BUILD_LEGACY_SILENT})

# Setup external data. this will automatically download the test data and
# baseline files to the build tree and set PARAVIEW_DATA_ROOT.
include(ParaViewExternalData)

find_package(Git QUIET)
include(ParaViewDetermineVersion)

#------------------------------------------------------------------------------
# Setup ParaView Environment
#------------------------------------------------------------------------------
# Determine ParaView Source Version
# Hardcoded version variables are readin from a separate file. This makes it
# easier to have a script to update version numbers automatically.
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "PARAVIEW")
determine_version("${CMAKE_CURRENT_SOURCE_DIR}" "${GIT_EXECUTABLE}" "PARAVIEW")

# Setting this ensures that "make install" will leave rpaths to external
# libraries (not part of the build-tree e.g. Qt, ffmpeg, etc.) intact on
# "make install". This ensures that one can install a version of ParaView on the
# build machine without any issues. If this not desired, simply specify
# CMAKE_INSTALL_RPATH_USE_LINK_PATH when configuring Paraview and
# "make install" will strip all rpaths, which is default behavior.
if (NOT DEFINED CMAKE_INSTALL_RPATH_USE_LINK_PATH)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif ()

if(PARAVIEW_USE_CUDA)
  # This needs to be a top level flag as enable_language needs
  # to be called from CMake entry point for variables to have
  # the correct scope.
  enable_language(CUDA)
endif()

set(BUILD_TESTING OFF)
if (PARAVIEW_BUILD_TESTING)
  set(BUILD_TESTING ON)
endif ()

if (PARAVIEW_USE_PYTHON)
  if (DEFINED PARAVIEW_PYTHON_VERSION)
    if (PARAVIEW_PYTHON_VERSION STREQUAL "3")
      message(DEPRECATION
        "Python 2 is no longer supported. Ignoring the explicit "
        "`PARAVIEW_PYTHON_VERSION=3` request as it is the only valid value "
        "now.")
    else ()
      message(FATAL_ERROR
        "Python 2 is no longer supported. Please either set "
        "`PARAVIEW_PYTHON_VERSION=3` or remove the option completely.")
    endif ()
    unset(PARAVIEW_PYTHON_VERSION)
    unset(PARAVIEW_PYTHON_VERSION CACHE)
  endif ()
  set(VTK_PYTHON_VERSION 3)
endif ()

if (PARAVIEW_USE_PYTHON AND PARAVIEW_BUILD_TESTING)
  # testing needs Python interpreter i.e. `PYTHON_EXECUTABLE` without which
  # our `find_python_module` checks to determine which tests to add or exclude
  # will fail. We find it here.
  find_package(Python3 3.3 QUIET COMPONENTS Interpreter)
  include(FindPythonModules)
endif()

set(paraview_features_needing_remote_modules
  PARAVIEW_ENABLE_MOMENTINVARIANTS
  PARAVIEW_ENABLE_LOOKINGGLASS)
set(use_vtk_remote_modules OFF)
foreach (option IN LISTS paraview_features_needing_remote_modules)
  if (${option})
    set(use_vtk_remote_modules ON)
    message(STATUS "Enabling remote modules for ${option}")
    break()
  endif ()
endforeach ()

if (PARAVIEW_USE_FORTRAN)
  enable_language(Fortran OPTIONAL)
endif ()

if (PARAVIEW_USE_EXTERNAL_VTK)
  find_package(VTK REQUIRED)
else ()
  list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/VTK/CMake")
  include(vtkCMakeBackports)
  include(vtkModule)
  include(vtkModuleDebugging)
  include(vtkEncodeString)
  include(vtkObjectFactory)
  include(vtkExternalData)
  include(vtkVersion)
  include(VTKDetermineVersion)
  set(VTK_VERSION
      "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")
  determine_version("${CMAKE_CURRENT_SOURCE_DIR}/VTK" "${GIT_EXECUTABLE}" "VTK")

  if (PARAVIEW_USE_PYTHON)
    include(vtkModuleWrapPython)
  endif ()
endif ()

set_property(GLOBAL
  PROPERTY
    "_vtk_module_autoinit_include" "\"vtkAutoInit.h\"")

# Note that the way that this flag gets used, it only has an effect on the
# initial configuration.
set(_paraview_plugins_default OFF)
if (PARAVIEW_BUILD_EDITION STREQUAL "CANONICAL")
  set(_paraview_plugins_default ON)
endif()
option(PARAVIEW_PLUGINS_DEFAULT "Default state for ParaView plugins" "${_paraview_plugins_default}")
mark_as_advanced(PARAVIEW_PLUGINS_DEFAULT)
set(paraview_default_plugins
  AcceleratedAlgorithms
  AnalyzeNIfTIReaderWriter
  ArrowGlyph
  BagPlotViewsAndFilters
  CDIReader
  Datamine
  DigitalRockPhysics
  EULATestPlugin
  EmbossingRepresentations
  EyeDomeLighting
  ExplicitStructuredGrid
  FlipBookPlugin
  GenericIOReader
  GeodesicMeasurement
  GMVReader
  HyperTreeGridADR
  IOSSReader
  LagrangianParticleTracker
  Moments
  MooseXfemClip
  NetCDFTimeAnnotationPlugin
  NonOrthogonalSource
  PacMan
  PanoramicProjectionView
  # ParFlow - Because of dependency to nlohmann_json
  SaveStateAndScreenshot
  SLACTools
  StreamLinesRepresentation
  StreamingParticles
  SurfaceLIC
  ThickenLayeredCells
  VTKmFilters
  pvblot)
foreach (paraview_default_plugin IN LISTS paraview_default_plugins)
  set("_paraview_plugin_default_${paraview_default_plugin}" "${PARAVIEW_PLUGINS_DEFAULT}")
endforeach ()

set(paraview_plugin_directories
  "${CMAKE_CURRENT_SOURCE_DIR}/Plugins")
paraview_plugin_find_plugins(paraview_plugin_files ${paraview_plugin_directories})
paraview_plugin_scan(
  PLUGIN_FILES      ${paraview_plugin_files}
  PROVIDES_PLUGINS  paraview_plugins
  REQUIRES_MODULES  paraview_plugin_required_modules)

set(paraview_client_directories
  "${CMAKE_CURRENT_SOURCE_DIR}/Clients"
  "${CMAKE_CURRENT_SOURCE_DIR}/Adaptors")
vtk_module_find_modules(paraview_client_module_files ${paraview_client_directories})
vtk_module_find_kits(paraview_client_kit_files ${paraview_client_directories})
vtk_module_scan(
  MODULE_FILES        ${paraview_client_module_files}
  KIT_FILES           ${paraview_client_kit_files}
  REQUEST_MODULES     ${paraview_client_requested_modules}
  REJECT_MODULES      ${paraview_client_rejected_modules}
  PROVIDES_MODULES    paraview_client_modules
  PROVIDES_KITS       paraview_client_kits
  REQUIRES_MODULES    paraview_client_required_modules
  UNRECOGNIZED_MODULES paraview_client_unrecognized_modules
  WANT_BY_DEFAULT     "${PARAVIEW_BUILD_ALL_MODULES}"
  ENABLE_TESTS        "${PARAVIEW_BUILD_TESTING}")

set(paraview_source_directories
  "${CMAKE_CURRENT_SOURCE_DIR}/Kits"
  "${CMAKE_CURRENT_SOURCE_DIR}/Qt"
  "${CMAKE_CURRENT_SOURCE_DIR}/VTKExtensions"
  "${CMAKE_CURRENT_SOURCE_DIR}/Web"
  "${CMAKE_CURRENT_SOURCE_DIR}/Remoting"
  "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty"
  "${CMAKE_CURRENT_SOURCE_DIR}/Utilities")
vtk_module_find_modules(paraview_module_files ${paraview_source_directories})
vtk_module_find_kits(paraview_kit_files ${paraview_source_directories})
vtk_module_scan(
  MODULE_FILES        ${paraview_module_files}
  KIT_FILES           ${paraview_kit_files}
  REQUEST_MODULES     ${paraview_requested_modules}
                      ${paraview_plugin_required_modules}
                      ${paraview_client_required_modules}
  REJECT_MODULES      ${paraview_rejected_modules}
  PROVIDES_MODULES    paraview_modules
  PROVIDES_KITS       paraview_kits
  REQUIRES_MODULES    paraview_required_modules
  UNRECOGNIZED_MODULES paraview_unrecognized_modules
  WANT_BY_DEFAULT     "${PARAVIEW_BUILD_ALL_MODULES}"
  ENABLE_TESTS        "${PARAVIEW_BUILD_TESTING}")

set(paraview_unrecognized_paraview_modules)
foreach (paraview_module IN LISTS paraview_unrecognized_modules)
  if (paraview_module MATCHES "ParaView::")
    list(APPEND paraview_unrecognized_paraview_modules
      "${paraview_module}")
  endif ()
endforeach ()

if (paraview_unrecognized_paraview_modules)
  list(REMOVE_ITEM paraview_unrecognized_paraview_modules ${paraview_rejected_modules})
endif()

if (paraview_unrecognized_paraview_modules)
  message(FATAL_ERROR
    "The following modules were requested or required, but not found: "
    "${paraview_unrecognized_paraview_modules}.")
endif ()

option(PARAVIEW_VERSIONED_INSTALL "Install artifacts with versioned names." ON)
mark_as_advanced(PARAVIEW_VERSIONED_INSTALL)
set(paraview_version_suffix "")
if (PARAVIEW_VERSIONED_INSTALL)
  set(paraview_version_suffix "-${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}")
endif ()

set(paraview_cmake_destination
  "${CMAKE_INSTALL_LIBDIR}/cmake/paraview${paraview_version_suffix}")

set(PARAVIEW_CUSTOM_LIBRARY_SUFFIX "<DEFAULT>"
  CACHE STRING "Custom library file name suffix (defaults to the version number)")
mark_as_advanced(PARAVIEW_CUSTOM_LIBRARY_SUFFIX)
if (PARAVIEW_CUSTOM_LIBRARY_SUFFIX STREQUAL "<DEFAULT>")
  if (PARAVIEW_VERSIONED_INSTALL)
    set(PARAVIEW_CUSTOM_LIBRARY_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}")
  else ()
    set(PARAVIEW_CUSTOM_LIBRARY_SUFFIX "")
  endif ()
endif ()

# Search for dependent libraries beside
set(_paraview_CMAKE_INSTALL_RPATH_save
  "${CMAKE_INSTALL_RPATH}")
if (APPLE)
  list(APPEND CMAKE_INSTALL_RPATH
    "@loader_path")
elseif (UNIX)
  list(APPEND CMAKE_INSTALL_RPATH
    "$ORIGIN")
endif ()

if (PARAVIEW_USE_EXTERNAL_VTK)
  if (paraview_required_modules OR paraview_unrecognized_modules)
    message(FATAL_ERROR
      "The following modules were requested or required, but not found: "
      "${paraview_required_modules};${paraview_unrecognized_modules}.")
  endif ()

  if (PARAVIEW_USE_PYTHON)
    find_package(Python3 3.3 QUIET REQUIRED COMPONENTS Interpreter)
    set(PYTHON_VERSION_MAJOR "${Python3_VERSION_MAJOR}")
    set(PYTHON_VERSION_MINOR "${Python3_VERSION_MINOR}")
    if (NOT PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX)
      vtk_module_python_default_destination(PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX)
    endif ()
  endif ()
else ()
  set(vtk_requested_modules)
  foreach (vtk_module IN LISTS
                paraview_requested_modules
                paraview_plugin_required_modules
                paraview_client_required_modules)
    if (vtk_module MATCHES "ParaView::")
      continue ()
    endif ()
    list(APPEND vtk_requested_modules
      "${vtk_module}")
  endforeach ()
  set(vtk_rejected_modules)
  foreach (vtk_module IN LISTS paraview_rejected_modules)
    if (vtk_module MATCHES "ParaView::")
      continue ()
    endif ()
    list(APPEND vtk_rejected_modules
      "${vtk_module}")
  endforeach ()

  set(vtk_enable_tests "OFF")
  if (PARAVIEW_BUILD_VTK_TESTING)
    set(vtk_enable_tests "${PARAVIEW_BUILD_TESTING}")
    # Needed for `rtImageTest.py` tests to work. See note in `vtkModuleTesting`
    # about this.
    set(VTK_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VTK")
  endif ()

  # Forward shared library setting to VTK.
  set(VTK_BUILD_SHARED_LIBS "${PARAVIEW_BUILD_SHARED_LIBS}")

  # Forward Python option to VTK.
  set(VTK_WRAP_PYTHON "${PARAVIEW_USE_PYTHON}")

  # Forward MPI option to VTK.
  set(VTK_USE_MPI "${PARAVIEW_USE_MPI}")
  set(VTK_SERIAL_TESTS_USE_MPIEXEC "${PARAVIEW_SERIAL_TESTS_USE_MPIEXEC}")

  # Forward CUDA option to VTK
  set(VTK_USE_CUDA "${PARAVIEW_USE_CUDA}")
  # Forward Memkind option to VTK.
  set(VTK_USE_MEMKIND "${PARAVIEW_USE_MEMKIND}")

  # Forward logging option to VTK.
  set(VTK_ENABLE_LOGGING ON)

  if (use_vtk_remote_modules)
    if (PARAVIEW_ENABLE_MOMENTINVARIANTS)
      set(VTK_MODULE_ENABLE_VTK_MomentInvariants WANT)
    endif()
    if (PARAVIEW_ENABLE_LOOKINGGLASS)
      set(VTK_MODULE_ENABLE_VTK_RenderingLookingGlass WANT)
    endif()
    # include the Remote module
    include("${CMAKE_CURRENT_SOURCE_DIR}/VTK/CMake/vtkDownload.cmake")
    # TODO: remove this after changing in vtkModuleRemote.cmake:160
    # "${VTK_SOURCE_DIR}/Remote/${_name}" to "${CMAKE_CURRENT_LIST_DIR}/${_name}"
    set(VTK_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VTK")
    add_subdirectory(VTK/Remote)
  endif()

  # Set up these variables now so that modules may acess them
  if (NOT PARAVIEW_USE_EXTERNAL_VTK AND PARAVIEW_INSTALL_DEVELOPMENT_FILES)
    set(vtk_cmake_dir
      "${CMAKE_CURRENT_SOURCE_DIR}/VTK/CMake")
    set(vtk_cmake_destination
      "${paraview_cmake_destination}/vtk")
    set(vtk_cmake_build_dir
      "${CMAKE_CURRENT_BINARY_DIR}/${vtk_cmake_destination}")
    set(VTK_RELOCATABLE_INSTALL
      "${PARAVIEW_RELOCATABLE_INSTALL}")
  endif ()

  vtk_module_find_modules(vtk_module_files "${CMAKE_CURRENT_SOURCE_DIR}/VTK")
  vtk_module_find_kits(vtk_kit_files "${CMAKE_CURRENT_SOURCE_DIR}/VTK")
  # Exclude VTK's examples.
  list(FILTER vtk_module_files EXCLUDE REGEX "/Examples/")
  list(FILTER vtk_kit_files EXCLUDE REGEX "/Examples/")
  vtk_module_scan(
    MODULE_FILES        ${vtk_module_files}
    KIT_FILES           ${vtk_kit_files}
    REQUEST_MODULES     ${vtk_requested_modules}
                        ${paraview_required_modules}
    REJECT_MODULES      ${vtk_rejected_modules}
    PROVIDES_MODULES    vtk_modules
    PROVIDES_KITS       vtk_kits
    REQUIRES_MODULES    vtk_required_modules
    UNRECOGNIZED_MODULES vtk_unrecognized_modules
    WANT_BY_DEFAULT     OFF
    ENABLE_TESTS        "${vtk_enable_tests}")

  if (vtk_unrecognized_modules AND vtk_rejected_modules)
    list(REMOVE_ITEM vtk_unrecognized_modules ${vtk_rejected_modules})
  endif()
  if (vtk_required_modules OR vtk_unrecognized_modules)
    message(FATAL_ERROR
      "The following modules were requested or required, but not found: "
      "${vtk_required_modules};${vtk_unrecognized_modules}.")
  endif ()

  if (PARAVIEW_USE_PYTHON)
    find_package("Python3" 3.3 QUIET REQUIRED COMPONENTS Interpreter)
    set(PYTHON_VERSION_MAJOR "${Python3_VERSION_MAJOR}")
    set(PYTHON_VERSION_MINOR "${Python3_VERSION_MINOR}")
    if (NOT PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX)
      vtk_module_python_default_destination(PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX)
    endif ()
    set(VTK_PYTHON_SITE_PACKAGES_SUFFIX "${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}")
  endif ()

  set(VTK_BUILD_QT_DESIGNER_PLUGIN OFF)
  set(VTK_INSTALL_PYTHON_EXES OFF)

  cmake_dependent_option(VTK_NO_PYTHON_THREADS "Disable Python Threads support" ON
    "PARAVIEW_USE_PYTHON" OFF)
  mark_as_advanced(VTK_NO_PYTHON_THREADS)
  cmake_dependent_option(VTK_PYTHON_FULL_THREADSAFE "Wrap all Python calls with the GIL" ON
    "NOT VTK_NO_PYTHON_THREADS" OFF)
  mark_as_advanced(VTK_PYTHON_FULL_THREADSAFE)
  cmake_dependent_option(VTK_USE_TK "Build VTK with Tk support" OFF
    "PARAVIEW_USE_PYTHON" OFF)

  cmake_dependent_option(PARAVIEW_WINDOWS_PYTHON_DEBUGGABLE
    "Explicitly indicate that the Python library is debuggable" OFF
    "WIN32;PARAVIEW_USE_PYTHON" OFF)
  mark_as_advanced(PARAVIEW_WINDOWS_PYTHON_DEBUGGABLE)
  set(VTK_WINDOWS_PYTHON_DEBUGGABLE ${PARAVIEW_WINDOWS_PYTHON_DEBUGGABLE})

  vtk_module_build(
    MODULES             ${vtk_modules}
    KITS                ${vtk_kits}
    PACKAGE             VTK
    INSTALL_EXPORT      VTK
    INSTALL_HEADERS     "${PARAVIEW_INSTALL_DEVELOPMENT_FILES}"
    HEADERS_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/paraview${paraview_version_suffix}"
    CMAKE_DESTINATION   "${paraview_cmake_destination}/vtk"
    LICENSE_DESTINATION "${CMAKE_INSTALL_LICENSEDIR}"
    HIERARCHY_DESTINATION "${CMAKE_INSTALL_LIBDIR}/vtk/hierarchy/${CMAKE_PROJECT_NAME}"
    LIBRARY_NAME_SUFFIX "${PARAVIEW_CUSTOM_LIBRARY_SUFFIX}"
    VERSION             "${PARAVIEW_VERSION}"
    SOVERSION           "1"
    TEST_DATA_TARGET    ParaViewData
    BUILD_WITH_KITS     "${PARAVIEW_BUILD_WITH_KITS}"
    USE_EXTERNAL        "${PARAVIEW_BUILD_WITH_EXTERNAL}"
    ENABLE_WRAPPING     ON
    TEST_INPUT_DATA_DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/VTK/Testing"
    TEST_OUTPUT_DATA_DIRECTORY  "${CMAKE_CURRENT_BINARY_DIR}/ExternalData/VTK/Testing")

  install(
    FILES       "${CMAKE_CURRENT_SOURCE_DIR}/VTK/Copyright.txt"
    DESTINATION "${CMAKE_INSTALL_LICENSEDIR}/VTK"
    COMPONENT   "license")
endif ()

# Determine whether the ParaView GUI will be built or not.
set(paraview_will_be_built 0)
if (PARAVIEW_USE_QT AND
    (WIN32 OR VTK_USE_X OR VTK_USE_COCOA))
  set(paraview_will_be_built 1)
endif ()

if (NOT PARAVIEW_USE_EXTERNAL_VTK AND PARAVIEW_INSTALL_DEVELOPMENT_FILES)
  include("${CMAKE_CURRENT_SOURCE_DIR}/VTK/CMake/vtkInstallCMakePackage.cmake")
endif ()

set(paraview_test_data_directory_input
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing")
set(paraview_test_data_directory_output
  "${CMAKE_CURRENT_BINARY_DIR}/ExternalData")
set(_paraview_add_tests_default_data_directory
  "${paraview_test_data_directory_output}")
set(_paraview_add_tests_default_test_data_target
  "ParaViewData")

vtk_module_build(
  MODULES             ${paraview_modules}
  KITS                ${paraview_kits}
  INSTALL_EXPORT      ParaView
  INSTALL_HEADERS     "${PARAVIEW_INSTALL_DEVELOPMENT_FILES}"
  HEADERS_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/paraview${paraview_version_suffix}"
  CMAKE_DESTINATION   "${paraview_cmake_destination}"
  LICENSE_DESTINATION "${CMAKE_INSTALL_LICENSEDIR}"
  HIERARCHY_DESTINATION "${CMAKE_INSTALL_LIBDIR}/vtk/hierarchy/${CMAKE_PROJECT_NAME}"
  LIBRARY_NAME_SUFFIX "${PARAVIEW_CUSTOM_LIBRARY_SUFFIX}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1"
  TEST_DATA_TARGET    ParaViewData
  BUILD_WITH_KITS     "${PARAVIEW_BUILD_WITH_KITS}"
  USE_EXTERNAL        "${PARAVIEW_BUILD_WITH_EXTERNAL}"
  TEST_INPUT_DATA_DIRECTORY   "${paraview_test_data_directory_input}"
  TEST_OUTPUT_DATA_DIRECTORY  "${paraview_test_data_directory_output}")

vtk_module_build(
  MODULES             ${paraview_client_modules}
  KITS                ${paraview_client_kits}
  PACKAGE             ParaViewClients
  INSTALL_EXPORT      ParaViewClients
  INSTALL_HEADERS     "${PARAVIEW_INSTALL_DEVELOPMENT_FILES}"
  HEADERS_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/paraview${paraview_version_suffix}"
  CMAKE_DESTINATION   "${paraview_cmake_destination}"
  LICENSE_DESTINATION "${CMAKE_INSTALL_LICENSEDIR}"
  HIERARCHY_DESTINATION "${CMAKE_INSTALL_LIBDIR}/vtk/hierarchy/${CMAKE_PROJECT_NAME}"
  LIBRARY_NAME_SUFFIX "${PARAVIEW_CUSTOM_LIBRARY_SUFFIX}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1"
  TEST_DATA_TARGET    ParaViewData
  BUILD_WITH_KITS     "${PARAVIEW_BUILD_WITH_KITS}"
  USE_EXTERNAL        "${PARAVIEW_BUILD_WITH_EXTERNAL}"
  TEST_INPUT_DATA_DIRECTORY   "${paraview_test_data_directory_input}"
  TEST_OUTPUT_DATA_DIRECTORY  "${paraview_test_data_directory_output}")

include(vtkModuleJson)
vtk_module_json(
  MODULES ${vtk_modules}
  OUTPUT "vtk-modules.json")
vtk_module_json(
  MODULES ${paraview_modules}
  OUTPUT "paraview-modules.json")
vtk_module_json(
  MODULES ${paraview_client_modules}
  OUTPUT "paraview-client-modules.json")

list(LENGTH vtk_modules vtk_modules_len)
list(LENGTH paraview_modules paraview_modules_len)
list(LENGTH paraview_client_modules paraview_client_modules_len)
message(STATUS "Enabled modules: VTK(${vtk_modules_len}), ParaView(${paraview_modules_len} + ${paraview_client_modules_len})")

set(autoload_plugins)
foreach (paraview_plugin IN LISTS paraview_plugins)
  option("PARAVIEW_PLUGIN_AUTOLOAD_${paraview_plugin}" "Autoload the ${paraview_plugin} plugin" OFF)
  mark_as_advanced("PARAVIEW_PLUGIN_AUTOLOAD_${paraview_plugin}")

  if (PARAVIEW_PLUGIN_AUTOLOAD_${paraview_plugin})
    list(APPEND autoload_plugins
      "${paraview_plugin}")
  endif ()
endforeach ()

paraview_plugin_build(
  HEADERS_DESTINATION   "${CMAKE_INSTALL_INCLUDEDIR}/paraview${paraview_version_suffix}"
  RUNTIME_DESTINATION   "${CMAKE_INSTALL_BINDIR}"
  LIBRARY_DESTINATION   "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY_SUBDIRECTORY  "paraview${paraview_version_suffix}/plugins"
  CMAKE_DESTINATION     "${paraview_cmake_destination}"
  PLUGINS_FILE_NAME "paraview.plugins.xml"
  TARGET            ParaView::paraview_plugins
  INSTALL_EXPORT    ParaViewPlugins
  INSTALL_HEADERS   "${PARAVIEW_INSTALL_DEVELOPMENT_FILES}"
  TARGET_COMPONENT  "development"
  PLUGINS_COMPONENT "plugins"
  PLUGINS ${paraview_plugins}
  AUTOLOAD ${autoload_plugins})

set(CMAKE_INSTALL_RPATH
  "${_paraview_CMAKE_INSTALL_RPATH_save}")

vtk_module_wrap_client_server(
  MODULES   ${paraview_client_modules}
            ${paraview_modules}
            ${vtk_modules}
  TARGET    paraview_client_server
  INSTALL_EXPORT ParaView)

paraview_server_manager_process(
  MODULES   ${paraview_modules}
            ${vtk_modules}
  TARGET    paraview_server_manager
  XML_FILES paraview_server_manager_files
  INSTALL_EXPORT ParaView)

if (PARAVIEW_USE_PYTHON)
  if (PARAVIEW_USE_EXTERNAL_VTK)
    # TODO: Check if VTK is already wrapped in Python.
    # Error out?
  else ()
    vtk_module_wrap_python(
      MODULES         ${vtk_modules}
      INSTALL_EXPORT  VTKPython
      INSTALL_HEADERS "${PARAVIEW_INSTALL_DEVELOPMENT_FILES}"
      PYTHON_PACKAGE  "vtkmodules"
      MODULE_DESTINATION  "${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}"
      CMAKE_DESTINATION   "${paraview_cmake_destination}/vtk"
      LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      USE_DEBUG_SUFFIX    "${PARAVIEW_WINDOWS_PYTHON_DEBUGGABLE}"
      WRAPPED_MODULES vtk_python_wrapped_modules
      TARGET          VTK::vtkpythonmodules)

    add_subdirectory(VTK/Wrapping/Python)

    if (PARAVIEW_INSTALL_DEVELOPMENT_FILES)
      export(
        EXPORT    VTKPython
        NAMESPACE VTK::
        FILE      "${CMAKE_BINARY_DIR}/${paraview_cmake_destination}/vtk/VTKPython-targets.cmake")
      install(
        EXPORT      VTKPython
        NAMESPACE   VTK::
        FILE        VTKPython-targets.cmake
        DESTINATION "${paraview_cmake_destination}/vtk"
        COMPONENT   "development")
    endif ()
  endif ()

  get_property(vtk_required_python_modules GLOBAL
    PROPERTY  vtk_required_python_modules)
  if (vtk_required_python_modules)
    list(REMOVE_DUPLICATES vtk_required_python_modules)
  endif ()
  string(REPLACE ";" "\n" vtk_required_python_modules "${vtk_required_python_modules}")
  file(WRITE "${CMAKE_BINARY_DIR}/requirements.txt"
    "${vtk_required_python_modules}\n")

  vtk_module_wrap_python(
    MODULES         ${paraview_modules}
                    ${paraview_client_modules}
    INSTALL_EXPORT  ParaViewPython
    INSTALL_HEADERS "${PARAVIEW_INSTALL_DEVELOPMENT_FILES}"
    PYTHON_PACKAGE  "paraview.modules"
    MODULE_DESTINATION  "${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}"
    CMAKE_DESTINATION   "${paraview_cmake_destination}"
    LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    USE_DEBUG_SUFFIX    "${PARAVIEW_WINDOWS_PYTHON_DEBUGGABLE}"
    WRAPPED_MODULES paraview_python_wrapped_modules
    TARGET          ParaView::pvpythonmodules
    DEPENDS         VTK::vtkpythonmodules)

  if (PARAVIEW_INSTALL_DEVELOPMENT_FILES)
    export(
      EXPORT    ParaViewPython
      NAMESPACE ParaView::
      FILE      "${CMAKE_BINARY_DIR}/${paraview_cmake_destination}/ParaViewPython-targets.cmake")
    install(
      EXPORT      ParaViewPython
      NAMESPACE   ParaView::
      FILE        ParaViewPython-targets.cmake
      DESTINATION "${paraview_cmake_destination}"
      COMPONENT   "development")
  endif ()
endif ()

if (PARAVIEW_ENABLE_WEB)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Web/package.json.in"
    "${CMAKE_CURRENT_BINARY_DIR}/package.json"
    @ONLY)
endif ()

add_subdirectory(Clients)

if (PARAVIEW_USE_PYTHON)
  add_subdirectory(Wrapping/Python)
endif ()

if (NOT PARAVIEW_USE_EXTERNAL_VTK)
  # Configure the CTestCustom.cmake file for exclusions.
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/VTK/CMake/CTestCustom.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/VTK/CTestCustom.cmake"
    @ONLY)
endif ()

# Configure the CTestCustom.cmake file for exclusions.
configure_file(
  "${paraview_cmake_dir}/CTestCustom.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake"
  @ONLY)

ExternalData_Add_Target(ParaViewData)

if (PARAVIEW_INSTALL_DEVELOPMENT_FILES)
  set(paraview_cmake_build_dir
    "${CMAKE_CURRENT_BINARY_DIR}/${paraview_cmake_destination}")
  include("${CMAKE_CURRENT_SOURCE_DIR}/CMake/ParaViewInstallCMakePackage.cmake")
endif ()

set(is_git FALSE)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  set(is_git TRUE)
endif ()

if (NOT WIN32 AND is_git AND Git_FOUND AND NOT GIT_VERSION_STRING VERSION_LESS "2.18.0")
  set(source_all)
  if (PARAVIEW_SOURCE_TARBALL_TARGETS)
    set(source_all ALL)
  endif ()

  add_custom_target(paraview-source ${source_all})
  foreach (format tgz txz zip)
    add_custom_target("paraview-source-${format}" ${source_all}
      COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/Utilities/Maintenance/create_tarballs.bash"
              "--${format}"
      WORKING_DIRECTORY
              "${CMAKE_CURRENT_BINARY_DIR}"
      COMMENT "Creating source tarball in ${format} format")
    add_dependencies("paraview-source-${format}"
      ParaViewData)
    add_dependencies(paraview-source
      "paraview-source-${format}")
  endforeach ()
  # This is a hack.  Git version 2.17 does NOT like running these in parallel and
  # reports that a lockfile is already locked.  In order for the build to succeed,
  # force these to run in serial.
  add_dependencies(paraview-source-tgz paraview-source-txz)
  add_dependencies(paraview-source-txz paraview-source-zip)
endif ()

# This is the directory under which all generated docs, such as Doxygen and
# Sphinx generated docs, are placed.
if (NOT PARAVIEW_GENERATED_DOCUMENTATION_OUTPUT_DIRECTORY)
  set(PARAVIEW_GENERATED_DOCUMENTATION_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/doc")
endif ()

#-----------------------------------------------------------------------------
# Build doxygen documentation.
if (PARAVIEW_BUILD_DEVELOPER_DOCUMENTATION)
  add_subdirectory(Utilities/Doxygen)
  add_subdirectory(Utilities/Sphinx)
  add_subdirectory(Documentation)
endif()

#------------------------------------------------------------------------------
# Test to check install tree
#------------------------------------------------------------------------------
if (BUILD_TESTING AND PARAVIEW_INSTALL_DEVELOPMENT_FILES)
  add_test( NAME pv.TestDevelopmentInstall
    COMMAND ${CMAKE_COMMAND}
    -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
    -DPARAVIEW_BINARY_DIR:PATH=${CMAKE_BINARY_DIR}
    -DPARAVIEW_INSTALL_DIR:PATH=${CMAKE_INSTALL_PREFIX}
    -DPARAVIEW_SOURCE_DIR:PATH=${CMAKE_SOURCE_DIR}
    -DPARAVIEW_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
    -DPARAVIEW_CMAKE_DESTINATION:STRING=${paraview_cmake_destination}
    -DPARAVIEW_BINDIR:STRING=${CMAKE_INSTALL_BINDIR}
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    -DCMAKE_GENERATOR:STRING=${CMAKE_GENERATOR}
    -DCMAKE_GENERATOR_PLATFORM:STRING=${CMAKE_GENERATOR_PLATFORM}
    -DCMAKE_GENERATOR_TOOLSET:STRING=${CMAKE_GENERATOR_TOOLSET}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CMake/ParaViewTestInstall.cmake)
  set_tests_properties(pv.TestDevelopmentInstall PROPERTIES
    LABELS "paraview"
    ENVIRONMENT "DESTDIR=${CMAKE_BINARY_DIR}/test-install"
    # 10 minutes should be enough time.
    TIMEOUT 600)
endif()
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Test to check that we can process all of the Catalyst editions
#------------------------------------------------------------------------------
if (BUILD_TESTING AND PARAVIEW_USE_PYTHON AND UNIX AND NOT APPLE)
if (FALSE) # XXX(FIXME)
  # disabling till we Catalyst editions are fixed post modularization
  add_test( NAME GenerateCatalystEditions
    COMMAND ${CMAKE_COMMAND}
    -DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
    -DPARAVIEW_SOURCE_DIR:PATH=${CMAKE_SOURCE_DIR}
    -DPARAVIEW_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
    -P ${CMAKE_SOURCE_DIR}/CMake/ParaViewTestCatalystCodeGeneration.cmake)
  set_tests_properties(GenerateCatalystEditions PROPERTIES
    LABELS "paraview;catalyst")
endif()
endif()
#------------------------------------------------------------------------------

install(
  FILES       "${CMAKE_CURRENT_SOURCE_DIR}/Copyright.txt"
              "${CMAKE_CURRENT_SOURCE_DIR}/License_v1.2.txt"
  DESTINATION "${CMAKE_INSTALL_LICENSEDIR}"
  COMPONENT   "license")

#-----------------------------------------------------------------------------
if (PARAVIEW_BUILD_EXAMPLES)
  add_subdirectory(Examples)
endif ()
