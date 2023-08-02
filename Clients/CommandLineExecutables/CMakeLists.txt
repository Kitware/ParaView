# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause

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
