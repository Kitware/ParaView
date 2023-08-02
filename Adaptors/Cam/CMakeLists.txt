# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause

set(headers
  fv_create_data.h
  se_create_data.h
)

set(sources
  fv_create_data.cxx
  se_create_data.cxx
)

set(template_classes
  Grid
)

vtk_module_add_module(ParaView::AdaptorsCam
  HEADERS          ${headers}
  SOURCES          ${sources}
  TEMPLATE_CLASSES ${template_classes}
)
vtk_module_client_server_exclude()
