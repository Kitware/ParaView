# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkAppendRectilinearGrid
  vtkAttributeDataReductionFilter
  vtkCleanArrays
  vtkCompleteArrays
  vtkEnvironmentAnnotationFilter
  vtkMergeBlocks
  vtkMinMax
  vtkPExtractHistogram
  vtkPVExtractHistogram2D
  vtkPVBox
  vtkPVChangeOfBasisHelper
  vtkPVCylinder
  vtkPVMergeTables
  vtkPVMergeTablesMultiBlock
  vtkPVPlane
  vtkPVTransform
  vtkPVRotateAroundOriginTransform
  vtkReductionFilter
  vtkSelectionSerializer)

vtk_module_add_module(ParaView::VTKExtensionsMisc
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/misc_filters.xml
        Resources/misc_utilities.xml)
