# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause

set(classes
  vtkAnnotateAttributeDataFilter
  vtkCDBWriter
  vtkPythonAnnotationFilter
  vtkPythonCalculator
  vtkPythonProgrammableFilter
  )

vtk_module_add_module(ParaView::VTKExtensionsFiltersPython
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/python_filters.xml
        Resources/python_misc.xml
        Resources/python_sources.xml)
