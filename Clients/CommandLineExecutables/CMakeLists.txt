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

# TODO: Restore Cray ATP support via a new module.
# TODO: Support static plugins.

# Set up rpaths
set(CMAKE_BUILD_RPATH_USE_ORIGIN 1)
if (UNIX AND NOT APPLE)
  file(RELATIVE_PATH _paraview_client_relpath
    "/prefix/${CMAKE_INSTALL_BINDIR}"
    "/prefix/${CMAKE_INSTALL_LIBDIR}")
  set(_paraview_client_origin_rpath
    "$ORIGIN/${_paraview_client_relpath}")

  list(APPEND CMAKE_INSTALL_RPATH
    "${_paraview_client_origin_rpath}")
endif()

# A plugin configuration file is required if either the GUI
# application is not built (the configuration file is provided
# there if so) or we're on macOS where the configuration file is
# then stored under the bundle for the application. Instead,
# provide our own configuration file instead.
if (APPLE OR NOT PARAVIEW_USE_QT)
  paraview_plugin_write_conf(
    NAME "paraview"
    PLUGINS_TARGETS ParaView::paraview_plugins
    BUILD_DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INSTALL_DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT "runtime")
endif ()

include("${CMAKE_CURRENT_SOURCE_DIR}/ParaViewCommandLineExecutables.cmake")

set(paraview_tools)
foreach (exe IN ITEMS pvserver pvdataserver pvrenderserver)
  paraview_add_executable("${exe}" "${exe}.cxx")
  list(APPEND paraview_tools
    "${exe}")
endforeach ()

if (PARAVIEW_USE_PYTHON)
  foreach (exe IN ITEMS pvbatch pvpython)
    paraview_add_executable("${exe}" "${exe}.cxx")
    list(APPEND paraview_tools
      "${exe}")
  endforeach ()
endif ()

if (paraview_will_be_built)
  list(APPEND paraview_tools
    paraview)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/paraview-config.in"
  "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/paraview-config"
  @ONLY)
install(
  PROGRAMS    "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/paraview-config"
  DESTINATION "${CMAKE_INSTALL_BINDIR}"
  COMPONENT   development)

if (PARAVIEW_INSTALL_DEVELOPMENT_FILES)
  export(
    EXPORT      ParaViewTools
    NAMESPACE   ParaView::
    FILE        "${CMAKE_BINARY_DIR}/${paraview_cmake_destination}/ParaViewTools-targets.cmake")
  install(
    EXPORT      ParaViewTools
    NAMESPACE   ParaView::
    FILE        "ParaViewTools-targets.cmake"
    DESTINATION "${paraview_cmake_destination}"
    COMPONENT   "development")
endif ()

# obsolete in ParaView 5.9
vtk_obsolete_setting(PARAVIEW_BUILD_MESA_LAUNCHER)
