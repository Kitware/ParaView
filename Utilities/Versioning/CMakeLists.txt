# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(headers
  vtkParaViewDeprecation.h)

if (PARAVIEW_VERSION_PATCH LESS "20000101")
  set(PARAVIEW_VERSION_EPOCH "${PARAVIEW_VERSION_PATCH}")
else ()
  set(PARAVIEW_VERSION_EPOCH "20230101")
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkPVVersionQuick.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPVVersionQuick.h"
  @ONLY)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkPVVersion.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPVVersion.h"
  @ONLY)
list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPVVersion.h"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPVVersionQuick.h")

vtk_module_add_module(ParaView::Versioning
  HEADERS ${headers}
  HEADER_ONLY)
