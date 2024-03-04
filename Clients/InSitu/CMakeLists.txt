# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkInSituInitializationHelper
  vtkInSituPythonConduitHelper
  vtkInSituPipeline
  vtkInSituPipelineIO
  vtkInSituPipelinePython
  )

vtk_module_add_module(ParaView::InSitu
  CLASSES ${classes})
vtk_module_client_server_exclude()

if (PARAVIEW_ENABLE_CATALYST AND TARGET VTK::catalyst)
  add_subdirectory(catalyst)
endif ()
