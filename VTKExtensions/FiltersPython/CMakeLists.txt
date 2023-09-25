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

find_python_module(numpy numpy_found)
if (numpy_found)
  paraview_add_server_manager_xmls(
    XMLS  Resources/python_numpy_filters.xml)
else ()
  message(STATUS "Numpy could not be found. Some Python filters will be disabled.")
endif()
