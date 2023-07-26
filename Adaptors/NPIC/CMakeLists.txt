# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
# SPDX-License-Identifier: BSD-3-CLAUSE

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
