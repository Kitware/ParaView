# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
# SPDX-License-Identifier: BSD-3-CLAUSE
set(classes
  vtkPythonSelector)
vtk_module_add_module(ParaView::VTKExtensionsExtractionPython
  CLASSES ${classes})
