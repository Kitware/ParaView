# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause

enable_language(Fortran)
include(FortranCInterface)

FortranCInterface_HEADER(PhastaAdaptorAPIMangling.h SYMBOLS
  createpointsandallocatecells insertblockofcells addfields)

set(headers
  PhastaAdaptor.h
)

set(sources
  PhastaAdaptor.cxx
)

vtk_module_add_module(ParaView::AdaptorsPhasta
  HEADERS ${headers}
  SOURCES ${sources}
)
vtk_module_client_server_exclude()
