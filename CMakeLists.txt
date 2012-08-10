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
cmake_minimum_required(VERSION 2.8.8 FATAL_ERROR)
project(ParaView)

# Disallow in-source build
if ("${ParaView_SOURCE_DIR}"  STREQUAL "${ParaView_BINARY_DIR}")
  message(FATAL_ERROR 
    "ParaView requires an out of source Build. "
    "Please create a separate binary directory and run CMake there.")
endif()

#------------------------------------------------------------------------------
# Setup CMake Environment
#------------------------------------------------------------------------------
# Setup cmake policies.
# TODO: We need to verify that these are still applicable after
# VTK-modularization changes.
foreach(policy CMP0012 CMP0013 CMP0014)
  if(POLICY ${policy})
    CMAKE_POLICY(SET ${policy} NEW)
  endif()
endforeach()

foreach(policy CMP0017)
  IF(POLICY ${policy})
    CMAKE_POLICY(SET ${policy} OLD)
  endif()
endforeach()

set (ParaView_CMAKE_DIR "${ParaView_SOURCE_DIR}/CMake")
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ParaView_CMAKE_DIR})


# Use the new version of the variable names for output of build process.
# These are consistent with what VTK sets.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if(UNIX)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
else()
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

include(Utilities/Git/Git.cmake)
include(ParaViewDetermineVersion)
include(CMakeDependentOption)
include(ParaViewMacros)
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Setup ParaView Environment
#------------------------------------------------------------------------------
# Determine ParaView Source Version
set (PARAVIEW_VERSION_MAJOR 3)
set (PARAVIEW_VERSION_MINOR 14)
set (PARAVIEW_VERSION_PATCH 1)
set (PARAVIEW_VERSION_PATCH_EXTRA "enhanced")
set (PARAVIEW_VERSION "3.14")
set (PARAVIEW_VERSION_FULL "3.14.1-enhanced")

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

#set (VTK_INSTALL_NO_LIBRARIES TRUE)
#set (VTK_INSTALL_NO_DEVELOPMENT TRUE)

# Disable installing of the Qt Designer plugin. There's no need for it in
# ParaView install rules.
set (VTK_INSTALL_NO_QT_PLUGIN TRUE)

# ParaView install the vtk python modules specifically to appropriate locations.
set (VTK_INSTALL_NO_PYTHON TRUE)

# for temporary backwards compatibility.
set (PV_INSTALL_BIN_DIR ${VTK_INSTALL_RUNTIME_DIR})
set (PV_INSTALL_LIB_DIR ${VTK_INSTALL_ARCHIVE_DIR})
set (PV_INSTALL_EXPORT_NAME ${VTK_INSTALL_EXPORT_NAME})

# Setting this ensures that "make install" will leave rpaths to external
# libraries (not part of the build-tree e.g. Qt, ffmpeg, etc.) intact on
# "make install". This ensures that one can install a version of ParaView on the
# build machine without any issues. If this not desired, simply comment the
# following line and "make install" will strip all rpaths, which is default
# behavior.
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Define ParaView specific options.
#------------------------------------------------------------------------------
option(BUILD_TESTING "Build ParaView Testing" ON)
option(BUILD_EXAMPLES "Build ParaView examples" OFF)
option(BUILD_SHARED_LIBS "Build ParaView using shared libraries" ON)
option(PARAVIEW_BUILD_QT_GUI "Enable ParaView Qt-based client" ON)
option(PARAVIEW_USE_MPI "Enable MPI support for parallel computing" OFF)
option(PARAVIEW_USE_PISTON "Build ParaView with Piston GPGPU filters" OFF)
option(PARAVIEW_USE_VISITBRIDGE "Build ParaView with VisIt readers." OFF)

if (UNIX AND NOT APPLE)
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
mark_as_advanced(PARAVIEW_USE_ICE_T
  PARAVIEW_USE_MPI_SSEND)

cmake_dependent_option(PARAVIEW_ENABLE_QT_SUPPORT
  "Build ParaView with Qt support (without GUI)" OFF
  "NOT PARAVIEW_BUILD_QT_GUI" ON)

option(PARAVIEW_ENABLE_PYTHON "Enable/Disable Python scripting support" OFF)

# Is this a 32 bit or 64bit build. Display this in about dialog.
if ("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
  set(PARAVIEW_BUILD_ARCHITECTURE "64")
else()
  set(PARAVIEW_BUILD_ARCHITECTURE "32")
endif()

FIND_PATH(PARAVIEW_DATA_ROOT ParaViewData.readme
  ${ParaView_SOURCE_DIR}/../ParaViewData
  ${ParaView_BINARY_DIR}/../ParaViewData
  $ENV{PARAVIEW_DATA_ROOT})

# CoProcessing related options.
# I am keeping them as they were, we can propobably minimize these as well.
option(PARAVIEW_ENABLE_COPROCESSING "Enable/Disable the CoProcessing library." ON)
cmake_dependent_option(BUILD_FORTRAN_COPROCESSING_ADAPTORS
  "Build a coprocessing adaptor that can be called from Fortran or C" OFF
  "PARAVIEW_ENABLE_COPROCESSING" OFF)
cmake_dependent_option(BUILD_PARTICLE_COPROCESSING_ADAPTORS
  "Build a coprocessing adaptor that renders particle systems" OFF
  "PARAVIEW_ENABLE_COPROCESSING" OFF)
mark_as_advanced(BUILD_PARTICLE_COPROCESSING_ADAPTORS)
mark_as_advanced(BUILD_FORTRAN_COPROCESSING_ADAPTORS)

#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Update state based on chosen/imported options.
#------------------------------------------------------------------------------
set (PARAVIEW_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
if (NOT PARAVIEW_INSTALL_DEVELOPMENT_FILES)
  set (VTK_INSTALL_NO_DEVELOPMENT TRUE)
endif()

# Setup Qt state
if (PARAVIEW_ENABLE_QT_SUPPORT)
  # need to set up Qt stuff here because there are Qt dependencies before
  # ParaView requires this minimum version of Qt, and let's do it here before
  # our first call to FindQt4.cmake
  set(QT_MIN_VERSION "4.7.0")
  set(QT_OFFICIAL_VERSION "4.8")
  set(QT_REQUIRED TRUE)
  find_package(Qt4)
  if (NOT QT4_FOUND)
    message(SEND_ERROR "Qt ${QT_MIN_VERSION} or greater not found.  "
      "Please check the QT_QMAKE_EXECUTABLE variable.")
  else()
    # check is Qtversion  is 4.8.*. If so, we are good. Otherwise we will post a
    # warning of versions (>= 4.7 && < 4.8). However we report errors for any
    # version less than 4.7
    string(REGEX MATCH "^4\\.[8]\\.[0-9]+" qt_version_match "${QTVERSION}")
    if (NOT qt_version_match)
      string(REGEX MATCH "^4\\.[0-6]+\\.[0-9]+" qt_version46_x_tmp "${QTVERSION}")
      if (qt_version46_x_tmp)
        message(SEND_ERROR "Qt ${QTVERSION} not supported. "
          "Please use ${QT_OFFICIAL_VERSION} (you may need to clean your dirtied cache)."
          "Minium required version is ${QT_MIN_VERSION}.")
      else (qt_version46_x_tmp)
        message(WARNING "Warning: You are using Qt ${QTVERSION}. "
          "Officially supported version is Qt ${QT_OFFICIAL_VERSION}")
      endif (qt_version46_x_tmp)
    endif (NOT qt_version_match)
  endif (NOT QT4_FOUND)
endif()
SET(PARAVIEW_QT_QMAKE_EXECUTABLE ${QT_QMAKE_EXECUTABLE})

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
set (VTK_Group_ParaViewQt ${PARAVIEW_BUILD_QT_GUI} CACHE BOOL "" FORCE)
set (VTK_Group_ParaViewPython ${PARAVIEW_ENABLE_PYTHON} CACHE BOOL "" FORCE)
set (VTK_Group_CoProcessing ${PARAVIEW_ENABLE_COPROCESSING} CACHE BOOL "" FORCE)

# Turn Cosmo and VPIC MPI build flags based on value of PARAVIEW_USE_MPI
set(VTK_COSMO_USE_MPI ${PARAVIEW_USE_MPI} CACHE BOOL "" FORCE)
mark_as_advanced(VTK_COSMOS_USE_MPI)
set(VTK_VPIC_USE_MPI ${PARAVIEW_USE_MPI} CACHE BOOL "" FORCE)
mark_as_advanced(VTK_VPIC_USE_MPI)

# this bring in states for additional VTK modules that ParaView GUI needs to
# work.
include(VTKModules)

#------------------------------------------------------------------------------

include_directories(${ParaView_BINARY_DIR})

#tell paraview about the visitbridge classes that it has to wrap and expose
#in the GUI
macro(PARAVIEW_INCLUDE_SERVERMANAGER_RESOURCES RESOURCES)
  set(PARAVIEW_EXTRA_SERVERMANAGER_RESOURCES
    ${PARAVIEW_EXTRA_SERVERMANAGER_RESOURCES} ${RESOURCES})
endmacro(PARAVIEW_INCLUDE_SERVERMANAGER_RESOURCES RESOURCES)

set(VISITBRIDGE_READERS_XML_FILE
  "${ParaView_SOURCE_DIR}/Utilities/VisItBridge/databases/visit_readers.xml")
paraview_include_servermanager_resources(${VISITBRIDGE_READERS_XML_FILE})

set(PARAVIEW_EXTRA_GUI_RESOURCES)
macro(PARAVIEW_INCLUDE_GUI_RESOURCES RESOURCES)
  set(PARAVIEW_EXTRA_GUI_RESOURCES ${PARAVIEW_EXTRA_GUI_RESOURCES} ${RESOURCES})
endmacro(PARAVIEW_INCLUDE_GUI_RESOURCES RESOURCES)

set(VISITBRIDGE_READERS_GUI_XML_FILE
  "${ParaView_SOURCE_DIR}/Utilities/VisItBridge/databases/visit_readers_gui.xml")
paraview_include_gui_resources(${VISITBRIDGE_READERS_GUI_XML_FILE})

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
  set (use_external_vtk TRUE CACHE INTERNAL "Using external VTK" FORCE)
endif()
set (__paraview_configured TRUE CACHE INTERNAL
  "ParaView has been configured" FORCE)


if (use_external_vtk)
  find_package(VTK REQUIRED ${required_vtk_modules})

  # TODO: Report errors if key VTK build settings don't match ParaView's. For
  # now we will assume they do since this setting is only meant for advanced
  # developers.

  # When VTK is brought in externally, there are several variables that VTK sets
  # that are no longer set.
  set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VTK_CMAKE_DIR})

else()
  #setup VTK environment and add vtk-subdir
  set (VTK_CMAKE_DIR ${ParaView_SOURCE_DIR}/VTK/CMake)
  set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VTK_CMAKE_DIR})
  include(vtkModuleAPI)
  include(vtkModuleMacros)

  # Change VTK default, since VTK is set up to enable TK when python wrapping is
  # enabled.
  option(VTK_USE_TK "Build VTK with Tk support" OFF)

  # turn of groups that VTK turns on by default. We will enable modules as
  # needed.
  set(VTK_Group_StandAlone OFF CACHE BOOL "" FORCE)
  mark_as_advanced(VTK_Group_StandAlone)
  set(VTK_Group_Rendering OFF CACHE BOOL "" FORCE)
  mark_as_advanced(VTK_Group_Rendering)

  set(VTK_WRAP_PYTHON ${PARAVIEW_ENABLE_PYTHON}
    CACHE INTERNAL "Should VTK Python wrapping be built?" FORCE)

  # FIXME: How do we handle this when using external VTK? We can't and hence we
  # need a better mechanism.
  set (VTK_NO_PYTHON_THREADS 1 CACHE INTERNAL "Disbale Python Threads support" FORCE)

  # turn some module ON to avoid warnings from VTK.
  # set(Module_vtkCommonExecutionModel  ON CACHE INTERNAL "" FORCE)

  set (ParaViewModulesDirs
    Utilities/ProcessXML
    ThirdParty/protobuf
    ThirdParty/QtTesting
    Utilities/VisItBridge/Library
    Utilities/VisItBridge/databases
    ParaViewCore/ClientServerStream
    ParaViewCore/PythonSupport
    ParaViewCore/Common
    ParaViewCore/VTKExtensions/Core
    ParaViewCore/VTKExtensions/Rendering
    ParaViewCore/VTKExtensions/Default
    ParaViewCore/ClientServerCore/Core
    ParaViewCore/ClientServerCore/Rendering
    ParaViewCore/ClientServerCore/Default
    ParaViewCore/ServerImplementation/Core
    ParaViewCore/ServerImplementation/Rendering
    ParaViewCore/ServerImplementation/Default
    ParaViewCore/ServerManager/Core
    ParaViewCore/ServerManager/Rendering
    ParaViewCore/ServerManager/Default
    ParaViewCore/ServerManager/SMApplication

    Qt/Widgets
    Qt/Core
    Qt/Components
    Qt/Python
    Qt/ApplicationComponents
  )

  if (PARAVIEW_USE_ICE_T)
    list(APPEND ParaViewModulesDirs
      ThirdParty/IceT)
  endif()

  if (PARAVIEW_ENABLE_COPROCESSING)
    list (APPEND ParaViewModulesDirs
      CoProcessing/Core
      CoProcessing/CoProcessor)
  endif()

  if (BUILD_FORTRAN_COPROCESSING_ADAPTORS)
    list (APPEND ParaViewModulesDirs
      CoProcessing/Adaptors/FortranAdaptors)
  endif()

  if (BUILD_PARTICLE_COPROCESSING_ADAPTORS)
    list (APPEND ParaViewModulesDirs
      CoProcessing/Adaptors/ParticleAdaptor)
  endif()

  if (BUILD_TESTING)
    list (APPEND  ParaViewModulesDirs
    Utilities/TestDriver)
  endif()

  foreach(dir ${ParaViewModulesDirs})
    vtk_add_to_module_search_path(
    "${ParaView_SOURCE_DIR}/${dir}"
    "${ParaView_BINARY_DIR}/${dir}")
  endforeach()

  enable_required_modules()

  # include VTK
  add_subdirectory(VTK)

  hide_enabled_modules()

  # keep VTK cmake variables from polluting the non-advanced space.
  mark_as_advanced(
    VTK_EXTRA_COMPILER_WARNINGS
    VTK_Group_CoProcessing
    VTK_Group_Imaging
    VTK_Group_MPI
    VTK_Group_ParaView
    VTK_Group_ParaViewCore
    VTK_Group_ParaViewPython
    VTK_Group_ParaViewQt
    VTK_Group_ParaViewRendering
    VTK_Group_Qt
    VTK_Group_Views
    VTK_USE_TK
    VTK_WRAP_JAVA
    VTK_WRAP_TCL)

  include(${ParaView_BINARY_DIR}/VTK/VTKConfig.cmake)
endif()

#------------------------------------------------------------------------------
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
add_subdirectory(Wrapping/ClientServer)

#------------------------------------------------------------------------------
# Package the paraview *.py files.
# This doesn't do the actual Python wrapping of modules. That's managed by VTK.
# This merely builds/installs/packages the ParaView specific *.py files.
#------------------------------------------------------------------------------
add_subdirectory(Wrapping/Python)

#------------------------------------------------------------------------------
if (PARAVIEW_BUILD_QT_GUI)
  # Don't build documentation is Qt GUI is not being built.
  add_subdirectory(Documentation)
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
  
add_subdirectory(CommandLineExecutables)
add_subdirectory(Applications)

#------------------------------------------------------------------------------
# We add a mechanism to incorporate arbitrary install rules into the
# build-process.
foreach(rule_file ${PARAVIEW_EXTRA_INSTALL_RULES_FILE})
  if (EXISTS "${rule_file}")
    message (STATUS "***** Incorporating custom cmake file : ${rule_file} ****")
    include("${rule_file}")
    message (STATUS "*********************************************************")
  endif (EXISTS "${rule_file}")
endforeach(rule_file ${PARAVIEW_EXTRA_INSTALL_RULES_FILE})

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
  # BuildExamples.cmake builds the examples as a separate project. This ensures
  # that examples can be built by themselves as well as avoiding pollution of
  # the ParaView target space with targets (and other things) from examples.
  include(${CMAKE_CURRENT_SOURCE_DIR}/Examples/BuildExamples.cmake)
endif()
