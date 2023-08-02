# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause

set(classes
  vtkCTHSource
  vtkCTHDataArray
)

set(headers
  CTHAdaptor.h
)

set(sources
  CTHAdaptor.cxx
)

vtk_module_add_module(ParaView::AdaptorsCTH
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${sources}
)
vtk_module_client_server_exclude()
