# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
if (NOT TARGET ParaView::ProcessXML)
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

  vtk_module_add_executable(ParaView::ProcessXML
    DEVELOPMENT
    ProcessXML.cxx)
endif ()
