# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(headers
  vtkParaViewDeprecation.h)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkPVVersion.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPVVersion.h"
  @ONLY)
list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPVVersion.h")

vtk_module_add_module(ParaView::Versioning
  HEADERS ${headers}
  HEADER_ONLY)
