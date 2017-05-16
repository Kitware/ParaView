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
cmake_minimum_required(VERSION 3.3)

project(ParaView)

# Disallow in-source build
if ("${ParaView_SOURCE_DIR}"  STREQUAL "${ParaView_BINARY_DIR}")
  message(FATAL_ERROR
    "ParaView requires an out of source Build. "
    "Please create a separate binary directory and run CMake there.")
endif()

#------------------------------------------------------------------------------
# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

#------------------------------------------------------------------------------
# Choose static or shared libraries.
option(BUILD_SHARED_LIBS "Build VTK with shared libraries." ON)
set(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

#------------------------------------------------------------------------------
# Protect against a stampede of static links at the same time.
if (NOT BUILD_SHARED_LIBS)
  set(PV_EXE_JOB_LINK_POOL static_exe_link)
  set_property(GLOBAL APPEND PROPERTY
    JOB_POOLS static_exe_link=1)
endif ()

#-------------------------------------------------------------------------------
set (ParaView_CMAKE_BINARY_DIR "${ParaView_BINARY_DIR}/CMake")
set (ParaView_CMAKE_DIR "${ParaView_SOURCE_DIR}/CMake")

#-------------------------------------------------------------------------------
# Set up CMAKE_MODULE_PATH
set (CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${ParaView_CMAKE_BINARY_DIR}
  ${ParaView_CMAKE_DIR}
)

# Use the new version of the variable names for output of build process.
# Make sure these are consistent with what VTK sets.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

include(Utilities/Git/Git.cmake)
include(ParaViewDetermineVersion)
include(CMakeDependentOption)
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Setup ParaView Environment
#------------------------------------------------------------------------------
# Determine ParaView Source Version
# Hardcoded version variables are readin from a separate file. This makes it
# easier to have a script to update version numbers automatically.
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "PARAVIEW")
determine_version(${ParaView_SOURCE_DIR} ${GIT_EXECUTABLE} "PARAVIEW")

# Setup some cross compiling related things.
if (CMAKE_CROSSCOMPILING AND NOT COMPILE_TOOLS_IMPORTED)
  find_package (ParaViewCompileTools REQUIRED)
  # this keeps VTK sub-dir from trying to import VTKCompileTools.
  set (COMPILE_TOOLS_IMPORTED TRUE)
endif()

#------------------------------------------------------------------------------
# Setup install directories (we use names with VTK_ prefix, since ParaView now
# is built as a custom "VTK" library.
if(NOT VTK_INSTALL_RUNTIME_DIR)
  set(VTK_INSTALL_RUNTIME_DIR bin)
endif()
if(NOT VTK_INSTALL_LIBRARY_DIR)
  set(VTK_INSTALL_LIBRARY_DIR lib/paraview-${PARAVIEW_VERSION})
endif()
# ParaView installs the vtk python modules specifically to appropriate locations.
if(NOT VTK_INSTALL_PYTHON_MODULE_DIR)
  set (VTK_INSTALL_PYTHON_MODULE_DIR "${VTK_INSTALL_LIBRARY_DIR}/site-packages" CACHE
    INTERNAL "Directory where python modules will be installed")
endif()
if(NOT VTK_BUILD_PYTHON_MODULE_DIR)
  set (VTK_BUILD_PYTHON_MODULE_DIR "${CMAKE_BINARY_DIR}/lib/site-packages" CACHE
    INTERNAL "Directory where python modules will be built")
endif()
if(NOT VTK_INSTALL_ARCHIVE_DIR)
  set(VTK_INSTALL_ARCHIVE_DIR lib/paraview-${PARAVIEW_VERSION})
endif()
if(NOT VTK_INSTALL_INCLUDE_DIR)
  set(VTK_INSTALL_INCLUDE_DIR include/paraview-${PARAVIEW_VERSION})
endif()
if(NOT VTK_INSTALL_DATA_DIR)
  set(VTK_INSTALL_DATA_DIR share/paraview-${PARAVIEW_VERSION})
endif()
if(NOT VTK_INSTALL_DOC_DIR)
  set(VTK_INSTALL_DOC_DIR share/doc/paraview-${PARAVIEW_VERSION})
endif()
if(NOT VTK_INSTALL_PACKAGE_DIR)
  set(VTK_INSTALL_PACKAGE_DIR "lib/cmake/paraview-${PARAVIEW_VERSION}")
endif()
if(NOT VTK_INSTALL_DOXYGEN_DIR)
  set(VTK_INSTALL_DOXYGEN_DIR ${VTK_INSTALL_DOC_DIR}/doxygen)
endif()
if(NOT VTK_INSTALL_EXPORT_NAME)
  set(VTK_INSTALL_EXPORT_NAME ParaViewTargets)
endif()
if(NOT VTK_MODULES_DIR)
  set(VTK_MODULES_DIR "${ParaView_BINARY_DIR}/${VTK_INSTALL_PACKAGE_DIR}/Modules")
endif()
if(NOT PARAVIEW_WWW_DIR)
  set(PARAVIEW_WWW_DIR "${ParaView_BINARY_DIR}/www")
endif()
set(PARAVIEW_MODULES_DIR ${VTK_MODULES_DIR})

if (NOT DEFINED VTK_CUSTOM_LIBRARY_SUFFIX)
  set (VTK_CUSTOM_LIBRARY_SUFFIX "-pv${PARAVIEW_VERSION}")
endif()

# Handle the target export file, this is used if building against a build tree.
if(NOT VTK_EXPORTS_FILE)
  set(VTK_EXPORTS_FILE "${ParaView_BINARY_DIR}/VTK/${VTK_INSTALL_EXPORT_NAME}.cmake")
endif()
file(REMOVE "${VTK_EXPORTS_FILE}")


if(NOT PV_INSTALL_PLUGIN_DIR)
  if (WIN32)
    set (PV_INSTALL_PLUGIN_DIR ${VTK_INSTALL_RUNTIME_DIR})
  else ()
    set (PV_INSTALL_PLUGIN_DIR ${VTK_INSTALL_LIBRARY_DIR})
  endif()
endif()

# Disable installing of the Qt Designer plugin. There's no need for it in
# ParaView install rules.
set (VTK_INSTALL_NO_QT_PLUGIN TRUE)

# Disable installing of the vtkpython executables.
set (VTK_INSTALL_NO_PYTHON_EXES TRUE)

# for temporary backwards compatibility.
set (PV_INSTALL_BIN_DIR ${VTK_INSTALL_RUNTIME_DIR})
set (PV_INSTALL_LIB_DIR ${VTK_INSTALL_ARCHIVE_DIR})
set (PV_INSTALL_EXPORT_NAME ${VTK_INSTALL_EXPORT_NAME})

# Setting this ensures that "make install" will leave rpaths to external
# libraries (not part of the build-tree e.g. Qt, ffmpeg, etc.) intact on
# "make install". This ensures that one can install a version of ParaView on the
# build machine without any issues. If this not desired, simply specify
# CMAKE_INSTALL_RPATH_USE_LINK_PATH when configuring Paraview and
# "make install" will strip all rpaths, which is default behavior.
if(NOT CMAKE_INSTALL_RPATH_USE_LINK_PATH )
  set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Define ParaView specific options.
#------------------------------------------------------------------------------
option(BUILD_TESTING "Build ParaView Testing" OFF)
option(BUILD_EXAMPLES "Build ParaView examples" OFF)
option(BUILD_SHARED_LIBS "Build ParaView using shared libraries" ON)
option(PARAVIEW_BUILD_QT_GUI "Enable ParaView Qt-based client" ON)
option(PARAVIEW_USE_VTKM "Build ParaView with VTKm many-core filters" OFF)
option(PARAVIEW_USE_MPI "Enable MPI support for parallel computing" OFF)
option(PARAVIEW_USE_OSPRAY "Build ParaView with OSPRay Ray Traced rendering" OFF)
option(PARAVIEW_USE_OPENVR "Build ParaView with OpenVR rendering" OFF)
set(VTK_OPENVR_OBJECT_FACTORY OFF
    CACHE INTERNAL "ParaView requires the OpenVR factory be disabled" FORCE)
option(PARAVIEW_USE_VISITBRIDGE "Build ParaView with VisIt readers." OFF)

# Xdmf3 is not ON by default since it depends on Boost.
option(PARAVIEW_ENABLE_XDMF3 "Enable Xdmf3 support (requires Boost)." OFF)

if (UNIX)
  option(PARAVIEW_ENABLE_FFMPEG "Enable FFMPEG Support." OFF)
endif()

if (UNIX)
  # Since development installs are currently being tested only on linuxes. We
  # can support these for other platforms if time permits.
  option(PARAVIEW_INSTALL_DEVELOPMENT_FILES
    "When enabled, \"make install\" will install development files" OFF)
endif()

cmake_dependent_option(PARAVIEW_USE_MPI_SSEND
  "Use MPI synchronous-send commands for communication" OFF
  "PARAVIEW_USE_MPI" OFF)
cmake_dependent_option(PARAVIEW_USE_ICE_T
  "Enable IceT (needed for parallel rendering)" ON
  "PARAVIEW_USE_MPI" OFF)
cmake_dependent_option(PARAVIEW_INITIALIZE_MPI_ON_CLIENT
  "Initialize MPI on client-processes by default. Can be overridden using command line arguments"
  OFF
  "PARAVIEW_USE_MPI" OFF)

mark_as_advanced(PARAVIEW_USE_ICE_T
  PARAVIEW_USE_MPI_SSEND
  PARAVIEW_INITIALIZE_MPI_ON_CLIENT)

cmake_dependent_option(PARAVIEW_ENABLE_QT_SUPPORT
  "Build ParaView with Qt support (without GUI)" OFF
  "NOT PARAVIEW_BUILD_QT_GUI" ON)

# Add an option to enable using Qt Webkit for widgets, as needed.
# Default is OFF. We don't want to depend on WebKit unless absolutely needed.
cmake_dependent_option(PARAVIEW_USE_QTWEBKIT
  "Use Qt WebKit components as needed." OFF
  "PARAVIEW_ENABLE_QT_SUPPORT" OFF)
mark_as_advanced(PARAVIEW_USE_QTWEBKIT)

# Add an option to enable using Qt Help, as needed.
# Default is ON to enable integrated help/documentation.
cmake_dependent_option(PARAVIEW_USE_QTHELP
  "Use Qt Help infrastructure as needed." ON
  "PARAVIEW_ENABLE_QT_SUPPORT" OFF)
mark_as_advanced(PARAVIEW_USE_QTHELP)

# This option controls the behavior of the build_help_project function.  If it is
# on then the .qch file will be generated, if not then the function does nothing.
cmake_dependent_option(PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION
       "Build the ParaView help menu documentation for ParaView and its plugins" ON
       "PARAVIEW_BUILD_QT_GUI" ON)
mark_as_advanced(PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION)


# If PARAVIEW_BUILD_QT_GUI is OFF, provide an option to the user to turn other command line
# executables ON/OFF.
cmake_dependent_option(PARAVIEW_ENABLE_COMMANDLINE_TOOLS
  "Build ParaView command-line tools" ON
  "NOT PARAVIEW_BUILD_QT_GUI" ON)

option(PARAVIEW_ENABLE_PYTHON "Enable/Disable Python scripting support" OFF)
cmake_dependent_option(PARAVIEW_USE_UNIFIED_BINDINGS "If enabled, Python bindings will back the ClientServer wrapping implementation" OFF
  PARAVIEW_ENABLE_PYTHON OFF)
mark_as_advanced(PARAVIEW_USE_UNIFIED_BINDINGS)
cmake_dependent_option(PARAVIEW_ENABLE_MATPLOTLIB "Enable/Disable Python scripting support" ON
  "PARAVIEW_ENABLE_PYTHON" OFF)
mark_as_advanced(PARAVIEW_ENABLE_MATPLOTLIB)

# If building on Unix with MPI enabled, we will present another option to
# enable building of CosmoTools VTK extensions. This option is by default
# OFF and set to OFF if ParaView is not built with MPI.
if (UNIX)
  cmake_dependent_option(PARAVIEW_ENABLE_COSMOTOOLS
    "Build ParaView with CosmoTools VTK Extensions" OFF
    "PARAVIEW_USE_MPI" OFF)
  mark_as_advanced(PARAVIEW_ENABLE_COSMOTOOLS)
endif()

# Is this a 32 bit or 64bit build. Display this in about dialog.
if ("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
  set(PARAVIEW_BUILD_ARCHITECTURE "64")
else()
  set(PARAVIEW_BUILD_ARCHITECTURE "32")
endif()

# setup external data. this will automatically download the test
# data and baseline files to the build tree and set PARAVIEW_DATA_ROOT.
include(ParaViewExternalData)
set(ExternalData_LINK_CONTENT MD5)
set(ExternalData_BINARY_ROOT          "${CMAKE_BINARY_DIR}/ExternalData")
set(PARAVIEW_TEST_DATA_DIR            "${CMAKE_CURRENT_SOURCE_DIR}/Testing/Data")
set(PARAVIEW_TEST_BASELINE_DIR        "${PARAVIEW_TEST_DATA_DIR}/Baseline")
set(PARAVIEW_TEST_OUTPUT_DATA_DIR     "${ExternalData_BINARY_ROOT}/Testing/Data")
set(PARAVIEW_TEST_OUTPUT_BASELINE_DIR "${PARAVIEW_TEST_OUTPUT_DATA_DIR}/Baseline")
set(PARAVIEW_TEST_OUTPUT_DIR          "${CMAKE_BINARY_DIR}/Testing/Temporary")
set(PARAVIEW_DATA_ROOT "${PARAVIEW_TEST_OUTPUT_DATA_DIR}")

if (APPLE)
  option(PARAVIEW_DO_UNIX_STYLE_INSTALLS
    "When enabled, install will result in unix-style install rather than an OsX app"
    OFF)
  mark_as_advanced(PARAVIEW_DO_UNIX_STYLE_INSTALLS)
endif()
if (APPLE AND PARAVIEW_DO_UNIX_STYLE_INSTALLS)
  # We are doing a unix-style install i.e. everything will be installed in
  # CMAKE_INSTALL_PREFIX/bin and CMAKE_INSTALL_PREFIX/lib etc. as on other unix
  # platforms. We still need to setup CMAKE_INSTALL_NAME_DIR correctly so that
  # the binaries point to appropriate location for the libraries.

  # 1. Make CMAKE_INSTALL_PREFIX publicly accessible, if it was hidden in
  #    previous pass
  get_property(is_internal CACHE CMAKE_INSTALL_PREFIX PROPERTY TYPE)
  if (is_internal STREQUAL "INTERNAL")
    set (CMAKE_INSTALL_PREFIX ${CACHED_CMAKE_INSTALL_PREFIX} CACHE PATH "Install prefix" FORCE)
  else()
    set (CMAKE_INSTALL_PREFIX ${CACHED_CMAKE_INSTALL_PREFIX} CACHE PATH "Install prefix")
  endif()
  unset(MACOSX_APP_INSTALL_PREFIX CACHE)

  if (PARAVIEW_CUSTOM_INSTALL_NAME_DIR)
    # Allow the user to set a custom install_name directory. This is useful for
    # using ParaView as an SDK where the binaries are not necessarily in the
    # same place as ParaView's binaries.
    set(CMAKE_INSTALL_NAME_DIR "${PARAVIEW_CUSTOM_INSTALL_NAME_DIR}/${VTK_INSTALL_LIBRARY_DIR}")
  else ()
    set(CMAKE_INSTALL_NAME_DIR "@rpath/")
  endif ()

  # ensure that we don't build forwarding executables on apple.
  set(VTK_BUILD_FORWARDING_EXECUTABLES FALSE)
elseif (APPLE AND NOT PARAVIEW_DO_UNIX_STYLE_INSTALLS)
  # If building on apple, we will set cmake rules for ParaView such that "make
  # install" will result in creation of an application bundle by default.
  # The trick we play is as such:
  # 1. We hide the CMAKE_INSTALL_PREFIX variable at force it to a
  #    internal/temporary location. All generic VTK/ParaView install rules
  #    happily go about installing under this CMAKE_INSTALL_PREFIX location.
  # 2. We provide a new vairable MACOSX_APP_INSTALL_PREFIX that user can set to
  #    point where he wants the app bundle to be placed (default is
  #    /Applications).
  # 3. We set CMAKE_INSTALL_NAME_DIR to point to location of runtime libraries
  #    in the app bundle. Thus when the libraries are installed, cmake
  #    automatically cleans up the install_name paths for all shared libs that
  #    we built to point to a location within the app.
  # 4. To make packaging of plugins easier, we install plugins under a directory
  #    named "plugins" in the temporary CMAKE_INSTALL_PREFIX location. This just
  #    a simple trick to avoid having to keep track of plugins we built.
  # 5. Every application that builds an app, then uses the
  #    ParaViewBrandingInstallApp.cmake or something similar to put all the
  #    libraries, plugins, python files etc. within the app bundle itself.
  # 6. Finally, the bundle generated under the temporary location is copied over
  #   to the path specified by MACOSX_APP_INSTALL_PREFIX.
  #
  # In keeping with our "WE INSTALL WHAT WE BUILD" rule, this app bundle is not
  # distributable to others since it does not include Qt, or other external
  # dependencies. For a distributable pacakage, refer to ParaView Super-build
  # instructions.

  # Try to preserve user-specified CMAKE_INSTALL_PREFIX if any.
  get_property(is_internal CACHE CMAKE_INSTALL_PREFIX PROPERTY TYPE)
  if (NOT is_internal STREQUAL "INTERNAL")
    # CMAKE_INSTALL_PREFIX is not internal yet, so it must be avialable to the
    # user, so save it.
    set (CACHED_CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" CACHE INTERNAL "")
  endif()

  set (CMAKE_INSTALL_PREFIX
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/__macos_install
    CACHE INTERNAL "" FORCE)
  set (MACOSX_APP_INSTALL_PREFIX
    "/Applications"
    CACHE PATH
    "Location where the *.app bundle must be installed.")
  set(CMAKE_INSTALL_NAME_DIR "@executable_path/../Libraries")
  set(PV_INSTALL_PLUGIN_DIR "Plugins")

  # ensure that we don't build forwarding executables on apple.
  set(VTK_BUILD_FORWARDING_EXECUTABLES FALSE)
endif()

if (UNIX AND NOT APPLE)
  set(VTK_BUILD_FORWARDING_EXECUTABLES TRUE)
endif()

set(paraview_web_default ON)
if (WIN32)
  include(VTK/CMake/FindPythonModules.cmake)
  find_python_module(win32api have_pywin32)
  set(paraview_web_default "${have_pywin32}")
endif ()
cmake_dependent_option(PARAVIEW_ENABLE_WEB "Enable/Disable web support" "${paraview_web_default}"
  "PARAVIEW_ENABLE_PYTHON" OFF)
mark_as_advanced(PARAVIEW_ENABLE_WEB)

if (PARAVIEW_ENABLE_WEB)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/package.json.in"
    "${CMAKE_CURRENT_BINARY_DIR}/package.json"
    @ONLY)
  # Request vtkParaViewWeb module
  list(APPEND vtkParaViewWeb_REQUEST_BY "Option PARAVIEW_ENABLE_WEB")
endif()

option(PARAVIEW_ENABLE_CATALYST "Enable Catalyst CoProcessing modules" ON)
cmake_dependent_option(PARAVIEW_BUILD_CATALYST_ADAPTORS
  "Build Adaptors for various simulation codes" OFF
  "PARAVIEW_ENABLE_CATALYST" OFF)
mark_as_advanced(PARAVIEW_BUILD_CATALYST_ADAPTORS)

if (PARAVIEW_ENABLE_CATALYST)
  # Request vtkPVCatalyst module.
  list(APPEND vtkPVCatalyst_REQUEST_BY "Option PARAVIEW_ENABLE_CATALYST")

  if (BUILD_TESTING)
    # Request vtkPVCatalystTestDriver module for testing.
    list(APPEND vtkPVCatalystTestDriver_REQUEST_BY "Option PARAVIEW_ENABLE_CATALYST, BUILD_TESTING")
  endif()
  # When Catalyst is enabled, Fortran is optionally needed. Hence we enable
  # Fortran at the top level itself. When individual module called
  # enable_language(...), it failed during first cmake configure but worked o
  # subsequent. enable_language(... OPTIONAL) overcomes that issue altogether.

  if (NOT WIN32)
    # Theoretically, CheckFortran should not be needed, but it
    # enable_language(OPTIONAL) fails with Ninja generator.
    include(CheckFortran)
    if  (CMAKE_Fortran_COMPILER)
      enable_language(Fortran OPTIONAL)
    endif()
  endif()
endif()

# Add ability to freeze Python modules.
cmake_dependent_option(PARAVIEW_FREEZE_PYTHON
  "Freeze Python packages/modules into the application." OFF
  "PARAVIEW_ENABLE_PYTHON;NOT WIN32" OFF)
mark_as_advanced(PARAVIEW_FREEZE_PYTHON)

#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Update state based on chosen/imported options.
#------------------------------------------------------------------------------
set (PARAVIEW_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
if (NOT PARAVIEW_INSTALL_DEVELOPMENT_FILES)
  set (VTK_INSTALL_NO_DEVELOPMENT TRUE)
endif()

# Setup Qt state. We just do a quick top level check for Qt's exisiting here.
# We let individual modules find_package(Qt..) as needed.
if(PARAVIEW_ENABLE_QT_SUPPORT)
  set(PARAVIEW_QT_VERSION "5" CACHE STRING  "Select Qt version to use 4 or 5")
  set_property(CACHE PARAVIEW_QT_VERSION PROPERTY STRINGS 4 5)
  # Force VTK to use the same Qt version as ParaView.
  set(VTK_QT_VERSION "${PARAVIEW_QT_VERSION}" CACHE INTERNAL "")
  set(VTK_BUILD_QT_DESIGNER_PLUGIN OFF CACHE INTERNAL "")
endif()

# Setup testing.
if (BUILD_TESTING)
  set (PARAVIEW_TEST_DIR ${ParaView_BINARY_DIR}/Testing/Temporary)
  make_directory(${PARAVIEW_TEST_DIR})
  enable_testing()
  include(CTest)
endif()

# Setup default state for ParaView module groups.
set (VTK_Group_ParaViewCore ON CACHE BOOL "")
set (VTK_Group_ParaViewRendering ON CACHE BOOL "")

if (PARAVIEW_ENABLE_COMMANDLINE_TOOLS)
  # Request the vtkPVServerManagerApplication module.
  list (APPEND vtkPVServerManagerApplication_REQUEST_BY
    "Option PARAVIEW_ENABLE_COMMANDLINE_TOOLS")
endif()

if (PARAVIEW_BUILD_QT_GUI)
  # Request the pqApplicationComponents module.
  list (APPEND pqApplicationComponents_REQUEST_BY "Option PARAVIEW_BUILD_QT_GUI")

  if (APPLE)
    # Enable high resolution when using Qt5, disable it when using Qt4
    if (${PARAVIEW_QT_VERSION} EQUAL 5)
      set(MAC_HIGH_RESOLUTION_CAPABLE "true")
    else()
      set(MAC_HIGH_RESOLUTION_CAPABLE "false")
    endif()
    file(MAKE_DIRECTORY ${ParaView_CMAKE_BINARY_DIR})
    configure_file(
      "${ParaView_CMAKE_DIR}/MacOSXBundleInfo.plist.in.in"
      "${ParaView_CMAKE_BINARY_DIR}/MacOSXBundleInfo.plist.in"
      @ONLY
    )
  endif()
endif()

#------------------------------------------------------------------------------
# Test to check install tree
#------------------------------------------------------------------------------
if (BUILD_TESTING AND PARAVIEW_INSTALL_DEVELOPMENT_FILES)
  add_test( NAME pv.TestDevelopmentInstall
    COMMAND ${CMAKE_COMMAND}
    -DPARAVIEW_BINARY_DIR:PATH=${CMAKE_BINARY_DIR}
    -DPARAVIEW_INSTALL_DIR:PATH=${CMAKE_INSTALL_PREFIX}
    -DPARAVIEW_SOURCE_DIR:PATH=${CMAKE_SOURCE_DIR}
    -DPARAVIEW_TEST_DIR:PATH=${PARAVIEW_TEST_DIR}
    -DPARAVIEW_VERSION:STRING=${PARAVIEW_VERSION}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CMake/ParaViewTestInstall.cmake)
  set_tests_properties(pv.TestDevelopmentInstall PROPERTIES
    LABELS "PARAVIEW"
    ENVIRONMENT "DESTDIR=${CMAKE_BINARY_DIR}/test-install")
endif()
#------------------------------------------------------------------------------

include_directories(${ParaView_BINARY_DIR})

set (PARAVIEW_MODULE_ROOTS)

#------------------------------------------------------------------------------
# Bring in VTK
#------------------------------------------------------------------------------
# ParaView provides an advanced option that can be used to make ParaView use a
# pre-built VTK. For that, the option must be passed to the first cmake command
# using -D. i.e. to use a separately built VTK, in a clean binary directory, do
# the following:
#   cmake -DUSE_EXTERNAL_VTK:BOOL=ON
# Changing the option after the first configure will have no effect.
if (NOT __paraview_configured AND USE_EXTERNAL_VTK)
  # the __paraview_configured ensures that USE_EXTERNAL_VTK has no effect except
  # in an empty binary dir.
  set (PARAVIEW_USING_EXTERNAL_VTK TRUE CACHE INTERNAL "Using external VTK" FORCE)
endif()
set (__paraview_configured TRUE CACHE INTERNAL
  "ParaView has been configured" FORCE)

if (PARAVIEW_USING_EXTERNAL_VTK)
  find_package(VTK REQUIRED)
  message(STATUS "Using External VTK from \"${VTK_DIR}\"")

  # Update CMAKE_MODULE_PATH to include VTK's cmake files.
  set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                         ${VTK_CMAKE_DIR}
                         ${VTK_MODULES_DIR})

  # Since importing VTK changes the VTK_MODULES_DIR variable to point to the
  # location where VTK's modules are, we update the variable. That way new
  # configuration files generated for modules ParaView adds go in the correct
  # location.
  set (VTK_MODULES_DIR ${PARAVIEW_MODULES_DIR})

else()

  # Update CMAKE_MODULE_PATH to include VTK's cmake files.
  set (VTK_CMAKE_DIR ${ParaView_SOURCE_DIR}/VTK/CMake)
  set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VTK_CMAKE_DIR})

  #----------------------------------------------------------------------------------
  # Set some flags that affect VTK's modules.
  cmake_dependent_option(VTK_NO_PYTHON_THREADS "Disable Python Threads support" ON PARAVIEW_ENABLE_PYTHON OFF)
  cmake_dependent_option(VTK_PYTHON_FULL_THREADSAFE "Ensure all calls to Python using GIL" OFF "NOT VTK_NO_PYTHON_THREADS" OFF)
  mark_as_advanced(VTK_NO_PYTHON_THREADS VTK_PYTHON_FULL_THREADSAFE)

  set(VTK_WRAP_PYTHON ${PARAVIEW_ENABLE_PYTHON}
    CACHE INTERNAL "Should VTK Python wrapping be built?" FORCE)

  # Turn Cosmo and VPIC MPI build flags based on value of PARAVIEW_USE_MPI
  set(VTK_VPIC_USE_MPI ${PARAVIEW_USE_MPI} CACHE BOOL "" FORCE)
  mark_as_advanced(VTK_VPIC_USE_MPI)

  # Change VTK default, since VTK is set up to enable TK when python wrapping is
  # enabled.
  option(VTK_USE_TK "Build VTK with Tk support" OFF)

  # turn of groups that VTK turns on by default. We will enable modules as
  # needed.
  set(VTK_Group_StandAlone OFF CACHE BOOL "" FORCE)
  set(VTK_Group_Rendering OFF CACHE BOOL "" FORCE)

  # set the default backend for PV to be OpenGL
  set(VTK_RENDERING_BACKEND_DEFAULT "OpenGL2")

endif()

#----------------------------------------------------------------------------------
# Import all essential CMake files. Based on whether we are using internal or
# external VTK, these modules will be imported for the appropropriate locations.
include(vtkModuleAPI)
include(vtkModuleMacros)
include(ParaViewMacros)

#----------------------------------------------------------------------------------
# Use consistent compiler flags in ParaView and VTK.
include(vtkDetermineCompilerFlags)

#----------------------------------------------------------------------------------
list (APPEND PARAVIEW_MODULE_ROOTS
             ThirdParty
             Utilities
             ParaViewCore
             Qt
             CoProcessing/Catalyst
             CoProcessing/TestDriver)

add_custom_target(ParaViewDoc)

if (PARAVIEW_ENABLE_WEB)
  list (APPEND PARAVIEW_MODULE_ROOTS Web)
endif()

# Paths to external module roots.
set(PARAVIEW_EXTERNAL_MODULE_ROOTS "" CACHE STRING
  "Semicolon separated absolute paths to external directories containing extra VTK/ParaView modules.")
mark_as_advanced(PARAVIEW_EXTERNAL_MODULE_ROOTS)
include(ParaViewModuleTop)

#----------------------------------------------------------------------------------
# Sanity check: OpenGL2 backend is required for Qt 5.
if(PARAVIEW_BUILD_QT_GUI
    AND PARAVIEW_QT_VERSION VERSION_GREATER "4"
    AND VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  message(SEND_ERROR
    "ParaView requires 'OpenGL2' rendering backend when building with Qt 5. "
    "Please set VTK_RENDERING_BACKEND to 'OpenGL2' (recommended) or build with Qt 4.")
endif()

#----------------------------------------------------------------------------------
# keep VTK cmake variables from polluting the non-advanced space.
mark_as_advanced(
  VTK_EXTRA_COMPILER_WARNINGS
  VTK_Group_Imaging
  VTK_Group_MPI
  VTK_Group_ParaView
  VTK_Group_ParaViewCore
  VTK_Group_ParaViewPython
  VTK_Group_ParaViewQt
  VTK_Group_ParaViewRendering
  VTK_Group_Qt
  VTK_Group_Rendering
  VTK_Group_StandAlone
  VTK_Group_Tk
  VTK_Group_Views
  VTK_Group_Web
  VTK_USE_TK
  VTK_WRAP_JAVA
  VTK_WRAP_TCL)
mark_as_advanced(
  VTK_ANDROID_BUILD
  VTK_IOS_BUILD
  VTK_USE_CXX11_FEATURES
  VTK_USE_LARGE_DATA
  VTK_USE_SYSTEM_GLEW
  VTK_PYTHON_VERSION)
#------------------------------------------------------------------------------
# Based on state of VTK modules, set up some variables that paraview needs to
# compile optional code.

configure_file(
  ${ParaView_SOURCE_DIR}/vtkPVConfig.h.in
  ${ParaView_BINARY_DIR}/vtkPVConfig.h
  @ONLY)

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES ${ParaView_BINARY_DIR}/vtkPVConfig.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
    COMPONENT Development)
endif()

#------------------------------------------------------------------------------
# Client-Server Wrapping for all Modules.
#------------------------------------------------------------------------------
# Wrap all modules ParaView/VTK knows about.
add_subdirectory(Wrapping/ClientServer)

#------------------------------------------------------------------------------
# Package the paraview *.py files.
# This doesn't do the actual Python wrapping of modules. That's managed by VTK.
# This merely builds/installs/packages the ParaView specific *.py files.
#------------------------------------------------------------------------------
add_subdirectory(Wrapping/Python)

#------------------------------------------------------------------------------
# Process modules that need to be add at the end, after all other modules have
# been processed.
if (PARAVIEW_ENABLE_PYTHON)
  add_subdirectory(Utilities/PythonInitializer)
endif()

if (PARAVIEW_ENABLE_PYTHON AND PARAVIEW_ENABLE_CATALYST)
  find_package(PythonLibs REQUIRED)
  vtk_add_module(${CMAKE_CURRENT_SOURCE_DIR}/CoProcessing/PythonCatalyst
                module.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/CoProcessing/PythonCatalyst
                Cxx
                Python)
  list(APPEND VTK_MODULES_ENABLED vtkPVPythonCatalyst)
  # the following variable needs to stay vtk-module
  set(vtk-module vtkPVPythonCatalyst)
  add_subdirectory("${${vtk-module}_SOURCE_DIR}" "${${vtk-module}_BINARY_DIR}")
  set(${vtk-module}_WRAP_PYTHON ON)
  set_property(GLOBAL APPEND PROPERTY VTK_PYTHON_WRAPPED ${vtk-module})

  set(vtkPVPythonCatalyst_INCLUDE_DIRS
    ${ParaView_BINARY_DIR}/CoProcessing/PythonCatalyst/
    ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessing/PythonCatalyst/)

  list(APPEND VTK_PYTHON_MODULES ${vtk-module})
  set(vtkPVPythonCatalyst_HEADERS vtkCPPythonScriptPipeline)
  vtk_add_python_wrapping(${vtk-module} pv_wrapping_sources)
  vtk_add_python_wrapping_library(${vtk-module} pv_wrapping_sources ${vtk-module})

  if (BUILD_TESTING)
    set (_test_module_name "${vtk-module}-Test-Cxx")
    add_subdirectory(
      "${${_test_module_name}_SOURCE_DIR}" "${${_test_module_name}_BINARY_DIR}")
  endif()

  set_property(TARGET vtkPVPythonCatalystPythonD PROPERTY INCLUDE_DIRECTORIES
    ${VTK_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}
    ${vtkPVPythonCatalyst_INCLUDE_DIRS}
    ${ParaView_BINARY_DIR}/CoProcessing/Catalyst)
  set_property(TARGET vtkPVPythonCatalystPython PROPERTY INCLUDE_DIRECTORIES
    ${VTK_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}
    ${vtkPVPythonCatalyst_INCLUDE_DIRS}
    ${ParaView_BINARY_DIR}/CoProcessing/Catalyst)

  # All the ${vtk-module}Python files are installed by VTK/Wrapping/Python/CMakeLists.txt
  # outside the module subsystem. Hence, we need to install the vtkPVPythonCatalystPython
  # library explicitly here (BUG #15085).
  # This code mirrors the code in VTK/Wrapping/Python/CMakeLists.txt
  if (NOT VTK_INSTALL_NO_RUNTIME AND BUILD_SHARED_LIBS AND NOT VTK_INSTALL_NO_LIBRARIES)
    install(TARGETS ${vtk-module}Python
      EXPORT ${VTK_INSTALL_EXPORT_NAME}
      RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_PYTHON_MODULE_DIR}/vtk COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR} COMPONENT Development
      )
  endif()
  unset(vtk-module)
endif()

#------------------------------------------------------------------------------
# Process ParaView Plugins. These are processed similar to VTK modules.
include(ParaViewPluginsMacros)
# This will set two variables:
# PARAVIEW_PLUGINS_ALL -- all available plugins.
# PARAVIEW_PLUGINLIST - list of library/target names for all enable plugins.
pv_process_plugins(
  ${CMAKE_CURRENT_SOURCE_DIR}/Plugins
  ${CMAKE_CURRENT_BINARY_DIR}/Plugins)

if (PARAVIEW_ENABLE_COMMANDLINE_TOOLS)
  add_subdirectory(CommandLineExecutables)
endif()
add_subdirectory(Applications)

#------------------------------------------------------------------------------
# We add a mechanism to incorporate arbitrary install rules into the
# build-process.
foreach(rule_file ${PARAVIEW_EXTRA_INSTALL_RULES_FILE})
  if (EXISTS "${rule_file}")
    message (STATUS "***** Incorporating custom cmake file : ${rule_file} ****")
    include("${rule_file}")
    message (STATUS "*********************************************************")
  endif ()
endforeach()

#------------------------------------------------------------------------------
# Lastly generate the ParaViewConfig.cmake so that other projects can depend on
# ParaView.
# We create two versions of ParaViewConfig.cmake for the build tree and the
# install tree.

# For build tree.
set (PARAVIEW_CONFIG_INSTALLED FALSE)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ParaViewConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/ParaViewConfig.cmake @ONLY)

set (PARAVIEW_CONFIG_INSTALLED TRUE)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ParaViewConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/ParaViewConfig.cmake @ONLY)
configure_file(ParaViewConfigVersion.cmake.in ParaViewConfigVersion.cmake @ONLY)

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/ParaViewConfig.cmake
          ${CMAKE_CURRENT_BINARY_DIR}/ParaViewConfigVersion.cmake
    DESTINATION ${VTK_INSTALL_PACKAGE_DIR}
    COMPONENT Development
  )

  install(
    DIRECTORY "${ParaView_SOURCE_DIR}/CMake/"
    DESTINATION ${VTK_INSTALL_PACKAGE_DIR}
    COMPONENT Development
  )

  #we depend on a couple VTK CMake files that aren't installed but we need
  #installed
  install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/VTK/CMake/TopologicalSort.cmake
          ${CMAKE_CURRENT_SOURCE_DIR}/VTK/CMake/vtkMakeInstantiator.h.in
          ${CMAKE_CURRENT_SOURCE_DIR}/VTK/CMake/vtkMakeInstantiator.cxx.in
    DESTINATION ${VTK_INSTALL_PACKAGE_DIR}
    COMPONENT Development
  )

endif()

#-----------------------------------------------------------------------------
# Export all targets at once from the build tree in their final configuration.
# We export these again so that we add ParaView's targets to the list.
get_property(_vtk_targets GLOBAL PROPERTY VTK_TARGETS)
get_property(_vtk_compiletools_targets GLOBAL PROPERTY VTK_COMPILETOOLS_TARGETS)
set (_vtk_all_targets ${_vtk_targets} ${_vtk_compiletools_targets})
if (_vtk_all_targets)
  list(REMOVE_DUPLICATES _vtk_all_targets)
  export(TARGETS ${_vtk_all_targets} FILE
    ${ParaView_BINARY_DIR}/ParaViewTargets.cmake)
endif()
# Add a virtual target that can be used to build all compile tools.
add_custom_target(pvCompileTools)
if (_vtk_compiletools_targets)
  list(REMOVE_DUPLICATES _vtk_compiletools_targets)
  export(TARGETS ${_vtk_compiletools_targets}
         FILE ${ParaView_BINARY_DIR}/ParaViewCompileToolsConfig.cmake)
  add_dependencies(pvCompileTools
      ${_vtk_compiletools_targets}
      vtkCompileTools)
endif()
unset(_vtk_targets)
unset(_vtk_compiletools_targets)
unset(_vtk_all_targets)

#-----------------------------------------------------------------------------
if (BUILD_EXAMPLES)
        # Don't do parallel make when cross compiling.
  # BuildExamples.cmake builds the examples as a separate project. This ensures
  # that examples can be built by themselves as well as avoiding pollution of
  # the ParaView target space with targets (and other things) from examples.
  include(${CMAKE_CURRENT_SOURCE_DIR}/Examples/BuildExamples.cmake)
  if (BUILD_TESTING AND PARAVIEW_ENABLE_PYTHON AND PARAVIEW_USE_MPI AND PARAVIEW_ENABLE_CATALYST)
    set_property(
      DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
      APPEND
      PROPERTY
      TEST_INCLUDE_FILE "${CMAKE_CURRENT_BINARY_DIR}/CatalystExamples.cmake")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/CatalystExamples.cmake" "subdirs(${CMAKE_CURRENT_BINARY_DIR}/Examples/All/Catalyst/)")
  ENDIF ()
endif()


#-----------------------------------------------------------------------------
if (PARAVIEW_BUILD_CATALYST_ADAPTORS AND NOT WIN32)
  # BuildAdaptors.cmake builds the adaptors as a separate project. We mark is
  # OPTIONAL for Catalyst packages that don't have the adaptors included.
  include(${CMAKE_CURRENT_SOURCE_DIR}/CoProcessing/Adaptors/BuildAdaptors.cmake
          OPTIONAL)
endif()

#-----------------------------------------------------------------------------
# Build doxygen documentation.
if (BUILD_DOCUMENTATION)
  add_subdirectory(Utilities/Doxygen)
  add_subdirectory(Utilities/Sphinx)
  add_subdirectory(Documentation)
endif()

#-----------------------------------------------------------------------------
# Configure the CTestCustom.cmake file for exclusions.
configure_file("${ParaView_CMAKE_DIR}/CTestCustom.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake" @ONLY)

#-----------------------------------------------------------------------------
# Create target to download data from the ParaViewData group.  This must come
# after all tests have been added that reference the group, so we put it last.
ExternalData_Add_Target(ParaViewData)
if(PARAVIEW_DATA_EXCLUDE_FROM_ALL)
  set_property(TARGET ParaViewData PROPERTY EXCLUDE_FROM_ALL 1)
  if(BUILD_TESTING)
    message(WARNING "PARAVIEW_DATA_EXCLUDE_FROM_ALL is ON so test data "
      "(needed because BUILD_TESTING is ON) may not be available "
      "without manually building the 'ParaViewData' target.")
  endif()
  if(TARGET VTKData)
    add_dependencies(ParaViewData VTKData)
  endif()
endif()

set(is_git FALSE)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  set(is_git TRUE)
endif ()

if (NOT WIN32 AND is_git)
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
endif ()
