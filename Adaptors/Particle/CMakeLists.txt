# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause

set(classes
  vtkParticlePipeline
)

set(headers
  ParticleAdaptor.h
)

set(sources
  ParticleAdaptor.cxx
)

vtk_module_add_module(ParaView::AdaptorsParticle
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${sources}
)
vtk_module_client_server_exclude()
