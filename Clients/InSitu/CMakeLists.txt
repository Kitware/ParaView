# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkInSituInitializationHelper
  vtkInSituPipeline
  vtkInSituPipelineIO
  vtkInSituPipelinePython
  )

vtk_module_add_module(ParaView::InSitu
  CLASSES ${classes})
vtk_module_client_server_exclude()

if (PARAVIEW_ENABLE_CATALYST)
  if (NOT TARGET VTK::IOCatalystConduit)
    message(FATAL_ERROR "Cannot build Catalyst implementation without VTK::IOCatalystConduit module enabled")
  endif ()

  vtk_module_find_package(PACKAGE catalyst VERSION 2.0 COMPONENTS SDK)
  set_property(GLOBAL PROPERTY vtk_catalyst_dir_ClientsInSitu "${catalyst_DIR}")

  add_subdirectory(catalyst)

  vtk_module_link(ParaView::InSitu
    PRIVATE
      catalyst::catalyst)
endif ()
