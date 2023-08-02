# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause

set(headers
  NPICAdaptor.h
)

set(sources
  NPICAdaptor.cxx
)

vtk_module_add_module(ParaView::AdaptorsNPIC
  HEADERS ${headers}
  SOURCES ${sources}
)
vtk_module_client_server_exclude()
