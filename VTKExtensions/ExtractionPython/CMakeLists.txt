# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPythonSelector)
vtk_module_add_module(ParaView::VTKExtensionsExtractionPython
  CLASSES ${classes})
